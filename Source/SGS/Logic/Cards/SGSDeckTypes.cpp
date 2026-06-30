#include "Logic/Cards/SGSDeckTypes.h"

namespace
{
FSGSDeckCardSpec MakeBasicCard(FName CardName, FSGSSuit Suit, int32 Number, int32 Count = 1)
{
	FSGSDeckCardSpec Spec;
	Spec.CardName = CardName;
	Spec.Suit = Suit;
	Spec.Number = Number;
	Spec.CardType = SGSGameplayTags::CardType_Basic.GetTag();
	Spec.Count = Count;
	return Spec;
}
}

TArray<FSGSDeckCardSpec> SGSDeckDefinitions::MakePlan0005SmokeDeck(int32 SeatCount)
{
	TArray<FSGSDeckCardSpec> Deck;
	const int32 SafeSeatCount = FMath::Max(SeatCount, 1);
	Deck.Reserve(SafeSeatCount * 4);

	for (int32 SeatIndex = 0; SeatIndex < SafeSeatCount; ++SeatIndex)
	{
		Deck.Add(MakeBasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 7));
		Deck.Add(MakeBasicCard(TEXT("Dodge"), SGSGameplayTags::Suit_Heart.GetTag(), 2));
		Deck.Add(MakeBasicCard(TEXT("Peach"), SGSGameplayTags::Suit_Diamond.GetTag(), 3));
		Deck.Add(MakeBasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 8));
	}

	return Deck;
}
