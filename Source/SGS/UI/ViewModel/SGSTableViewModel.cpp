#include "UI/ViewModel/SGSTableViewModel.h"

#include "Logic/Cards/SGSCard.h"
#include "Logic/Commands/SGSCommandRouter.h"
#include "Logic/Engine/SGSGameContext.h"
#include "Logic/Engine/SGSGameDriver.h"
#include "Logic/Players/SGSDecisionTypes.h"
#include "Logic/Players/SGSSeat.h"
#include "UI/Bridge/SGSLocalHumanDecisionAgent.h"

namespace
{
FString TagLeaf(const FGameplayTag& Tag)
{
	FString Full = Tag.ToString();
	FString Left;
	FString Right;
	if (Full.Split(TEXT("."), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		return Right;
	}
	return Full;
}

FString FormatCard(const USGSCard& Card)
{
	return FString::Printf(TEXT("%s %s%d"), *Card.CardName.ToString(), *TagLeaf(Card.Suit), Card.Number);
}

void AddUniqueInts(TArray<int32>& Target, const TArray<int32>& Source)
{
	for (int32 Value : Source)
	{
		Target.AddUnique(Value);
	}
}
}

FSGSTableViewSnapshot FSGSTableViewModel::Build(
	const USGSGameDriver* Driver,
	const USGSLocalHumanDecisionAgent* DecisionAgent,
	int32 ViewerSeat)
{
	FSGSTableViewSnapshot Snapshot;
	Snapshot.ViewerSeat = ViewerSeat;

	if (Driver == nullptr)
	{
		return Snapshot;
	}

	USGSGameContext* Context = Driver->GetContext();
	Snapshot.CurrentSeatIndex = Driver->GetCurrentSeatIndex();
	Snapshot.CurrentPhase = Driver->GetCurrentPhase();
	Snapshot.bGameOver = Driver->IsGameOver();

	const TArray<FSGSCommandLogEntry>& CommandLog = Driver->GetCommandLog();
	if (CommandLog.Num() > 0)
	{
		const FSGSCommandLogEntry& LastEntry = CommandLog.Last();
		Snapshot.LastCommand = FString::Printf(
			TEXT("%s %s"),
			*LastEntry.Lifecycle.ToString(),
			*LastEntry.Command.ToLogString());
	}

	if (DecisionAgent != nullptr)
	{
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
	}

	if (Context == nullptr)
	{
		return Snapshot;
	}

	Snapshot.DrawPileCount = Context->GetCardsInZone(SGSGameplayTags::CardZone_DrawPile.GetTag()).Num();
	Snapshot.DiscardPileCount = Context->GetCardsInZone(SGSGameplayTags::CardZone_DiscardPile.GetTag()).Num();

	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		const USGSSeat* Seat = Context->GetSeat(SeatIndex);
		if (Seat == nullptr)
		{
			continue;
		}

		FSGSSeatViewData SeatView;
		SeatView.SeatIndex = SeatIndex;
		SeatView.DisplayName = Seat->DisplayName.IsEmpty()
			? FString::Printf(TEXT("Seat %d"), SeatIndex)
			: Seat->DisplayName;
		SeatView.Health = Seat->Health;
		SeatView.MaxHealth = Seat->MaxHealth;
		SeatView.bIsAlive = Seat->bIsAlive;
		SeatView.bIsCurrent = SeatIndex == Snapshot.CurrentSeatIndex;
		SeatView.bIsSelectableTarget = Snapshot.Prompt.SelectableTargetSeatIndices.Contains(SeatIndex);
		SeatView.HandCount = Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), SeatIndex).Num();
		Snapshot.Seats.Add(MoveTemp(SeatView));
	}

	for (USGSCard* Card : Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), ViewerSeat))
	{
		if (Card == nullptr)
		{
			continue;
		}

		FSGSCardViewData CardView;
		CardView.CardId = Card->CardId;
		CardView.CardName = Card->CardName;
		CardView.DisplayText = FormatCard(*Card);
		CardView.Suit = Card->Suit;
		CardView.Number = Card->Number;
		CardView.bSelectable = Snapshot.Prompt.SelectableCardIds.Contains(Card->CardId);
		Snapshot.HandCards.Add(MoveTemp(CardView));
	}

	return Snapshot;
}
