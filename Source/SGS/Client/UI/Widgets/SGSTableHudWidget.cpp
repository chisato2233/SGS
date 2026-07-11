#include "Client/UI/Widgets/SGSTableHudWidget.h"

#include "Client/UI/Features/Table/Assets/SGSTableAssetCatalog.h"
#include "Client/UI/Features/Table/Components/SGSTableComponents.h"
#include "Client/UI/Features/Table/Controller/SGSTableFeatureController.h"
#include "Client/UI/Layout/SGSTableLayout.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SViewport.h"

namespace
{
FString TagLeaf(const FGameplayTag& Tag)
{
	FString Full = Tag.ToString();
	FString Left;
	FString Right;
	return Full.Split(TEXT("."), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd)
		? Right
		: Full;
}

FText MakeHeaderText(const FSGSTableViewSnapshot& Snapshot)
{
	return FText::FromString(FString::Printf(
		TEXT("SGS Local Table | Viewer %d | Actor %d | Phase %s | Draw %d | Discard %d%s"),
		Snapshot.ViewerSeat,
		Snapshot.CurrentSeatIndex,
		*TagLeaf(Snapshot.CurrentPhase),
		Snapshot.DrawPileCount,
		Snapshot.DiscardPileCount,
		Snapshot.bGameOver ? TEXT(" | Game Over") : TEXT("")));
}

FSGSTableSeatProps MakeSeatProps(
	const FSGSSeatViewData& Seat,
	FVector2D Size,
	int32 ViewerSeat,
	int32 SelectedTargetSeat,
	FSGSTableAssetCatalog& Assets)
{
	FSGSTableSeatProps Props;
	Props.SeatIndex = Seat.SeatIndex;
	Props.NameText = FText::FromString(FString::Printf(
		TEXT("%s%s"),
		Seat.bIsCurrent ? TEXT("[TURN] ") : TEXT(""),
		*Seat.DisplayName));
	Props.StatusText = FText::FromString(FString::Printf(
		TEXT("HP %d/%d  |  Hand %d%s"),
		Seat.Health,
		Seat.MaxHealth,
		Seat.HandCount,
		Seat.bIsAlive ? TEXT("") : TEXT("  |  OUT")));
	Props.Size = Size;
	Props.PortraitBrush = Assets.GetSeatPortraitBrush(Seat.SeatIndex);
	Props.bAlive = Seat.bIsAlive;
	Props.bSelectable = Seat.bIsSelectableTarget;
	Props.bSelected = Seat.SeatIndex == SelectedTargetSeat;
	Props.bCurrent = Seat.bIsCurrent;
	Props.bViewer = Seat.SeatIndex == ViewerSeat;
	return Props;
}

FSGSTableCardProps MakeCardProps(
	const FSGSCardViewData& Card,
	FVector2D Size,
	int32 SelectedCardId,
	FSGSTableAssetCatalog& Assets)
{
	FSGSTableCardProps Props;
	Props.CardId = Card.CardId;
	Props.CornerText = FText::FromString(FString::Printf(TEXT("%d %s"), Card.Number, *TagLeaf(Card.Suit)));
	Props.FooterText = FText::FromString(FString::Printf(
		TEXT("%s  #%d"),
		*Card.CardName.ToString(),
		Card.CardId));
	Props.Size = Size;
	Props.FaceBrush = Assets.GetCardFaceBrush(Card.CardName);
	Props.bSelectable = Card.bSelectable;
	Props.bSelected = Card.CardId == SelectedCardId;
	return Props;
}

FSGSTableShellProps MakeShellProps(
	FSGSTableFeatureController& Controller,
	FSGSTableAssetCatalog& Assets,
	FVector2D ViewSize)
{
	const FSGSTableViewSnapshot& Snapshot = Controller.GetSnapshot();
	const FSGSTableUIInteractionState& Interaction = Controller.GetInteraction();
	const FSGSTableLayoutMetrics Layout = FSGSTableLayoutMetrics::Make(
		ViewSize,
		Snapshot.Seats.Num(),
		Snapshot.ViewerSeat);

	FSGSTableShellProps Props;
	Props.BackgroundBrush = Assets.GetBackgroundBrush();
	Props.CenterArea = Layout.CenterArea;
	Props.ControlArea = Layout.ControlArea;
	Props.HandArea = Layout.HandArea;
	Props.CenterInfo.HeaderText = MakeHeaderText(Snapshot);
	Props.CenterInfo.LastCommandText = FText::FromString(Snapshot.LastCommand);
	Props.Seats.Reserve(Snapshot.Seats.Num());
	for (const FSGSSeatViewData& Seat : Snapshot.Seats)
	{
		const FSGSTableSeatLayout* SeatLayout = Layout.FindSeat(Seat.SeatIndex);
		if (SeatLayout == nullptr)
		{
			continue;
		}
		FSGSTablePositionedSeatProps& PositionedSeat = Props.Seats.AddDefaulted_GetRef();
		PositionedSeat.Position = SeatLayout->Position;
		PositionedSeat.Seat = MakeSeatProps(
			Seat,
			SeatLayout->Size,
			Snapshot.ViewerSeat,
			Interaction.SelectedTargetSeat,
			Assets);
	}

	Props.DecisionBar.PromptText = FText::FromString(Controller.GetPromptText());
	Props.DecisionBar.UIContext = Controller.GetUIContext();
	Props.DecisionBar.LayoutScale = Layout.LayoutScale;
	Props.DecisionBar.bCanConfirm = Controller.IsConfirmEnabled();
	Props.DecisionBar.bCanPass = Snapshot.Prompt.bHasPrompt && Snapshot.Prompt.bAllowPass;
	Props.Hand.CardSize = Layout.HandCardSize;
	Props.Hand.LayoutScale = Layout.LayoutScale;
	Props.Hand.AvailableWidth = FMath::Max(0.0f, Layout.HandArea.Right - Layout.HandArea.Left);
	Props.Hand.Cards.Reserve(Snapshot.HandCards.Num());
	for (const FSGSCardViewData& Card : Snapshot.HandCards)
	{
		Props.Hand.Cards.Add(MakeCardProps(
			Card,
			Layout.HandCardSize,
			Interaction.SelectedCardId,
			Assets));
	}
	return Props;
}
}

SSGSTableHudWidget::SSGSTableHudWidget()
	: Lifetime(TEXT("TableHud"))
{
}

SSGSTableHudWidget::~SSGSTableHudWidget()
{
	Lifetime.Reset();
}

void SSGSTableHudWidget::Construct(const FArguments& InArgs)
{
	Controller = InArgs._Controller;
	check(Controller.IsValid());
	AssetCatalog = MakeShared<FSGSTableAssetCatalog>();
	CurrentViewSize = GetLayoutViewSize();
	ChildSlot
	[
		BuildContent()
	];

	TWeakPtr<SSGSTableHudWidget> WeakOwner = SharedThis(this);
	Controller->GetPublicRevisionState().Subscribe(
		Lifetime,
		[WeakOwner](int32 Revision)
		{
			if (const TSharedPtr<SSGSTableHudWidget> Pinned = WeakOwner.Pin())
			{
				Pinned->HandlePublicRevision(Revision);
			}
		},
		false);
	Controller->GetPrivateRevisionState().Subscribe(
		Lifetime,
		[WeakOwner](int32 Revision)
		{
			if (const TSharedPtr<SSGSTableHudWidget> Pinned = WeakOwner.Pin())
			{
				Pinned->HandlePrivateRevision(Revision);
			}
		},
		false);
	Controller->GetInteractionState().Subscribe(
		Lifetime,
		[WeakOwner](const FSGSTableUIInteractionState& Interaction)
		{
			if (const TSharedPtr<SSGSTableHudWidget> Pinned = WeakOwner.Pin())
			{
				Pinned->HandleInteraction(Interaction);
			}
		},
		false);
}

void SSGSTableHudWidget::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const FVector2D NewViewSize = AllottedGeometry.GetLocalSize();
	if (NewViewSize.X > 1.0f
		&& NewViewSize.Y > 1.0f
		&& !NewViewSize.Equals(ArrangedViewSize, 1.0f))
	{
		ArrangedViewSize = NewViewSize;
		CurrentViewSize = NewViewSize;
		UpdateShell(ESGSTableViewChange::Layout);
	}
}

FVector2D SSGSTableHudWidget::GetLayoutViewSize() const
{
	if (ArrangedViewSize.X > 1.0f && ArrangedViewSize.Y > 1.0f)
	{
		return ArrangedViewSize;
	}
	const FVector2D LocalSize = GetCachedGeometry().GetLocalSize();
	if (LocalSize.X > 1.0f && LocalSize.Y > 1.0f)
	{
		return LocalSize;
	}
	if (GEngine != nullptr && GEngine->GameViewport != nullptr)
	{
		const TSharedPtr<SViewport> ViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
		if (ViewportWidget.IsValid())
		{
			const FVector2D ViewportSize = ViewportWidget->GetCachedGeometry().GetLocalSize();
			if (ViewportSize.X > 1.0f && ViewportSize.Y > 1.0f)
			{
				return ViewportSize;
			}
		}
	}
	return FSGSTableAssetCatalog::BackgroundImageSize();
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildContent()
{
	return SAssignNew(ShellWidget, SSGSTableShellWidget)
		.Props(MakeShellProps(*Controller, *AssetCatalog, CurrentViewSize))
		.OnCardClicked(this, &SSGSTableHudWidget::OnCardClicked)
		.OnSeatClicked(this, &SSGSTableHudWidget::OnSeatClicked)
		.OnConfirmClicked(this, &SSGSTableHudWidget::OnConfirmClicked)
		.OnPassClicked(this, &SSGSTableHudWidget::OnPassClicked);
}

void SSGSTableHudWidget::UpdateShell(ESGSTableViewChange Change)
{
	if (ShellWidget.IsValid())
	{
		ShellWidget->SetProps(MakeShellProps(*Controller, *AssetCatalog, CurrentViewSize), Change);
	}
}

void SSGSTableHudWidget::HandlePublicRevision(int32 Revision)
{
	UpdateShell(ESGSTableViewChange::PublicState);
}

void SSGSTableHudWidget::HandlePrivateRevision(int32 Revision)
{
	UpdateShell(ESGSTableViewChange::PrivateState);
}

void SSGSTableHudWidget::HandleInteraction(const FSGSTableUIInteractionState& Interaction)
{
	UpdateShell(ESGSTableViewChange::Interaction);
}

FReply SSGSTableHudWidget::OnCardClicked(int32 CardId)
{
	Controller->SelectCard(CardId);
	return FReply::Handled();
}

FReply SSGSTableHudWidget::OnSeatClicked(int32 SeatIndex)
{
	Controller->SelectTarget(SeatIndex);
	return FReply::Handled();
}

FReply SSGSTableHudWidget::OnConfirmClicked()
{
	Controller->Confirm();
	return FReply::Handled();
}

FReply SSGSTableHudWidget::OnPassClicked()
{
	Controller->Pass();
	return FReply::Handled();
}
