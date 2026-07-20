#include "Server/Skills/SGSStandardSkillRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"
#include "Server/Rules/StandardCards/SGSStandardTrickRules.h"

namespace
{
FSGSRuleDescriptor MakeSkillDescriptor(
	FName RuleName,
	FName RuleKind,
	FGameplayTag Intent,
	FName SkillName,
	int32 Priority,
	bool bWildcardIntent = false)
{
	FSGSRuleDescriptor Descriptor;
	Descriptor.RuleName = RuleName;
	Descriptor.RuleKindTag = RuleKind;
	Descriptor.IntentTag = Intent;
	Descriptor.SubjectName = SkillName;
	Descriptor.Priority = Priority;
	Descriptor.bWildcardIntent = bWildcardIntent;
	Descriptor.bWildcardWindow = true;
	return Descriptor;
}

bool OwnsSkill(const USGSGameContext* Context, int32 SeatIndex, FName SkillName)
{
	const USGSSeat* Seat = Context != nullptr ? Context->GetSeat(SeatIndex) : nullptr;
	return Seat != nullptr && Seat->bIsAlive && Seat->HasSkill(SkillName);
}

bool IsRedSuit(FGameplayTag Suit)
{
	return Suit.MatchesTagExact(SGSGameplayTags::Suit_Heart.GetTag())
		|| Suit.MatchesTagExact(SGSGameplayTags::Suit_Diamond.GetTag());
}

int32 CountSeatStatus(const FSGSRuleQueryContext& Context, int32 SeatIndex, FGameplayTag StatusTag)
{
	int32 Count = 0;
	if (Context.ActiveEffects == nullptr)
	{
		return Count;
	}
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
}

FName FSGSPaoxiaoModifierRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Paoxiao.SlashUseLimit"));
}

FSGSRuleDescriptor FSGSPaoxiaoModifierRule::GetDescriptor() const
{
	return MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::Modifier(),
		FGameplayTag(),
		SGSRuleQueries::SlashUseLimit(),
		200,
		true);
}

void FSGSPaoxiaoModifierRule::ModifyNumericQuery(
	const FSGSRuleQueryContext& Context,
	FSGSNumericRuleQuery& Query) const
{
	if (Query.QueryName == SGSRuleQueries::SlashUseLimit()
		&& Context.Phase.MatchesTagExact(SGSGameplayTags::Phase_Play.GetTag())
		&& OwnsSkill(Context.GameContext, Query.ActorSeat, FName(TEXT("Paoxiao"))))
	{
		Query.Value = TNumericLimits<int32>::Max();
	}
}

FName FSGSWushuangModifierRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Wushuang.RequiredResponseCount"));
}

FSGSRuleDescriptor FSGSWushuangModifierRule::GetDescriptor() const
{
	return MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::Modifier(),
		FGameplayTag(),
		SGSRuleQueries::RequiredResponseCount(),
		200,
		true);
}

void FSGSWushuangModifierRule::ModifyNumericQuery(
	const FSGSRuleQueryContext& Context,
	FSGSNumericRuleQuery& Query) const
{
	if (Query.QueryName == SGSRuleQueries::RequiredResponseCount()
		&& (Query.CardName == TEXT("Dodge") || Query.CardName == TEXT("Slash"))
		&& OwnsSkill(Context.GameContext, Query.ActorSeat, FName(TEXT("Wushuang"))))
	{
		Query.Value = FMath::Max(Query.Value, 2);
	}
}

FName FSGSWushengViewAsRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Wusheng.ViewAsSlash"));
}

FSGSRuleDescriptor FSGSWushengViewAsRule::GetDescriptor() const
{
	return MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::ViewAs(),
		SGSGameplayTags::PlayAction_ActivateSkill.GetTag(),
		FName(TEXT("Wusheng")),
		200);
}

void FSGSWushengViewAsRule::CollectSkillOptions(
	const FSGSRuleQueryContext& Context,
	FSGSSkillOptionQuery& Query) const
{
	const bool bPlayQuery = Query.QueryName == SGSRuleQueries::PlaySkillOptions();
	const bool bResponseQuery = Query.QueryName == SGSRuleQueries::ResponseSkillOptions()
		&& Query.AcceptedCardNames.Contains(FName(TEXT("Slash")));
	if ((!bPlayQuery && !bResponseQuery)
		|| !OwnsSkill(Context.GameContext, Query.ActorSeat, FName(TEXT("Wusheng")))
		|| (bPlayQuery
			&& (!Context.Phase.MatchesTagExact(SGSGameplayTags::Phase_Play.GetTag())
				|| CountSeatStatus(Context, Query.ActorSeat, SGSGameplayTags::Status_SlashUsed.GetTag()) >= 1)))
	{
		return;
	}

	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("Wusheng"));
	Option.DisplayName = FName(TEXT("武圣"));
	Option.RuleKindTag = SGSRuleKinds::ViewAs();
	Option.ResultCardName = FName(TEXT("Slash"));
	Option.MinCardCount = 1;
	Option.MaxCardCount = 1;
	Option.MinTargetCount = bPlayQuery ? 1 : 0;
	Option.MaxTargetCount = bPlayQuery ? 1 : 0;
	FSGSNumericRuleQuery DistanceQuery;
	DistanceQuery.QueryName = SGSRuleQueries::SlashTargetDistance();
	DistanceQuery.ActorSeat = Query.ActorSeat;
	DistanceQuery.CardName = FName(TEXT("Slash"));
	DistanceQuery.BaseValue = 1;
	const int32 SlashDistance = Context.RuleRegistry != nullptr
		? Context.RuleRegistry->ApplyNumericModifiers(Context, DistanceQuery)
		: 1;

	for (USGSCard* Card : Context.GameContext->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), Query.ActorSeat))
	{
		if (Card != nullptr && IsRedSuit(Card->Suit) && Card->CardName != TEXT("Slash"))
		{
			Option.SelectableCardIds.Add(Card->CardId);
			Option.CandidateCardSelections.Add({ Card->CardId });
		}
	}
	if (bPlayQuery)
	{
		for (int32 SeatIndex = 0; SeatIndex < Context.GameContext->NumSeats(); ++SeatIndex)
		{
			const USGSSeat* Target = Context.GameContext->GetSeat(SeatIndex);
			if (Target != nullptr && Target->bIsAlive && SeatIndex != Query.ActorSeat
				&& Context.GameContext->GetDistance(Query.ActorSeat, SeatIndex) <= SlashDistance)
			{
				Option.TargetSeatIndices.Add(SeatIndex);
			}
		}
	}
	if (!Option.SelectableCardIds.IsEmpty() && (!bPlayQuery || !Option.TargetSeatIndices.IsEmpty()))
	{
		Query.Options.Add(MoveTemp(Option));
	}
}

FName FSGSWushengResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Wusheng.ResponseSlash"));
}

FSGSRuleDescriptor FSGSWushengResponseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Slash")),
		SGSStandardTrickRules::EffectResponseWindow(),
		300);
}

bool FSGSWushengResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Context.RuleInvocation.WindowName == SGSStandardTrickRules::EffectResponseWindow()
		&& Payload.SkillName == TEXT("Wusheng")
		&& Payload.CardName == TEXT("Slash");
}

FSGSStatus FSGSWushengResponseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	USGSCard* Card = Context.GameContext->FindCardById(Payload.CardId);
	if (!OwnsSkill(Context.GameContext, Context.RuleInvocation.ActorSeat, FName(TEXT("Wusheng")))
		|| Card == nullptr
		|| !IsRedSuit(Card->Suit))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidWushengResponse")),
			TEXT("Wusheng response requires one red hand card as Slash."));
	}
	return MakeValue();
}

FSGSStatus FSGSWushengResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	if (FSGSStatus Status = SGSBasicCardRuleHelpers::DiscardHandCard(
		Context,
		Context.GameContext->FindCardById(Payload.CardId),
		Context.RuleInvocation.ActorSeat);
		Status.HasError())
	{
		return Status;
	}
	return SGSStandardTrickRules::ContinueAfterAcceptedResponse(Context);
}

bool FSGSWushengViewAsRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	return Context.RuleInvocation.IntentTag.MatchesTagExact(SGSGameplayTags::PlayAction_ActivateSkill.GetTag())
		&& Payload.SkillName == TEXT("Wusheng");
}

FSGSStatus FSGSWushengViewAsRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	if (!OwnsSkill(Context.GameContext, Context.RuleInvocation.ActorSeat, FName(TEXT("Wusheng")))
		|| Payload.ResultCardName != TEXT("Slash")
		|| Payload.SelectedCardIds.Num() != 1)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Skill.InvalidWusheng")), TEXT("Wusheng requires one red hand card as Slash."));
	}

	USGSCard* MaterialCard = Context.GameContext->FindCardById(Payload.SelectedCardIds[0]);
	if (MaterialCard == nullptr || !IsRedSuit(MaterialCard->Suit))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Skill.InvalidWushengCard")), TEXT("Wusheng material must be red."));
	}
	return SGSBasicCardRuleHelpers::ValidateSlashUse(Context, MaterialCard, Payload.TargetSeatIndices);
}

FSGSStatus FSGSWushengViewAsRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::ExecuteSlashUse(
		Context,
		Context.GameContext->FindCardById(Payload.SelectedCardIds[0]),
		Payload.TargetSeatIndices,
		GetRuleName());
}

FName FSGSJianxiongTriggerRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Jianxiong.DamageAfter"));
}

FSGSRuleDescriptor FSGSJianxiongTriggerRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor = MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::Trigger(),
		SGSGameplayTags::GameEvent_DamageAfter.GetTag(),
		NAME_None,
		100);
	Descriptor.bWildcardSubject = true;
	return Descriptor;
}

bool FSGSJianxiongTriggerRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRuleEventPayload& Payload) const
{
	if (!Payload.EventTag.MatchesTagExact(SGSGameplayTags::GameEvent_DamageAfter.GetTag())
		|| !OwnsSkill(Context.GameContext, Payload.TargetSeat, FName(TEXT("Jianxiong"))))
	{
		return false;
	}
	const FSGSDamageEventData* Damage = Payload.EventData.GetPtr<FSGSDamageEventData>();
	const FSGSCardState* State = Damage != nullptr ? Context.GameContext->FindCardStateById(Damage->CardId) : nullptr;
	return State != nullptr
		&& (State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Processing.GetTag())
			|| State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_DiscardPile.GetTag())
			|| State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Judgement.GetTag()));
}

FSGSStatus FSGSJianxiongTriggerRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRuleEventPayload& Payload) const
{
	const FSGSDamageEventData* Damage = Payload.EventData.GetPtr<FSGSDamageEventData>();
	USGSCard* DamageCard = Damage != nullptr ? Context.GameContext->FindCardById(Damage->CardId) : nullptr;
	const FSGSCardState* State = Damage != nullptr ? Context.GameContext->FindCardStateById(Damage->CardId) : nullptr;
	return Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			{ DamageCard },
			State->Zone,
			State->OwnerSeat,
			SGSGameplayTags::CardZone_Hand.GetTag(),
			Payload.TargetSeat,
			{ SGSCardMoveReasons::Gain(), {} }),
		Payload.SourceCommandId);
}

FName FSGSFankuiTriggerRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Fankui.DamageAfter"));
}

FSGSRuleDescriptor FSGSFankuiTriggerRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor = MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::Trigger(),
		SGSGameplayTags::GameEvent_DamageAfter.GetTag(),
		NAME_None,
		90);
	Descriptor.bWildcardSubject = true;
	return Descriptor;
}

bool FSGSFankuiTriggerRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRuleEventPayload& Payload) const
{
	if (!OwnsSkill(Context.GameContext, Payload.TargetSeat, FName(TEXT("Fankui")))
		|| Payload.SourceSeat == INDEX_NONE
		|| Payload.SourceSeat == Payload.TargetSeat)
	{
		return false;
	}
	const USGSSeat* Source = Context.GameContext->GetSeat(Payload.SourceSeat);
	return Source != nullptr && Source->bIsAlive
		&& (!Context.GameContext->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), Payload.SourceSeat).IsEmpty()
			|| !Context.GameContext->GetCardsInZone(SGSGameplayTags::CardZone_Equipment.GetTag(), Payload.SourceSeat).IsEmpty());
}

FSGSStatus FSGSFankuiTriggerRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRuleEventPayload& Payload) const
{
	FSGSCardZone FromZone = SGSGameplayTags::CardZone_Equipment.GetTag();
	TArray<USGSCard*> Cards = Context.GameContext->GetCardsInZone(FromZone, Payload.SourceSeat);
	if (Cards.IsEmpty())
	{
		FromZone = SGSGameplayTags::CardZone_Hand.GetTag();
		Cards = Context.GameContext->GetCardsInZone(FromZone, Payload.SourceSeat);
	}
	return Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			{ Cards[0] },
			FromZone,
			Payload.SourceSeat,
			SGSGameplayTags::CardZone_Hand.GetTag(),
			Payload.TargetSeat,
			{ SGSCardMoveReasons::Gain(), { Payload.SourceSeat } }),
		Payload.SourceCommandId);
}

FName FSGSZhihengActionRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Zhiheng.Action"));
}

FSGSRuleDescriptor FSGSZhihengActionRule::GetDescriptor() const
{
	return MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_ActivateSkill.GetTag(),
		FName(TEXT("Zhiheng")),
		200);
}

void FSGSZhihengActionRule::CollectSkillOptions(
	const FSGSRuleQueryContext& Context,
	FSGSSkillOptionQuery& Query) const
{
	if (Query.QueryName != SGSRuleQueries::PlaySkillOptions()
		|| !OwnsSkill(Context.GameContext, Query.ActorSeat, FName(TEXT("Zhiheng")))
		|| CountSeatStatus(Context, Query.ActorSeat, SGSGameplayTags::Status_ZhihengUsed.GetTag()) > 0)
	{
		return;
	}

	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("Zhiheng"));
	Option.DisplayName = FName(TEXT("制衡"));
	Option.RuleKindTag = SGSRuleKinds::Action();
	Option.MinCardCount = 1;
	Option.MinTargetCount = 0;
	Option.MaxTargetCount = 0;
	for (USGSCard* Card : Context.GameContext->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), Query.ActorSeat))
	{
		if (Card != nullptr)
		{
			Option.SelectableCardIds.Add(Card->CardId);
			Option.CandidateCardSelections.Add({ Card->CardId });
		}
	}
	Option.MaxCardCount = Option.SelectableCardIds.Num();
	if (Option.SelectableCardIds.Num() > 1)
	{
		Option.CandidateCardSelections.Add(Option.SelectableCardIds);
	}
	if (!Option.SelectableCardIds.IsEmpty())
	{
		Query.Options.Add(MoveTemp(Option));
	}
}

bool FSGSZhihengActionRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	return Context.RuleInvocation.IntentTag.MatchesTagExact(SGSGameplayTags::PlayAction_ActivateSkill.GetTag())
		&& Payload.SkillName == TEXT("Zhiheng");
}

FSGSStatus FSGSZhihengActionRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	const int32 SeatIndex = Context.RuleInvocation.ActorSeat;
	if (!OwnsSkill(Context.GameContext, SeatIndex, FName(TEXT("Zhiheng")))
		|| Payload.SelectedCardIds.IsEmpty()
		|| SGSBasicCardRuleHelpers::HasStatus(Context, SeatIndex, SGSGameplayTags::Status_ZhihengUsed.GetTag()))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Skill.InvalidZhiheng")), TEXT("Zhiheng requires at least one hand card and is limited to once per play phase."));
	}
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		const FSGSCardState* State = Context.GameContext->FindCardStateById(CardId);
		if (State == nullptr || State->OwnerSeat != SeatIndex
			|| !State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag()))
		{
			return SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Skill.InvalidZhihengCard")), TEXT("Zhiheng can only discard cards from the acting seat's hand."));
		}
	}
	return MakeValue();
}

FSGSStatus FSGSZhihengActionRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	const int32 SeatIndex = Context.RuleInvocation.ActorSeat;
	TArray<USGSCard*> Cards;
	Cards.Reserve(Payload.SelectedCardIds.Num());
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		Cards.Add(Context.GameContext->FindCardById(CardId));
	}
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			MoveTemp(Cards),
			SGSGameplayTags::CardZone_Hand.GetTag(),
			SeatIndex,
			SGSGameplayTags::CardZone_DiscardPile.GetTag(),
			INDEX_NONE,
			{ SGSCardMoveReasons::Discard(), {} }),
		SGSBasicCardRuleHelpers::GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}

	SGSBasicCardRuleHelpers::AddStatus(
		Context,
		SeatIndex,
		FName(TEXT("SGS.ActiveEffect.ZhihengUsed")),
		SGSGameplayTags::Status_ZhihengUsed.GetTag(),
		FSGSDurationSpec::ThisPhase(SeatIndex, Context.TimingPoint));
	return Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeDrawCardsStep(SeatIndex, Payload.SelectedCardIds.Num()),
		SGSBasicCardRuleHelpers::GetCommandId(Context));
}

FName FSGSRendeActionRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Rende.Action"));
}

FSGSRuleDescriptor FSGSRendeActionRule::GetDescriptor() const
{
	return MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_ActivateSkill.GetTag(),
		FName(TEXT("Rende")),
		200);
}

void FSGSRendeActionRule::CollectSkillOptions(
	const FSGSRuleQueryContext& Context,
	FSGSSkillOptionQuery& Query) const
{
	if (Query.QueryName != SGSRuleQueries::PlaySkillOptions()
		|| !OwnsSkill(Context.GameContext, Query.ActorSeat, FName(TEXT("Rende"))))
	{
		return;
	}
	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("Rende"));
	Option.DisplayName = FName(TEXT("仁德"));
	Option.RuleKindTag = SGSRuleKinds::Action();
	Option.MinCardCount = 1;
	Option.MinTargetCount = 1;
	Option.MaxTargetCount = 1;
	for (USGSCard* Card : Context.GameContext->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), Query.ActorSeat))
	{
		if (Card != nullptr)
		{
			Option.SelectableCardIds.Add(Card->CardId);
			Option.CandidateCardSelections.Add({ Card->CardId });
		}
	}
	Option.MaxCardCount = Option.SelectableCardIds.Num();
	if (Option.SelectableCardIds.Num() > 1)
	{
		Option.CandidateCardSelections.Add(Option.SelectableCardIds);
	}
	for (int32 SeatIndex = 0; SeatIndex < Context.GameContext->NumSeats(); ++SeatIndex)
	{
		const USGSSeat* Target = Context.GameContext->GetSeat(SeatIndex);
		if (Target != nullptr && Target->bIsAlive && SeatIndex != Query.ActorSeat)
		{
			Option.TargetSeatIndices.Add(SeatIndex);
		}
	}
	if (!Option.SelectableCardIds.IsEmpty() && !Option.TargetSeatIndices.IsEmpty())
	{
		Query.Options.Add(MoveTemp(Option));
	}
}

bool FSGSRendeActionRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	return Context.RuleInvocation.IntentTag.MatchesTagExact(SGSGameplayTags::PlayAction_ActivateSkill.GetTag())
		&& Payload.SkillName == TEXT("Rende");
}

FSGSStatus FSGSRendeActionRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	const int32 SourceSeat = Context.RuleInvocation.ActorSeat;
	if (!OwnsSkill(Context.GameContext, SourceSeat, FName(TEXT("Rende")))
		|| Payload.SelectedCardIds.IsEmpty()
		|| Payload.TargetSeatIndices.Num() != 1
		|| Payload.TargetSeatIndices[0] == SourceSeat)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Skill.InvalidRende")), TEXT("Rende requires hand cards and one other living target."));
	}
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		const FSGSCardState* State = Context.GameContext->FindCardStateById(CardId);
		if (State == nullptr || State->OwnerSeat != SourceSeat
			|| !State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag()))
		{
			return SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Skill.InvalidRendeCard")), TEXT("Rende can only give the acting seat's hand cards."));
		}
	}
	return MakeValue();
}

FSGSStatus FSGSRendeActionRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	const int32 SourceSeat = Context.RuleInvocation.ActorSeat;
	const int32 TargetSeat = Payload.TargetSeatIndices[0];
	TArray<USGSCard*> Cards;
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		Cards.Add(Context.GameContext->FindCardById(CardId));
	}
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			MoveTemp(Cards),
			SGSGameplayTags::CardZone_Hand.GetTag(),
			SourceSeat,
			SGSGameplayTags::CardZone_Hand.GetTag(),
			TargetSeat,
			{ SGSCardMoveReasons::Gain(), { TargetSeat } }),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	USGSSeat* Source = Context.GameContext->GetSeat(SourceSeat);
	const int32 PreviousCount = Source->RendeGivenCardCount;
	Source->RendeGivenCardCount += Payload.SelectedCardIds.Num();
	if (!Source->bRendeHealedThisTurn && PreviousCount < 2 && Source->RendeGivenCardCount >= 2)
	{
		Source->bRendeHealedThisTurn = true;
		return Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeHealStep(SourceSeat, 1),
			Context.RuleInvocation.SourceCommandId);
	}
	return MakeValue();
}
