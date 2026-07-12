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
		&& A.SelectedTargetSeat == B.SelectedTargetSeat
		&& A.SelectedSkillName == B.SelectedSkillName;
}

bool SameHandPresentation(
	const FSGSTableHandPresentationState& A,
	const FSGSTableHandPresentationState& B)
{
	return A.OrderedCardIds == B.OrderedCardIds;
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

FSGSTableUIStateStore::FSGSTableUIStateStore(int32 InViewerSeat)
	: ViewerSeat(InViewerSeat)
	, SnapshotState(FSGSTableViewSnapshot(), &SameSnapshotRevision)
	, InteractionState(FSGSTableUIInteractionState(), &SameInteraction)
	, HandPresentationState(FSGSTableHandPresentationState(), &SameHandPresentation)
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
	SnapshotState.Set(MoveTemp(AcceptedSnapshot));
	ReconcileHandPresentation();
	NormalizeSelection();
	return true;
}

bool FSGSTableUIStateStore::SelectCard(int32 CardId)
{
	if (!IsCardSelectable(CardId))
	{
		return false;
	}

	FSGSTableUIInteractionState Next = InteractionState.Get();
	Next.SelectedCardId = CardId;
	const TArray<int32> Targets = GetTargetsForCard(CardId);
	if (Targets.Num() == 1)
	{
		Next.SelectedTargetSeat = Targets[0];
	}
	else if (!Targets.Contains(Next.SelectedTargetSeat))
	{
		Next.SelectedTargetSeat = INDEX_NONE;
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

	Next.SelectedTargetSeat = TargetSeatIndex;
	InteractionState.Set(MoveTemp(Next));
	return true;
}

bool FSGSTableUIStateStore::SelectSkill(FName SkillName)
{
	if (!bHasSnapshot || !GetSnapshot().Prompt.bHasPrompt || !GetSnapshot().Prompt.bIsResponse)
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
	Next.SelectedTargetSeat = INDEX_NONE;
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
		if (Skill->bRequiresCard && !Skill->SelectableCardIds.Contains(Interaction.SelectedCardId))
		{
			return false;
		}
	}
	else if (!GetSnapshot().Prompt.SelectableCardIds.Contains(Interaction.SelectedCardId))
	{
		return false;
	}

	const TArray<int32> Targets = GetTargetsForCurrentSelection();
	return Targets.IsEmpty()
		|| Targets.Num() == 1
		|| Targets.Contains(Interaction.SelectedTargetSeat);
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
		if (!Skill->bRequiresCard || !Skill->SelectableCardIds.Contains(Next.SelectedCardId))
		{
			Next.SelectedCardId = INDEX_NONE;
		}
	}
	else if (!Snapshot.Prompt.SelectableCardIds.Contains(Next.SelectedCardId))
	{
		Next.SelectedCardId = INDEX_NONE;
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
	}
	else if (!Targets.Contains(Next.SelectedTargetSeat))
	{
		Next.SelectedTargetSeat = INDEX_NONE;
	}
	InteractionState.Set(MoveTemp(Next));
}
