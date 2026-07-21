#include "Server/Rules/StandardCards/SGSEquipmentRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Actions/BasicCards/SGSSlashRule.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"
#include "Server/Rules/Resolution/SGSJudgementResolution.h"
#include "Server/Rules/Resolution/SGSLordAssistResolution.h"
#include "Server/Rules/StandardCards/SGSStandardTrickRules.h"

FName FSGSEquipmentUseRule::GetRuleName() const
{
	return FName(*FString::Printf(TEXT("SGS.Rule.Equipment.%s"), *Definition.CardName.ToString()));
}

FSGSRuleDescriptor FSGSEquipmentUseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_UseCard.GetTag(),
		Definition.CardName,
		NAME_None,
		100);
}

bool FSGSEquipmentUseRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSUseCardRulePayload& Payload) const
{
	return Context.RuleInvocation.IntentTag.MatchesTagExact(SGSGameplayTags::PlayAction_UseCard.GetTag())
		&& Payload.CardName == Definition.CardName;
}

FSGSStatus FSGSEquipmentUseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSUseCardRulePayload& Payload) const
{
	USGSCard* Card = Context.GameContext->FindCardById(Payload.CardId);
	if (Card == nullptr
		|| !Payload.TargetSeatIndices.IsEmpty()
		|| !Definition.EquipSlot.IsValid())
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidEquipment")),
			TEXT("Equipment requires its physical card and no target."));
	}
	return MakeValue();
}

FSGSStatus FSGSEquipmentUseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSUseCardRulePayload& Payload) const
{
	return Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeEquipCardStep(
			Context.RuleInvocation.ActorSeat,
			Context.GameContext->FindCardById(Payload.CardId),
			Definition.EquipSlot),
		Context.RuleInvocation.SourceCommandId);
}

FName FSGSEquipmentModifierRule::GetRuleName() const
{
	return FName(*FString::Printf(TEXT("SGS.Rule.Equipment.Modifier.%s"), *QueryName.ToString()));
}

FSGSRuleDescriptor FSGSEquipmentModifierRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor;
	Descriptor.RuleName = GetRuleName();
	Descriptor.RuleKindTag = SGSRuleKinds::Modifier();
	Descriptor.SubjectName = QueryName;
	Descriptor.Priority = 100;
	Descriptor.bWildcardIntent = true;
	Descriptor.bWildcardWindow = true;
	return Descriptor;
}

void FSGSEquipmentModifierRule::ModifyNumericQuery(
	const FSGSRuleQueryContext& Context,
	FSGSNumericRuleQuery& Query) const
{
	const USGSSeat* Seat = Context.GameContext != nullptr
		? Context.GameContext->GetSeat(Query.ActorSeat)
		: nullptr;
	if (Seat == nullptr)
	{
		return;
	}

	if (QueryName == SGSRuleQueries::SlashUseLimit())
	{
		const USGSCard* Weapon = Seat->Equipment.FindRef(SGSGameplayTags::EquipSlot_Weapon.GetTag()).Get();
		if (Weapon != nullptr && Weapon->CardName == TEXT("Crossbow"))
		{
			Query.Value = TNumericLimits<int32>::Max();
		}
		return;
	}
	if (QueryName == SGSRuleQueries::SlashTargetCount())
	{
		const USGSCard* Weapon = Seat->Equipment.FindRef(SGSGameplayTags::EquipSlot_Weapon.GetTag()).Get();
		if (Weapon != nullptr && Weapon->CardName == TEXT("Halberd")
			&& Context.GameContext->GetCardsInZone(
				SGSGameplayTags::CardZone_Hand.GetTag(), Query.ActorSeat).Num() == 1)
		{
			Query.Value = FMath::Max(Query.Value, 3);
		}
		return;
	}

	if (QueryName == SGSRuleQueries::SlashTargetDistance())
	{
		const USGSCard* Weapon = Seat->Equipment.FindRef(SGSGameplayTags::EquipSlot_Weapon.GetTag()).Get();
		const FSGSStandardCardDefinition* Definition = Weapon != nullptr
			? SGSStandardCardDefinitions::Find(Weapon->CardName)
			: nullptr;
		if (Definition != nullptr)
		{
			Query.Value = FMath::Max(Query.Value, Definition->AttackRange);
		}
	}
}

FName FSGSBaguaOptionRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Equipment.Bagua.Option"));
}

FSGSRuleDescriptor FSGSBaguaOptionRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor;
	Descriptor.RuleName = GetRuleName();
	Descriptor.RuleKindTag = SGSRuleKinds::ViewAs();
	Descriptor.SubjectName = FName(TEXT("BaguaArmor"));
	Descriptor.Priority = 100;
	Descriptor.bWildcardIntent = true;
	Descriptor.bWildcardWindow = true;
	return Descriptor;
}

void FSGSBaguaOptionRule::CollectSkillOptions(
	const FSGSRuleQueryContext& Context,
	FSGSSkillOptionQuery& Query) const
{
	if (Query.QueryName != SGSRuleQueries::ResponseSkillOptions()
		|| !Query.AcceptedCardNames.Contains(FName(TEXT("Dodge"))))
	{
		return;
	}
	if (Context.bArmorIgnored)
	{
		return;
	}
	const USGSSeat* Seat = Context.GameContext->GetSeat(Query.ActorSeat);
	const USGSCard* Armor = Seat != nullptr
		? Seat->Equipment.FindRef(SGSGameplayTags::EquipSlot_Armor.GetTag()).Get()
		: nullptr;
	if (Armor == nullptr || Armor->CardName != TEXT("BaguaArmor"))
	{
		return;
	}
	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("BaguaArmor"));
	Option.DisplayName = FName(TEXT("八卦阵"));
	Option.RuleKindTag = SGSRuleKinds::ViewAs();
	Option.ResultCardName = FName(TEXT("Dodge"));
	Option.MinCardCount = 0;
	Option.MaxCardCount = 0;
	Query.Options.Add(MoveTemp(Option));
}

FName FSGSBaguaResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Equipment.Bagua.Response"));
}

FSGSRuleDescriptor FSGSBaguaResponseRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor = SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Dodge")),
		NAME_None,
		400);
	Descriptor.bWildcardWindow = true;
	return Descriptor;
}

bool FSGSBaguaResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("BaguaArmor")
		&& Payload.CardName == TEXT("Dodge")
		&& (Context.RuleInvocation.WindowName == SGSBasicCardRuleHelpers::SlashDodgeWindowName()
			|| Context.RuleInvocation.WindowName == SGSStandardTrickRules::EffectResponseWindow()
			|| Context.RuleInvocation.WindowName == SGSLordAssistResolution::WindowName());
}

FSGSStatus FSGSBaguaResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	return SGSJudgementResolution::Start(
		Context,
		Context.RuleInvocation.ActorSeat,
		FName(TEXT("BaguaArmor")),
		SGSEquipmentRules::BaguaAfterJudgementContinuation());
}

FName SGSEquipmentRules::BaguaAfterJudgementContinuation()
{
	return FName(TEXT("SGS.Resolution.Continuation.BaguaAfterJudgement"));
}

FSGSStatus SGSEquipmentRules::ContinueBaguaAfterJudgement(
	FSGSRuleExecutionContext& Context,
	int32 ResultCardId)
{
	USGSCard* Card = Context.GameContext->FindCardById(ResultCardId);
	const bool bRed = Card != nullptr
		&& (Card->Suit.MatchesTagExact(SGSGameplayTags::Suit_Heart.GetTag())
			|| Card->Suit.MatchesTagExact(SGSGameplayTags::Suit_Diamond.GetTag()));
	if (Card != nullptr)
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				{ Card },
				SGSGameplayTags::CardZone_Processing.GetTag(),
				INDEX_NONE,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE,
				{ SGSCardMoveReasons::Cleanup(), {} }),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
	}

	if (Context.RuleInvocation.WindowName == SGSBasicCardRuleHelpers::SlashDodgeWindowName())
	{
		if (!bRed)
		{
			return SGSBasicCardRuleHelpers::ResolveSlashHit(Context);
		}
		return SGSBasicCardRuleHelpers::ResolveSuccessfulSlashDodge(Context);
	}
	if (Context.RuleInvocation.WindowName == SGSLordAssistResolution::WindowName())
	{
		return SGSLordAssistResolution::Continue(Context, bRed);
	}
	return bRed
		? SGSStandardTrickRules::ContinueAfterAcceptedResponse(Context)
		: SGSStandardTrickRules::ContinueAfterDeclinedResponse(Context);
}

namespace
{
const USGSCard* GetEquippedWeapon(const USGSGameContext* Context, int32 SeatIndex)
{
	const USGSSeat* Seat = Context != nullptr ? Context->GetSeat(SeatIndex) : nullptr;
	return Seat != nullptr ? Seat->Equipment.FindRef(SGSGameplayTags::EquipSlot_Weapon.GetTag()).Get() : nullptr;
}

bool HasSpear(const USGSGameContext* Context, int32 SeatIndex)
{
	const USGSCard* Weapon = GetEquippedWeapon(Context, SeatIndex);
	return Weapon != nullptr && Weapon->CardName == TEXT("Spear");
}
}

FName FSGSSpearOptionRule::GetRuleName() const { return FName(TEXT("SGS.Rule.Equipment.Spear.Option")); }
FSGSRuleDescriptor FSGSSpearOptionRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor;
	Descriptor.RuleName = GetRuleName();
	Descriptor.RuleKindTag = SGSRuleKinds::ViewAs();
	Descriptor.SubjectName = FName(TEXT("Spear"));
	Descriptor.Priority = 120;
	Descriptor.bWildcardIntent = true;
	Descriptor.bWildcardWindow = true;
	return Descriptor;
}
void FSGSSpearOptionRule::CollectSkillOptions(
	const FSGSRuleQueryContext& Context,
	FSGSSkillOptionQuery& Query) const
{
	const bool bPlay = Query.QueryName == SGSRuleQueries::PlaySkillOptions();
	const bool bResponse = Query.QueryName == SGSRuleQueries::ResponseSkillOptions()
		&& Query.AcceptedCardNames.Contains(FName(TEXT("Slash")));
	if ((!bPlay && !bResponse) || !HasSpear(Context.GameContext, Query.ActorSeat))
	{
		return;
	}

	TArray<USGSCard*> Hand = Context.GameContext->GetCardsInZone(
		SGSGameplayTags::CardZone_Hand.GetTag(), Query.ActorSeat);
	if (Hand.Num() < 2)
	{
		return;
	}
	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("Spear"));
	Option.DisplayName = FName(TEXT("丈八蛇矛"));
	Option.RuleKindTag = SGSRuleKinds::ViewAs();
	Option.ResultCardName = FName(TEXT("Slash"));
	Option.MinCardCount = 2;
	Option.MaxCardCount = 2;
	Option.MinTargetCount = bPlay ? 1 : 0;
	Option.MaxTargetCount = bPlay ? 1 : 0;
	for (const USGSCard* Card : Hand)
	{
		if (Card != nullptr)
		{
			Option.SelectableCardIds.Add(Card->CardId);
		}
	}
	for (int32 First = 0; First < Option.SelectableCardIds.Num(); ++First)
	{
		for (int32 Second = First + 1; Second < Option.SelectableCardIds.Num(); ++Second)
		{
			Option.CandidateCardSelections.Add({ Option.SelectableCardIds[First], Option.SelectableCardIds[Second] });
		}
	}
	if (bPlay)
	{
		FSGSNumericRuleQuery LimitQuery;
		LimitQuery.QueryName = SGSRuleQueries::SlashUseLimit();
		LimitQuery.ActorSeat = Query.ActorSeat;
		LimitQuery.CardName = FName(TEXT("Slash"));
		LimitQuery.BaseValue = 1;
		const int32 Limit = Context.RuleRegistry != nullptr
			? Context.RuleRegistry->ApplyNumericModifiers(Context, LimitQuery)
			: 1;
		int32 Used = 0;
		if (Context.ActiveEffects != nullptr)
		{
			for (const FSGSStableHandle Handle : Context.ActiveEffects->QueryByTag(SGSGameplayTags::Status_SlashUsed.GetTag()))
			{
				const FSGSActiveEffect* Effect = Context.ActiveEffects->Find(Handle);
				Used += Effect != nullptr && Effect->OwnerSeat == Query.ActorSeat ? 1 : 0;
			}
		}
		if (Used >= Limit)
		{
			return;
		}
		FSGSNumericRuleQuery DistanceQuery;
		DistanceQuery.QueryName = SGSRuleQueries::SlashTargetDistance();
		DistanceQuery.ActorSeat = Query.ActorSeat;
		DistanceQuery.CardName = FName(TEXT("Slash"));
		DistanceQuery.BaseValue = 1;
		const int32 Distance = Context.RuleRegistry != nullptr
			? Context.RuleRegistry->ApplyNumericModifiers(Context, DistanceQuery)
			: 1;
		for (int32 SeatIndex = 0; SeatIndex < Context.GameContext->NumSeats(); ++SeatIndex)
		{
			const USGSSeat* Target = Context.GameContext->GetSeat(SeatIndex);
			if (Target != nullptr && Target->bIsAlive && SeatIndex != Query.ActorSeat
				&& Context.GameContext->GetDistance(Query.ActorSeat, SeatIndex) <= Distance)
			{
				Option.TargetSeatIndices.Add(SeatIndex);
			}
		}
	}
	if (!bPlay || !Option.TargetSeatIndices.IsEmpty())
	{
		Query.Options.Add(MoveTemp(Option));
	}
}

FName FSGSSpearActionRule::GetRuleName() const { return FName(TEXT("SGS.Rule.Equipment.Spear.Action")); }
FSGSRuleDescriptor FSGSSpearActionRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::ViewAs(), SGSGameplayTags::PlayAction_ActivateSkill.GetTag(),
		FName(TEXT("Spear")), NAME_None, 220);
}
FSGSStatus FSGSSpearActionRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	if (!HasSpear(Context.GameContext, Context.RuleInvocation.ActorSeat)
		|| Payload.ResultCardName != TEXT("Slash")
		|| Payload.SelectedCardIds.Num() != 2)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Equipment.InvalidSpear")), TEXT("Spear requires exactly two hand cards as Slash."));
	}
	return SGSBasicCardRuleHelpers::ValidateVirtualSlashUse(
		Context, Context.RuleInvocation.ActorSeat, Payload.TargetSeatIndices);
}
FSGSStatus FSGSSpearActionRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::ExecuteVirtualSlashUse(
		Context, Payload.SelectedCardIds, Payload.TargetSeatIndices, GetRuleName());
}

FName FSGSSpearResponseRule::GetRuleName() const { return FName(TEXT("SGS.Rule.Equipment.Spear.Response")); }
FSGSRuleDescriptor FSGSSpearResponseRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor = SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Slash")), NAME_None, 320);
	Descriptor.bWildcardWindow = true;
	return Descriptor;
}
bool FSGSSpearResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("Spear")
		&& Payload.CardName == TEXT("Slash")
		&& (Context.RuleInvocation.WindowName == SGSStandardTrickRules::EffectResponseWindow()
			|| Context.RuleInvocation.WindowName == SGSLordAssistResolution::WindowName()
			|| Context.RuleInvocation.WindowName == SGSBasicCardRuleHelpers::BladeWindowName());
}
FSGSStatus FSGSSpearResponseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return HasSpear(Context.GameContext, Context.RuleInvocation.ActorSeat)
		&& Payload.SelectedCardIds.Num() == 2
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Equipment.InvalidSpearResponse")), TEXT("Spear response requires exactly two hand cards."));
}
FSGSStatus FSGSSpearResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	if (Context.RuleInvocation.WindowName == SGSBasicCardRuleHelpers::BladeWindowName())
	{
		return SGSBasicCardRuleHelpers::RestartSlashAfterBlade(
			Context, Payload.SelectedCardIds, INDEX_NONE, true);
	}
	TArray<USGSCard*> Cards;
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		Cards.Add(Context.GameContext->FindCardById(CardId));
	}
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			Cards, SGSGameplayTags::CardZone_Hand.GetTag(), Context.RuleInvocation.ActorSeat,
			SGSGameplayTags::CardZone_DiscardPile.GetTag(), INDEX_NONE,
			{ SGSCardMoveReasons::Respond(), { Context.RuleInvocation.ActorSeat } }),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	return Context.RuleInvocation.WindowName == SGSLordAssistResolution::WindowName()
		? SGSLordAssistResolution::Continue(Context, true)
		: SGSStandardTrickRules::ContinueAfterAcceptedResponse(Context);
}

FName FSGSEquipmentCardChoiceRule::GetRuleName() const
{
	return FName(*FString::Printf(TEXT("SGS.Rule.Equipment.%s.Choice"), *ChoiceName.ToString()));
}
FSGSRuleDescriptor FSGSEquipmentCardChoiceRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_ChooseCards.GetTag(),
		ChoiceName, WindowName, 230);
}
FSGSStatus FSGSEquipmentCardChoiceRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSChooseCardsRulePayload& Payload) const
{
	const FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSSlashResolutionState* Slash = Frame != nullptr ? Frame->GetState<FSGSSlashResolutionState>() : nullptr;
	if (Slash == nullptr)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Equipment.MissingSlash")), TEXT("Equipment card choice requires an active Slash."));
	}
	if (ChoiceName == SGSBasicCardRuleHelpers::AxeChoiceName())
	{
		if (Payload.SelectedCardIds.Num() != 2)
		{
			return SGSBasicCardRuleHelpers::MakeRuleError(
				FName(TEXT("SGS.Equipment.InvalidAxeCost")), TEXT("Axe requires exactly two cards."));
		}
		for (const int32 CardId : Payload.SelectedCardIds)
		{
			const FSGSCardState* State = Context.GameContext->FindCardStateById(CardId);
			if (State == nullptr || State->OwnerSeat != Slash->SourceSeat
				|| (!State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag())
					&& !State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Equipment.GetTag())))
			{
				return SGSBasicCardRuleHelpers::MakeRuleError(
					FName(TEXT("SGS.Equipment.InvalidAxeCost")), TEXT("Axe cost cards must still belong to the Slash source."));
			}
		}
		return MakeValue();
	}
	if (Payload.SelectedCardIds.Num() != 1)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Equipment.InvalidKylinBowChoice")), TEXT("Kylin Bow requires one horse."));
	}
	const USGSSeat* Target = Context.GameContext->GetSeat(Slash->TargetSeat);
	const USGSCard* Chosen = Context.GameContext->FindCardById(Payload.SelectedCardIds[0]);
	const bool bIsHorse = Target != nullptr && Chosen != nullptr
		&& (Target->Equipment.FindRef(SGSGameplayTags::EquipSlot_DefenseHorse.GetTag()).Get() == Chosen
			|| Target->Equipment.FindRef(SGSGameplayTags::EquipSlot_OffenseHorse.GetTag()).Get() == Chosen);
	return bIsHorse
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Equipment.InvalidKylinBowChoice")), TEXT("Kylin Bow can only discard a target horse."));
}
FSGSStatus FSGSEquipmentCardChoiceRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSChooseCardsRulePayload& Payload) const
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSSlashResolutionState* Slash = Frame != nullptr ? Frame->GetMutableState<FSGSSlashResolutionState>() : nullptr;
	check(Slash != nullptr);
	if (ChoiceName == SGSBasicCardRuleHelpers::AxeChoiceName())
	{
		for (const FSGSCardZone Zone : {
			SGSGameplayTags::CardZone_Hand.GetTag(), SGSGameplayTags::CardZone_Equipment.GetTag() })
		{
			TArray<USGSCard*> Cards;
			for (const int32 CardId : Payload.SelectedCardIds)
			{
				const FSGSCardState* State = Context.GameContext->FindCardStateById(CardId);
				if (State != nullptr && State->Zone.MatchesTagExact(Zone))
				{
					Cards.Add(State->Card.Get());
				}
			}
			if (!Cards.IsEmpty())
			{
				if (FSGSStatus Status = Context.Runtime->RunEffectStep(
					SGSStandardEffectSteps::MakeMoveCardsStep(
						Cards, Zone, Slash->SourceSeat,
						SGSGameplayTags::CardZone_DiscardPile.GetTag(), INDEX_NONE,
						{ SGSCardMoveReasons::Discard(), { Slash->TargetSeat } }),
					Context.RuleInvocation.SourceCommandId);
					Status.HasError())
				{
					return Status;
				}
			}
		}
		return SGSBasicCardRuleHelpers::ResolveSlashHit(Context);
	}

	const FSGSCardState* State = Context.GameContext->FindCardStateById(Payload.SelectedCardIds[0]);
	check(State != nullptr);
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			{ State->Card.Get() }, SGSGameplayTags::CardZone_Equipment.GetTag(), Slash->TargetSeat,
			SGSGameplayTags::CardZone_DiscardPile.GetTag(), INDEX_NONE,
			{ SGSCardMoveReasons::Discard(), { Slash->TargetSeat } }),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	return SGSBasicCardRuleHelpers::ContinueSlashAfterDamageTriggers(Context);
}

FName FSGSEquipmentChoicePassRule::GetRuleName() const
{
	return FName(*FString::Printf(TEXT("SGS.Rule.Equipment.%s.Pass"), *WindowName.ToString()));
}
FSGSRuleDescriptor FSGSEquipmentChoicePassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None, WindowName, 220);
}
FSGSStatus FSGSEquipmentChoicePassRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSPassRulePayload&) const
{
	return WindowName == SGSBasicCardRuleHelpers::AxeWindowName()
		? SGSBasicCardRuleHelpers::FinishDodgedSlash(Context)
		: SGSBasicCardRuleHelpers::ContinueSlashAfterDamageTriggers(Context);
}

FName FSGSBladeSlashRule::GetRuleName() const { return FName(TEXT("SGS.Rule.Equipment.Blade.Slash")); }
FSGSRuleDescriptor FSGSBladeSlashRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Slash")), SGSBasicCardRuleHelpers::BladeWindowName(), 330);
}
bool FSGSBladeSlashRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName.IsNone();
}
FSGSStatus FSGSBladeSlashRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	const USGSCard* Card = Context.GameContext->FindCardById(Payload.CardId);
	return Payload.SkillName.IsNone() && Card != nullptr && Card->CardName == TEXT("Slash")
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Equipment.InvalidBladeSlash")), TEXT("Blade follow-up requires a physical Slash."));
}
FSGSStatus FSGSBladeSlashRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::RestartSlashAfterBlade(Context, { Payload.CardId }, Payload.CardId, false);
}

FName FSGSBladePassRule::GetRuleName() const { return FName(TEXT("SGS.Rule.Equipment.Blade.Pass")); }
FSGSRuleDescriptor FSGSBladePassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None, SGSBasicCardRuleHelpers::BladeWindowName(), 320);
}
FSGSStatus FSGSBladePassRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSPassRulePayload&) const
{
	return SGSBasicCardRuleHelpers::FinishDodgedSlash(Context);
}

void SGSEquipmentRules::Register(FSGSRuleRegistry& Registry)
{
	const TArray<FName> EquipmentNames = {
		TEXT("Crossbow"), TEXT("DoubleSword"), TEXT("QinggangSword"), TEXT("Blade"),
		TEXT("Spear"), TEXT("Axe"), TEXT("Halberd"), TEXT("KylinBow"),
		TEXT("BaguaArmor"), TEXT("RenwangShield"), TEXT("Jueying"), TEXT("Dilu"),
		TEXT("ZhuahuangFeidian"), TEXT("Chitu"), TEXT("Dayuan"), TEXT("Zixing")
	};
	for (const FName CardName : EquipmentNames)
	{
		if (const FSGSStandardCardDefinition* Definition = SGSStandardCardDefinitions::Find(CardName))
		{
			Registry.RegisterRule(MakeShared<FSGSEquipmentUseRule>(*Definition));
		}
	}
	Registry.RegisterRule(MakeShared<FSGSEquipmentModifierRule>(SGSRuleQueries::SlashUseLimit()));
	Registry.RegisterRule(MakeShared<FSGSEquipmentModifierRule>(SGSRuleQueries::SlashTargetDistance()));
	Registry.RegisterRule(MakeShared<FSGSEquipmentModifierRule>(SGSRuleQueries::SlashTargetCount()));
	Registry.RegisterRule(MakeShared<FSGSBaguaOptionRule>());
	Registry.RegisterRule(MakeShared<FSGSBaguaResponseRule>());
	Registry.RegisterRule(MakeShared<FSGSSpearOptionRule>());
	Registry.RegisterRule(MakeShared<FSGSSpearActionRule>());
	Registry.RegisterRule(MakeShared<FSGSSpearResponseRule>());
	Registry.RegisterRule(MakeShared<FSGSEquipmentCardChoiceRule>(
		SGSBasicCardRuleHelpers::AxeChoiceName(), SGSBasicCardRuleHelpers::AxeWindowName()));
	Registry.RegisterRule(MakeShared<FSGSEquipmentChoicePassRule>(SGSBasicCardRuleHelpers::AxeWindowName()));
	Registry.RegisterRule(MakeShared<FSGSEquipmentCardChoiceRule>(
		SGSBasicCardRuleHelpers::KylinBowChoiceName(), SGSBasicCardRuleHelpers::KylinBowWindowName()));
	Registry.RegisterRule(MakeShared<FSGSEquipmentChoicePassRule>(SGSBasicCardRuleHelpers::KylinBowWindowName()));
	Registry.RegisterRule(MakeShared<FSGSBladeSlashRule>());
	Registry.RegisterRule(MakeShared<FSGSBladePassRule>());
}
