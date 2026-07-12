#include "Client/UI/Features/Table/Components/SGSTableComponents.h"

#include "Client/UI/Features/Table/Components/SGSTableDecisionPanelWidget.h"
#include "Client/UI/Features/Table/Components/SGSTableHandWidget.h"
#include "Client/UI/Theme/SGSUITheme.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
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

void SSGSTableShellWidget::Construct(const FArguments& InArgs)
{
	Props = InArgs._Props;
	OnCardClicked = InArgs._OnCardClicked;
	OnHandReordered = InArgs._OnHandReordered;
	OnSeatClicked = InArgs._OnSeatClicked;
	OnSkillClicked = InArgs._OnSkillClicked;
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
	if (EnumHasAnyFlags(Change, ESGSTableViewChange::HandPresentation))
	{
		RebuildHand();
	}
}

void SSGSTableShellWidget::RebuildShell()
{
	SeatHosts.Reset();
	TSharedRef<SConstraintCanvas> Canvas = SNew(SConstraintCanvas)
		.Clipping(EWidgetClipping::ClipToBounds);

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

	RebuildSeats();
	RebuildDecisionBar();
	RebuildHand();
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
			SNew(SSGSTableDecisionPanelWidget)
				.Props(Props.DecisionBar)
				.OnSkillClicked(OnSkillClicked)
				.OnConfirmClicked(OnConfirmClicked)
				.OnPassClicked(OnPassClicked));
	}
}

void SSGSTableShellWidget::RebuildHand()
{
	if (!HandHost.IsValid())
	{
		return;
	}

	if (!HandWidget.IsValid())
	{
		SAssignNew(HandWidget, SSGSTableHandWidget)
			.Props(Props.Hand)
			.OnCardClicked(OnCardClicked)
			.OnHandReordered(OnHandReordered);
		HandHost->SetContent(HandWidget.ToSharedRef());
		MountedHandHost = HandHost;
		return;
	}

	HandWidget->SetProps(Props.Hand);
	if (MountedHandHost.Pin() != HandHost)
	{
		HandHost->SetContent(HandWidget.ToSharedRef());
		MountedHandHost = HandHost;
	}
}
