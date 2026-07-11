#include "Misc/AutomationTest.h"

#include "Camera/CameraComponent.h"
#include "Client/Game/SGSTablePawn.h"
#include "Server/AI/SGSScriptedDecisionAgent.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Shared/Cards/SGSDeckTypes.h"
#include "Server/Commands/SGSCommandRouter.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Engine/SGSGameDriver.h"
#include "Server/Players/SGSSeat.h"
#include "Server/UI/SGSTableSnapshotBuilder.h"
#include "Client/UI/Features/Table/State/SGSTableUIStateStore.h"
#include "Client/UI/Features/Table/Controller/SGSTableFeatureController.h"
#include "Client/UI/Core/Context/SGSUIContext.h"
#include "Client/UI/Layout/SGSTableLayout.h"
#include "Client/UI/Bridge/SGSLocalHumanDecisionAgent.h"
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

TScriptInterface<ISGSDecisionAgent> MakePlan0011ScriptedAgent(USGSScriptedDecisionAgent*& OutAgent)
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
	Agents.Add(MakePlan0011ScriptedAgent(OutScriptedAgent));

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

USGSGameDriver* StartEightSeatLocalGame(USGSLocalHumanDecisionAgent*& OutLocalAgent)
{
	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	Agents.Add(MakeLocalAgent(OutLocalAgent));

	for (int32 SeatIndex = 1; SeatIndex < 8; ++SeatIndex)
	{
		USGSScriptedDecisionAgent* ScriptedAgent = nullptr;
		Agents.Add(MakePlan0011ScriptedAgent(ScriptedAgent));
	}

	FSGSGameStartConfig Config;
	Config.RandomSeed = 13;
	Config.InitialDeck = SGSDeckDefinitions::MakePlan0005SmokeDeck(8);
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
	Agents.Add(MakePlan0011ScriptedAgent(OutScriptedAgent));

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
	return SGSComposeTableViewSnapshot(
		FSGSTableSnapshotBuilder::BuildPublicSnapshot(Driver),
		FSGSTableSnapshotBuilder::BuildPrivateSnapshot(Driver, Agent, ViewerSeat));
}

bool HasAnyOpponentSeatOverlap(const FSGSTableLayoutMetrics& Layout)
{
	for (int32 Index = 0; Index < Layout.Seats.Num(); ++Index)
	{
		const FSGSTableSeatLayout& A = Layout.Seats[Index];
		if (A.RelativePosition == 0)
		{
			continue;
		}

		for (int32 OtherIndex = Index + 1; OtherIndex < Layout.Seats.Num(); ++OtherIndex)
		{
			const FSGSTableSeatLayout& B = Layout.Seats[OtherIndex];
			if (B.RelativePosition == 0)
			{
				continue;
			}

			if (FSGSTableLayoutMetrics::RectsOverlap(Layout.GetSeatRect(A), Layout.GetSeatRect(B)))
			{
				return true;
			}
		}
	}
	return false;
}

bool OpponentSeatsOverlapHandArea(const FSGSTableLayoutMetrics& Layout)
{
	for (const FSGSTableSeatLayout& Seat : Layout.Seats)
	{
		if (Seat.RelativePosition != 0
			&& FSGSTableLayoutMetrics::RectsOverlap(Layout.GetSeatRect(Seat), Layout.HandArea))
		{
			return true;
		}
	}
	return false;
}

bool MainTableAreasOverlap(const FSGSTableLayoutMetrics& Layout)
{
	const FSGSTableSeatLayout* MainSeat = Layout.FindSeat(0);
	if (MainSeat == nullptr)
	{
		return true;
	}

	const FSlateRect MainSeatRect = Layout.GetSeatRect(*MainSeat);
	return FSGSTableLayoutMetrics::RectsOverlap(MainSeatRect, Layout.HandArea)
		|| FSGSTableLayoutMetrics::RectsOverlap(MainSeatRect, Layout.ControlArea)
		|| FSGSTableLayoutMetrics::RectsOverlap(Layout.HandArea, Layout.ControlArea);
}

bool BackgroundCoversView(const FSlateRect& BackgroundArea, FVector2D ViewSize)
{
	const float Tolerance = 0.5f;
	return BackgroundArea.Left <= Tolerance
		&& BackgroundArea.Top <= Tolerance
		&& BackgroundArea.Right + Tolerance >= ViewSize.X
		&& BackgroundArea.Bottom + Tolerance >= ViewSize.Y;
}

bool BackgroundKeepsAspect(const FSlateRect& BackgroundArea, FVector2D ImageSize)
{
	const float Width = BackgroundArea.Right - BackgroundArea.Left;
	const float Height = BackgroundArea.Bottom - BackgroundArea.Top;
	if (Width <= 0.0f || Height <= 0.0f || ImageSize.X <= 0.0f || ImageSize.Y <= 0.0f)
	{
		return false;
	}
	return FMath::IsNearlyEqual(Width / Height, ImageSize.X / ImageSize.Y, 0.001f);
}

bool RectIsInsideView(const FSlateRect& Rect, FVector2D ViewSize)
{
	const float Tolerance = 0.5f;
	return Rect.Left >= -Tolerance
		&& Rect.Top >= -Tolerance
		&& Rect.Right <= ViewSize.X + Tolerance
		&& Rect.Bottom <= ViewSize.Y + Tolerance;
}

bool TableContentStaysInsideView(const FSGSTableLayoutMetrics& Layout)
{
	for (const FSGSTableSeatLayout& Seat : Layout.Seats)
	{
		if (!RectIsInsideView(Layout.GetSeatRect(Seat), Layout.ViewSize))
		{
			return false;
		}
	}

	return RectIsInsideView(Layout.HandArea, Layout.ViewSize)
		&& RectIsInsideView(Layout.ControlArea, Layout.ViewSize)
		&& RectIsInsideView(Layout.CenterArea, Layout.ViewSize);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0011M1LocalUIBridgeTest,
	"SGS.Plan0011M1.LocalUI.DecisionBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0011M1LocalUIBridgeTest::RunTest(const FString& Parameters)
{
	const ASGSTablePawn* TablePawnCDO = GetDefault<ASGSTablePawn>();
	const UCameraComponent* TableCamera = TablePawnCDO != nullptr
		? TablePawnCDO->FindComponentByClass<UCameraComponent>()
		: nullptr;
	TestNotNull(TEXT("Table pawn owns a camera component."), TableCamera);
	if (TableCamera != nullptr)
	{
		TestEqual(TEXT("Table camera uses orthographic projection."), TableCamera->ProjectionMode, ECameraProjectionMode::Orthographic);
		TestTrue(TEXT("Table camera is centered over the world table."),
			TableCamera->GetRelativeLocation().Equals(FVector(0.0f, 0.0f, 1200.0f), 0.1f));
		TestTrue(TEXT("Table camera looks straight down at the table."),
			TableCamera->GetRelativeRotation().Vector().Equals(FVector(0.0f, 0.0f, -1.0f), 0.001f));
		TestTrue(TEXT("Table camera width contains the development table."), TableCamera->OrthoWidth >= 1200.0f);
	}

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

	const FSGSTablePublicSnapshot PlayPublicSnapshot = FSGSTableSnapshotBuilder::BuildPublicSnapshot(PlayDriver);
	const FSGSPlayerPrivateSnapshot PlayPrivateSnapshot = FSGSTableSnapshotBuilder::BuildPrivateSnapshot(PlayDriver, PlayLocalAgent, 0);
	const FSGSTableViewSnapshot PlaySnapshot = SGSComposeTableViewSnapshot(PlayPublicSnapshot, PlayPrivateSnapshot);
	TestEqual(TEXT("Public snapshot exposes two seats."), PlayPublicSnapshot.Seats.Num(), 2);
	TestTrue(TEXT("Private snapshot exposes local hand cards."), PlayPrivateSnapshot.HandCards.Num() > 0);
	TestTrue(TEXT("Private snapshot exposes local prompt."), PlayPrivateSnapshot.Prompt.bHasPrompt);
	TestTrue(TEXT("ViewModel exposes a local play prompt."), PlaySnapshot.Prompt.bHasPrompt && !PlaySnapshot.Prompt.bIsResponse);
	TestEqual(TEXT("ViewModel exposes two seats."), PlaySnapshot.Seats.Num(), 2);
	TestTrue(TEXT("ViewModel exposes local hand cards."), PlaySnapshot.HandCards.Num() > 0);
	FSGSTableFeatureBindings PlayUIBindings;
	PlayUIBindings.ReadSnapshot = [PlayDriver, PlayLocalAgent]()
	{
		return BuildLocalTableSnapshot(PlayDriver, PlayLocalAgent, 0);
	};
	TSharedRef<FSGSTableFeatureController> PlayUIController = MakeShared<FSGSTableFeatureController>(
		0,
		MoveTemp(PlayUIBindings),
		MakeShared<FSGSUIContext>());
	PlayUIController->RefreshFromHost();
	TSharedRef<SSGSTableHudWidget> HudWidget = SNew(SSGSTableHudWidget)
		.Controller(PlayUIController);
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

	USGSLocalHumanDecisionAgent* EightSeatLocalAgent = nullptr;
	USGSGameDriver* EightSeatDriver = StartEightSeatLocalGame(EightSeatLocalAgent);
	const FSGSTableViewSnapshot EightSeatSnapshot = BuildLocalTableSnapshot(EightSeatDriver, EightSeatLocalAgent, 0);
	TestEqual(TEXT("Default local table snapshot exposes eight seats."), EightSeatSnapshot.Seats.Num(), 8);
	TestEqual(TEXT("Default local table snapshot is viewed by seat zero."), EightSeatSnapshot.ViewerSeat, 0);
	TestTrue(TEXT("Default local table snapshot exposes draw pile count."), EightSeatSnapshot.DrawPileCount >= 0);
	TestTrue(TEXT("Default local table snapshot exposes local hand."), EightSeatSnapshot.HandCards.Num() > 0);
	TestTrue(TEXT("Default local table snapshot exposes a local prompt."), EightSeatSnapshot.Prompt.bHasPrompt);

	FSGSTableFeatureBindings EightSeatUIBindings;
	EightSeatUIBindings.ReadSnapshot = [EightSeatDriver]()
	{
		return SGSComposeTableViewSnapshot(
			FSGSTableSnapshotBuilder::BuildPublicSnapshot(EightSeatDriver),
			FSGSTableSnapshotBuilder::BuildPrivateSnapshot(EightSeatDriver, nullptr, 0));
	};
	TSharedRef<FSGSTableFeatureController> EightSeatUIController = MakeShared<FSGSTableFeatureController>(
		0,
		MoveTemp(EightSeatUIBindings),
		MakeShared<FSGSUIContext>());
	EightSeatUIController->RefreshFromHost();
	TSharedRef<SSGSTableHudWidget> EightSeatHudWidget = SNew(SSGSTableHudWidget)
		.Controller(EightSeatUIController);
	EightSeatHudWidget->SlatePrepass();
	const FVector2D EightSeatDesiredSize = EightSeatHudWidget->GetDesiredSize();
	TestTrue(TEXT("Eight-seat Slate HUD has non-zero desired size."),
		EightSeatDesiredSize.X > 0.0f && EightSeatDesiredSize.Y > 0.0f);

	const TArray<FVector2D> LayoutViewSizes = {
		FVector2D(1920.0f, 1080.0f),
		FVector2D(1600.0f, 900.0f),
		FVector2D(1280.0f, 720.0f),
		FVector2D(960.0f, 540.0f),
		FVector2D(640.0f, 360.0f),
	};
	for (const FVector2D& ViewSize : LayoutViewSizes)
	{
		const FSGSTableLayoutMetrics Layout = FSGSTableLayoutMetrics::Make(ViewSize, 8, 0);
		TestEqual(FString::Printf(TEXT("Eight-seat layout %.0fx%.0f exposes eight seats."), ViewSize.X, ViewSize.Y), Layout.Seats.Num(), 8);
		TestTrue(FString::Printf(TEXT("Eight-seat layout %.0fx%.0f preserves the real viewport size."), ViewSize.X, ViewSize.Y), Layout.ViewSize.Equals(ViewSize, 0.1f));
		TestFalse(FString::Printf(TEXT("Eight-seat layout %.0fx%.0f opponent seats do not overlap."), ViewSize.X, ViewSize.Y), HasAnyOpponentSeatOverlap(Layout));
		TestFalse(FString::Printf(TEXT("Eight-seat layout %.0fx%.0f opponent seats do not overlap the hand area."), ViewSize.X, ViewSize.Y), OpponentSeatsOverlapHandArea(Layout));
		TestFalse(FString::Printf(TEXT("Eight-seat layout %.0fx%.0f main areas do not overlap."), ViewSize.X, ViewSize.Y), MainTableAreasOverlap(Layout));
		TestTrue(FString::Printf(TEXT("Eight-seat layout %.0fx%.0f stays inside the viewport."), ViewSize.X, ViewSize.Y), TableContentStaysInsideView(Layout));
		TestTrue(FString::Printf(TEXT("Eight-seat layout %.0fx%.0f uses portrait hand cards."), ViewSize.X, ViewSize.Y), Layout.HandCardSize.Y > Layout.HandCardSize.X);
		if (ViewSize.X < 1280.0f || ViewSize.Y < 720.0f)
		{
			TestTrue(FString::Printf(TEXT("Eight-seat layout %.0fx%.0f scales down for compact/high-DPI viewports."), ViewSize.X, ViewSize.Y), Layout.LayoutScale < 1.0f);
		}
	}

	const FVector2D BackgroundImageSize(1334.0f, 750.0f);
	const TArray<FVector2D> BackgroundViewSizes = {
		FVector2D(1920.0f, 1080.0f),
		FVector2D(2560.0f, 1080.0f),
		FVector2D(1280.0f, 900.0f),
	};
	for (const FVector2D& ViewSize : BackgroundViewSizes)
	{
		const FSlateRect BackgroundArea = FSGSTableLayoutMetrics::MakeBackgroundCoverRect(ViewSize, BackgroundImageSize);
		TestTrue(FString::Printf(TEXT("Background cover %.0fx%.0f covers viewport."), ViewSize.X, ViewSize.Y), BackgroundCoversView(BackgroundArea, ViewSize));
		TestTrue(FString::Printf(TEXT("Background cover %.0fx%.0f keeps source aspect."), ViewSize.X, ViewSize.Y), BackgroundKeepsAspect(BackgroundArea, BackgroundImageSize));
	}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0011M2TableUIStateStoreTest,
	"SGS.Plan0011M2.Table.StateStore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0011M2TableUIStateStoreTest::RunTest(const FString& Parameters)
{
	FSGSTableUIStateStore Store(0);
	int32 NotificationCount = 0;
	FSGSUILifetimeScope StoreTestLifetime(TEXT("TableStateStoreTest"));
	Store.GetSnapshotStateValue().Subscribe(
		StoreTestLifetime,
		[&NotificationCount](const FSGSTableViewSnapshot&)
		{
			++NotificationCount;
		},
		false);
	Store.GetInteractionStateValue().Subscribe(
		StoreTestLifetime,
		[&NotificationCount](const FSGSTableUIInteractionState&)
		{
			++NotificationCount;
		},
		false);

	FSGSTableViewSnapshot InitialSnapshot;
	InitialSnapshot.PublicRevision = 1;
	InitialSnapshot.PrivateRevision = 1;
	InitialSnapshot.ViewerSeat = 0;
	InitialSnapshot.Prompt.bHasPrompt = true;
	InitialSnapshot.Prompt.SelectableCardIds.Add(11);
	InitialSnapshot.Prompt.SelectableTargetSeatIndices.Add(1);
	InitialSnapshot.Prompt.SelectableTargetSeatIndices.Add(2);
	TArray<int32> InitialTargets;
	InitialTargets.Add(1);
	InitialTargets.Add(2);
	InitialSnapshot.Prompt.SetTargetSeatIndicesForCard(11, InitialTargets);

	TestTrue(TEXT("Initial local snapshot is accepted."), Store.IngestSnapshot(InitialSnapshot));
	TestTrue(TEXT("Accepted snapshot notifies its scope."), NotificationCount == 1);
	TestEqual(TEXT("Store keeps the local-player viewer seat."), Store.GetViewerSeat(), 0);
	TestTrue(TEXT("Store exposes the accepted snapshot."), Store.HasSnapshot() && Store.GetSnapshot().PrivateRevision == 1);

	TestTrue(TEXT("Selectable card enters local interaction state."), Store.SelectCard(11));
	TestEqual(TEXT("Selecting a card stores its stable card id."), Store.GetInteractionState().SelectedCardId, 11);
	TestEqual(TEXT("Multiple targets do not select a target implicitly."), Store.GetInteractionState().SelectedTargetSeat, INDEX_NONE);
	TestTrue(TEXT("Legal target enters local interaction state."), Store.SelectTarget(1));
	TestEqual(TEXT("Store keeps the selected target seat."), Store.GetInteractionState().SelectedTargetSeat, 1);
	TestFalse(TEXT("Illegal target is rejected by the store."), Store.SelectTarget(7));

	FSGSTableViewSnapshot DuplicateSnapshot = InitialSnapshot;
	TestFalse(TEXT("Duplicate revisions are ignored."), Store.IngestSnapshot(DuplicateSnapshot));
	TestEqual(TEXT("Duplicate revisions do not notify subscribers."), NotificationCount, 3);

	FSGSTableViewSnapshot RegressedSnapshot = InitialSnapshot;
	RegressedSnapshot.PublicRevision = 0;
	RegressedSnapshot.PrivateRevision = 2;
	TestFalse(TEXT("Mixed snapshots with a regressed public revision are rejected."), Store.IngestSnapshot(RegressedSnapshot));
	TestEqual(TEXT("Rejected revisions do not notify subscribers."), NotificationCount, 3);

	FSGSTableViewSnapshot NextSnapshot = InitialSnapshot;
	NextSnapshot.PrivateRevision = 2;
	NextSnapshot.Prompt.SelectableCardIds.Reset();
	NextSnapshot.Prompt.TargetSeatOptions.Reset();
	TestTrue(TEXT("A newer private revision is accepted."), Store.IngestSnapshot(NextSnapshot));
	TestEqual(TEXT("New snapshots clear selections that are no longer legal."), Store.GetInteractionState().SelectedCardId, INDEX_NONE);
	TestEqual(TEXT("New snapshots clear stale target selections."), Store.GetInteractionState().SelectedTargetSeat, INDEX_NONE);

	FSGSTableViewSnapshot WrongViewerSnapshot = NextSnapshot;
	WrongViewerSnapshot.PrivateRevision = 3;
	WrongViewerSnapshot.ViewerSeat = 1;
	TestFalse(TEXT("A snapshot for another local player is rejected."), Store.IngestSnapshot(WrongViewerSnapshot));
	TestEqual(TEXT("Cross-player snapshots do not notify this scope."), NotificationCount, 5);

	Store.ClearSelection();
	TestEqual(TEXT("Clearing an already empty selection is idempotent."), NotificationCount, 5);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
