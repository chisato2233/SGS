#pragma once

// Server-only rule layer: validated Commands become typed RuleInvocation, then
// Rules enqueue Effects or open controlled response windows through ISGSRuleRuntime.

#include "CoreMinimal.h"
#include "Shared/Commands/SGSCommand.h"
#include "Shared/Core/SGSError.h"
#include "Server/Effects/SGSEffectPipeline.h"
#include "Server/Rules/SGSResolutionStack.h"
#include "Server/Rules/SGSRuleDescriptor.h"
#include "Server/Rules/SGSRuleInvocation.h"

class USGSCard;
class USGSGameContext;
struct FSGSCommandExecutionContext;

struct SGS_API FSGSRuleResponseWindowSpec
{
	int32 SeatIndex = INDEX_NONE;
	FName WindowName = NAME_None;
	FName RequiredCardName = NAME_None;
	int32 EffectSourceSeat = INDEX_NONE;
	int32 EffectTargetSeat = INDEX_NONE;
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
	virtual bool CanHandle(const FSGSRuleExecutionContext& Context) const = 0;
	virtual FSGSStatus Validate(FSGSRuleExecutionContext& Context) const = 0;
	virtual FSGSStatus Execute(FSGSRuleExecutionContext& Context) const = 0;

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
