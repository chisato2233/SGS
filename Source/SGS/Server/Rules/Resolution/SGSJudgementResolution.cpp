#include "Server/Rules/Resolution/SGSJudgementResolution.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Rules/Payloads/SGSRuleEventPayloads.h"

FName SGSJudgementResolution::ResumeAfterTriggersContinuation()
{
	return FName(TEXT("SGS.Resolution.Continuation.JudgementAfterTriggers"));
}

FSGSJudgementResolutionState* SGSJudgementResolution::FindActiveState(FSGSResolutionStack& Stack)
{
	FSGSResolutionFrame* Frame = Stack.FindLatestFrameWithState<FSGSJudgementResolutionState>();
	return Frame != nullptr ? Frame->GetMutableState<FSGSJudgementResolutionState>() : nullptr;
}

FSGSStatus SGSJudgementResolution::Start(
	FSGSRuleExecutionContext& Context,
	int32 JudgedSeat,
	FName ReasonName,
	FName ParentContinuation)
{
	FSGSResolutionStack& Stack = Context.Runtime->GetResolutionStack();
	FSGSResolutionFrame* ParentFrame = Stack.GetCurrentFrame();
	check(ParentFrame != nullptr);
	ParentFrame->OnChildCompletedContinuation = ParentContinuation;

	FSGSJudgementResolutionState State;
	State.JudgedSeat = JudgedSeat;
	State.ReasonName = ReasonName;

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = FName(TEXT("SGS.Rule.Judgement"));
	Frame.SourceCommandId = Context.RuleInvocation.SourceCommandId;
	Frame.ActorSeat = JudgedSeat;
	Frame.SourceSeat = JudgedSeat;
	Frame.TargetSeat = JudgedSeat;
	Frame.FrameState = FInstancedStruct::Make(State);
	const FSGSStableHandle JudgementFrameHandle = Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	TSharedRef<TObjectPtr<USGSCard>> ResultCard = MakeShared<TObjectPtr<USGSCard>>();
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeJudgementDrawStep(JudgedSeat, ResultCard),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	FSGSResolutionFrame* JudgementFrame = Stack.FindFrame(JudgementFrameHandle);
	check(JudgementFrame != nullptr);
	FSGSJudgementResolutionState* MutableState = JudgementFrame->GetMutableState<FSGSJudgementResolutionState>();
	check(MutableState != nullptr);
	MutableState->ResultCardId = ResultCard.Get().Get() != nullptr ? ResultCard.Get()->CardId : INDEX_NONE;

	FSGSRuleEventPayload Event;
	Event.EventTag = SGSGameplayTags::GameEvent_JudgementRevealed.GetTag();
	Event.EventName = FName(TEXT("JudgementRevealed"));
	Event.SourceSeat = JudgedSeat;
	Event.TargetSeat = JudgedSeat;
	Event.SourceCommandId = Context.RuleInvocation.SourceCommandId;
	Event.TimingPoint = Context.TimingPoint;
	Event.TimingPoint.Step = SGSTimingSteps::After();
	Event.TimingPoint.SubOrder += 1;
	FSGSJudgementEventData EventData;
	EventData.JudgedSeat = JudgedSeat;
	EventData.ReasonName = ReasonName;
	EventData.ResultCardId = MutableState->ResultCardId;
	Event.EventData = FInstancedStruct::Make(EventData);
	JudgementFrame->OnChildCompletedContinuation = ResumeAfterTriggersContinuation();
	if (FSGSStatus Status = Context.Runtime->PublishTimingEvent(Event); Status.HasError())
	{
		return Status;
	}
	if (Stack.GetCurrentFrameHandle() != JudgementFrameHandle)
	{
		return MakeValue();
	}
	JudgementFrame = Stack.FindFrame(JudgementFrameHandle);
	check(JudgementFrame != nullptr);
	JudgementFrame->OnChildCompletedContinuation = NAME_None;
	return ContinueAfterTriggers(Context);
}

FSGSStatus SGSJudgementResolution::ContinueAfterTriggers(FSGSRuleExecutionContext& Context)
{
	return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.JudgementReady")));
}
