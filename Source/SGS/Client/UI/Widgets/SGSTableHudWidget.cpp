#include "Client/UI/Widgets/SGSTableHudWidget.h"

#include "Client/Game/SGSPlayerController.h"
#include "Client/UI/Theme/SGSUITheme.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

namespace
{
FString HudTagLeaf(const FGameplayTag& Tag)
{
	FString Full = Tag.ToString();
	FString Left;
	FString Right;
	if (Full.Split(TEXT("."), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		return Right;
	}
	return Full;
}

FSlateColor ButtonTint(bool bSelected, bool bAvailable, bool bCurrent)
{
	if (bSelected)
	{
		return FSGSUITheme::ControlTint(ESGSUIControlTone::Selected);
	}
	if (bAvailable)
	{
		return FSGSUITheme::ControlTint(ESGSUIControlTone::Available);
	}
	if (bCurrent)
	{
		return FSGSUITheme::ControlTint(ESGSUIControlTone::Current);
	}
	return FSGSUITheme::ControlTint(ESGSUIControlTone::Normal);
}
}

void SSGSTableHudWidget::Construct(const FArguments& InArgs)
{
	PlayerController = InArgs._PlayerController;
	SnapshotProvider = InArgs._SnapshotProvider;
	ViewerSeat = InArgs._ViewerSeat;

	ChildSlot
	[
		SNullWidget::NullWidget
	];

	Refresh(/*bForceRebuild*/ true);
	RegisterActiveTimer(FSGSUITheme::RefreshIntervalSeconds(), FWidgetActiveTimerDelegate::CreateSP(this, &SSGSTableHudWidget::HandleRefreshTimer));
}

EActiveTimerReturnType SSGSTableHudWidget::HandleRefreshTimer(double InCurrentTime, float InDeltaTime)
{
	Refresh();
	return EActiveTimerReturnType::Continue;
}

void SSGSTableHudWidget::Refresh(bool bForceRebuild)
{
	if (const ASGSPlayerController* Controller = PlayerController.Get())
	{
		Snapshot = Controller->BuildTableViewSnapshot();
	}
	else
	{
		Snapshot = SnapshotProvider ? SnapshotProvider() : FSGSTableViewSnapshot();
	}
	Snapshot.ViewerSeat = ViewerSeat;
	CurrentViewSize = GetLayoutViewSize();
	NormalizeSelection();

	if (!bForceRebuild && !ShouldRebuildContent())
	{
		return;
	}

	ChildSlot
	[
		BuildContent()
	];
	MarkContentRendered();
}

void SSGSTableHudWidget::NormalizeSelection()
{
	if (!Snapshot.Prompt.SelectableCardIds.Contains(SelectedCardId))
	{
		SelectedCardId = INDEX_NONE;
		SelectedTargetSeat = INDEX_NONE;
		return;
	}

	const TArray<int32> Targets = GetTargetsForCard(SelectedCardId);
	if (Targets.Num() == 1)
	{
		SelectedTargetSeat = Targets[0];
	}
	else if (SelectedTargetSeat != INDEX_NONE && !Targets.Contains(SelectedTargetSeat))
	{
		SelectedTargetSeat = INDEX_NONE;
	}
}

bool SSGSTableHudWidget::ShouldRebuildContent() const
{
	return !bHasRenderedContent
		|| Snapshot.PublicRevision != LastRenderedPublicRevision
		|| Snapshot.PrivateRevision != LastRenderedPrivateRevision
		|| SelectedCardId != LastRenderedSelectedCardId
		|| SelectedTargetSeat != LastRenderedSelectedTargetSeat
		|| !CurrentViewSize.Equals(LastRenderedViewSize, 1.0f);
}

void SSGSTableHudWidget::MarkContentRendered()
{
	LastRenderedPublicRevision = Snapshot.PublicRevision;
	LastRenderedPrivateRevision = Snapshot.PrivateRevision;
	LastRenderedSelectedCardId = SelectedCardId;
	LastRenderedSelectedTargetSeat = SelectedTargetSeat;
	LastRenderedViewSize = CurrentViewSize;
	bHasRenderedContent = true;
}

FVector2D SSGSTableHudWidget::GetLayoutViewSize() const
{
	const FVector2D LocalSize = GetCachedGeometry().GetLocalSize();
	if (LocalSize.X >= 800.0f && LocalSize.Y >= 540.0f)
	{
		return LocalSize;
	}
	return FVector2D(1280.0f, 720.0f);
}

const FSlateBrush* SSGSTableHudWidget::GetBackgroundBrush()
{
	if (!BackgroundBrush.IsValid())
	{
		const FString BackgroundPath = FPaths::ProjectContentDir() / TEXT("ImportedAssets/NoName/Background/ol_bg.jpg");
		BackgroundBrush = MakeShared<FSlateDynamicImageBrush>(
			FName(*BackgroundPath),
			FVector2D(1366.0f, 768.0f));
	}
	return BackgroundBrush.Get();
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildContent()
{
	const FSGSTableLayoutMetrics Layout = FSGSTableLayoutMetrics::Make(
		CurrentViewSize,
		Snapshot.Seats.Num(),
		Snapshot.ViewerSeat);

	TSharedRef<SConstraintCanvas> Canvas = SNew(SConstraintCanvas);

	const FSlateRect CenterArea = Layout.CenterArea;
	Canvas->AddSlot()
		.Anchors(FAnchors(0.0f, 0.0f))
		.Offset(FMargin(
			CenterArea.Left,
			CenterArea.Top,
			FMath::Max(0.0f, CenterArea.Right - CenterArea.Left),
			FMath::Max(0.0f, CenterArea.Bottom - CenterArea.Top)))
		[
			BuildCenterInfo()
		];

	for (const FSGSSeatViewData& Seat : Snapshot.Seats)
	{
		const FSGSTableSeatLayout* SeatLayout = Layout.FindSeat(Seat.SeatIndex);
		if (SeatLayout == nullptr)
		{
			continue;
		}

		Canvas->AddSlot()
			.Anchors(FAnchors(0.0f, 0.0f))
			.Offset(FMargin(
				SeatLayout->Position.X,
				SeatLayout->Position.Y,
				SeatLayout->Size.X,
				SeatLayout->Size.Y))
			[
				BuildSeatButton(Seat, SeatLayout->Size)
			];
	}

	const FSlateRect ControlArea = Layout.ControlArea;
	Canvas->AddSlot()
		.Anchors(FAnchors(0.0f, 0.0f))
		.Offset(FMargin(
			ControlArea.Left,
			ControlArea.Top,
			FMath::Max(0.0f, ControlArea.Right - ControlArea.Left),
			FMath::Max(0.0f, ControlArea.Bottom - ControlArea.Top)))
		[
			BuildControls()
		];

	const FSlateRect HandArea = Layout.HandArea;
	Canvas->AddSlot()
		.Anchors(FAnchors(0.0f, 0.0f))
		.Offset(FMargin(
			HandArea.Left,
			HandArea.Top,
			FMath::Max(0.0f, HandArea.Right - HandArea.Left),
			FMath::Max(0.0f, HandArea.Bottom - HandArea.Top)))
		[
			BuildHand()
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage)
				.Image(GetBackgroundBrush())
				.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
		]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.30f))
		]
		+ SOverlay::Slot()
		[
			Canvas
		];
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildHeader() const
{
	const FString Header = FString::Printf(
		TEXT("SGS Local Table | Viewer %d | Actor %d | Phase %s | Draw %d | Discard %d%s"),
		Snapshot.ViewerSeat,
		Snapshot.CurrentSeatIndex,
		*HudTagLeaf(Snapshot.CurrentPhase),
		Snapshot.DrawPileCount,
		Snapshot.DiscardPileCount,
		Snapshot.bGameOver ? TEXT(" | Game Over") : TEXT(""));

	return SNew(STextBlock)
		.Text(FText::FromString(Header))
		.AutoWrapText(true);
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildSeatButton(const FSGSSeatViewData& Seat, FVector2D Size)
{
	const bool bSelected = Seat.SeatIndex == SelectedTargetSeat;
	const FString StateText = Seat.bIsAlive ? TEXT("Alive") : TEXT("Out");
	const FString Label = FString::Printf(
		TEXT("%s%s\nHP %d/%d\nHand %d\n%s"),
		Seat.bIsCurrent ? TEXT("> ") : TEXT(""),
		*Seat.DisplayName,
		Seat.Health,
		Seat.MaxHealth,
		Seat.HandCount,
		*StateText);

	return SNew(SBox)
		.WidthOverride(Size.X)
		.HeightOverride(Size.Y)
		[
			SNew(SButton)
			.ButtonColorAndOpacity(ButtonTint(bSelected, Seat.bIsSelectableTarget, Seat.bIsCurrent))
			.OnClicked(this, &SSGSTableHudWidget::OnSeatClicked, Seat.SeatIndex)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
			]
		];
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildHand()
{
	TSharedRef<SScrollBox> HandScroll = SNew(SScrollBox)
		.Orientation(Orient_Horizontal);

	if (Snapshot.HandCards.Num() == 0)
	{
		HandScroll->AddSlot()
		.Padding(FSGSUITheme::CardPadding())
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Hand is empty.")))
		];
		return HandScroll;
	}

	for (const FSGSCardViewData& Card : Snapshot.HandCards)
	{
		HandScroll->AddSlot()
		.Padding(FSGSUITheme::CardPadding())
		[
			BuildCardButton(Card)
		];
	}

	return HandScroll;
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildCardButton(const FSGSCardViewData& Card)
{
	const bool bSelected = Card.CardId == SelectedCardId;
	const FString Label = FString::Printf(TEXT("#%d\n%s"), Card.CardId, *Card.DisplayText);

	const FVector2D Size = FSGSUITheme::CardButtonSize();
	return SNew(SBox)
		.WidthOverride(Size.X)
		.HeightOverride(Size.Y)
		[
			SNew(SButton)
			.IsEnabled(Card.bSelectable)
			.ButtonColorAndOpacity(ButtonTint(bSelected, Card.bSelectable, false))
			.OnClicked(this, &SSGSTableHudWidget::OnCardClicked, Card.CardId)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
			]
		];
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildControls()
{
	TSharedRef<SVerticalBox> Controls = SNew(SVerticalBox);

	Controls->AddSlot()
	.AutoHeight()
	.Padding(FSGSUITheme::PromptGapPadding())
	[
		SNew(STextBlock)
		.Text(FText::FromString(GetPromptText()))
		.AutoWrapText(true)
	];

	TSharedRef<SHorizontalBox> Buttons = SNew(SHorizontalBox);
	const FVector2D ActionButtonMinSize = FSGSUITheme::ActionButtonMinSize();
	Buttons->AddSlot()
	.AutoWidth()
	.Padding(FSGSUITheme::ButtonGapPadding())
	[
		SNew(SBox)
		.MinDesiredWidth(ActionButtonMinSize.X)
		.MinDesiredHeight(ActionButtonMinSize.Y)
		[
			SNew(SButton)
			.IsEnabled(IsConfirmEnabled())
			.OnClicked(this, &SSGSTableHudWidget::OnConfirmClicked)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Confirm")))
			]
		]
	];

	Buttons->AddSlot()
	.AutoWidth()
	[
		SNew(SBox)
		.MinDesiredWidth(ActionButtonMinSize.X)
		.MinDesiredHeight(ActionButtonMinSize.Y)
		[
			SNew(SButton)
			.IsEnabled(Snapshot.Prompt.bHasPrompt && Snapshot.Prompt.bAllowPass)
			.OnClicked(this, &SSGSTableHudWidget::OnPassClicked)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Pass")))
			]
		]
	];

	Controls->AddSlot()
	.AutoHeight()
	[
		Buttons
	];

	if (!Snapshot.LastCommand.IsEmpty())
	{
		Controls->AddSlot()
		.AutoHeight()
		.Padding(FSGSUITheme::PromptGapPadding())
		[
			SNew(STextBlock)
			.Text(FText::FromString(Snapshot.LastCommand))
			.AutoWrapText(true)
		];
	}

	return Controls;
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildCenterInfo() const
{
	TSharedRef<SVerticalBox> Center = SNew(SVerticalBox);
	Center->AddSlot()
		.AutoHeight()
		[
			BuildHeader()
		];

	if (!Snapshot.LastCommand.IsEmpty())
	{
		Center->AddSlot()
			.AutoHeight()
			.Padding(FSGSUITheme::PromptGapPadding())
			[
				SNew(STextBlock)
				.Text(FText::FromString(Snapshot.LastCommand))
				.AutoWrapText(true)
			];
	}

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.35f))
		.Padding(FSGSUITheme::RootPadding())
		[
			Center
		];
}

FReply SSGSTableHudWidget::OnCardClicked(int32 CardId)
{
	SelectedCardId = CardId;
	const TArray<int32> Targets = GetTargetsForCard(CardId);
	if (Targets.Num() == 1)
	{
		SelectedTargetSeat = Targets[0];
	}
	else if (!Targets.Contains(SelectedTargetSeat))
	{
		SelectedTargetSeat = INDEX_NONE;
	}
	Refresh();
	return FReply::Handled();
}

FReply SSGSTableHudWidget::OnSeatClicked(int32 SeatIndex)
{
	const TArray<int32> Targets = GetTargetsForCard(SelectedCardId);
	if (Targets.Contains(SeatIndex))
	{
		SelectedTargetSeat = SeatIndex;
		Refresh();
	}
	return FReply::Handled();
}

FReply SSGSTableHudWidget::OnConfirmClicked()
{
	ASGSPlayerController* Controller = PlayerController.Get();
	if (Controller == nullptr)
	{
		return FReply::Handled();
	}

	if (Snapshot.Prompt.bIsResponse)
	{
		Controller->SubmitResponseCard(SelectedCardId, SelectedTargetSeat);
	}
	else
	{
		Controller->SubmitUseCard(SelectedCardId, SelectedTargetSeat);
	}

	SelectedCardId = INDEX_NONE;
	SelectedTargetSeat = INDEX_NONE;
	Refresh();
	return FReply::Handled();
}

FReply SSGSTableHudWidget::OnPassClicked()
{
	if (ASGSPlayerController* Controller = PlayerController.Get())
	{
		Controller->SubmitPass();
	}

	SelectedCardId = INDEX_NONE;
	SelectedTargetSeat = INDEX_NONE;
	Refresh();
	return FReply::Handled();
}

bool SSGSTableHudWidget::IsConfirmEnabled() const
{
	if (!Snapshot.Prompt.bHasPrompt || !Snapshot.Prompt.SelectableCardIds.Contains(SelectedCardId))
	{
		return false;
	}

	const TArray<int32> Targets = GetTargetsForCard(SelectedCardId);
	return Targets.Num() == 0 || Targets.Num() == 1 || Targets.Contains(SelectedTargetSeat);
}

TArray<int32> SSGSTableHudWidget::GetTargetsForCard(int32 CardId) const
{
	if (const TArray<int32>* Targets = Snapshot.Prompt.FindTargetSeatIndicesForCard(CardId))
	{
		return *Targets;
	}
	return TArray<int32>();
}

FString SSGSTableHudWidget::GetPromptText() const
{
	if (!Snapshot.Prompt.bHasPrompt)
	{
		return TEXT("No local decision pending.");
	}

	if (Snapshot.Prompt.bIsResponse)
	{
		return FString::Printf(
			TEXT("Response window: %s | Need: %s | Select a card, then Confirm or Pass."),
			*Snapshot.Prompt.WindowName.ToString(),
			*Snapshot.Prompt.RequiredCardName.ToString());
	}

	return TEXT("Play phase: select a legal card, choose a highlighted target if needed, then Confirm or Pass.");
}
