#include "Server/Rules/StandardCards/SGSEquipmentRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"
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
			|| Context.RuleInvocation.WindowName == SGSStandardTrickRules::EffectResponseWindow());
}

FSGSStatus FSGSBaguaResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	TSharedRef<TObjectPtr<USGSCard>> JudgementCard = MakeShared<TObjectPtr<USGSCard>>();
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeJudgementDrawStep(Context.RuleInvocation.ActorSeat, JudgementCard),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	USGSCard* Card = JudgementCard.Get().Get();
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
		if (FSGSStatus Status = SGSBasicCardRuleHelpers::DiscardProcessingCard(Context); Status.HasError())
		{
			return Status;
		}
		return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.SlashDodgedByBagua")));
	}
	return bRed
		? SGSStandardTrickRules::ContinueAfterAcceptedResponse(Context)
		: SGSStandardTrickRules::ContinueAfterDeclinedResponse(Context);
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
	Registry.RegisterRule(MakeShared<FSGSBaguaOptionRule>());
	Registry.RegisterRule(MakeShared<FSGSBaguaResponseRule>());
}
