#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/Actions/BasicCards/SGSSlashRule.h"
#include "Server/Rules/Responses/BasicCards/SGSDyingPeachRules.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"
#include "Server/Rules/Resolution/SGSDamageResolution.h"

namespace
{
FSGSStatus EliminateDyingSeatAndFinish(FSGSRuleExecutionContext& Context, int32 DyingSeatIndex)
{
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeEliminateSeatStep(
			DyingSeatIndex,
			SGSBasicCardRuleHelpers::GetCurrentEffectSourceSeat(Context),
			FName(TEXT("NoPeach"))),
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
		WindowSpec.AcceptedCardNames.Add(FName(TEXT("Peach")));
		if (ResponderSeat == DyingState->DyingSeat)
		{
			WindowSpec.AcceptedCardNames.Add(FName(TEXT("Analeptic")));
		}
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

FName SGSBasicCardRuleHelpers::SlashDamageTriggerContinuation()
{
	return FName(TEXT("SGS.Resolution.Continuation.SlashAfterDamageTriggers"));
}

FName SGSBasicCardRuleHelpers::AxeWindowName() { return FName(TEXT("Slash.Axe")); }
FName SGSBasicCardRuleHelpers::AxeChoiceName() { return FName(TEXT("Axe.Discard")); }
FName SGSBasicCardRuleHelpers::BladeWindowName() { return FName(TEXT("Slash.Blade")); }
FName SGSBasicCardRuleHelpers::KylinBowWindowName() { return FName(TEXT("Slash.KylinBow")); }
FName SGSBasicCardRuleHelpers::KylinBowChoiceName() { return FName(TEXT("KylinBow.DiscardHorse")); }

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

bool SGSBasicCardRuleHelpers::HasStatus(const FSGSRuleExecutionContext& Context, int32 SeatIndex, FGameplayTag StatusTag)
{
	for (const FSGSStableHandle Handle : Context.ActiveEffects->QueryByTag(StatusTag))
	{
		const FSGSActiveEffect* Effect = Context.ActiveEffects->Find(Handle);
		if (Effect != nullptr && Effect->OwnerSeat == SeatIndex)
		{
			return true;
		}
	}

	return false;
}

int32 SGSBasicCardRuleHelpers::CountStatus(const FSGSRuleExecutionContext& Context, int32 SeatIndex, FGameplayTag StatusTag)
{
	int32 Count = 0;
	for (const FSGSStableHandle Handle : Context.ActiveEffects->QueryByTag(StatusTag))
	{
		const FSGSActiveEffect* Effect = Context.ActiveEffects->Find(Handle);
		if (Effect != nullptr && Effect->OwnerSeat == SeatIndex)
		{
			++Count;
		}
	}
	return Count;
}

void SGSBasicCardRuleHelpers::AddStatus(
	FSGSRuleExecutionContext& Context,
	int32 SeatIndex,
	FName EffectName,
	FGameplayTag StatusTag,
	FSGSDurationSpec Duration)
{
	FSGSActiveEffect Effect;
	Effect.EffectName = EffectName;
	Effect.PrimaryTag = StatusTag;
	Effect.SourceCommandId = GetCommandId(Context);
	Effect.OwnerSeat = SeatIndex;
	Effect.TargetSeat = SeatIndex;
	Effect.Duration = MoveTemp(Duration);
	Effect.CreatedAt = Context.TimingPoint;
	Context.ActiveEffects->Add(MoveTemp(Effect));
}

bool SGSBasicCardRuleHelpers::ConsumeStatus(FSGSRuleExecutionContext& Context, int32 SeatIndex, FGameplayTag StatusTag)
{
	for (const FSGSStableHandle Handle : Context.ActiveEffects->QueryByTag(StatusTag))
	{
		const FSGSActiveEffect* Effect = Context.ActiveEffects->Find(Handle);
		if (Effect != nullptr && Effect->OwnerSeat == SeatIndex)
		{
			Context.ActiveEffects->Expire(Handle, Context.TimingPoint, SGSActiveEffectExpireReasons::Manual());
			return true;
		}
	}

	return false;
}

FSGSStatus SGSBasicCardRuleHelpers::DiscardHandCard(FSGSRuleExecutionContext& Context, USGSCard* Card, int32 SeatIndex)
{
	if (Card == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidCard")),
			TEXT("Expected a valid hand card to discard."));
	}

	FSGSCardMoveEventMetadata Metadata;
	Metadata.Reason = Context.RuleInvocation.IntentTag.MatchesTagExact(SGSGameplayTags::PlayAction_RespondCard.GetTag())
		? SGSCardMoveReasons::Respond()
		: SGSCardMoveReasons::Use();
	if (const FSGSUseCardRulePayload* UsePayload = Context.RuleInvocation.GetPayload<FSGSUseCardRulePayload>())
	{
		Metadata.RelatedTargetSeatIndices = UsePayload->TargetSeatIndices;
	}
	else if (const FSGSRespondCardRulePayload* ResponsePayload = Context.RuleInvocation.GetPayload<FSGSRespondCardRulePayload>())
	{
		if (ResponsePayload->EffectSourceSeat != INDEX_NONE)
		{
			Metadata.RelatedTargetSeatIndices.Add(ResponsePayload->EffectSourceSeat);
		}
		if (ResponsePayload->EffectTargetSeat != INDEX_NONE
			&& ResponsePayload->EffectTargetSeat != ResponsePayload->EffectSourceSeat)
		{
			Metadata.RelatedTargetSeatIndices.Add(ResponsePayload->EffectTargetSeat);
		}
	}

	return Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ Card },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		SeatIndex,
		SGSGameplayTags::CardZone_DiscardPile.GetTag(),
		INDEX_NONE,
		MoveTemp(Metadata)),
		GetCommandId(Context));
}

FSGSStatus SGSBasicCardRuleHelpers::DiscardProcessingCard(FSGSRuleExecutionContext& Context)
{
	const int32 ProcessingCardId = Context.Runtime->GetResolutionStack().FindLatestProcessingCardId();
	USGSCard* ProcessingCard = ProcessingCardId != INDEX_NONE ? Context.GameContext->FindCardById(ProcessingCardId) : nullptr;
	const FSGSCardState* ProcessingState = ProcessingCard != nullptr
		? Context.GameContext->FindCardStateById(ProcessingCardId)
		: nullptr;
	if (ProcessingCard == nullptr
		|| ProcessingState == nullptr
		|| !ProcessingState->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Processing.GetTag()))
	{
		return MakeValue();
	}

	return Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ ProcessingCard },
		SGSGameplayTags::CardZone_Processing.GetTag(),
		INDEX_NONE,
		SGSGameplayTags::CardZone_DiscardPile.GetTag(),
		INDEX_NONE,
		{ SGSCardMoveReasons::Cleanup(), {} }),
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

FSGSStatus SGSBasicCardRuleHelpers::ExecuteHealCard(
	FSGSRuleExecutionContext& Context,
	int32 CardId,
	FName CardName,
	int32 HealTargetSeat,
	int32 HealAmount)
{
	USGSCard* Card = FindPayloadCard(Context, CardId);
	if (Card == nullptr || Card->CardName != CardName || Context.GameContext->GetSeat(HealTargetSeat) == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidHealCard")),
			TEXT("Heal rule requires the expected card and a valid target."));
	}

	if (FSGSStatus Status = DiscardHandCard(Context, Card, GetCommandSeat(Context)); Status.HasError())
	{
		return Status;
	}

	return Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeHealStep(HealTargetSeat, HealAmount),
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
	FSGSResolutionFrame* SlashFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSSlashResolutionState* SlashState = SlashFrame != nullptr
		? SlashFrame->GetMutableState<FSGSSlashResolutionState>()
		: nullptr;
	if (SlashState == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.MissingSlashResolution")),
			TEXT("Slash hit requires the current Slash resolution state."));
	}

	return SGSDamageResolution::Start(
		Context,
		SlashState->SourceSeat,
		SlashState->TargetSeat,
		SlashState->DamageAmount,
		SlashState->SlashCardId,
		SlashDamageTriggerContinuation());
}

namespace
{
FSGSStatus DiscardSlashMaterials(FSGSRuleExecutionContext& Context)
{
	const FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSSlashResolutionState* State = Frame != nullptr
		? Frame->GetState<FSGSSlashResolutionState>()
		: nullptr;
	if (State == nullptr || State->MaterialCardIds.IsEmpty())
	{
		return SGSBasicCardRuleHelpers::DiscardProcessingCard(Context);
	}
	TArray<USGSCard*> Cards;
	for (const int32 CardId : State->MaterialCardIds)
	{
		const FSGSCardState* CardState = Context.GameContext->FindCardStateById(CardId);
		if (CardState != nullptr
			&& CardState->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Processing.GetTag()))
		{
			Cards.Add(CardState->Card.Get());
		}
	}
	return Cards.IsEmpty()
		? MakeValue()
		: Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				Cards,
				SGSGameplayTags::CardZone_Processing.GetTag(),
				INDEX_NONE,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE,
				{ SGSCardMoveReasons::Cleanup(), {} }),
			Context.RuleInvocation.SourceCommandId);
}

FSGSStatus ContinueCurrentSlashTarget(FSGSRuleExecutionContext& Context);

FSGSStatus AdvanceSlashTarget(FSGSRuleExecutionContext& Context, FName Reason)
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSSlashResolutionState* State = Frame != nullptr ? Frame->GetMutableState<FSGSSlashResolutionState>() : nullptr;
	if (State == nullptr)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.MissingSlashResolution")), TEXT("Slash target advance requires an active Slash."));
	}
	++State->TargetIndex;
	if (!State->TargetSeatIndices.IsValidIndex(State->TargetIndex))
	{
		if (FSGSStatus Status = DiscardSlashMaterials(Context); Status.HasError())
		{
			return Status;
		}
		return SGSBasicCardRuleHelpers::CompleteCurrentFrame(Context, Reason);
	}
	State->TargetSeat = State->TargetSeatIndices[State->TargetIndex];
	State->DodgeCount = 0;
	State->bAxeOffered = false;
	State->bBladeOffered = false;
	State->bKylinBowOffered = false;
	Frame->TargetSeat = State->TargetSeat;
	return ContinueCurrentSlashTarget(Context);
}

FSGSStatus ContinueCurrentSlashTarget(FSGSRuleExecutionContext& Context)
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSSlashResolutionState* State = Frame != nullptr ? Frame->GetMutableState<FSGSSlashResolutionState>() : nullptr;
	if (State == nullptr)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.MissingSlashResolution")), TEXT("Slash target resolution requires an active Slash."));
	}
	const USGSSeat* Target = Context.GameContext->GetSeat(State->TargetSeat);
	if (Target == nullptr || !Target->bIsAlive)
	{
		return AdvanceSlashTarget(Context, FName(TEXT("SGS.Resolution.SlashTargetsResolved")));
	}
	const USGSCard* Armor = Target->Equipment.FindRef(SGSGameplayTags::EquipSlot_Armor.GetTag()).Get();
	const USGSCard* SlashCard = State->SlashCardId != INDEX_NONE
		? Context.GameContext->FindCardById(State->SlashCardId)
		: nullptr;
	const bool bBlackSlash = SlashCard != nullptr
		&& (SlashCard->Suit.MatchesTagExact(SGSGameplayTags::Suit_Spade.GetTag())
			|| SlashCard->Suit.MatchesTagExact(SGSGameplayTags::Suit_Club.GetTag()));
	if (!State->bIgnoreArmor && Armor != nullptr && Armor->CardName == TEXT("RenwangShield") && bBlackSlash)
	{
		return AdvanceSlashTarget(Context, FName(TEXT("SGS.Resolution.SlashTargetsResolved")));
	}

	FSGSRuleResponseWindowSpec Spec;
	Spec.SeatIndex = State->TargetSeat;
	Spec.WindowName = SGSBasicCardRuleHelpers::SlashDodgeWindowName();
	Spec.RequiredCardName = FName(TEXT("Dodge"));
	Spec.AcceptedCardNames.Add(FName(TEXT("Dodge")));
	Spec.ContextName = FName(TEXT("Slash"));
	Spec.EffectSourceSeat = State->SourceSeat;
	Spec.EffectTargetSeat = State->TargetSeat;
	return Context.Runtime->OpenResponseWindow(Spec)
		? MakeValue()
		: SGSBasicCardRuleHelpers::ResolveSlashHit(Context);
}
}

FSGSStatus SGSBasicCardRuleHelpers::ContinueSlashAfterDamageTriggers(FSGSRuleExecutionContext& Context)
{
	FSGSResolutionFrame* SlashFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSSlashResolutionState* SlashState = SlashFrame != nullptr
		? SlashFrame->GetMutableState<FSGSSlashResolutionState>()
		: nullptr;
	if (SlashState == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.MissingSlashResolution")),
			TEXT("Slash damage continuation requires the current Slash resolution state."));
	}

	USGSSeat* Source = Context.GameContext->GetSeat(SlashState->SourceSeat);
	USGSSeat* Target = Context.GameContext->GetSeat(SlashState->TargetSeat);
	const USGSCard* Weapon = Source != nullptr
		? Source->Equipment.FindRef(SGSGameplayTags::EquipSlot_Weapon.GetTag()).Get()
		: nullptr;
	if (!SlashState->bKylinBowOffered && Weapon != nullptr && Weapon->CardName == TEXT("KylinBow") && Target != nullptr)
	{
		TArray<USGSCard*> Horses;
		for (const FGameplayTag Slot : {
			SGSGameplayTags::EquipSlot_DefenseHorse.GetTag(),
			SGSGameplayTags::EquipSlot_OffenseHorse.GetTag() })
		{
			if (USGSCard* Horse = Target->Equipment.FindRef(Slot).Get())
			{
				Horses.Add(Horse);
			}
		}
		if (!Horses.IsEmpty())
		{
			SlashState->bKylinBowOffered = true;
			FSGSRuleResponseWindowSpec Spec;
			Spec.SeatIndex = SlashState->SourceSeat;
			Spec.WindowName = KylinBowWindowName();
			Spec.ContextName = FName(TEXT("KylinBow"));
			Spec.EffectSourceSeat = SlashState->SourceSeat;
			Spec.EffectTargetSeat = SlashState->TargetSeat;
			Spec.bAllowPass = true;
			Spec.bIsCardChoice = true;
			Spec.ChoiceName = KylinBowChoiceName();
			Spec.MinChoiceCount = 1;
			Spec.MaxChoiceCount = 1;
			for (const USGSCard* Horse : Horses)
			{
				Spec.CardChoiceOptions.Add({ Horse->CardId, Horse->CardName, Horse->Suit, Horse->Number, true });
			}
			if (Context.Runtime->OpenResponseWindow(Spec))
			{
				return MakeValue();
			}
		}
	}

	return AdvanceSlashTarget(Context, FName(TEXT("SGS.Resolution.SlashHitResolved")));
}

FSGSStatus SGSBasicCardRuleHelpers::ResolveSuccessfulSlashDodge(FSGSRuleExecutionContext& Context)
{
	FSGSResolutionFrame* SlashFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSSlashResolutionState* SlashState = SlashFrame != nullptr
		? SlashFrame->GetMutableState<FSGSSlashResolutionState>()
		: nullptr;
	if (SlashState == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.MissingSlashResolution")),
			TEXT("Successful Dodge requires the current Slash resolution state."));
	}

	++SlashState->DodgeCount;
	if (SlashState->DodgeCount < SlashState->RequiredDodgeCount)
	{
		FSGSRuleResponseWindowSpec WindowSpec;
		WindowSpec.SeatIndex = SlashState->TargetSeat;
		WindowSpec.WindowName = SlashDodgeWindowName();
		WindowSpec.RequiredCardName = FName(TEXT("Dodge"));
		WindowSpec.AcceptedCardNames.Add(FName(TEXT("Dodge")));
		WindowSpec.ContextName = FName(TEXT("Slash"));
		WindowSpec.EffectSourceSeat = SlashState->SourceSeat;
		WindowSpec.EffectTargetSeat = SlashState->TargetSeat;
		if (Context.Runtime->OpenResponseWindow(WindowSpec))
		{
			return MakeValue();
		}
		return ResolveSlashHit(Context);
	}

	const USGSSeat* Source = Context.GameContext->GetSeat(SlashState->SourceSeat);
	const USGSCard* Weapon = Source != nullptr
		? Source->Equipment.FindRef(SGSGameplayTags::EquipSlot_Weapon.GetTag()).Get()
		: nullptr;
	if (!SlashState->bAxeOffered && Weapon != nullptr && Weapon->CardName == TEXT("Axe"))
	{
		TArray<USGSCard*> CostCards = Context.GameContext->GetCardsInZone(
			SGSGameplayTags::CardZone_Hand.GetTag(), SlashState->SourceSeat);
		CostCards.Append(Context.GameContext->GetCardsInZone(
			SGSGameplayTags::CardZone_Equipment.GetTag(), SlashState->SourceSeat));
		if (CostCards.Num() >= 2)
		{
			SlashState->bAxeOffered = true;
			FSGSRuleResponseWindowSpec Spec;
			Spec.SeatIndex = SlashState->SourceSeat;
			Spec.WindowName = AxeWindowName();
			Spec.ContextName = FName(TEXT("Axe"));
			Spec.EffectSourceSeat = SlashState->SourceSeat;
			Spec.EffectTargetSeat = SlashState->TargetSeat;
			Spec.bAllowPass = true;
			Spec.bIsCardChoice = true;
			Spec.ChoiceName = AxeChoiceName();
			Spec.MinChoiceCount = 2;
			Spec.MaxChoiceCount = 2;
			for (const USGSCard* Card : CostCards)
			{
				if (Card != nullptr)
				{
					Spec.CardChoiceOptions.Add({ Card->CardId, Card->CardName, Card->Suit, Card->Number, true });
				}
			}
			if (Context.Runtime->OpenResponseWindow(Spec))
			{
				return MakeValue();
			}
		}
	}
	if (!SlashState->bBladeOffered && Weapon != nullptr && Weapon->CardName == TEXT("Blade"))
	{
		bool bCanContinue = Context.GameContext->GetCardsInZone(
			SGSGameplayTags::CardZone_Hand.GetTag(), SlashState->SourceSeat).ContainsByPredicate(
			[](const USGSCard* Card) { return Card != nullptr && Card->CardName == TEXT("Slash"); });
		FSGSRuleResponseWindowSpec Spec;
		Spec.SeatIndex = SlashState->SourceSeat;
		Spec.WindowName = BladeWindowName();
		Spec.RequiredCardName = FName(TEXT("Slash"));
		Spec.AcceptedCardNames.Add(FName(TEXT("Slash")));
		Spec.ContextName = FName(TEXT("Blade"));
		Spec.EffectSourceSeat = SlashState->SourceSeat;
		Spec.EffectTargetSeat = SlashState->TargetSeat;
		Spec.bAllowPass = true;
		if (!bCanContinue && Context.RuleRegistry != nullptr)
		{
			FSGSRuleQueryContext QueryContext;
			QueryContext.GameContext = Context.GameContext;
			QueryContext.ActiveEffects = Context.ActiveEffects;
			QueryContext.RuleRegistry = Context.RuleRegistry;
			QueryContext.Phase = Context.TimingPoint.Phase;
			QueryContext.ActorSeat = SlashState->SourceSeat;
			QueryContext.WindowName = BladeWindowName();
			QueryContext.RequiredCardName = FName(TEXT("Slash"));
			const TArray<FName> Accepted = { FName(TEXT("Slash")) };
			QueryContext.AcceptedCardNames = Accepted;
			FSGSSkillOptionQuery Query;
			Query.QueryName = SGSRuleQueries::ResponseSkillOptions();
			Query.ActorSeat = SlashState->SourceSeat;
			Query.WindowName = BladeWindowName();
			Query.RequiredCardName = FName(TEXT("Slash"));
			Query.AcceptedCardNames = Accepted;
			Spec.SkillOptions = Context.RuleRegistry->CollectSkillOptions(QueryContext, MoveTemp(Query));
			bCanContinue = !Spec.SkillOptions.IsEmpty();
		}
		if (bCanContinue)
		{
			SlashState->bBladeOffered = true;
			if (Context.Runtime->OpenResponseWindow(Spec))
			{
				return MakeValue();
			}
		}
	}
	return FinishDodgedSlash(Context);
}

FSGSStatus SGSBasicCardRuleHelpers::FinishDodgedSlash(FSGSRuleExecutionContext& Context)
{
	return AdvanceSlashTarget(Context, FName(TEXT("SGS.Resolution.SlashDodged")));
}

FSGSStatus SGSBasicCardRuleHelpers::RestartSlashAfterBlade(
	FSGSRuleExecutionContext& Context,
	TConstArrayView<int32> MaterialCardIds,
	int32 SlashCardId,
	bool bVirtualSlash)
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSSlashResolutionState* Previous = Frame != nullptr ? Frame->GetMutableState<FSGSSlashResolutionState>() : nullptr;
	if (Previous == nullptr)
	{
		return MakeRuleError(FName(TEXT("SGS.Rule.MissingSlashResolution")), TEXT("Blade requires an active Slash."));
	}
	const int32 SourceSeat = Previous->SourceSeat;
	const int32 TargetSeat = Previous->TargetSeat;
	if (FSGSStatus Status = DiscardSlashMaterials(Context); Status.HasError())
	{
		return Status;
	}
	TArray<USGSCard*> Materials;
	for (const int32 CardId : MaterialCardIds)
	{
		Materials.Add(Context.GameContext->FindCardById(CardId));
	}
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			Materials, SGSGameplayTags::CardZone_Hand.GetTag(), SourceSeat,
			SGSGameplayTags::CardZone_Processing.GetTag(), INDEX_NONE,
			{ SGSCardMoveReasons::Use(), { TargetSeat } }),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}

	FSGSSlashResolutionState Next;
	Next.SlashCardId = SlashCardId;
	Next.bVirtualSlash = bVirtualSlash;
	Next.MaterialCardIds.Append(MaterialCardIds.GetData(), MaterialCardIds.Num());
	Next.SourceSeat = SourceSeat;
	Next.TargetSeat = TargetSeat;
	Next.TargetSeatIndices.Add(TargetSeat);
	Next.DamageAmount = 1;
	FSGSNumericRuleQuery CountQuery;
	CountQuery.QueryName = SGSRuleQueries::RequiredResponseCount();
	CountQuery.ActorSeat = SourceSeat;
	CountQuery.TargetSeat = TargetSeat;
	CountQuery.CardName = FName(TEXT("Dodge"));
	CountQuery.BaseValue = 1;
	FSGSRuleQueryContext QueryContext;
	QueryContext.GameContext = Context.GameContext;
	QueryContext.ActiveEffects = Context.ActiveEffects;
	QueryContext.RuleRegistry = Context.RuleRegistry;
	QueryContext.Phase = Context.TimingPoint.Phase;
	QueryContext.ActorSeat = SourceSeat;
	Next.RequiredDodgeCount = Context.RuleRegistry != nullptr
		? Context.RuleRegistry->ApplyNumericModifiers(QueryContext, CountQuery)
		: 1;
	Frame->ProcessingCardId = SlashCardId;
	Frame->FrameState = FInstancedStruct::Make(Next);

	FSGSRuleResponseWindowSpec Spec;
	Spec.SeatIndex = TargetSeat;
	Spec.WindowName = SlashDodgeWindowName();
	Spec.RequiredCardName = FName(TEXT("Dodge"));
	Spec.AcceptedCardNames.Add(FName(TEXT("Dodge")));
	Spec.ContextName = FName(TEXT("Slash"));
	Spec.EffectSourceSeat = SourceSeat;
	Spec.EffectTargetSeat = TargetSeat;
	return Context.Runtime->OpenResponseWindow(Spec) ? MakeValue() : ResolveSlashHit(Context);
}

FSGSStatus SGSBasicCardRuleHelpers::ValidateSlashUse(
	FSGSRuleExecutionContext& Context,
	USGSCard* MaterialCard,
	TConstArrayView<int32> TargetSeatIndices)
{
	if (MaterialCard == nullptr)
	{
		return MakeRuleError(FName(TEXT("SGS.Rule.InvalidSlash")), TEXT("Physical Slash requires one material card."));
	}
	return ValidateVirtualSlashUse(Context, GetCommandSeat(Context), TargetSeatIndices);
}

FSGSStatus SGSBasicCardRuleHelpers::ValidateVirtualSlashUse(
	FSGSRuleExecutionContext& Context,
	int32 SourceSeat,
	TConstArrayView<int32> TargetSeatIndices)
{
	FSGSNumericRuleQuery TargetCountQuery;
	TargetCountQuery.QueryName = SGSRuleQueries::SlashTargetCount();
	TargetCountQuery.ActorSeat = SourceSeat;
	TargetCountQuery.CardName = FName(TEXT("Slash"));
	TargetCountQuery.BaseValue = 1;
	FSGSRuleQueryContext QueryContext;
	QueryContext.GameContext = Context.GameContext;
	QueryContext.ActiveEffects = Context.ActiveEffects;
	QueryContext.RuleRegistry = Context.RuleRegistry;
	QueryContext.Phase = Context.TimingPoint.Phase;
	QueryContext.ActorSeat = SourceSeat;
	const int32 MaxTargetCount = Context.RuleRegistry != nullptr
		? Context.RuleRegistry->ApplyNumericModifiers(QueryContext, TargetCountQuery)
		: 1;
	if (TargetSeatIndices.IsEmpty() || TargetSeatIndices.Num() > MaxTargetCount)
	{
		return MakeRuleError(FName(TEXT("SGS.Rule.InvalidSlash")), TEXT("Slash target count is outside the current limit."));
	}
	TSet<int32> UniqueTargets;
	for (const int32 TargetSeat : TargetSeatIndices)
	{
		const USGSSeat* Target = Context.GameContext->GetSeat(TargetSeat);
		if (Target == nullptr || !Target->bIsAlive || TargetSeat == SourceSeat || UniqueTargets.Contains(TargetSeat))
		{
			return MakeRuleError(FName(TEXT("SGS.Rule.InvalidTarget")), TEXT("Slash targets must be distinct living non-self seats."));
		}
		UniqueTargets.Add(TargetSeat);
	}

	FSGSNumericRuleQuery LimitQuery;
	LimitQuery.QueryName = SGSRuleQueries::SlashUseLimit();
	LimitQuery.ActorSeat = SourceSeat;
	LimitQuery.TargetSeat = TargetSeatIndices[0];
	LimitQuery.CardName = FName(TEXT("Slash"));
	LimitQuery.BaseValue = 1;
	const int32 SlashLimit = Context.RuleRegistry != nullptr
		? Context.RuleRegistry->ApplyNumericModifiers(QueryContext, LimitQuery)
		: LimitQuery.BaseValue;
	if (CountStatus(Context, SourceSeat, SGSGameplayTags::Status_SlashUsed.GetTag()) >= SlashLimit)
	{
		return MakeRuleError(FName(TEXT("SGS.Rule.SlashAlreadyUsed")), TEXT("The acting seat has reached its Slash limit."));
	}

	FSGSNumericRuleQuery DistanceQuery;
	DistanceQuery.QueryName = SGSRuleQueries::SlashTargetDistance();
	DistanceQuery.ActorSeat = SourceSeat;
	DistanceQuery.TargetSeat = TargetSeatIndices[0];
	DistanceQuery.CardName = FName(TEXT("Slash"));
	DistanceQuery.BaseValue = 1;
	const int32 SlashDistance = Context.RuleRegistry != nullptr
		? Context.RuleRegistry->ApplyNumericModifiers(QueryContext, DistanceQuery)
		: DistanceQuery.BaseValue;
	for (const int32 TargetSeat : TargetSeatIndices)
	{
		if (Context.GameContext->GetDistance(SourceSeat, TargetSeat) > SlashDistance)
		{
			return MakeRuleError(FName(TEXT("SGS.Rule.TargetOutOfDistance")), TEXT("Slash target is out of distance."));
		}
	}

	return MakeValue();
}

FSGSStatus SGSBasicCardRuleHelpers::ExecuteSlashUse(
	FSGSRuleExecutionContext& Context,
	USGSCard* MaterialCard,
	TConstArrayView<int32> TargetSeatIndices,
	FName SourceRuleName)
{
	const int32 SourceSeat = GetCommandSeat(Context);
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ MaterialCard },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		SourceSeat,
		SGSGameplayTags::CardZone_Processing.GetTag(),
		INDEX_NONE,
		{ SGSCardMoveReasons::Use(), TArray<int32>(TargetSeatIndices) }),
		GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}
	return StartSlashResolution(
		Context,
		SourceSeat,
		TargetSeatIndices,
		MaterialCard->CardId,
		false,
		SourceRuleName,
		{ MaterialCard->CardId });
}

FSGSStatus SGSBasicCardRuleHelpers::ExecuteVirtualSlashUse(
	FSGSRuleExecutionContext& Context,
	TConstArrayView<int32> MaterialCardIds,
	TConstArrayView<int32> TargetSeatIndices,
	FName SourceRuleName)
{
	const int32 SourceSeat = GetCommandSeat(Context);
	TArray<USGSCard*> Materials;
	Materials.Reserve(MaterialCardIds.Num());
	for (const int32 CardId : MaterialCardIds)
	{
		USGSCard* Card = Context.GameContext->FindCardById(CardId);
		if (Card == nullptr)
		{
			return MakeRuleError(FName(TEXT("SGS.Rule.InvalidSlashMaterial")), TEXT("Virtual Slash material is missing."));
		}
		Materials.Add(Card);
	}
	if (!Materials.IsEmpty())
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				Materials,
				SGSGameplayTags::CardZone_Hand.GetTag(),
				SourceSeat,
				SGSGameplayTags::CardZone_Processing.GetTag(),
				INDEX_NONE,
				{ SGSCardMoveReasons::Use(), TArray<int32>(TargetSeatIndices) }),
			GetCommandId(Context));
			Status.HasError())
		{
			return Status;
		}
	}
	return StartSlashResolution(
		Context,
		SourceSeat,
		TargetSeatIndices,
		INDEX_NONE,
		true,
		SourceRuleName,
		MaterialCardIds);
}

FSGSStatus SGSBasicCardRuleHelpers::StartSlashResolution(
	FSGSRuleExecutionContext& Context,
	int32 SourceSeat,
	int32 TargetSeat,
	int32 SlashCardId,
	bool bVirtualSlash,
	FName SourceRuleName,
	TConstArrayView<int32> MaterialCardIds)
{
	const TArray<int32> Targets = { TargetSeat };
	return StartSlashResolution(
		Context,
		SourceSeat,
		Targets,
		SlashCardId,
		bVirtualSlash,
		SourceRuleName,
		MaterialCardIds);
}

FSGSStatus SGSBasicCardRuleHelpers::StartSlashResolution(
	FSGSRuleExecutionContext& Context,
	int32 SourceSeat,
	TConstArrayView<int32> TargetSeatIndices,
	int32 SlashCardId,
	bool bVirtualSlash,
	FName SourceRuleName,
	TConstArrayView<int32> MaterialCardIds)
{
	check(!TargetSeatIndices.IsEmpty());
	const int32 TargetSeat = TargetSeatIndices[0];
	FSGSSlashResolutionState SlashState;
	SlashState.SlashCardId = SlashCardId;
	SlashState.bVirtualSlash = bVirtualSlash;
	SlashState.MaterialCardIds.Append(MaterialCardIds.GetData(), MaterialCardIds.Num());
	if (!bVirtualSlash && SlashCardId != INDEX_NONE && SlashState.MaterialCardIds.IsEmpty())
	{
		SlashState.MaterialCardIds.Add(SlashCardId);
	}
	SlashState.SourceSeat = SourceSeat;
	SlashState.TargetSeat = TargetSeat;
	SlashState.TargetSeatIndices.Append(TargetSeatIndices.GetData(), TargetSeatIndices.Num());
	const USGSSeat* Source = Context.GameContext->GetSeat(SourceSeat);
	const USGSCard* Weapon = Source != nullptr
		? Source->Equipment.FindRef(SGSGameplayTags::EquipSlot_Weapon.GetTag()).Get()
		: nullptr;
	SlashState.bIgnoreArmor = Weapon != nullptr && Weapon->CardName == TEXT("QinggangSword");
	SlashState.DamageAmount = ConsumeStatus(Context, SourceSeat, SGSGameplayTags::Status_AnalepticBoost.GetTag()) ? 2 : 1;
	FSGSNumericRuleQuery ResponseCountQuery;
	ResponseCountQuery.QueryName = SGSRuleQueries::RequiredResponseCount();
	ResponseCountQuery.ActorSeat = SourceSeat;
	ResponseCountQuery.TargetSeat = TargetSeat;
	ResponseCountQuery.CardName = FName(TEXT("Dodge"));
	ResponseCountQuery.BaseValue = 1;
	FSGSRuleQueryContext ResponseQueryContext;
	ResponseQueryContext.GameContext = Context.GameContext;
	ResponseQueryContext.ActiveEffects = Context.ActiveEffects;
	ResponseQueryContext.RuleRegistry = Context.RuleRegistry;
	ResponseQueryContext.Phase = Context.TimingPoint.Phase;
	ResponseQueryContext.ActorSeat = SourceSeat;
	SlashState.RequiredDodgeCount = Context.RuleRegistry != nullptr
		? Context.RuleRegistry->ApplyNumericModifiers(ResponseQueryContext, ResponseCountQuery)
		: 1;
	AddStatus(
		Context,
		SourceSeat,
		FName(TEXT("SGS.ActiveEffect.SlashUsed")),
		SGSGameplayTags::Status_SlashUsed.GetTag(),
		FSGSDurationSpec::ThisPhase(SourceSeat, Context.TimingPoint));

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = SourceRuleName;
	Frame.SourceCommandId = GetCommandId(Context);
	Frame.ActorSeat = SourceSeat;
	Frame.SourceSeat = SourceSeat;
	Frame.TargetSeat = TargetSeat;
	Frame.ProcessingCardId = SlashCardId;
	Frame.OnChildCompletedContinuation = SGSResolutionContinuations::FinishParentCardResolution();
	Frame.FrameState = FInstancedStruct::Make(SlashState);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));
	return ContinueCurrentSlashTarget(Context);
}
