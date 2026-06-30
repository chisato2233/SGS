#include "AI/SGSBasicAIAgent.h"

void USGSBasicAIAgent::RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided)
{
	for (const FSGSCardActionOption& Option : Request.Options)
	{
		if (Option.CardName == TEXT("Slash") && Option.TargetSeatIndices.Num() > 0)
		{
			FSGSPlayPhaseDecision Decision;
			Decision.Command = FSGSCommand::MakeUseCard(
				Request.CommandId,
				Request.RequestId,
				Request.SeatIndex,
				Request.Phase,
				Option.CardId,
				TArray<int32>{ Option.TargetSeatIndices[0] },
				FName(TEXT("AI")),
				FName(TEXT("BasicAI.Slash")));
			OnDecided.ExecuteIfBound(Decision);
			return;
		}
	}

	for (const FSGSCardActionOption& Option : Request.Options)
	{
		if (Option.CardName == TEXT("Peach"))
		{
			FSGSPlayPhaseDecision Decision;
			Decision.Command = FSGSCommand::MakeUseCard(
				Request.CommandId,
				Request.RequestId,
				Request.SeatIndex,
				Request.Phase,
				Option.CardId,
				TArray<int32>{ Request.SeatIndex },
				FName(TEXT("AI")),
				FName(TEXT("BasicAI.Peach")));
			OnDecided.ExecuteIfBound(Decision);
			return;
		}
	}

	FSGSPlayPhaseDecision Decision;
	Decision.Command = FSGSCommand::MakePass(
		Request.CommandId,
		Request.RequestId,
		Request.SeatIndex,
		Request.Phase,
		FName(TEXT("AI")),
		FName(TEXT("BasicAI.Pass")));
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
		Decision.Command = FSGSCommand::MakeRespondCard(
			Request.CommandId,
			Request.RequestId,
			Request.SeatIndex,
			Request.Phase,
			Request.ResponseCardIds[0],
			MoveTemp(Targets),
			Request.WindowName,
			FName(TEXT("AI")),
			FName(TEXT("BasicAI.Response")));
		OnDecided.ExecuteIfBound(Decision);
		return;
	}

	FSGSResponseDecision Decision;
	Decision.Command = FSGSCommand::MakePass(
		Request.CommandId,
		Request.RequestId,
		Request.SeatIndex,
		Request.Phase,
		FName(TEXT("AI")),
		Request.WindowName.IsNone() ? FName(TEXT("BasicAI.ResponsePass")) : Request.WindowName);
	OnDecided.ExecuteIfBound(Decision);
}
