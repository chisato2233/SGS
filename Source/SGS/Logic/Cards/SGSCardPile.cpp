#include "Logic/Cards/SGSCardPile.h"

#include "Logic/Cards/SGSCard.h"
#include "Math/RandomStream.h"

bool USGSCardPile::Contains(const USGSCard* Card) const
{
	return Card != nullptr && Cards.Contains(Card);
}

void USGSCardPile::AddToTop(USGSCard* Card)
{
	if (Card != nullptr)
	{
		Cards.Insert(Card, 0);
	}
}

void USGSCardPile::AddToBottom(USGSCard* Card)
{
	if (Card != nullptr)
	{
		Cards.Add(Card);
	}
}

void USGSCardPile::AddManyToBottom(const TArray<USGSCard*>& InCards)
{
	for (USGSCard* Card : InCards)
	{
		AddToBottom(Card);
	}
}

TArray<USGSCard*> USGSCardPile::TakeFromTop(int32 Count)
{
	TArray<USGSCard*> Taken;
	const int32 TakeCount = FMath::Clamp(Count, 0, Cards.Num());
	if (TakeCount == 0)
	{
		return Taken;
	}

	Taken.Reserve(TakeCount);
	for (int32 Index = 0; Index < TakeCount; ++Index)
	{
		Taken.Add(Cards[Index]);
	}
	Cards.RemoveAt(0, TakeCount);
	return Taken;
}

bool USGSCardPile::RemoveCard(USGSCard* Card)
{
	return Card != nullptr && Cards.Remove(Card) > 0;
}

void USGSCardPile::Shuffle(const FRandomStream& Stream)
{
	// Fisher–Yates，使用确定性随机流（可复现）。
	const int32 LastIndex = Cards.Num() - 1;
	for (int32 Index = 0; Index < LastIndex; ++Index)
	{
		const int32 SwapIndex = Stream.RandRange(Index, LastIndex);
		if (SwapIndex != Index)
		{
			Cards.Swap(Index, SwapIndex);
		}
	}
}
