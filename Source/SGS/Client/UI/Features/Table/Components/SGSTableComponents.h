#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"

struct FSlateBrush;
class FSGSUIContext;
class SBox;
class SSGSTableDecisionPanelWidget;
class SSGSTableHandWidget;

DECLARE_DELEGATE_RetVal_OneParam(FReply, FSGSOnTableCardClicked, int32);
DECLARE_DELEGATE_RetVal_OneParam(FReply, FSGSOnTableSeatClicked, int32);
DECLARE_DELEGATE_RetVal_OneParam(FReply, FSGSOnTableSkillClicked, FName);
DECLARE_DELEGATE_RetVal_OneParam(bool, FSGSOnTableHandReordered, const TArray<int32>&);

struct SGS_API FSGSTableCardProps
{
	int32 CardId = INDEX_NONE;
	FText CornerText;
	FText FooterText;
	FVector2D Size = FVector2D::ZeroVector;
	const FSlateBrush* FaceBrush = nullptr;
	bool bSelectable = false;
	bool bSelected = false;
	bool bDimmed = false;
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
	FText TitleText;
	FText PromptText;
	FText ContextText;
	FText ConfirmText;
	FText PassText;
	TSharedPtr<FSGSUIContext> UIContext;
	struct FSkillOption
	{
		FName SkillName = NAME_None;
		FText Label;
		bool bSelected = false;
		bool bEnabled = true;
	};
	TArray<FSkillOption> SkillOptions;
	float LayoutScale = 1.0f;
	bool bHasPrompt = false;
	bool bIsResponse = false;
	bool bShowActions = true;
	bool bCanConfirm = false;
	bool bCanPass = false;
};

struct SGS_API FSGSTablePositionedSeatProps
{
	FVector2D Position = FVector2D::ZeroVector;
	FSGSTableSeatProps Seat;
};

struct SGS_API FSGSTableShellProps
{
	const FSlateBrush* BackgroundBrush = nullptr;
	FSlateRect ControlArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	FSlateRect HandArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	TArray<FSGSTablePositionedSeatProps> Seats;
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
	HandPresentation = 1 << 4,
	All = Layout | PublicState | PrivateState | Interaction | HandPresentation
};
ENUM_CLASS_FLAGS(ESGSTableViewChange);

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

// Table 的纯组合壳。它只解释矩形、props 和回调，不读取快照、Store、
// PlayerController 或资源路径。
class SGS_API SSGSTableShellWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableShellWidget) {}
		SLATE_ARGUMENT(FSGSTableShellProps, Props)
		SLATE_EVENT(FSGSOnTableCardClicked, OnCardClicked)
		SLATE_EVENT(FSGSOnTableHandReordered, OnHandReordered)
		SLATE_EVENT(FSGSOnTableSeatClicked, OnSeatClicked)
		SLATE_EVENT(FSGSOnTableSkillClicked, OnSkillClicked)
		SLATE_EVENT(FOnClicked, OnConfirmClicked)
		SLATE_EVENT(FOnClicked, OnPassClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetProps(FSGSTableShellProps InProps, ESGSTableViewChange Change);

private:
	void RebuildShell();
	void RebuildSeats();
	void RebuildDecisionBar();
	void RebuildHand();

	FSGSTableShellProps Props;
	FSGSOnTableCardClicked OnCardClicked;
	FSGSOnTableHandReordered OnHandReordered;
	FSGSOnTableSeatClicked OnSeatClicked;
	FSGSOnTableSkillClicked OnSkillClicked;
	FOnClicked OnConfirmClicked;
	FOnClicked OnPassClicked;
	TMap<int32, TSharedPtr<SBox>> SeatHosts;
	TSharedPtr<SBox> DecisionHost;
	TSharedPtr<SBox> HandHost;
	TSharedPtr<SSGSTableHandWidget> HandWidget;
	TWeakPtr<SBox> MountedHandHost;
};
