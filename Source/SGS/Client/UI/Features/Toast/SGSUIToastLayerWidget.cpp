#include "Client/UI/Features/Toast/SGSUIToastLayerWidget.h"

#include "Client/UI/Core/Context/SGSUIContext.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
FLinearColor ToastColor(ESGSUIToastTone Tone)
{
	switch (Tone)
	{
	case ESGSUIToastTone::Success:
		return FLinearColor(0.06f, 0.30f, 0.16f, 0.94f);
	case ESGSUIToastTone::Warning:
		return FLinearColor(0.42f, 0.25f, 0.04f, 0.94f);
	case ESGSUIToastTone::Error:
		return FLinearColor(0.42f, 0.06f, 0.06f, 0.94f);
	default:
		return FLinearColor(0.04f, 0.06f, 0.09f, 0.92f);
	}
}
}

SSGSUIToastLayerWidget::SSGSUIToastLayerWidget()
	: Lifetime(TEXT("ToastLayer"))
{
}

SSGSUIToastLayerWidget::~SSGSUIToastLayerWidget()
{
	Lifetime.Reset();
}

void SSGSUIToastLayerWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(24.0f, 48.0f))
	[
		SNullWidget::NullWidget
	];

	if (InArgs._UIContext.IsValid())
	{
		TWeakPtr<SSGSUIToastLayerWidget> WeakOwner = SharedThis(this);
		InArgs._UIContext->Signals().Toast().SubscribeWeak<SSGSUIToastLayerWidget>(
			Lifetime,
			WeakOwner,
			[](SSGSUIToastLayerWidget& Owner, const FSGSUIToastRequest& Request)
			{
				Owner.HandleToast(Request);
			});
	}
}

void SSGSUIToastLayerWidget::HandleToast(const FSGSUIToastRequest& Request)
{
	if (Request.Message.IsEmpty())
	{
		return;
	}

	ExpiryTime = FPlatformTime::Seconds() + FMath::Max(0.25f, Request.DurationSeconds);
	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(24.0f, 48.0f))
	[
		SNew(SBox)
			.MaxDesiredWidth(620.0f)
		[
			SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(ToastColor(Request.Tone))
				.Padding(FMargin(18.0f, 10.0f))
			[
				SNew(STextBlock)
					.Text(Request.Message)
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FLinearColor::White)
			]
		]
	];
	RegisterActiveTimer(
		0.1f,
		FWidgetActiveTimerDelegate::CreateSP(this, &SSGSUIToastLayerWidget::HandleExpiryTimer));
}

EActiveTimerReturnType SSGSUIToastLayerWidget::HandleExpiryTimer(double CurrentTime, float DeltaTime)
{
	if (FPlatformTime::Seconds() < ExpiryTime)
	{
		return EActiveTimerReturnType::Continue;
	}

	ChildSlot
	[
		SNullWidget::NullWidget
	];
	return EActiveTimerReturnType::Stop;
}
