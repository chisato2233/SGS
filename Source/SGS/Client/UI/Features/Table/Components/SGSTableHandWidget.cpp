#include "Client/UI/Features/Table/Components/SGSTableHandWidget.h"

#include "Animation/CurveSequence.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/DragAndDrop.h"
#include "InputCoreTypes.h"
#include "Math/TransformCalculus2D.h"
#include "Rendering/SlateRenderTransform.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
constexpr float DragSmoothingTime = 0.04f;
constexpr float NeighborSmoothingTime = 0.10f;
constexpr float NeighborDampingRatio = 0.90f;
constexpr float ReturnDuration = 0.18f;
constexpr float MaxSimulationStep = 1.0f / 120.0f;
constexpr float MaxSimulationTime = 0.10f;
constexpr float PositionTolerance = 0.20f;
constexpr float VelocityTolerance = 0.50f;
constexpr float ScaleTolerance = 0.001f;
constexpr float RotationTolerance = 0.05f;
constexpr float SelectedScale = 1.06f;
constexpr float HoverScale = 1.09f;
constexpr float DragScale = 1.10f;

FSlateColor CardTint(const FSGSTableCardProps& Props)
{
	if (Props.bSelected)
	{
		return FLinearColor(0.93f, 0.68f, 0.18f, 1.0f);
	}
	if (Props.bSelectable)
	{
		return FLinearColor(0.13f, 0.14f, 0.15f, 1.0f);
	}
	return FLinearColor(0.13f, 0.14f, 0.15f, 1.0f);
}

bool SameCardProps(const FSGSTableCardProps& A, const FSGSTableCardProps& B)
{
	return A.CardId == B.CardId
		&& A.CornerText.IdenticalTo(B.CornerText)
		&& A.FooterText.IdenticalTo(B.FooterText)
		&& A.Size.Equals(B.Size)
		&& A.FaceBrush == B.FaceBrush
		&& A.bSelectable == B.bSelectable
		&& A.bSelected == B.bSelected
		&& A.bDimmed == B.bDimmed;
}

bool SameHandProps(const FSGSTableHandProps& A, const FSGSTableHandProps& B)
{
	if (!A.CardSize.Equals(B.CardSize)
		|| !FMath::IsNearlyEqual(A.LayoutScale, B.LayoutScale)
		|| !FMath::IsNearlyEqual(A.AvailableWidth, B.AvailableWidth)
		|| A.Cards.Num() != B.Cards.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < A.Cards.Num(); ++Index)
	{
		if (!SameCardProps(A.Cards[Index], B.Cards[Index]))
		{
			return false;
		}
	}
	return true;
}

struct FSGSHandCardEntry
{
	FSGSTableCardProps Props;
	TSharedPtr<SSGSTableCardWidget> Widget;
	SConstraintCanvas::FSlot* Slot = nullptr;
	FVector2D BasePosition = FVector2D::ZeroVector;
	FVector2D CurrentTranslation = FVector2D::ZeroVector;
	FVector2D TargetTranslation = FVector2D::ZeroVector;
	FVector2D TranslationVelocity = FVector2D::ZeroVector;
	float CurrentScale = 1.0f;
	float TargetScale = 1.0f;
	float ScaleVelocity = 0.0f;
	float CurrentRotation = 0.0f;
	float TargetRotation = 0.0f;
	float RotationVelocity = 0.0f;
	float ReturnStartScale = 1.0f;
	float ReturnStartRotation = 0.0f;
	TSharedPtr<FCurveSequence> ReturnSequence;
	FCurveHandle ReturnCurve;
	bool bHovered = false;
	bool bHasBasePosition = false;
};
}

struct FSGSTableHandWidgetModel
{
	FSGSTableHandProps Props;
	TSharedPtr<SBorder> RootBorder;
	TSharedPtr<SScrollBox> HandScroll;
	SScrollBox::FSlot* ScrollSlot = nullptr;
	TSharedPtr<SBox> CanvasBox;
	TSharedPtr<SConstraintCanvas> HandCanvas;
	TSharedPtr<STextBlock> EmptyText;
	TMap<int32, TSharedPtr<FSGSHandCardEntry>> Entries;
	TArray<int32> CommittedOrder;
	TArray<int32> PreviewOrder;
	TSharedPtr<FActiveTimerHandle> ActiveTimerHandle;
	TWeakPtr<FSGSCardDragDropOperation> DragOperation;
	int32 DraggedCardId = INDEX_NONE;
	FVector2D PointerScreenPosition = FVector2D::ZeroVector;
	FVector2D PointerCanvasPosition = FVector2D::ZeroVector;
	FVector2D PointerDelta = FVector2D::ZeroVector;
	FVector2D GrabOffset = FVector2D::ZeroVector;
	float InnerPadding = 0.0f;
	float CardLift = 0.0f;
	float HoverLift = 0.0f;
	float Stride = 0.0f;
	float StartOffset = 0.0f;
	float InnerWidth = 0.0f;
	float LastInsertionChangeX = 0.0f;
	bool bPointerInsideHand = false;
	bool bHasProps = false;
};

namespace
{
FVector2D BasePositionForIndex(const FSGSTableHandWidgetModel& Model, int32 Index)
{
	return FVector2D(Model.StartOffset + Model.Stride * Index, Model.CardLift);
}

void ApplyRenderTransform(FSGSHandCardEntry& Entry)
{
	if (!Entry.Widget.IsValid())
	{
		return;
	}

	if (Entry.CurrentTranslation.IsNearlyZero(PositionTolerance)
		&& FMath::IsNearlyEqual(Entry.CurrentScale, 1.0f, ScaleTolerance)
		&& FMath::IsNearlyZero(Entry.CurrentRotation, RotationTolerance))
	{
		Entry.Widget->SetRenderTransform(TOptional<FSlateRenderTransform>());
		return;
	}

	const FSlateRenderTransform RenderTransform = ::Concatenate(
		FScale2D(Entry.CurrentScale),
		FQuat2D(FMath::DegreesToRadians(Entry.CurrentRotation)),
		Entry.CurrentTranslation);
	Entry.Widget->SetRenderTransform(RenderTransform);
}

void RefreshZOrders(FSGSTableHandWidgetModel& Model)
{
	for (const TPair<int32, TSharedPtr<FSGSHandCardEntry>>& Pair : Model.Entries)
	{
		const TSharedPtr<FSGSHandCardEntry>& Entry = Pair.Value;
		if (!Entry.IsValid() || Entry->Slot == nullptr)
		{
			continue;
		}

		const int32 VisualIndex = Model.CommittedOrder.IndexOfByKey(Pair.Key);
		const float DesiredZOrder = Pair.Key == Model.DraggedCardId
			? 1000.0f
			: Entry->Props.bSelected
				? 100.0f + FMath::Max(VisualIndex, 0)
				: static_cast<float>(FMath::Max(VisualIndex, 0));
		if (!FMath::IsNearlyEqual(Entry->Slot->GetZOrder(), DesiredZOrder))
		{
			Entry->Slot->SetZOrder(DesiredZOrder);
		}
	}
}

void UpdateMotionTargets(FSGSTableHandWidgetModel& Model)
{
	for (const TPair<int32, TSharedPtr<FSGSHandCardEntry>>& Pair : Model.Entries)
	{
		FSGSHandCardEntry& Entry = *Pair.Value;
		const int32 PreviewIndex = Model.PreviewOrder.IndexOfByKey(Pair.Key);
		const FVector2D PreviewPosition = PreviewIndex == INDEX_NONE
			? Entry.BasePosition
			: BasePositionForIndex(Model, PreviewIndex);

		Entry.TargetTranslation = PreviewPosition - Entry.BasePosition;
		if (Entry.Props.bSelected)
		{
			Entry.TargetTranslation.Y -= Model.CardLift;
		}
		else if (Entry.bHovered)
		{
			Entry.TargetTranslation.Y -= Model.HoverLift;
		}

		Entry.TargetScale = Entry.bHovered ? HoverScale : Entry.Props.bSelected ? SelectedScale : 1.0f;
		Entry.TargetRotation = 0.0f;
		if (Pair.Key == Model.DraggedCardId)
		{
			Entry.TargetTranslation = Model.PointerCanvasPosition - Model.GrabOffset - Entry.BasePosition;
			Entry.TargetScale = DragScale;
			Entry.TargetRotation = FMath::Clamp(Model.PointerDelta.X * 0.12f, -4.0f, 4.0f);
		}
		else if (Entry.ReturnSequence.IsValid())
		{
			const float Alpha = Entry.ReturnCurve.GetLerp();
			Entry.TargetScale = FMath::Lerp(Entry.ReturnStartScale, Entry.TargetScale, Alpha);
			Entry.TargetRotation = FMath::Lerp(Entry.ReturnStartRotation, 0.0f, Alpha);
			if (!Entry.ReturnSequence->IsPlaying())
			{
				Entry.ReturnSequence.Reset();
				Entry.ReturnCurve = FCurveHandle();
			}
		}
	}
}

void RecomputeLayout(FSGSTableHandWidgetModel& Model, bool bPreserveVisualPosition)
{
	Model.InnerPadding = 4.0f * Model.Props.LayoutScale;
	Model.CardLift = 18.0f * Model.Props.LayoutScale;
	Model.HoverLift = 13.0f * Model.Props.LayoutScale;
	const float UsableWidth = FMath::Max(
		0.0f,
		Model.Props.AvailableWidth - Model.InnerPadding * 2.0f);
	const float MaxStride = Model.Props.CardSize.X + 8.0f * Model.Props.LayoutScale;
	const float MinStride = 38.0f * Model.Props.LayoutScale;
	Model.Stride = 0.0f;
	if (Model.CommittedOrder.Num() > 1)
	{
		Model.Stride = (UsableWidth - Model.Props.CardSize.X)
			/ static_cast<float>(Model.CommittedOrder.Num() - 1);
		Model.Stride = FMath::Clamp(Model.Stride, MinStride, MaxStride);
	}

	const float CardsWidth = Model.CommittedOrder.IsEmpty()
		? 0.0f
		: Model.Props.CardSize.X
			+ Model.Stride * static_cast<float>(Model.CommittedOrder.Num() - 1);
	Model.InnerWidth = FMath::Max(UsableWidth, CardsWidth);
	Model.StartOffset = CardsWidth > 0.0f && CardsWidth < UsableWidth
		? (UsableWidth - CardsWidth) * 0.5f
		: 0.0f;

	if (Model.RootBorder.IsValid())
	{
		Model.RootBorder->SetPadding(FMargin(
			4.0f * Model.Props.LayoutScale,
			2.0f * Model.Props.LayoutScale));
	}
	if (Model.ScrollSlot != nullptr)
	{
		Model.ScrollSlot->SetPadding(FMargin(Model.InnerPadding, 0.0f));
	}
	if (Model.CanvasBox.IsValid())
	{
		Model.CanvasBox->SetWidthOverride(Model.InnerWidth);
		Model.CanvasBox->SetHeightOverride(Model.Props.CardSize.Y + Model.CardLift);
	}

	for (int32 Index = 0; Index < Model.CommittedOrder.Num(); ++Index)
	{
		const int32 CardId = Model.CommittedOrder[Index];
		const TSharedPtr<FSGSHandCardEntry>* FoundEntry = Model.Entries.Find(CardId);
		if (FoundEntry == nullptr || !FoundEntry->IsValid())
		{
			continue;
		}

		FSGSHandCardEntry& Entry = **FoundEntry;
		const FVector2D OldVisualPosition = Entry.BasePosition + Entry.CurrentTranslation;
		const FVector2D NewBasePosition = BasePositionForIndex(Model, Index);
		if (bPreserveVisualPosition && Entry.bHasBasePosition)
		{
			Entry.CurrentTranslation = OldVisualPosition - NewBasePosition;
		}
		else
		{
			Entry.CurrentTranslation = FVector2D(
				0.0f,
				Entry.Props.bSelected ? -Model.CardLift : 0.0f);
		}
		Entry.BasePosition = NewBasePosition;
		Entry.bHasBasePosition = true;
		if (Entry.Slot != nullptr)
		{
			Entry.Slot->SetOffset(FMargin(
				NewBasePosition.X,
				NewBasePosition.Y,
				Model.Props.CardSize.X,
				Model.Props.CardSize.Y));
		}
	}

	UpdateMotionTargets(Model);
	RefreshZOrders(Model);
}

void UpdatePreviewOrderFromPointer(FSGSTableHandWidgetModel& Model)
{
	if (Model.DraggedCardId == INDEX_NONE || !Model.Entries.Contains(Model.DraggedCardId))
	{
		return;
	}

	TArray<int32> OtherCards = Model.CommittedOrder;
	OtherCards.RemoveSingle(Model.DraggedCardId);
	const float DragCenterX = Model.PointerCanvasPosition.X
		- Model.GrabOffset.X
		+ Model.Props.CardSize.X * 0.5f;
	int32 ProposedIndex = 0;
	for (int32 Index = 0; Index < OtherCards.Num(); ++Index)
	{
		const float SlotCenter = Model.StartOffset
			+ Model.Stride * static_cast<float>(Index)
			+ Model.Props.CardSize.X * 0.5f;
		if (DragCenterX > SlotCenter)
		{
			++ProposedIndex;
		}
	}

	const int32 CurrentIndex = Model.PreviewOrder.IndexOfByKey(Model.DraggedCardId);
	const float Hysteresis = Model.Stride * 0.20f;
	if (CurrentIndex != INDEX_NONE
		&& ProposedIndex != CurrentIndex
		&& FMath::Abs(DragCenterX - Model.LastInsertionChangeX) < Hysteresis)
	{
		return;
	}

	Model.PreviewOrder = MoveTemp(OtherCards);
	Model.PreviewOrder.Insert(Model.DraggedCardId, FMath::Clamp(ProposedIndex, 0, Model.PreviewOrder.Num()));
	if (ProposedIndex != CurrentIndex)
	{
		Model.LastInsertionChangeX = DragCenterX;
	}
	UpdateMotionTargets(Model);
}

void StartReturnCurve(FSGSHandCardEntry& Entry, const TSharedRef<SWidget>& OwnerWidget)
{
	Entry.ReturnStartScale = Entry.CurrentScale;
	Entry.ReturnStartRotation = Entry.CurrentRotation;
	Entry.ReturnSequence = MakeShared<FCurveSequence>();
	Entry.ReturnCurve = Entry.ReturnSequence->AddCurve(0.0f, ReturnDuration, ECurveEaseFunction::CubicOut);
	// Hand 自己的 Active Timer 是唯一时钟；CurveSequence 只负责采样 easing。
	Entry.ReturnSequence->Play(OwnerWidget, false, 0.0f, false);
}
}

class FSGSCardDragDropOperation final : public FGameDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FSGSCardDragDropOperation, FGameDragDropOperation)

	static TSharedRef<FSGSCardDragDropOperation> New(
		int32 InCardId,
		const TSharedRef<SSGSTableHandWidget>& InOwner,
		const FSGSTableCardProps& InCardProps,
		const FVector2D& InInitialScreenPosition,
		float InInitialScale,
		float InInitialRotation)
	{
		TSharedRef<FSGSCardDragDropOperation> Operation = MakeShared<FSGSCardDragDropOperation>();
		Operation->CardId = InCardId;
		Operation->Owner = InOwner;
		Operation->DecoratorWidget = SNew(SBox)
			.Visibility(EVisibility::HitTestInvisible)
		[
			SAssignNew(Operation->DecoratorCard, SSGSTableCardWidget)
				.Props(InCardProps)
		];
		Operation->SetVisualState(InInitialScreenPosition, InInitialScale, InInitialRotation);
		Operation->Construct();
		return Operation;
	}

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
	{
		return DecoratorWidget;
	}

	virtual void OnDragged(const FDragDropEvent& DragDropEvent) override
	{
		if (const TSharedPtr<SSGSTableHandWidget> Pinned = Owner.Pin())
		{
			Pinned->HandleDragMoved(DragDropEvent.GetScreenSpacePosition());
		}
		FGameDragDropOperation::OnDragged(DragDropEvent);
	}

	virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent) override
	{
		if (const TSharedPtr<SSGSTableHandWidget> Pinned = Owner.Pin())
		{
			Pinned->HandleDragOperationEnded(CardId, bDroppedInHand);
		}
		FDragDropOperation::OnDrop(bDropWasHandled, MouseEvent);
	}

	void MarkDroppedInHand()
	{
		bDroppedInHand = true;
	}

	int32 GetCardId() const
	{
		return CardId;
	}

	void SetVisualState(const FVector2D& ScreenPosition, float Scale, float Rotation)
	{
		DecoratorPosition = ScreenPosition;
		if (DecoratorCard.IsValid())
		{
			const FSlateRenderTransform RenderTransform = ::Concatenate(
				FScale2D(Scale),
				FQuat2D(FMath::DegreesToRadians(Rotation)),
				FVector2D::ZeroVector);
			DecoratorCard->SetRenderTransform(RenderTransform);
		}
	}

private:
	int32 CardId = INDEX_NONE;
	TWeakPtr<SSGSTableHandWidget> Owner;
	TSharedPtr<SWidget> DecoratorWidget;
	TSharedPtr<SSGSTableCardWidget> DecoratorCard;
	bool bDroppedInHand = false;
};

void SSGSTableCardWidget::Construct(const FArguments& InArgs)
{
	OnCardClicked = InArgs._OnCardClicked;
	OnCardDragDetected = InArgs._OnCardDragDetected;
	OnCardHoverChanged = InArgs._OnCardHoverChanged;

	TSharedRef<SOverlay> CardFace = SNew(SOverlay)
		.Clipping(EWidgetClipping::ClipToBounds);
	CardFace->AddSlot()
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.82f, 0.76f, 0.62f, 1.0f))
	];
	CardFace->AddSlot()
	[
		SAssignNew(FaceImage, SImage)
			.ColorAndOpacity(FLinearColor::White)
	];
	CardFace->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(3.0f))
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.97f, 0.93f, 0.82f, 0.88f))
			.Padding(FMargin(2.0f, 0.0f))
		[
			SAssignNew(CornerText, STextBlock)
				.ColorAndOpacity(FLinearColor(0.08f, 0.06f, 0.045f, 1.0f))
		]
	];
	CardFace->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.018f, 0.78f))
			.Padding(FMargin(2.0f))
		[
			SAssignNew(FooterText, STextBlock)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
		]
	];
	CardFace->AddSlot()
	[
		SAssignNew(UnavailableShade, SBorder)
			.Visibility(EVisibility::Collapsed)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.62f))
	];

	ChildSlot
	[
		SAssignNew(SizeBox, SBox)
		[
			SAssignNew(InteractionBorder, SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.Padding(FMargin(2.0f))
			[
				CardFace
			]
		]
	];
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	SetProps(InArgs._Props);
}

void SSGSTableCardWidget::SetProps(const FSGSTableCardProps& InProps)
{
	ensureMsgf(
		Props.CardId == INDEX_NONE || Props.CardId == InProps.CardId,
		TEXT("A keyed table card widget cannot change CardId."));
	Props = InProps;
	if (SizeBox.IsValid())
	{
		SizeBox->SetWidthOverride(Props.Size.X);
		SizeBox->SetHeightOverride(Props.Size.Y);
	}
	if (InteractionBorder.IsValid())
	{
		InteractionBorder->SetBorderBackgroundColor(CardTint(Props));
	}
	if (UnavailableShade.IsValid())
	{
		UnavailableShade->SetVisibility(
			Props.bDimmed ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}
	if (FaceImage.IsValid())
	{
		FaceImage->SetImage(Props.FaceBrush);
		FaceImage->SetVisibility(Props.FaceBrush != nullptr ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (CornerText.IsValid())
	{
		CornerText->SetText(Props.CornerText);
	}
	if (FooterText.IsValid())
	{
		FooterText->SetText(Props.FooterText);
	}
}

FReply SSGSTableCardWidget::OnMouseButtonDown(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressPending = true;
	return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
}

FReply SSGSTableCardWidget::OnMouseButtonUp(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !bPressPending)
	{
		return FReply::Unhandled();
	}

	bPressPending = false;
	if (Props.bSelectable && OnCardClicked.IsBound())
	{
		return OnCardClicked.Execute(Props.CardId);
	}
	return FReply::Handled();
}

FReply SSGSTableCardWidget::OnDragDetected(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	bPressPending = false;
	return OnCardDragDetected.IsBound()
		? OnCardDragDetected.Execute(Props.CardId, MouseEvent)
		: FReply::Unhandled();
}

void SSGSTableCardWidget::OnMouseEnter(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	if (OnCardHoverChanged.IsBound())
	{
		OnCardHoverChanged.Execute(Props.CardId, true);
	}
}

void SSGSTableCardWidget::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);
	if (OnCardHoverChanged.IsBound())
	{
		OnCardHoverChanged.Execute(Props.CardId, false);
	}
}

SSGSTableHandWidget::SSGSTableHandWidget()
	: Model(MakeUnique<FSGSTableHandWidgetModel>())
{
}

SSGSTableHandWidget::~SSGSTableHandWidget()
{
	if (Model.IsValid() && Model->ActiveTimerHandle.IsValid())
	{
		UnRegisterActiveTimer(Model->ActiveTimerHandle.ToSharedRef());
		Model->ActiveTimerHandle.Reset();
	}
}

void SSGSTableHandWidget::Construct(const FArguments& InArgs)
{
	OnCardClicked = InArgs._OnCardClicked;
	OnHandReordered = InArgs._OnHandReordered;

	SAssignNew(Model->HandScroll, SScrollBox)
		.Orientation(Orient_Horizontal)
		.ScrollBarVisibility(EVisibility::Collapsed);
	Model->HandScroll->AddSlot()
		.Expose(Model->ScrollSlot)
		.VAlign(VAlign_Bottom)
	[
		SAssignNew(Model->CanvasBox, SBox)
		[
			SAssignNew(Model->HandCanvas, SConstraintCanvas)
				.Clipping(EWidgetClipping::ClipToBounds)
		]
	];

	ChildSlot
	[
		SAssignNew(Model->RootBorder, SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.72f))
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				Model->HandScroll.ToSharedRef()
			]
			+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
			[
				SAssignNew(Model->EmptyText, STextBlock)
					.Text(FText::FromString(TEXT("Hand is empty.")))
					.ColorAndOpacity(FLinearColor(0.82f, 0.82f, 0.78f, 1.0f))
					.Visibility(EVisibility::HitTestInvisible)
			]
		]
	];

	SetProps(InArgs._Props);
}

void SSGSTableHandWidget::SetProps(const FSGSTableHandProps& InProps)
{
	FSGSTableHandProps AcceptedProps = InProps;
	AcceptedProps.Cards.Reset();
	AcceptedProps.Cards.Reserve(InProps.Cards.Num());
	TSet<int32> IncomingCardIds;
	IncomingCardIds.Reserve(InProps.Cards.Num());
	for (const FSGSTableCardProps& Card : InProps.Cards)
	{
		if (Card.CardId == INDEX_NONE || IncomingCardIds.Contains(Card.CardId))
		{
			ensureMsgf(false, TEXT("Hand props must contain unique, valid CardIds."));
			continue;
		}
		IncomingCardIds.Add(Card.CardId);
		AcceptedProps.Cards.Add(Card);
	}
	if (Model->bHasProps && SameHandProps(Model->Props, AcceptedProps))
	{
		return;
	}

	const bool bDraggedCardRemoved = Model->DraggedCardId != INDEX_NONE
		&& !IncomingCardIds.Contains(Model->DraggedCardId);
	if (bDraggedCardRemoved)
	{
		FinishDrag(false);
	}

	TArray<int32> RemovedCardIds;
	for (const TPair<int32, TSharedPtr<FSGSHandCardEntry>>& Pair : Model->Entries)
	{
		if (!IncomingCardIds.Contains(Pair.Key))
		{
			RemovedCardIds.Add(Pair.Key);
		}
	}
	for (const int32 CardId : RemovedCardIds)
	{
		if (const TSharedPtr<FSGSHandCardEntry>* Entry = Model->Entries.Find(CardId))
		{
			if (Entry->IsValid() && (*Entry)->Widget.IsValid())
			{
				Model->HandCanvas->RemoveSlot((*Entry)->Widget.ToSharedRef());
			}
		}
		Model->Entries.Remove(CardId);
	}

	Model->Props = MoveTemp(AcceptedProps);
	Model->bHasProps = true;
	Model->CommittedOrder.Reset(Model->Props.Cards.Num());
	for (const FSGSTableCardProps& Card : Model->Props.Cards)
	{
		Model->CommittedOrder.Add(Card.CardId);
		TSharedPtr<FSGSHandCardEntry>* Existing = Model->Entries.Find(Card.CardId);
		if (Existing != nullptr && Existing->IsValid())
		{
			(*Existing)->Props = Card;
			(*Existing)->Widget->SetProps(Card);
			continue;
		}

		TSharedPtr<FSGSHandCardEntry> NewEntry = MakeShared<FSGSHandCardEntry>();
		NewEntry->Props = Card;
		Model->HandCanvas->AddSlot()
			.Expose(NewEntry->Slot)
			.Anchors(FAnchors(0.0f, 0.0f))
			.Alignment(FVector2D::ZeroVector)
		[
			SAssignNew(NewEntry->Widget, SSGSTableCardWidget)
				.Props(Card)
				.OnCardClicked(OnCardClicked)
				.OnCardDragDetected(this, &SSGSTableHandWidget::HandleCardDragDetected)
				.OnCardHoverChanged(this, &SSGSTableHandWidget::HandleCardHoverChanged)
		];
		if (Model->ActiveTimerHandle.IsValid())
		{
			NewEntry->Widget->ForceVolatile(true);
		}
		Model->Entries.Add(Card.CardId, MoveTemp(NewEntry));
	}

	Model->PreviewOrder = Model->CommittedOrder;
	RecomputeLayout(*Model, true);
	if (Model->DraggedCardId != INDEX_NONE)
	{
		UpdatePreviewOrderFromPointer(*Model);
	}
	if (Model->EmptyText.IsValid())
	{
		Model->EmptyText->SetVisibility(
			Model->CommittedOrder.IsEmpty()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed);
	}
	EnsureMotionTimer();

	if (bDraggedCardRemoved && FSlateApplication::IsInitialized())
	{
		const TSharedPtr<FDragDropOperation> CurrentOperation =
			FSlateApplication::Get().GetDragDroppingContent();
		if (CurrentOperation.IsValid()
			&& CurrentOperation->IsOfType<FSGSCardDragDropOperation>())
		{
			FSlateApplication::Get().CancelDragDrop();
		}
	}
}

FReply SSGSTableHandWidget::HandleCardDragDetected(
	int32 CardId,
	const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FSGSHandCardEntry>* Entry = Model->Entries.Find(CardId);
	if (Entry == nullptr || !Entry->IsValid() || !Model->HandCanvas.IsValid())
	{
		return FReply::Unhandled();
	}

	Model->HandScroll->EndInertialScrolling();
	Model->DraggedCardId = CardId;
	Model->PreviewOrder = Model->CommittedOrder;
	Model->PointerScreenPosition = MouseEvent.GetScreenSpacePosition();
	Model->PointerCanvasPosition = Model->HandCanvas->GetCachedGeometry().AbsoluteToLocal(
		Model->PointerScreenPosition);
	Model->PointerDelta = FVector2D::ZeroVector;
	Model->GrabOffset = Model->PointerCanvasPosition
		- ((*Entry)->BasePosition + (*Entry)->CurrentTranslation);
	Model->LastInsertionChangeX = Model->PointerCanvasPosition.X
		- Model->GrabOffset.X
		+ Model->Props.CardSize.X * 0.5f;
	Model->bPointerInsideHand = true;
	(*Entry)->bHovered = false;
	RefreshZOrders(*Model);
	UpdateMotionTargets(*Model);
	EnsureMotionTimer();
	const FVector2D InitialScreenPosition =
		Model->HandCanvas->GetCachedGeometry().LocalToAbsolute(
			(*Entry)->BasePosition + (*Entry)->CurrentTranslation);
	const TSharedRef<FSGSCardDragDropOperation> Operation =
		FSGSCardDragDropOperation::New(
			CardId,
			SharedThis(this),
			(*Entry)->Props,
			InitialScreenPosition,
			(*Entry)->CurrentScale,
			(*Entry)->CurrentRotation);
	Model->DragOperation = Operation;
	(*Entry)->Widget->SetRenderOpacity(0.0f);

	return FReply::Handled().BeginDragDrop(Operation);
}

void SSGSTableHandWidget::HandleCardHoverChanged(int32 CardId, bool bHovered)
{
	const TSharedPtr<FSGSHandCardEntry>* Entry = Model->Entries.Find(CardId);
	if (Entry == nullptr || !Entry->IsValid())
	{
		return;
	}
	(*Entry)->bHovered = bHovered && CardId != Model->DraggedCardId;
	UpdateMotionTargets(*Model);
	EnsureMotionTimer();
}

void SSGSTableHandWidget::HandleDragMoved(const FVector2D& ScreenPosition)
{
	if (Model->DraggedCardId == INDEX_NONE || !Model->HandCanvas.IsValid())
	{
		return;
	}
	const FVector2D NewCanvasPosition =
		Model->HandCanvas->GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
	Model->PointerDelta = NewCanvasPosition - Model->PointerCanvasPosition;
	Model->PointerScreenPosition = ScreenPosition;
	Model->PointerCanvasPosition = NewCanvasPosition;
	if (Model->bPointerInsideHand)
	{
		UpdatePreviewOrderFromPointer(*Model);
	}
	else
	{
		UpdateMotionTargets(*Model);
	}
	EnsureMotionTimer();
}

FReply SSGSTableHandWidget::OnDragOver(
	const FGeometry& MyGeometry,
	const FDragDropEvent& DragDropEvent)
{
	const TSharedPtr<FSGSCardDragDropOperation> Operation =
		DragDropEvent.GetOperationAs<FSGSCardDragDropOperation>();
	if (!Operation.IsValid() || Operation->GetCardId() != Model->DraggedCardId)
	{
		return FReply::Unhandled();
	}
	Model->bPointerInsideHand = true;
	HandleDragMoved(DragDropEvent.GetScreenSpacePosition());
	return FReply::Handled();
}

void SSGSTableHandWidget::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	SCompoundWidget::OnDragLeave(DragDropEvent);
	const TSharedPtr<FSGSCardDragDropOperation> Operation =
		DragDropEvent.GetOperationAs<FSGSCardDragDropOperation>();
	if (!Operation.IsValid() || Operation->GetCardId() != Model->DraggedCardId)
	{
		return;
	}
	Model->bPointerInsideHand = false;
	Model->PreviewOrder = Model->CommittedOrder;
	UpdateMotionTargets(*Model);
	RefreshZOrders(*Model);
}

FReply SSGSTableHandWidget::OnDrop(
	const FGeometry& MyGeometry,
	const FDragDropEvent& DragDropEvent)
{
	const TSharedPtr<FSGSCardDragDropOperation> Operation =
		DragDropEvent.GetOperationAs<FSGSCardDragDropOperation>();
	if (!Operation.IsValid() || Operation->GetCardId() != Model->DraggedCardId)
	{
		return FReply::Unhandled();
	}

	Model->bPointerInsideHand = true;
	HandleDragMoved(DragDropEvent.GetScreenSpacePosition());
	Operation->MarkDroppedInHand();
	FinishDrag(true);
	return FReply::Handled();
}

void SSGSTableHandWidget::HandleDragOperationEnded(int32 CardId, bool bDroppedInHand)
{
	if (CardId == Model->DraggedCardId)
	{
		FinishDrag(bDroppedInHand);
	}
}

void SSGSTableHandWidget::FinishDrag(bool bCommit)
{
	const int32 FinishedCardId = Model->DraggedCardId;
	if (FinishedCardId == INDEX_NONE)
	{
		return;
	}

	const TArray<int32> PreviousOrder = Model->CommittedOrder;
	const TArray<int32> CandidateOrder = bCommit ? Model->PreviewOrder : PreviousOrder;
	Model->DraggedCardId = INDEX_NONE;
	Model->bPointerInsideHand = false;
	Model->PointerDelta = FVector2D::ZeroVector;
	const bool bOrderChanged = CandidateOrder != PreviousOrder;
	const bool bAccepted = bCommit
		&& (!bOrderChanged
			|| (OnHandReordered.IsBound() && OnHandReordered.Execute(CandidateOrder)));
	Model->CommittedOrder = bAccepted ? CandidateOrder : PreviousOrder;
	Model->PreviewOrder = Model->CommittedOrder;

	if (const TSharedPtr<FSGSHandCardEntry>* Entry = Model->Entries.Find(FinishedCardId))
	{
		if (Entry->IsValid())
		{
			(*Entry)->Widget->SetRenderOpacity(1.0f);
			(*Entry)->bHovered = false;
			if (!bAccepted)
			{
				StartReturnCurve(**Entry, SharedThis(this));
			}
		}
	}
	Model->DragOperation.Reset();

	RecomputeLayout(*Model, true);
	EnsureMotionTimer();
}

void SSGSTableHandWidget::EnsureMotionTimer()
{
	if (Model->ActiveTimerHandle.IsValid())
	{
		return;
	}
	for (const TPair<int32, TSharedPtr<FSGSHandCardEntry>>& Pair : Model->Entries)
	{
		if (Pair.Value.IsValid() && Pair.Value->Widget.IsValid())
		{
			Pair.Value->Widget->ForceVolatile(true);
		}
	}
	Model->ActiveTimerHandle = RegisterActiveTimer(
		0.0f,
		FWidgetActiveTimerDelegate::CreateSP(this, &SSGSTableHandWidget::UpdateMotion));
}

EActiveTimerReturnType SSGSTableHandWidget::UpdateMotion(double CurrentTime, float DeltaTime)
{
	if (Model->DraggedCardId != INDEX_NONE && Model->bPointerInsideHand && Model->HandScroll.IsValid())
	{
		const FGeometry& ScrollGeometry = Model->HandScroll->GetCachedGeometry();
		const FVector2D PointerInScroll = ScrollGeometry.AbsoluteToLocal(Model->PointerScreenPosition);
		const float ScrollWidth = ScrollGeometry.GetLocalSize().X;
		const float EdgeSize = FMath::Min(48.0f * Model->Props.LayoutScale, ScrollWidth / 3.0f);
		float Direction = 0.0f;
		if (EdgeSize > 0.0f && PointerInScroll.X < EdgeSize)
		{
			Direction = -(1.0f - PointerInScroll.X / EdgeSize);
		}
		else if (EdgeSize > 0.0f && PointerInScroll.X > ScrollWidth - EdgeSize)
		{
			Direction = (PointerInScroll.X - (ScrollWidth - EdgeSize)) / EdgeSize;
		}

		if (!FMath::IsNearlyZero(Direction))
		{
			const float OldOffset = Model->HandScroll->GetScrollOffset();
			const float NewOffset = FMath::Clamp(
				OldOffset + Direction * 900.0f * Model->Props.LayoutScale * DeltaTime,
				0.0f,
				Model->HandScroll->GetScrollOffsetOfEnd());
			if (!FMath::IsNearlyEqual(OldOffset, NewOffset))
			{
				Model->HandScroll->SetScrollOffset(NewOffset);
				Model->PointerCanvasPosition.X += NewOffset - OldOffset;
				UpdatePreviewOrderFromPointer(*Model);
			}
		}
	}

	UpdateMotionTargets(*Model);
	float RemainingTime = FMath::Min(FMath::Max(DeltaTime, 0.0f), MaxSimulationTime);
	while (RemainingTime > UE_SMALL_NUMBER)
	{
		const float Step = FMath::Min(RemainingTime, MaxSimulationStep);
		for (const TPair<int32, TSharedPtr<FSGSHandCardEntry>>& Pair : Model->Entries)
		{
			FSGSHandCardEntry& Entry = *Pair.Value;
			const bool bDragged = Pair.Key == Model->DraggedCardId;
			FMath::SpringDamperSmoothing(
				Entry.CurrentTranslation,
				Entry.TranslationVelocity,
				Entry.TargetTranslation,
				FVector2D::ZeroVector,
				Step,
				bDragged ? DragSmoothingTime : NeighborSmoothingTime,
				bDragged ? 1.0f : NeighborDampingRatio);
			FMath::SpringDamperSmoothing(
				Entry.CurrentScale,
				Entry.ScaleVelocity,
				Entry.TargetScale,
				0.0f,
				Step,
				NeighborSmoothingTime,
				1.0f);
			FMath::SpringDamperSmoothing(
				Entry.CurrentRotation,
				Entry.RotationVelocity,
				Entry.TargetRotation,
				0.0f,
				Step,
				NeighborSmoothingTime,
				NeighborDampingRatio);
		}
		RemainingTime -= Step;
	}

	bool bAnyMoving = Model->DraggedCardId != INDEX_NONE;
	for (const TPair<int32, TSharedPtr<FSGSHandCardEntry>>& Pair : Model->Entries)
	{
		FSGSHandCardEntry& Entry = *Pair.Value;
		const bool bTranslationSettled = Entry.CurrentTranslation.Equals(
			Entry.TargetTranslation,
			PositionTolerance)
			&& Entry.TranslationVelocity.SizeSquared() <= FMath::Square(VelocityTolerance);
		const bool bScaleSettled = FMath::IsNearlyEqual(
			Entry.CurrentScale,
			Entry.TargetScale,
			ScaleTolerance)
			&& FMath::Abs(Entry.ScaleVelocity) <= ScaleTolerance;
		const bool bRotationSettled = FMath::IsNearlyEqual(
			Entry.CurrentRotation,
			Entry.TargetRotation,
			RotationTolerance)
			&& FMath::Abs(Entry.RotationVelocity) <= RotationTolerance;
		if (bTranslationSettled)
		{
			Entry.CurrentTranslation = Entry.TargetTranslation;
			Entry.TranslationVelocity = FVector2D::ZeroVector;
		}
		if (bScaleSettled)
		{
			Entry.CurrentScale = Entry.TargetScale;
			Entry.ScaleVelocity = 0.0f;
		}
		if (bRotationSettled)
		{
			Entry.CurrentRotation = Entry.TargetRotation;
			Entry.RotationVelocity = 0.0f;
		}
		ApplyRenderTransform(Entry);
		if (Pair.Key == Model->DraggedCardId)
		{
			if (const TSharedPtr<FSGSCardDragDropOperation> Operation =
				Model->DragOperation.Pin())
			{
				const FVector2D ScreenPosition =
					Model->HandCanvas->GetCachedGeometry().LocalToAbsolute(
						Entry.BasePosition + Entry.CurrentTranslation);
				Operation->SetVisualState(
					ScreenPosition,
					Entry.CurrentScale,
					Entry.CurrentRotation);
			}
		}
		bAnyMoving |= !bTranslationSettled
			|| !bScaleSettled
			|| !bRotationSettled
			|| Entry.ReturnSequence.IsValid();
	}
	Model->PointerDelta *= FMath::Exp(-20.0f * DeltaTime);

	if (bAnyMoving)
	{
		return EActiveTimerReturnType::Continue;
	}

	for (const TPair<int32, TSharedPtr<FSGSHandCardEntry>>& Pair : Model->Entries)
	{
		if (Pair.Value.IsValid() && Pair.Value->Widget.IsValid())
		{
			Pair.Value->Widget->ForceVolatile(false);
		}
	}
	Model->ActiveTimerHandle.Reset();
	return EActiveTimerReturnType::Stop;
}
