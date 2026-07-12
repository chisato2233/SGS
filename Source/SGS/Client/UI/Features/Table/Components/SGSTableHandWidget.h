#pragma once

#include "Client/UI/Features/Table/Components/SGSTableComponents.h"

class FDragDropEvent;
struct FPointerEvent;
class FSGSCardDragDropOperation;
class SBorder;
class SBox;
class SImage;
class STextBlock;
struct FSGSTableHandWidgetModel;

DECLARE_DELEGATE_RetVal_TwoParams(
	FReply,
	FSGSOnTableCardDragDetected,
	int32,
	const FPointerEvent&);
DECLARE_DELEGATE_TwoParams(FSGSOnTableCardHoverChanged, int32, bool);

// 一张长期复用的手牌叶组件。点击选择与拖拽检测在这里分流，但所有
// 重排和动画状态均由最近的 Hand owner 持有。
class SGS_API SSGSTableCardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableCardWidget) {}
		SLATE_ARGUMENT(FSGSTableCardProps, Props)
		SLATE_EVENT(FSGSOnTableCardClicked, OnCardClicked)
		SLATE_EVENT(FSGSOnTableCardDragDetected, OnCardDragDetected)
		SLATE_EVENT(FSGSOnTableCardHoverChanged, OnCardHoverChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetProps(const FSGSTableCardProps& InProps);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

private:
	FSGSTableCardProps Props;
	FSGSOnTableCardClicked OnCardClicked;
	FSGSOnTableCardDragDetected OnCardDragDetected;
	FSGSOnTableCardHoverChanged OnCardHoverChanged;
	TSharedPtr<SBox> SizeBox;
	TSharedPtr<SBorder> InteractionBorder;
	TSharedPtr<SBorder> UnavailableShade;
	TSharedPtr<SImage> FaceImage;
	TSharedPtr<STextBlock> CornerText;
	TSharedPtr<STextBlock> FooterText;
	bool bPressPending = false;
};

// 手牌组件拥有 keyed 子节点、预览顺序和唯一动画时钟。每帧只更新
// RenderTransform；Canvas slot 只在卡牌集合、布局或最终顺序变化时更新。
class SGS_API SSGSTableHandWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableHandWidget) {}
		SLATE_ARGUMENT(FSGSTableHandProps, Props)
		SLATE_EVENT(FSGSOnTableCardClicked, OnCardClicked)
		SLATE_EVENT(FSGSOnTableHandReordered, OnHandReordered)
	SLATE_END_ARGS()

	SSGSTableHandWidget();
	virtual ~SSGSTableHandWidget() override;

	void Construct(const FArguments& InArgs);
	void SetProps(const FSGSTableHandProps& InProps);

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

private:
	friend class FSGSCardDragDropOperation;

	FReply HandleCardDragDetected(int32 CardId, const FPointerEvent& MouseEvent);
	void HandleCardHoverChanged(int32 CardId, bool bHovered);
	void HandleDragMoved(const FVector2D& ScreenPosition);
	void HandleDragOperationEnded(int32 CardId, bool bDroppedInHand);
	EActiveTimerReturnType UpdateMotion(double CurrentTime, float DeltaTime);
	void EnsureMotionTimer();
	void FinishDrag(bool bCommit);

	TUniquePtr<FSGSTableHandWidgetModel> Model;
	FSGSOnTableCardClicked OnCardClicked;
	FSGSOnTableHandReordered OnHandReordered;
};
