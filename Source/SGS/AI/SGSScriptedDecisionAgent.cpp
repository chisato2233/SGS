#include "AI/SGSScriptedDecisionAgent.h"

FSGSScriptedDecisionStep USGSScriptedDecisionAgent::MakeCardStep(FName CardName, int32 TargetSeatIndex)
{
	FSGSScriptedDecisionStep Step;
	Step.bPass = false;
	Step.CardName = CardName;
	Step.TargetSeatIndex = TargetSeatIndex;
	return Step;
}

void USGSScriptedDecisionAgent::EnqueuePlayCard(FName CardName, int32 TargetSeatIndex)
{
	PlaySteps.Add(MakeCardStep(CardName, TargetSeatIndex));
}

void USGSScriptedDecisionAgent::EnqueuePlayPass()
{
	PlaySteps.Add(FSGSScriptedDecisionStep());
}

void USGSScriptedDecisionAgent::EnqueueInvalidPlayCard(int32 TargetSeatIndex)
{
	FSGSScriptedDecisionStep Step = MakeCardStep(TEXT("Invalid"), TargetSeatIndex);
	Step.bUseInvalidCard = true;
	PlaySteps.Add(Step);
}

void USGSScriptedDecisionAgent::EnqueueResponseCard(FName CardName, int32 TargetSeatIndex)
{
	ResponseSteps.Add(MakeCardStep(CardName, TargetSeatIndex));
}

void USGSScriptedDecisionAgent::EnqueueResponsePass()
{
	ResponseSteps.Add(FSGSScriptedDecisionStep());
}

FSGSScriptedDecisionStep USGSScriptedDecisionAgent::PopPlayStep()
{
	if (PlaySteps.Num() == 0)
	{
		return FSGSScriptedDecisionStep();
	}

	FSGSScriptedDecisionStep Step = PlaySteps[0];
	PlaySteps.RemoveAt(0);
	return Step;
}

FSGSScriptedDecisionStep USGSScriptedDecisionAgent::PopResponseStep()
{
	if (ResponseSteps.Num() == 0)
	{
		return FSGSScriptedDecisionStep();
	}

	FSGSScriptedDecisionStep Step = ResponseSteps[0];
	ResponseSteps.RemoveAt(0);
	return Step;
}

void USGSScriptedDecisionAgent::RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided)
{
	const FSGSScriptedDecisionStep Step = PopPlayStep();
	FSGSPlayPhaseDecision Decision;
	if (!Step.bPass)
	{
		int32 CardId = INDEX_NONE;
		if (Step.bUseInvalidCard)
		{
			CardId = 999999;
		}
		else
		{
			for (const FSGSCardActionOption& Option : Request.Options)
			{
				if (Option.CardName == Step.CardName)
				{
					CardId = Option.CardId;
					break;
				}
			}
		}

		if (CardId != INDEX_NONE)
		{
			TArray<int32> Targets;
			if (Step.TargetSeatIndex != INDEX_NONE)
			{
				Targets.Add(Step.TargetSeatIndex);
			}
			Decision.Command = FSGSCommand::MakeUseCard(
				Request.CommandId,
				Request.RequestId,
				Request.SeatIndex,
				Request.Phase,
				CardId,
				MoveTemp(Targets),
				FName(TEXT("Scripted")),
				Step.CardName.IsNone() ? FName(TEXT("Scripted.UseCard")) : Step.CardName);
			OnDecided.ExecuteIfBound(Decision);
			return;
		}
	}

	Decision.Command = FSGSCommand::MakePass(
		Request.CommandId,
		Request.RequestId,
		Request.SeatIndex,
		Request.Phase,
		FName(TEXT("Scripted")),
		FName(TEXT("Scripted.Pass")));
	OnDecided.ExecuteIfBound(Decision);
}

void USGSScriptedDecisionAgent::RequestResponseAction(const FSGSResponseRequest& Request, FSGSResponseDecisionDelegate OnDecided)
{
	const FSGSScriptedDecisionStep Step = PopResponseStep();
	FSGSResponseDecision Decision;
	if (!Step.bPass && Step.CardName == Request.RequiredCardName && Request.ResponseCardIds.Num() > 0)
	{
		TArray<int32> Targets;
		if (Step.TargetSeatIndex != INDEX_NONE)
		{
			Targets.Add(Step.TargetSeatIndex);
		}
		Decision.Command = FSGSCommand::MakeRespondCard(
			Request.CommandId,
			Request.RequestId,
			Request.SeatIndex,
			Request.Phase,
			Request.ResponseCardIds[0],
			MoveTemp(Targets),
			Request.WindowName,
			FName(TEXT("Scripted")),
			Step.CardName);
		OnDecided.ExecuteIfBound(Decision);
		return;
	}

	Decision.Command = FSGSCommand::MakePass(
		Request.CommandId,
		Request.RequestId,
		Request.SeatIndex,
		Request.Phase,
		FName(TEXT("Scripted")),
		Request.WindowName.IsNone() ? FName(TEXT("Scripted.ResponsePass")) : Request.WindowName);
	OnDecided.ExecuteIfBound(Decision);
}
