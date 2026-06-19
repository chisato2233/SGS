#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptInterface.h"
#include "Core/SGSTypes.h"
#include "Logic/Engine/SGSGameEvents.h"
#include "Logic/Players/SGSDecisionAgent.h"
#include "SGSGameDriver.generated.h"

class USGSGameContext;

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

	// 事件总线：观察者在此订阅对局生命周期事件。
	FSGSOnGameEvent& OnGameEvent() { return GameEventDelegate; }

	USGSGameContext* GetContext() const { return Context; }

	bool IsGameOver() const { return bGameOver; }
	int32 GetTurnsPlayed() const { return TurnsPlayed; }

private:
	// 蹦床循环：连续推进所有「同步完成」的阶段，遇到挂起（等待决策）或对局结束即停。
	void Pump();

	// 进入当前阶段：出牌阶段会发起异步决策并挂起；其余阶段无内容，立即推进。
	void EnterCurrentPhase();

	// 结束当前阶段并推进到下一阶段 / 下一回合 / 结束对局。
	void AdvanceAfterPhase();

	void BeginTurn();
	void EndGame();
	void Broadcast(ESGSGameEvent Event);

	// 出牌阶段动作的应答回调（可能同步或跨帧触发）。
	void OnPlayActionDecided(const FSGSPlayPhaseDecision& Decision);

	static ESGSPhase NextPhase(ESGSPhase Phase);

	UPROPERTY()
	TObjectPtr<USGSGameContext> Context;

	int32 CurrentSeatIndex = INDEX_NONE;
	ESGSPhase CurrentPhase = ESGSPhase::None;
	int32 TurnsPlayed = 0;
	int32 PendingRequestId = 0;

	bool bGameOver = false;
	bool bWaitingForDecision = false;

	// 防止应答同步回调时重入 Pump（AI 立即应答的常见情形）。
	bool bPumping = false;

	FSGSOnGameEvent GameEventDelegate;

	// 骨架期终止条件占位：跑满这么多个回合即结束。胜负条件实现后替换（见 Plan 0002 路线图）。
	static constexpr int32 MaxTurnsForSkeleton = 8;

	// 摸牌阶段固定摸牌数（标准规则）。
	static constexpr int32 DrawCountPerTurn = 2;

	// 起始手牌数（标准规则）。
	static constexpr int32 StartingHandSize = 4;
};
