#include "Server/Rules/Phases/SGSDiscardPhaseRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"

FName SGSDiscardPhaseRules::WindowName()
{
	return FName(TEXT("Phase.Discard"));
}

FName FSGSDiscardPhaseResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Phase.Discard"));
}

FSGSRuleDescriptor FSGSDiscardPhaseResponseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("DiscardPhase")),
		SGSDiscardPhaseRules::WindowName(),
		300);
}

bool FSGSDiscardPhaseResponseRule::CanHandlePayload(
	const FSGSRuleExecutionContext&,
	const FSGSRespondCardRulePayload& Payload) const
{
	return Payload.SkillName == TEXT("DiscardPhase");
}

FSGSStatus FSGSDiscardPhaseResponseRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	const FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSDiscardPhaseResolutionState* State = Frame != nullptr
		? Frame->GetState<FSGSDiscardPhaseResolutionState>()
		: nullptr;
	if (State == nullptr || Payload.SelectedCardIds.Num() != State->RequiredDiscardCount)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidDiscardCount")),
			TEXT("Discard phase requires the exact current hand excess."));
	}
	TSet<int32> UniqueCards;
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		const FSGSCardState* CardState = Context.GameContext->FindCardStateById(CardId);
		if (UniqueCards.Contains(CardId)
			|| CardState == nullptr
			|| CardState->OwnerSeat != State->SeatIndex
			|| !CardState->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag()))
		{
			return SGSBasicCardRuleHelpers::MakeRuleError(
				FName(TEXT("SGS.Rule.InvalidDiscardCard")),
				TEXT("Discard phase selections must be distinct cards in the acting seat's hand."));
		}
		UniqueCards.Add(CardId);
	}
	return MakeValue();
}

FSGSStatus FSGSDiscardPhaseResponseRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	const FSGSDiscardPhaseResolutionState* State = Context.Runtime->GetResolutionStack()
		.GetCurrentFrame()->GetState<FSGSDiscardPhaseResolutionState>();
	TArray<USGSCard*> Cards;
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		Cards.Add(Context.GameContext->FindCardById(CardId));
	}
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeMoveCardsStep(
			MoveTemp(Cards),
			SGSGameplayTags::CardZone_Hand.GetTag(), State->SeatIndex,
			SGSGameplayTags::CardZone_DiscardPile.GetTag(), INDEX_NONE,
			{ SGSCardMoveReasons::Discard(), {} }),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.DiscardPhaseComplete")));
}

void SGSDiscardPhaseRules::Register(FSGSRuleRegistry& Registry)
{
	Registry.RegisterRule(MakeShared<FSGSDiscardPhaseResponseRule>());
}
