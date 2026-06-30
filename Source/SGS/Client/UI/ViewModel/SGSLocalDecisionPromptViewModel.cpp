#include "Client/UI/ViewModel/SGSLocalDecisionPromptViewModel.h"

#include "Client/UI/Bridge/SGSLocalHumanDecisionAgent.h"
#include "Shared/Decisions/SGSDecisionTypes.h"

namespace
{
void AddUniqueInts(TArray<int32>& Target, const TArray<int32>& Source)
{
	for (int32 Value : Source)
	{
		Target.AddUnique(Value);
	}
}

void ApplySelectionState(FSGSTableViewSnapshot& Snapshot)
{
	for (FSGSSeatViewData& Seat : Snapshot.Seats)
	{
		Seat.bIsSelectableTarget = Snapshot.Prompt.SelectableTargetSeatIndices.Contains(Seat.SeatIndex);
	}

	for (FSGSCardViewData& Card : Snapshot.HandCards)
	{
		Card.bSelectable = Snapshot.Prompt.SelectableCardIds.Contains(Card.CardId);
	}
}
}

void FSGSLocalDecisionPromptViewModel::Apply(
	FSGSTableViewSnapshot& Snapshot,
	const USGSLocalHumanDecisionAgent* DecisionAgent)
{
	Snapshot.Prompt = FSGSDecisionPromptViewData();

	if (DecisionAgent == nullptr)
	{
		ApplySelectionState(Snapshot);
		return;
	}

	if (const FSGSPlayPhaseRequest* PlayRequest = DecisionAgent->GetPendingPlayRequest())
	{
		Snapshot.Prompt.bHasPrompt = true;
		Snapshot.Prompt.bIsResponse = false;
		Snapshot.Prompt.bAllowPass = PlayRequest->bAllowPass;
		for (const FSGSCardActionOption& Option : PlayRequest->Options)
		{
			Snapshot.Prompt.SelectableCardIds.AddUnique(Option.CardId);
			AddUniqueInts(Snapshot.Prompt.SelectableTargetSeatIndices, Option.TargetSeatIndices);
			Snapshot.Prompt.TargetSeatIndicesByCardId.Add(Option.CardId, Option.TargetSeatIndices);
		}
	}
	else if (const FSGSResponseRequest* ResponseRequest = DecisionAgent->GetPendingResponseRequest())
	{
		Snapshot.Prompt.bHasPrompt = true;
		Snapshot.Prompt.bIsResponse = true;
		Snapshot.Prompt.WindowName = ResponseRequest->WindowName;
		Snapshot.Prompt.RequiredCardName = ResponseRequest->RequiredCardName;
		Snapshot.Prompt.bAllowPass = ResponseRequest->bAllowPass;
		AddUniqueInts(Snapshot.Prompt.SelectableCardIds, ResponseRequest->ResponseCardIds);

		TArray<int32> ResponseTargets;
		if (ResponseRequest->EffectTargetSeat != INDEX_NONE)
		{
			Snapshot.Prompt.SelectableTargetSeatIndices.AddUnique(ResponseRequest->EffectTargetSeat);
			ResponseTargets.Add(ResponseRequest->EffectTargetSeat);
		}

		for (int32 CardId : ResponseRequest->ResponseCardIds)
		{
			Snapshot.Prompt.TargetSeatIndicesByCardId.Add(CardId, ResponseTargets);
		}
	}

	ApplySelectionState(Snapshot);
}
