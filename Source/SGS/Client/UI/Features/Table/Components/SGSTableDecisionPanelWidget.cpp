#include "Client/UI/Features/Table/Components/SGSTableDecisionPanelWidget.h"

#include "Client/UI/Primitives/SGSUIFocusTarget.h"
#include "Client/UI/Theme/SGSUITheme.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SSGSTableDecisionPanelWidget::Construct(const FArguments& InArgs)
{
	const FSGSTableDecisionBarProps& Props = InArgs._Props;
	const FVector2D ActionButtonSize = FSGSUITheme::ActionButtonMinSize() * Props.LayoutScale;
	const float Gap = 5.0f * Props.LayoutScale;

	TSharedRef<SHorizontalBox> ActionRow = SNew(SHorizontalBox)
		.Visibility(Props.bShowActions ? EVisibility::Visible : EVisibility::Collapsed);
	if (!Props.SkillOptions.IsEmpty())
	{
		ActionRow->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("技能")))
				.ColorAndOpacity(FLinearColor(0.75f, 0.72f, 0.62f, 1.0f))
		];
	}
	for (const FSGSTableDecisionBarProps::FSkillOption& Skill : Props.SkillOptions)
	{
		const FName SkillName = Skill.SkillName;
		const FSGSOnTableSkillClicked OnSkillClicked = InArgs._OnSkillClicked;
		ActionRow->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
		[
			SNew(SButton)
				.IsEnabled(Skill.bEnabled)
				.ButtonColorAndOpacity(
					Skill.bSelected
						? FLinearColor(0.86f, 0.53f, 0.12f, 1.0f)
						: FLinearColor(0.18f, 0.34f, 0.26f, 1.0f))
				.ContentPadding(FMargin(8.0f * Props.LayoutScale, 2.0f * Props.LayoutScale))
				.OnClicked_Lambda([OnSkillClicked, SkillName]()
				{
					return OnSkillClicked.IsBound()
						? OnSkillClicked.Execute(SkillName)
						: FReply::Unhandled();
				})
			[
				SNew(STextBlock)
					.Text(Skill.Label)
					.ColorAndOpacity(FLinearColor::White)
			]
		];
	}

	ActionRow->AddSlot().FillWidth(1.0f);
	ActionRow->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
	[
		SNew(SBox)
			.MinDesiredWidth(ActionButtonSize.X)
			.MinDesiredHeight(ActionButtonSize.Y)
		[
			SNew(SSGSUIFocusTarget)
				.UIContext(Props.UIContext)
				.TargetId(TEXT("Table.Confirm"))
			[
				SNew(SButton)
					.IsEnabled(Props.bCanConfirm)
					.OnClicked(InArgs._OnConfirmClicked)
				[
					SNew(STextBlock)
						.Text(Props.ConfirmText)
						.Justification(ETextJustify::Center)
				]
			]
		]
	];
	ActionRow->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
	[
		SNew(SBox)
			.Visibility(Props.bCanPass ? EVisibility::Visible : EVisibility::Collapsed)
			.MinDesiredWidth(ActionButtonSize.X)
			.MinDesiredHeight(ActionButtonSize.Y)
		[
			SNew(SButton)
				.OnClicked(InArgs._OnPassClicked)
			[
				SNew(STextBlock)
					.Text(Props.PassText)
					.Justification(ETextJustify::Center)
			]
		]
	];

	ChildSlot
	[
		SNew(SBorder)
			.Visibility(Props.bHasPrompt ? EVisibility::Visible : EVisibility::Collapsed)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(
				Props.bIsResponse
					? FLinearColor(0.12f, 0.035f, 0.025f, 0.94f)
					: FLinearColor(0.015f, 0.02f, 0.025f, 0.88f))
			.Padding(FMargin(8.0f * Props.LayoutScale, 4.0f * Props.LayoutScale))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 3.0f * Props.LayoutScale))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.0f, 0.0f, 10.0f * Props.LayoutScale, 0.0f))
				[
					SNew(STextBlock)
						.Text(Props.TitleText)
						.ColorAndOpacity(FLinearColor(0.98f, 0.72f, 0.25f, 1.0f))
				]
				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(Props.PromptText)
						.ColorAndOpacity(FLinearColor(0.96f, 0.94f, 0.86f, 1.0f))
				]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(Props.ContextText)
						.ColorAndOpacity(FLinearColor(0.73f, 0.76f, 0.80f, 1.0f))
				]
			]
			+ SVerticalBox::Slot()
				.AutoHeight()
			[
				ActionRow
			]
		]
	];
}
