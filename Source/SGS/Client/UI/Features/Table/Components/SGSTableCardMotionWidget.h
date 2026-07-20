#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Layout/SGSTableLayout.h"
#include "Shared/UI/SGSTableViewTypes.h"
#include "Widgets/SCompoundWidget.h"

struct FSlateBrush;
struct FSGSTableCardMotionWidgetModel;

struct SGS_API FSGSTableMotionCueProps
{
	FSGSTableCardMotionCueViewData Cue;
	TArray<const FSlateBrush*> VisibleCardBrushes;
};

struct SGS_API FSGSTableMotionProps
{
	int32 MotionEpoch = INDEX_NONE;
	int32 ViewerSeat = INDEX_NONE;
	FSGSTableLayoutMetrics Layout;
	FVector2D CardSize = FVector2D(64.0f, 90.0f);
	const FSlateBrush* CardBackBrush = nullptr;
	TArray<FSGSTableMotionCueProps> PendingCues;
};

DECLARE_DELEGATE_OneParam(FSGSOnTableMotionCueFinished, int32);

// 全桌唯一的瞬时牌演出层。只在有牌移动时注册一个 Active Timer；每帧仅更新
// transient card 的 RenderTransform/Opacity，并在一批结束后确认消费序号。
class SGS_API SSGSTableCardMotionWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableCardMotionWidget) {}
		SLATE_ARGUMENT(FSGSTableMotionProps, Props)
		SLATE_EVENT(FSGSOnTableMotionCueFinished, OnCueFinished)
	SLATE_END_ARGS()

	SSGSTableCardMotionWidget();
	virtual ~SSGSTableCardMotionWidget() override;
	void Construct(const FArguments& InArgs);
	void SetProps(FSGSTableMotionProps InProps);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	void ResetMotion();
	void ScheduleNextBatch();
	void EnsureActiveTimer();
	EActiveTimerReturnType UpdateMotion(double CurrentTime, float DeltaTime);

	TUniquePtr<FSGSTableCardMotionWidgetModel> Model;
	FSGSOnTableMotionCueFinished OnCueFinished;
};
