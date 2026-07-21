#include "Misc/AutomationTest.h"

#include "Client/UI/Bridge/SGSLocalHumanDecisionAgent.h"
#include "Client/UI/Features/Table/State/SGSTableUIStateStore.h"
#include "Client/UI/Layout/SGSTableLayout.h"
#include "Server/AI/SGSScriptedDecisionAgent.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Engine/SGSGameDriver.h"
#include "Server/UI/SGSTableSnapshotBuilder.h"
#include "Shared/Cards/SGSCard.h"
#include "Shared/Cards/SGSDeckTypes.h"
#include "Shared/Core/SGSGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FSGSDeckCardSpec MakeMotionCard(FName Name, FGameplayTag Suit, int32 Number)
{
	FSGSDeckCardSpec Spec;
	Spec.CardName = Name;
	Spec.Suit = Suit;
	Spec.Number = Number;
	Spec.CardType = SGSGameplayTags::CardType_Basic.GetTag();
	return Spec;
}

TScriptInterface<ISGSDecisionAgent> MakeLocalMotionAgent(USGSLocalHumanDecisionAgent*& OutAgent)
{
	OutAgent = NewObject<USGSLocalHumanDecisionAgent>();
	TScriptInterface<ISGSDecisionAgent> Ref;
	Ref.SetObject(OutAgent);
	Ref.SetInterface(Cast<ISGSDecisionAgent>(OutAgent));
	return Ref;
}

TScriptInterface<ISGSDecisionAgent> MakeScriptedMotionAgent(USGSScriptedDecisionAgent*& OutAgent)
{
	OutAgent = NewObject<USGSScriptedDecisionAgent>();
	TScriptInterface<ISGSDecisionAgent> Ref;
	Ref.SetObject(OutAgent);
	Ref.SetInterface(Cast<ISGSDecisionAgent>(OutAgent));
	return Ref;
}

TArray<FSGSDeckCardSpec> MakeMotionDeck(bool bLocalDodge)
{
	TArray<FSGSDeckCardSpec> Deck;
	Deck.Add(MakeMotionCard(bLocalDodge ? TEXT("Dodge") : TEXT("Slash"), SGSGameplayTags::Suit_Spade.GetTag(), 7));
	Deck.Add(MakeMotionCard(TEXT("Peach"), SGSGameplayTags::Suit_Heart.GetTag(), 3));
	Deck.Add(MakeMotionCard(TEXT("Analeptic"), SGSGameplayTags::Suit_Diamond.GetTag(), 9));
	Deck.Add(MakeMotionCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), 10));
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Deck.Add(MakeMotionCard(TEXT("Slash"), SGSGameplayTags::Suit_Club.GetTag(), (Index % 13) + 1));
	}
	return Deck;
}

USGSGameDriver* StartMotionGame(
	USGSLocalHumanDecisionAgent*& OutLocal,
	USGSScriptedDecisionAgent*& OutOpponent,
	bool bLocalDodge,
	int32 OpponentHealth = 4,
	bool bOpponentUsesSlash = false)
{
	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	Agents.Add(MakeLocalMotionAgent(OutLocal));
	Agents.Add(MakeScriptedMotionAgent(OutOpponent));
	if (bOpponentUsesSlash)
	{
		OutOpponent->EnqueuePlayCard(TEXT("Slash"), 0);
	}
	FSGSGameStartConfig Config;
	Config.RandomSeed = 1105;
	Config.InitialDeck = MakeMotionDeck(bLocalDodge);
	Config.bShuffleInitialDeck = false;
	Config.StartingHandSize = 4;
	Config.MaxTurns = bOpponentUsesSlash ? 2 : 1;
	Config.InitialHealthBySeat.Add(1, OpponentHealth);
	USGSGameDriver* Driver = NewObject<USGSGameDriver>();
	Driver->StartGame(Agents, Config);
	return Driver;
}

const FSGSCardMoveEventPayload* FindMove(
	const USGSGameDriver& Driver,
	FName Reason,
	int32 FromSeat = INDEX_NONE)
{
	for (const FSGSEventLogEntry& Entry : Driver.GetReplayLog().GetEventLog())
	{
		if (Entry.CardMove.IsSet()
			&& Entry.CardMove->Reason == Reason
			&& (FromSeat == INDEX_NONE || Entry.CardMove->FromSeat == FromSeat))
		{
			return &Entry.CardMove.GetValue();
		}
	}
	return nullptr;
}

int32 FindSlashCard(const USGSLocalHumanDecisionAgent* Agent)
{
	const FSGSPlayPhaseRequest* Request = Agent != nullptr ? Agent->GetPendingPlayRequest() : nullptr;
	if (Request == nullptr)
	{
		return INDEX_NONE;
	}
	for (const FSGSCardActionOption& Option : Request->Options)
	{
		if (Option.CardName == TEXT("Slash") && Option.TargetSeatIndices.Contains(1))
		{
			return Option.CardId;
		}
	}
	return INDEX_NONE;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0011M5AuthorityPrivacyTest,
	"SGS.Plan0011M5.Motion.AuthorityPrivacy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0011M5AuthorityPrivacyTest::RunTest(const FString& Parameters)
{
	USGSLocalHumanDecisionAgent* Local = nullptr;
	USGSScriptedDecisionAgent* Opponent = nullptr;
	USGSGameDriver* Driver = StartMotionGame(Local, Opponent, false, 1);
	const FSGSTablePublicSnapshot PublicBeforePlay = FSGSTableSnapshotBuilder::BuildPublicSnapshot(Driver);
	const FSGSPlayerPrivateSnapshot PrivateBeforePlay = FSGSTableSnapshotBuilder::BuildPrivateSnapshot(Driver, Local, 0);
	const FSGSTableViewSnapshot LocalView = SGSComposeTableViewSnapshot(PublicBeforePlay, PrivateBeforePlay);

	TestTrue(TEXT("Opening deal produces an InitialDeal motion."), FindMove(*Driver, SGSCardMoveReasons::InitialDeal()) != nullptr);
	TestTrue(TEXT("Turn draw produces a Draw motion."), FindMove(*Driver, SGSCardMoveReasons::Draw()) != nullptr);
	TestTrue(TEXT("Public opening and draw cues carry counts."), PublicBeforePlay.CardMotionCues.ContainsByPredicate(
		[](const FSGSTableCardMotionCueViewData& Cue)
		{
			return (Cue.Reason == SGSCardMoveReasons::InitialDeal() || Cue.Reason == SGSCardMoveReasons::Draw())
				&& Cue.CardCount > 0;
		}));
	for (const FSGSTableCardMotionCueViewData& Cue : PublicBeforePlay.CardMotionCues)
	{
		if (Cue.Reason == SGSCardMoveReasons::InitialDeal() || Cue.Reason == SGSCardMoveReasons::Draw())
		{
			TestTrue(TEXT("Public draw cue never exposes card identities."), Cue.VisibleCards.IsEmpty());
		}
	}
	TestTrue(TEXT("Private overlay restores local draw faces."), LocalView.CardMotionCues.ContainsByPredicate(
		[](const FSGSTableCardMotionCueViewData& Cue)
		{
			return Cue.ToSeat == 0 && !Cue.VisibleCards.IsEmpty();
		}));
	TestFalse(TEXT("Private overlay does not reveal opponent opening cards."), LocalView.CardMotionCues.ContainsByPredicate(
		[](const FSGSTableCardMotionCueViewData& Cue)
		{
			return Cue.ToSeat == 1 && !Cue.VisibleCards.IsEmpty();
		}));

	const int32 SlashId = FindSlashCard(Local);
	TestTrue(TEXT("Motion test has a legal Slash."), SlashId != INDEX_NONE);
	TestTrue(TEXT("Slash submission succeeds."), Local->SubmitUseCard(SlashId, 1));
	if (Local->HasPendingResponseRequest())
	{
		TestTrue(TEXT("Local player may decline rescuing the dying opponent."), Local->SubmitPass());
	}
	const FSGSCardMoveEventPayload* UseMove = FindMove(*Driver, SGSCardMoveReasons::Use(), 0);
	TestTrue(TEXT("Slash produces a public use motion."), UseMove != nullptr);
	if (UseMove != nullptr)
	{
		TestTrue(TEXT("Use motion keeps its target."), UseMove->RelatedTargetSeatIndices.Contains(1));
	}
	TestTrue(TEXT("Processing cleanup is separately marked."), FindMove(*Driver, SGSCardMoveReasons::Cleanup()) != nullptr);
	TestTrue(TEXT("Death cleanup emits visible discard motion."), FindMove(*Driver, SGSCardMoveReasons::Discard(), 1) != nullptr);

	const FSGSTablePublicSnapshot PublicAfterPlay = FSGSTableSnapshotBuilder::BuildPublicSnapshot(Driver);
	TestTrue(TEXT("Public use/discard cues expose card faces."), PublicAfterPlay.CardMotionCues.ContainsByPredicate(
		[](const FSGSTableCardMotionCueViewData& Cue)
		{
			return (Cue.Reason == SGSCardMoveReasons::Use() || Cue.Reason == SGSCardMoveReasons::Discard())
				&& Cue.VisibleCards.Num() == Cue.CardCount;
		}));
	TestTrue(TEXT("Motion replay keeps invariants."), Driver->GetReplayLog().CheckInvariants());

	USGSLocalHumanDecisionAgent* ResponseLocal = nullptr;
	USGSScriptedDecisionAgent* ResponseOpponent = nullptr;
	USGSGameDriver* ResponseDriver = StartMotionGame(ResponseLocal, ResponseOpponent, true, 4, true);
	TestTrue(TEXT("Local player can finish play phase before response scenario."), ResponseLocal->SubmitPass());
	const FSGSResponseRequest* ResponseRequest = ResponseLocal->GetPendingResponseRequest();
	TestTrue(TEXT("Opponent Slash opens a local response."), ResponseRequest != nullptr);
	int32 DodgeId = INDEX_NONE;
	if (ResponseRequest != nullptr)
	{
		for (const int32 CandidateId : ResponseRequest->ResponseCardIds)
		{
			if (const USGSCard* Card = ResponseDriver->GetContext()->FindCardById(CandidateId);
				Card != nullptr && Card->CardName == TEXT("Dodge"))
			{
				DodgeId = CandidateId;
				break;
			}
		}
	}
	TestTrue(TEXT("Response scenario exposes Dodge."), DodgeId != INDEX_NONE);
	TestTrue(TEXT("Dodge response succeeds."), ResponseLocal->SubmitResponseCard(DodgeId));
	TestTrue(TEXT("Dodge produces a Respond motion."), FindMove(*ResponseDriver, SGSCardMoveReasons::Respond(), 0) != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0011M5QueueTest,
	"SGS.Plan0011M5.Motion.Queue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0011M5QueueTest::RunTest(const FString& Parameters)
{
	FSGSTableUIStateStore Store(0, INDEX_NONE);
	FSGSTableViewSnapshot Snapshot;
	Snapshot.PublicRevision = 1;
	Snapshot.PrivateRevision = 1;
	Snapshot.ViewerSeat = 0;
	Snapshot.MotionEpoch = 1;
	Snapshot.MotionWindowStartSequence = 0;
	Snapshot.MotionLatestSequence = 2;
	for (int32 Sequence : { 2, 0, 1 })
	{
		FSGSTableCardMotionCueViewData Cue;
		Cue.Sequence = Sequence;
		Cue.Reason = SGSCardMoveReasons::Draw();
		Cue.CardCount = 1;
		Snapshot.CardMotionCues.Add(Cue);
	}
	TestTrue(TEXT("Initial motion snapshot is accepted."), Store.IngestSnapshot(Snapshot));
	TestEqual(TEXT("Out-of-order cues are queued in authority order."), Store.GetMotionPresentation().PendingCues[0].Sequence, 0);
	TestEqual(TEXT("All unique cues are queued once."), Store.GetMotionPresentation().PendingCues.Num(), 3);
	Store.AcknowledgeMotionCue(0);
	TestEqual(TEXT("Acknowledgement advances the cursor."), Store.GetMotionPresentation().LastConsumedSequence, 0);

	FSGSTableViewSnapshot DuplicateEvents = Snapshot;
	DuplicateEvents.PublicRevision = 2;
	TestTrue(TEXT("New snapshot revision with duplicate cues is accepted."), Store.IngestSnapshot(DuplicateEvents));
	TestEqual(TEXT("Duplicate cues are not queued again."), Store.GetMotionPresentation().PendingCues.Num(), 2);

	FSGSTableViewSnapshot NewEpoch = Snapshot;
	NewEpoch.PublicRevision = 3;
	NewEpoch.MotionEpoch = 2;
	NewEpoch.CardMotionCues.SetNum(1);
	NewEpoch.CardMotionCues[0].Sequence = 0;
	NewEpoch.MotionLatestSequence = 0;
	TestTrue(TEXT("New match epoch is accepted."), Store.IngestSnapshot(NewEpoch));
	TestEqual(TEXT("New epoch clears old cues."), Store.GetMotionPresentation().PendingCues.Num(), 1);
	TestEqual(TEXT("New epoch restarts its motion cursor."), Store.GetMotionPresentation().PendingCues[0].Sequence, 0);

	FSGSTableUIStateStore GapStore(0, INDEX_NONE);
	FSGSTableViewSnapshot GapSnapshot;
	GapSnapshot.PublicRevision = 1;
	GapSnapshot.PrivateRevision = 1;
	GapSnapshot.ViewerSeat = 0;
	GapSnapshot.MotionEpoch = 3;
	GapSnapshot.MotionWindowStartSequence = 5;
	GapSnapshot.MotionLatestSequence = 6;
	TestTrue(TEXT("Gap snapshot is accepted."), GapStore.IngestSnapshot(GapSnapshot));
	TestTrue(TEXT("Window gap fast-forwards without stale animations."), GapStore.GetMotionPresentation().PendingCues.IsEmpty());
	TestEqual(TEXT("Gap fast-forward lands at authority latest."), GapStore.GetMotionPresentation().LastConsumedSequence, 6);

	FSGSTableUIStateStore OverflowStore(0, INDEX_NONE);
	FSGSTableViewSnapshot OverflowSnapshot;
	OverflowSnapshot.PublicRevision = 1;
	OverflowSnapshot.PrivateRevision = 1;
	OverflowSnapshot.ViewerSeat = 0;
	OverflowSnapshot.MotionEpoch = 4;
	OverflowSnapshot.MotionWindowStartSequence = 0;
	OverflowSnapshot.MotionLatestSequence = 32;
	for (int32 Sequence = 0; Sequence <= 32; ++Sequence)
	{
		FSGSTableCardMotionCueViewData Cue;
		Cue.Sequence = Sequence;
		OverflowSnapshot.CardMotionCues.Add(Cue);
	}
	TestTrue(TEXT("Overflow snapshot is accepted."), OverflowStore.IngestSnapshot(OverflowSnapshot));
	TestTrue(TEXT("Queue overflow clears transient motion."), OverflowStore.GetMotionPresentation().PendingCues.IsEmpty());
	TestEqual(TEXT("Queue overflow fast-forwards to latest."), OverflowStore.GetMotionPresentation().LastConsumedSequence, 32);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSGSPlan0011M5LayoutTest,
	"SGS.Plan0011M5.Motion.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSGSPlan0011M5LayoutTest::RunTest(const FString& Parameters)
{
	for (const FVector2D ViewSize : {
		FVector2D(1920.0f, 1080.0f),
		FVector2D(1600.0f, 900.0f),
		FVector2D(1280.0f, 720.0f),
		FVector2D(960.0f, 540.0f) })
	{
		const FSGSTableLayoutMetrics Layout = FSGSTableLayoutMetrics::Make(ViewSize, 8, 0);
		TestFalse(TEXT("Draw pile does not overlap hand."), FSGSTableLayoutMetrics::RectsOverlap(Layout.DrawPileArea, Layout.HandArea));
		TestFalse(TEXT("Discard pile does not overlap controls."), FSGSTableLayoutMetrics::RectsOverlap(Layout.DiscardPileArea, Layout.ControlArea));
		TestFalse(TEXT("Play area does not overlap controls."), FSGSTableLayoutMetrics::RectsOverlap(Layout.PlayArea, Layout.ControlArea));
		for (const FSGSTableSeatLayout& Seat : Layout.Seats)
		{
			if (Seat.RelativePosition != 0)
			{
				TestFalse(TEXT("Draw pile does not overlap opponent seats."), FSGSTableLayoutMetrics::RectsOverlap(Layout.DrawPileArea, Layout.GetSeatRect(Seat)));
				TestFalse(TEXT("Discard pile does not overlap opponent seats."), FSGSTableLayoutMetrics::RectsOverlap(Layout.DiscardPileArea, Layout.GetSeatRect(Seat)));
			}
		}
	}
	return true;
}

#endif
