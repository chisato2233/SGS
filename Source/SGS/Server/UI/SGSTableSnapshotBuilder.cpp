#include "Server/UI/SGSTableSnapshotBuilder.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Commands/SGSCommandRouter.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Engine/SGSGameDriver.h"
#include "Server/Players/SGSSeat.h"

namespace
{
FString SnapshotTagLeaf(const FGameplayTag& Tag)
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
	return FString::Printf(TEXT("%s %s%d"), *Card.CardName.ToString(), *SnapshotTagLeaf(Card.Suit), Card.Number);
}
}

FSGSTableViewSnapshot FSGSTableSnapshotBuilder::Build(const USGSGameDriver* Driver, int32 ViewerSeat)
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
		const FString DetailSuffix = LastEntry.Detail.IsEmpty()
			? FString()
			: FString::Printf(TEXT(" Detail=%s"), *LastEntry.Detail);
		Snapshot.LastCommand = FString::Printf(
			TEXT("%s %s%s"),
			*LastEntry.Lifecycle.ToString(),
			*LastEntry.Command.ToLogString(),
			*DetailSuffix);
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
		Snapshot.HandCards.Add(MoveTemp(CardView));
	}

	return Snapshot;
}
