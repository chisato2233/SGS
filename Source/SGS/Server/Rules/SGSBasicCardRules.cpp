#include "Server/Rules/SGSBasicCardRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"

namespace
{
constexpr const TCHAR* SlashDodgeWindowName = TEXT("Slash.Dodge");
constexpr const TCHAR* DyingPeachWindowName = TEXT("Dying.Peach");

FSGSRuleDescriptor MakeBasicRuleDescriptor(
	FName RuleName,
	FName RuleKindTag,
	FGameplayTag IntentTag,
	FName SubjectName,
	FName WindowName,
	int32 Priority,
	bool bWildcardIntent = false,
	bool bWildcardSubject = false,
	bool bWildcardWindow = false)
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

bool IsIntent(const FSGSRuleExecutionContext& Context, const FNativeGameplayTag& Type)
{
	return Context.RuleInvocation.IntentTag.MatchesTagExact(Type.GetTag());
}

bool IsWindow(FName ActualWindowName, const TCHAR* ExpectedWindowName)
{
	return ActualWindowName == FName(ExpectedWindowName);
}

FSGSStatus MakeRuleError(FName Code, FString Message)
{
	return MakeError(FSGSError::Make(Code, MoveTemp(Message)));
}

USGSCard* FindPayloadCard(const FSGSRuleExecutionContext& Context, int32 CardId)
{
	return Context.GameContext != nullptr ? Context.GameContext->FindCardById(CardId) : nullptr;
}

int32 GetCommandSeat(const FSGSRuleExecutionContext& Context)
{
	return Context.Command != nullptr ? Context.Command->SeatIndex : INDEX_NONE;
}

FSGSCommandId GetCommandId(const FSGSRuleExecutionContext& Context)
{
	return Context.Command != nullptr ? Context.Command->CommandId : FSGSCommandId();
}

FSGSStatus DiscardHandCard(FSGSRuleExecutionContext& Context, USGSCard* Card, int32 SeatIndex)
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

FSGSStatus DiscardProcessingCard(FSGSRuleExecutionContext& Context)
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

FSGSStatus CompleteCurrentFrame(FSGSRuleExecutionContext& Context, FName Reason)
{
	return Context.Runtime->CompleteCurrentFrame(Reason);
}

int32 GetCurrentEffectSourceSeat(const FSGSRuleExecutionContext& Context)
{
	return Context.Runtime->GetResolutionStack().FindLatestEffectSourceSeat();
}

int32 GetCurrentEffectTargetSeat(const FSGSRuleExecutionContext& Context)
{
	return Context.Runtime->GetResolutionStack().FindLatestEffectTargetSeat();
}

int32 GetPeachHealTarget(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload)
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

FSGSStatus ExecutePeachHeal(FSGSRuleExecutionContext& Context, int32 PeachCardId, int32 HealTargetSeat)
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

FSGSStatus EliminateDyingSeatAndFinish(FSGSRuleExecutionContext& Context, int32 DyingSeatIndex)
{
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeEliminateSeatStep(DyingSeatIndex, FName(TEXT("NoPeach"))),
		GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}

	return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.DyingPeachNoPeach")));
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
		WindowSpec.WindowName = FName(DyingPeachWindowName);
		WindowSpec.RequiredCardName = FName(TEXT("Peach"));
		WindowSpec.EffectSourceSeat = DyingFrame.SourceSeat;
		WindowSpec.EffectTargetSeat = DyingFrame.TargetSeat;
		if (Context.Runtime->OpenResponseWindow(WindowSpec))
		{
			return true;
		}
	}

	return false;
}

FSGSStatus ContinueDyingPeachOrEliminate(FSGSRuleExecutionContext& Context)
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

FSGSStatus StartDyingPeachResolution(FSGSRuleExecutionContext& Context, int32 DyingSeatIndex)
{
	FSGSResolutionStack& Stack = Context.Runtime->GetResolutionStack();
	if (FSGSResolutionFrame* ExistingDyingFrame = Stack.FindLatestDyingFrameForSeat(DyingSeatIndex))
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
	Frame.WindowName = FName(DyingPeachWindowName);
	Frame.RequiredCardName = FName(TEXT("Peach"));
	Frame.OnChildCompletedContinuation = SGSResolutionContinuations::ResumeDyingPeach();
	Frame.FrameState = FInstancedStruct::Make(DyingState);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	return ContinueDyingPeachOrEliminate(Context);
}

FSGSStatus ResolveSlashHit(FSGSRuleExecutionContext& Context)
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
}

FName FSGSPassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Pass"));
}

FSGSRuleDescriptor FSGSPassRule::GetDescriptor() const
{
	return MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None,
		NAME_None,
		-1000);
}

bool FSGSPassRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	return IsIntent(Context, SGSGameplayTags::PlayAction_Pass) && Payload.WindowName.IsNone();
}

FSGSStatus FSGSPassRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	Context.Runtime->AdvanceAfterPhase();
	return MakeValue();
}

FName FSGSSlashRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Slash"));
}

FSGSRuleDescriptor FSGSSlashRule::GetDescriptor() const
{
	return MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_UseCard.GetTag(),
		FName(TEXT("Slash")),
		NAME_None,
		100);
}

bool FSGSSlashRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	return IsIntent(Context, SGSGameplayTags::PlayAction_UseCard)
		&& Context.RuleInvocation.WindowName.IsNone()
		&& Payload.CardName == TEXT("Slash");
}

FSGSStatus FSGSSlashRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	if (Payload.TargetSeatIndices.Num() != 1)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidTarget")),
			TEXT("Slash requires exactly one target."));
	}

	const int32 SourceSeat = GetCommandSeat(Context);
	const int32 TargetSeat = Payload.TargetSeatIndices[0];
	USGSCard* SlashCard = FindPayloadCard(Context, Payload.CardId);
	if (SlashCard == nullptr || SlashCard->CardName != TEXT("Slash") || Context.GameContext->GetSeat(TargetSeat) == nullptr || TargetSeat == SourceSeat)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidSlash")),
			TEXT("Slash requires a Slash card and a valid non-self target."));
	}

	if (Context.GameContext->GetDistance(SourceSeat, TargetSeat) > 1)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.TargetOutOfDistance")),
			TEXT("Slash target is out of distance."));
	}

	return MakeValue();
}

FSGSStatus FSGSSlashRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	USGSCard* SlashCard = FindPayloadCard(Context, Payload.CardId);
	const int32 SourceSeat = GetCommandSeat(Context);
	const int32 TargetSeat = Payload.TargetSeatIndices[0];

	if (FSGSStatus Status = Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ SlashCard },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		SourceSeat,
		SGSGameplayTags::CardZone_Processing.GetTag(),
		INDEX_NONE),
		GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}

	FSGSSlashResolutionState SlashState;
	SlashState.SlashCardId = SlashCard->CardId;
	SlashState.SourceSeat = SourceSeat;
	SlashState.TargetSeat = TargetSeat;

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = GetRuleName();
	Frame.SourceCommandId = GetCommandId(Context);
	Frame.ActorSeat = SourceSeat;
	Frame.SourceSeat = SourceSeat;
	Frame.TargetSeat = TargetSeat;
	Frame.ProcessingCardId = SlashCard->CardId;
	Frame.OnChildCompletedContinuation = SGSResolutionContinuations::FinishParentCardResolution();
	Frame.FrameState = FInstancedStruct::Make(SlashState);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	FSGSRuleResponseWindowSpec WindowSpec;
	WindowSpec.SeatIndex = TargetSeat;
	WindowSpec.WindowName = FName(SlashDodgeWindowName);
	WindowSpec.RequiredCardName = FName(TEXT("Dodge"));
	WindowSpec.EffectSourceSeat = SourceSeat;
	WindowSpec.EffectTargetSeat = TargetSeat;
	if (Context.Runtime->OpenResponseWindow(WindowSpec))
	{
		return MakeValue();
	}

	return ResolveSlashHit(Context);
}

FName FSGSDodgePassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.DodgePass"));
}

FSGSRuleDescriptor FSGSDodgePassRule::GetDescriptor() const
{
	return MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None,
		FName(SlashDodgeWindowName),
		210);
}

bool FSGSDodgePassRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	return IsIntent(Context, SGSGameplayTags::PlayAction_Pass) && IsWindow(Payload.WindowName, SlashDodgeWindowName);
}

FSGSStatus FSGSDodgePassRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	return ResolveSlashHit(Context);
}

FName FSGSDodgeResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.DodgeResponse"));
}

FSGSRuleDescriptor FSGSDodgeResponseRule::GetDescriptor() const
{
	return MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Dodge")),
		FName(SlashDodgeWindowName),
		200);
}

bool FSGSDodgeResponseRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	return IsIntent(Context, SGSGameplayTags::PlayAction_RespondCard)
		&& IsWindow(Payload.WindowName, SlashDodgeWindowName)
		&& Payload.CardName == TEXT("Dodge");
}

FSGSStatus FSGSDodgeResponseRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	USGSCard* DodgeCard = FindPayloadCard(Context, Payload.CardId);
	if (DodgeCard == nullptr || DodgeCard->CardName != TEXT("Dodge"))
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidDodge")),
			TEXT("Dodge response requires a Dodge card."));
	}

	return MakeValue();
}

FSGSStatus FSGSDodgeResponseRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	USGSCard* DodgeCard = FindPayloadCard(Context, Payload.CardId);
	if (FSGSStatus Status = DiscardHandCard(Context, DodgeCard, GetCommandSeat(Context)); Status.HasError())
	{
		return Status;
	}
	if (FSGSStatus Status = DiscardProcessingCard(Context); Status.HasError())
	{
		return Status;
	}

	return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.SlashDodged")));
}

FName FSGSPeachRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Peach"));
}

FSGSRuleDescriptor FSGSPeachRule::GetDescriptor() const
{
	return MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_UseCard.GetTag(),
		FName(TEXT("Peach")),
		NAME_None,
		100);
}

bool FSGSPeachRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	return IsIntent(Context, SGSGameplayTags::PlayAction_UseCard)
		&& Context.RuleInvocation.WindowName.IsNone()
		&& Payload.CardName == TEXT("Peach");
}

FSGSStatus FSGSPeachRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	const int32 HealTargetSeat = GetPeachHealTarget(Context, Payload);
	USGSCard* PeachCard = FindPayloadCard(Context, Payload.CardId);
	if (PeachCard == nullptr || PeachCard->CardName != TEXT("Peach") || Context.GameContext->GetSeat(HealTargetSeat) == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidPeach")),
			TEXT("Peach requires a Peach card and a valid heal target."));
	}

	return MakeValue();
}

FSGSStatus FSGSPeachRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	const int32 HealTargetSeat = GetPeachHealTarget(Context, Payload);
	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = GetRuleName();
	Frame.SourceCommandId = GetCommandId(Context);
	Frame.ActorSeat = GetCommandSeat(Context);
	Frame.SourceSeat = GetCommandSeat(Context);
	Frame.TargetSeat = HealTargetSeat;
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	if (FSGSStatus Status = ExecutePeachHeal(Context, Payload.CardId, HealTargetSeat); Status.HasError())
	{
		Context.Runtime->AbortAllFrames(FName(TEXT("SGS.Resolution.PeachFailed")));
		return Status;
	}

	return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.PeachResolved")));
}

FName FSGSDyingPeachPassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.DyingPeachPass"));
}

FSGSRuleDescriptor FSGSDyingPeachPassRule::GetDescriptor() const
{
	return MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None,
		FName(DyingPeachWindowName),
		210);
}

bool FSGSDyingPeachPassRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	return IsIntent(Context, SGSGameplayTags::PlayAction_Pass) && IsWindow(Payload.WindowName, DyingPeachWindowName);
}

FSGSStatus FSGSDyingPeachPassRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	return ContinueDyingPeachOrEliminate(Context);
}

FName FSGSDyingPeachRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.DyingPeach"));
}

FSGSRuleDescriptor FSGSDyingPeachRule::GetDescriptor() const
{
	return MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Peach")),
		FName(DyingPeachWindowName),
		200);
}

bool FSGSDyingPeachRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	return IsIntent(Context, SGSGameplayTags::PlayAction_RespondCard)
		&& IsWindow(Payload.WindowName, DyingPeachWindowName)
		&& Payload.CardName == TEXT("Peach");
}

FSGSStatus FSGSDyingPeachRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	USGSCard* PeachCard = FindPayloadCard(Context, Payload.CardId);
	if (PeachCard == nullptr || PeachCard->CardName != TEXT("Peach"))
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidDyingPeach")),
			TEXT("Dying.Peach response requires a Peach card."));
	}

	return MakeValue();
}

FSGSStatus FSGSDyingPeachRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	FSGSResolutionFrame* DyingFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSDyingPeachResolutionState* DyingState = DyingFrame != nullptr ? DyingFrame->GetState<FSGSDyingPeachResolutionState>() : nullptr;
	if (DyingFrame == nullptr || DyingState == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.MissingResolutionFrame")),
			TEXT("Dying.Peach response requires the current DyingPeach frame."));
	}

	const int32 DyingSeatIndex = DyingState->DyingSeat;
	if (FSGSStatus Status = ExecutePeachHeal(Context, Payload.CardId, DyingSeatIndex); Status.HasError())
	{
		return Status;
	}

	const USGSSeat* DyingSeat = Context.GameContext->GetSeat(DyingSeatIndex);
	if (DyingSeat != nullptr && DyingSeat->Health > 0)
	{
		return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.DyingPeachResolved")));
	}

	return ContinueDyingPeachOrEliminate(Context);
}
