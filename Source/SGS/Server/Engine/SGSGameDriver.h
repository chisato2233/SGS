#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptInterface.h"
#include "Shared/Core/SGSError.h"
#include "Shared/Core/SGSTypes.h"
#include "Shared/Cards/SGSDeckTypes.h"
#include "Server/Commands/SGSCommandRouter.h"
#include "Server/Effects/SGSEffectPipeline.h"
#include "Server/Engine/SGSGameEvents.h"
#include "Server/Rules/SGSRuleRegistry.h"
#include "Server/Rules/SGSResolutionStack.h"
#include "Server/Timing/SGSActiveEffectTimeline.h"
#include "Shared/Decisions/SGSDecisionAgent.h"
#include "SGSGameDriver.generated.h"

class USGSGameContext;
class USGSCard;
class FSGSDriverRuleRuntime;
struct FSGSRuleEventPayload;

struct SGS_API FSGSGameStartConfig
{
	int32 RandomSeed = 0;
	TArray<FSGSDeckCardSpec> InitialDeck;
	bool bShuffleInitialDeck = true;
	int32 StartingHandSize = 4;
	int32 MaxTurns = 8;
	TMap<int32, int32> InitialHealthBySeat;
};

// 服务器权威的对局驱动器：推进回合/阶段、在出牌阶段向决策代理请求动作。
//
// 设计要点（见 ProjectBrief 第 3 节 / Plan 0002）：
// - 非阻塞：等待决策时挂起而非阻塞游戏线程；应答回调到达后从挂起点恢复。
// - 蹦床(trampoline)推进：同步完成的阶段在一个 Pump 循环内连续推进，避免深递归与定时器
//   （结算不依赖 wallclock，见 Rulers 硬约束 #5）。
// - 单次决策用 RequestId 配对，丢弃过期/重复应答。
UCLASS()
class SGS_API USGSGameDriver : public UObject
{
	GENERATED_BODY()

public:
	// 服务器：按给定决策代理（每个代理对应一个座位）开始一局。RandomSeed 用于可复现洗牌。
	void StartGame(const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents, int32 RandomSeed = 0);
	void StartGame(const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents, const FSGSGameStartConfig& Config);

	// 事件总线：观察者在此订阅对局生命周期事件。
	FSGSOnGameEvent& OnGameEvent() { return GameEventDelegate; }

	USGSGameContext* GetContext() const { return Context; }
	int32 GetCurrentSeatIndex() const { return CurrentSeatIndex; }
	FSGSPhase GetCurrentPhase() const { return CurrentPhase; }

	bool IsGameOver() const { return bGameOver; }
	int32 GetTurnsPlayed() const { return TurnsPlayed; }
	const TArray<FSGSCommandLogEntry>& GetCommandLog() const { return CommandRouter.GetLogEntries(); }
	const FSGSReplayLog& GetReplayLog() const { return ReplayLog; }

private:
	// 蹦床循环：连续推进所有「同步完成」的阶段，遇到挂起（等待决策）或对局结束即停。
	void Pump();

	// 进入当前阶段：出牌阶段会发起异步决策并挂起；其余阶段无内容，立即推进。
	void EnterCurrentPhase();

	// 结束当前阶段并推进到下一阶段 / 下一回合 / 结束对局。
	void AdvanceAfterPhase();

	void BeginTurn();
	void EndGame();
	void Broadcast(FSGSGameEvent Event);

	// 出牌阶段动作的应答回调（可能同步或跨帧触发）。
	void OnPlayActionDecided(const FSGSPlayPhaseDecision& Decision);
	void OnResponseActionDecided(const FSGSResponseDecision& Decision);
	bool ResolveCommandThroughRules(const FSGSCommand& Command);
	TSGSResult<FSGSCommand> SubmitCommandWithFallback(
		const FSGSCommand& Command,
		const FSGSCommandExecutionContext& ExecutionContext);
	FSGSCommandExecutionContext MakeCommandExecutionContext() const;
	FSGSCommand MakeFallbackPassCommand(FName Reason) const;
	FSGSEffectContext MakeEffectContext(FSGSCommandId CommandId = FSGSCommandId());
	FSGSTimingPoint MakeCurrentTimingPoint(FName Step);
	void BuildInitialDeck(const TArray<FSGSDeckCardSpec>& InitialDeck, bool bShuffle);
	FSGSPlayPhaseRequest MakePlayPhaseRequest();
	FSGSResponseRequest MakeResponseRequest(int32 SeatIndex, FName WindowName, FName RequiredCardName, int32 EffectSourceSeat, int32 EffectTargetSeat);
	void DeferResponseRequest(const FSGSResponseRequest& Request, const TScriptInterface<ISGSDecisionAgent>& Agent);
	void DispatchDeferredResponseRequest();
	FSGSStatus FinishCurrentResolution(FName Reason = FName(TEXT("SGS.Resolution.Complete")));
	FSGSStatus ResumeResolutionParentAfterChild(const FSGSResolutionFrame& CompletedFrame);
	bool OpenNextDyingPeachResponseWindow(FSGSResolutionFrame& DyingFrame);
	FSGSStatus ContinueDyingPeachFrame(FSGSResolutionFrame& DyingFrame);
	void ClearDeferredResponseRequest();
	FSGSStatus PublishTimingEvent(const FSGSRuleEventPayload& Payload);
	void ExecuteDrawPhaseThroughPipeline();
	FSGSStatus RunEffectStep(FSGSEffectStep Step, FSGSCommandId CommandId = FSGSCommandId());
	void SyncReplayLog();

	static FSGSPhase NextPhase(FSGSPhase Phase);
	FSGSCommandId AllocateCommandId();

	UPROPERTY()
	TObjectPtr<USGSGameContext> Context;

	int32 CurrentSeatIndex = INDEX_NONE;
	FSGSPhase CurrentPhase = SGSGameplayTags::Phase_None.GetTag();
	int32 TurnsPlayed = 0;
	int32 PendingRequestId = 0;
	FSGSCommandId PendingCommandId;
	int32 NextCommandIdValue = 0;
	int64 NextTimingSequence = 0;
	FSGSResponseRequest DeferredResponseRequest;
	TScriptInterface<ISGSDecisionAgent> DeferredResponseAgent;
	bool bHasDeferredResponseRequest = false;
	int32 CurrentMaxTurns = 8;
	int32 CurrentStartingHandSize = 4;

	bool bGameOver = false;
	bool bWaitingForDecision = false;

	// 防止应答同步回调时重入 Pump（AI 立即应答的常见情形）。
	bool bPumping = false;

	FSGSOnGameEvent GameEventDelegate;
	FSGSCommandRouter CommandRouter;
	FSGSRuleRegistry RuleRegistry;
	FSGSResolutionStack ResolutionStack;
	FSGSEffectPipeline EffectPipeline;
	FSGSActiveEffectTimeline ActiveEffectTimeline;
	FSGSReplayLog ReplayLog;

	// 摸牌阶段固定摸牌数（标准规则）。
	static constexpr int32 DrawCountPerTurn = 2;

	friend class FSGSDriverRuleRuntime;
};
