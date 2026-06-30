#include "Server/AI/SGSBasicAIAgent.h"

#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"

void USGSBasicAIAgent::RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided)
{
	for (const FSGSCardActionOption& Option : Request.Options)
	{
		if (Option.CardName == TEXT("Slash") && Option.TargetSeatIndices.Num() > 0)
		{
			FSGSPlayPhaseDecision Decision;
			Decision.Command = FSGSCommandFactory::Make(
				FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), FName(TEXT("BasicAI.Slash"))),
				FSGSUseCardCommandPayload(Option.CardId, TArray<int32>{ Option.TargetSeatIndices[0] }));
			OnDecided.ExecuteIfBound(Decision);
			return;
		}
	}

	for (const FSGSCardActionOption& Option : Request.Options)
	{
		if (Option.CardName == TEXT("Peach"))
		{
			FSGSPlayPhaseDecision Decision;
			Decision.Command = FSGSCommandFactory::Make(
				FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), FName(TEXT("BasicAI.Peach"))),
				FSGSUseCardCommandPayload(Option.CardId, TArray<int32>{ Request.SeatIndex }));
			OnDecided.ExecuteIfBound(Decision);
			return;
		}
	}

	FSGSPlayPhaseDecision Decision;
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), FName(TEXT("BasicAI.Pass"))),
		FSGSPassCommandPayload());
	OnDecided.ExecuteIfBound(Decision);
}

void USGSBasicAIAgent::RequestResponseAction(const FSGSResponseRequest& Request, FSGSResponseDecisionDelegate OnDecided)
{
	if (Request.ResponseCardIds.Num() > 0 && !Request.RequiredCardName.IsNone())
	{
		FSGSResponseDecision Decision;
		TArray<int32> Targets;
		if (Request.RequiredCardName == TEXT("Peach") && Request.EffectTargetSeat != INDEX_NONE)
		{
			Targets.Add(Request.EffectTargetSeat);
		}
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), FName(TEXT("BasicAI.Response"))),
			FSGSRespondCardCommandPayload(Request.ResponseCardIds[0], MoveTemp(Targets), Request.WindowName));
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
