#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Layout/SGSTableLayout.h"
#include "Shared/UI/SGSTableViewTypes.h"
#include "Widgets/SCompoundWidget.h"

class ASGSPlayerController;
struct FSlateDynamicImageBrush;

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
	void Refresh(bool bForceRebuild = false);
	void NormalizeSelection();
	bool ShouldRebuildContent() const;
	void MarkContentRendered();
	FVector2D GetLayoutViewSize() const;
	const FSlateBrush* GetBackgroundBrush();

	TSharedRef<SWidget> BuildContent();
	TSharedRef<SWidget> BuildHeader() const;
	TSharedRef<SWidget> BuildSeatButton(const FSGSSeatViewData& Seat, FVector2D Size);
	TSharedRef<SWidget> BuildHand();
	TSharedRef<SWidget> BuildCardButton(const FSGSCardViewData& Card);
	TSharedRef<SWidget> BuildControls();
	TSharedRef<SWidget> BuildCenterInfo() const;

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
	int32 LastRenderedPublicRevision = INDEX_NONE;
	int32 LastRenderedPrivateRevision = INDEX_NONE;
	int32 LastRenderedSelectedCardId = INDEX_NONE;
	int32 LastRenderedSelectedTargetSeat = INDEX_NONE;
	FVector2D LastRenderedViewSize = FVector2D::ZeroVector;
	FVector2D CurrentViewSize = FVector2D::ZeroVector;
	TSharedPtr<FSlateDynamicImageBrush> BackgroundBrush;
	bool bHasRenderedContent = false;
};
