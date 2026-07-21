#include "Server/Rules/Resolution/SGSDamageResolution.h"

#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Payloads/SGSRuleEventPayloads.h"

FName SGSDamageResolution::ResumeAfterTriggersContinuation()
{
	return FName(TEXT("SGS.Resolution.Continuation.DamageAfterTriggers"));
}

FName SGSDamageResolution::FinishAfterDyingContinuation()
{
	return FName(TEXT("SGS.Resolution.Continuation.DamageAfterDying"));
}

FSGSStatus SGSDamageResolution::Start(
	FSGSRuleExecutionContext& Context,
	int32 SourceSeat,
	int32 TargetSeat,
	int32 Amount,
	int32 CardId,
	FName ParentContinuation)
{
	FSGSResolutionStack& Stack = Context.Runtime->GetResolutionStack();
	FSGSResolutionFrame* ParentFrame = Stack.GetCurrentFrame();
	check(ParentFrame != nullptr);
	ParentFrame->OnChildCompletedContinuation = ParentContinuation;

	FSGSDamageResolutionState State;
	State.SourceSeat = SourceSeat;
	State.TargetSeat = TargetSeat;
	State.Amount = Amount;
	State.CardId = CardId;

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = FName(TEXT("SGS.Rule.Damage"));
	Frame.SourceCommandId = Context.RuleInvocation.SourceCommandId;
	Frame.ActorSeat = SourceSeat != INDEX_NONE ? SourceSeat : TargetSeat;
	Frame.SourceSeat = SourceSeat;
	Frame.TargetSeat = TargetSeat;
	Frame.FrameState = FInstancedStruct::Make(State);
	const FSGSStableHandle DamageFrameHandle = Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeDamageStep(SourceSeat, TargetSeat, Amount),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}

	FSGSRuleEventPayload DamageAfter;
	DamageAfter.EventTag = SGSGameplayTags::GameEvent_DamageAfter.GetTag();
	DamageAfter.EventName = FName(TEXT("DamageAfter"));
	DamageAfter.SourceSeat = SourceSeat;
	DamageAfter.TargetSeat = TargetSeat;
	DamageAfter.SourceCommandId = Context.RuleInvocation.SourceCommandId;
	DamageAfter.TimingPoint = Context.TimingPoint;
	DamageAfter.TimingPoint.Step = SGSTimingSteps::After();
	DamageAfter.TimingPoint.SubOrder += 1;
	FSGSDamageEventData DamageData;
	DamageData.CardId = CardId;
	DamageData.Amount = Amount;
	DamageAfter.EventData = FInstancedStruct::Make(DamageData);
	FSGSResolutionFrame* DamageFrame = Stack.FindFrame(DamageFrameHandle);
	check(DamageFrame != nullptr);
	DamageFrame->OnChildCompletedContinuation = ResumeAfterTriggersContinuation();
	if (FSGSStatus Status = Context.Runtime->PublishTimingEvent(DamageAfter); Status.HasError())
	{
		return Status;
	}
	if (Stack.GetCurrentFrameHandle() != DamageFrameHandle)
	{
		return MakeValue();
	}
	DamageFrame = Stack.FindFrame(DamageFrameHandle);
	check(DamageFrame != nullptr);
	DamageFrame->OnChildCompletedContinuation = NAME_None;
	return ContinueAfterTriggers(Context);
}

FSGSStatus SGSDamageResolution::ContinueAfterTriggers(FSGSRuleExecutionContext& Context)
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSDamageResolutionState* State = Frame != nullptr
		? Frame->GetState<FSGSDamageResolutionState>()
		: nullptr;
	if (State == nullptr)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.MissingDamageResolution")),
			TEXT("Damage continuation requires the current damage resolution state."));
	}

	const USGSSeat* Target = Context.GameContext->GetSeat(State->TargetSeat);
	if (Target != nullptr && Target->bIsAlive && Target->Health <= 0)
	{
		Frame->OnChildCompletedContinuation = FinishAfterDyingContinuation();
		return SGSBasicCardRuleHelpers::StartDyingPeachResolution(Context, State->TargetSeat);
	}
	return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.DamageComplete")));
}
