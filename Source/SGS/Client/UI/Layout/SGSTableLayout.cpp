#include "Client/UI/Layout/SGSTableLayout.h"

namespace
{
float ClampX(float X, float Width, float ViewWidth, float Padding)
{
	return FMath::Clamp(X, Padding, FMath::Max(Padding, ViewWidth - Width - Padding));
}

float ClampY(float Y, float Height, float ViewHeight, float Padding)
{
	return FMath::Clamp(Y, Padding, FMath::Max(Padding, ViewHeight - Height - Padding));
}

FVector2D MakePosition(float X, float Y, const FVector2D& Size, const FVector2D& ViewSize, float Padding)
{
	return FVector2D(
		ClampX(X, Size.X, ViewSize.X, Padding),
		ClampY(Y, Size.Y, ViewSize.Y, Padding));
}

FVector2D MakeNovaEightPosition(int32 RelativePosition, const FVector2D& ViewSize, const FVector2D& SeatSize, float Padding)
{
	switch (RelativePosition)
	{
	case 1:
		return MakePosition(ViewSize.X - 150.0f, ViewSize.Y * 0.55f - 143.0f, SeatSize, ViewSize, Padding);
	case 2:
		return MakePosition(ViewSize.X - 150.0f, ViewSize.Y * 0.10f - 50.0f, SeatSize, ViewSize, Padding);
	case 3:
		return MakePosition(ViewSize.X * 0.75f - 112.5f, 0.0f, SeatSize, ViewSize, Padding);
	case 4:
		return MakePosition(ViewSize.X * 0.50f - 75.0f, 0.0f, SeatSize, ViewSize, Padding);
	case 5:
		return MakePosition(ViewSize.X * 0.25f - 37.5f, 0.0f, SeatSize, ViewSize, Padding);
	case 6:
		return MakePosition(0.0f, ViewSize.Y * 0.10f - 50.0f, SeatSize, ViewSize, Padding);
	case 7:
		return MakePosition(0.0f, ViewSize.Y * 0.55f - 143.0f, SeatSize, ViewSize, Padding);
	default:
		return FVector2D::ZeroVector;
	}
}

FVector2D MakeFallbackRingPosition(int32 RelativePosition, int32 SeatCount, const FVector2D& ViewSize, const FVector2D& SeatSize, float Padding)
{
	const int32 OpponentCount = FMath::Max(SeatCount - 1, 1);
	const float T = OpponentCount > 1
		? static_cast<float>(RelativePosition - 1) / static_cast<float>(OpponentCount - 1)
		: 0.5f;
	const float X = FMath::Lerp(Padding, ViewSize.X - SeatSize.X - Padding, T);
	const float Y = RelativePosition == 1 && SeatCount <= 2
		? Padding
		: FMath::Lerp(Padding, ViewSize.Y * 0.22f, FMath::Abs(0.5f - T) * 2.0f);
	return MakePosition(X, Y, SeatSize, ViewSize, Padding);
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
		FMath::Max(InViewSize.X, 800.0f),
		FMath::Max(InViewSize.Y, 540.0f));

	const float Padding = Metrics.ScreenPadding.Left;
	const float BottomRailHeight = 180.0f;
	const float MainSeatTop = Metrics.ViewSize.Y - Metrics.MainSeatSize.Y - Metrics.BottomRailPadding.Bottom;
	const float HandLeft = Metrics.MainSeatSize.X + 24.0f;
	const float HandRight = Metrics.ViewSize.X - 24.0f;
	const float HandTop = Metrics.ViewSize.Y - 136.0f;
	Metrics.HandArea = FSlateRect(
		HandLeft,
		HandTop,
		FMath::Max(HandLeft + 120.0f, HandRight),
		Metrics.ViewSize.Y - 16.0f);
	Metrics.ControlArea = FSlateRect(
		HandLeft,
		FMath::Max(220.0f, Metrics.ViewSize.Y - BottomRailHeight - 56.0f),
		FMath::Max(HandLeft + 120.0f, HandRight),
		FMath::Max(260.0f, Metrics.ViewSize.Y - BottomRailHeight - 8.0f));
	Metrics.CenterArea = FSlateRect(
		Metrics.MainSeatSize.X + 32.0f,
		Metrics.SeatSize.Y + 16.0f,
		Metrics.ViewSize.X - Metrics.SeatSize.X - 32.0f,
		Metrics.ViewSize.Y - BottomRailHeight - 72.0f);

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
			SeatLayout.Position = MakePosition(Padding, MainSeatTop, SeatLayout.Size, Metrics.ViewSize, Padding);
		}
		else if (SafeSeatCount == 8)
		{
			SeatLayout.Position = MakeNovaEightPosition(SeatLayout.RelativePosition, Metrics.ViewSize, SeatLayout.Size, Padding);
		}
		else
		{
			SeatLayout.Position = MakeFallbackRingPosition(SeatLayout.RelativePosition, SafeSeatCount, Metrics.ViewSize, SeatLayout.Size, Padding);
		}

		Metrics.Seats.Add(MoveTemp(SeatLayout));
	}

	return Metrics;
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
