#include "Client/UI/Features/Table/Components/SGSTableSeatWidget.h"

#include "Client/UI/Theme/SGSUITheme.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
FSlateColor SeatButtonTint(bool bSelected, bool bSelectable)
{
	if (bSelected)
	{
		return FSGSUITheme::ControlTint(ESGSUIControlTone::Selected);
	}
	if (bSelectable)
	{
		return FSGSUITheme::ControlTint(ESGSUIControlTone::Available);
	}
	return FSGSUITheme::ControlTint(ESGSUIControlTone::Normal);
}
}

void SSGSTableSeatWidget::Construct(const FArguments& InArgs)
{
	Props = InArgs._Props;
	OnSeatClicked = InArgs._OnSeatClicked;

	ChildSlot
	[
		SAssignNew(SizeBox, SBox)
			.WidthOverride(Props.Size.X)
			.HeightOverride(Props.Size.Y)
		[
			SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor_Lambda([this]()
				{
					FLinearColor Color = FSGSUITheme::SeatCurrentGlowColor();
					Color.A = Props.bCurrent ? FSGSUITheme::SeatCurrentGlowOuterOpacity() : 0.0f;
					return Color;
				})
				.Padding(FMargin(FSGSUITheme::SeatCurrentGlowOuterWidth() * Props.LayoutScale))
			[
				SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
					.BorderBackgroundColor_Lambda([this]()
					{
						FLinearColor Color = FSGSUITheme::SeatCurrentGlowColor();
						Color.A = Props.bCurrent ? FSGSUITheme::SeatCurrentGlowInnerOpacity() : 0.0f;
						return Color;
					})
					.Padding(FMargin(FSGSUITheme::SeatCurrentGlowInnerWidth() * Props.LayoutScale))
				[
					SNew(SButton)
						.ButtonColorAndOpacity_Lambda([this]()
						{
							return SeatButtonTint(Props.bSelected, Props.bSelectable);
						})
						.ContentPadding(FMargin((Props.bViewer ? 2.0f : 3.0f) * Props.LayoutScale))
						.OnClicked(this, &SSGSTableSeatWidget::HandleClicked)
					[
						SNew(SOverlay)
							.Clipping(EWidgetClipping::ClipToBounds)
						+ SOverlay::Slot()
						[
							SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
								.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.055f, 1.0f))
						]
						+ SOverlay::Slot()
						[
							SNew(SImage)
								.Image_Lambda([this]() { return Props.FrameBrush; })
								.ColorAndOpacity_Lambda([this]()
								{
									return Props.bAlive
										? FLinearColor::White
										: FLinearColor(0.36f, 0.36f, 0.36f, 0.9f);
								})
								.Visibility(EVisibility::HitTestInvisible)
						]
						+ SOverlay::Slot()
						.Padding(TAttribute<FMargin>::CreateLambda(
							[this]() { return GetPortraitPadding(); }))
						[
							SNew(SScaleBox)
								.Stretch(EStretch::ScaleToFill)
								.StretchDirection(EStretchDirection::Both)
								.Clipping(EWidgetClipping::ClipToBounds)
							[
								SNew(SImage)
									.Image_Lambda([this]() { return Props.PortraitBrush; })
									.ColorAndOpacity_Lambda([this]()
									{
										return Props.bAlive
											? FLinearColor::White
											: FLinearColor(0.32f, 0.32f, 0.32f, 0.82f);
									})
							]
						]
						+ SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Top)
						[
							SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
								.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.82f))
								.Padding(FMargin(5.0f, 3.0f))
							[
								SNew(STextBlock)
									.Text_Lambda([this]() { return Props.NameText; })
									.ColorAndOpacity(FLinearColor::White)
									.ShadowOffset(FVector2D(1.0f, 1.0f))
							]
						]
						+ SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Bottom)
						[
							SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
								.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.88f))
								.Padding(FMargin(4.0f, 3.0f))
							[
								SNew(STextBlock)
									.Text_Lambda([this]() { return GetFooterText(); })
									.Justification(ETextJustify::Center)
									.ColorAndOpacity(FLinearColor(0.95f, 0.89f, 0.72f, 1.0f))
							]
						]
						+ SOverlay::Slot()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Bottom)
						.Padding(FMargin(
							0.0f,
							0.0f,
							FSGSUITheme::SeatHealthRightInset() * Props.LayoutScale,
							FSGSUITheme::SeatHealthBottomInset() * Props.LayoutScale))
						[
							SAssignNew(HealthHost, SBox)
						]
					]
				]
			]
		]
	];

	RebuildHealthDisplay();
}

void SSGSTableSeatWidget::SetProps(FSGSTableSeatProps InProps)
{
	const bool bHealthLayoutChanged =
		RenderedMaxHealth != InProps.MaxHealth
		|| !FMath::IsNearlyEqual(RenderedLayoutScale, InProps.LayoutScale);
	Props = MoveTemp(InProps);
	if (SizeBox.IsValid())
	{
		SizeBox->SetWidthOverride(Props.Size.X);
		SizeBox->SetHeightOverride(Props.Size.Y);
	}
	if (bHealthLayoutChanged)
	{
		RebuildHealthDisplay();
	}
	else
	{
		UpdateHealthDisplay();
	}
	Invalidate(EInvalidateWidgetReason::Layout);
}

FReply SSGSTableSeatWidget::HandleClicked() const
{
	return OnSeatClicked.IsBound()
		? OnSeatClicked.Execute(Props.SeatIndex)
		: FReply::Unhandled();
}

void SSGSTableSeatWidget::RebuildHealthDisplay()
{
	RenderedMaxHealth = Props.MaxHealth;
	RenderedLayoutScale = Props.LayoutScale;
	HealthPips.Reset();
	HealthTextBlock.Reset();
	if (!HealthHost.IsValid())
	{
		return;
	}

	if (Props.MaxHealth <= 0)
	{
		HealthHost->SetContent(SNullWidget::NullWidget);
		return;
	}

	if (Props.MaxHealth > 9)
	{
		HealthHost->SetContent(
			SAssignNew(HealthTextBlock, STextBlock)
				.ColorAndOpacity(FLinearColor::White)
				.ShadowOffset(FVector2D(1.0f, 1.0f)));
		UpdateHealthDisplay();
		return;
	}

	const float PipSize = FSGSUITheme::SeatHealthPipSize() * Props.LayoutScale;
	const float PipGap = FSGSUITheme::SeatHealthPipGap() * Props.LayoutScale;
	TSharedRef<SVerticalBox> PipColumn = SNew(SVerticalBox);
	HealthPips.Reserve(Props.MaxHealth);
	for (int32 Index = 0; Index < Props.MaxHealth; ++Index)
	{
		TSharedPtr<SImage> Pip;
		PipColumn->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, Index + 1 < Props.MaxHealth ? PipGap : 0.0f))
		[
			SNew(SBox)
				.WidthOverride(PipSize)
				.HeightOverride(PipSize)
			[
				SAssignNew(Pip, SImage)
					.Image(Props.HealthLostBrush)
			]
		];
		HealthPips.Add(Pip);
	}
	HealthHost->SetContent(PipColumn);
	UpdateHealthDisplay();
}

void SSGSTableSeatWidget::UpdateHealthDisplay()
{
	if (HealthTextBlock.IsValid())
	{
		HealthTextBlock->SetText(FText::FromString(FString::Printf(
			TEXT("%d/%d"),
			Props.Health,
			Props.MaxHealth)));
		return;
	}

	const FSlateBrush* ActiveBrush = GetActiveHealthBrush();
	for (int32 Index = 0; Index < HealthPips.Num(); ++Index)
	{
		const bool bFilled = Props.bAlive && Props.MaxHealth - Index <= Props.Health;
		HealthPips[Index]->SetImage(bFilled ? ActiveBrush : Props.HealthLostBrush);
		HealthPips[Index]->SetColorAndOpacity(FLinearColor::White);
	}
}

const FSlateBrush* SSGSTableSeatWidget::GetActiveHealthBrush() const
{
	if (Props.Health == Props.MaxHealth
		|| Props.Health > FMath::RoundToInt(static_cast<float>(Props.MaxHealth) * 0.5f))
	{
		return Props.HealthHighBrush;
	}
	if (Props.Health > FMath::FloorToInt(static_cast<float>(Props.MaxHealth) / 3.0f))
	{
		return Props.HealthMidBrush;
	}
	return Props.HealthLowBrush;
}

FMargin SSGSTableSeatWidget::GetPortraitPadding() const
{
	if (Props.FrameBrush == nullptr)
	{
		return FMargin(0.0f);
	}
	const float EdgeInset = Props.Size.Y * FSGSUITheme::SeatPortraitEdgeInsetRatio();
	return FMargin(
		Props.Size.X * FSGSUITheme::SeatPortraitLeftInsetRatio(),
		EdgeInset,
		EdgeInset,
		EdgeInset);
}

FText SSGSTableSeatWidget::GetFooterText() const
{
	FString Footer = FString::Printf(
		TEXT("Hand %d%s"),
		Props.HandCount,
		Props.bAlive ? TEXT("") : TEXT("  |  OUT"));
	if (!Props.PublicZoneText.IsEmpty())
	{
		Footer += TEXT("\n");
		Footer += Props.PublicZoneText.ToString();
	}
	return FText::FromString(MoveTemp(Footer));
}
