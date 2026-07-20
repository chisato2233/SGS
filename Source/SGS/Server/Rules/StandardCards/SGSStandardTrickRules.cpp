#include "Server/Rules/StandardCards/SGSStandardTrickRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Cards/SGSStandardCardDefinitions.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"

namespace
{
FName NullificationStage() { return FName(TEXT("SGS.Trick.Stage.Nullification")); }
FName EffectResponseStage() { return FName(TEXT("SGS.Trick.Stage.EffectResponse")); }
FName DuelStage() { return FName(TEXT("SGS.Trick.Stage.Duel")); }

FSGSStandardTrickResolutionState* GetState(FSGSRuleExecutionContext& Context)
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	return Frame != nullptr ? Frame->GetMutableState<FSGSStandardTrickResolutionState>() : nullptr;
}

bool HasHandCardNamed(const USGSGameContext& Context, int32 SeatIndex, FName CardName)
{
	for (USGSCard* Card : Context.GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), SeatIndex))
	{
		if (Card != nullptr && Card->CardName == CardName)
		{
			return true;
		}
	}
	return false;
}

bool CanRespondWith(FSGSRuleExecutionContext& Context, int32 SeatIndex, FName CardName, FName WindowName)
{
	if (HasHandCardNamed(*Context.GameContext, SeatIndex, CardName))
	{
		return true;
	}
	if (Context.RuleRegistry == nullptr)
	{
		return false;
	}
	FSGSRuleQueryContext QueryContext;
	QueryContext.GameContext = Context.GameContext;
	QueryContext.ActiveEffects = Context.ActiveEffects;
	QueryContext.RuleRegistry = Context.RuleRegistry;
	QueryContext.Phase = Context.TimingPoint.Phase;
	QueryContext.ActorSeat = SeatIndex;
	QueryContext.WindowName = WindowName;
	QueryContext.RequiredCardName = CardName;
	const TArray<FName> AcceptedCards = { CardName };
	QueryContext.AcceptedCardNames = AcceptedCards;
	FSGSSkillOptionQuery Query;
	Query.QueryName = SGSRuleQueries::ResponseSkillOptions();
	Query.ActorSeat = SeatIndex;
	Query.WindowName = WindowName;
	Query.RequiredCardName = CardName;
	Query.AcceptedCardNames = AcceptedCards;
	return !Context.RuleRegistry->CollectSkillOptions(QueryContext, MoveTemp(Query)).IsEmpty();
}

int32 CurrentTarget(const FSGSStandardTrickResolutionState& State)
{
	return State.TargetSeatIndices.IsValidIndex(State.TargetIndex)
		? State.TargetSeatIndices[State.TargetIndex]
		: INDEX_NONE;
}

FSGSStatus FinishTrick(FSGSRuleExecutionContext& Context)
{
	if (FSGSStatus Status = SGSBasicCardRuleHelpers::DiscardProcessingCard(Context); Status.HasError())
	{
		return Status;
	}
	return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.StandardTrickComplete")));
}

FSGSStatus AdvanceTarget(FSGSRuleExecutionContext& Context)
{
	FSGSStandardTrickResolutionState* State = GetState(Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(FName(TEXT("SGS.Trick.MissingState")), TEXT("Standard trick frame has no state.")));
	}
	++State->TargetIndex;
	State->Stage = NAME_None;
	State->RequiredResponseCard = NAME_None;
	return SGSStandardTrickRules::Continue(Context);
}

FSGSStatus ApplyTrickDamage(FSGSRuleExecutionContext& Context, int32 SourceSeat, int32 TargetSeat)
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSStandardTrickResolutionState* State = Frame != nullptr
		? Frame->GetMutableState<FSGSStandardTrickResolutionState>()
		: nullptr;
	if (Frame == nullptr || State == nullptr)
	{
		return MakeError(FSGSError::Make(FName(TEXT("SGS.Trick.MissingState")), TEXT("Damage requires a standard trick frame.")));
	}

	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeDamageStep(SourceSeat, TargetSeat, 1),
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
	DamageData.CardId = Frame->ProcessingCardId;
	DamageData.Amount = 1;
	DamageAfter.EventData = FInstancedStruct::Make(DamageData);
	if (FSGSStatus Status = Context.Runtime->PublishTimingEvent(DamageAfter); Status.HasError())
	{
		return Status;
	}

	const USGSSeat* Target = Context.GameContext->GetSeat(TargetSeat);
	if (Target != nullptr && Target->bIsAlive && Target->Health <= 0)
	{
		Frame->OnChildCompletedContinuation = SGSStandardTrickRules::ResumeContinuation();
		return SGSBasicCardRuleHelpers::StartDyingPeachResolution(Context, TargetSeat);
	}
	return SGSStandardTrickRules::Continue(Context);
}

FSGSStatus ResolveCurrentEffect(FSGSRuleExecutionContext& Context)
{
	FSGSStandardTrickResolutionState* State = GetState(Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(FName(TEXT("SGS.Trick.MissingState")), TEXT("Standard trick frame has no state.")));
	}
	const int32 TargetSeat = CurrentTarget(*State);
	const int32 SourceSeat = State->SourceSeat;

	if (State->CardName == TEXT("ExNihilo"))
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeDrawCardsStep(TargetSeat, 2),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
		return AdvanceTarget(Context);
	}
	if (State->CardName == TEXT("GodSalvation"))
	{
		const USGSSeat* Target = Context.GameContext->GetSeat(TargetSeat);
		if (Target != nullptr && Target->Health < Target->MaxHealth)
		{
			if (FSGSStatus Status = Context.Runtime->RunEffectStep(
				SGSStandardEffectSteps::MakeHealStep(TargetSeat, 1),
				Context.RuleInvocation.SourceCommandId);
				Status.HasError())
			{
				return Status;
			}
		}
		return AdvanceTarget(Context);
	}
	if (State->CardName == TEXT("AmazingGrace"))
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeDrawCardsStep(TargetSeat, 1),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
		return AdvanceTarget(Context);
	}
	if (State->CardName == TEXT("Dismantlement") || State->CardName == TEXT("Snatch"))
	{
		USGSCard* Chosen = nullptr;
		FSGSCardZone FromZone = SGSGameplayTags::CardZone_None.GetTag();
		for (const FSGSCardZone Zone : {
			SGSGameplayTags::CardZone_Equipment.GetTag(),
			SGSGameplayTags::CardZone_Judgement.GetTag(),
			SGSGameplayTags::CardZone_Hand.GetTag() })
		{
			TArray<USGSCard*> Cards = Context.GameContext->GetCardsInZone(Zone, TargetSeat);
			if (!Cards.IsEmpty())
			{
				Chosen = Cards[0];
				FromZone = Zone;
				break;
			}
		}
		if (Chosen != nullptr)
		{
			const bool bSnatch = State->CardName == TEXT("Snatch");
			if (FSGSStatus Status = Context.Runtime->RunEffectStep(
				SGSStandardEffectSteps::MakeMoveCardsStep(
					{ Chosen },
					FromZone,
					TargetSeat,
					bSnatch ? SGSGameplayTags::CardZone_Hand.GetTag() : SGSGameplayTags::CardZone_DiscardPile.GetTag(),
					bSnatch ? SourceSeat : INDEX_NONE,
					{ bSnatch ? SGSCardMoveReasons::Gain() : SGSCardMoveReasons::Discard(), { TargetSeat } }),
				Context.RuleInvocation.SourceCommandId);
				Status.HasError())
			{
				return Status;
			}
		}
		return AdvanceTarget(Context);
	}
	if (State->CardName == TEXT("Indulgence")
		|| State->CardName == TEXT("SupplyShortage")
		|| State->CardName == TEXT("Lightning"))
	{
		FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
		USGSCard* TrickCard = Frame != nullptr ? Context.GameContext->FindCardById(Frame->ProcessingCardId) : nullptr;
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				{ TrickCard },
				SGSGameplayTags::CardZone_Processing.GetTag(),
				INDEX_NONE,
				SGSGameplayTags::CardZone_Judgement.GetTag(),
				TargetSeat,
				{ SGSCardMoveReasons::Use(), { TargetSeat } }),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
		return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.DelayedTrickPlaced")));
	}

	if (State->CardName == TEXT("SavageAssault") || State->CardName == TEXT("ArcheryAttack"))
	{
		State->Stage = EffectResponseStage();
		State->RequiredResponseCard = State->CardName == TEXT("SavageAssault") ? FName(TEXT("Slash")) : FName(TEXT("Dodge"));
		if (CanRespondWith(Context, TargetSeat, State->RequiredResponseCard, SGSStandardTrickRules::EffectResponseWindow()))
		{
			FSGSRuleResponseWindowSpec Spec;
			Spec.SeatIndex = TargetSeat;
			Spec.WindowName = SGSStandardTrickRules::EffectResponseWindow();
			Spec.RequiredCardName = State->RequiredResponseCard;
			Spec.AcceptedCardNames.Add(State->RequiredResponseCard);
			Spec.ContextName = State->CardName;
			Spec.EffectSourceSeat = SourceSeat;
			Spec.EffectTargetSeat = TargetSeat;
			if (Context.Runtime->OpenResponseWindow(Spec))
			{
				return MakeValue();
			}
		}
		++State->TargetIndex;
		State->Stage = NAME_None;
		return ApplyTrickDamage(Context, SourceSeat, TargetSeat);
	}

	if (State->CardName == TEXT("Duel"))
	{
		State->Stage = DuelStage();
		State->RequiredResponseCard = FName(TEXT("Slash"));
		State->DuelResponderSeat = TargetSeat;
		State->DuelOpponentSeat = SourceSeat;
		return SGSStandardTrickRules::Continue(Context);
	}

	return AdvanceTarget(Context);
}

FSGSStatus ContinueNullification(FSGSRuleExecutionContext& Context)
{
	FSGSStandardTrickResolutionState* State = GetState(Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(FName(TEXT("SGS.Trick.MissingState")), TEXT("Standard trick frame has no state.")));
	}
	const int32 SeatCount = Context.GameContext->NumSeats();
	while (State->NullificationVisited < SeatCount)
	{
		const int32 ResponderSeat = State->NullificationNextSeat;
		State->NullificationNextSeat = (State->NullificationNextSeat + 1) % SeatCount;
		++State->NullificationVisited;
		const USGSSeat* Seat = Context.GameContext->GetSeat(ResponderSeat);
		if (Seat == nullptr || !Seat->bIsAlive
			|| !HasHandCardNamed(*Context.GameContext, ResponderSeat, FName(TEXT("Nullification"))))
		{
			continue;
		}

		FSGSRuleResponseWindowSpec Spec;
		Spec.SeatIndex = ResponderSeat;
		Spec.WindowName = SGSStandardTrickRules::NullificationWindow();
		Spec.RequiredCardName = FName(TEXT("Nullification"));
		Spec.AcceptedCardNames.Add(FName(TEXT("Nullification")));
		Spec.ContextName = State->CardName;
		Spec.EffectSourceSeat = State->SourceSeat;
		Spec.EffectTargetSeat = CurrentTarget(*State);
		if (Context.Runtime->OpenResponseWindow(Spec))
		{
			return MakeValue();
		}
	}

	if (State->bNullified)
	{
		return AdvanceTarget(Context);
	}
	return ResolveCurrentEffect(Context);
}

TArray<int32> MakeTargets(const USGSGameContext& Context, int32 SourceSeat, FName TargetMode, TConstArrayView<int32> ExplicitTargets)
{
	TArray<int32> Targets;
	if (TargetMode == SGSCardTargetModes::Self() || TargetMode == SGSCardTargetModes::Other())
	{
		Targets.Append(ExplicitTargets.GetData(), ExplicitTargets.Num());
		return Targets;
	}
	if (TargetMode == SGSCardTargetModes::AllAlive())
	{
		for (int32 Offset = 0; Offset < Context.NumSeats(); ++Offset)
		{
			const int32 SeatIndex = (SourceSeat + Offset) % Context.NumSeats();
			const USGSSeat* Seat = Context.GetSeat(SeatIndex);
			if (Seat != nullptr && Seat->bIsAlive)
			{
				Targets.Add(SeatIndex);
			}
		}
	}
	else if (TargetMode == SGSCardTargetModes::AllOthers())
	{
		for (int32 Offset = 1; Offset < Context.NumSeats(); ++Offset)
		{
			const int32 SeatIndex = (SourceSeat + Offset) % Context.NumSeats();
			const USGSSeat* Seat = Context.GetSeat(SeatIndex);
			if (Seat != nullptr && Seat->bIsAlive)
			{
				Targets.Add(SeatIndex);
			}
		}
	}
	return Targets;
}
}

FName SGSStandardTrickRules::NullificationWindow() { return FName(TEXT("Trick.Nullification")); }
FName SGSStandardTrickRules::EffectResponseWindow() { return FName(TEXT("Trick.EffectResponse")); }
FName SGSStandardTrickRules::ResumeContinuation() { return FName(TEXT("SGS.Resolution.Continuation.ResumeStandardTrick")); }

FSGSStatus SGSStandardTrickRules::Continue(FSGSRuleExecutionContext& Context)
{
	FSGSStandardTrickResolutionState* State = GetState(Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(FName(TEXT("SGS.Trick.MissingState")), TEXT("Standard trick frame has no state.")));
	}
	if (State->TargetIndex >= State->TargetSeatIndices.Num())
	{
		return FinishTrick(Context);
	}
	if (State->Stage == DuelStage())
	{
		const int32 Responder = State->DuelResponderSeat;
		if (State->ResponseCount == 0)
		{
			FSGSNumericRuleQuery CountQuery;
			CountQuery.QueryName = SGSRuleQueries::RequiredResponseCount();
			CountQuery.ActorSeat = State->DuelOpponentSeat;
			CountQuery.TargetSeat = Responder;
			CountQuery.CardName = FName(TEXT("Slash"));
			CountQuery.BaseValue = 1;
			FSGSRuleQueryContext QueryContext;
			QueryContext.GameContext = Context.GameContext;
			QueryContext.ActiveEffects = Context.ActiveEffects;
			QueryContext.RuleRegistry = Context.RuleRegistry;
			QueryContext.Phase = Context.TimingPoint.Phase;
			QueryContext.ActorSeat = State->DuelOpponentSeat;
			State->RequiredResponseCount = Context.RuleRegistry != nullptr
				? Context.RuleRegistry->ApplyNumericModifiers(QueryContext, CountQuery)
				: 1;
		}
		if (CanRespondWith(Context, Responder, FName(TEXT("Slash")), EffectResponseWindow()))
		{
			FSGSRuleResponseWindowSpec Spec;
			Spec.SeatIndex = Responder;
			Spec.WindowName = EffectResponseWindow();
			Spec.RequiredCardName = FName(TEXT("Slash"));
			Spec.AcceptedCardNames.Add(FName(TEXT("Slash")));
			Spec.ContextName = FName(TEXT("Duel"));
			Spec.EffectSourceSeat = State->DuelOpponentSeat;
			Spec.EffectTargetSeat = Responder;
			if (Context.Runtime->OpenResponseWindow(Spec))
			{
				return MakeValue();
			}
		}
		const int32 DamagedSeat = Responder;
		const int32 DamageSource = State->DuelOpponentSeat;
		++State->TargetIndex;
		State->Stage = NAME_None;
		return ApplyTrickDamage(Context, DamageSource, DamagedSeat);
	}
	if (State->Stage == NullificationStage())
	{
		return ContinueNullification(Context);
	}
	if (State->Stage == EffectResponseStage())
	{
		return MakeValue();
	}

	State->Stage = NullificationStage();
	State->NullificationNextSeat = State->SourceSeat;
	State->NullificationVisited = 0;
	State->bNullified = false;
	return ContinueNullification(Context);
}

FSGSStatus SGSStandardTrickRules::ContinueAfterAcceptedResponse(FSGSRuleExecutionContext& Context)
{
	FSGSStandardTrickResolutionState* State = GetState(Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(FName(TEXT("SGS.Trick.MissingState")), TEXT("Accepted response has no standard trick state.")));
	}
	if (State->Stage == DuelStage())
	{
		if (++State->ResponseCount < State->RequiredResponseCount)
		{
			return Continue(Context);
		}
		State->ResponseCount = 0;
		State->RequiredResponseCount = 1;
		Swap(State->DuelResponderSeat, State->DuelOpponentSeat);
		return Continue(Context);
	}
	return AdvanceTarget(Context);
}

FSGSStatus SGSStandardTrickRules::ContinueAfterDeclinedResponse(FSGSRuleExecutionContext& Context)
{
	FSGSStandardTrickResolutionState* State = GetState(Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(FName(TEXT("SGS.Trick.MissingState")), TEXT("Declined response has no standard trick state.")));
	}
	const int32 DamagedSeat = State->Stage == DuelStage() ? State->DuelResponderSeat : CurrentTarget(*State);
	const int32 DamageSource = State->Stage == DuelStage() ? State->DuelOpponentSeat : State->SourceSeat;
	++State->TargetIndex;
	State->Stage = NAME_None;
	return ApplyTrickDamage(Context, DamageSource, DamagedSeat);
}

FName FSGSStandardTrickUseRule::GetRuleName() const
{
	return FName(*FString::Printf(TEXT("SGS.Rule.Trick.%s"), *CardName.ToString()));
}

FSGSRuleDescriptor FSGSStandardTrickUseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Action(), SGSGameplayTags::PlayAction_UseCard.GetTag(), CardName, NAME_None, 100);
}

bool FSGSStandardTrickUseRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSUseCardRulePayload& Payload) const
{
	return Context.RuleInvocation.IntentTag.MatchesTagExact(SGSGameplayTags::PlayAction_UseCard.GetTag())
		&& Payload.CardName == CardName;
}

FSGSStatus FSGSStandardTrickUseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSUseCardRulePayload& Payload) const
{
	const FSGSStandardCardDefinition* Definition = SGSStandardCardDefinitions::Find(CardName);
	USGSCard* Card = Context.GameContext->FindCardById(Payload.CardId);
	if (Definition == nullptr || Card == nullptr || Card->CardName != CardName)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Rule.InvalidTrick")), TEXT("Trick command does not match a standard trick card."));
	}
	if ((Definition->TargetMode == SGSCardTargetModes::Self()
			|| Definition->TargetMode == SGSCardTargetModes::Other())
		&& Payload.TargetSeatIndices.Num() != 1)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Rule.InvalidTarget")), TEXT("This trick requires exactly one target."));
	}
	return MakeValue();
}

FSGSStatus FSGSStandardTrickUseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSUseCardRulePayload& Payload) const
{
	USGSCard* Card = Context.GameContext->FindCardById(Payload.CardId);
	const int32 SourceSeat = Context.RuleInvocation.ActorSeat;
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			{ Card }, SGSGameplayTags::CardZone_Hand.GetTag(), SourceSeat,
			SGSGameplayTags::CardZone_Processing.GetTag(), INDEX_NONE,
			{ SGSCardMoveReasons::Use(), Payload.TargetSeatIndices }),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}

	const FSGSStandardCardDefinition* Definition = SGSStandardCardDefinitions::Find(CardName);
	FSGSStandardTrickResolutionState State;
	State.CardName = CardName;
	State.SourceSeat = SourceSeat;
	State.TargetSeatIndices = MakeTargets(*Context.GameContext, SourceSeat, Definition->TargetMode, Payload.TargetSeatIndices);

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = GetRuleName();
	Frame.SourceCommandId = Context.RuleInvocation.SourceCommandId;
	Frame.ActorSeat = SourceSeat;
	Frame.SourceSeat = SourceSeat;
	Frame.TargetSeat = State.TargetSeatIndices.IsEmpty() ? INDEX_NONE : State.TargetSeatIndices[0];
	Frame.ProcessingCardId = Card->CardId;
	Frame.FrameState = FInstancedStruct::Make(State);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));
	return SGSStandardTrickRules::Continue(Context);
}

FName FSGSTrickNullificationPassRule::GetRuleName() const { return FName(TEXT("SGS.Rule.Trick.Nullification.Pass")); }
FSGSRuleDescriptor FSGSTrickNullificationPassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_Pass.GetTag(), NAME_None,
		SGSStandardTrickRules::NullificationWindow(), 200);
}
FSGSStatus FSGSTrickNullificationPassRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload&) const
{
	return SGSStandardTrickRules::Continue(Context);
}

FName FSGSTrickNullificationResponseRule::GetRuleName() const { return FName(TEXT("SGS.Rule.Trick.Nullification.Response")); }
FSGSRuleDescriptor FSGSTrickNullificationResponseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(), FName(TEXT("Nullification")),
		SGSStandardTrickRules::NullificationWindow(), 210);
}
FSGSStatus FSGSTrickNullificationResponseRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	USGSCard* Card = Context.GameContext->FindCardById(Payload.CardId);
	return Card != nullptr && Card->CardName == TEXT("Nullification")
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Trick.InvalidNullification")), TEXT("Nullification response requires Nullification."));
}
FSGSStatus FSGSTrickNullificationResponseRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	if (FSGSStatus Status = SGSBasicCardRuleHelpers::DiscardHandCard(
		Context, Context.GameContext->FindCardById(Payload.CardId), Context.RuleInvocation.ActorSeat);
		Status.HasError())
	{
		return Status;
	}
	FSGSStandardTrickResolutionState* State = GetState(Context);
	State->bNullified = !State->bNullified;
	State->NullificationNextSeat = (Context.RuleInvocation.ActorSeat + 1) % Context.GameContext->NumSeats();
	State->NullificationVisited = 0;
	return SGSStandardTrickRules::Continue(Context);
}

FName FSGSTrickEffectPassRule::GetRuleName() const { return FName(TEXT("SGS.Rule.Trick.Effect.Pass")); }
FSGSRuleDescriptor FSGSTrickEffectPassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_Pass.GetTag(), NAME_None,
		SGSStandardTrickRules::EffectResponseWindow(), 200);
}
FSGSStatus FSGSTrickEffectPassRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload&) const
{
	return SGSStandardTrickRules::ContinueAfterDeclinedResponse(Context);
}

FName FSGSTrickEffectResponseRule::GetRuleName() const { return FName(TEXT("SGS.Rule.Trick.Effect.Response")); }
FSGSRuleDescriptor FSGSTrickEffectResponseRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor = SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(), NAME_None,
		SGSStandardTrickRules::EffectResponseWindow(), 210);
	Descriptor.bWildcardSubject = true;
	return Descriptor;
}
FSGSStatus FSGSTrickEffectResponseRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	FSGSStandardTrickResolutionState* State = GetState(Context);
	USGSCard* Card = Context.GameContext->FindCardById(Payload.CardId);
	return State != nullptr && Card != nullptr && Card->CardName == State->RequiredResponseCard
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Trick.InvalidEffectResponse")), TEXT("The response card does not satisfy this trick."));
}
FSGSStatus FSGSTrickEffectResponseRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	if (FSGSStatus Status = SGSBasicCardRuleHelpers::DiscardHandCard(
		Context, Context.GameContext->FindCardById(Payload.CardId), Context.RuleInvocation.ActorSeat);
		Status.HasError())
	{
		return Status;
	}
	return SGSStandardTrickRules::ContinueAfterAcceptedResponse(Context);
}

void SGSStandardTrickRules::Register(FSGSRuleRegistry& Registry)
{
	for (const FName CardName : {
		FName(TEXT("ExNihilo")), FName(TEXT("Dismantlement")), FName(TEXT("Snatch")), FName(TEXT("Duel")),
		FName(TEXT("SavageAssault")), FName(TEXT("ArcheryAttack")), FName(TEXT("GodSalvation")),
		FName(TEXT("AmazingGrace")), FName(TEXT("Indulgence")), FName(TEXT("SupplyShortage")), FName(TEXT("Lightning")) })
	{
		Registry.RegisterRule(MakeShared<FSGSStandardTrickUseRule>(CardName));
	}
	Registry.RegisterRule(MakeShared<FSGSTrickNullificationPassRule>());
	Registry.RegisterRule(MakeShared<FSGSTrickNullificationResponseRule>());
	Registry.RegisterRule(MakeShared<FSGSTrickEffectPassRule>());
	Registry.RegisterRule(MakeShared<FSGSTrickEffectResponseRule>());
}
