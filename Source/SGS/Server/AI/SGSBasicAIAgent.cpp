#include "Server/AI/SGSBasicAIAgent.h"

#include "Server/AI/SGSAIEvaluation.h"
#include "Server/Engine/SGSGameContext.h"

void USGSBasicAIAgent::BindToSeat(
	USGSGameContext* InContext,
	int32 InSeatIndex,
	const FSGSAIEvaluatorRegistry* InEvaluatorRegistry)
{
	Context = InContext;
	SeatIndex = InSeatIndex;
	EvaluatorRegistry = InEvaluatorRegistry;
}

void USGSBasicAIAgent::RequestPlayPhaseAction(
	const FSGSPlayPhaseRequest& Request,
	FSGSPlayPhaseDecisionDelegate OnDecided)
{
	const USGSGameContext* GameContext = Context.Get();
	check(GameContext != nullptr && EvaluatorRegistry != nullptr);
	const FSGSAIWorldView WorldView = FSGSAIWorldViewBuilder::Build(*GameContext, SeatIndex, Request.Phase);
	OnDecided.ExecuteIfBound(FSGSAIDecisionEngine::DecidePlay(Request, WorldView, *EvaluatorRegistry));
}

void USGSBasicAIAgent::RequestResponseAction(
	const FSGSResponseRequest& Request,
	FSGSResponseDecisionDelegate OnDecided)
{
	const USGSGameContext* GameContext = Context.Get();
	check(GameContext != nullptr && EvaluatorRegistry != nullptr);
	const FSGSAIWorldView WorldView = FSGSAIWorldViewBuilder::Build(*GameContext, SeatIndex, Request.Phase);
	OnDecided.ExecuteIfBound(FSGSAIDecisionEngine::DecideResponse(Request, WorldView, *EvaluatorRegistry));
}
