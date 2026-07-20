#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"

enum class ESGSUIControlTone : uint8
{
	Normal,
	Current,
	Available,
	Selected
};

class SGS_API FSGSUITheme
{
public:
	static FMargin RootPadding();
	static FMargin SectionPadding();
	static FMargin ItemPadding();
	static FMargin CardPadding();
	static FMargin PromptGapPadding();
	static FMargin ButtonGapPadding();
	static FVector2D SeatButtonMinSize();
	static FVector2D CardButtonSize();
	static FVector2D ActionButtonMinSize();
	static FSlateColor ControlTint(ESGSUIControlTone Tone);
	static FLinearColor SeatCurrentGlowColor();
	static float SeatCurrentGlowOuterWidth();
	static float SeatCurrentGlowInnerWidth();
	static float SeatCurrentGlowOuterOpacity();
	static float SeatCurrentGlowInnerOpacity();
	static float SeatHealthPipSize();
	static float SeatHealthPipGap();
	static float SeatHealthRightInset();
	static float SeatHealthBottomInset();
	static FLinearColor SeatHealthHighColor();
	static FLinearColor SeatHealthMidColor();
	static FLinearColor SeatHealthLowColor();
	static FLinearColor SeatHealthLostColor();
};
