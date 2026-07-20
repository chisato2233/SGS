#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"

struct FSlateBrush;
class SBox;
class SImage;
class STextBlock;

DECLARE_DELEGATE_RetVal_OneParam(FReply, FSGSOnTableSeatClicked, int32);

struct SGS_API FSGSTableSeatProps
{
	int32 SeatIndex = INDEX_NONE;
	FText NameText;
	FVector2D Size = FVector2D::ZeroVector;
	const FSlateBrush* PortraitBrush = nullptr;
	int32 Health = 0;
	int32 MaxHealth = 0;
	int32 HandCount = 0;
	float LayoutScale = 1.0f;
	bool bAlive = true;
	bool bSelectable = false;
	bool bSelected = false;
	bool bCurrent = false;
	bool bViewer = false;
};

// 单个牌桌座位的纯展示组件。
// 只消费 Props、发出点击意图，不读取快照、规则状态或资源路径。
// 头像与回合状态原位更新；仅体力上限或布局比例变化时重建体力子节点。
class SGS_API SSGSTableSeatWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableSeatWidget) {}
		SLATE_ARGUMENT(FSGSTableSeatProps, Props)
		SLATE_EVENT(FSGSOnTableSeatClicked, OnSeatClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetProps(FSGSTableSeatProps InProps);

private:
	FReply HandleClicked() const;
	void RebuildHealthDisplay();
	void UpdateHealthDisplay();
	FLinearColor GetActiveHealthColor() const;
	FText GetFooterText() const;

	FSGSTableSeatProps Props;
	FSGSOnTableSeatClicked OnSeatClicked;
	TSharedPtr<SBox> HealthHost;
	TSharedPtr<STextBlock> HealthTextBlock;
	TArray<TSharedPtr<SImage>> HealthPips;
	int32 RenderedMaxHealth = INDEX_NONE;
	float RenderedLayoutScale = 0.0f;
};
