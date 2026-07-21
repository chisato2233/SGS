#pragma once

// Server-only rule layer: validated Commands become typed RuleInvocation, then
// Rules enqueue Effects or open controlled response windows through ISGSRuleRuntime.

#include "CoreMinimal.h"
#include "Shared/Commands/SGSCommand.h"
#include "Shared/Core/SGSError.h"
#include "Shared/Decisions/SGSDecisionTypes.h"
#include "Server/Effects/SGSEffectPipeline.h"
#include "Server/Rules/Core/SGSRuleDescriptor.h"
#include "Server/Rules/Core/SGSRuleInvocation.h"
#include "Server/Rules/Resolution/SGSResolutionStack.h"

class USGSCard;
class USGSGameContext;
class FSGSRuleRegistry;
struct FSGSCommandExecutionContext;

struct SGS_API FSGSRuleQueryContext
{
	const USGSGameContext* GameContext = nullptr;
	const FSGSActiveEffectTimeline* ActiveEffects = nullptr;
	const FSGSRuleRegistry* RuleRegistry = nullptr;
	FSGSPhase Phase = SGSGameplayTags::Phase_None.GetTag();
	int32 ActorSeat = INDEX_NONE;
	FName WindowName = NAME_None;
	FName RequiredCardName = NAME_None;
	TConstArrayView<FName> AcceptedCardNames;
	bool bArmorIgnored = false;
};

struct SGS_API FSGSNumericRuleQuery
{
	FName QueryName = NAME_None;
	int32 ActorSeat = INDEX_NONE;
	int32 TargetSeat = INDEX_NONE;
	FName CardName = NAME_None;
	int32 BaseValue = 0;
	int32 Value = 0;
};

struct SGS_API FSGSSkillOptionQuery
{
	FName QueryName = NAME_None;
	int32 ActorSeat = INDEX_NONE;
	FName WindowName = NAME_None;
	FName RequiredCardName = NAME_None;
	TArray<FName> AcceptedCardNames;
	TArray<FSGSDecisionSkillOption> Options;
};

namespace SGSRuleQueries
{
	inline FName SlashUseLimit() { return FName(TEXT("SGS.RuleQuery.SlashUseLimit")); }
	inline FName SlashTargetDistance() { return FName(TEXT("SGS.RuleQuery.SlashTargetDistance")); }
	inline FName SlashTargetCount() { return FName(TEXT("SGS.RuleQuery.SlashTargetCount")); }
	inline FName RequiredResponseCount() { return FName(TEXT("SGS.RuleQuery.RequiredResponseCount")); }
	inline FName PeachHealAmount() { return FName(TEXT("SGS.RuleQuery.PeachHealAmount")); }
	inline FName PlaySkillOptions() { return FName(TEXT("SGS.RuleQuery.PlaySkillOptions")); }
	inline FName ResponseSkillOptions() { return FName(TEXT("SGS.RuleQuery.ResponseSkillOptions")); }
}

struct SGS_API FSGSRuleResponseWindowSpec
{
	int32 SeatIndex = INDEX_NONE;
	FName WindowName = NAME_None;
	FName RequiredCardName = NAME_None;
	TArray<FName> AcceptedCardNames;
	FName ContextName = NAME_None;
	int32 EffectSourceSeat = INDEX_NONE;
	int32 EffectTargetSeat = INDEX_NONE;
	bool bAllowPass = true;
	TArray<FSGSDecisionSkillOption> SkillOptions;
	bool bIsCardChoice = false;
	FName ChoiceName = NAME_None;
	int32 MinChoiceCount = 0;
	int32 MaxChoiceCount = 0;
	TArray<FSGSDecisionCardChoiceOption> CardChoiceOptions;
	TArray<TArray<int32>> CandidateCardSelections;
	bool bIsOptionChoice = false;
	TArray<FSGSDecisionNamedOption> NamedOptions;
};

class SGS_API ISGSRuleRuntime
{
public:
	virtual ~ISGSRuleRuntime() = default;

	virtual FSGSStatus RunEffectStep(FSGSEffectStep Step, FSGSCommandId CommandId) = 0;
	virtual bool OpenResponseWindow(const FSGSRuleResponseWindowSpec& Spec) = 0;
	virtual void AdvanceAfterPhase() = 0;
	virtual FSGSStableHandle PushResolutionFrame(FSGSResolutionFrame Frame) = 0;
	virtual FSGSStatus CompleteCurrentFrame(FName Reason) = 0;
	virtual FSGSStatus AbortAllFrames(FName Reason) = 0;
	virtual FSGSResolutionStack& GetResolutionStack() = 0;
	virtual const FSGSResolutionStack& GetResolutionStack() const = 0;
	virtual FSGSStatus PublishTimingEvent(const FSGSRuleEventPayload& Payload) = 0;
	virtual FSGSStatus ContinueTimingEventDispatch() { return MakeValue(); }
	virtual FSGSStatus ContinueGeneralSelection() { return MakeValue(); }
	virtual void RequestCurrentPhaseResume() {}
};

struct SGS_API FSGSRuleExecutionContext
{
	USGSGameContext* GameContext = nullptr;
	const FSGSCommand* Command = nullptr;
	const FSGSCommandExecutionContext* CommandExecutionContext = nullptr;
	FSGSReplayLog* ReplayLog = nullptr;
	FSGSActiveEffectTimeline* ActiveEffects = nullptr;
	FSGSTimingPoint TimingPoint;
	FSGSRuleInvocation RuleInvocation;
	ISGSRuleRuntime* Runtime = nullptr;
	const FSGSRuleRegistry* RuleRegistry = nullptr;

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(GameContext != nullptr, TEXT("RuleExecutionContext requires a GameContext."));
		bOk &= ensureMsgf(Command != nullptr, TEXT("RuleExecutionContext requires a Command."));
		bOk &= ensureMsgf(CommandExecutionContext != nullptr, TEXT("RuleExecutionContext requires CommandExecutionContext."));
		bOk &= ensureMsgf(ReplayLog != nullptr, TEXT("RuleExecutionContext requires a ReplayLog."));
		bOk &= ensureMsgf(ActiveEffects != nullptr, TEXT("RuleExecutionContext requires ActiveEffects."));
		bOk &= ensureMsgf(Runtime != nullptr, TEXT("RuleExecutionContext requires a Runtime."));
		bOk &= TimingPoint.CheckInvariants();
		bOk &= RuleInvocation.CheckInvariants();
		return bOk;
	}
};

class SGS_API ISGSRule
{
public:
	virtual ~ISGSRule() = default;

	virtual FName GetRuleName() const = 0;
	virtual FSGSRuleDescriptor GetDescriptor() const = 0;
	virtual const UScriptStruct* GetExpectedPayloadStruct() const { return nullptr; }
	virtual bool CanHandle(const FSGSRuleExecutionContext& Context) const = 0;
	virtual FSGSStatus Validate(FSGSRuleExecutionContext& Context) const = 0;
	virtual FSGSStatus Execute(FSGSRuleExecutionContext& Context) const = 0;

	// Modifier / ViewAs 仍是同一 Registry 中的规则部件；查询只读权威状态，
	// 修改查询结果或追加服务器合法候选，不直接改变对局状态。
	virtual void ModifyNumericQuery(const FSGSRuleQueryContext& Context, FSGSNumericRuleQuery& Query) const
	{
		(void)Context;
		(void)Query;
	}
	virtual void CollectSkillOptions(const FSGSRuleQueryContext& Context, FSGSSkillOptionQuery& Query) const
	{
		(void)Context;
		(void)Query;
	}

	bool CheckInvariants() const
	{
		bool bOk = true;
		const FSGSRuleDescriptor Descriptor = GetDescriptor();
		bOk &= ensureMsgf(!GetRuleName().IsNone(), TEXT("SGS rule requires a name."));
		bOk &= Descriptor.CheckInvariants();
		bOk &= ensureMsgf(
			GetRuleName() == Descriptor.RuleName,
			TEXT("SGS rule name %s does not match descriptor %s."),
			*GetRuleName().ToString(),
			*Descriptor.RuleName.ToString());
		return bOk;
	}
};
