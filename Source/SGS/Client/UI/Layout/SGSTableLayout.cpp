#include "Client/UI/Layout/SGSTableLayout.h"

#include "Client/UI/Theme/SGSUITheme.h"

namespace
{
constexpr float ReferenceViewWidth = 1280.0f;
constexpr float ReferenceViewHeight = 720.0f;

float ClampCoordinate(float Value, float Extent, float ViewExtent, float SafeMargin)
{
	const float MaxValue = ViewExtent - Extent - SafeMargin;
	if (MaxValue < SafeMargin)
	{
		return FMath::Max(0.0f, (ViewExtent - Extent) * 0.5f);
	}
	return FMath::Clamp(Value, SafeMargin, MaxValue);
}

FVector2D MakePosition(
	float X,
	float Y,
	const FVector2D& Size,
	const FVector2D& ViewSize,
	float SafeMargin)
{
	return FVector2D(
		ClampCoordinate(X, Size.X, ViewSize.X, SafeMargin),
		ClampCoordinate(Y, Size.Y, ViewSize.Y, SafeMargin));
}

FVector2D MakeNovaEightPosition(
	int32 RelativePosition,
	const FSlateRect& ArenaArea,
	const FVector2D& ViewSize,
	const FVector2D& SeatSize,
	float SafeMargin)
{
	// Source reference:
	// QSgsRef/NoName/noname-for-dummies-main/layout/nova/layout.css
	// section: "位置(8人)", selectors [data-number="8"] > .player[data-position="N"].
	const FVector2D ArenaSize(
		ArenaArea.Right - ArenaArea.Left,
		ArenaArea.Bottom - ArenaArea.Top);
	switch (RelativePosition)
	{
	case 1:
	{
		const float UpperSeatY = ArenaArea.Top
			+ ArenaSize.Y * 0.10f
			- SeatSize.Y * (50.0f / 196.0f);
		const float NovaY = ArenaArea.Top
			+ ArenaSize.Y * 0.55f
			- SeatSize.Y * (143.0f / 196.0f);
		return MakePosition(
			ArenaArea.Right - SeatSize.X,
			FMath::Max(NovaY, UpperSeatY + SeatSize.Y + 4.0f),
			SeatSize,
			ViewSize,
			SafeMargin);
	}
	case 2:
		return MakePosition(
			ArenaArea.Right - SeatSize.X,
			ArenaArea.Top + ArenaSize.Y * 0.10f - SeatSize.Y * (50.0f / 196.0f),
			SeatSize,
			ViewSize,
			SafeMargin);
	case 3:
		return MakePosition(
			ArenaArea.Left + (ArenaSize.X - SeatSize.X) * 0.75f,
			ArenaArea.Top,
			SeatSize,
			ViewSize,
			SafeMargin);
	case 4:
		return MakePosition(
			ArenaArea.Left + (ArenaSize.X - SeatSize.X) * 0.50f,
			ArenaArea.Top,
			SeatSize,
			ViewSize,
			SafeMargin);
	case 5:
		return MakePosition(
			ArenaArea.Left + (ArenaSize.X - SeatSize.X) * 0.25f,
			ArenaArea.Top,
			SeatSize,
			ViewSize,
			SafeMargin);
	case 6:
		return MakePosition(
			ArenaArea.Left,
			ArenaArea.Top + ArenaSize.Y * 0.10f - SeatSize.Y * (50.0f / 196.0f),
			SeatSize,
			ViewSize,
			SafeMargin);
	case 7:
	{
		const float UpperSeatY = ArenaArea.Top
			+ ArenaSize.Y * 0.10f
			- SeatSize.Y * (50.0f / 196.0f);
		const float NovaY = ArenaArea.Top
			+ ArenaSize.Y * 0.55f
			- SeatSize.Y * (143.0f / 196.0f);
		return MakePosition(
			ArenaArea.Left,
			FMath::Max(NovaY, UpperSeatY + SeatSize.Y + 4.0f),
			SeatSize,
			ViewSize,
			SafeMargin);
	}
	default:
		return FVector2D::ZeroVector;
	}
}

FVector2D MakeFallbackRingPosition(
	int32 RelativePosition,
	int32 SeatCount,
	const FSlateRect& ArenaArea,
	const FVector2D& ViewSize,
	const FVector2D& SeatSize,
	float SafeMargin)
{
	const int32 OpponentCount = FMath::Max(SeatCount - 1, 1);
	const float T = OpponentCount > 1
		? static_cast<float>(RelativePosition - 1) / static_cast<float>(OpponentCount - 1)
		: 0.5f;
	const float X = FMath::Lerp(ArenaArea.Left, ArenaArea.Right - SeatSize.X, T);
	const float Y = RelativePosition == 1 && SeatCount <= 2
		? ArenaArea.Top
		: FMath::Lerp(
			ArenaArea.Top,
			ArenaArea.Top + (ArenaArea.Bottom - ArenaArea.Top) * 0.22f,
			FMath::Abs(0.5f - T) * 2.0f);
	return MakePosition(X, Y, SeatSize, ViewSize, SafeMargin);
}

int32 RelativeSeatPosition(int32 SeatIndex, int32 SeatCount, int32 ViewerSeat)
{
	if (SeatCount <= 0 || SeatIndex == INDEX_NONE || ViewerSeat == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	return (SeatIndex - ViewerSeat + SeatCount) % SeatCount;
}
}

FSGSTableLayoutMetrics FSGSTableLayoutMetrics::Make(FVector2D InViewSize, int32 SeatCount, int32 ViewerSeat)
{
	FSGSTableLayoutMetrics Metrics;
	Metrics.ViewSize = FVector2D(
		FMath::Max(InViewSize.X, 1.0f),
		FMath::Max(InViewSize.Y, 1.0f));
	Metrics.LayoutScale = FMath::Clamp(
		FMath::Min(
			Metrics.ViewSize.X / ReferenceViewWidth,
			Metrics.ViewSize.Y / ReferenceViewHeight),
		0.50f,
		1.25f);
	Metrics.SeatSize = FSGSUITheme::SeatButtonMinSize();
	// 本地武将是底部视觉锚点，在保持同一纵横比的前提下略大于对手。
	Metrics.MainSeatSize = FSGSUITheme::SeatButtonMinSize() * 1.10f;
	Metrics.HandCardSize = FSGSUITheme::CardButtonSize();
	Metrics.SafeMargin *= Metrics.LayoutScale;
	Metrics.SeatSize *= Metrics.LayoutScale;
	Metrics.MainSeatSize *= Metrics.LayoutScale;
	Metrics.HandCardSize *= Metrics.LayoutScale;
	Metrics.BackgroundArea = MakeBackgroundCoverRect(Metrics.ViewSize, Metrics.BackgroundImageSize);
	Metrics.ArenaArea = FSlateRect(
		Metrics.ViewSize.X * 0.03f,
		Metrics.ViewSize.Y * 0.03f,
		Metrics.ViewSize.X * 0.97f,
		Metrics.ViewSize.Y + 30.0f * Metrics.LayoutScale);

	// Nova 的 position 0 是独立的完整武将面板，底部留 40px；手牌栏从其右侧开始。
	const float MainSeatTop = Metrics.ArenaArea.Bottom - 40.0f * Metrics.LayoutScale - Metrics.MainSeatSize.Y;
	const float HandLeft = Metrics.ArenaArea.Left + Metrics.MainSeatSize.X;
	const float HandRight = FMath::Max(HandLeft, Metrics.ArenaArea.Right);
	const float HandRailHeight = Metrics.HandCardSize.Y + 14.0f * Metrics.LayoutScale;
	const float HandBottom = Metrics.ArenaArea.Bottom - 38.0f * Metrics.LayoutScale;
	const float HandTop = HandBottom - HandRailHeight;
	Metrics.HandArea = FSlateRect(
		HandLeft,
		HandTop,
		HandRight,
		HandBottom);
	const float ControlBottom = HandTop - 8.0f * Metrics.LayoutScale;
	// 统一决策面板需要一行上下文提示和一行技能/确认操作。
	const float ControlHeight = 76.0f * Metrics.LayoutScale;
	Metrics.ControlArea = FSlateRect(
		HandLeft,
		ControlBottom - ControlHeight,
		HandRight,
		ControlBottom);
	const float CenterWidth = FMath::Min(
		520.0f * Metrics.LayoutScale,
		FMath::Max(160.0f * Metrics.LayoutScale, Metrics.ViewSize.X - 2.0f * (Metrics.SeatSize.X + Metrics.SafeMargin * 2.0f)));
	const float CenterHeight = 64.0f * Metrics.LayoutScale;
	Metrics.CenterArea = FSlateRect(
		Metrics.ViewSize.X * 0.5f - CenterWidth * 0.5f,
		Metrics.ViewSize.Y * 0.5f - CenterHeight * 0.5f,
		Metrics.ViewSize.X * 0.5f + CenterWidth * 0.5f,
		Metrics.ViewSize.Y * 0.5f + CenterHeight * 0.5f);
	const FVector2D PileSize(58.0f * Metrics.LayoutScale, 82.0f * Metrics.LayoutScale);
	const FVector2D PlaySize(230.0f * Metrics.LayoutScale, 112.0f * Metrics.LayoutScale);
	const FVector2D ViewCenter = Metrics.ViewSize * 0.5f;
	Metrics.PlayArea = FSlateRect(
		ViewCenter.X - PlaySize.X * 0.5f,
		ViewCenter.Y - PlaySize.Y * 0.5f,
		ViewCenter.X + PlaySize.X * 0.5f,
		ViewCenter.Y + PlaySize.Y * 0.5f);
	Metrics.DrawPileArea = FSlateRect(
		Metrics.PlayArea.Left - 22.0f * Metrics.LayoutScale - PileSize.X,
		ViewCenter.Y - PileSize.Y * 0.5f,
		Metrics.PlayArea.Left - 22.0f * Metrics.LayoutScale,
		ViewCenter.Y + PileSize.Y * 0.5f);
	Metrics.DiscardPileArea = FSlateRect(
		Metrics.PlayArea.Right + 22.0f * Metrics.LayoutScale,
		ViewCenter.Y - PileSize.Y * 0.5f,
		Metrics.PlayArea.Right + 22.0f * Metrics.LayoutScale + PileSize.X,
		ViewCenter.Y + PileSize.Y * 0.5f);

	const int32 SafeSeatCount = FMath::Max(SeatCount, 0);
	Metrics.Seats.Reserve(SafeSeatCount);
	for (int32 SeatIndex = 0; SeatIndex < SafeSeatCount; ++SeatIndex)
	{
		FSGSTableSeatLayout SeatLayout;
		SeatLayout.SeatIndex = SeatIndex;
		SeatLayout.RelativePosition = RelativeSeatPosition(SeatIndex, SafeSeatCount, ViewerSeat);
		SeatLayout.Size = SeatLayout.RelativePosition == 0
			? Metrics.MainSeatSize
			: Metrics.SeatSize;

		if (SeatLayout.RelativePosition == 0)
		{
			SeatLayout.Position = MakePosition(
				Metrics.ArenaArea.Left,
				MainSeatTop,
				SeatLayout.Size,
				Metrics.ViewSize,
				Metrics.SafeMargin);
		}
		else if (SafeSeatCount == 8)
		{
			SeatLayout.Position = MakeNovaEightPosition(
				SeatLayout.RelativePosition,
				Metrics.ArenaArea,
				Metrics.ViewSize,
				SeatLayout.Size,
				Metrics.SafeMargin);
		}
		else
		{
			SeatLayout.Position = MakeFallbackRingPosition(
				SeatLayout.RelativePosition,
				SafeSeatCount,
				Metrics.ArenaArea,
				Metrics.ViewSize,
				SeatLayout.Size,
				Metrics.SafeMargin);
		}

		Metrics.Seats.Add(MoveTemp(SeatLayout));
	}

	return Metrics;
}

FSlateRect FSGSTableLayoutMetrics::MakeBackgroundCoverRect(FVector2D ViewSize, FVector2D ImageSize)
{
	const FVector2D SafeViewSize(
		FMath::Max(ViewSize.X, 1.0f),
		FMath::Max(ViewSize.Y, 1.0f));
	const FVector2D SafeImageSize(
		FMath::Max(ImageSize.X, 1.0f),
		FMath::Max(ImageSize.Y, 1.0f));
	const float Scale = FMath::Max(
		SafeViewSize.X / SafeImageSize.X,
		SafeViewSize.Y / SafeImageSize.Y);
	const FVector2D ScaledSize = SafeImageSize * Scale;
	const FVector2D Offset = (SafeViewSize - ScaledSize) * 0.5f;
	return FSlateRect(
		Offset.X,
		Offset.Y,
		Offset.X + ScaledSize.X,
		Offset.Y + ScaledSize.Y);
}

bool FSGSTableLayoutMetrics::RectsOverlap(const FSlateRect& A, const FSlateRect& B)
{
	return A.Left < B.Right
		&& A.Right > B.Left
		&& A.Top < B.Bottom
		&& A.Bottom > B.Top;
}

const FSGSTableSeatLayout* FSGSTableLayoutMetrics::FindSeat(int32 SeatIndex) const
{
	return Seats.FindByPredicate(
		[SeatIndex](const FSGSTableSeatLayout& Seat)
		{
			return Seat.SeatIndex == SeatIndex;
		});
}

FSlateRect FSGSTableLayoutMetrics::GetSeatRect(const FSGSTableSeatLayout& Seat) const
{
	return FSlateRect(
		Seat.Position.X,
		Seat.Position.Y,
		Seat.Position.X + Seat.Size.X,
		Seat.Position.Y + Seat.Size.Y);
}

FSlateRect FSGSTableLayoutMetrics::GetSeatRect(int32 SeatIndex) const
{
	if (const FSGSTableSeatLayout* Seat = FindSeat(SeatIndex))
	{
		return GetSeatRect(*Seat);
	}
	return FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
}
