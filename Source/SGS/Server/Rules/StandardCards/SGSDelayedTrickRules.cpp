#include "Server/Rules/StandardCards/SGSDelayedTrickRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"

namespace
{
bool IsSuit(const USGSCard* Card, const FNativeGameplayTag& Suit)
{
	return Card != nullptr && Card->Suit.MatchesTagExact(Suit.GetTag());
}

int32 FindNextLightningSeat(const USGSGameContext& Context, int32 CurrentSeat)
{
	for (int32 Offset = 1; Offset < Context.NumSeats(); ++Offset)
	{
		const int32 SeatIndex = (CurrentSeat + Offset) % Context.NumSeats();
		const USGSSeat* Seat = Context.GetSeat(SeatIndex);
		if (Seat == nullptr || !Seat->bIsAlive)
		{
			continue;
		}
		const bool bHasLightning = Context.GetCardsInZone(
			SGSGameplayTags::CardZone_Judgement.GetTag(), SeatIndex).ContainsByPredicate(
			[](const USGSCard* Card)
			{
				return Card != nullptr && Card->CardName == TEXT("Lightning");
			});
		if (!bHasLightning)
		{
			return SeatIndex;
		}
	}
	return INDEX_NONE;
}
}

FName FSGSDelayedTrickJudgementRule::GetRuleName() const
{
	return FName(*FString::Printf(TEXT("SGS.Rule.Judgement.%s"), *CardName.ToString()));
}

FSGSRuleDescriptor FSGSDelayedTrickJudgementRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::GameRule(),
		SGSGameplayTags::GameEvent_PhaseBegin.GetTag(),
		CardName,
		SGSTimingSteps::Resolve(),
		100);
}

bool FSGSDelayedTrickJudgementRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSJudgementRulePayload& Payload) const
{
	const FSGSCardState* State = Context.GameContext->FindCardStateById(Payload.DelayedTrickCardId);
	return State != nullptr && State->CardName == CardName;
}

FSGSStatus FSGSDelayedTrickJudgementRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSJudgementRulePayload& Payload) const
{
	const FSGSCardState* State = Context.GameContext->FindCardStateById(Payload.DelayedTrickCardId);
	if (State == nullptr
		|| State->OwnerSeat != Payload.SeatIndex
		|| !State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Judgement.GetTag()))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidJudgement")),
			TEXT("Delayed trick must be in the judged seat's judgement zone."));
	}
	return MakeValue();
}

FSGSStatus FSGSDelayedTrickJudgementRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSJudgementRulePayload& Payload) const
{
	USGSCard* DelayedCard = Context.GameContext->FindCardById(Payload.DelayedTrickCardId);
	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = GetRuleName();
	Frame.SourceCommandId = Context.RuleInvocation.SourceCommandId;
	Frame.ActorSeat = Payload.SeatIndex;
	Frame.SourceSeat = Payload.SeatIndex;
	Frame.TargetSeat = Payload.SeatIndex;
	Frame.ProcessingCardId = Payload.DelayedTrickCardId;
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	TSharedRef<TObjectPtr<USGSCard>> JudgementCard = MakeShared<TObjectPtr<USGSCard>>();
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeJudgementDrawStep(Payload.SeatIndex, JudgementCard),
		Context.RuleInvocation.SourceCommandId);
		Status.HasError())
	{
		return Status;
	}
	USGSCard* ResultCard = JudgementCard.Get().Get();
	if (ResultCard != nullptr)
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				{ ResultCard },
				SGSGameplayTags::CardZone_Processing.GetTag(),
				INDEX_NONE,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE,
				{ SGSCardMoveReasons::Cleanup(), { Payload.SeatIndex } }),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
	}

	bool bLightningHit = false;
	if (CardName == TEXT("Indulgence"))
	{
		if (!IsSuit(ResultCard, SGSGameplayTags::Suit_Heart))
		{
			SGSBasicCardRuleHelpers::AddStatus(
				Context, Payload.SeatIndex, FName(TEXT("SGS.ActiveEffect.SkipPlay")),
				SGSGameplayTags::Status_SkipPlayPhase.GetTag(),
				FSGSDurationSpec::ThisTurn(Payload.SeatIndex, Context.TimingPoint));
		}
	}
	else if (CardName == TEXT("SupplyShortage"))
	{
		if (!IsSuit(ResultCard, SGSGameplayTags::Suit_Club))
		{
			SGSBasicCardRuleHelpers::AddStatus(
				Context, Payload.SeatIndex, FName(TEXT("SGS.ActiveEffect.SkipDraw")),
				SGSGameplayTags::Status_SkipDrawPhase.GetTag(),
				FSGSDurationSpec::ThisTurn(Payload.SeatIndex, Context.TimingPoint));
		}
	}
	else if (CardName == TEXT("Lightning"))
	{
		bLightningHit = IsSuit(ResultCard, SGSGameplayTags::Suit_Spade)
			&& ResultCard->Number >= 2
			&& ResultCard->Number <= 9;
	}

	if (CardName == TEXT("Lightning") && !bLightningHit)
	{
		const int32 NextSeat = FindNextLightningSeat(*Context.GameContext, Payload.SeatIndex);
		if (NextSeat != INDEX_NONE)
		{
			if (FSGSStatus Status = Context.Runtime->RunEffectStep(
				SGSStandardEffectSteps::MakeMoveCardsStep(
					{ DelayedCard },
					SGSGameplayTags::CardZone_Judgement.GetTag(),
					Payload.SeatIndex,
					SGSGameplayTags::CardZone_Judgement.GetTag(),
					NextSeat,
					{ SGSCardMoveReasons::Use(), { NextSeat } }),
				Context.RuleInvocation.SourceCommandId);
				Status.HasError())
			{
				return Status;
			}
		}
	}
	else
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				{ DelayedCard },
				SGSGameplayTags::CardZone_Judgement.GetTag(),
				Payload.SeatIndex,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE,
				{ SGSCardMoveReasons::Cleanup(), { Payload.SeatIndex } }),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
	}

	Context.Runtime->RequestCurrentPhaseResume();
	if (bLightningHit)
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeDamageStep(INDEX_NONE, Payload.SeatIndex, 3),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}

		FSGSRuleEventPayload DamageAfter;
		DamageAfter.EventTag = SGSGameplayTags::GameEvent_DamageAfter.GetTag();
		DamageAfter.EventName = FName(TEXT("DamageAfter"));
		DamageAfter.TargetSeat = Payload.SeatIndex;
		DamageAfter.SourceCommandId = Context.RuleInvocation.SourceCommandId;
		DamageAfter.TimingPoint = Context.TimingPoint;
		DamageAfter.TimingPoint.Step = SGSTimingSteps::After();
		FSGSDamageEventData DamageData;
		DamageData.CardId = Payload.DelayedTrickCardId;
		DamageData.Amount = 3;
		DamageAfter.EventData = FInstancedStruct::Make(DamageData);
		if (FSGSStatus Status = Context.Runtime->PublishTimingEvent(DamageAfter); Status.HasError())
		{
			return Status;
		}

		const USGSSeat* Seat = Context.GameContext->GetSeat(Payload.SeatIndex);
		if (Seat != nullptr && Seat->Health <= 0)
		{
			if (FSGSResolutionFrame* Parent = Context.Runtime->GetResolutionStack().GetCurrentFrame())
			{
				Parent->OnChildCompletedContinuation = SGSResolutionContinuations::FinishParentCardResolution();
			}
			return SGSBasicCardRuleHelpers::StartDyingPeachResolution(Context, Payload.SeatIndex);
		}
	}

	return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.JudgementComplete")));
}

void SGSDelayedTrickRules::Register(FSGSRuleRegistry& Registry)
{
	Registry.RegisterRule(MakeShared<FSGSDelayedTrickJudgementRule>(FName(TEXT("Indulgence"))));
	Registry.RegisterRule(MakeShared<FSGSDelayedTrickJudgementRule>(FName(TEXT("SupplyShortage"))));
	Registry.RegisterRule(MakeShared<FSGSDelayedTrickJudgementRule>(FName(TEXT("Lightning"))));
}
