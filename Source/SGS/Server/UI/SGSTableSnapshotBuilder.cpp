#include "Server/UI/SGSTableSnapshotBuilder.h"

#include "Shared/Cards/SGSCard.h"
#include "Client/UI/Bridge/SGSLocalHumanDecisionAgent.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Engine/SGSGameDriver.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Players/SGSSeat.h"
#include "Shared/Decisions/SGSDecisionTypes.h"

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

FSGSCardViewData MakeCardView(const USGSCard& Card)
{
	FSGSCardViewData CardView;
	CardView.CardId = Card.CardId;
	CardView.CardName = Card.CardName;
	CardView.DisplayText = FormatCard(Card);
	CardView.Suit = Card.Suit;
	CardView.Number = Card.Number;
	return CardView;
}

bool IsPublicCardFaceReason(FName Reason)
{
	return Reason == SGSCardMoveReasons::Use()
		|| Reason == SGSCardMoveReasons::Respond()
		|| Reason == SGSCardMoveReasons::Discard();
}

FSGSTableCardMotionCueViewData MakeMotionCue(
	const FSGSCardMoveEventPayload& Move,
	const USGSGameContext& Context,
	bool bRevealFaces)
{
	FSGSTableCardMotionCueViewData Cue;
	Cue.Sequence = Move.Sequence;
	Cue.Reason = Move.Reason;
	Cue.FromZone = Move.FromZone;
	Cue.FromSeat = Move.FromSeat;
	Cue.ToZone = Move.ToZone;
	Cue.ToSeat = Move.ToSeat;
	Cue.CardCount = Move.CardIds.Num();
	Cue.RelatedTargetSeatIndices = Move.RelatedTargetSeatIndices;
	Cue.bCleanup = Move.Reason == SGSCardMoveReasons::Cleanup();
	if (bRevealFaces && !Cue.bCleanup)
	{
		for (const int32 CardId : Move.CardIds)
		{
			if (const USGSCard* Card = Context.FindCardById(CardId))
			{
				Cue.VisibleCards.Add(MakeCardView(*Card));
			}
		}
	}
	return Cue;
}

TArray<const FSGSCardMoveEventPayload*> GetRecentCardMoves(const USGSGameDriver& Driver)
{
	constexpr int32 MaxMotionEvents = 64;
	TArray<const FSGSCardMoveEventPayload*> Moves;
	for (const FSGSEventLogEntry& Entry : Driver.GetReplayLog().GetEventLog())
	{
		if (Entry.CardMove.IsSet())
		{
			Moves.Add(&Entry.CardMove.GetValue());
		}
	}
	if (Moves.Num() > MaxMotionEvents)
	{
		Moves.RemoveAt(0, Moves.Num() - MaxMotionEvents, EAllowShrinking::No);
	}
	return Moves;
}

void AddUniqueInts(TArray<int32>& Target, const TArray<int32>& Source)
{
	for (int32 Value : Source)
	{
		Target.AddUnique(Value);
	}
}

void ApplyPromptSnapshot(FSGSPlayerPrivateSnapshot& Snapshot, const USGSLocalHumanDecisionAgent* DecisionAgent)
{
	Snapshot.Prompt = FSGSDecisionPromptViewData();

	if (DecisionAgent == nullptr)
	{
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
			Snapshot.Prompt.SetTargetSeatIndicesForCard(Option.CardId, Option.TargetSeatIndices);
		}
	}
	else if (const FSGSResponseRequest* ResponseRequest = DecisionAgent->GetPendingResponseRequest())
	{
		Snapshot.Prompt.bHasPrompt = true;
		Snapshot.Prompt.bIsResponse = true;
		Snapshot.Prompt.WindowName = ResponseRequest->WindowName;
		Snapshot.Prompt.RequiredCardName = ResponseRequest->RequiredCardName;
		Snapshot.Prompt.AcceptedCardNames = ResponseRequest->AcceptedCardNames;
		Snapshot.Prompt.ContextName = ResponseRequest->ContextName;
		Snapshot.Prompt.EffectSourceSeat = ResponseRequest->EffectSourceSeat;
		Snapshot.Prompt.EffectTargetSeat = ResponseRequest->EffectTargetSeat;
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
			Snapshot.Prompt.SetTargetSeatIndicesForCard(CardId, ResponseTargets);
		}

		for (const FSGSDecisionSkillOption& SkillOption : ResponseRequest->SkillOptions)
		{
			if (SkillOption.SkillName.IsNone())
			{
				continue;
			}

			FSGSDecisionSkillViewData SkillView;
			SkillView.SkillName = SkillOption.SkillName;
			SkillView.DisplayName = SkillOption.DisplayName.IsNone()
				? SkillOption.SkillName.ToString()
				: SkillOption.DisplayName.ToString();
			SkillView.bRequiresCard = SkillOption.bRequiresCard;
			SkillView.SelectableCardIds = SkillOption.SelectableCardIds;
			SkillView.SelectableTargetSeatIndices = SkillOption.TargetSeatIndices;
			if (SkillView.SelectableTargetSeatIndices.IsEmpty()
				&& ResponseRequest->EffectTargetSeat != INDEX_NONE)
			{
				SkillView.SelectableTargetSeatIndices.Add(ResponseRequest->EffectTargetSeat);
			}
			AddUniqueInts(
				Snapshot.Prompt.SelectableTargetSeatIndices,
				SkillView.SelectableTargetSeatIndices);
			Snapshot.Prompt.SkillOptions.Add(MoveTemp(SkillView));
		}
	}
}
}

FSGSTablePublicSnapshot FSGSTableSnapshotBuilder::BuildPublicSnapshot(const USGSGameDriver* Driver)
{
	FSGSTablePublicSnapshot Snapshot;

	if (Driver == nullptr)
	{
		return Snapshot;
	}

	USGSGameContext* Context = Driver->GetContext();
	Snapshot.CurrentSeatIndex = Driver->GetTurnSeatIndex();
	Snapshot.CurrentPhase = Driver->GetCurrentPhase();
	Snapshot.bGameOver = Driver->IsGameOver();
	Snapshot.GameResult = Driver->GetGameResult();

	if (Context == nullptr)
	{
		return Snapshot;
	}

	Snapshot.DrawPileCount = Context->GetCardsInZone(SGSGameplayTags::CardZone_DrawPile.GetTag()).Num();
	Snapshot.DiscardPileCount = Context->GetCardsInZone(SGSGameplayTags::CardZone_DiscardPile.GetTag()).Num();
	const TArray<USGSCard*> DiscardPile = Context->GetCardsInZone(SGSGameplayTags::CardZone_DiscardPile.GetTag());
	if (!DiscardPile.IsEmpty() && DiscardPile.Last() != nullptr)
	{
		Snapshot.bHasDiscardTopCard = true;
		Snapshot.DiscardTopCard = MakeCardView(*DiscardPile.Last());
	}

	Snapshot.MotionEpoch = Driver->GetMotionEpoch();
	const TArray<const FSGSCardMoveEventPayload*> RecentMoves = GetRecentCardMoves(*Driver);
	if (!RecentMoves.IsEmpty())
	{
		Snapshot.MotionWindowStartSequence = RecentMoves[0]->Sequence;
		Snapshot.MotionLatestSequence = RecentMoves.Last()->Sequence;
		Snapshot.CardMotionCues.Reserve(RecentMoves.Num());
		for (const FSGSCardMoveEventPayload* Move : RecentMoves)
		{
			Snapshot.CardMotionCues.Add(MakeMotionCue(*Move, *Context, IsPublicCardFaceReason(Move->Reason)));
		}
	}

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
		SeatView.GeneralId = Seat->GeneralId;
		SeatView.Faction = Seat->Faction;
		SeatView.Health = Seat->Health;
		SeatView.MaxHealth = Seat->MaxHealth;
		SeatView.bIsAlive = Seat->bIsAlive;
		SeatView.bIsCurrent = !Snapshot.bGameOver && SeatIndex == Snapshot.CurrentSeatIndex;
		if (Seat->Identity == SGSGameplayTags::Identity_Lord.GetTag()
			|| !Seat->bIsAlive
			|| Snapshot.bGameOver)
		{
			SeatView.Identity = Seat->Identity;
		}
		SeatView.HandCount = Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), SeatIndex).Num();
		Snapshot.Seats.Add(MoveTemp(SeatView));
	}

	return Snapshot;
}

FSGSPlayerPrivateSnapshot FSGSTableSnapshotBuilder::BuildPrivateSnapshot(
	const USGSGameDriver* Driver,
	const USGSLocalHumanDecisionAgent* DecisionAgent,
	int32 ViewerSeat)
{
	FSGSPlayerPrivateSnapshot Snapshot;
	Snapshot.ViewerSeat = ViewerSeat;

	if (Driver == nullptr)
	{
		ApplyPromptSnapshot(Snapshot, DecisionAgent);
		return Snapshot;
	}

	USGSGameContext* Context = Driver->GetContext();
	if (Context == nullptr)
	{
		ApplyPromptSnapshot(Snapshot, DecisionAgent);
		return Snapshot;
	}

	Snapshot.MotionEpoch = Driver->GetMotionEpoch();
	for (const FSGSCardMoveEventPayload* Move : GetRecentCardMoves(*Driver))
	{
		const bool bViewerDraw = Move->ToZone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag())
			&& Move->ToSeat == ViewerSeat;
		if (bViewerDraw)
		{
			Snapshot.CardMotionCueOverrides.Add(MakeMotionCue(*Move, *Context, true));
		}
	}

	if (const USGSSeat* Viewer = Context->GetSeat(ViewerSeat))
	{
		Snapshot.ViewerIdentity = Viewer->Identity;
	}

	for (USGSCard* Card : Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), ViewerSeat))
	{
		if (Card == nullptr)
		{
			continue;
		}

		Snapshot.HandCards.Add(MakeCardView(*Card));
	}

	ApplyPromptSnapshot(Snapshot, DecisionAgent);
	return Snapshot;
}

FSGSTableViewSnapshot FSGSTableSnapshotBuilder::Build(const USGSGameDriver* Driver, int32 ViewerSeat)
{
	return SGSComposeTableViewSnapshot(
		BuildPublicSnapshot(Driver),
		BuildPrivateSnapshot(Driver, nullptr, ViewerSeat));
}

FSGSTableViewSnapshot FSGSTableSnapshotBuilder::Build(
	const USGSGameDriver* Driver,
	const USGSLocalHumanDecisionAgent* DecisionAgent,
	int32 ViewerSeat)
{
	return SGSComposeTableViewSnapshot(
		BuildPublicSnapshot(Driver),
		BuildPrivateSnapshot(Driver, DecisionAgent, ViewerSeat));
}
