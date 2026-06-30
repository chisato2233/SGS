#include "Misc/AutomationTest.h"

#include "Server/AI/SGSScriptedDecisionAgent.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Shared/Cards/SGSDeckTypes.h"
#include "Server/Commands/SGSCommandRouter.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Engine/SGSGameDriver.h"
#include "Server/Players/SGSSeat.h"
#include "Server/UI/SGSTableSnapshotBuilder.h"
#include "Client/UI/Bridge/SGSLocalHumanDecisionAgent.h"
#include "Client/UI/ViewModel/SGSLocalDecisionPromptViewModel.h"
#include "Client/UI/Widgets/SGSTableHudWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FSGSDeckCardSpec MakePlan0011BasicCard(FName CardName, FSGSSuit Suit, int32 Number)
{
	FSGSDeckCardSpec Spec;
	Spec.CardName = CardName;
	Spec.Suit = Suit;
	Spec.Number = Number;
	Spec.CardType = SGSGameplayTags::CardType_Basic.GetTag();
	return Spec;
}

TScriptInterface<ISGSDecisionAgent> MakeLocalAgent(USGSLocalHumanDecisionAgent*& OutAgent)
{
	OutAgent = NewObject<USGSLocalHumanDecisionAgent>();
	TScriptInterface<ISGSDecisionAgent> Ref;
	Ref.SetObject(OutAgent);
	Ref.SetInterface(Cast<ISGSDecisionAgent>(OutAgent));
	return Ref;
}

TScriptInterface<ISGSDecisionAgent> MakeScriptedAgent(USGSScriptedDecisionAgent*& OutAgent)
{
	OutAgent = NewObject<USGSScriptedDecisionAgent>();
	TScriptInterface<ISGSDecisionAgent> Ref;
	Ref.SetObject(OutAgent);
	Ref.SetInterface(Cast<ISGSDecisionAgent>(OutAgent));
	return Ref;
}

TArray<FSGSDeckCardSpec> MakeLocalPlayDeck()
{
	TArray<FSGSDeckCardSpec> Deck;
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 7));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 8));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 9));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Heart.GetTag(), 10));

	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 11));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 12));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 13));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Heart.GetTag(), 1));
	return Deck;
}

TArray<FSGSDeckCardSpec> MakeLocalResponseDeck()
{
	TArray<FSGSDeckCardSpec> Deck;
	Deck.Add(MakePlan0011BasicCard(TEXT("Dodge"), SGSGameplayTags::Suit_Heart.GetTag(), 2));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 8));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 9));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Heart.GetTag(), 10));

	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 7));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 11));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 12));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Heart.GetTag(), 13));
	return Deck;
}

TArray<FSGSDeckCardSpec> MakeLocalDyingPeachDeck()
{
	TArray<FSGSDeckCardSpec> Deck;
	Deck.Add(MakePlan0011BasicCard(TEXT("Peach"), SGSGameplayTags::Suit_Diamond.GetTag(), 3));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 8));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 9));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Heart.GetTag(), 10));

	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 7));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 11));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Diamond.GetTag(), 12));
	Deck.Add(MakePlan0011BasicCard(TEXT("Slash"), SGSGameplayTags::Suit_Heart.GetTag(), 13));
	return Deck;
}

USGSGameDriver* StartTwoSeatGame(
	USGSLocalHumanDecisionAgent*& OutLocalAgent,
	USGSScriptedDecisionAgent*& OutScriptedAgent,
	TArray<FSGSDeckCardSpec> Deck,
	int32 MaxTurns)
{
	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	Agents.Add(MakeLocalAgent(OutLocalAgent));
	Agents.Add(MakeScriptedAgent(OutScriptedAgent));

	FSGSGameStartConfig Config;
	Config.RandomSeed = 11;
	Config.InitialDeck = MoveTemp(Deck);
	Config.bShuffleInitialDeck = false;
	Config.StartingHandSize = 4;
	Config.MaxTurns = MaxTurns;

	USGSGameDriver* Driver = NewObject<USGSGameDriver>();
	Driver->StartGame(Agents, Config);
	return Driver;
}

USGSGameDriver* StartFourSeatLocalGame(USGSLocalHumanDecisionAgent*& OutLocalAgent)
{
	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	Agents.Add(MakeLocalAgent(OutLocalAgent));

	for (int32 SeatIndex = 1; SeatIndex < 4; ++SeatIndex)
	{
		USGSScriptedDecisionAgent* ScriptedAgent = nullptr;
		Agents.Add(MakeScriptedAgent(ScriptedAgent));
	}

	FSGSGameStartConfig Config;
	Config.RandomSeed = 13;
	Config.InitialDeck = SGSDeckDefinitions::MakePlan0005SmokeDeck(4);
	Config.bShuffleInitialDeck = false;
	Config.StartingHandSize = 4;
	Config.MaxTurns = 1;

	USGSGameDriver* Driver = NewObject<USGSGameDriver>();
	Driver->StartGame(Agents, Config);
	return Driver;
}

USGSGameDriver* StartLocalDyingPeachGame(
	USGSLocalHumanDecisionAgent*& OutLocalAgent,
	USGSScriptedDecisionAgent*& OutScriptedAgent)
{
	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	Agents.Add(MakeLocalAgent(OutLocalAgent));
	Agents.Add(MakeScriptedAgent(OutScriptedAgent));

	FSGSGameStartConfig Config;
	Config.RandomSeed = 17;
	Config.InitialDeck = MakeLocalDyingPeachDeck();
	Config.bShuffleInitialDeck = false;
	Config.StartingHandSize = 4;
	Config.MaxTurns = 2;
	Config.InitialHealthBySeat.Add(0, 1);

	USGSGameDriver* Driver = NewObject<USGSGameDriver>();
	Driver->StartGame(Agents, Config);
	return Driver;
}

int32 FindPlayableCardId(const USGSLocalHumanDecisionAgent* Agent, FName CardName, int32 TargetSeat)
{
	const FSGSPlayPhaseRequest* Request = Agent != nullptr ? Agent->GetPendingPlayRequest() : nullptr;
	if (Request == nullptr)
	{
		return INDEX_NONE;
	}

	for (const FSGSCardActionOption& Option : Request->Options)
	{
		if (Option.CardName == CardName && Option.TargetSeatIndices.Contains(TargetSeat))
		{
			return Option.CardId;
		}
	}
	return INDEX_NONE;
}

int32 FindResponseCardId(const USGSLocalHumanDecisionAgent* Agent, FName CardName)
{
	const FSGSResponseRequest* Request = Agent != nullptr ? Agent->GetPendingResponseRequest() : nullptr;
	if (Request == nullptr || Request->RequiredCardName != CardName || Request->ResponseCardIds.Num() == 0)
	{
		return INDEX_NONE;
	}
	return Request->ResponseCardIds[0];
}

bool HasExecutedLocalCommand(const USGSGameDriver* Driver, const FNativeGameplayTag& CommandType)
{
	if (Driver == nullptr)
	{
		return false;
	}

	for (const FSGSCommandLogEntry& Entry : Driver->GetCommandLog())
	{
		if (Entry.bSucceeded
			&& Entry.Lifecycle == SGSCommandLifecycle::Executed()
			&& Entry.Command.SourceChannel == TEXT("LocalUI")
			&& Entry.Command.Type.MatchesTagExact(CommandType.GetTag()))
		{
			return true;
		}
	}
	return false;
}

FSGSTableViewSnapshot BuildLocalTableSnapshot(
	const USGSGameDriver* Driver,
	const USGSLocalHumanDecisionAgent* Agent,
	int32 ViewerSeat)
{
	FSGSTableViewSnapshot Snapshot = FSGSTableSnapshotBuilder::Build(Driver, ViewerSeat);
	FSGSLocalDecisionPromptViewModel::Apply(Snapshot, Agent);
	return Snapshot;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0011M1LocalUIBridgeTest,
	"SGS.Plan0011M1.LocalUI.DecisionBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0011M1LocalUIBridgeTest::RunTest(const FString& Parameters)
{
	USGSLocalHumanDecisionAgent* PlayLocalAgent = nullptr;
	USGSScriptedDecisionAgent* PlaySeat1Agent = nullptr;
	PlaySeat1Agent = nullptr;
	USGSGameDriver* PlayDriver = StartTwoSeatGame(
		PlayLocalAgent,
		PlaySeat1Agent,
		MakeLocalPlayDeck(),
		1);
	PlaySeat1Agent->EnqueueResponsePass();

	TestNotNull(TEXT("Local play driver is created."), PlayDriver);
	TestTrue(TEXT("Local agent receives a play prompt."), PlayLocalAgent->HasPendingPlayRequest());

	const FSGSTableViewSnapshot PlaySnapshot = BuildLocalTableSnapshot(PlayDriver, PlayLocalAgent, 0);
	TestTrue(TEXT("ViewModel exposes a local play prompt."), PlaySnapshot.Prompt.bHasPrompt && !PlaySnapshot.Prompt.bIsResponse);
	TestEqual(TEXT("ViewModel exposes two seats."), PlaySnapshot.Seats.Num(), 2);
	TestTrue(TEXT("ViewModel exposes local hand cards."), PlaySnapshot.HandCards.Num() > 0);
	TSharedRef<SSGSTableHudWidget> HudWidget = SNew(SSGSTableHudWidget)
		.SnapshotProvider([PlayDriver]()
			{
				return FSGSTableSnapshotBuilder::Build(PlayDriver, 0);
			})
		.DecisionAgent(PlayLocalAgent)
		.ViewerSeat(0);
	TestTrue(TEXT("Slate HUD widget can be constructed for the local match."), HudWidget->GetVisibility().IsVisible());
	HudWidget->SlatePrepass();
	TestTrue(TEXT("Slate HUD widget has non-zero desired size."), HudWidget->GetDesiredSize().X > 0.0f && HudWidget->GetDesiredSize().Y > 0.0f);

	const int32 SlashCardId = FindPlayableCardId(PlayLocalAgent, TEXT("Slash"), 1);
	TestTrue(TEXT("Local prompt contains a legal Slash target."), SlashCardId != INDEX_NONE);
	TestTrue(TEXT("Submitting local Slash succeeds."), PlayLocalAgent->SubmitUseCard(SlashCardId, 1));
	TestEqual(TEXT("Local Slash damages target through GameContext."), PlayDriver->GetContext()->GetSeat(1)->Health, 3);
	TestTrue(TEXT("Local Slash is audited as a LocalUI UseCard command."),
		HasExecutedLocalCommand(PlayDriver, SGSGameplayTags::PlayAction_UseCard));
	TestTrue(TEXT("Local play scenario keeps invariants."), PlayDriver->GetContext()->CheckInvariants());

	USGSLocalHumanDecisionAgent* FourSeatLocalAgent = nullptr;
	USGSGameDriver* FourSeatDriver = StartFourSeatLocalGame(FourSeatLocalAgent);
	const FSGSTableViewSnapshot FourSeatSnapshot = BuildLocalTableSnapshot(FourSeatDriver, FourSeatLocalAgent, 0);
	TestEqual(TEXT("Default local table snapshot exposes four seats."), FourSeatSnapshot.Seats.Num(), 4);
	TestEqual(TEXT("Default local table snapshot is viewed by seat zero."), FourSeatSnapshot.ViewerSeat, 0);
	TestTrue(TEXT("Default local table snapshot exposes draw pile count."), FourSeatSnapshot.DrawPileCount >= 0);
	TestTrue(TEXT("Default local table snapshot exposes local hand."), FourSeatSnapshot.HandCards.Num() > 0);
	TestTrue(TEXT("Default local table snapshot exposes a local prompt."), FourSeatSnapshot.Prompt.bHasPrompt);

	TSharedRef<SSGSTableHudWidget> FourSeatHudWidget = SNew(SSGSTableHudWidget)
		.SnapshotProvider([FourSeatDriver]()
			{
				return FSGSTableSnapshotBuilder::Build(FourSeatDriver, 0);
			})
		.DecisionAgent(FourSeatLocalAgent)
		.ViewerSeat(0);
	FourSeatHudWidget->SlatePrepass();
	const FVector2D FourSeatDesiredSize = FourSeatHudWidget->GetDesiredSize();
	TestTrue(TEXT("Four-seat Slate HUD has non-zero desired size."),
		FourSeatDesiredSize.X > 0.0f && FourSeatDesiredSize.Y > 0.0f);

	USGSLocalHumanDecisionAgent* ResponseLocalAgent = nullptr;
	USGSScriptedDecisionAgent* ResponseSeat1Agent = nullptr;
	USGSGameDriver* ResponseDriver = StartTwoSeatGame(
		ResponseLocalAgent,
		ResponseSeat1Agent,
		MakeLocalResponseDeck(),
		2);
	ResponseSeat1Agent->EnqueuePlayCard(TEXT("Slash"), 0);

	TestTrue(TEXT("Local response scenario starts at local play prompt."), ResponseLocalAgent->HasPendingPlayRequest());
	TestTrue(TEXT("Local player can pass their play phase."), ResponseLocalAgent->SubmitPass());
	TestTrue(TEXT("Local agent receives a Dodge response prompt."), ResponseLocalAgent->HasPendingResponseRequest());

	const FSGSTableViewSnapshot ResponseSnapshot = BuildLocalTableSnapshot(ResponseDriver, ResponseLocalAgent, 0);
	TestTrue(TEXT("ViewModel exposes a local response prompt."), ResponseSnapshot.Prompt.bHasPrompt && ResponseSnapshot.Prompt.bIsResponse);
	TestEqual(TEXT("Response prompt asks for Dodge."), ResponseSnapshot.Prompt.RequiredCardName, FName(TEXT("Dodge")));

	const int32 DodgeCardId = FindResponseCardId(ResponseLocalAgent, TEXT("Dodge"));
	TestTrue(TEXT("Local response prompt contains Dodge card."), DodgeCardId != INDEX_NONE);
	TestTrue(TEXT("Submitting local Dodge succeeds."), ResponseLocalAgent->SubmitResponseCard(DodgeCardId));
	TestEqual(TEXT("Local Dodge prevents Slash damage."), ResponseDriver->GetContext()->GetSeat(0)->Health, 4);
	TestTrue(TEXT("Local Dodge is audited as a LocalUI RespondCard command."),
		HasExecutedLocalCommand(ResponseDriver, SGSGameplayTags::PlayAction_RespondCard));
	TestTrue(TEXT("Local response scenario keeps invariants."), ResponseDriver->GetContext()->CheckInvariants());

	USGSLocalHumanDecisionAgent* PeachLocalAgent = nullptr;
	USGSScriptedDecisionAgent* PeachSeat1Agent = nullptr;
	USGSGameDriver* PeachDriver = StartLocalDyingPeachGame(PeachLocalAgent, PeachSeat1Agent);
	PeachSeat1Agent->EnqueuePlayCard(TEXT("Slash"), 0);

	TestTrue(TEXT("Local Peach scenario starts at local play prompt."), PeachLocalAgent->HasPendingPlayRequest());
	TestTrue(TEXT("Local player can pass before being attacked."), PeachLocalAgent->SubmitPass());
	TestTrue(TEXT("Local player receives a Dodge response before dying."), PeachLocalAgent->HasPendingResponseRequest());
	TestEqual(TEXT("First response window asks for Dodge."), PeachLocalAgent->GetPendingResponseRequest()->RequiredCardName, FName(TEXT("Dodge")));
	TestTrue(TEXT("Local player can pass the Dodge response."), PeachLocalAgent->SubmitPass());
	TestTrue(TEXT("Local player receives a Peach dying response prompt."), PeachLocalAgent->HasPendingResponseRequest());
	TestEqual(TEXT("Dying response prompt asks for Peach."), PeachLocalAgent->GetPendingResponseRequest()->RequiredCardName, FName(TEXT("Peach")));

	const FSGSTableViewSnapshot PeachSnapshot = BuildLocalTableSnapshot(PeachDriver, PeachLocalAgent, 0);
	TestTrue(TEXT("ViewModel exposes the local Peach response prompt."), PeachSnapshot.Prompt.bHasPrompt && PeachSnapshot.Prompt.bIsResponse);
	TestEqual(TEXT("ViewModel Peach prompt targets the dying local seat."), PeachSnapshot.Prompt.RequiredCardName, FName(TEXT("Peach")));

	const int32 PeachCardId = FindResponseCardId(PeachLocalAgent, TEXT("Peach"));
	TestTrue(TEXT("Local dying prompt contains Peach card."), PeachCardId != INDEX_NONE);
	TestTrue(TEXT("Submitting local Peach succeeds."), PeachLocalAgent->SubmitResponseCard(PeachCardId));
	TestTrue(TEXT("Local Peach rescue keeps the local player alive."), PeachDriver->GetContext()->GetSeat(0)->bIsAlive);
	TestEqual(TEXT("Local Peach rescue restores local player to one HP."), PeachDriver->GetContext()->GetSeat(0)->Health, 1);
	TestTrue(TEXT("Local Peach response is audited as a LocalUI RespondCard command."),
		HasExecutedLocalCommand(PeachDriver, SGSGameplayTags::PlayAction_RespondCard));
	TestTrue(TEXT("Local Peach scenario keeps invariants."), PeachDriver->GetContext()->CheckInvariants());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
