#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Core/State/SGSUIState.h"
#include "Shared/UI/SGSTableViewTypes.h"

struct SGS_API FSGSTableUIInteractionState
{
	int32 SelectedCardId = INDEX_NONE;
	int32 SelectedTargetSeat = INDEX_NONE;
	FName SelectedSkillName = NAME_None;
};

// 仅描述当前客户端的手牌展示顺序。它不属于权威快照，也不会参与规则提交。
struct SGS_API FSGSTableHandPresentationState
{
	TArray<int32> OrderedCardIds;
};

struct SGS_API FSGSTableMotionPresentationState
{
	int32 MotionEpoch = INDEX_NONE;
	int32 LastConsumedSequence = INDEX_NONE;
	int32 LastQueuedSequence = INDEX_NONE;
	TArray<FSGSTableCardMotionCueViewData> PendingCues;
};

// Table feature 的本地非权威状态边界。通用 observable/selector/batch 由 Core
// 提供；revision、viewer 和合法选择归一化仍完全属于 Table。
class SGS_API FSGSTableUIStateStore
{
public:
	explicit FSGSTableUIStateStore(
		int32 InViewerSeat = INDEX_NONE,
		int32 InInitialMotionSequence = INDEX_NONE);

	bool IngestSnapshot(const FSGSTableViewSnapshot& InSnapshot);
	bool SelectCard(int32 CardId);
	bool SelectTarget(int32 TargetSeatIndex);
	bool SelectSkill(FName SkillName);
	bool ReorderHand(TConstArrayView<int32> OrderedCardIds);
	void AcknowledgeMotionCue(int32 Sequence);
	void ClearMotionQueueToLatest();
	void ClearSelection();
	bool IsCardSelectable(int32 CardId) const;
	bool IsTargetSelectable(int32 TargetSeatIndex) const;
	bool IsSelectionComplete() const;

	bool HasSnapshot() const { return bHasSnapshot; }
	int32 GetViewerSeat() const { return ViewerSeat; }
	const FSGSTableViewSnapshot& GetSnapshot() const { return SnapshotState.Get(); }
	const FSGSTableUIInteractionState& GetInteractionState() const { return InteractionState.Get(); }
	const FSGSTableHandPresentationState& GetHandPresentation() const
	{
		return HandPresentationState.Get();
	}
	const FSGSTableMotionPresentationState& GetMotionPresentation() const
	{
		return MotionPresentationState.Get();
	}

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
	const TSGSUIObservable<FSGSTableHandPresentationState>& GetHandPresentationState() const
	{
		return HandPresentationState;
	}
	const TSGSUIObservable<FSGSTableViewSnapshot>& GetSnapshotStateValue() const
	{
		return SnapshotState;
	}
	const TSGSUIObservable<FSGSTableMotionPresentationState>& GetMotionPresentationState() const
	{
		return MotionPresentationState;
	}

private:
	bool IsRevisionAccepted(const FSGSTableViewSnapshot& InSnapshot) const;
	const FSGSDecisionSkillViewData* GetSelectedSkillOption() const;
	TArray<int32> GetTargetsForCard(int32 CardId) const;
	TArray<int32> GetTargetsForCurrentSelection() const;
	void ReconcileHandPresentation();
	void NormalizeSelection();
	void ReconcileMotionPresentation(const FSGSTableViewSnapshot& InSnapshot);

	int32 ViewerSeat = INDEX_NONE;
	int32 InitialMotionSequence = INDEX_NONE;
	bool bHasSnapshot = false;
	TSGSUIObservable<FSGSTableViewSnapshot> SnapshotState;
	TSGSUIObservable<FSGSTableUIInteractionState> InteractionState;
	TSGSUIObservable<FSGSTableHandPresentationState> HandPresentationState;
	TSGSUIObservable<FSGSTableMotionPresentationState> MotionPresentationState;
	TSGSUISelector<FSGSTableViewSnapshot, int32> PublicRevisionSelector;
	TSGSUISelector<FSGSTableViewSnapshot, int32> PrivateRevisionSelector;
};
