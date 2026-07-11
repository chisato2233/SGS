#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Core/State/SGSUIState.h"
#include "Shared/UI/SGSTableViewTypes.h"

struct SGS_API FSGSTableUIInteractionState
{
	int32 SelectedCardId = INDEX_NONE;
	int32 SelectedTargetSeat = INDEX_NONE;
};

// Table feature 的本地非权威状态边界。通用 observable/selector/batch 由 Core
// 提供；revision、viewer 和合法选择归一化仍完全属于 Table。
class SGS_API FSGSTableUIStateStore
{
public:
	explicit FSGSTableUIStateStore(int32 InViewerSeat = INDEX_NONE);

	bool IngestSnapshot(const FSGSTableViewSnapshot& InSnapshot);
	bool SelectCard(int32 CardId);
	bool SelectTarget(int32 TargetSeatIndex);
	void ClearSelection();

	bool HasSnapshot() const { return bHasSnapshot; }
	int32 GetViewerSeat() const { return ViewerSeat; }
	const FSGSTableViewSnapshot& GetSnapshot() const { return SnapshotState.Get(); }
	const FSGSTableUIInteractionState& GetInteractionState() const { return InteractionState.Get(); }

	const TSGSUIObservable<int32>& GetPublicRevisionState() const
	{
		return PublicRevisionSelector.GetObservable();
	}

	const TSGSUIObservable<int32>& GetPrivateRevisionState() const
	{
		return PrivateRevisionSelector.GetObservable();
	}

	const TSGSUIObservable<FSGSTableUIInteractionState>& GetInteractionStateValue() const
	{
		return InteractionState;
	}
	const TSGSUIObservable<FSGSTableViewSnapshot>& GetSnapshotStateValue() const
	{
		return SnapshotState;
	}

private:
	bool IsRevisionAccepted(const FSGSTableViewSnapshot& InSnapshot) const;
	TArray<int32> GetTargetsForCard(int32 CardId) const;
	void NormalizeSelection();

	int32 ViewerSeat = INDEX_NONE;
	bool bHasSnapshot = false;
	TSGSUIObservable<FSGSTableViewSnapshot> SnapshotState;
	TSGSUIObservable<FSGSTableUIInteractionState> InteractionState;
	TSGSUISelector<FSGSTableViewSnapshot, int32> PublicRevisionSelector;
	TSGSUISelector<FSGSTableViewSnapshot, int32> PrivateRevisionSelector;
};
