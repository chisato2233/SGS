#include "Misc/AutomationTest.h"

#include "Shared/Core/SGSGameplayTags.h"
#include "Shared/Cards/SGSCard.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Shared/Queries/SGSTargetQueryTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0004GameContextPrimitivesTest,
	"SGS.Plan0004.GameContextPrimitives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0004GameContextPrimitivesTest::RunTest(const FString& Parameters)
{
	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	Agents.SetNum(4);

	USGSGameContext* Context = NewObject<USGSGameContext>();
	Context->Initialize(Agents, 12345);

	int32 CardsMovedEvents = 0;
	int32 DamageEvents = 0;
	int32 HealthChangedEvents = 0;
	int32 SeatDyingEvents = 0;
	Context->OnCardsMoved().AddLambda([&CardsMovedEvents](const FSGSCardMoveInfo&)
	{
		++CardsMovedEvents;
	});
	Context->OnDamage().AddLambda([&DamageEvents](const FSGSDamageInfo&)
	{
		++DamageEvents;
	});
	Context->OnHealthChanged().AddLambda([&HealthChangedEvents](int32, int32)
	{
		++HealthChangedEvents;
	});
	Context->OnSeatDying().AddLambda([&SeatDyingEvents](int32)
	{
		++SeatDyingEvents;
	});

	TestEqual(TEXT("Context creates four seats."), Context->NumSeats(), 4);
	TestTrue(TEXT("Initial context invariants hold."), Context->CheckInvariants());

	USGSCard* Slash = Context->CreateCard(
		TEXT("Slash"),
		SGSGameplayTags::Suit_Spade.GetTag(),
		7,
		SGSGameplayTags::CardType_Basic.GetTag());
	USGSCard* Dodge = Context->CreateCard(
		TEXT("Dodge"),
		SGSGameplayTags::Suit_Heart.GetTag(),
		2,
		SGSGameplayTags::CardType_Basic.GetTag());
	USGSCard* Peach = Context->CreateCard(
		TEXT("Peach"),
		SGSGameplayTags::Suit_Diamond.GetTag(),
		3,
		SGSGameplayTags::CardType_Basic.GetTag());
	USGSCard* Duel = Context->CreateCard(
		TEXT("Duel"),
		SGSGameplayTags::Suit_Club.GetTag(),
		12,
		SGSGameplayTags::CardType_Trick.GetTag());

	TestNotNull(TEXT("Slash card exists."), Slash);
	TestNotNull(TEXT("Dodge card exists."), Dodge);
	TestNotNull(TEXT("Peach card exists."), Peach);
	TestNotNull(TEXT("Duel card exists."), Duel);
	TestTrue(TEXT("Spade card is black."), Slash->GetColor().MatchesTagExact(SGSGameplayTags::CardColor_Black.GetTag()));
	TestTrue(TEXT("Heart card is red."), Dodge->GetColor().MatchesTagExact(SGSGameplayTags::CardColor_Red.GetTag()));
	TestEqual(TEXT("Created cards enter draw pile."), Context->GetCardsInZone(SGSGameplayTags::CardZone_DrawPile.GetTag()).Num(), 4);

	TArray<USGSCard*> Seat0Drawn = Context->DrawCards(0, 2);
	TestEqual(TEXT("Seat 0 draws two cards."), Seat0Drawn.Num(), 2);
	TestEqual(TEXT("Seat 0 hand has two cards."), Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), 0).Num(), 2);
	TestEqual(TEXT("Draw pile has two cards left."), Context->GetCardsInZone(SGSGameplayTags::CardZone_DrawPile.GetTag()).Num(), 2);
	TestTrue(TEXT("Draw broadcasts card movement."), CardsMovedEvents > 0);

	FSGSCardQuery HandQuery;
	HandQuery.Zone = SGSGameplayTags::CardZone_Hand.GetTag();
	HandQuery.SeatIndex = 0;
	HandQuery.ViewerSeatIndex = 0;
	HandQuery.RequiredCardType = SGSGameplayTags::CardType_Basic.GetTag();
	const FSGSCardQueryResult VisibleHand = Context->QueryCards(HandQuery);
	TestEqual(TEXT("Owner can query own basic hand cards."), VisibleHand.Targets.Num(), 2);

	HandQuery.ViewerSeatIndex = 1;
	const FSGSCardQueryResult HiddenHand = Context->QueryCards(HandQuery);
	TestEqual(TEXT("Other seats cannot see private hand cards."), HiddenHand.Targets.Num(), 0);
	TestTrue(TEXT("Hidden hand query records a rejection."), HiddenHand.Rejections.Num() > 0);

	Context->DiscardFromHand(0, { Seat0Drawn[0] });
	TestEqual(TEXT("Discard removes one card from seat 0 hand."), Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), 0).Num(), 1);
	TestEqual(TEXT("Discard pile receives one card."), Context->GetCardsInZone(SGSGameplayTags::CardZone_DiscardPile.GetTag()).Num(), 1);

	TArray<USGSCard*> Seat1Drawn = Context->DrawCards(1, 3);
	TSet<USGSCard*> UniqueSeat1Drawn;
	for (USGSCard* Card : Seat1Drawn)
	{
		UniqueSeat1Drawn.Add(Card);
	}
	TestEqual(TEXT("Seat 1 draws through draw-pile depletion and discard reshuffle."), Seat1Drawn.Num(), 3);
	TestEqual(TEXT("Seat 1 draw result has no duplicate card pointers."), UniqueSeat1Drawn.Num(), 3);
	TestEqual(TEXT("Draw pile is empty after reshuffle draw."), Context->GetCardsInZone(SGSGameplayTags::CardZone_DrawPile.GetTag()).Num(), 0);
	TestEqual(TEXT("Discard pile is empty after reshuffle draw."), Context->GetCardsInZone(SGSGameplayTags::CardZone_DiscardPile.GetTag()).Num(), 0);
	TestEqual(TEXT("Seat 1 hand has three cards."), Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), 1).Num(), 3);

	Context->ApplyDamage(0, 1, 2);
	TestEqual(TEXT("Damage lowers health."), Context->GetSeat(1)->Health, 2);
	TestEqual(TEXT("Damage event fired once."), DamageEvents, 1);
	TestEqual(TEXT("Health changed event fired for damage."), HealthChangedEvents, 1);

	Context->Heal(1, 1);
	TestEqual(TEXT("Heal restores health."), Context->GetSeat(1)->Health, 3);
	TestEqual(TEXT("Health changed event fired for heal."), HealthChangedEvents, 2);

	Context->ApplyDamage(0, 1, 4);
	TestEqual(TEXT("Lethal damage floors health at zero."), Context->GetSeat(1)->Health, 0);
	TestEqual(TEXT("Dying event fired once."), SeatDyingEvents, 1);

	TestEqual(TEXT("Adjacent seats have distance one."), Context->GetDistance(0, 1), 1);
	TestEqual(TEXT("Opposite seats in a four-seat ring have distance two."), Context->GetDistance(0, 2), 2);
	TestEqual(TEXT("Ring wraps to distance one."), Context->GetDistance(0, 3), 1);

	Context->GetSeat(2)->Equipment.Add(SGSGameplayTags::EquipSlot_DefenseHorse.GetTag(), nullptr);
	TestEqual(TEXT("Defense horse increases distance to target."), Context->GetDistance(0, 2), 3);
	Context->GetSeat(0)->Equipment.Add(SGSGameplayTags::EquipSlot_OffenseHorse.GetTag(), nullptr);
	TestEqual(TEXT("Offense horse decreases outgoing distance."), Context->GetDistance(0, 2), 2);

	FSGSSeatQuery DistanceQuery;
	DistanceQuery.SourceSeat = 0;
	DistanceQuery.MaxDistance = 1;
	const FSGSSeatQueryResult DistanceTargets = Context->QuerySeats(DistanceQuery);
	TestEqual(TEXT("Distance query finds two adjacent seats."), DistanceTargets.Targets.Num(), 2);

	TestTrue(TEXT("Final context invariants hold."), Context->CheckInvariants());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
