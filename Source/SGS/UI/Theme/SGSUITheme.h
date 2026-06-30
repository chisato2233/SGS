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
	static float RefreshIntervalSeconds();
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
};
