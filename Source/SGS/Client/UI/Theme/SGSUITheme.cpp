#include "Client/UI/Theme/SGSUITheme.h"

FMargin FSGSUITheme::RootPadding()
{
	return FMargin(10.0f);
}

FMargin FSGSUITheme::SectionPadding()
{
	return FMargin(6.0f);
}

FMargin FSGSUITheme::ItemPadding()
{
	return FMargin(3.0f);
}

FMargin FSGSUITheme::CardPadding()
{
	return FMargin(4.0f);
}

FMargin FSGSUITheme::PromptGapPadding()
{
	return FMargin(0.0f, 0.0f, 0.0f, 6.0f);
}

FMargin FSGSUITheme::ButtonGapPadding()
{
	return FMargin(0.0f, 0.0f, 6.0f, 0.0f);
}

FVector2D FSGSUITheme::SeatButtonMinSize()
{
	return FVector2D(168.0f, 232.0f);
}

FVector2D FSGSUITheme::CardButtonSize()
{
	return FVector2D(104.0f, 132.0f);
}

FVector2D FSGSUITheme::ActionButtonMinSize()
{
	return FVector2D(96.0f, 32.0f);
}

FSlateColor FSGSUITheme::ControlTint(ESGSUIControlTone Tone)
{
	switch (Tone)
	{
	case ESGSUIControlTone::Selected:
		return FSlateColor(FLinearColor(0.28f, 0.54f, 0.86f, 1.0f));
	case ESGSUIControlTone::Available:
		return FSlateColor(FLinearColor(0.26f, 0.62f, 0.43f, 1.0f));
	case ESGSUIControlTone::Current:
		return FSlateColor(FLinearColor(0.78f, 0.67f, 0.34f, 1.0f));
	case ESGSUIControlTone::Normal:
	default:
		return FSlateColor(FLinearColor(0.10f, 0.11f, 0.12f, 0.85f));
	}
}
