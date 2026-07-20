#include "Client/UI/Features/Table/Components/SGSTableComponents.h"

#include "Client/UI/Features/Table/Components/SGSTableDecisionPanelWidget.h"
#include "Client/UI/Features/Table/Components/SGSTableHandWidget.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SOverlay.h"

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
	SeatWidgets.Reset();
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
	if (SeatHosts.Num() != Props.Seats.Num() || SeatWidgets.Num() > Props.Seats.Num())
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
		if (TSharedPtr<SSGSTableSeatWidget>* ExistingWidget =
			SeatWidgets.Find(PositionedSeat.Seat.SeatIndex))
		{
			(*ExistingWidget)->SetProps(PositionedSeat.Seat);
			continue;
		}

		TSharedPtr<SSGSTableSeatWidget> SeatWidget;
		SAssignNew(SeatWidget, SSGSTableSeatWidget)
			.Props(PositionedSeat.Seat)
			.OnSeatClicked(OnSeatClicked);
		(*SeatHost)->SetContent(SeatWidget.ToSharedRef());
		SeatWidgets.Add(PositionedSeat.Seat.SeatIndex, MoveTemp(SeatWidget));
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
