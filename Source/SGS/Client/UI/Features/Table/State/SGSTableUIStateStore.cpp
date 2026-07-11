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
		&& A.SelectedTargetSeat == B.SelectedTargetSeat;
}
}

FSGSTableUIStateStore::FSGSTableUIStateStore(int32 InViewerSeat)
	: ViewerSeat(InViewerSeat)
	, SnapshotState(FSGSTableViewSnapshot(), &SameSnapshotRevision)
	, InteractionState(FSGSTableUIInteractionState(), &SameInteraction)
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

	FSGSTableViewSnapshot AcceptedSnapshot = InSnapshot;
	if (ViewerSeat != INDEX_NONE)
	{
		AcceptedSnapshot.ViewerSeat = ViewerSeat;
	}

	FSGSUIStateBatch Batch;
	bHasSnapshot = true;
	SnapshotState.Set(MoveTemp(AcceptedSnapshot));
	NormalizeSelection();
	return true;
}

bool FSGSTableUIStateStore::SelectCard(int32 CardId)
{
	const FSGSTableViewSnapshot& Snapshot = GetSnapshot();
	if (!bHasSnapshot
		|| !Snapshot.Prompt.bHasPrompt
		|| !Snapshot.Prompt.SelectableCardIds.Contains(CardId))
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
	const TArray<int32> Targets = GetTargetsForCard(Next.SelectedCardId);
	if (Next.SelectedCardId == INDEX_NONE || !Targets.Contains(TargetSeatIndex))
	{
		return false;
	}

	Next.SelectedTargetSeat = TargetSeatIndex;
	InteractionState.Set(MoveTemp(Next));
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
	if (const TArray<int32>* Targets = GetSnapshot().Prompt.FindTargetSeatIndicesForCard(CardId))
	{
		return *Targets;
	}
	return TArray<int32>();
}

void FSGSTableUIStateStore::NormalizeSelection()
{
	const FSGSTableViewSnapshot& Snapshot = GetSnapshot();
	FSGSTableUIInteractionState Next = InteractionState.Get();
	if (!Snapshot.Prompt.bHasPrompt
		|| !Snapshot.Prompt.SelectableCardIds.Contains(Next.SelectedCardId))
	{
		InteractionState.Set(FSGSTableUIInteractionState());
		return;
	}

	const TArray<int32> Targets = GetTargetsForCard(Next.SelectedCardId);
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
