#include "Server/AI/SGSBasicAIAgent.h"

#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Shared/Cards/SGSCard.h"

namespace
{
int32 GetTargetPriority(FGameplayTag ActorIdentity, FGameplayTag TargetIdentity)
{
	if (ActorIdentity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		|| ActorIdentity.MatchesTagExact(SGSGameplayTags::Identity_Loyalist.GetTag()))
	{
		if (TargetIdentity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag()))
		{
			return 0;
		}
		return TargetIdentity.MatchesTagExact(SGSGameplayTags::Identity_Renegade.GetTag()) ? 1 : INDEX_NONE;
	}

	if (ActorIdentity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag()))
	{
		if (TargetIdentity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag()))
		{
			return 0;
		}
		if (TargetIdentity.MatchesTagExact(SGSGameplayTags::Identity_Loyalist.GetTag()))
		{
			return 1;
		}
		return TargetIdentity.MatchesTagExact(SGSGameplayTags::Identity_Renegade.GetTag()) ? 2 : INDEX_NONE;
	}

	if (TargetIdentity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag()))
	{
		return 0;
	}
	if (TargetIdentity.MatchesTagExact(SGSGameplayTags::Identity_Loyalist.GetTag()))
	{
		return 1;
	}
	return TargetIdentity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag()) ? 2 : INDEX_NONE;
}

void SubmitUseCard(
	const FSGSPlayPhaseRequest& Request,
	const FSGSCardActionOption& Option,
	int32 TargetSeatIndex,
	FName SourceName,
	const FSGSPlayPhaseDecisionDelegate& OnDecided)
{
	TArray<int32> Targets;
	if (TargetSeatIndex != INDEX_NONE)
	{
		Targets.Add(TargetSeatIndex);
	}

	FSGSPlayPhaseDecision Decision;
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), SourceName),
		FSGSUseCardCommandPayload(Option.CardId, MoveTemp(Targets)));
	OnDecided.ExecuteIfBound(Decision);
}
}

void USGSBasicAIAgent::BindToSeat(USGSGameContext* InContext, int32 InSeatIndex)
{
	Context = InContext;
	SeatIndex = InSeatIndex;
}

int32 USGSBasicAIAgent::ChooseTarget(TConstArrayView<int32> CandidateSeatIndices) const
{
	const USGSGameContext* GameContext = Context.Get();
	const USGSSeat* Self = GameContext != nullptr ? GameContext->GetSeat(SeatIndex) : nullptr;
	if (Self == nullptr)
	{
		return INDEX_NONE;
	}
	if (!Self->Identity.IsValid())
	{
		return CandidateSeatIndices.IsEmpty() ? INDEX_NONE : CandidateSeatIndices[0];
	}

	int32 BestSeatIndex = INDEX_NONE;
	int32 BestPriority = MAX_int32;
	int32 BestHealth = MAX_int32;
	for (const int32 CandidateSeatIndex : CandidateSeatIndices)
	{
		const USGSSeat* Candidate = GameContext->GetSeat(CandidateSeatIndex);
		if (Candidate == nullptr || !Candidate->bIsAlive)
		{
			continue;
		}

		const int32 Priority = GetTargetPriority(Self->Identity, Candidate->Identity);
		if (Priority == INDEX_NONE)
		{
			continue;
		}

		if (Priority < BestPriority
			|| (Priority == BestPriority && Candidate->Health < BestHealth)
			|| (Priority == BestPriority && Candidate->Health == BestHealth && CandidateSeatIndex < BestSeatIndex))
		{
			BestSeatIndex = CandidateSeatIndex;
			BestPriority = Priority;
			BestHealth = Candidate->Health;
		}
	}

	return BestSeatIndex;
}

bool USGSBasicAIAgent::CanRescue(int32 TargetSeatIndex) const
{
	if (TargetSeatIndex == SeatIndex)
	{
		return true;
	}

	const USGSGameContext* GameContext = Context.Get();
	const USGSSeat* Self = GameContext != nullptr ? GameContext->GetSeat(SeatIndex) : nullptr;
	const USGSSeat* Target = GameContext != nullptr ? GameContext->GetSeat(TargetSeatIndex) : nullptr;
	if (Self == nullptr || Target == nullptr
		|| Self->Identity.MatchesTagExact(SGSGameplayTags::Identity_Renegade.GetTag()))
	{
		return false;
	}
	if (!Self->Identity.IsValid())
	{
		return true;
	}

	const bool bSelfIsLordSide = Self->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		|| Self->Identity.MatchesTagExact(SGSGameplayTags::Identity_Loyalist.GetTag());
	const bool bTargetIsLordSide = Target->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		|| Target->Identity.MatchesTagExact(SGSGameplayTags::Identity_Loyalist.GetTag());
	return bSelfIsLordSide
		? bTargetIsLordSide
		: Self->Identity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag())
			&& Target->Identity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag());
}

void USGSBasicAIAgent::RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided)
{
	const USGSGameContext* GameContext = Context.Get();
	const USGSSeat* Self = GameContext != nullptr ? GameContext->GetSeat(SeatIndex) : nullptr;
	if (Self != nullptr && Self->Health < Self->MaxHealth)
	{
		if (const FSGSCardActionOption* Peach = Request.Options.FindByPredicate(
			[](const FSGSCardActionOption& Option) { return Option.CardName == TEXT("Peach"); }))
		{
			SubmitUseCard(Request, *Peach, SeatIndex, FName(TEXT("BasicAI.Peach")), OnDecided);
			return;
		}
	}

	const FSGSCardActionOption* Slash = nullptr;
	int32 SlashTarget = INDEX_NONE;
	for (const FSGSCardActionOption& Option : Request.Options)
	{
		if (Option.CardName == TEXT("Slash"))
		{
			const int32 Target = ChooseTarget(Option.TargetSeatIndices);
			if (Target != INDEX_NONE)
			{
				Slash = &Option;
				SlashTarget = Target;
				break;
			}
		}
	}

	if (Slash != nullptr)
	{
		if (const FSGSCardActionOption* Analeptic = Request.Options.FindByPredicate(
			[](const FSGSCardActionOption& Option) { return Option.CardName == TEXT("Analeptic"); }))
		{
			const int32 Target = Analeptic->TargetSeatIndices.IsEmpty()
				? INDEX_NONE
				: SeatIndex;
			SubmitUseCard(Request, *Analeptic, Target, FName(TEXT("BasicAI.Analeptic")), OnDecided);
			return;
		}

		SubmitUseCard(Request, *Slash, SlashTarget, FName(TEXT("BasicAI.Slash")), OnDecided);
		return;
	}

	FSGSPlayPhaseDecision Decision;
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), FName(TEXT("BasicAI.Pass"))),
		FSGSPassCommandPayload());
	OnDecided.ExecuteIfBound(Decision);
}

void USGSBasicAIAgent::RequestResponseAction(const FSGSResponseRequest& Request, FSGSResponseDecisionDelegate OnDecided)
{
	const USGSGameContext* GameContext = Context.Get();
	auto FindResponseCard = [&Request, GameContext](FName CardName) -> int32
	{
		if (GameContext == nullptr)
		{
			return INDEX_NONE;
		}
		for (const int32 CardId : Request.ResponseCardIds)
		{
			const USGSCard* Card = GameContext->FindCardById(CardId);
			if (Card != nullptr && Card->CardName == CardName)
			{
				return CardId;
			}
		}
		return INDEX_NONE;
	};

	int32 ResponseCardId = FindResponseCard(TEXT("Dodge"));
	if (ResponseCardId == INDEX_NONE && CanRescue(Request.EffectTargetSeat))
	{
		ResponseCardId = FindResponseCard(TEXT("Peach"));
		if (ResponseCardId == INDEX_NONE && Request.EffectTargetSeat == SeatIndex)
		{
			ResponseCardId = FindResponseCard(TEXT("Analeptic"));
		}
	}

	if (ResponseCardId != INDEX_NONE)
	{
		FSGSResponseDecision Decision;
		TArray<int32> Targets;
		const USGSCard* Card = GameContext->FindCardById(ResponseCardId);
		if (Card != nullptr
			&& (Card->CardName == TEXT("Peach") || Card->CardName == TEXT("Analeptic"))
			&& Request.EffectTargetSeat != INDEX_NONE)
		{
			Targets.Add(Request.EffectTargetSeat);
		}
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), FName(TEXT("BasicAI.Response"))),
			FSGSRespondCardCommandPayload(ResponseCardId, MoveTemp(Targets), Request.WindowName));
		OnDecided.ExecuteIfBound(Decision);
		return;
	}

	FSGSResponseDecision Decision;
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(
			Request,
			FName(TEXT("AI")),
			Request.WindowName.IsNone() ? FName(TEXT("BasicAI.ResponsePass")) : Request.WindowName),
		FSGSPassCommandPayload());
	OnDecided.ExecuteIfBound(Decision);
}
