#include "Server/AI/SGSBasicAIAgent.h"

#include "Server/AI/SGSAIEvaluation.h"
#include "Server/Engine/SGSGameContext.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
template <typename TDecision, typename TDecisionDelegate>
void SubmitAfterThinkDelay(
	USGSBasicAIAgent* Agent,
	float ThinkDelaySeconds,
	TDecision Decision,
	TDecisionDelegate OnDecided)
{
	if (ThinkDelaySeconds <= 0.0f)
	{
		OnDecided.ExecuteIfBound(Decision);
		return;
	}

	UWorld* World = Agent->GetWorld();
	check(World != nullptr);
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateWeakLambda(
		Agent,
		[Decision = MoveTemp(Decision), OnDecided = MoveTemp(OnDecided)]() mutable
		{
			OnDecided.ExecuteIfBound(Decision);
		});
	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, MoveTemp(TimerDelegate), ThinkDelaySeconds, false);
}
}

void USGSBasicAIAgent::BindToSeat(
	USGSGameContext* InContext,
	int32 InSeatIndex,
	const FSGSAIEvaluatorRegistry* InEvaluatorRegistry,
	float InThinkDelaySeconds)
{
	Context = InContext;
	SeatIndex = InSeatIndex;
	EvaluatorRegistry = InEvaluatorRegistry;
	ThinkDelaySeconds = InThinkDelaySeconds;
}

void USGSBasicAIAgent::RequestPlayPhaseAction(
	const FSGSPlayPhaseRequest& Request,
	FSGSPlayPhaseDecisionDelegate OnDecided)
{
	const USGSGameContext* GameContext = Context.Get();
	check(GameContext != nullptr && EvaluatorRegistry != nullptr);
	const FSGSAIWorldView WorldView = FSGSAIWorldViewBuilder::Build(*GameContext, SeatIndex, Request.Phase);
	SubmitAfterThinkDelay(
		this,
		ThinkDelaySeconds,
		FSGSAIDecisionEngine::DecidePlay(Request, WorldView, *EvaluatorRegistry),
		MoveTemp(OnDecided));
}

void USGSBasicAIAgent::RequestResponseAction(
	const FSGSResponseRequest& Request,
	FSGSResponseDecisionDelegate OnDecided)
{
	const USGSGameContext* GameContext = Context.Get();
	check(GameContext != nullptr && EvaluatorRegistry != nullptr);
	const FSGSAIWorldView WorldView = FSGSAIWorldViewBuilder::Build(*GameContext, SeatIndex, Request.Phase);
	SubmitAfterThinkDelay(
		this,
		ThinkDelaySeconds,
		FSGSAIDecisionEngine::DecideResponse(Request, WorldView, *EvaluatorRegistry),
		MoveTemp(OnDecided));
}
