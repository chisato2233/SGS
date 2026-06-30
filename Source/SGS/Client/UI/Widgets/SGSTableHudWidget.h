#pragma once

#include "CoreMinimal.h"
#include "Shared/UI/SGSTableViewTypes.h"
#include "Widgets/SCompoundWidget.h"

class ASGSPlayerController;

class SGS_API SSGSTableHudWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableHudWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ASGSPlayerController>, PlayerController)
		SLATE_ARGUMENT(TFunction<FSGSTableViewSnapshot()>, SnapshotProvider)
		SLATE_ARGUMENT(int32, ViewerSeat)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	EActiveTimerReturnType HandleRefreshTimer(double InCurrentTime, float InDeltaTime);
	void Refresh();
	void NormalizeSelection();

	TSharedRef<SWidget> BuildContent();
	TSharedRef<SWidget> BuildHeader() const;
	TSharedRef<SWidget> BuildSeats();
	TSharedRef<SWidget> BuildSeatButton(const FSGSSeatViewData& Seat);
	TSharedRef<SWidget> BuildHand();
	TSharedRef<SWidget> BuildCardButton(const FSGSCardViewData& Card);
	TSharedRef<SWidget> BuildControls();

	FReply OnCardClicked(int32 CardId);
	FReply OnSeatClicked(int32 SeatIndex);
	FReply OnConfirmClicked();
	FReply OnPassClicked();

	bool IsConfirmEnabled() const;
	TArray<int32> GetTargetsForCard(int32 CardId) const;
	FString GetPromptText() const;

	TWeakObjectPtr<ASGSPlayerController> PlayerController;
	TFunction<FSGSTableViewSnapshot()> SnapshotProvider;
	int32 ViewerSeat = 0;
	FSGSTableViewSnapshot Snapshot;
	int32 SelectedCardId = INDEX_NONE;
	int32 SelectedTargetSeat = INDEX_NONE;
};
