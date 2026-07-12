#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/Responses/BasicCards/SGSDyingPeachRules.h"

namespace
{
FSGSStatus EliminateDyingSeatAndFinish(FSGSRuleExecutionContext& Context, int32 DyingSeatIndex)
{
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeEliminateSeatStep(DyingSeatIndex, FName(TEXT("NoPeach"))),
		SGSBasicCardRuleHelpers::GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}

	return SGSBasicCardRuleHelpers::CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.DyingPeachNoPeach")));
}

bool OpenNextDyingPeachResponseWindow(FSGSRuleExecutionContext& Context, FSGSResolutionFrame& DyingFrame)
{
	FSGSDyingPeachResolutionState* DyingState = DyingFrame.GetMutableState<FSGSDyingPeachResolutionState>();
	if (DyingState == nullptr)
	{
		return false;
	}

	const USGSSeat* DyingSeat = Context.GameContext->GetSeat(DyingState->DyingSeat);
	if (DyingSeat == nullptr || !DyingSeat->bIsAlive || DyingSeat->Health > 0)
	{
		return false;
	}

	DyingState->bNeedsHealthRecheck = false;
	while (DyingState->ResponderSeatIndices.IsValidIndex(DyingState->NextResponderIndex))
	{
		const int32 ResponderSeat = DyingState->ResponderSeatIndices[DyingState->NextResponderIndex++];
		const USGSSeat* Responder = Context.GameContext->GetSeat(ResponderSeat);
		if (Responder == nullptr || !Responder->bIsAlive)
		{
			continue;
		}

		FSGSRuleResponseWindowSpec WindowSpec;
		WindowSpec.SeatIndex = ResponderSeat;
		WindowSpec.WindowName = SGSBasicCardRuleHelpers::DyingPeachWindowName();
		WindowSpec.RequiredCardName = FName(TEXT("Peach"));
		WindowSpec.ContextName = FName(TEXT("Dying"));
		WindowSpec.EffectSourceSeat = DyingFrame.SourceSeat;
		WindowSpec.EffectTargetSeat = DyingFrame.TargetSeat;
		if (Context.Runtime->OpenResponseWindow(WindowSpec))
		{
			return true;
		}
	}

	return false;
}

FSGSResolutionFrame* FindLatestDyingFrameForSeat(FSGSResolutionStack& Stack, int32 DyingSeatIndex)
{
	return Stack.FindLatestFrameByPredicate([DyingSeatIndex](FSGSResolutionFrame& Frame)
	{
		const FSGSDyingPeachResolutionState* DyingState = Frame.GetState<FSGSDyingPeachResolutionState>();
		return DyingState != nullptr && DyingState->DyingSeat == DyingSeatIndex;
	});
}
}

FName SGSBasicCardRuleHelpers::SlashDodgeWindowName()
{
	return FName(TEXT("Slash.Dodge"));
}

FName SGSBasicCardRuleHelpers::DyingPeachWindowName()
{
	return FName(TEXT("Dying.Peach"));
}

FSGSRuleDescriptor SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
	FName RuleName,
	FName RuleKindTag,
	FGameplayTag IntentTag,
	FName SubjectName,
	FName WindowName,
	int32 Priority,
	bool bWildcardIntent,
	bool bWildcardSubject,
	bool bWildcardWindow)
{
	FSGSRuleDescriptor Descriptor;
	Descriptor.RuleName = RuleName;
	Descriptor.RuleKindTag = RuleKindTag;
	Descriptor.IntentTag = IntentTag;
	Descriptor.SubjectName = SubjectName;
	Descriptor.WindowName = WindowName;
	Descriptor.Priority = Priority;
	Descriptor.bWildcardIntent = bWildcardIntent;
	Descriptor.bWildcardSubject = bWildcardSubject;
	Descriptor.bWildcardWindow = bWildcardWindow;
	return Descriptor;
}

bool SGSBasicCardRuleHelpers::IsIntent(const FSGSRuleExecutionContext& Context, const FNativeGameplayTag& Type)
{
	return Context.RuleInvocation.IntentTag.MatchesTagExact(Type.GetTag());
}

bool SGSBasicCardRuleHelpers::IsWindow(FName ActualWindowName, FName ExpectedWindowName)
{
	return ActualWindowName == ExpectedWindowName;
}

FSGSStatus SGSBasicCardRuleHelpers::MakeRuleError(FName Code, FString Message)
{
	return MakeError(FSGSError::Make(Code, MoveTemp(Message)));
}

USGSCard* SGSBasicCardRuleHelpers::FindPayloadCard(const FSGSRuleExecutionContext& Context, int32 CardId)
{
	return Context.GameContext != nullptr ? Context.GameContext->FindCardById(CardId) : nullptr;
}

int32 SGSBasicCardRuleHelpers::GetCommandSeat(const FSGSRuleExecutionContext& Context)
{
	return Context.Command != nullptr ? Context.Command->SeatIndex : INDEX_NONE;
}

FSGSCommandId SGSBasicCardRuleHelpers::GetCommandId(const FSGSRuleExecutionContext& Context)
{
	return Context.Command != nullptr ? Context.Command->CommandId : FSGSCommandId();
}

FSGSStatus SGSBasicCardRuleHelpers::DiscardHandCard(FSGSRuleExecutionContext& Context, USGSCard* Card, int32 SeatIndex)
{
	if (Card == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidCard")),
			TEXT("Expected a valid hand card to discard."));
	}

	return Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ Card },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		SeatIndex,
		SGSGameplayTags::CardZone_DiscardPile.GetTag(),
		INDEX_NONE),
		GetCommandId(Context));
}

FSGSStatus SGSBasicCardRuleHelpers::DiscardProcessingCard(FSGSRuleExecutionContext& Context)
{
	const int32 ProcessingCardId = Context.Runtime->GetResolutionStack().FindLatestProcessingCardId();
	USGSCard* ProcessingCard = ProcessingCardId != INDEX_NONE ? Context.GameContext->FindCardById(ProcessingCardId) : nullptr;
	if (ProcessingCard == nullptr)
	{
		return MakeValue();
	}

	return Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ ProcessingCard },
		SGSGameplayTags::CardZone_Processing.GetTag(),
		INDEX_NONE,
		SGSGameplayTags::CardZone_DiscardPile.GetTag(),
		INDEX_NONE),
		GetCommandId(Context));
}

FSGSStatus SGSBasicCardRuleHelpers::CompleteCurrentFrame(FSGSRuleExecutionContext& Context, FName Reason)
{
	return Context.Runtime->CompleteCurrentFrame(Reason);
}

int32 SGSBasicCardRuleHelpers::GetCurrentEffectSourceSeat(const FSGSRuleExecutionContext& Context)
{
	return Context.Runtime->GetResolutionStack().FindLatestEffectSourceSeat();
}

int32 SGSBasicCardRuleHelpers::GetCurrentEffectTargetSeat(const FSGSRuleExecutionContext& Context)
{
	return Context.Runtime->GetResolutionStack().FindLatestEffectTargetSeat();
}

int32 SGSBasicCardRuleHelpers::GetPeachHealTarget(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload)
{
	if (Payload.EffectTargetSeat != INDEX_NONE)
	{
		return Payload.EffectTargetSeat;
	}
	if (Payload.TargetSeatIndices.Num() > 0)
	{
		return Payload.TargetSeatIndices[0];
	}
	return GetCommandSeat(Context);
}

FSGSStatus SGSBasicCardRuleHelpers::ExecutePeachHeal(FSGSRuleExecutionContext& Context, int32 PeachCardId, int32 HealTargetSeat)
{
	USGSCard* PeachCard = FindPayloadCard(Context, PeachCardId);
	if (PeachCard == nullptr || PeachCard->CardName != TEXT("Peach") || Context.GameContext->GetSeat(HealTargetSeat) == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidPeach")),
			TEXT("Peach rule requires a Peach card and a valid heal target."));
	}

	if (FSGSStatus Status = DiscardHandCard(Context, PeachCard, GetCommandSeat(Context)); Status.HasError())
	{
		return Status;
	}

	return Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeHealStep(HealTargetSeat, 1),
		GetCommandId(Context));
}

FSGSStatus SGSBasicCardRuleHelpers::ContinueDyingPeachOrEliminate(FSGSRuleExecutionContext& Context)
{
	FSGSResolutionFrame* DyingFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSDyingPeachResolutionState* DyingState = DyingFrame != nullptr ? DyingFrame->GetMutableState<FSGSDyingPeachResolutionState>() : nullptr;
	if (DyingFrame == nullptr || DyingState == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.MissingResolutionFrame")),
			TEXT("Dying.Peach requires the current resolution frame to carry DyingPeach state."));
	}

	const int32 DyingSeatIndex = DyingState->DyingSeat;
	const USGSSeat* DyingSeat = Context.GameContext->GetSeat(DyingSeatIndex);
	if (DyingSeat == nullptr || !DyingSeat->bIsAlive || DyingSeat->Health > 0)
	{
		return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.DyingPeachResolved")));
	}

	if (OpenNextDyingPeachResponseWindow(Context, *DyingFrame))
	{
		return MakeValue();
	}

	return EliminateDyingSeatAndFinish(Context, DyingSeatIndex);
}

FSGSStatus SGSBasicCardRuleHelpers::StartDyingPeachResolution(FSGSRuleExecutionContext& Context, int32 DyingSeatIndex)
{
	FSGSResolutionStack& Stack = Context.Runtime->GetResolutionStack();
	if (FSGSResolutionFrame* ExistingDyingFrame = FindLatestDyingFrameForSeat(Stack, DyingSeatIndex))
	{
		if (FSGSDyingPeachResolutionState* ExistingDyingState = ExistingDyingFrame->GetMutableState<FSGSDyingPeachResolutionState>())
		{
			ExistingDyingState->bNeedsHealthRecheck = true;
		}

		return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.DuplicateDyingMarked")));
	}

	FSGSDyingPeachResolutionState DyingState;
	DyingState.DyingSeat = DyingSeatIndex;
	DyingState.NextResponderIndex = 0;

	if (Context.GameContext->GetSeat(DyingSeatIndex) != nullptr)
	{
		DyingState.ResponderSeatIndices.Add(DyingSeatIndex);
	}
	for (int32 SeatIndex = 0; SeatIndex < Context.GameContext->NumSeats(); ++SeatIndex)
	{
		if (SeatIndex == DyingSeatIndex)
		{
			continue;
		}

		const USGSSeat* Seat = Context.GameContext->GetSeat(SeatIndex);
		if (Seat != nullptr && Seat->bIsAlive)
		{
			DyingState.ResponderSeatIndices.Add(SeatIndex);
		}
	}

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = FName(TEXT("SGS.Rule.DyingPeach"));
	Frame.SourceCommandId = GetCommandId(Context);
	Frame.ActorSeat = DyingSeatIndex;
	Frame.SourceSeat = GetCurrentEffectSourceSeat(Context);
	Frame.TargetSeat = DyingSeatIndex;
	Frame.WindowName = DyingPeachWindowName();
	Frame.RequiredCardName = FName(TEXT("Peach"));
	Frame.OnChildCompletedContinuation = SGSResolutionContinuations::ResumeDyingPeach();
	Frame.FrameState = FInstancedStruct::Make(DyingState);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	return ContinueDyingPeachOrEliminate(Context);
}

FSGSStatus SGSBasicCardRuleHelpers::ResolveSlashHit(FSGSRuleExecutionContext& Context)
{
	if (FSGSStatus Status = DiscardProcessingCard(Context); Status.HasError())
	{
		return Status;
	}

	const int32 SourceSeat = GetCurrentEffectSourceSeat(Context);
	const int32 TargetSeat = GetCurrentEffectTargetSeat(Context);
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeDamageStep(SourceSeat, TargetSeat, 1),
		GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}

	const USGSSeat* DamagedSeat = Context.GameContext->GetSeat(TargetSeat);
	if (DamagedSeat != nullptr && DamagedSeat->bIsAlive && DamagedSeat->Health <= 0)
	{
		return StartDyingPeachResolution(Context, TargetSeat);
	}

	return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.SlashHitResolved")));
}
