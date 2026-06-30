#include "UI/Widgets/SGSTableHudWidget.h"

#include "UI/Bridge/SGSLocalHumanDecisionAgent.h"
#include "UI/Theme/SGSUITheme.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
FString TagLeaf(const FGameplayTag& Tag)
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
	GameDriver = InArgs._GameDriver;
	DecisionAgent = InArgs._DecisionAgent;
	ViewerSeat = InArgs._ViewerSeat;

	ChildSlot
	[
		SNullWidget::NullWidget
	];

	Refresh();
	RegisterActiveTimer(FSGSUITheme::RefreshIntervalSeconds(), FWidgetActiveTimerDelegate::CreateSP(this, &SSGSTableHudWidget::HandleRefreshTimer));
}

EActiveTimerReturnType SSGSTableHudWidget::HandleRefreshTimer(double InCurrentTime, float InDeltaTime)
{
	Refresh();
	return EActiveTimerReturnType::Continue;
}

void SSGSTableHudWidget::Refresh()
{
	Snapshot = FSGSTableViewModel::Build(GameDriver.Get(), DecisionAgent.Get(), ViewerSeat);
	NormalizeSelection();

	ChildSlot
	[
		BuildContent()
	];
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

TSharedRef<SWidget> SSGSTableHudWidget::BuildContent()
{
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);

	Root->AddSlot()
	.AutoHeight()
	.Padding(FSGSUITheme::SectionPadding())
	[
		BuildHeader()
	];

	Root->AddSlot()
	.AutoHeight()
	.Padding(FSGSUITheme::SectionPadding())
	[
		BuildSeats()
	];

	Root->AddSlot()
	.FillHeight(1.0f)
	.Padding(FSGSUITheme::SectionPadding())
	[
		BuildHand()
	];

	Root->AddSlot()
	.AutoHeight()
	.Padding(FSGSUITheme::SectionPadding())
	[
		BuildControls()
	];

	return SNew(SBorder)
		.Padding(FSGSUITheme::RootPadding())
		[
			Root
		];
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildHeader() const
{
	const FString Header = FString::Printf(
		TEXT("SGS Local Table | Viewer %d | Actor %d | Phase %s | Draw %d | Discard %d%s"),
		Snapshot.ViewerSeat,
		Snapshot.CurrentSeatIndex,
		*TagLeaf(Snapshot.CurrentPhase),
		Snapshot.DrawPileCount,
		Snapshot.DiscardPileCount,
		Snapshot.bGameOver ? TEXT(" | Game Over") : TEXT(""));

	return SNew(STextBlock)
		.Text(FText::FromString(Header))
		.AutoWrapText(true);
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildSeats()
{
	TSharedRef<SHorizontalBox> Seats = SNew(SHorizontalBox);
	for (const FSGSSeatViewData& Seat : Snapshot.Seats)
	{
		Seats->AddSlot()
		.FillWidth(1.0f)
		.Padding(FSGSUITheme::ItemPadding())
		[
			BuildSeatButton(Seat)
		];
	}
	return Seats;
}

TSharedRef<SWidget> SSGSTableHudWidget::BuildSeatButton(const FSGSSeatViewData& Seat)
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

	const FVector2D MinSize = FSGSUITheme::SeatButtonMinSize();
	return SNew(SBox)
		.MinDesiredWidth(MinSize.X)
		.MinDesiredHeight(MinSize.Y)
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
	USGSLocalHumanDecisionAgent* Agent = DecisionAgent.Get();
	if (Agent == nullptr)
	{
		return FReply::Handled();
	}

	if (Snapshot.Prompt.bIsResponse)
	{
		Agent->SubmitResponseCard(SelectedCardId, SelectedTargetSeat);
	}
	else
	{
		Agent->SubmitUseCard(SelectedCardId, SelectedTargetSeat);
	}

	SelectedCardId = INDEX_NONE;
	SelectedTargetSeat = INDEX_NONE;
	Refresh();
	return FReply::Handled();
}

FReply SSGSTableHudWidget::OnPassClicked()
{
	if (USGSLocalHumanDecisionAgent* Agent = DecisionAgent.Get())
	{
		Agent->SubmitPass();
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
	if (const TArray<int32>* Targets = Snapshot.Prompt.TargetSeatIndicesByCardId.Find(CardId))
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
