#include "Client/UI/Features/Table/Components/SGSTableComponents.h"

#include "Client/UI/Theme/SGSUITheme.h"
#include "Client/UI/Primitives/SGSUIFocusTarget.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
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

void SSGSTableCardWidget::Construct(const FArguments& InArgs)
{
	const FSGSTableCardProps& Props = InArgs._Props;
	CardId = Props.CardId;
	OnCardClicked = InArgs._OnCardClicked;

	TSharedRef<SOverlay> CardFace = SNew(SOverlay)
		.Clipping(EWidgetClipping::ClipToBounds);
	CardFace->AddSlot()
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.82f, 0.76f, 0.62f, 1.0f))
	];

	if (Props.FaceBrush != nullptr)
	{
		CardFace->AddSlot()
		[
			SNew(SImage)
				.Image(Props.FaceBrush)
				.ColorAndOpacity(FLinearColor::White)
		];
	}

	CardFace->AddSlot()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	.Padding(FMargin(3.0f))
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.97f, 0.93f, 0.82f, 0.88f))
			.Padding(FMargin(2.0f, 0.0f))
		[
			SNew(STextBlock)
				.Text(Props.CornerText)
				.ColorAndOpacity(FLinearColor(0.08f, 0.06f, 0.045f, 1.0f))
		]
	];

	CardFace->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Bottom)
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.018f, 0.78f))
			.Padding(FMargin(2.0f))
		[
			SNew(STextBlock)
				.Text(Props.FooterText)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
		]
	];

	ChildSlot
	[
		SNew(SBox)
			.WidthOverride(Props.Size.X)
			.HeightOverride(Props.Size.Y)
		[
			SNew(SButton)
				.IsEnabled(Props.bSelectable)
				.ButtonColorAndOpacity(ButtonTint(Props.bSelected, Props.bSelectable, false))
				.ContentPadding(FMargin(2.0f))
				.OnClicked(this, &SSGSTableCardWidget::HandleClicked)
			[
				CardFace
			]
		]
	];
}

FReply SSGSTableCardWidget::HandleClicked() const
{
	return OnCardClicked.IsBound() ? OnCardClicked.Execute(CardId) : FReply::Unhandled();
}

void SSGSTableSeatWidget::Construct(const FArguments& InArgs)
{
	const FSGSTableSeatProps& Props = InArgs._Props;
	SeatIndex = Props.SeatIndex;
	OnSeatClicked = InArgs._OnSeatClicked;

	TSharedRef<SOverlay> PlayerPanel = SNew(SOverlay)
		.Clipping(EWidgetClipping::ClipToBounds);
	PlayerPanel->AddSlot()
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.055f, 1.0f))
	];

	if (Props.PortraitBrush != nullptr)
	{
		PlayerPanel->AddSlot()
		[
			SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFill)
				.StretchDirection(EStretchDirection::Both)
				.Clipping(EWidgetClipping::ClipToBounds)
			[
				SNew(SImage)
					.Image(Props.PortraitBrush)
					.ColorAndOpacity(Props.bAlive
						? FLinearColor::White
						: FLinearColor(0.32f, 0.32f, 0.32f, 0.82f))
			]
		];
	}

	PlayerPanel->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Top)
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.82f))
			.Padding(FMargin(5.0f, 3.0f))
		[
			SNew(STextBlock)
				.Text(Props.NameText)
				.ColorAndOpacity(FLinearColor::White)
				.ShadowOffset(FVector2D(1.0f, 1.0f))
		]
	];

	PlayerPanel->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Bottom)
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.88f))
			.Padding(FMargin(4.0f, 3.0f))
		[
			SNew(STextBlock)
				.Text(Props.StatusText)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor(0.95f, 0.89f, 0.72f, 1.0f))
		]
	];

	ChildSlot
	[
		SNew(SBox)
			.WidthOverride(Props.Size.X)
			.HeightOverride(Props.Size.Y)
		[
			SNew(SButton)
				.ButtonColorAndOpacity(ButtonTint(Props.bSelected, Props.bSelectable, Props.bCurrent))
				.ContentPadding(FMargin(Props.bViewer ? 2.0f : 3.0f))
				.OnClicked(this, &SSGSTableSeatWidget::HandleClicked)
			[
				PlayerPanel
			]
		]
	];
}

FReply SSGSTableSeatWidget::HandleClicked() const
{
	return OnSeatClicked.IsBound() ? OnSeatClicked.Execute(SeatIndex) : FReply::Unhandled();
}

void SSGSTableHandWidget::Construct(const FArguments& InArgs)
{
	const FSGSTableHandProps& Props = InArgs._Props;
	const FSGSOnTableCardClicked OnCardClicked = InArgs._OnCardClicked;
	TSharedRef<SScrollBox> HandScroll = SNew(SScrollBox)
		.Orientation(Orient_Horizontal)
		.ScrollBarVisibility(EVisibility::Collapsed);

	if (Props.Cards.IsEmpty())
	{
		HandScroll->AddSlot()
		.Padding(FMargin(8.0f * Props.LayoutScale))
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("Hand is empty.")))
				.ColorAndOpacity(FLinearColor(0.82f, 0.82f, 0.78f, 1.0f))
		];
	}
	else
	{
		const float InnerPadding = 4.0f * Props.LayoutScale;
		const float CardLift = 6.0f * Props.LayoutScale;
		const float UsableWidth = FMath::Max(0.0f, Props.AvailableWidth - InnerPadding * 2.0f);
		const float MaxStride = Props.CardSize.X + 8.0f * Props.LayoutScale;
		const float MinStride = 32.0f * Props.LayoutScale;
		float Stride = 0.0f;
		if (Props.Cards.Num() > 1)
		{
			Stride = (UsableWidth - Props.CardSize.X) / static_cast<float>(Props.Cards.Num() - 1);
			Stride = FMath::Clamp(Stride, MinStride, MaxStride);
		}

		const float CardsWidth = Props.CardSize.X + Stride * FMath::Max(Props.Cards.Num() - 1, 0);
		const float InnerWidth = FMath::Max(UsableWidth, CardsWidth);
		const float InnerHeight = Props.CardSize.Y + CardLift;
		TSharedRef<SConstraintCanvas> HandCanvas = SNew(SConstraintCanvas)
			.Clipping(EWidgetClipping::ClipToBounds);
		for (int32 CardIndex = 0; CardIndex < Props.Cards.Num(); ++CardIndex)
		{
			const FSGSTableCardProps& Card = Props.Cards[CardIndex];
			HandCanvas->AddSlot()
				.Anchors(FAnchors(0.0f, 0.0f))
				.Alignment(FVector2D::ZeroVector)
				.Offset(FMargin(
					Stride * CardIndex,
					Card.bSelected ? 0.0f : CardLift,
					Props.CardSize.X,
					Props.CardSize.Y))
				.ZOrder(Card.bSelected ? 100 : CardIndex)
			[
				SNew(SSGSTableCardWidget)
					.Props(Card)
					.OnCardClicked(OnCardClicked)
			];
		}

		HandScroll->AddSlot()
		.Padding(FMargin(InnerPadding, 0.0f))
		.VAlign(VAlign_Bottom)
		[
			SNew(SBox)
				.WidthOverride(InnerWidth)
				.HeightOverride(InnerHeight)
			[
				HandCanvas
			]
		];
	}

	ChildSlot
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.72f))
			.Padding(FMargin(4.0f * Props.LayoutScale, 2.0f * Props.LayoutScale))
		[
			HandScroll
		]
	];
}

void SSGSTableDecisionBarWidget::Construct(const FArguments& InArgs)
{
	const FSGSTableDecisionBarProps& Props = InArgs._Props;
	const FVector2D ActionButtonMinSize = FSGSUITheme::ActionButtonMinSize() * Props.LayoutScale;

	ChildSlot
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.82f))
			.Padding(FMargin(4.0f * Props.LayoutScale))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(FMargin(6.0f * Props.LayoutScale, 0.0f))
			[
				SNew(STextBlock)
					.Text(Props.PromptText)
					.ColorAndOpacity(FLinearColor(0.93f, 0.91f, 0.82f, 1.0f))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 6.0f * Props.LayoutScale, 0.0f))
			[
				SNew(SBox)
					.MinDesiredWidth(ActionButtonMinSize.X)
					.MinDesiredHeight(ActionButtonMinSize.Y)
				[
					SNew(SSGSUIFocusTarget)
						.UIContext(Props.UIContext)
						.TargetId(TEXT("Table.Confirm"))
					[
						SNew(SButton)
							.IsEnabled(Props.bCanConfirm)
							.OnClicked(InArgs._OnConfirmClicked)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Confirm")))
						]
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
					.MinDesiredWidth(ActionButtonMinSize.X)
					.MinDesiredHeight(ActionButtonMinSize.Y)
				[
					SNew(SButton)
						.IsEnabled(Props.bCanPass)
						.OnClicked(InArgs._OnPassClicked)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Pass")))
					]
				]
			]
		]
	];
}

void SSGSTableCenterInfoWidget::Construct(const FArguments& InArgs)
{
	const FSGSTableCenterInfoProps& Props = InArgs._Props;
	TSharedRef<SVerticalBox> Center = SNew(SVerticalBox);
	Center->AddSlot()
	.AutoHeight()
	[
		SNew(STextBlock)
			.Text(Props.HeaderText)
			.AutoWrapText(true)
	];

	if (!Props.LastCommandText.IsEmpty())
	{
		Center->AddSlot()
		.AutoHeight()
		.Padding(FSGSUITheme::PromptGapPadding())
		[
			SNew(STextBlock)
				.Text(Props.LastCommandText)
				.AutoWrapText(true)
		];
	}

	ChildSlot
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.35f))
			.Padding(FSGSUITheme::RootPadding())
		[
			Center
		]
	];
}

void SSGSTableShellWidget::Construct(const FArguments& InArgs)
{
	Props = InArgs._Props;
	OnCardClicked = InArgs._OnCardClicked;
	OnSeatClicked = InArgs._OnSeatClicked;
	OnConfirmClicked = InArgs._OnConfirmClicked;
	OnPassClicked = InArgs._OnPassClicked;
	RebuildShell();
}

void SSGSTableShellWidget::SetProps(FSGSTableShellProps InProps, ESGSTableViewChange Change)
{
	Props = MoveTemp(InProps);
	if (EnumHasAnyFlags(Change, ESGSTableViewChange::Layout))
	{
		RebuildShell();
		return;
	}
	if (EnumHasAnyFlags(Change, ESGSTableViewChange::PublicState))
	{
		RebuildCenter();
		RebuildSeats();
		RebuildDecisionBar();
	}
	if (EnumHasAnyFlags(Change, ESGSTableViewChange::PrivateState))
	{
		RebuildSeats();
		RebuildHand();
		RebuildDecisionBar();
	}
	if (EnumHasAnyFlags(Change, ESGSTableViewChange::Interaction))
	{
		RebuildSeats();
		RebuildHand();
		RebuildDecisionBar();
	}
}

void SSGSTableShellWidget::RebuildShell()
{
	SeatHosts.Reset();
	TSharedRef<SConstraintCanvas> Canvas = SNew(SConstraintCanvas)
		.Clipping(EWidgetClipping::ClipToBounds);

	Canvas->AddSlot()
		.Anchors(FAnchors(0.0f, 0.0f))
		.Alignment(FVector2D::ZeroVector)
		.Offset(FMargin(
			Props.CenterArea.Left,
			Props.CenterArea.Top,
			FMath::Max(0.0f, Props.CenterArea.Right - Props.CenterArea.Left),
			FMath::Max(0.0f, Props.CenterArea.Bottom - Props.CenterArea.Top)))
	[
		SAssignNew(CenterHost, SBox)
	];

	for (const FSGSTablePositionedSeatProps& PositionedSeat : Props.Seats)
	{
		TSharedPtr<SBox> SeatHost;
		Canvas->AddSlot()
			.Anchors(FAnchors(0.0f, 0.0f))
			.Alignment(FVector2D::ZeroVector)
			.Offset(FMargin(
				PositionedSeat.Position.X,
				PositionedSeat.Position.Y,
				PositionedSeat.Seat.Size.X,
				PositionedSeat.Seat.Size.Y))
		[
			SAssignNew(SeatHost, SBox)
		];
		SeatHosts.Add(PositionedSeat.Seat.SeatIndex, SeatHost);
	}

	Canvas->AddSlot()
		.Anchors(FAnchors(0.0f, 0.0f))
		.Alignment(FVector2D::ZeroVector)
		.Offset(FMargin(
			Props.ControlArea.Left,
			Props.ControlArea.Top,
			FMath::Max(0.0f, Props.ControlArea.Right - Props.ControlArea.Left),
			FMath::Max(0.0f, Props.ControlArea.Bottom - Props.ControlArea.Top)))
	[
		SAssignNew(DecisionHost, SBox)
	];

	Canvas->AddSlot()
		.Anchors(FAnchors(0.0f, 0.0f))
		.Alignment(FVector2D::ZeroVector)
		.Offset(FMargin(
			Props.HandArea.Left,
			Props.HandArea.Top,
			FMath::Max(0.0f, Props.HandArea.Right - Props.HandArea.Left),
			FMath::Max(0.0f, Props.HandArea.Bottom - Props.HandArea.Top)))
	[
		SAssignNew(HandHost, SBox)
	];

	ChildSlot
	[
		SNew(SOverlay)
			.Clipping(EWidgetClipping::ClipToBounds)
		+ SOverlay::Slot()
		[
			SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFill)
				.StretchDirection(EStretchDirection::Both)
				.Clipping(EWidgetClipping::ClipToBounds)
			[
				SNew(SImage)
					.Image(Props.BackgroundBrush)
					.ColorAndOpacity(FLinearColor::White)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.12f))
		]
		+ SOverlay::Slot()
		[
			Canvas
		]
	];

	RebuildCenter();
	RebuildSeats();
	RebuildDecisionBar();
	RebuildHand();
}

void SSGSTableShellWidget::RebuildCenter()
{
	if (CenterHost.IsValid())
	{
		CenterHost->SetContent(SNew(SSGSTableCenterInfoWidget).Props(Props.CenterInfo));
	}
}

void SSGSTableShellWidget::RebuildSeats()
{
	if (SeatHosts.Num() != Props.Seats.Num())
	{
		RebuildShell();
		return;
	}
	for (const FSGSTablePositionedSeatProps& PositionedSeat : Props.Seats)
	{
		TSharedPtr<SBox>* SeatHost = SeatHosts.Find(PositionedSeat.Seat.SeatIndex);
		if (SeatHost == nullptr || !SeatHost->IsValid())
		{
			RebuildShell();
			return;
		}
		(*SeatHost)->SetContent(
			SNew(SSGSTableSeatWidget)
				.Props(PositionedSeat.Seat)
				.OnSeatClicked(OnSeatClicked));
	}
}

void SSGSTableShellWidget::RebuildDecisionBar()
{
	if (DecisionHost.IsValid())
	{
		DecisionHost->SetContent(
			SNew(SSGSTableDecisionBarWidget)
				.Props(Props.DecisionBar)
				.OnConfirmClicked(OnConfirmClicked)
				.OnPassClicked(OnPassClicked));
	}
}

void SSGSTableShellWidget::RebuildHand()
{
	if (HandHost.IsValid())
	{
		HandHost->SetContent(
			SNew(SSGSTableHandWidget)
				.Props(Props.Hand)
				.OnCardClicked(OnCardClicked));
	}
}
