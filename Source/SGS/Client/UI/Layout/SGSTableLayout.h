#pragma once

#include "CoreMinimal.h"

struct SGS_API FSGSTableSeatLayout
{
	int32 SeatIndex = INDEX_NONE;
	int32 RelativePosition = INDEX_NONE;
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::ZeroVector;
};

struct SGS_API FSGSTableLayoutMetrics
{
	FVector2D ViewSize = FVector2D::ZeroVector;
	FVector2D SeatSize = FVector2D(150.0f, 196.0f);
	FVector2D MainSeatSize = FVector2D(150.0f, 196.0f);
	FMargin ScreenPadding = FMargin(12.0f);
	FMargin BottomRailPadding = FMargin(12.0f);
	FSlateRect HandArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	FSlateRect ControlArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	FSlateRect CenterArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	TArray<FSGSTableSeatLayout> Seats;

	static FSGSTableLayoutMetrics Make(FVector2D InViewSize, int32 SeatCount, int32 ViewerSeat);
	static bool RectsOverlap(const FSlateRect& A, const FSlateRect& B);

	const FSGSTableSeatLayout* FindSeat(int32 SeatIndex) const;
	FSlateRect GetSeatRect(const FSGSTableSeatLayout& Seat) const;
};
