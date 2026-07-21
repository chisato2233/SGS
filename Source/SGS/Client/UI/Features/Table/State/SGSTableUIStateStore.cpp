#include "Client/UI/Features/Table/State/SGSTableUIStateStore.h"

namespace
{
bool SameSnapshotRevision(const FSGSTableViewSnapshot& A, const FSGSTableViewSnapshot& B)
{
	return A.PublicRevision == B.PublicRevision
		&& A.PrivateRevision == B.PrivateRevision
		&& A.ViewerSeat == B.ViewerSeat;
}

bool SameInteraction(const FSGSTableUIInteractionState& A, const FSGSTableUIInteractionState& B)
{
	return A.SelectedCardId == B.SelectedCardId
		&& A.SelectedCardIds == B.SelectedCardIds
		&& A.SelectedTargetSeat == B.SelectedTargetSeat
		&& A.SelectedTargetSeatIndices == B.SelectedTargetSeatIndices
		&& A.SelectedSkillName == B.SelectedSkillName;
}

bool SameHandPresentation(
	const FSGSTableHandPresentationState& A,
	const FSGSTableHandPresentationState& B)
{
	return A.OrderedCardIds == B.OrderedCardIds;
}

bool SameMotionPresentation(
	const FSGSTableMotionPresentationState& A,
	const FSGSTableMotionPresentationState& B)
{
	if (A.MotionEpoch != B.MotionEpoch
		|| A.LastConsumedSequence != B.LastConsumedSequence
		|| A.LastQueuedSequence != B.LastQueuedSequence
		|| A.PendingCues.Num() != B.PendingCues.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < A.PendingCues.Num(); ++Index)
	{
		if (A.PendingCues[Index].Sequence != B.PendingCues[Index].Sequence
			|| A.PendingCues[Index].VisibleCards.Num() != B.PendingCues[Index].VisibleCards.Num())
		{
			return false;
		}
	}
	return true;
}

bool HasValidHandCardIds(const FSGSTableViewSnapshot& Snapshot)
{
	TSet<int32> CardIds;
	CardIds.Reserve(Snapshot.HandCards.Num());
	for (const FSGSCardViewData& Card : Snapshot.HandCards)
	{
		if (Card.CardId == INDEX_NONE || CardIds.Contains(Card.CardId))
		{
			return false;
		}
		CardIds.Add(Card.CardId);
	}
	return true;
}

bool RejectHandReorder(const TCHAR* Reason)
{
	ensureMsgf(false, TEXT("Rejected local hand reorder: %s"), Reason);
	return false;
}
}

FSGSTableUIStateStore::FSGSTableUIStateStore(int32 InViewerSeat, int32 InInitialMotionSequence)
	: ViewerSeat(InViewerSeat)
	, InitialMotionSequence(InInitialMotionSequence)
	, SnapshotState(FSGSTableViewSnapshot(), &SameSnapshotRevision)
	, InteractionState(FSGSTableUIInteractionState(), &SameInteraction)
	, HandPresentationState(FSGSTableHandPresentationState(), &SameHandPresentation)
	, MotionPresentationState(FSGSTableMotionPresentationState(), &SameMotionPresentation)
	, PublicRevisionSelector(
		SnapshotState,
		[](const FSGSTableViewSnapshot& Snapshot) { return Snapshot.PublicRevision; },
		[](int32 A, int32 B) { return A == B; })
	, PrivateRevisionSelector(
		SnapshotState,
		[](const FSGSTableViewSnapshot& Snapshot) { return Snapshot.PrivateRevision; },
		[](int32 A, int32 B) { return A == B; })
{
}

bool FSGSTableUIStateStore::IngestSnapshot(const FSGSTableViewSnapshot& InSnapshot)
{
	if (!IsRevisionAccepted(InSnapshot))
	{
		return false;
	}
	if (!HasValidHandCardIds(InSnapshot))
	{
		ensureMsgf(false, TEXT("Rejected table snapshot with invalid or duplicate hand CardIds."));
		return false;
	}

	FSGSTableViewSnapshot AcceptedSnapshot = InSnapshot;
	if (ViewerSeat != INDEX_NONE)
	{
		AcceptedSnapshot.ViewerSeat = ViewerSeat;
	}

	FSGSUIStateBatch Batch;
	bHasSnapshot = true;
	ReconcileMotionPresentation(AcceptedSnapshot);
	SnapshotState.Set(MoveTemp(AcceptedSnapshot));
	ReconcileHandPresentation();
	NormalizeSelection();
	return true;
}

void FSGSTableUIStateStore::AcknowledgeMotionCue(int32 Sequence)
{
	FSGSTableMotionPresentationState Next = MotionPresentationState.Get();
	if (Sequence <= Next.LastConsumedSequence)
	{
		return;
	}
	Next.LastConsumedSequence = Sequence;
	Next.PendingCues.RemoveAll([Sequence](const FSGSTableCardMotionCueViewData& Cue)
	{
		return Cue.Sequence <= Sequence;
	});
	MotionPresentationState.Set(MoveTemp(Next));
}

void FSGSTableUIStateStore::ClearMotionQueueToLatest()
{
	FSGSTableMotionPresentationState Next = MotionPresentationState.Get();
	Next.PendingCues.Reset();
	if (bHasSnapshot)
	{
		Next.LastConsumedSequence = FMath::Max(Next.LastConsumedSequence, GetSnapshot().MotionLatestSequence);
		Next.LastQueuedSequence = FMath::Max(Next.LastQueuedSequence, Next.LastConsumedSequence);
	}
	MotionPresentationState.Set(MoveTemp(Next));
}

bool FSGSTableUIStateStore::SelectCard(int32 CardId)
{
	if (!IsCardSelectable(CardId))
	{
		return false;
	}

	FSGSTableUIInteractionState Next = InteractionState.Get();
	if (const FSGSDecisionSkillViewData* Skill = GetSelectedSkillOption())
	{
		if (Skill->MaxCardCount > 1)
		{
			if (Next.SelectedCardIds.Contains(CardId))
			{
				Next.SelectedCardIds.Remove(CardId);
			}
			else if (Next.SelectedCardIds.Num() < Skill->MaxCardCount)
			{
				Next.SelectedCardIds.Add(CardId);
			}
		}
		else
		{
			Next.SelectedCardIds = { CardId };
		}
		Next.SelectedCardId = Next.SelectedCardIds.IsEmpty() ? INDEX_NONE : Next.SelectedCardIds.Last();
	}
	else if (GetSnapshot().Prompt.bIsCardChoice)
	{
		if (Next.SelectedCardIds.Contains(CardId))
		{
			Next.SelectedCardIds.Remove(CardId);
		}
		else if (Next.SelectedCardIds.Num() < GetSnapshot().Prompt.MaxChoiceCount)
		{
			Next.SelectedCardIds.Add(CardId);
		}
		Next.SelectedCardId = Next.SelectedCardIds.IsEmpty() ? INDEX_NONE : Next.SelectedCardIds.Last();
	}
	else
	{
		Next.SelectedCardId = CardId;
		Next.SelectedCardIds = { CardId };
	}
	const TArray<int32> Targets = GetTargetsForCard(CardId);
	const FSGSCardTargetViewData* TargetOption = GetSnapshot().Prompt.TargetSeatOptions.FindByPredicate(
		[CardId](const FSGSCardTargetViewData& Candidate) { return Candidate.CardId == CardId; });
	if (Targets.Num() == 1 && TargetOption != nullptr && TargetOption->MinTargetCount == 1)
	{
		Next.SelectedTargetSeat = Targets[0];
		Next.SelectedTargetSeatIndices = { Targets[0] };
	}
	else
	{
		Next.SelectedTargetSeat = INDEX_NONE;
		Next.SelectedTargetSeatIndices.Reset();
	}
	InteractionState.Set(MoveTemp(Next));
	return true;
}

bool FSGSTableUIStateStore::SelectTarget(int32 TargetSeatIndex)
{
	FSGSTableUIInteractionState Next = InteractionState.Get();
	const TArray<int32> Targets = GetTargetsForCurrentSelection();
	if (!Targets.Contains(TargetSeatIndex))
	{
		return false;
	}

	const int32 MaxTargets = GetMaxTargetCountForCurrentSelection();
	if (MaxTargets > 1)
	{
		if (Next.SelectedTargetSeatIndices.Contains(TargetSeatIndex))
		{
			Next.SelectedTargetSeatIndices.Remove(TargetSeatIndex);
		}
		else if (Next.SelectedTargetSeatIndices.Num() < MaxTargets)
		{
			Next.SelectedTargetSeatIndices.Add(TargetSeatIndex);
		}
	}
	else
	{
		Next.SelectedTargetSeatIndices = { TargetSeatIndex };
	}
	Next.SelectedTargetSeat = Next.SelectedTargetSeatIndices.IsEmpty()
		? INDEX_NONE
		: Next.SelectedTargetSeatIndices.Last();
	InteractionState.Set(MoveTemp(Next));
	return true;
}

bool FSGSTableUIStateStore::SelectSkill(FName SkillName)
{
	if (!bHasSnapshot || !GetSnapshot().Prompt.bHasPrompt)
	{
		return false;
	}
	const FSGSDecisionSkillViewData* Skill = GetSnapshot().Prompt.FindSkillOption(SkillName);
	if (Skill == nullptr)
	{
		return false;
	}

	FSGSTableUIInteractionState Next = InteractionState.Get();
	Next.SelectedSkillName = Next.SelectedSkillName == SkillName ? NAME_None : SkillName;
	Next.SelectedCardId = INDEX_NONE;
	Next.SelectedCardIds.Reset();
	Next.SelectedTargetSeat = INDEX_NONE;
	Next.SelectedTargetSeatIndices.Reset();
	FSGSUIStateBatch Batch;
	InteractionState.Set(MoveTemp(Next));
	NormalizeSelection();
	return true;
}

bool FSGSTableUIStateStore::IsCardSelectable(int32 CardId) const
{
	if (!bHasSnapshot || !GetSnapshot().Prompt.bHasPrompt)
	{
		return false;
	}
	if (const FSGSDecisionSkillViewData* Skill = GetSelectedSkillOption())
	{
		return Skill->bRequiresCard && Skill->SelectableCardIds.Contains(CardId);
	}
	return GetSnapshot().Prompt.SelectableCardIds.Contains(CardId);
}

bool FSGSTableUIStateStore::IsTargetSelectable(int32 TargetSeatIndex) const
{
	return GetTargetsForCurrentSelection().Contains(TargetSeatIndex);
}

bool FSGSTableUIStateStore::IsSelectionComplete() const
{
	if (!bHasSnapshot || !GetSnapshot().Prompt.bHasPrompt)
	{
		return false;
	}
	const FSGSTableUIInteractionState& Interaction = GetInteractionState();
	if (const FSGSDecisionSkillViewData* Skill = GetSelectedSkillOption())
	{
		if (Interaction.SelectedCardIds.Num() < Skill->MinCardCount
			|| Interaction.SelectedCardIds.Num() > Skill->MaxCardCount)
		{
			return false;
		}
		for (const int32 CardId : Interaction.SelectedCardIds)
		{
			if (!Skill->SelectableCardIds.Contains(CardId))
			{
				return false;
			}
		}
	}
	else if (GetSnapshot().Prompt.bIsCardChoice)
	{
		if (Interaction.SelectedCardIds.Num() < GetSnapshot().Prompt.MinChoiceCount
			|| Interaction.SelectedCardIds.Num() > GetSnapshot().Prompt.MaxChoiceCount)
		{
			return false;
		}
		for (const int32 CardId : Interaction.SelectedCardIds)
		{
			if (!GetSnapshot().Prompt.SelectableCardIds.Contains(CardId))
			{
				return false;
			}
		}
	}
	else if (!GetSnapshot().Prompt.SelectableCardIds.Contains(Interaction.SelectedCardId))
	{
		return false;
	}

	const TArray<int32> Targets = GetTargetsForCurrentSelection();
	const int32 MinTargets = GetMinTargetCountForCurrentSelection();
	const int32 MaxTargets = GetMaxTargetCountForCurrentSelection();
	if (Interaction.SelectedTargetSeatIndices.Num() < MinTargets
		|| Interaction.SelectedTargetSeatIndices.Num() > MaxTargets)
	{
		return false;
	}
	for (const int32 TargetSeat : Interaction.SelectedTargetSeatIndices)
	{
		if (!Targets.Contains(TargetSeat))
		{
			return false;
		}
	}
	return true;
}

bool FSGSTableUIStateStore::ReorderHand(TConstArrayView<int32> OrderedCardIds)
{
	if (!bHasSnapshot || OrderedCardIds.Num() != GetSnapshot().HandCards.Num())
	{
		return RejectHandReorder(TEXT("the ordered set does not match the current hand size"));
	}

	TSet<int32> RemainingCardIds;
	RemainingCardIds.Reserve(GetSnapshot().HandCards.Num());
	for (const FSGSCardViewData& Card : GetSnapshot().HandCards)
	{
		if (Card.CardId == INDEX_NONE || RemainingCardIds.Contains(Card.CardId))
		{
			return RejectHandReorder(TEXT("the current snapshot contains invalid CardIds"));
		}
		RemainingCardIds.Add(Card.CardId);
	}

	FSGSTableHandPresentationState Next;
	Next.OrderedCardIds.Reserve(OrderedCardIds.Num());
	for (const int32 CardId : OrderedCardIds)
	{
		if (!RemainingCardIds.Remove(CardId))
		{
			return RejectHandReorder(TEXT("the order contains a duplicate or unknown CardId"));
		}
		Next.OrderedCardIds.Add(CardId);
	}

	if (!RemainingCardIds.IsEmpty())
	{
		return RejectHandReorder(TEXT("the order omits one or more current cards"));
	}

	HandPresentationState.Set(MoveTemp(Next));
	return true;
}

void FSGSTableUIStateStore::ClearSelection()
{
	InteractionState.Set(FSGSTableUIInteractionState());
}

bool FSGSTableUIStateStore::IsRevisionAccepted(const FSGSTableViewSnapshot& InSnapshot) const
{
	if (ViewerSeat != INDEX_NONE
		&& InSnapshot.ViewerSeat != INDEX_NONE
		&& InSnapshot.ViewerSeat != ViewerSeat)
	{
		return false;
	}

	if (!bHasSnapshot)
	{
		return true;
	}

	const FSGSTableViewSnapshot& Current = GetSnapshot();
	if (InSnapshot.PublicRevision < Current.PublicRevision
		|| InSnapshot.PrivateRevision < Current.PrivateRevision)
	{
		return false;
	}

	return InSnapshot.PublicRevision > Current.PublicRevision
		|| InSnapshot.PrivateRevision > Current.PrivateRevision;
}

TArray<int32> FSGSTableUIStateStore::GetTargetsForCard(int32 CardId) const
{
	if (const FSGSDecisionSkillViewData* Skill = GetSelectedSkillOption())
	{
		return Skill->SelectableTargetSeatIndices;
	}
	if (const TArray<int32>* Targets = GetSnapshot().Prompt.FindTargetSeatIndicesForCard(CardId))
	{
		return *Targets;
	}
	return TArray<int32>();
}

const FSGSDecisionSkillViewData* FSGSTableUIStateStore::GetSelectedSkillOption() const
{
	const FName SkillName = InteractionState.Get().SelectedSkillName;
	return bHasSnapshot && !SkillName.IsNone()
		? GetSnapshot().Prompt.FindSkillOption(SkillName)
		: nullptr;
}

TArray<int32> FSGSTableUIStateStore::GetTargetsForCurrentSelection() const
{
	return GetTargetsForCard(InteractionState.Get().SelectedCardId);
}

int32 FSGSTableUIStateStore::GetMinTargetCountForCurrentSelection() const
{
	if (const FSGSDecisionSkillViewData* Skill = GetSelectedSkillOption())
	{
		return Skill->MinTargetCount;
	}
	const int32 CardId = InteractionState.Get().SelectedCardId;
	const FSGSCardTargetViewData* Option = GetSnapshot().Prompt.TargetSeatOptions.FindByPredicate(
		[CardId](const FSGSCardTargetViewData& Candidate) { return Candidate.CardId == CardId; });
	return Option != nullptr ? Option->MinTargetCount : 0;
}

int32 FSGSTableUIStateStore::GetMaxTargetCountForCurrentSelection() const
{
	if (const FSGSDecisionSkillViewData* Skill = GetSelectedSkillOption())
	{
		return Skill->MaxTargetCount;
	}
	const int32 CardId = InteractionState.Get().SelectedCardId;
	const FSGSCardTargetViewData* Option = GetSnapshot().Prompt.TargetSeatOptions.FindByPredicate(
		[CardId](const FSGSCardTargetViewData& Candidate) { return Candidate.CardId == CardId; });
	return Option != nullptr ? Option->MaxTargetCount : 0;
}

void FSGSTableUIStateStore::ReconcileHandPresentation()
{
	TSet<int32> SnapshotCardIds;
	SnapshotCardIds.Reserve(GetSnapshot().HandCards.Num());
	for (const FSGSCardViewData& Card : GetSnapshot().HandCards)
	{
		if (Card.CardId != INDEX_NONE)
		{
			SnapshotCardIds.Add(Card.CardId);
		}
	}

	FSGSTableHandPresentationState Next;
	Next.OrderedCardIds.Reserve(SnapshotCardIds.Num());
	TSet<int32> AddedCardIds;
	AddedCardIds.Reserve(SnapshotCardIds.Num());
	for (const int32 CardId : HandPresentationState.Get().OrderedCardIds)
	{
		if (SnapshotCardIds.Contains(CardId) && !AddedCardIds.Contains(CardId))
		{
			Next.OrderedCardIds.Add(CardId);
			AddedCardIds.Add(CardId);
		}
	}

	for (const FSGSCardViewData& Card : GetSnapshot().HandCards)
	{
		if (Card.CardId != INDEX_NONE && !AddedCardIds.Contains(Card.CardId))
		{
			Next.OrderedCardIds.Add(Card.CardId);
			AddedCardIds.Add(Card.CardId);
		}
	}

	HandPresentationState.Set(MoveTemp(Next));
}

void FSGSTableUIStateStore::NormalizeSelection()
{
	const FSGSTableViewSnapshot& Snapshot = GetSnapshot();
	FSGSTableUIInteractionState Next = InteractionState.Get();
	if (!Snapshot.Prompt.bHasPrompt)
	{
		InteractionState.Set(FSGSTableUIInteractionState());
		return;
	}
	const FSGSDecisionSkillViewData* Skill = Next.SelectedSkillName.IsNone()
		? nullptr
		: Snapshot.Prompt.FindSkillOption(Next.SelectedSkillName);
	if (!Next.SelectedSkillName.IsNone() && Skill == nullptr)
	{
		Next = FSGSTableUIInteractionState();
	}
	else if (Skill != nullptr)
	{
		Next.SelectedCardIds.RemoveAll(
			[Skill](int32 CardId)
			{
				return !Skill->SelectableCardIds.Contains(CardId);
			});
		if (Next.SelectedCardIds.Num() > Skill->MaxCardCount)
		{
			Next.SelectedCardIds.SetNum(Skill->MaxCardCount);
		}
		if (!Skill->bRequiresCard)
		{
			Next.SelectedCardIds.Reset();
		}
		Next.SelectedCardId = Next.SelectedCardIds.IsEmpty() ? INDEX_NONE : Next.SelectedCardIds.Last();
	}
	else if (Snapshot.Prompt.bIsCardChoice)
	{
		Next.SelectedCardIds.RemoveAll(
			[&Snapshot](int32 CardId)
			{
				return !Snapshot.Prompt.SelectableCardIds.Contains(CardId);
			});
		if (Next.SelectedCardIds.Num() > Snapshot.Prompt.MaxChoiceCount)
		{
			Next.SelectedCardIds.SetNum(Snapshot.Prompt.MaxChoiceCount);
		}
		Next.SelectedCardId = Next.SelectedCardIds.IsEmpty() ? INDEX_NONE : Next.SelectedCardIds.Last();
	}
	else if (!Snapshot.Prompt.SelectableCardIds.Contains(Next.SelectedCardId))
	{
		Next.SelectedCardId = INDEX_NONE;
		Next.SelectedCardIds.Reset();
	}

	TArray<int32> Targets;
	if (Skill != nullptr)
	{
		Targets = Skill->SelectableTargetSeatIndices;
	}
	else if (const TArray<int32>* CardTargets =
		Snapshot.Prompt.FindTargetSeatIndicesForCard(Next.SelectedCardId))
	{
		Targets = *CardTargets;
	}
	if (Targets.Num() == 1)
	{
		Next.SelectedTargetSeat = Targets[0];
		Next.SelectedTargetSeatIndices = { Targets[0] };
	}
	else
	{
		Next.SelectedTargetSeatIndices.RemoveAll(
			[&Targets](int32 SeatIndex) { return !Targets.Contains(SeatIndex); });
		if (Next.SelectedTargetSeatIndices.Num() > GetMaxTargetCountForCurrentSelection())
		{
			Next.SelectedTargetSeatIndices.SetNum(GetMaxTargetCountForCurrentSelection());
		}
		Next.SelectedTargetSeat = Next.SelectedTargetSeatIndices.IsEmpty()
			? INDEX_NONE
			: Next.SelectedTargetSeatIndices.Last();
	}
	InteractionState.Set(MoveTemp(Next));
}

void FSGSTableUIStateStore::ReconcileMotionPresentation(const FSGSTableViewSnapshot& InSnapshot)
{
	constexpr int32 MaxPendingMotionCues = 32;
	FSGSTableMotionPresentationState Next = MotionPresentationState.Get();
	if (Next.MotionEpoch != InSnapshot.MotionEpoch)
	{
		Next = FSGSTableMotionPresentationState();
		Next.MotionEpoch = InSnapshot.MotionEpoch;
		Next.LastConsumedSequence = InitialMotionSequence;
		Next.LastQueuedSequence = InitialMotionSequence;
	}

	const int32 Cursor = FMath::Max(Next.LastConsumedSequence, Next.LastQueuedSequence);
	if (InSnapshot.MotionLatestSequence >= 0
		&& Cursor < InSnapshot.MotionWindowStartSequence - 1)
	{
		Next.PendingCues.Reset();
		Next.LastConsumedSequence = InSnapshot.MotionLatestSequence;
		Next.LastQueuedSequence = InSnapshot.MotionLatestSequence;
		MotionPresentationState.Set(MoveTemp(Next));
		return;
	}

	TArray<FSGSTableCardMotionCueViewData> SortedCues = InSnapshot.CardMotionCues;
	SortedCues.Sort([](const FSGSTableCardMotionCueViewData& Left, const FSGSTableCardMotionCueViewData& Right)
	{
		return Left.Sequence < Right.Sequence;
	});
	for (const FSGSTableCardMotionCueViewData& Cue : SortedCues)
	{
		if (FSGSTableCardMotionCueViewData* Existing = Next.PendingCues.FindByPredicate(
			[Sequence = Cue.Sequence](const FSGSTableCardMotionCueViewData& Pending)
			{
				return Pending.Sequence == Sequence;
			}); Existing != nullptr)
		{
			if (Cue.VisibleCards.Num() > Existing->VisibleCards.Num())
			{
				*Existing = Cue;
			}
			continue;
		}
		if (Cue.Sequence > Next.LastQueuedSequence && Cue.Sequence > Next.LastConsumedSequence)
		{
			Next.PendingCues.Add(Cue);
			Next.LastQueuedSequence = Cue.Sequence;
		}
	}

	if (Next.PendingCues.Num() > MaxPendingMotionCues)
	{
		Next.PendingCues.Reset();
		Next.LastConsumedSequence = InSnapshot.MotionLatestSequence;
		Next.LastQueuedSequence = InSnapshot.MotionLatestSequence;
	}
	MotionPresentationState.Set(MoveTemp(Next));
}
