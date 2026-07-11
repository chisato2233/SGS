#pragma once

#include "CoreMinimal.h"

struct SGS_API FSGSTableSeatLayout
{
	int32 SeatIndex = INDEX_NONE;
	int32 RelativePosition = INDEX_NONE;
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::ZeroVector;
};

// 牌桌 HUD 的纯布局结果。坐标直接使用当前 Slate viewport 尺寸；
// 背景只保持图片比例做 cover/crop，座位按 NoName Nova 的百分比公式排布。
// 这里不保存任何规则事实，调用方只能把 ViewModel 渲染进这些矩形。
struct SGS_API FSGSTableLayoutMetrics
{
	FVector2D ViewSize = FVector2D::ZeroVector;
	float LayoutScale = 1.0f;
	float SafeMargin = 12.0f;
	FVector2D SeatSize = FVector2D::ZeroVector;
	FVector2D MainSeatSize = FVector2D::ZeroVector;
	FVector2D HandCardSize = FVector2D::ZeroVector;
	FVector2D BackgroundImageSize = FVector2D(1334.0f, 750.0f);
	FSlateRect BackgroundArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	FSlateRect ArenaArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	FSlateRect HandArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	FSlateRect ControlArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	FSlateRect CenterArea = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	TArray<FSGSTableSeatLayout> Seats;

	static FSGSTableLayoutMetrics Make(FVector2D InViewSize, int32 SeatCount, int32 ViewerSeat);
	static FSlateRect MakeBackgroundCoverRect(FVector2D ViewSize, FVector2D ImageSize);
	static bool RectsOverlap(const FSlateRect& A, const FSlateRect& B);

	const FSGSTableSeatLayout* FindSeat(int32 SeatIndex) const;
	FSlateRect GetSeatRect(const FSGSTableSeatLayout& Seat) const;
};
