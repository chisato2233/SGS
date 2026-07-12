#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Features/Table/State/SGSTableUIStateStore.h"

class FSGSUIContext;

struct SGS_API FSGSTableFeatureBindings
{
	TFunction<FSGSTableViewSnapshot()> ReadSnapshot;
	TFunction<bool(int32 CardId, int32 TargetSeat)> SubmitUseCard;
	TFunction<bool(int32 CardId, int32 TargetSeat, FName SkillName)> SubmitResponseCard;
	TFunction<bool()> SubmitPass;
};

// Table 组件树唯一业务协调入口。Host 只注入快照和 intent adapter；Widget
// 不再持有 PlayerController，也不负责合法选择或 RPC 分支。
class SGS_API FSGSTableFeatureController
{
public:
	FSGSTableFeatureController(
		int32 InViewerSeat,
		FSGSTableFeatureBindings InBindings,
		TSharedRef<FSGSUIContext> InUIContext);

	bool RefreshFromHost();
	bool SelectCard(int32 CardId);
	bool SelectTarget(int32 SeatIndex);
	bool SelectSkill(FName SkillName);
	bool ReorderHand(TConstArrayView<int32> OrderedCardIds);
	bool Confirm();
	bool Pass();

	const FSGSTableViewSnapshot& GetSnapshot() const { return State.GetSnapshot(); }
	const FSGSTableUIInteractionState& GetInteraction() const { return State.GetInteractionState(); }
	const FSGSTableHandPresentationState& GetHandPresentation() const
	{
		return State.GetHandPresentation();
	}
	const TSGSUIObservable<int32>& GetPublicRevisionState() const { return State.GetPublicRevisionState(); }
	const TSGSUIObservable<int32>& GetPrivateRevisionState() const { return State.GetPrivateRevisionState(); }
	const TSGSUIObservable<FSGSTableUIInteractionState>& GetInteractionState() const
	{
		return State.GetInteractionStateValue();
	}
	const TSGSUIObservable<FSGSTableHandPresentationState>& GetHandPresentationState() const
	{
		return State.GetHandPresentationState();
	}

	TSharedRef<FSGSUIContext> GetUIContext() const { return UIContext; }
	bool IsCardSelectable(int32 CardId) const { return State.IsCardSelectable(CardId); }
	bool IsTargetSelectable(int32 SeatIndex) const { return State.IsTargetSelectable(SeatIndex); }
	bool IsConfirmEnabled() const;
	FString GetPromptTitle() const;
	FString GetPromptText() const;
	FString GetPromptContextText() const;
	FString GetConfirmLabel() const;
	FString GetPassLabel() const;

private:
	void PublishToast(const FText& Message, bool bSuccess);
	void FocusConfirmIfReady();

	FSGSTableFeatureBindings Bindings;
	TSharedRef<FSGSUIContext> UIContext;
	FSGSTableUIStateStore State;
};
