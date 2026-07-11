#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"

struct FSlateBrush;
class FSGSUIContext;
class SBox;

DECLARE_DELEGATE_RetVal_OneParam(FReply, FSGSOnTableCardClicked, int32);
DECLARE_DELEGATE_RetVal_OneParam(FReply, FSGSOnTableSeatClicked, int32);

struct SGS_API FSGSTableCardProps
{
	int32 CardId = INDEX_NONE;
	FText CornerText;
	FText FooterText;
	FVector2D Size = FVector2D::ZeroVector;
	const FSlateBrush* FaceBrush = nullptr;
	bool bSelectable = false;
	bool bSelected = false;
};

struct SGS_API FSGSTableSeatProps
{
	int32 SeatIndex = INDEX_NONE;
	FText NameText;
	FText StatusText;
	FVector2D Size = FVector2D::ZeroVector;
	const FSlateBrush* PortraitBrush = nullptr;
	bool bAlive = true;
	bool bSelectable = false;
	bool bSelected = false;
	bool bCurrent = false;
	bool bViewer = false;
};

struct SGS_API FSGSTableHandProps
{
	TArray<FSGSTableCardProps> Cards;
	FVector2D CardSize = FVector2D::ZeroVector;
	float LayoutScale = 1.0f;
	float AvailableWidth = 0.0f;
};

struct SGS_API FSGSTableDecisionBarProps
{
	FText PromptText;
	TSharedPtr<FSGSUIContext> UIContext;
	float LayoutScale = 1.0f;
	bool bCanConfirm = false;
	bool bCanPass = false;
};

struct SGS_API FSGSTableCenterInfoProps
{
	FText HeaderText;
	FText LastCommandText;
};

struct SGS_API FSGSTablePositionedSeatProps
{
	FVector2D Position = FVector2D::ZeroVector;
	FSGSTableSeatProps Seat;
};

struct SGS_API FSGSTableShellProps
{
	const FSlateBrush* BackgroundBrush = nullptr;
	FSlateRect CenterArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	FSlateRect ControlArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	FSlateRect HandArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	TArray<FSGSTablePositionedSeatProps> Seats;
	FSGSTableCenterInfoProps CenterInfo;
	FSGSTableDecisionBarProps DecisionBar;
	FSGSTableHandProps Hand;
};

enum class ESGSTableViewChange : uint8
{
	None = 0,
	Layout = 1 << 0,
	PublicState = 1 << 1,
	PrivateState = 1 << 2,
	Interaction = 1 << 3,
	All = Layout | PublicState | PrivateState | Interaction
};
ENUM_CLASS_FLAGS(ESGSTableViewChange);

class SGS_API SSGSTableCardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableCardWidget) {}
		SLATE_ARGUMENT(FSGSTableCardProps, Props)
		SLATE_EVENT(FSGSOnTableCardClicked, OnCardClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply HandleClicked() const;

	int32 CardId = INDEX_NONE;
	FSGSOnTableCardClicked OnCardClicked;
};

class SGS_API SSGSTableSeatWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableSeatWidget) {}
		SLATE_ARGUMENT(FSGSTableSeatProps, Props)
		SLATE_EVENT(FSGSOnTableSeatClicked, OnSeatClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply HandleClicked() const;

	int32 SeatIndex = INDEX_NONE;
	FSGSOnTableSeatClicked OnSeatClicked;
};

class SGS_API SSGSTableHandWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableHandWidget) {}
		SLATE_ARGUMENT(FSGSTableHandProps, Props)
		SLATE_EVENT(FSGSOnTableCardClicked, OnCardClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

class SGS_API SSGSTableDecisionBarWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableDecisionBarWidget) {}
		SLATE_ARGUMENT(FSGSTableDecisionBarProps, Props)
		SLATE_EVENT(FOnClicked, OnConfirmClicked)
		SLATE_EVENT(FOnClicked, OnPassClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

class SGS_API SSGSTableCenterInfoWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableCenterInfoWidget) {}
		SLATE_ARGUMENT(FSGSTableCenterInfoProps, Props)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

// Table 的纯组合壳。它只解释矩形、props 和回调，不读取快照、Store、
// PlayerController 或资源路径。
class SGS_API SSGSTableShellWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableShellWidget) {}
		SLATE_ARGUMENT(FSGSTableShellProps, Props)
		SLATE_EVENT(FSGSOnTableCardClicked, OnCardClicked)
		SLATE_EVENT(FSGSOnTableSeatClicked, OnSeatClicked)
		SLATE_EVENT(FOnClicked, OnConfirmClicked)
		SLATE_EVENT(FOnClicked, OnPassClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetProps(FSGSTableShellProps InProps, ESGSTableViewChange Change);

private:
	void RebuildShell();
	void RebuildCenter();
	void RebuildSeats();
	void RebuildDecisionBar();
	void RebuildHand();

	FSGSTableShellProps Props;
	FSGSOnTableCardClicked OnCardClicked;
	FSGSOnTableSeatClicked OnSeatClicked;
	FOnClicked OnConfirmClicked;
	FOnClicked OnPassClicked;
	TSharedPtr<SBox> CenterHost;
	TMap<int32, TSharedPtr<SBox>> SeatHosts;
	TSharedPtr<SBox> DecisionHost;
	TSharedPtr<SBox> HandHost;
};
