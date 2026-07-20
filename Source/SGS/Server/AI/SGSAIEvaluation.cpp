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
	if (Self == nullptr || Target == nullptr || !Self->bIdentityKnown)
	{
		return ESGSAIRelation::Unknown;
	}
	if (!Target->bIdentityKnown)
	{
		if (FMath::Abs(Target->HostilityToLord) < 12.0)
		{
			return ESGSAIRelation::Unknown;
		}
		const bool bSelfRebel = Self->Identity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag());
		const bool bTargetHostileToLord = Target->HostilityToLord > 0.0;
		return bSelfRebel == bTargetHostileToLord
			? ESGSAIRelation::Friendly
			: ESGSAIRelation::Hostile;
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

double ScoreSlashTargets(const FSGSAIWorldView& WorldView, const FSGSAIActionCandidate& Candidate)
{
	double Score = 0.0;
	for (const int32 TargetSeat : Candidate.TargetSeatIndices)
	{
		const FSGSAISeatView* Target = WorldView.FindSeat(TargetSeat);
		const double MissingHealth = Target != nullptr
			? static_cast<double>(Target->MaxHealth - Target->Health)
			: 0.0;
		switch (GetRelation(WorldView, TargetSeat))
		{
		case ESGSAIRelation::Friendly: Score -= 100.0; break;
		case ESGSAIRelation::Hostile: Score += 40.0 + 5.0 * MissingHealth; break;
		case ESGSAIRelation::Unknown: Score += 10.0 + 5.0 * MissingHealth; break;
		}
	}
	return Score;
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

class FSGSLegalResponseAIEvaluator final : public ISGSAIActionEvaluator
{
public:
	virtual FName GetEvaluatorId() const override { return FName(TEXT("SGS.AI.Response.LegalAction")); }
	virtual int32 GetPriority() const override { return 100; }
	virtual double Evaluate(const FSGSAIEvaluationContext& Context, const FSGSAIActionCandidate& Candidate) const override
	{
		return Context.DecisionKind == SGSAIDecisionKinds::Response()
			&& Context.WindowName != FName(TEXT("Dying.Peach"))
			&& Candidate.ActionKind != SGSAIActionKinds::ChooseCards()
			&& !Candidate.bPass
			? 70.0
			: 0.0;
	}
};

class FSGSCardChoiceAIEvaluator final : public ISGSAIActionEvaluator
{
public:
	virtual FName GetEvaluatorId() const override { return FName(TEXT("SGS.AI.Response.CardChoice")); }
	virtual int32 GetPriority() const override { return 90; }
	virtual double Evaluate(
		const FSGSAIEvaluationContext& Context,
		const FSGSAIActionCandidate& Candidate) const override
	{
		if (Candidate.ActionKind != SGSAIActionKinds::ChooseCards())
		{
			return 0.0;
		}
		if (Candidate.SkillName == TEXT("Axe.Discard"))
		{
			double Benefit = 15.0;
			switch (GetRelation(Context.WorldView, Context.EffectTargetSeat))
			{
			case ESGSAIRelation::Friendly: Benefit = -100.0; break;
			case ESGSAIRelation::Hostile: Benefit = 55.0; break;
			case ESGSAIRelation::Unknown: Benefit = 15.0; break;
			}
			double Cost = 0.0;
			for (const int32 CardId : Candidate.SelectedCardIds)
			{
				const FSGSAICardView* Card = Context.WorldView.FindOwnCard(CardId);
				if (Card == nullptr) { Cost += 12.0; continue; }
				if (Card->CardName == TEXT("Peach")) Cost += 35.0;
				else if (Card->CardName == TEXT("Dodge") || Card->CardName == TEXT("Nullification")) Cost += 24.0;
				else if (Card->CardName == TEXT("Slash")) Cost += 18.0;
				else Cost += 12.0;
			}
			return Benefit - Cost;
		}
		if (Candidate.SkillName == TEXT("KylinBow.DiscardHorse"))
		{
			switch (GetRelation(Context.WorldView, Context.EffectTargetSeat))
			{
			case ESGSAIRelation::Friendly: return -100.0;
			case ESGSAIRelation::Hostile: return 25.0;
			case ESGSAIRelation::Unknown: return 5.0;
			}
		}
		if (Candidate.SkillName == TEXT("AmazingGrace.Choose"))
		{
			if (Candidate.CardName == TEXT("Peach")) return 35.0;
			if (Candidate.CardName == TEXT("Dodge") || Candidate.CardName == TEXT("Nullification")) return 28.0;
			if (Candidate.CardName == TEXT("Slash")) return 20.0;
			return 12.0;
		}
		return 1.0;
	}
};

class FSGSSemanticAIEvaluator final : public ISGSAIActionEvaluator
{
public:
	FSGSSemanticAIEvaluator(FName InId, FSGSAIBehaviorSemantics InSemantics)
		: Id(InId)
		, Semantics(MoveTemp(InSemantics))
	{
	}

	virtual FName GetEvaluatorId() const override { return Id; }
	virtual double Evaluate(const FSGSAIEvaluationContext& Context, const FSGSAIActionCandidate& Candidate) const override
	{
		if (Context.DecisionKind != SGSAIDecisionKinds::Play())
		{
			return 0.0;
		}

		double Score = Semantics.Order * 0.25 + Semantics.Usefulness;
		auto ScoreTarget = [this, &Context](int32 TargetSeat)
		{
			const ESGSAIRelation Relation = GetRelation(Context.WorldView, TargetSeat);
			if (Semantics.TargetAttitude == SGSAIEffectAttitudes::Harmful())
			{
				return Relation == ESGSAIRelation::Friendly
					? -100.0
					: Relation == ESGSAIRelation::Hostile ? Semantics.Effect : Semantics.Effect * 0.3;
			}
			if (Semantics.TargetAttitude == SGSAIEffectAttitudes::Helpful())
			{
				return Relation == ESGSAIRelation::Hostile
					? -60.0
					: Relation == ESGSAIRelation::Friendly ? Semantics.Effect : Semantics.Effect * 0.2;
			}
			return Semantics.Effect;
		};

		if (!Candidate.TargetSeatIndices.IsEmpty())
		{
			for (const int32 TargetSeat : Candidate.TargetSeatIndices)
			{
				Score += ScoreTarget(TargetSeat);
			}
		}
		else if (Semantics.TargetAttitude == SGSAIEffectAttitudes::Harmful()
			|| Semantics.TargetAttitude == SGSAIEffectAttitudes::Helpful())
		{
			for (const FSGSAISeatView& Seat : Context.WorldView.Seats)
			{
				if (Seat.bIsAlive && Seat.SeatIndex != Context.WorldView.ObserverSeat)
				{
					Score += ScoreTarget(Seat.SeatIndex) * 0.35;
				}
			}
		}
		else
		{
			Score += Semantics.Effect;
		}
		return Score + Semantics.Threat * 0.1;
	}

private:
	FName Id;
	FSGSAIBehaviorSemantics Semantics;
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
		return ScoreSlashTargets(Context.WorldView, Candidate);
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

		double BestFollowUpScore = 0.0;
		for (const FSGSAIActionCandidate& FollowUp : Context.Candidates)
		{
			if (FollowUp.CardName == TEXT("Slash") && !FollowUp.TargetSeatIndices.IsEmpty())
			{
				BestFollowUpScore = FMath::Max(
					BestFollowUpScore,
					ScoreSlashTargets(Context.WorldView, FollowUp));
			}
		}
		return BestFollowUpScore > 0.0 ? BestFollowUpScore + 20.0 : 0.0;
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
		if (!Option.CandidateTargetSelections.IsEmpty())
		{
			for (const TArray<int32>& Targets : Option.CandidateTargetSelections)
			{
				FSGSAIActionCandidate Targeted = Candidate;
				Targeted.TargetSeatIndices = Targets;
				Targeted.StableOrder = StableOrder++;
				Candidates.Add(MoveTemp(Targeted));
			}
		}
		else
		{
			AddCandidateForTargets(Candidates, MoveTemp(Candidate), Option.TargetSeatIndices, StableOrder);
		}
	}

	for (const FSGSDecisionSkillOption& Option : Request.SkillOptions)
	{
		for (const TArray<int32>& CardSelection : Option.CandidateCardSelections)
		{
			if (CardSelection.Num() < Option.MinCardCount || CardSelection.Num() > Option.MaxCardCount)
			{
				continue;
			}
			FSGSAIActionCandidate Candidate;
			Candidate.ActionKind = SGSAIActionKinds::ActivateSkill();
			Candidate.CardName = Option.ResultCardName;
			Candidate.SkillName = Option.SkillName;
			Candidate.RuleKindTag = Option.RuleKindTag;
			Candidate.SelectedCardIds = CardSelection;
			Candidate.CardId = CardSelection.Num() == 1 ? CardSelection[0] : INDEX_NONE;
			AddCandidateForTargets(Candidates, MoveTemp(Candidate), Option.TargetSeatIndices, StableOrder);
		}
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
	if (Request.bIsOptionChoice)
	{
		for (const FSGSDecisionNamedOption& Option : Request.NamedOptions)
		{
			FSGSAIActionCandidate Candidate;
			Candidate.ActionKind = SGSAIActionKinds::ChooseOption();
			Candidate.SkillName = Option.OptionName;
			Candidate.StableOrder = StableOrder++;
			Candidates.Add(MoveTemp(Candidate));
		}
		return Candidates;
	}
	if (Request.bIsCardChoice)
	{
		TArray<TArray<int32>> Selections = Request.CandidateCardSelections;
		if (Selections.IsEmpty() && Request.MinChoiceCount == 1 && Request.MaxChoiceCount == 1)
		{
			for (const FSGSDecisionCardChoiceOption& Option : Request.CardChoiceOptions)
			{
				Selections.Add({ Option.CardId });
			}
		}
		if (Selections.IsEmpty() && Request.MinChoiceCount > 0)
		{
			TArray<int32> Selection;
			for (int32 Index = 0; Index < Request.MinChoiceCount && Request.CardChoiceOptions.IsValidIndex(Index); ++Index)
			{
				Selection.Add(Request.CardChoiceOptions[Index].CardId);
			}
			Selections.Add(MoveTemp(Selection));
		}
		for (const TArray<int32>& Selection : Selections)
		{
			if (Selection.Num() < Request.MinChoiceCount || Selection.Num() > Request.MaxChoiceCount)
			{
				continue;
			}
			FSGSAIActionCandidate Candidate;
			Candidate.ActionKind = SGSAIActionKinds::ChooseCards();
			Candidate.SkillName = Request.ChoiceName;
			Candidate.SelectedCardIds = Selection;
			Candidate.CardId = Selection.Num() == 1 ? Selection[0] : INDEX_NONE;
			if (Selection.Num() == 1)
			{
				if (const FSGSDecisionCardChoiceOption* Option = Request.CardChoiceOptions.FindByPredicate(
					[&Selection](const FSGSDecisionCardChoiceOption& Item) { return Item.CardId == Selection[0]; }))
				{
					Candidate.CardName = Option->bFaceVisible ? Option->CardName : NAME_None;
				}
			}
			Candidate.StableOrder = StableOrder++;
			Candidates.Add(MoveTemp(Candidate));
		}
		return Candidates;
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
		TArray<TArray<int32>> SkillSelections = Option.CandidateCardSelections;
		if (SkillSelections.IsEmpty())
		{
			for (const int32 CardId : Option.SelectableCardIds)
			{
				SkillSelections.Add({ CardId });
			}
		}
		if (!Option.RequiresCard())
		{
			SkillSelections.AddUnique(TArray<int32>());
		}
		const TConstArrayView<int32> Targets = Option.TargetSeatIndices.IsEmpty()
			? TConstArrayView<int32>(DefaultTargets)
			: TConstArrayView<int32>(Option.TargetSeatIndices);
		for (const TArray<int32>& CardSelection : SkillSelections)
		{
			FSGSAIActionCandidate Candidate;
			Candidate.ActionKind = SGSAIActionKinds::RespondSkill();
			Candidate.SelectedCardIds = CardSelection;
			Candidate.CardId = CardSelection.IsEmpty() ? INDEX_NONE : CardSelection[0];
			Candidate.SkillName = Option.SkillName;
			Candidate.CardName = Option.ResultCardName;
			if (Candidate.CardName.IsNone())
			{
				if (const FSGSAICardView* Card = WorldView.FindOwnCard(Candidate.CardId))
				{
					Candidate.CardName = Card->CardName;
				}
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

FName SGSAIEffectAttitudes::Neutral() { return FName(TEXT("SGS.AI.Attitude.Neutral")); }
FName SGSAIEffectAttitudes::Helpful() { return FName(TEXT("SGS.AI.Attitude.Helpful")); }
FName SGSAIEffectAttitudes::Harmful() { return FName(TEXT("SGS.AI.Attitude.Harmful")); }

void FSGSAIBeliefModel::Reset(int32 SeatCount)
{
	HostilityToLordBySeat.Init(0.0, FMath::Max(SeatCount, 0));
}

void FSGSAIBeliefModel::Observe(const FSGSAIPublicActionObservation& Observation, int32 PublicLordSeat)
{
	if (!HostilityToLordBySeat.IsValidIndex(Observation.ActorSeat))
	{
		return;
	}
	double Delta = 0.0;
	if (Observation.TargetSeatIndices.Contains(PublicLordSeat))
	{
		if (Observation.TargetAttitude == SGSAIEffectAttitudes::Harmful())
		{
			Delta += Observation.Strength;
		}
		else if (Observation.TargetAttitude == SGSAIEffectAttitudes::Helpful())
		{
			Delta -= Observation.Strength;
		}
	}
	for (const int32 TargetSeat : Observation.TargetSeatIndices)
	{
		if (!HostilityToLordBySeat.IsValidIndex(TargetSeat))
		{
			continue;
		}
		if (Observation.TargetAttitude == SGSAIEffectAttitudes::Harmful())
		{
			Delta -= HostilityToLordBySeat[TargetSeat] * 0.2;
		}
		else if (Observation.TargetAttitude == SGSAIEffectAttitudes::Helpful())
		{
			Delta += HostilityToLordBySeat[TargetSeat] * 0.15;
		}
	}
	HostilityToLordBySeat[Observation.ActorSeat] = FMath::Clamp(
		HostilityToLordBySeat[Observation.ActorSeat] + Delta,
		-100.0,
		100.0);
}

double FSGSAIBeliefModel::GetHostilityToLord(int32 SeatIndex) const
{
	return HostilityToLordBySeat.IsValidIndex(SeatIndex) ? HostilityToLordBySeat[SeatIndex] : 0.0;
}

const FSGSAICardView* FSGSAIWorldView::FindOwnCard(int32 CardId) const
{
	return OwnHand.FindByPredicate([CardId](const FSGSAICardView& Card) { return Card.CardId == CardId; });
}

FName SGSAIActionKinds::Pass() { return FName(TEXT("Pass")); }
FName SGSAIActionKinds::UseCard() { return FName(TEXT("UseCard")); }
FName SGSAIActionKinds::RespondCard() { return FName(TEXT("RespondCard")); }
FName SGSAIActionKinds::RespondSkill() { return FName(TEXT("RespondSkill")); }
FName SGSAIActionKinds::ActivateSkill() { return FName(TEXT("ActivateSkill")); }
FName SGSAIActionKinds::ChooseCards() { return FName(TEXT("ChooseCards")); }
FName SGSAIActionKinds::ChooseOption() { return FName(TEXT("ChooseOption")); }
FName SGSAIDecisionKinds::Play() { return FName(TEXT("Play")); }
FName SGSAIDecisionKinds::Response() { return FName(TEXT("Response")); }

void FSGSAIEvaluatorRegistry::Reset()
{
	GlobalEvaluators.Reset();
	CardEvaluators.Reset();
	SkillEvaluators.Reset();
	CardSemantics.Reset();
	SkillSemantics.Reset();
}

void FSGSAIEvaluatorRegistry::RegisterCardSemantics(FName CardName, FSGSAIBehaviorSemantics Semantics)
{
	CardSemantics.Add(CardName, MoveTemp(Semantics));
}

void FSGSAIEvaluatorRegistry::RegisterSkillSemantics(FName SkillName, FSGSAIBehaviorSemantics Semantics)
{
	SkillSemantics.Add(SkillName, Semantics);
	RegisterSkill(
		SkillName,
		MakeShared<FSGSSemanticAIEvaluator>(
			FName(*FString::Printf(TEXT("SGS.AI.Semantics.Skill.%s"), *SkillName.ToString())),
			MoveTemp(Semantics)));
}

const FSGSAIBehaviorSemantics* FSGSAIEvaluatorRegistry::FindSemantics(const FSGSAIActionCandidate& Candidate) const
{
	if (!Candidate.SkillName.IsNone())
	{
		if (const FSGSAIBehaviorSemantics* Semantics = SkillSemantics.Find(Candidate.SkillName))
		{
			return Semantics;
		}
	}
	return CardSemantics.Find(Candidate.CardName);
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

FSGSAIWorldView FSGSAIWorldViewBuilder::Build(
	const USGSGameContext& Context,
	int32 ObserverSeat,
	FSGSPhase Phase,
	const FSGSAIBeliefModel* Beliefs)
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
		SeatView.HostilityToLord = Beliefs != nullptr ? Beliefs->GetHostilityToLord(SeatIndex) : 0.0;
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
			View.OwnHand.Add({Card->CardId, Card->CardName, Card->Suit, Card->Number, Card->CardType});
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
	else if (Best.Candidate.ActionKind == SGSAIActionKinds::ActivateSkill())
	{
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), Best.Candidate.SkillName),
			FSGSActivateSkillCommandPayload(
				Best.Candidate.SkillName,
				Best.Candidate.RuleKindTag,
				Best.Candidate.CardName,
				Best.Candidate.SelectedCardIds,
				Best.Candidate.TargetSeatIndices));
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
		const FSGSCommandBuildRequest BuildRequest = FSGSCommandBuildRequest::FromDecisionRequest(
			Request,
			FName(TEXT("AI")),
			Best.Candidate.SkillName.IsNone() ? Best.Candidate.CardName : Best.Candidate.SkillName);
		if (Best.Candidate.ActionKind == SGSAIActionKinds::ChooseCards())
		{
			Decision.Command = FSGSCommandFactory::Make(
				BuildRequest,
				FSGSChooseCardsCommandPayload(Request.ChoiceName, Request.WindowName, Best.Candidate.SelectedCardIds));
		}
		else if (Best.Candidate.ActionKind == SGSAIActionKinds::ChooseOption())
		{
			Decision.Command = FSGSCommandFactory::Make(
				BuildRequest,
				FSGSChooseOptionCommandPayload(Request.ChoiceName, Request.WindowName, Best.Candidate.SkillName));
		}
		else
		{
			Decision.Command = FSGSCommandFactory::Make(
				BuildRequest,
				FSGSRespondCardCommandPayload(
					Best.Candidate.SelectedCardIds.IsEmpty() && Best.Candidate.CardId != INDEX_NONE
						? TArray<int32>{ Best.Candidate.CardId }
						: Best.Candidate.SelectedCardIds,
					Best.Candidate.TargetSeatIndices,
					Request.WindowName,
					Best.Candidate.SkillName,
					Best.Candidate.CardName));
		}
	}
	if (OutEvaluatedCandidates != nullptr)
	{
		*OutEvaluatedCandidates = MoveTemp(EvaluatedCandidates);
	}
	return Decision;
}

void SGSBasicAIEvaluators::Register(FSGSAIEvaluatorRegistry& Registry)
{
	Registry.RegisterCardSemantics(TEXT("Slash"), { 20, 10, 30, 20, SGSAIEffectAttitudes::Harmful() });
	Registry.RegisterCardSemantics(TEXT("Peach"), { 18, 15, 30, 5, SGSAIEffectAttitudes::Helpful() });
	Registry.RegisterCardSemantics(TEXT("Dodge"), { 0, 20, 0, 5, SGSAIEffectAttitudes::Neutral() });
	Registry.RegisterCardSemantics(TEXT("Analeptic"), { 16, 10, 12, 8, SGSAIEffectAttitudes::Helpful() });
	Registry.RegisterGlobal(MakeShared<FSGSLegalResponseAIEvaluator>());
	Registry.RegisterGlobal(MakeShared<FSGSCardChoiceAIEvaluator>());
	Registry.RegisterCard(FName(TEXT("Dodge")), MakeShared<FSGSDodgeAIEvaluator>());
	Registry.RegisterCard(FName(TEXT("Peach")), MakeShared<FSGSPeachAIEvaluator>());
	Registry.RegisterCard(FName(TEXT("Slash")), MakeShared<FSGSSlashAIEvaluator>());
	Registry.RegisterCard(FName(TEXT("Analeptic")), MakeShared<FSGSAnalepticAIEvaluator>());
}

void SGSStandardAISemantics::Register(FSGSAIEvaluatorRegistry& Registry)
{
	auto Register = [&Registry](
		FName CardName,
		double Order,
		double Useful,
		double Effect,
		double Threat,
		FName Attitude)
	{
		FSGSAIBehaviorSemantics Semantics{Order, Useful, Effect, Threat, Attitude};
		Registry.RegisterCardSemantics(CardName, Semantics);
		Registry.RegisterCard(
			CardName,
			MakeShared<FSGSSemanticAIEvaluator>(
				FName(*FString::Printf(TEXT("SGS.AI.Semantics.%s"), *CardName.ToString())),
				Semantics));
	};
	Register(TEXT("ExNihilo"), 25, 20, 25, 5, SGSAIEffectAttitudes::Helpful());
	Register(TEXT("Dismantlement"), 18, 12, 25, 12, SGSAIEffectAttitudes::Harmful());
	Register(TEXT("Snatch"), 20, 14, 28, 12, SGSAIEffectAttitudes::Harmful());
	Register(TEXT("Duel"), 17, 10, 25, 18, SGSAIEffectAttitudes::Harmful());
	Register(TEXT("SavageAssault"), 14, 8, 18, 20, SGSAIEffectAttitudes::Harmful());
	Register(TEXT("ArcheryAttack"), 14, 8, 18, 20, SGSAIEffectAttitudes::Harmful());
	Register(TEXT("GodSalvation"), 10, 5, 15, 6, SGSAIEffectAttitudes::Helpful());
	Register(TEXT("AmazingGrace"), 12, 8, 8, 5, SGSAIEffectAttitudes::Neutral());
	Register(TEXT("Indulgence"), 18, 12, 24, 12, SGSAIEffectAttitudes::Harmful());
	Register(TEXT("SupplyShortage"), 18, 12, 22, 12, SGSAIEffectAttitudes::Harmful());
	Register(TEXT("Lightning"), 8, 5, 4, 14, SGSAIEffectAttitudes::Neutral());
	for (const FName EquipmentName : {
		FName(TEXT("Crossbow")), FName(TEXT("DoubleSword")), FName(TEXT("QinggangSword")), FName(TEXT("Blade")),
		FName(TEXT("Spear")), FName(TEXT("Axe")), FName(TEXT("Halberd")), FName(TEXT("KylinBow")),
		FName(TEXT("BaguaArmor")), FName(TEXT("RenwangShield")), FName(TEXT("Jueying")), FName(TEXT("Dilu")),
		FName(TEXT("ZhuahuangFeidian")), FName(TEXT("Chitu")), FName(TEXT("Dayuan")), FName(TEXT("Zixing")) })
	{
		Register(EquipmentName, 16, 10, 8, 8, SGSAIEffectAttitudes::Neutral());
	}
}
