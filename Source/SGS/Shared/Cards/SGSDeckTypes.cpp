#include "Shared/Cards/SGSDeckTypes.h"

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

FSGSDeckCardSpec MakeCard(
	FName CardName,
	FSGSCardType CardType,
	FSGSSuit Suit,
	int32 Number,
	int32 Count = 1)
{
	FSGSDeckCardSpec Spec;
	Spec.CardName = CardName;
	Spec.CardType = CardType;
	Spec.Suit = Suit;
	Spec.Number = Number;
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

TArray<FSGSDeckCardSpec> SGSDeckDefinitions::MakeMinimalIdentityDeck()
{
	return {
		MakeBasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 7, 32),
		MakeBasicCard(TEXT("Dodge"), SGSGameplayTags::Suit_Heart.GetTag(), 2, 16),
		MakeBasicCard(TEXT("Peach"), SGSGameplayTags::Suit_Diamond.GetTag(), 3, 8),
		MakeBasicCard(TEXT("Analeptic"), SGSGameplayTags::Suit_Club.GetTag(), 9, 8)
	};
}

TArray<FSGSDeckCardSpec> SGSDeckDefinitions::MakeStandardIdentityDeck()
{
	const FSGSCardType Basic = SGSGameplayTags::CardType_Basic.GetTag();
	const FSGSCardType Trick = SGSGameplayTags::CardType_Trick.GetTag();
	const FSGSCardType Equipment = SGSGameplayTags::CardType_Equipment.GetTag();
	const FSGSSuit Spade = SGSGameplayTags::Suit_Spade.GetTag();
	const FSGSSuit Heart = SGSGameplayTags::Suit_Heart.GetTag();
	const FSGSSuit Club = SGSGameplayTags::Suit_Club.GetTag();
	const FSGSSuit Diamond = SGSGameplayTags::Suit_Diamond.GetTag();
	return {
		MakeCard(TEXT("Slash"), Basic, Spade, 7, 15),
		MakeCard(TEXT("Slash"), Basic, Club, 8, 15),
		MakeCard(TEXT("Dodge"), Basic, Heart, 2, 8),
		MakeCard(TEXT("Dodge"), Basic, Diamond, 2, 7),
		MakeCard(TEXT("Peach"), Basic, Heart, 3, 4),
		MakeCard(TEXT("Peach"), Basic, Diamond, 3, 4),
		MakeCard(TEXT("Analeptic"), Basic, Spade, 9, 2),
		MakeCard(TEXT("Analeptic"), Basic, Club, 9, 3),
		MakeCard(TEXT("ExNihilo"), Trick, Heart, 7, 4),
		MakeCard(TEXT("Dismantlement"), Trick, Spade, 3, 3),
		MakeCard(TEXT("Dismantlement"), Trick, Club, 3, 3),
		MakeCard(TEXT("Snatch"), Trick, Spade, 4, 2),
		MakeCard(TEXT("Snatch"), Trick, Diamond, 4, 3),
		MakeCard(TEXT("Duel"), Trick, Spade, 1, 3),
		MakeCard(TEXT("SavageAssault"), Trick, Spade, 7, 3),
		MakeCard(TEXT("ArcheryAttack"), Trick, Heart, 1),
		MakeCard(TEXT("GodSalvation"), Trick, Heart, 1),
		MakeCard(TEXT("AmazingGrace"), Trick, Heart, 3, 2),
		MakeCard(TEXT("Nullification"), Trick, Spade, 11, 2),
		MakeCard(TEXT("Nullification"), Trick, Club, 12),
		MakeCard(TEXT("Indulgence"), Trick, Spade, 6, 3),
		MakeCard(TEXT("SupplyShortage"), Trick, Club, 4, 2),
		MakeCard(TEXT("Lightning"), Trick, Spade, 1, 2),
		MakeCard(TEXT("Crossbow"), Equipment, Club, 1),
		MakeCard(TEXT("DoubleSword"), Equipment, Spade, 2),
		MakeCard(TEXT("QinggangSword"), Equipment, Spade, 6),
		MakeCard(TEXT("Blade"), Equipment, Spade, 5),
		MakeCard(TEXT("Spear"), Equipment, Spade, 12),
		MakeCard(TEXT("Axe"), Equipment, Diamond, 5),
		MakeCard(TEXT("Halberd"), Equipment, Diamond, 12),
		MakeCard(TEXT("KylinBow"), Equipment, Heart, 5),
		MakeCard(TEXT("BaguaArmor"), Equipment, Spade, 2),
		MakeCard(TEXT("RenwangShield"), Equipment, Club, 2),
		MakeCard(TEXT("Jueying"), Equipment, Spade, 5),
		MakeCard(TEXT("Dilu"), Equipment, Club, 5),
		MakeCard(TEXT("ZhuahuangFeidian"), Equipment, Heart, 13),
		MakeCard(TEXT("Chitu"), Equipment, Heart, 5),
		MakeCard(TEXT("Dayuan"), Equipment, Spade, 13),
		MakeCard(TEXT("Zixing"), Equipment, Diamond, 13),
	};
}
