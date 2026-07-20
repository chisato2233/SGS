#include "Client/UI/Features/Table/Components/SGSTableCardMotionWidget.h"

#include "Math/TransformCalculus2D.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderTransform.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"

namespace
{
const FName InitialDealReason(TEXT("SGS.CardMove.InitialDeal"));
const FName DrawReason(TEXT("SGS.CardMove.Draw"));
const FName RewardDrawReason(TEXT("SGS.CardMove.RewardDraw"));
const FName UseReason(TEXT("SGS.CardMove.Use"));
const FName RespondReason(TEXT("SGS.CardMove.Respond"));
const FName DiscardReason(TEXT("SGS.CardMove.Discard"));

constexpr float DrawDuration = 0.55f;
constexpr float DrawCardStagger = 0.06f;
constexpr float OpeningSeatBatchStagger = 0.12f;
constexpr float PlayTravelDuration = 0.32f;
constexpr float PlayHoldDuration = 0.45f;
constexpr float PileTravelDuration = 0.22f;
constexpr float DiscardTravelDuration = 0.35f;
constexpr float DiscardHoldDuration = 0.18f;

FVector2D RectCenter(const FSlateRect& Rect)
{
	return FVector2D((Rect.Left + Rect.Right) * 0.5f, (Rect.Top + Rect.Bottom) * 0.5f);
}

float Ease(float Alpha)
{
	return FMath::InterpEaseInOut(0.0f, 1.0f, FMath::Clamp(Alpha, 0.0f, 1.0f), 2.0f);
}

bool IsDrawReason(FName Reason)
{
	return Reason == InitialDealReason || Reason == DrawReason || Reason == RewardDrawReason;
}

FVector2D SeatOrHandCenter(const FSGSTableMotionProps& Props, int32 SeatIndex)
{
	if (SeatIndex == Props.ViewerSeat)
	{
		return RectCenter(Props.Layout.HandArea);
	}
	return RectCenter(Props.Layout.GetSeatRect(SeatIndex));
}

float CueDuration(const FSGSTableCardMotionCueViewData& Cue)
{
	if (Cue.bCleanup)
	{
		return 0.001f;
	}
	if (IsDrawReason(Cue.Reason))
	{
		return DrawDuration + DrawCardStagger * FMath::Max(Cue.CardCount - 1, 0);
	}
	if (Cue.Reason == UseReason || Cue.Reason == RespondReason)
	{
		return PlayTravelDuration + PlayHoldDuration + PileTravelDuration;
	}
	return DiscardTravelDuration + DiscardHoldDuration + PileTravelDuration;
}
}

struct FSGSActiveTableMotion
{
	FSGSTableMotionCueProps Props;
	TArray<TSharedPtr<SBox>> Cards;
	TArray<TSharedPtr<SImage>> CardImages;
	float StartDelay = 0.0f;
};

struct FSGSTableCardMotionWidgetModel
{
	FSGSTableMotionProps Props;
	TSharedPtr<SConstraintCanvas> Canvas;
	TArray<FSGSActiveTableMotion> Active;
	TSharedPtr<FActiveTimerHandle> ActiveTimerHandle;
	double BatchStartTime = -1.0;
	double LastCurrentTime = -1.0;
	int32 BatchAcknowledgeSequence = INDEX_NONE;
	int32 LastAcknowledgedSequence = INDEX_NONE;
};

SSGSTableCardMotionWidget::SSGSTableCardMotionWidget()
	: Model(MakeUnique<FSGSTableCardMotionWidgetModel>())
{
}

SSGSTableCardMotionWidget::~SSGSTableCardMotionWidget()
{
	ResetMotion();
}

void SSGSTableCardMotionWidget::Construct(const FArguments& InArgs)
{
	OnCueFinished = InArgs._OnCueFinished;
	Model->Props = InArgs._Props;
	Model->LastAcknowledgedSequence = INDEX_NONE;
	ChildSlot
	[
		SAssignNew(Model->Canvas, SConstraintCanvas)
			.Clipping(EWidgetClipping::ClipToBounds)
	];
	SetVisibility(EVisibility::HitTestInvisible);
	ScheduleNextBatch();
}

void SSGSTableCardMotionWidget::SetProps(FSGSTableMotionProps InProps)
{
	const bool bEpochChanged = Model->Props.MotionEpoch != InProps.MotionEpoch;
	if (bEpochChanged)
	{
		ResetMotion();
		Model->LastAcknowledgedSequence = INDEX_NONE;
	}
	Model->Props = MoveTemp(InProps);
	for (FSGSActiveTableMotion& Active : Model->Active)
	{
		for (const TSharedPtr<SBox>& Card : Active.Cards)
		{
			Card->SetWidthOverride(Model->Props.CardSize.X);
			Card->SetHeightOverride(Model->Props.CardSize.Y);
		}
		if (const FSGSTableMotionCueProps* Updated = Model->Props.PendingCues.FindByPredicate(
			[Sequence = Active.Props.Cue.Sequence](const FSGSTableMotionCueProps& Candidate)
			{
				return Candidate.Cue.Sequence == Sequence;
			}))
		{
			Active.Props = *Updated;
			for (int32 CardIndex = 0; CardIndex < Active.CardImages.Num(); ++CardIndex)
			{
				const FSlateBrush* Brush = Updated->VisibleCardBrushes.IsValidIndex(CardIndex)
					? Updated->VisibleCardBrushes[CardIndex]
					: Model->Props.CardBackBrush;
				Active.CardImages[CardIndex]->SetImage(
					Brush != nullptr ? Brush : FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));
			}
		}
	}

	if (!Model->Active.IsEmpty())
	{
		const bool bActiveStillPending = Model->Props.PendingCues.ContainsByPredicate(
			[this](const FSGSTableMotionCueProps& Candidate)
			{
				return Model->Active.ContainsByPredicate([Sequence = Candidate.Cue.Sequence](const FSGSActiveTableMotion& Active)
				{
					return Active.Props.Cue.Sequence == Sequence;
				});
			});
		if (!bActiveStillPending)
		{
			ResetMotion();
		}
	}
	ScheduleNextBatch();
}

void SSGSTableCardMotionWidget::ResetMotion()
{
	if (Model->ActiveTimerHandle.IsValid())
	{
		UnRegisterActiveTimer(Model->ActiveTimerHandle.ToSharedRef());
		Model->ActiveTimerHandle.Reset();
	}
	if (Model->Canvas.IsValid())
	{
		Model->Canvas->ClearChildren();
	}
	Model->Active.Reset();
	Model->BatchStartTime = -1.0;
	Model->LastCurrentTime = -1.0;
	Model->BatchAcknowledgeSequence = INDEX_NONE;
}

void SSGSTableCardMotionWidget::ScheduleNextBatch()
{
	if (!Model->Active.IsEmpty() || !Model->Canvas.IsValid())
	{
		return;
	}

	TArray<const FSGSTableMotionCueProps*> Batch;
	for (const FSGSTableMotionCueProps& Candidate : Model->Props.PendingCues)
	{
		if (Candidate.Cue.Sequence <= Model->LastAcknowledgedSequence)
		{
			continue;
		}
		if (Batch.IsEmpty())
		{
			Batch.Add(&Candidate);
			if (Candidate.Cue.Reason != InitialDealReason)
			{
				break;
			}
			continue;
		}
		if (Batch[0]->Cue.Reason == InitialDealReason && Candidate.Cue.Reason == InitialDealReason)
		{
			Batch.Add(&Candidate);
		}
		else
		{
			break;
		}
	}
	if (Batch.IsEmpty())
	{
		return;
	}

	const int32 SeatCount = Model->Props.Layout.Seats.Num();
	for (const FSGSTableMotionCueProps* CueProps : Batch)
	{
		FSGSActiveTableMotion& Active = Model->Active.AddDefaulted_GetRef();
		Active.Props = *CueProps;
		if (CueProps->Cue.Reason == InitialDealReason && SeatCount > 0)
		{
			const int32 Relative = (CueProps->Cue.ToSeat - Model->Props.ViewerSeat + SeatCount) % SeatCount;
			Active.StartDelay = FMath::Min(Relative, SeatCount - Relative) * OpeningSeatBatchStagger;
		}
		Model->BatchAcknowledgeSequence = FMath::Max(
			Model->BatchAcknowledgeSequence,
			CueProps->Cue.Sequence);

		if (!CueProps->Cue.bCleanup)
		{
			for (int32 CardIndex = 0; CardIndex < FMath::Max(CueProps->Cue.CardCount, 1); ++CardIndex)
			{
				const FSlateBrush* Brush = CueProps->VisibleCardBrushes.IsValidIndex(CardIndex)
					? CueProps->VisibleCardBrushes[CardIndex]
					: Model->Props.CardBackBrush;
				TSharedPtr<SBox> Card;
				TSharedPtr<SImage> CardImage;
				Model->Canvas->AddSlot()
					.Anchors(FAnchors(0.0f, 0.0f))
					.Alignment(FVector2D::ZeroVector)
					.Offset(FMargin(0.0f))
					.AutoSize(true)
				[
					SAssignNew(Card, SBox)
						.WidthOverride(Model->Props.CardSize.X)
						.HeightOverride(Model->Props.CardSize.Y)
					[
						SAssignNew(CardImage, SImage)
							.Image(Brush != nullptr ? Brush : FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
							.ColorAndOpacity(Brush != nullptr ? FLinearColor::White : FLinearColor(0.12f, 0.14f, 0.18f, 1.0f))
					]
				];
				Card->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
				Active.Cards.Add(MoveTemp(Card));
				Active.CardImages.Add(MoveTemp(CardImage));
			}
		}
	}

	Model->BatchStartTime = -1.0;
	EnsureActiveTimer();
}

void SSGSTableCardMotionWidget::EnsureActiveTimer()
{
	if (!Model->ActiveTimerHandle.IsValid() && !Model->Active.IsEmpty())
	{
		Model->ActiveTimerHandle = RegisterActiveTimer(
			0.0f,
			FWidgetActiveTimerDelegate::CreateSP(this, &SSGSTableCardMotionWidget::UpdateMotion));
	}
}

EActiveTimerReturnType SSGSTableCardMotionWidget::UpdateMotion(double CurrentTime, float DeltaTime)
{
	(void)DeltaTime;
	if (Model->BatchStartTime < 0.0)
	{
		Model->BatchStartTime = CurrentTime;
	}
	Model->LastCurrentTime = CurrentTime;

	TArray<int32> FinishedIndices;
	for (int32 ActiveIndex = 0; ActiveIndex < Model->Active.Num(); ++ActiveIndex)
	{
		FSGSActiveTableMotion& Active = Model->Active[ActiveIndex];
		const FSGSTableCardMotionCueViewData& Cue = Active.Props.Cue;
		const float Elapsed = static_cast<float>(CurrentTime - Model->BatchStartTime) - Active.StartDelay;
		if (Elapsed < 0.0f)
		{
			for (const TSharedPtr<SBox>& Card : Active.Cards)
			{
				Card->SetRenderOpacity(0.0f);
			}
			continue;
		}

		const FVector2D CardHalf = Model->Props.CardSize * 0.5f;
		const FVector2D Source = IsDrawReason(Cue.Reason)
			? RectCenter(Model->Props.Layout.DrawPileArea)
			: SeatOrHandCenter(Model->Props, Cue.FromSeat);
		const FVector2D PlayCenter = RectCenter(Model->Props.Layout.PlayArea);
		const FVector2D PileCenter = RectCenter(Model->Props.Layout.DiscardPileArea);
		const FVector2D DrawTarget = SeatOrHandCenter(Model->Props, Cue.ToSeat);

		for (int32 CardIndex = 0; CardIndex < Active.Cards.Num(); ++CardIndex)
		{
			const float CardElapsed = IsDrawReason(Cue.Reason)
				? Elapsed - CardIndex * DrawCardStagger
				: Elapsed;
			const float FanOffset = (CardIndex - (Active.Cards.Num() - 1) * 0.5f) * 18.0f * Model->Props.Layout.LayoutScale;
			FVector2D Position = Source;
			if (IsDrawReason(Cue.Reason))
			{
				Position = FMath::Lerp(Source, DrawTarget + FVector2D(FanOffset * 0.35f, 0.0f), Ease(CardElapsed / DrawDuration));
			}
			else if (Cue.Reason == UseReason || Cue.Reason == RespondReason)
			{
				if (CardElapsed < PlayTravelDuration)
				{
					Position = FMath::Lerp(Source, PlayCenter + FVector2D(FanOffset, 0.0f), Ease(CardElapsed / PlayTravelDuration));
				}
				else if (CardElapsed < PlayTravelDuration + PlayHoldDuration)
				{
					Position = PlayCenter + FVector2D(FanOffset, 0.0f);
				}
				else
				{
					const float PileAlpha = (CardElapsed - PlayTravelDuration - PlayHoldDuration) / PileTravelDuration;
					Position = FMath::Lerp(PlayCenter + FVector2D(FanOffset, 0.0f), PileCenter, Ease(PileAlpha));
				}
			}
			else
			{
				const FVector2D FanCenter = PlayCenter + FVector2D(FanOffset, 12.0f * Model->Props.Layout.LayoutScale);
				if (CardElapsed < DiscardTravelDuration)
				{
					Position = FMath::Lerp(Source, FanCenter, Ease(CardElapsed / DiscardTravelDuration));
				}
				else if (CardElapsed < DiscardTravelDuration + DiscardHoldDuration)
				{
					Position = FanCenter;
				}
				else
				{
					const float PileAlpha = (CardElapsed - DiscardTravelDuration - DiscardHoldDuration) / PileTravelDuration;
					Position = FMath::Lerp(FanCenter, PileCenter, Ease(PileAlpha));
				}
			}
			const float Pulse = 0.96f + 0.04f * FMath::Sin(FMath::Clamp(CardElapsed, 0.0f, 0.5f) * UE_PI * 2.0f);
			Active.Cards[CardIndex]->SetRenderOpacity(CardElapsed >= 0.0f ? 1.0f : 0.0f);
			Active.Cards[CardIndex]->SetRenderTransform(::Concatenate(
				FScale2D(Pulse),
				Position - CardHalf));
		}

		if (Elapsed >= CueDuration(Cue))
		{
			FinishedIndices.Add(ActiveIndex);
		}
	}

	for (int32 Index = FinishedIndices.Num() - 1; Index >= 0; --Index)
	{
		FSGSActiveTableMotion& Active = Model->Active[FinishedIndices[Index]];
		for (const TSharedPtr<SBox>& Card : Active.Cards)
		{
			Model->Canvas->RemoveSlot(Card.ToSharedRef());
		}
		Model->Active.RemoveAt(FinishedIndices[Index]);
	}

	if (Model->Active.IsEmpty())
	{
		const int32 CompletedSequence = Model->BatchAcknowledgeSequence;
		Model->LastAcknowledgedSequence = FMath::Max(Model->LastAcknowledgedSequence, CompletedSequence);
		Model->BatchAcknowledgeSequence = INDEX_NONE;
		Model->BatchStartTime = -1.0;
		if (OnCueFinished.IsBound() && CompletedSequence != INDEX_NONE)
		{
			OnCueFinished.Execute(CompletedSequence);
		}
		ScheduleNextBatch();
		if (Model->Active.IsEmpty())
		{
			Model->ActiveTimerHandle.Reset();
			return EActiveTimerReturnType::Stop;
		}
		return EActiveTimerReturnType::Continue;
	}

	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

int32 SSGSTableCardMotionWidget::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 BaseLayer = SCompoundWidget::OnPaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);
	int32 ResultLayer = BaseLayer;
	for (const FSGSActiveTableMotion& Active : Model->Active)
	{
		const FSGSTableCardMotionCueViewData& Cue = Active.Props.Cue;
		if (Cue.Reason != UseReason && Cue.Reason != RespondReason)
		{
			continue;
		}
		const float Elapsed = Model->BatchStartTime >= 0.0 && Model->LastCurrentTime >= 0.0
			? static_cast<float>(Model->LastCurrentTime - Model->BatchStartTime) - Active.StartDelay
			: -1.0f;
		if (Elapsed < 0.0f || Elapsed > PlayTravelDuration + PlayHoldDuration)
		{
			continue;
		}

		const FVector2D Source = SeatOrHandCenter(Model->Props, Cue.FromSeat);
		for (const int32 TargetSeat : Cue.RelatedTargetSeatIndices)
		{
			const FSlateRect TargetRect = Model->Props.Layout.GetSeatRect(TargetSeat);
			const FVector2D Target = RectCenter(TargetRect);
			TArray<FVector2f> Points;
			Points.Add(FVector2f(Source));
			Points.Add(FVector2f(Target));
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				++ResultLayer,
				AllottedGeometry.ToPaintGeometry(),
				Points,
				ESlateDrawEffect::None,
				Cue.Reason == RespondReason
					? FLinearColor(0.25f, 0.72f, 1.0f, 0.85f)
					: FLinearColor(1.0f, 0.26f, 0.12f, 0.85f),
				true,
				3.0f * Model->Props.Layout.LayoutScale);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++ResultLayer,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(TargetRect.Right - TargetRect.Left, TargetRect.Bottom - TargetRect.Top),
					FSlateLayoutTransform(FVector2D(TargetRect.Left, TargetRect.Top))),
				FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
				ESlateDrawEffect::None,
				FLinearColor(1.0f, 0.25f, 0.08f, 0.12f));
		}
	}
	return ResultLayer;
}
