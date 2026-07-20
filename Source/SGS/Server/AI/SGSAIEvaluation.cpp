#include "Server/AI/SGSAIEvaluation.h"

#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Shared/Cards/SGSCard.h"
#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"
#include "Shared/Core/SGSGameplayTags.h"

namespace
{
enum class ESGSAIRelation : uint8
{
	Friendly,
	Hostile,
	Unknown
};

bool IsLordSide(FGameplayTag Identity)
{
	return Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		|| Identity.MatchesTagExact(SGSGameplayTags::Identity_Loyalist.GetTag());
}

ESGSAIRelation GetRelation(const FSGSAIWorldView& WorldView, int32 TargetSeatIndex)
{
	if (TargetSeatIndex == WorldView.ObserverSeat)
	{
		return ESGSAIRelation::Friendly;
	}

	const FSGSAISeatView* Self = WorldView.FindSeat(WorldView.ObserverSeat);
	const FSGSAISeatView* Target = WorldView.FindSeat(TargetSeatIndex);
	if (Self == nullptr || Target == nullptr || !Self->bIdentityKnown || !Target->bIdentityKnown)
	{
		return ESGSAIRelation::Unknown;
	}

	if (Self->Identity.MatchesTagExact(SGSGameplayTags::Identity_Renegade.GetTag())
		|| Target->Identity.MatchesTagExact(SGSGameplayTags::Identity_Renegade.GetTag()))
	{
		return ESGSAIRelation::Hostile;
	}

	const bool bSelfRebel = Self->Identity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag());
	const bool bTargetRebel = Target->Identity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag());
	if ((IsLordSide(Self->Identity) && IsLordSide(Target->Identity)) || (bSelfRebel && bTargetRebel))
	{
		return ESGSAIRelation::Friendly;
	}
	return ESGSAIRelation::Hostile;
}

class FSGSDodgeAIEvaluator final : public ISGSAIActionEvaluator
{
public:
	virtual FName GetEvaluatorId() const override { return FName(TEXT("SGS.AI.Basic.Dodge")); }
	virtual double Evaluate(const FSGSAIEvaluationContext& Context, const FSGSAIActionCandidate&) const override
	{
		return Context.DecisionKind == SGSAIDecisionKinds::Response() ? 100.0 : 0.0;
	}
};

class FSGSPeachAIEvaluator final : public ISGSAIActionEvaluator
{
public:
	virtual FName GetEvaluatorId() const override { return FName(TEXT("SGS.AI.Basic.Peach")); }
	virtual double Evaluate(const FSGSAIEvaluationContext& Context, const FSGSAIActionCandidate&) const override
	{
		if (Context.DecisionKind == SGSAIDecisionKinds::Response())
		{
			if (Context.EffectTargetSeat == Context.WorldView.ObserverSeat)
			{
				return 100.0;
			}
			return GetRelation(Context.WorldView, Context.EffectTargetSeat) == ESGSAIRelation::Friendly ? 80.0 : 0.0;
		}

		const FSGSAISeatView* Self = Context.WorldView.FindSeat(Context.WorldView.ObserverSeat);
		return Self != nullptr && Self->Health < Self->MaxHealth
			? 60.0 + 10.0 * static_cast<double>(Self->MaxHealth - Self->Health)
			: 0.0;
	}
};

class FSGSSlashAIEvaluator final : public ISGSAIActionEvaluator
{
public:
	virtual FName GetEvaluatorId() const override { return FName(TEXT("SGS.AI.Basic.Slash")); }
	virtual double Evaluate(const FSGSAIEvaluationContext& Context, const FSGSAIActionCandidate& Candidate) const override
	{
		if (Candidate.TargetSeatIndices.IsEmpty())
		{
			return 0.0;
		}

		const FSGSAISeatView* Target = Context.WorldView.FindSeat(Candidate.TargetSeatIndices[0]);
		const double MissingHealth = Target != nullptr ? static_cast<double>(Target->MaxHealth - Target->Health) : 0.0;
		switch (GetRelation(Context.WorldView, Candidate.TargetSeatIndices[0]))
		{
		case ESGSAIRelation::Friendly:
			return -100.0;
		case ESGSAIRelation::Hostile:
			return 40.0 + 5.0 * MissingHealth;
		case ESGSAIRelation::Unknown:
		default:
			return 10.0 + 5.0 * MissingHealth;
		}
	}
};

class FSGSAnalepticAIEvaluator final : public ISGSAIActionEvaluator
{
public:
	virtual FName GetEvaluatorId() const override { return FName(TEXT("SGS.AI.Basic.Analeptic")); }
	virtual double Evaluate(const FSGSAIEvaluationContext& Context, const FSGSAIActionCandidate&) const override
	{
		if (Context.DecisionKind == SGSAIDecisionKinds::Response())
		{
			return Context.EffectTargetSeat == Context.WorldView.ObserverSeat ? 100.0 : 0.0;
		}

		return Context.Candidates.ContainsByPredicate(
			[](const FSGSAIActionCandidate& Candidate)
			{
				return Candidate.CardName == TEXT("Slash") && !Candidate.TargetSeatIndices.IsEmpty();
			})
			? 20.0
			: 0.0;
	}
};

void AddCandidateForTargets(
	TArray<FSGSAIActionCandidate>& Candidates,
	FSGSAIActionCandidate Candidate,
	TConstArrayView<int32> TargetSeatIndices,
	int32& StableOrder)
{
	if (TargetSeatIndices.IsEmpty())
	{
		Candidate.StableOrder = StableOrder++;
		Candidates.Add(MoveTemp(Candidate));
		return;
	}

	for (const int32 TargetSeatIndex : TargetSeatIndices)
	{
		FSGSAIActionCandidate TargetedCandidate = Candidate;
		TargetedCandidate.TargetSeatIndices.Add(TargetSeatIndex);
		TargetedCandidate.StableOrder = StableOrder++;
		Candidates.Add(MoveTemp(TargetedCandidate));
	}
}

FSGSAIActionCandidate MakePassCandidate(int32 StableOrder)
{
	FSGSAIActionCandidate Candidate;
	Candidate.ActionKind = SGSAIActionKinds::Pass();
	Candidate.StableOrder = StableOrder;
	Candidate.bPass = true;
	return Candidate;
}

TArray<FSGSAIActionCandidate> BuildPlayCandidates(const FSGSPlayPhaseRequest& Request)
{
	TArray<FSGSAIActionCandidate> Candidates;
	int32 StableOrder = 0;
	if (Request.bAllowPass)
	{
		Candidates.Add(MakePassCandidate(StableOrder++));
	}

	for (const FSGSCardActionOption& Option : Request.Options)
	{
		FSGSAIActionCandidate Candidate;
		Candidate.ActionKind = SGSAIActionKinds::UseCard();
		Candidate.CardId = Option.CardId;
		Candidate.CardName = Option.CardName;
		AddCandidateForTargets(Candidates, MoveTemp(Candidate), Option.TargetSeatIndices, StableOrder);
	}
	return Candidates;
}

TArray<FSGSAIActionCandidate> BuildResponseCandidates(const FSGSResponseRequest& Request, const FSGSAIWorldView& WorldView)
{
	TArray<FSGSAIActionCandidate> Candidates;
	int32 StableOrder = 0;
	if (Request.bAllowPass)
	{
		Candidates.Add(MakePassCandidate(StableOrder++));
	}

	TArray<int32> DefaultTargets;
	if (Request.EffectTargetSeat != INDEX_NONE)
	{
		DefaultTargets.Add(Request.EffectTargetSeat);
	}

	for (const int32 CardId : Request.ResponseCardIds)
	{
		const FSGSAICardView* Card = WorldView.FindOwnCard(CardId);
		if (Card == nullptr)
		{
			continue;
		}

		FSGSAIActionCandidate Candidate;
		Candidate.ActionKind = SGSAIActionKinds::RespondCard();
		Candidate.CardId = CardId;
		Candidate.CardName = Card->CardName;
		AddCandidateForTargets(Candidates, MoveTemp(Candidate), DefaultTargets, StableOrder);
	}

	for (const FSGSDecisionSkillOption& Option : Request.SkillOptions)
	{
		TArray<int32> SkillCards = Option.SelectableCardIds;
		if (!Option.bRequiresCard)
		{
			SkillCards.Add(INDEX_NONE);
		}
		const TConstArrayView<int32> Targets = Option.TargetSeatIndices.IsEmpty()
			? TConstArrayView<int32>(DefaultTargets)
			: TConstArrayView<int32>(Option.TargetSeatIndices);
		for (const int32 CardId : SkillCards)
		{
			FSGSAIActionCandidate Candidate;
			Candidate.ActionKind = SGSAIActionKinds::RespondSkill();
			Candidate.CardId = CardId;
			Candidate.SkillName = Option.SkillName;
			if (const FSGSAICardView* Card = WorldView.FindOwnCard(CardId))
			{
				Candidate.CardName = Card->CardName;
			}
			AddCandidateForTargets(Candidates, MoveTemp(Candidate), Targets, StableOrder);
		}
	}
	return Candidates;
}

const FSGSAIEvaluatedCandidate& EvaluateCandidates(
	const FSGSAIEvaluationContext& Context,
	const FSGSAIEvaluatorRegistry& Registry,
	TArray<FSGSAIEvaluatedCandidate>& EvaluatedCandidates)
{
	EvaluatedCandidates.Reserve(Context.Candidates.Num());
	for (const FSGSAIActionCandidate& Candidate : Context.Candidates)
	{
		FSGSAIEvaluatedCandidate& Evaluated = EvaluatedCandidates.AddDefaulted_GetRef();
		Evaluated.Candidate = Candidate;
		TArray<TSharedRef<ISGSAIActionEvaluator>> Evaluators;
		Registry.GatherEvaluators(Candidate, Evaluators);
		for (const TSharedRef<ISGSAIActionEvaluator>& Evaluator : Evaluators)
		{
			const double Value = Evaluator->Evaluate(Context, Candidate);
			if (!FMath::IsNearlyZero(Value))
			{
				Evaluated.Score.Contributions.Add({Evaluator->GetEvaluatorId(), Value});
				Evaluated.Score.Total += Value;
			}
		}
	}

	const FSGSAIEvaluatedCandidate* Best = &EvaluatedCandidates[0];
	for (const FSGSAIEvaluatedCandidate& Candidate : EvaluatedCandidates)
	{
		if (Candidate.Score.Total > Best->Score.Total
			|| (FMath::IsNearlyEqual(Candidate.Score.Total, Best->Score.Total)
				&& Candidate.Candidate.StableOrder < Best->Candidate.StableOrder))
		{
			Best = &Candidate;
		}
	}
	return *Best;
}
}

const FSGSAISeatView* FSGSAIWorldView::FindSeat(int32 SeatIndex) const
{
	return Seats.FindByPredicate([SeatIndex](const FSGSAISeatView& Seat) { return Seat.SeatIndex == SeatIndex; });
}

const FSGSAICardView* FSGSAIWorldView::FindOwnCard(int32 CardId) const
{
	return OwnHand.FindByPredicate([CardId](const FSGSAICardView& Card) { return Card.CardId == CardId; });
}

FName SGSAIActionKinds::Pass() { return FName(TEXT("Pass")); }
FName SGSAIActionKinds::UseCard() { return FName(TEXT("UseCard")); }
FName SGSAIActionKinds::RespondCard() { return FName(TEXT("RespondCard")); }
FName SGSAIActionKinds::RespondSkill() { return FName(TEXT("RespondSkill")); }
FName SGSAIDecisionKinds::Play() { return FName(TEXT("Play")); }
FName SGSAIDecisionKinds::Response() { return FName(TEXT("Response")); }

void FSGSAIEvaluatorRegistry::Reset()
{
	GlobalEvaluators.Reset();
	CardEvaluators.Reset();
	SkillEvaluators.Reset();
}

void FSGSAIEvaluatorRegistry::RegisterGlobal(TSharedRef<ISGSAIActionEvaluator> Evaluator)
{
	GlobalEvaluators.Add(MoveTemp(Evaluator));
}

void FSGSAIEvaluatorRegistry::RegisterCard(FName CardName, TSharedRef<ISGSAIActionEvaluator> Evaluator)
{
	CardEvaluators.FindOrAdd(CardName).Add(MoveTemp(Evaluator));
}

void FSGSAIEvaluatorRegistry::RegisterSkill(FName SkillName, TSharedRef<ISGSAIActionEvaluator> Evaluator)
{
	SkillEvaluators.FindOrAdd(SkillName).Add(MoveTemp(Evaluator));
}

void FSGSAIEvaluatorRegistry::GatherEvaluators(
	const FSGSAIActionCandidate& Candidate,
	TArray<TSharedRef<ISGSAIActionEvaluator>>& OutEvaluators) const
{
	OutEvaluators.Append(GlobalEvaluators);
	if (const TArray<TSharedRef<ISGSAIActionEvaluator>>* Evaluators = CardEvaluators.Find(Candidate.CardName))
	{
		OutEvaluators.Append(*Evaluators);
	}
	if (const TArray<TSharedRef<ISGSAIActionEvaluator>>* Evaluators = SkillEvaluators.Find(Candidate.SkillName))
	{
		OutEvaluators.Append(*Evaluators);
	}
	OutEvaluators.StableSort(
		[](const TSharedRef<ISGSAIActionEvaluator>& Left, const TSharedRef<ISGSAIActionEvaluator>& Right)
		{
			if (Left->GetPriority() != Right->GetPriority())
			{
				return Left->GetPriority() > Right->GetPriority();
			}
			return Left->GetEvaluatorId().LexicalLess(Right->GetEvaluatorId());
		});
}

FSGSAIWorldView FSGSAIWorldViewBuilder::Build(const USGSGameContext& Context, int32 ObserverSeat, FSGSPhase Phase)
{
	FSGSAIWorldView View;
	View.ObserverSeat = ObserverSeat;
	View.Phase = Phase;
	View.Seats.Reserve(Context.NumSeats());
	for (int32 SeatIndex = 0; SeatIndex < Context.NumSeats(); ++SeatIndex)
	{
		const USGSSeat* Seat = Context.GetSeat(SeatIndex);
		if (Seat == nullptr)
		{
			continue;
		}

		FSGSAISeatView& SeatView = View.Seats.AddDefaulted_GetRef();
		SeatView.SeatIndex = SeatIndex;
		SeatView.GeneralId = Seat->GeneralId;
		SeatView.Health = Seat->Health;
		SeatView.MaxHealth = Seat->MaxHealth;
		SeatView.HandCount = Context.GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), SeatIndex).Num();
		SeatView.bIsAlive = Seat->bIsAlive;
		SeatView.bIdentityKnown = SeatIndex == ObserverSeat
			|| Seat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
			|| !Seat->bIsAlive;
		if (SeatView.bIdentityKnown)
		{
			SeatView.Identity = Seat->Identity;
		}
	}

	for (const USGSCard* Card : Context.GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), ObserverSeat))
	{
		if (Card != nullptr)
		{
			View.OwnHand.Add({Card->CardId, Card->CardName});
		}
	}
	return View;
}

FSGSPlayPhaseDecision FSGSAIDecisionEngine::DecidePlay(
	const FSGSPlayPhaseRequest& Request,
	const FSGSAIWorldView& WorldView,
	const FSGSAIEvaluatorRegistry& Registry,
	TArray<FSGSAIEvaluatedCandidate>* OutEvaluatedCandidates)
{
	const TArray<FSGSAIActionCandidate> Candidates = BuildPlayCandidates(Request);
	TArray<FSGSAIEvaluatedCandidate> EvaluatedCandidates;
	const FSGSAIEvaluationContext Context{WorldView, Candidates, SGSAIDecisionKinds::Play()};
	const FSGSAIEvaluatedCandidate& Best = EvaluateCandidates(Context, Registry, EvaluatedCandidates);

	FSGSPlayPhaseDecision Decision;
	if (Best.Candidate.bPass)
	{
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), FName(TEXT("BasicAI.Pass"))),
			FSGSPassCommandPayload());
	}
	else
	{
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), Best.Candidate.CardName),
			FSGSUseCardCommandPayload(Best.Candidate.CardId, Best.Candidate.TargetSeatIndices));
	}
	if (OutEvaluatedCandidates != nullptr)
	{
		*OutEvaluatedCandidates = MoveTemp(EvaluatedCandidates);
	}
	return Decision;
}

FSGSResponseDecision FSGSAIDecisionEngine::DecideResponse(
	const FSGSResponseRequest& Request,
	const FSGSAIWorldView& WorldView,
	const FSGSAIEvaluatorRegistry& Registry,
	TArray<FSGSAIEvaluatedCandidate>* OutEvaluatedCandidates)
{
	const TArray<FSGSAIActionCandidate> Candidates = BuildResponseCandidates(Request, WorldView);
	TArray<FSGSAIEvaluatedCandidate> EvaluatedCandidates;
	const FSGSAIEvaluationContext Context{
		WorldView,
		Candidates,
		SGSAIDecisionKinds::Response(),
		Request.WindowName,
		Request.ContextName,
		Request.EffectSourceSeat,
		Request.EffectTargetSeat};
	const FSGSAIEvaluatedCandidate& Best = EvaluateCandidates(Context, Registry, EvaluatedCandidates);

	FSGSResponseDecision Decision;
	if (Best.Candidate.bPass)
	{
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(
				Request,
				FName(TEXT("AI")),
				Request.WindowName.IsNone() ? FName(TEXT("BasicAI.ResponsePass")) : Request.WindowName),
			FSGSPassCommandPayload());
	}
	else
	{
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(
				Request,
				FName(TEXT("AI")),
				Best.Candidate.SkillName.IsNone() ? Best.Candidate.CardName : Best.Candidate.SkillName),
			FSGSRespondCardCommandPayload(
				Best.Candidate.CardId,
				Best.Candidate.TargetSeatIndices,
				Request.WindowName,
				Best.Candidate.SkillName));
	}
	if (OutEvaluatedCandidates != nullptr)
	{
		*OutEvaluatedCandidates = MoveTemp(EvaluatedCandidates);
	}
	return Decision;
}

void SGSBasicAIEvaluators::Register(FSGSAIEvaluatorRegistry& Registry)
{
	Registry.RegisterCard(FName(TEXT("Dodge")), MakeShared<FSGSDodgeAIEvaluator>());
	Registry.RegisterCard(FName(TEXT("Peach")), MakeShared<FSGSPeachAIEvaluator>());
	Registry.RegisterCard(FName(TEXT("Slash")), MakeShared<FSGSSlashAIEvaluator>());
	Registry.RegisterCard(FName(TEXT("Analeptic")), MakeShared<FSGSAnalepticAIEvaluator>());
}
