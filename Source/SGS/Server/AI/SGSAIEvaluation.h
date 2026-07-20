#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/SGSTypes.h"
#include "Shared/Decisions/SGSDecisionTypes.h"

class USGSGameContext;

struct SGS_API FSGSAICardView
{
	int32 CardId = INDEX_NONE;
	FName CardName = NAME_None;
	FGameplayTag Suit;
	int32 Number = 0;
	FGameplayTag CardType;
};

struct SGS_API FSGSAISeatView
{
	int32 SeatIndex = INDEX_NONE;
	FName GeneralId = NAME_None;
	int32 Health = 0;
	int32 MaxHealth = 0;
	int32 HandCount = 0;
	bool bIsAlive = false;
	bool bIdentityKnown = false;
	FGameplayTag Identity;
	double HostilityToLord = 0.0;
};

namespace SGSAIEffectAttitudes
{
	SGS_API FName Neutral();
	SGS_API FName Helpful();
	SGS_API FName Harmful();
}

struct SGS_API FSGSAIBehaviorSemantics
{
	double Order = 0.0;
	double Usefulness = 0.0;
	double Effect = 0.0;
	double Threat = 0.0;
	FName TargetAttitude = NAME_None;
};

struct SGS_API FSGSAIPublicActionObservation
{
	int32 ActorSeat = INDEX_NONE;
	TArray<int32> TargetSeatIndices;
	FName TargetAttitude = NAME_None;
	double Strength = 0.0;
};

class SGS_API FSGSAIBeliefModel
{
public:
	void Reset(int32 SeatCount);
	void Observe(const FSGSAIPublicActionObservation& Observation, int32 PublicLordSeat);
	double GetHostilityToLord(int32 SeatIndex) const;

private:
	TArray<double> HostilityToLordBySeat;
};

// AI 的只读信息边界。权威隐藏身份和其他玩家手牌不会进入本结构。
struct SGS_API FSGSAIWorldView
{
	int32 ObserverSeat = INDEX_NONE;
	FSGSPhase Phase = SGSGameplayTags::Phase_None.GetTag();
	TArray<FSGSAISeatView> Seats;
	TArray<FSGSAICardView> OwnHand;

	const FSGSAISeatView* FindSeat(int32 SeatIndex) const;
	const FSGSAICardView* FindOwnCard(int32 CardId) const;
};

namespace SGSAIDecisionKinds
{
	SGS_API FName Play();
	SGS_API FName Response();
}

namespace SGSAIActionKinds
{
	SGS_API FName Pass();
	SGS_API FName UseCard();
	SGS_API FName RespondCard();
	SGS_API FName RespondSkill();
	SGS_API FName ActivateSkill();
}

struct SGS_API FSGSAIActionCandidate
{
	FName ActionKind = NAME_None;
	int32 CardId = INDEX_NONE;
	FName CardName = NAME_None;
	FName SkillName = NAME_None;
	FName RuleKindTag = NAME_None;
	TArray<int32> SelectedCardIds;
	TArray<int32> TargetSeatIndices;
	int32 StableOrder = 0;
	bool bPass = false;
};

struct SGS_API FSGSAIScoreContribution
{
	FName EvaluatorId = NAME_None;
	double Value = 0.0;
};

struct SGS_API FSGSAIScoreBreakdown
{
	TArray<FSGSAIScoreContribution> Contributions;
	double Total = 0.0;
};

struct SGS_API FSGSAIEvaluatedCandidate
{
	FSGSAIActionCandidate Candidate;
	FSGSAIScoreBreakdown Score;
};

struct SGS_API FSGSAIEvaluationContext
{
	const FSGSAIWorldView& WorldView;
	TConstArrayView<FSGSAIActionCandidate> Candidates;
	FName DecisionKind = NAME_None;
	FName WindowName = NAME_None;
	FName ContextName = NAME_None;
	int32 EffectSourceSeat = INDEX_NONE;
	int32 EffectTargetSeat = INDEX_NONE;
};

class SGS_API ISGSAIActionEvaluator
{
public:
	virtual ~ISGSAIActionEvaluator() = default;
	virtual FName GetEvaluatorId() const = 0;
	virtual int32 GetPriority() const { return 0; }
	virtual double Evaluate(const FSGSAIEvaluationContext& Context, const FSGSAIActionCandidate& Candidate) const = 0;
};

// 每局一份，无全局可变状态。规则包可按全局、牌名或技能名追加局部语义。
class SGS_API FSGSAIEvaluatorRegistry
{
public:
	void Reset();
	void RegisterGlobal(TSharedRef<ISGSAIActionEvaluator> Evaluator);
	void RegisterCard(FName CardName, TSharedRef<ISGSAIActionEvaluator> Evaluator);
	void RegisterSkill(FName SkillName, TSharedRef<ISGSAIActionEvaluator> Evaluator);
	void RegisterCardSemantics(FName CardName, FSGSAIBehaviorSemantics Semantics);
	void RegisterSkillSemantics(FName SkillName, FSGSAIBehaviorSemantics Semantics);
	const FSGSAIBehaviorSemantics* FindSemantics(const FSGSAIActionCandidate& Candidate) const;
	void GatherEvaluators(const FSGSAIActionCandidate& Candidate, TArray<TSharedRef<ISGSAIActionEvaluator>>& OutEvaluators) const;

private:
	TArray<TSharedRef<ISGSAIActionEvaluator>> GlobalEvaluators;
	TMap<FName, TArray<TSharedRef<ISGSAIActionEvaluator>>> CardEvaluators;
	TMap<FName, TArray<TSharedRef<ISGSAIActionEvaluator>>> SkillEvaluators;
	TMap<FName, FSGSAIBehaviorSemantics> CardSemantics;
	TMap<FName, FSGSAIBehaviorSemantics> SkillSemantics;
};

class SGS_API FSGSAIWorldViewBuilder
{
public:
	static FSGSAIWorldView Build(
		const USGSGameContext& Context,
		int32 ObserverSeat,
		FSGSPhase Phase,
		const FSGSAIBeliefModel* Beliefs = nullptr);
};

class SGS_API FSGSAIDecisionEngine
{
public:
	static FSGSPlayPhaseDecision DecidePlay(
		const FSGSPlayPhaseRequest& Request,
		const FSGSAIWorldView& WorldView,
		const FSGSAIEvaluatorRegistry& Registry,
		TArray<FSGSAIEvaluatedCandidate>* OutEvaluatedCandidates = nullptr);

	static FSGSResponseDecision DecideResponse(
		const FSGSResponseRequest& Request,
		const FSGSAIWorldView& WorldView,
		const FSGSAIEvaluatorRegistry& Registry,
		TArray<FSGSAIEvaluatedCandidate>* OutEvaluatedCandidates = nullptr);
};

namespace SGSBasicAIEvaluators
{
	SGS_API void Register(FSGSAIEvaluatorRegistry& Registry);
}

namespace SGSStandardAISemantics
{
	SGS_API void Register(FSGSAIEvaluatorRegistry& Registry);
}
