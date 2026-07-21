#include "Server/Rules/Resolution/SGSLordAssistResolution.h"

#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

FName SGSLordAssistResolution::WindowName()
{
	return FName(TEXT("Skill.LordAssist.Provider"));
}

FName SGSLordAssistResolution::ResumeParentContinuation()
{
	return FName(TEXT("SGS.Resolution.Continuation.LordAssistToParent"));
}

FSGSStatus SGSLordAssistResolution::Start(
	FSGSRuleExecutionContext& Context,
	FName SkillName,
	int32 LordSeat,
	FName RequiredCardName,
	int32 EffectTargetSeat,
	FGameplayTag ProviderFaction,
	bool bPlaySlash)
{
	FSGSResolutionStack& Stack = Context.Runtime->GetResolutionStack();
	FSGSResolutionFrame* ParentFrame = Stack.GetCurrentFrame();
	const FName OriginWindowName = Context.RuleInvocation.WindowName;
	if (ParentFrame != nullptr && !OriginWindowName.IsNone())
	{
		ParentFrame->OnChildCompletedContinuation = ResumeParentContinuation();
	}

	FSGSLordAssistResolutionState State;
	State.SkillName = SkillName;
	State.LordSeat = LordSeat;
	State.RequiredCardName = RequiredCardName;
	State.EffectTargetSeat = EffectTargetSeat;
	State.OriginWindowName = OriginWindowName;
	State.bPlaySlash = bPlaySlash;
	for (int32 Offset = 1; Offset < Context.GameContext->NumSeats(); ++Offset)
	{
		const int32 SeatIndex = (LordSeat + Offset) % Context.GameContext->NumSeats();
		const USGSSeat* Seat = Context.GameContext->GetSeat(SeatIndex);
		if (Seat != nullptr && Seat->bIsAlive && Seat->Faction.MatchesTagExact(ProviderFaction))
		{
			State.ProviderSeats.Add(SeatIndex);
		}
	}

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = FName(*FString::Printf(TEXT("SGS.Skill.%s.Assist"), *SkillName.ToString()));
	Frame.SourceCommandId = Context.RuleInvocation.SourceCommandId;
	Frame.ActorSeat = LordSeat;
	Frame.SourceSeat = LordSeat;
	Frame.TargetSeat = EffectTargetSeat;
	Frame.FrameState = FInstancedStruct::Make(State);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));
	return Continue(Context, false);
}

FSGSStatus SGSLordAssistResolution::Continue(FSGSRuleExecutionContext& Context, bool bAccepted)
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSLordAssistResolutionState* State = Frame != nullptr
		? Frame->GetMutableState<FSGSLordAssistResolutionState>()
		: nullptr;
	check(State != nullptr);
	if (bAccepted)
	{
		State->bSucceeded = true;
		if (State->bPlaySlash)
		{
			Frame->OnChildCompletedContinuation = SGSResolutionContinuations::FinishParentCardResolution();
			return SGSBasicCardRuleHelpers::StartSlashResolution(
				Context,
				State->LordSeat,
				State->EffectTargetSeat,
				INDEX_NONE,
				true,
				FName(TEXT("SGS.Skill.Jijiang.Slash")));
		}
		return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.LordAssistAccepted")));
	}

	while (State->ProviderSeats.IsValidIndex(State->NextProviderIndex))
	{
		const int32 ProviderSeat = State->ProviderSeats[State->NextProviderIndex++];
		FSGSRuleResponseWindowSpec Spec;
		Spec.SeatIndex = ProviderSeat;
		Spec.WindowName = WindowName();
		Spec.RequiredCardName = State->RequiredCardName;
		Spec.AcceptedCardNames.Add(State->RequiredCardName);
		Spec.ContextName = State->SkillName;
		Spec.EffectSourceSeat = State->LordSeat;
		Spec.EffectTargetSeat = State->EffectTargetSeat;
		if (Context.Runtime->OpenResponseWindow(Spec))
		{
			return MakeValue();
		}
	}
	if (State->bPlaySlash)
	{
		SGSBasicCardRuleHelpers::AddStatus(
			Context,
			State->LordSeat,
			FName(TEXT("SGS.ActiveEffect.JijiangFailed")),
			SGSGameplayTags::Status_JijiangFailed.GetTag(),
			FSGSDurationSpec::ThisPhase(State->LordSeat, Context.TimingPoint));
	}
	return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.LordAssistDeclined")));
}
