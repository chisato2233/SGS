#include "Server/Rules/StandardCards/SGSDelayedTrickRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"
#include "Server/Rules/Resolution/SGSDamageResolution.h"
#include "Server/Rules/Resolution/SGSJudgementResolution.h"

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
	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = GetRuleName();
	Frame.SourceCommandId = Context.RuleInvocation.SourceCommandId;
	Frame.ActorSeat = Payload.SeatIndex;
	Frame.SourceSeat = Payload.SeatIndex;
	Frame.TargetSeat = Payload.SeatIndex;
	Frame.ProcessingCardId = Payload.DelayedTrickCardId;
	FSGSDelayedTrickResolutionState ResolutionState;
	ResolutionState.JudgedSeat = Payload.SeatIndex;
	ResolutionState.DelayedTrickCardId = Payload.DelayedTrickCardId;
	ResolutionState.CardName = CardName;
	Frame.FrameState = FInstancedStruct::Make(ResolutionState);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));
	return SGSJudgementResolution::Start(
		Context,
		Payload.SeatIndex,
		CardName,
		SGSDelayedTrickRules::AfterJudgementContinuation());
}

FName SGSDelayedTrickRules::AfterJudgementContinuation()
{
	return FName(TEXT("SGS.Resolution.Continuation.DelayedTrickAfterJudgement"));
}

FSGSStatus SGSDelayedTrickRules::ContinueAfterJudgement(
	FSGSRuleExecutionContext& Context,
	int32 ResultCardId)
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSDelayedTrickResolutionState* ResolutionState = Frame != nullptr
		? Frame->GetMutableState<FSGSDelayedTrickResolutionState>()
		: nullptr;
	check(ResolutionState != nullptr);
	const int32 JudgedSeat = ResolutionState->JudgedSeat;
	const int32 DelayedTrickCardId = ResolutionState->DelayedTrickCardId;
	const FName CardName = ResolutionState->CardName;
	USGSCard* DelayedCard = Context.GameContext->FindCardById(DelayedTrickCardId);
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
				{ SGSCardMoveReasons::Cleanup(), { JudgedSeat } }),
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
				Context, JudgedSeat, FName(TEXT("SGS.ActiveEffect.SkipPlay")),
				SGSGameplayTags::Status_SkipPlayPhase.GetTag(),
				FSGSDurationSpec::ThisTurn(JudgedSeat, Context.TimingPoint));
		}
	}
	else if (CardName == TEXT("SupplyShortage"))
	{
		if (!IsSuit(ResultCard, SGSGameplayTags::Suit_Club))
		{
			SGSBasicCardRuleHelpers::AddStatus(
				Context, JudgedSeat, FName(TEXT("SGS.ActiveEffect.SkipDraw")),
				SGSGameplayTags::Status_SkipDrawPhase.GetTag(),
				FSGSDurationSpec::ThisTurn(JudgedSeat, Context.TimingPoint));
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
		const int32 NextSeat = FindNextLightningSeat(*Context.GameContext, JudgedSeat);
		if (NextSeat != INDEX_NONE)
		{
			if (FSGSStatus Status = Context.Runtime->RunEffectStep(
				SGSStandardEffectSteps::MakeMoveCardsStep(
					{ DelayedCard },
					SGSGameplayTags::CardZone_Judgement.GetTag(),
					JudgedSeat,
					SGSGameplayTags::CardZone_Judgement.GetTag(),
					NextSeat,
					{ SGSCardMoveReasons::Use(), { NextSeat } }),
				Context.RuleInvocation.SourceCommandId);
				Status.HasError())
			{
				return Status;
			}
		}
		else if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				{ DelayedCard },
				SGSGameplayTags::CardZone_Judgement.GetTag(),
				JudgedSeat,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE,
				{ SGSCardMoveReasons::Cleanup(), { JudgedSeat } }),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
	}
	else
	{
		if (FSGSStatus Status = Context.Runtime->RunEffectStep(
			SGSStandardEffectSteps::MakeMoveCardsStep(
				{ DelayedCard },
				SGSGameplayTags::CardZone_Judgement.GetTag(),
				JudgedSeat,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE,
				{ SGSCardMoveReasons::Cleanup(), { JudgedSeat } }),
			Context.RuleInvocation.SourceCommandId);
			Status.HasError())
		{
			return Status;
		}
	}

	Context.Runtime->RequestCurrentPhaseResume();
	if (bLightningHit)
	{
		return SGSDamageResolution::Start(
			Context,
			INDEX_NONE,
			JudgedSeat,
			3,
			DelayedTrickCardId,
			SGSResolutionContinuations::FinishParentCardResolution());
	}

	return Context.Runtime->CompleteCurrentFrame(FName(TEXT("SGS.Resolution.JudgementComplete")));
}

void SGSDelayedTrickRules::Register(FSGSRuleRegistry& Registry)
{
	Registry.RegisterRule(MakeShared<FSGSDelayedTrickJudgementRule>(FName(TEXT("Indulgence"))));
	Registry.RegisterRule(MakeShared<FSGSDelayedTrickJudgementRule>(FName(TEXT("SupplyShortage"))));
	Registry.RegisterRule(MakeShared<FSGSDelayedTrickJudgementRule>(FName(TEXT("Lightning"))));
}
