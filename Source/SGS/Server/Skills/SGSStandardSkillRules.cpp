#include "Server/Skills/SGSStandardSkillRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"
#include "Server/Rules/Resolution/SGSDamageResolution.h"
#include "Server/Rules/Resolution/SGSJudgementResolution.h"
#include "Server/Rules/Resolution/SGSLordAssistResolution.h"
#include "Server/Rules/Resolution/SGSTimingEventResolution.h"
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

FName JianxiongWindow() { return FName(TEXT("Skill.Jianxiong")); }
FName FankuiWindow() { return FName(TEXT("Skill.Fankui")); }
FName FankuiChoiceWindow() { return FName(TEXT("Skill.Fankui.ChooseCard")); }
FName FankuiChoiceName() { return FName(TEXT("Fankui.Choose")); }
FName GanglieInvokeWindow() { return FName(TEXT("Skill.Ganglie.Invoke")); }
FName GanglieDiscardWindow() { return FName(TEXT("Skill.Ganglie.Discard")); }
FName GuicaiWindow() { return FName(TEXT("Skill.Guicai")); }

int32 FindLivingSkillOwner(const USGSGameContext* Context, FName SkillName)
{
	if (Context == nullptr)
	{
		return INDEX_NONE;
	}
	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		if (OwnsSkill(Context, SeatIndex, SkillName))
		{
			return SeatIndex;
		}
	}
	return INDEX_NONE;
}

const FSGSRuleEventPayload* GetTimingEvent(const FSGSRuleExecutionContext& Context)
{
	const FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSTimingEventResolutionState* State = Frame != nullptr
		? Frame->GetState<FSGSTimingEventResolutionState>()
		: nullptr;
	return State != nullptr ? &State->EventPayload : nullptr;
}

FSGSStatus OpenOptionalTrigger(
	FSGSRuleExecutionContext& Context,
	int32 OwnerSeat,
	FName SkillName,
	FName DisplayName,
	FName WindowName,
	int32 EffectSourceSeat)
{
	FSGSDecisionSkillOption Option;
	Option.SkillName = SkillName;
	Option.DisplayName = DisplayName;
	Option.RuleKindTag = SGSRuleKinds::Trigger();
	Option.MinCardCount = 0;
	Option.MaxCardCount = 0;

	FSGSRuleResponseWindowSpec Spec;
	Spec.SeatIndex = OwnerSeat;
	Spec.WindowName = WindowName;
	Spec.ContextName = SkillName;
	Spec.EffectSourceSeat = EffectSourceSeat;
	Spec.EffectTargetSeat = OwnerSeat;
	Spec.SkillOptions.Add(MoveTemp(Option));
	Context.Runtime->OpenResponseWindow(Spec);
	return MakeValue();
}

FSGSStatus ResolveGanglieFailure(FSGSRuleExecutionContext& Context)
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	if (Event == nullptr)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidGanglieEvent")),
			TEXT("Ganglie requires its damage event."));
	}
	FSGSResolutionFrame* TimingFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	check(TimingFrame != nullptr);
	TimingFrame->OnChildCompletedContinuation = SGSTimingEventResolution::ResumeDispatchAfterChild();
	return SGSDamageResolution::Start(
		Context,
		Event->TargetSeat,
		Event->SourceSeat,
		1,
		INDEX_NONE,
		SGSTimingEventResolution::ResumeDispatchAfterChild());
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

FName FSGSJiuyuanModifierRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Jiuyuan.PeachHealAmount"));
}

FSGSRuleDescriptor FSGSJiuyuanModifierRule::GetDescriptor() const
{
	return MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::Modifier(),
		FGameplayTag(),
		SGSRuleQueries::PeachHealAmount(),
		200,
		true);
}

void FSGSJiuyuanModifierRule::ModifyNumericQuery(
	const FSGSRuleQueryContext& Context,
	FSGSNumericRuleQuery& Query) const
{
	if (Query.QueryName != SGSRuleQueries::PeachHealAmount()
		|| Query.CardName != TEXT("Peach")
		|| Query.ActorSeat == Query.TargetSeat)
	{
		return;
	}
	const USGSSeat* Responder = Context.GameContext != nullptr
		? Context.GameContext->GetSeat(Query.ActorSeat)
		: nullptr;
	const USGSSeat* Target = Context.GameContext != nullptr
		? Context.GameContext->GetSeat(Query.TargetSeat)
		: nullptr;
	if (Responder != nullptr
		&& Target != nullptr
		&& Responder->Faction.MatchesTagExact(SGSGameplayTags::Faction_Wu.GetTag())
		&& Target->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		&& Target->HasSkill(FName(TEXT("Jiuyuan"))))
	{
		++Query.Value;
	}
}

FName FSGSHujiaOptionRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Hujia.Option"));
}

FSGSRuleDescriptor FSGSHujiaOptionRule::GetDescriptor() const
{
	return MakeSkillDescriptor(
		GetRuleName(), SGSRuleKinds::ViewAs(), FGameplayTag(), FName(TEXT("Hujia")), 180, true);
}

void FSGSHujiaOptionRule::CollectSkillOptions(
	const FSGSRuleQueryContext& Context,
	FSGSSkillOptionQuery& Query) const
{
	const USGSSeat* Lord = Context.GameContext != nullptr
		? Context.GameContext->GetSeat(Query.ActorSeat)
		: nullptr;
	if (Query.QueryName != SGSRuleQueries::ResponseSkillOptions()
		|| !Query.AcceptedCardNames.Contains(FName(TEXT("Dodge")))
		|| Lord == nullptr
		|| !Lord->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		|| !Lord->HasSkill(FName(TEXT("Hujia"))))
	{
		return;
	}
	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("Hujia"));
	Option.DisplayName = FName(TEXT("护驾"));
	Option.RuleKindTag = SGSRuleKinds::ViewAs();
	Option.ResultCardName = FName(TEXT("Dodge"));
	Query.Options.Add(MoveTemp(Option));
}

FName FSGSJijiangOptionRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Jijiang.Option"));
}

FSGSRuleDescriptor FSGSJijiangOptionRule::GetDescriptor() const
{
	return MakeSkillDescriptor(
		GetRuleName(), SGSRuleKinds::ViewAs(), FGameplayTag(), FName(TEXT("Jijiang")), 180, true);
}

void FSGSJijiangOptionRule::CollectSkillOptions(
	const FSGSRuleQueryContext& Context,
	FSGSSkillOptionQuery& Query) const
{
	const USGSSeat* Lord = Context.GameContext != nullptr
		? Context.GameContext->GetSeat(Query.ActorSeat)
		: nullptr;
	const bool bPlay = Query.QueryName == SGSRuleQueries::PlaySkillOptions();
	const bool bResponse = Query.QueryName == SGSRuleQueries::ResponseSkillOptions()
		&& Query.AcceptedCardNames.Contains(FName(TEXT("Slash")));
	if ((!bPlay && !bResponse)
		|| Lord == nullptr
		|| !Lord->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		|| !Lord->HasSkill(FName(TEXT("Jijiang"))))
	{
		return;
	}
	if (bPlay
		&& CountSeatStatus(Context, Query.ActorSeat, SGSGameplayTags::Status_JijiangFailed.GetTag()) > 0)
	{
		return;
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
		if (CountSeatStatus(Context, Query.ActorSeat, SGSGameplayTags::Status_SlashUsed.GetTag()) >= Limit)
		{
			return;
		}
	}

	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("Jijiang"));
	Option.DisplayName = FName(TEXT("激将"));
	Option.RuleKindTag = bPlay ? SGSRuleKinds::Action() : SGSRuleKinds::ViewAs();
	Option.ResultCardName = FName(TEXT("Slash"));
	Option.MinTargetCount = bPlay ? 1 : 0;
	Option.MaxTargetCount = bPlay ? 1 : 0;
	if (bPlay)
	{
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

FName FSGSHujiaResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Hujia.Invoke"));
}

FSGSRuleDescriptor FSGSHujiaResponseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Dodge")), NAME_None, 350, false, false, true);
}

bool FSGSHujiaResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("Hujia") && Payload.CardName == TEXT("Dodge");
}

FSGSStatus FSGSHujiaResponseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	const USGSSeat* Lord = Context.GameContext->GetSeat(Context.RuleInvocation.ActorSeat);
	return Lord != nullptr
		&& Lord->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		&& Lord->HasSkill(FName(TEXT("Hujia")))
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidHujia")), TEXT("Only the Wei lord may invoke Hujia."));
}

FSGSStatus FSGSHujiaResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return SGSLordAssistResolution::Start(
		Context,
		FName(TEXT("Hujia")),
		Context.RuleInvocation.ActorSeat,
		FName(TEXT("Dodge")),
		Payload.EffectTargetSeat,
		SGSGameplayTags::Faction_Wei.GetTag(),
		false);
}

FName FSGSJijiangActionRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Jijiang.Action"));
}

FSGSRuleDescriptor FSGSJijiangActionRule::GetDescriptor() const
{
	return MakeSkillDescriptor(
		GetRuleName(), SGSRuleKinds::Action(), SGSGameplayTags::PlayAction_ActivateSkill.GetTag(),
		FName(TEXT("Jijiang")), 200);
}

bool FSGSJijiangActionRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSActivateSkillRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("Jijiang") && Payload.ResultCardName == TEXT("Slash");
}

FSGSStatus FSGSJijiangActionRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	const USGSSeat* Lord = Context.GameContext->GetSeat(Context.RuleInvocation.ActorSeat);
	if (Lord == nullptr
		|| !Lord->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		|| !Lord->HasSkill(FName(TEXT("Jijiang"))))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidJijiang")), TEXT("Only the Shu lord may invoke Jijiang."));
	}
	if (SGSBasicCardRuleHelpers::HasStatus(
		Context, Context.RuleInvocation.ActorSeat, SGSGameplayTags::Status_JijiangFailed.GetTag()))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.JijiangUnavailable")), TEXT("Jijiang already failed in this play phase."));
	}
	return SGSBasicCardRuleHelpers::ValidateVirtualSlashUse(
		Context, Context.RuleInvocation.ActorSeat, Payload.TargetSeatIndices);
}

FSGSStatus FSGSJijiangActionRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSActivateSkillRulePayload& Payload) const
{
	return SGSLordAssistResolution::Start(
		Context,
		FName(TEXT("Jijiang")),
		Context.RuleInvocation.ActorSeat,
		FName(TEXT("Slash")),
		Payload.TargetSeatIndices[0],
		SGSGameplayTags::Faction_Shu.GetTag(),
		true);
}

FName FSGSJijiangResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Jijiang.Response"));
}

FSGSRuleDescriptor FSGSJijiangResponseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Slash")), NAME_None, 350, false, false, true);
}

bool FSGSJijiangResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("Jijiang") && Payload.CardName == TEXT("Slash");
}

FSGSStatus FSGSJijiangResponseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	const USGSSeat* Lord = Context.GameContext->GetSeat(Context.RuleInvocation.ActorSeat);
	return Lord != nullptr
		&& Lord->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		&& Lord->HasSkill(FName(TEXT("Jijiang")))
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidJijiang")), TEXT("Only the Shu lord may invoke Jijiang."));
}

FSGSStatus FSGSJijiangResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return SGSLordAssistResolution::Start(
		Context,
		FName(TEXT("Jijiang")),
		Context.RuleInvocation.ActorSeat,
		FName(TEXT("Slash")),
		Payload.EffectTargetSeat,
		SGSGameplayTags::Faction_Shu.GetTag(),
		false);
}

FName FSGSLordAssistCardRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.LordAssist.Card"));
}

FSGSRuleDescriptor FSGSLordAssistCardRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		NAME_None, SGSLordAssistResolution::WindowName(), 300, false, true);
}

bool FSGSLordAssistCardRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	const FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSLordAssistResolutionState* State = Frame != nullptr
		? Frame->GetState<FSGSLordAssistResolutionState>()
		: nullptr;
	return State != nullptr && Payload.SkillName.IsNone() && Payload.CardName == State->RequiredCardName;
}

FSGSStatus FSGSLordAssistCardRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	const FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSLordAssistResolutionState* State = Frame->GetState<FSGSLordAssistResolutionState>();
	USGSCard* Card = Context.GameContext->FindCardById(Payload.CardId);
	return Card != nullptr && Card->CardName == State->RequiredCardName
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidLordAssistCard")), TEXT("The provided card does not satisfy the lord skill."));
}

FSGSStatus FSGSLordAssistCardRule::ExecutePayload(
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
	return SGSLordAssistResolution::Continue(Context, true);
}

FName FSGSLordAssistPassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.LordAssist.Pass"));
}

FSGSRuleDescriptor FSGSLordAssistPassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None, SGSLordAssistResolution::WindowName(), 300);
}

FSGSStatus FSGSLordAssistPassRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSPassRulePayload&) const
{
	return SGSLordAssistResolution::Continue(Context, false);
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
	FSGSRuleDescriptor Descriptor = SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Slash")),
		NAME_None,
		300);
	Descriptor.bWildcardWindow = true;
	return Descriptor;
}

bool FSGSWushengResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return (Context.RuleInvocation.WindowName == SGSStandardTrickRules::EffectResponseWindow()
			|| Context.RuleInvocation.WindowName == SGSLordAssistResolution::WindowName()
			|| Context.RuleInvocation.WindowName == SGSBasicCardRuleHelpers::BladeWindowName())
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
	if (Context.RuleInvocation.WindowName == SGSBasicCardRuleHelpers::BladeWindowName())
	{
		return SGSBasicCardRuleHelpers::RestartSlashAfterBlade(
			Context, { Payload.CardId }, Payload.CardId, false);
	}
	if (FSGSStatus Status = SGSBasicCardRuleHelpers::DiscardHandCard(
		Context,
		Context.GameContext->FindCardById(Payload.CardId),
		Context.RuleInvocation.ActorSeat);
		Status.HasError())
	{
		return Status;
	}
	return Context.RuleInvocation.WindowName == SGSLordAssistResolution::WindowName()
		? SGSLordAssistResolution::Continue(Context, true)
		: SGSStandardTrickRules::ContinueAfterAcceptedResponse(Context);
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
	return OpenOptionalTrigger(
		Context,
		Payload.TargetSeat,
		FName(TEXT("Jianxiong")),
		FName(TEXT("奸雄")),
		JianxiongWindow(),
		Payload.SourceSeat);
}

FName FSGSJianxiongResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Jianxiong.Invoke"));
}

FSGSRuleDescriptor FSGSJianxiongResponseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Jianxiong")), JianxiongWindow(), 300);
}

bool FSGSJianxiongResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("Jianxiong");
}

FSGSStatus FSGSJianxiongResponseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	return Event != nullptr && OwnsSkill(Context.GameContext, Event->TargetSeat, FName(TEXT("Jianxiong")))
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Skill.InvalidJianxiong")), TEXT("Jianxiong is not available for this damage event."));
}

FSGSStatus FSGSJianxiongResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	const FSGSDamageEventData* Damage = Event != nullptr ? Event->EventData.GetPtr<FSGSDamageEventData>() : nullptr;
	USGSCard* DamageCard = Damage != nullptr ? Context.GameContext->FindCardById(Damage->CardId) : nullptr;
	const FSGSCardState* State = Damage != nullptr ? Context.GameContext->FindCardStateById(Damage->CardId) : nullptr;
	if (DamageCard != nullptr && State != nullptr)
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				{ DamageCard }, State->Zone, State->OwnerSeat,
				SGSGameplayTags::CardZone_Hand.GetTag(), Event->TargetSeat,
				{ SGSCardMoveReasons::Gain(), {} }),
			Event->SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
	}
	return Context.Runtime->ContinueTimingEventDispatch();
}

FName FSGSJianxiongPassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Jianxiong.Pass"));
}

FSGSRuleDescriptor FSGSJianxiongPassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None, JianxiongWindow(), 300);
}

FSGSStatus FSGSJianxiongPassRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSPassRulePayload&) const
{
	return Context.Runtime->ContinueTimingEventDispatch();
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
	return OpenOptionalTrigger(
		Context,
		Payload.TargetSeat,
		FName(TEXT("Fankui")),
		FName(TEXT("反馈")),
		FankuiWindow(),
		Payload.SourceSeat);
}

FName FSGSFankuiResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Fankui.Invoke"));
}

FSGSRuleDescriptor FSGSFankuiResponseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Fankui")), FankuiWindow(), 300);
}

bool FSGSFankuiResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("Fankui");
}

FSGSStatus FSGSFankuiResponseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	return Event != nullptr && OwnsSkill(Context.GameContext, Event->TargetSeat, FName(TEXT("Fankui")))
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(FName(TEXT("SGS.Skill.InvalidFankui")), TEXT("Fankui is not available for this damage event."));
}

FSGSStatus FSGSFankuiResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	FSGSRuleResponseWindowSpec Spec;
	Spec.SeatIndex = Event->TargetSeat;
	Spec.WindowName = FankuiChoiceWindow();
	Spec.ContextName = FName(TEXT("Fankui"));
	Spec.EffectSourceSeat = Event->SourceSeat;
	Spec.EffectTargetSeat = Event->TargetSeat;
	Spec.bAllowPass = false;
	Spec.bIsCardChoice = true;
	Spec.ChoiceName = FankuiChoiceName();
	Spec.MinChoiceCount = 1;
	Spec.MaxChoiceCount = 1;
	for (const FSGSCardZone Zone : {
		SGSGameplayTags::CardZone_Equipment.GetTag(), SGSGameplayTags::CardZone_Hand.GetTag() })
	{
		for (const USGSCard* Card : Context.GameContext->GetCardsInZone(Zone, Event->SourceSeat))
		{
			if (Card != nullptr)
			{
				const bool bVisible = Zone.MatchesTagExact(SGSGameplayTags::CardZone_Equipment.GetTag());
				Spec.CardChoiceOptions.Add({
					Card->CardId,
					bVisible ? Card->CardName : NAME_None,
					bVisible ? Card->Suit : SGSGameplayTags::Suit_None.GetTag(),
					bVisible ? Card->Number : 0,
					bVisible });
			}
		}
	}
	return !Spec.CardChoiceOptions.IsEmpty() && Context.Runtime->OpenResponseWindow(Spec)
		? MakeValue()
		: Context.Runtime->ContinueTimingEventDispatch();
}

FName FSGSFankuiPassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Fankui.Pass"));
}

FSGSRuleDescriptor FSGSFankuiPassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None, FankuiWindow(), 300);
}

FSGSStatus FSGSFankuiPassRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSPassRulePayload&) const
{
	return Context.Runtime->ContinueTimingEventDispatch();
}

FName FSGSFankuiCardChoiceRule::GetRuleName() const { return FName(TEXT("SGS.Skill.Fankui.ChooseCard")); }
FSGSRuleDescriptor FSGSFankuiCardChoiceRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_ChooseCards.GetTag(),
		FankuiChoiceName(), FankuiChoiceWindow(), 310);
}
FSGSStatus FSGSFankuiCardChoiceRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSChooseCardsRulePayload& Payload) const
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	const FSGSCardState* State = Payload.SelectedCardIds.Num() == 1
		? Context.GameContext->FindCardStateById(Payload.SelectedCardIds[0])
		: nullptr;
	return Event != nullptr && State != nullptr && State->OwnerSeat == Event->SourceSeat
		&& (State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag())
			|| State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Equipment.GetTag()))
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidFankuiChoice")), TEXT("Fankui choice is no longer available."));
}
FSGSStatus FSGSFankuiCardChoiceRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSChooseCardsRulePayload& Payload) const
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	const FSGSCardState* State = Context.GameContext->FindCardStateById(Payload.SelectedCardIds[0]);
	check(Event != nullptr && State != nullptr);
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			{ State->Card.Get() }, State->Zone, Event->SourceSeat,
			SGSGameplayTags::CardZone_Hand.GetTag(), Event->TargetSeat,
			{ SGSCardMoveReasons::Gain(), { Event->SourceSeat } }),
		Event->SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	return Context.Runtime->ContinueTimingEventDispatch();
}

FName FSGSGanglieTriggerRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Ganglie.DamageAfter"));
}

FSGSRuleDescriptor FSGSGanglieTriggerRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor = MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::Trigger(),
		SGSGameplayTags::GameEvent_DamageAfter.GetTag(),
		NAME_None,
		80);
	Descriptor.bWildcardSubject = true;
	return Descriptor;
}

bool FSGSGanglieTriggerRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRuleEventPayload& Payload) const
{
	if (!OwnsSkill(Context.GameContext, Payload.TargetSeat, FName(TEXT("Ganglie")))
		|| Payload.SourceSeat == INDEX_NONE
		|| Payload.SourceSeat == Payload.TargetSeat)
	{
		return false;
	}
	const USGSSeat* Source = Context.GameContext->GetSeat(Payload.SourceSeat);
	return Source != nullptr && Source->bIsAlive;
}

FSGSStatus FSGSGanglieTriggerRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRuleEventPayload& Payload) const
{
	return OpenOptionalTrigger(
		Context,
		Payload.TargetSeat,
		FName(TEXT("Ganglie")),
		FName(TEXT("刚烈")),
		GanglieInvokeWindow(),
		Payload.SourceSeat);
}

FName FSGSGanglieInvokeRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Ganglie.Invoke"));
}

FSGSRuleDescriptor FSGSGanglieInvokeRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Ganglie")), GanglieInvokeWindow(), 300);
}

bool FSGSGanglieInvokeRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("Ganglie");
}

FSGSStatus FSGSGanglieInvokeRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	return Event != nullptr
		&& OwnsSkill(Context.GameContext, Event->TargetSeat, FName(TEXT("Ganglie")))
		&& Event->SourceSeat != INDEX_NONE
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidGanglie")),
			TEXT("Ganglie is not available for this damage event."));
}

FSGSStatus FSGSGanglieInvokeRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload&) const
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	return SGSJudgementResolution::Start(
		Context,
		Event->TargetSeat,
		FName(TEXT("Ganglie")),
		SGSStandardSkillRules::GanglieAfterJudgementContinuation());
}

FName SGSStandardSkillRules::GanglieAfterJudgementContinuation()
{
	return FName(TEXT("SGS.Resolution.Continuation.GanglieAfterJudgement"));
}

FSGSStatus SGSStandardSkillRules::ContinueGanglieAfterJudgement(
	FSGSRuleExecutionContext& Context,
	int32 ResultCardId)
{
	const FSGSRuleEventPayload* Event = GetTimingEvent(Context);
	check(Event != nullptr);
	const int32 SkillOwnerSeat = Event->TargetSeat;
	const int32 DamageSourceSeat = Event->SourceSeat;
	USGSCard* ResultCard = Context.GameContext->FindCardById(ResultCardId);
	if (ResultCard != nullptr)
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				{ ResultCard },
				SGSGameplayTags::CardZone_Processing.GetTag(),
				INDEX_NONE,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE,
				{ SGSCardMoveReasons::Cleanup(), { SkillOwnerSeat } }),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
	}
	if (ResultCard != nullptr && ResultCard->Suit.MatchesTagExact(SGSGameplayTags::Suit_Heart.GetTag()))
	{
		return Context.Runtime->ContinueTimingEventDispatch();
	}

	const TArray<USGSCard*> SourceHand = Context.GameContext->GetCardsInZone(
		SGSGameplayTags::CardZone_Hand.GetTag(), DamageSourceSeat);
	if (SourceHand.Num() < 2)
	{
		return ResolveGanglieFailure(Context);
	}

	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("GanglieDiscard"));
	Option.DisplayName = FName(TEXT("弃两张牌"));
	Option.RuleKindTag = SGSRuleKinds::Response();
	Option.MinCardCount = 2;
	Option.MaxCardCount = 2;
	for (const USGSCard* Card : SourceHand)
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

	FSGSRuleResponseWindowSpec Spec;
	Spec.SeatIndex = DamageSourceSeat;
	Spec.WindowName = GanglieDiscardWindow();
	Spec.ContextName = FName(TEXT("Ganglie"));
	Spec.EffectSourceSeat = SkillOwnerSeat;
	Spec.EffectTargetSeat = DamageSourceSeat;
	Spec.SkillOptions.Add(MoveTemp(Option));
	return Context.Runtime->OpenResponseWindow(Spec)
		? MakeValue()
		: ResolveGanglieFailure(Context);
}

FName FSGSGanglieDiscardRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Ganglie.Discard"));
}

FSGSRuleDescriptor FSGSGanglieDiscardRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("GanglieDiscard")), GanglieDiscardWindow(), 300);
}

bool FSGSGanglieDiscardRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("GanglieDiscard");
}

FSGSStatus FSGSGanglieDiscardRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	if (Payload.SelectedCardIds.Num() != 2)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidGanglieDiscard")),
			TEXT("Ganglie requires exactly two hand cards to avoid damage."));
	}
	TSet<int32> UniqueCards;
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		const FSGSCardState* State = Context.GameContext->FindCardStateById(CardId);
		if (UniqueCards.Contains(CardId)
			|| State == nullptr
			|| State->OwnerSeat != Context.RuleInvocation.ActorSeat
			|| !State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag()))
		{
			return SGSBasicCardRuleHelpers::MakeRuleError(
				FName(TEXT("SGS.Skill.InvalidGanglieDiscardCard")),
				TEXT("Ganglie discard materials must be two distinct hand cards."));
		}
		UniqueCards.Add(CardId);
	}
	return MakeValue();
}

FSGSStatus FSGSGanglieDiscardRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	TArray<USGSCard*> Cards;
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		Cards.Add(Context.GameContext->FindCardById(CardId));
	}
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			MoveTemp(Cards),
			SGSGameplayTags::CardZone_Hand.GetTag(),
			Context.RuleInvocation.ActorSeat,
			SGSGameplayTags::CardZone_DiscardPile.GetTag(),
			INDEX_NONE,
			{ SGSCardMoveReasons::Discard(), {} }),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	return Context.Runtime->ContinueTimingEventDispatch();
}

FName FSGSGangliePassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Ganglie.Pass"));
}

FSGSRuleDescriptor FSGSGangliePassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None, NAME_None, 310, false, false, true);
}

bool FSGSGangliePassRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSPassRulePayload&) const
{
	return Context.RuleInvocation.WindowName == GanglieInvokeWindow()
		|| Context.RuleInvocation.WindowName == GanglieDiscardWindow();
}

FSGSStatus FSGSGangliePassRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSPassRulePayload&) const
{
	return Context.RuleInvocation.WindowName == GanglieDiscardWindow()
		? ResolveGanglieFailure(Context)
		: Context.Runtime->ContinueTimingEventDispatch();
}

FName FSGSGuicaiTriggerRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Guicai.JudgementRevealed"));
}

FSGSRuleDescriptor FSGSGuicaiTriggerRule::GetDescriptor() const
{
	FSGSRuleDescriptor Descriptor = MakeSkillDescriptor(
		GetRuleName(),
		SGSRuleKinds::Trigger(),
		SGSGameplayTags::GameEvent_JudgementRevealed.GetTag(),
		NAME_None,
		100);
	Descriptor.bWildcardSubject = true;
	return Descriptor;
}

bool FSGSGuicaiTriggerRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRuleEventPayload& Payload) const
{
	if (!Payload.EventTag.MatchesTagExact(SGSGameplayTags::GameEvent_JudgementRevealed.GetTag()))
	{
		return false;
	}
	const int32 OwnerSeat = FindLivingSkillOwner(Context.GameContext, FName(TEXT("Guicai")));
	return OwnerSeat != INDEX_NONE
		&& !Context.GameContext->GetCardsInZone(
			SGSGameplayTags::CardZone_Hand.GetTag(), OwnerSeat).IsEmpty();
}

FSGSStatus FSGSGuicaiTriggerRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRuleEventPayload& Payload) const
{
	const int32 OwnerSeat = FindLivingSkillOwner(Context.GameContext, FName(TEXT("Guicai")));
	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("Guicai"));
	Option.DisplayName = FName(TEXT("鬼才"));
	Option.RuleKindTag = SGSRuleKinds::Trigger();
	Option.MinCardCount = 1;
	Option.MaxCardCount = 1;
	for (const USGSCard* Card : Context.GameContext->GetCardsInZone(
		SGSGameplayTags::CardZone_Hand.GetTag(), OwnerSeat))
	{
		if (Card != nullptr)
		{
			Option.SelectableCardIds.Add(Card->CardId);
			Option.CandidateCardSelections.Add({ Card->CardId });
		}
	}

	FSGSRuleResponseWindowSpec Spec;
	Spec.SeatIndex = OwnerSeat;
	Spec.WindowName = GuicaiWindow();
	Spec.ContextName = FName(TEXT("Guicai"));
	Spec.EffectSourceSeat = OwnerSeat;
	Spec.EffectTargetSeat = Payload.TargetSeat;
	Spec.SkillOptions.Add(MoveTemp(Option));
	Context.Runtime->OpenResponseWindow(Spec);
	return MakeValue();
}

FName FSGSGuicaiResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Guicai.Replace"));
}

FSGSRuleDescriptor FSGSGuicaiResponseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Guicai")), GuicaiWindow(), 300);
}

bool FSGSGuicaiResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("Guicai");
}

FSGSStatus FSGSGuicaiResponseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	const FSGSCardState* Replacement = Payload.SelectedCardIds.Num() == 1
		? Context.GameContext->FindCardStateById(Payload.SelectedCardIds[0])
		: nullptr;
	return OwnsSkill(Context.GameContext, Context.RuleInvocation.ActorSeat, FName(TEXT("Guicai")))
		&& Replacement != nullptr
		&& Replacement->OwnerSeat == Context.RuleInvocation.ActorSeat
		&& Replacement->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag())
		&& SGSJudgementResolution::FindActiveState(Context.Runtime->GetResolutionStack()) != nullptr
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Skill.InvalidGuicai")),
			TEXT("Guicai requires one hand card during an active judgement."));
}

FSGSStatus FSGSGuicaiResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	FSGSJudgementResolutionState* Judgement = SGSJudgementResolution::FindActiveState(
		Context.Runtime->GetResolutionStack());
	USGSCard* PreviousCard = Context.GameContext->FindCardById(Judgement->ResultCardId);
	USGSCard* ReplacementCard = Context.GameContext->FindCardById(Payload.SelectedCardIds[0]);
	const int32 PreviousCardId = Judgement->ResultCardId;
	if (PreviousCard != nullptr)
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				{ PreviousCard },
				SGSGameplayTags::CardZone_Processing.GetTag(), INDEX_NONE,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(), INDEX_NONE,
				{ SGSCardMoveReasons::Cleanup(), { Judgement->JudgedSeat } }),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
	}
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			{ ReplacementCard },
			SGSGameplayTags::CardZone_Hand.GetTag(), Context.RuleInvocation.ActorSeat,
			SGSGameplayTags::CardZone_Processing.GetTag(), INDEX_NONE,
			{ SGSCardMoveReasons::Use(), { Judgement->JudgedSeat } }),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	Judgement->ResultCardId = ReplacementCard->CardId;
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeRuleOutcomeStep(
			FName(TEXT("SGS.Event.JudgementReplaced")),
			FString::Printf(
				TEXT("JudgedSeat=%d PreviousCard=%d ReplacementCard=%d Skill=Guicai"),
				Judgement->JudgedSeat,
				PreviousCardId,
				ReplacementCard->CardId)),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	return Context.Runtime->ContinueTimingEventDispatch();
}

FName FSGSGuicaiPassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Skill.Guicai.Pass"));
}

FSGSRuleDescriptor FSGSGuicaiPassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(), SGSRuleKinds::Response(), SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None, GuicaiWindow(), 300);
}

FSGSStatus FSGSGuicaiPassRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSPassRulePayload&) const
{
	return Context.Runtime->ContinueTimingEventDispatch();
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
