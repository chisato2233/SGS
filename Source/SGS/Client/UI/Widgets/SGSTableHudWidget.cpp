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

FString IdentityDisplayName(const FGameplayTag& Identity)
{
	if (Identity == SGSGameplayTags::Identity_Lord.GetTag())
	{
		return TEXT("主公");
	}
	if (Identity == SGSGameplayTags::Identity_Loyalist.GetTag())
	{
		return TEXT("忠臣");
	}
	if (Identity == SGSGameplayTags::Identity_Rebel.GetTag())
	{
		return TEXT("反贼");
	}
	if (Identity == SGSGameplayTags::Identity_Renegade.GetTag())
	{
		return TEXT("内奸");
	}
	return FString();
}

FSGSTableSeatProps MakeSeatProps(
	const FSGSSeatViewData& Seat,
	FVector2D Size,
	float LayoutScale,
	int32 ViewerSeat,
	int32 SelectedTargetSeat,
	bool bSelectable,
	FSGSTableAssetCatalog& Assets)
{
	FSGSTableSeatProps Props;
	Props.SeatIndex = Seat.SeatIndex;
	const FString IdentityName = IdentityDisplayName(Seat.Identity);
	const FString IdentityPrefix = IdentityName.IsEmpty()
		? FString()
		: FString::Printf(TEXT("[%s] "), *IdentityName);
	Props.NameText = FText::FromString(FString::Printf(
		TEXT("%s%s"),
		*IdentityPrefix,
		*Seat.DisplayName));
	Props.Size = Size;
	Props.PortraitBrush = Assets.GetGeneralPortraitBrush(Seat.GeneralId);
	Props.FrameBrush = Assets.GetSeatFrameBrush(Seat.Faction);
	Props.HealthHighBrush = Assets.GetSeatHealthBrush(ESGSSeatHealthVisual::High);
	Props.HealthMidBrush = Assets.GetSeatHealthBrush(ESGSSeatHealthVisual::Mid);
	Props.HealthLowBrush = Assets.GetSeatHealthBrush(ESGSSeatHealthVisual::Low);
	Props.HealthLostBrush = Assets.GetSeatHealthBrush(ESGSSeatHealthVisual::Lost);
	Props.Health = Seat.Health;
	Props.MaxHealth = Seat.MaxHealth;
	Props.HandCount = Seat.HandCount;
	Props.LayoutScale = LayoutScale;
	Props.bAlive = Seat.bIsAlive;
	Props.bSelectable = bSelectable;
	Props.bSelected = Seat.SeatIndex == SelectedTargetSeat;
	Props.bCurrent = Seat.bIsCurrent;
	Props.bViewer = Seat.SeatIndex == ViewerSeat;
	return Props;
}

FSGSTableCardProps MakeCardProps(
	const FSGSCardViewData& Card,
	FVector2D Size,
	int32 SelectedCardId,
	bool bSelectable,
	bool bDimmed,
	FSGSTableAssetCatalog& Assets)
{
	FSGSTableCardProps Props;
	Props.CardId = Card.CardId;
	Props.CornerText = FText::FromString(FString::Printf(TEXT("%d %s"), Card.Number, *TagLeaf(Card.Suit)));
	const FString CardName = Card.CardName == FName(TEXT("Analeptic"))
		? TEXT("酒")
		: Card.CardName.ToString();
	Props.FooterText = FText::FromString(FString::Printf(
		TEXT("%s  #%d"),
		*CardName,
		Card.CardId));
	Props.Size = Size;
	Props.FaceBrush = Assets.GetCardFaceBrush(Card.CardName);
	Props.bSelectable = bSelectable;
	Props.bSelected = Card.CardId == SelectedCardId;
	Props.bDimmed = bDimmed;
	return Props;
}

FSGSTableShellProps MakeShellProps(
	FSGSTableFeatureController& Controller,
	FSGSTableAssetCatalog& Assets,
	FVector2D ViewSize)
{
	const FSGSTableViewSnapshot& Snapshot = Controller.GetSnapshot();
	const FSGSTableUIInteractionState& Interaction = Controller.GetInteraction();
	const bool bGameFinished = Snapshot.GameResult.IsFinished();
	const FSGSTableLayoutMetrics Layout = FSGSTableLayoutMetrics::Make(
		ViewSize,
		Snapshot.Seats.Num(),
		Snapshot.ViewerSeat);

	FSGSTableShellProps Props;
	Props.BackgroundBrush = Assets.GetBackgroundBrush();
	Props.ControlArea = Layout.ControlArea;
	Props.HandArea = Layout.HandArea;
	Props.DrawPile.Area = Layout.DrawPileArea;
	Props.DrawPile.CardBrush = Assets.GetCardBackBrush();
	Props.DrawPile.Label = FText::FromString(TEXT("牌堆"));
	Props.DrawPile.Count = Snapshot.DrawPileCount;
	Props.DrawPile.bShowCard = Snapshot.DrawPileCount > 0;
	Props.PlayArea.Area = Layout.PlayArea;
	Props.PlayArea.Label = FText::FromString(TEXT("出牌区"));
	Props.DiscardPile.Area = Layout.DiscardPileArea;
	Props.DiscardPile.Label = FText::FromString(TEXT("弃牌"));
	Props.DiscardPile.Count = Snapshot.DiscardPileCount;
	Props.DiscardPile.CardBrush = Snapshot.bHasDiscardTopCard
		? Assets.GetCardFaceBrush(Snapshot.DiscardTopCard.CardName)
		: nullptr;
	Props.DiscardPile.bShowCard = Snapshot.bHasDiscardTopCard;

	Props.Motion.MotionEpoch = Snapshot.MotionEpoch;
	Props.Motion.ViewerSeat = Snapshot.ViewerSeat;
	Props.Motion.Layout = Layout;
	Props.Motion.CardSize = FVector2D(64.0f, 90.0f) * Layout.LayoutScale;
	Props.Motion.CardBackBrush = Assets.GetCardBackBrush();
	Props.Motion.PendingCues.Reserve(Controller.GetMotionPresentation().PendingCues.Num());
	for (const FSGSTableCardMotionCueViewData& Cue : Controller.GetMotionPresentation().PendingCues)
	{
		FSGSTableMotionCueProps& CueProps = Props.Motion.PendingCues.AddDefaulted_GetRef();
		CueProps.Cue = Cue;
		CueProps.VisibleCardBrushes.Reserve(Cue.VisibleCards.Num());
		for (const FSGSCardViewData& Card : Cue.VisibleCards)
		{
			CueProps.VisibleCardBrushes.Add(Assets.GetCardFaceBrush(Card.CardName));
		}
	}
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
			Layout.LayoutScale,
			Snapshot.ViewerSeat,
			Interaction.SelectedTargetSeat,
			!bGameFinished && Controller.IsTargetSelectable(Seat.SeatIndex),
			Assets);
	}

	Props.DecisionBar.bHasPrompt = bGameFinished || Snapshot.Prompt.bHasPrompt;
	Props.DecisionBar.bIsResponse = !bGameFinished && Snapshot.Prompt.bIsResponse;
	Props.DecisionBar.bShowActions = !bGameFinished;
	if (bGameFinished)
	{
		TArray<FString> WinningIdentityNames;
		for (const FGameplayTag& Identity : Snapshot.GameResult.WinningIdentities)
		{
			WinningIdentityNames.AddUnique(IdentityDisplayName(Identity));
		}
		Props.DecisionBar.TitleText = FText::FromString(
			Snapshot.GameResult.WinningSeatIndices.Contains(Snapshot.ViewerSeat)
				? TEXT("胜利")
				: TEXT("失败"));
		Props.DecisionBar.PromptText = FText::FromString(FString::Printf(
			TEXT("获胜阵营：%s"),
			*FString::Join(WinningIdentityNames, TEXT("、"))));
	}
	else
	{
		Props.DecisionBar.TitleText = FText::FromString(Controller.GetPromptTitle());
		Props.DecisionBar.PromptText = FText::FromString(Controller.GetPromptText());
		Props.DecisionBar.ContextText = FText::FromString(Controller.GetPromptContextText());
	}
	Props.DecisionBar.ConfirmText = FText::FromString(Controller.GetConfirmLabel());
	Props.DecisionBar.PassText = FText::FromString(Controller.GetPassLabel());
	Props.DecisionBar.UIContext = Controller.GetUIContext();
	Props.DecisionBar.LayoutScale = Layout.LayoutScale;
	Props.DecisionBar.bCanConfirm = !bGameFinished && Controller.IsConfirmEnabled();
	Props.DecisionBar.bCanPass = !bGameFinished && Snapshot.Prompt.bHasPrompt && Snapshot.Prompt.bAllowPass;
	if (!bGameFinished)
	{
		Props.DecisionBar.SkillOptions.Reserve(Snapshot.Prompt.SkillOptions.Num());
		for (const FSGSDecisionSkillViewData& Skill : Snapshot.Prompt.SkillOptions)
		{
			FSGSTableDecisionBarProps::FSkillOption& SkillProps =
				Props.DecisionBar.SkillOptions.AddDefaulted_GetRef();
			SkillProps.SkillName = Skill.SkillName;
			SkillProps.Label = FText::FromString(
				Skill.DisplayName.IsEmpty() ? Skill.SkillName.ToString() : Skill.DisplayName);
			SkillProps.bSelected = Interaction.SelectedSkillName == Skill.SkillName;
		}
	}
	Props.Hand.CardSize = Layout.HandCardSize;
	Props.Hand.LayoutScale = Layout.LayoutScale;
	Props.Hand.AvailableWidth = FMath::Max(0.0f, Layout.HandArea.Right - Layout.HandArea.Left);
	Props.Hand.Cards.Reserve(Snapshot.HandCards.Num());
	TMap<int32, const FSGSCardViewData*> CardsById;
	CardsById.Reserve(Snapshot.HandCards.Num());
	for (const FSGSCardViewData& Card : Snapshot.HandCards)
	{
		CardsById.Add(Card.CardId, &Card);
	}

	TSet<int32> AddedCardIds;
	AddedCardIds.Reserve(Snapshot.HandCards.Num());
	for (const int32 CardId : Controller.GetHandPresentation().OrderedCardIds)
	{
		if (const FSGSCardViewData* const* Card = CardsById.Find(CardId))
		{
			Props.Hand.Cards.Add(MakeCardProps(
				**Card,
				Layout.HandCardSize,
				Interaction.SelectedCardId,
				!bGameFinished && Controller.IsCardSelectable(CardId),
				Snapshot.Prompt.bHasPrompt && !Controller.IsCardSelectable(CardId),
				Assets));
			AddedCardIds.Add(CardId);
		}
	}

	// 首帧或异常输入下仍保持完整可见；Store 会在下一次有效快照中归一化顺序。
	for (const FSGSCardViewData& Card : Snapshot.HandCards)
	{
		if (!AddedCardIds.Contains(Card.CardId))
		{
			Props.Hand.Cards.Add(MakeCardProps(
				Card,
				Layout.HandCardSize,
				Interaction.SelectedCardId,
				!bGameFinished && Controller.IsCardSelectable(Card.CardId),
				Snapshot.Prompt.bHasPrompt && !Controller.IsCardSelectable(Card.CardId),
				Assets));
		}
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
	Controller->GetHandPresentationState().Subscribe(
		Lifetime,
		[WeakOwner](const FSGSTableHandPresentationState& Presentation)
		{
			if (const TSharedPtr<SSGSTableHudWidget> Pinned = WeakOwner.Pin())
			{
				Pinned->HandleHandPresentation(Presentation);
			}
		},
		false);
	Controller->GetMotionPresentationState().Subscribe(
		Lifetime,
		[WeakOwner](const FSGSTableMotionPresentationState& Presentation)
		{
			if (const TSharedPtr<SSGSTableHudWidget> Pinned = WeakOwner.Pin())
			{
				Pinned->HandleMotionPresentation(Presentation);
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
		.OnHandReordered(this, &SSGSTableHudWidget::OnHandReordered)
		.OnSeatClicked(this, &SSGSTableHudWidget::OnSeatClicked)
		.OnSkillClicked(this, &SSGSTableHudWidget::OnSkillClicked)
		.OnConfirmClicked(this, &SSGSTableHudWidget::OnConfirmClicked)
		.OnPassClicked(this, &SSGSTableHudWidget::OnPassClicked)
		.OnMotionCueFinished(this, &SSGSTableHudWidget::OnMotionCueFinished);
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

void SSGSTableHudWidget::HandleHandPresentation(const FSGSTableHandPresentationState& Presentation)
{
	UpdateShell(ESGSTableViewChange::HandPresentation);
}

void SSGSTableHudWidget::HandleMotionPresentation(const FSGSTableMotionPresentationState& Presentation)
{
	UpdateShell(ESGSTableViewChange::Motion);
}

void SSGSTableHudWidget::OnMotionCueFinished(int32 Sequence)
{
	if (Controller.IsValid())
	{
		Controller->AcknowledgeMotionCue(Sequence);
	}
}

FReply SSGSTableHudWidget::OnCardClicked(int32 CardId)
{
	Controller->SelectCard(CardId);
	return FReply::Handled();
}

bool SSGSTableHudWidget::OnHandReordered(const TArray<int32>& OrderedCardIds)
{
	return Controller->ReorderHand(OrderedCardIds);
}

FReply SSGSTableHudWidget::OnSeatClicked(int32 SeatIndex)
{
	Controller->SelectTarget(SeatIndex);
	return FReply::Handled();
}

FReply SSGSTableHudWidget::OnSkillClicked(FName SkillName)
{
	Controller->SelectSkill(SkillName);
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
