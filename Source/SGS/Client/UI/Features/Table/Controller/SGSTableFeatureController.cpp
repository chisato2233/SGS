#include "Client/UI/Features/Table/Controller/SGSTableFeatureController.h"

#include "Client/UI/Core/Context/SGSUIContext.h"

namespace
{
const FName ConfirmFocusTarget(TEXT("Table.Confirm"));

FString DisplayDecisionName(FName Name)
{
	if (Name == TEXT("Slash")) return TEXT("杀");
	if (Name == TEXT("Dodge")) return TEXT("闪");
	if (Name == TEXT("Peach")) return TEXT("桃");
	if (Name == TEXT("Nullification") || Name == TEXT("Wuxie")) return TEXT("无懈可击");
	if (Name == TEXT("ArrowBarrage") || Name == TEXT("Wanjian")) return TEXT("万箭齐发");
	if (Name == TEXT("BarbarianInvasion") || Name == TEXT("Nanman")) return TEXT("南蛮入侵");
	if (Name == TEXT("Dying")) return TEXT("濒死求桃");
	return Name.IsNone() ? FString() : Name.ToString();
}

FString SeatDisplayName(const FSGSTableViewSnapshot& Snapshot, int32 SeatIndex)
{
	const FSGSSeatViewData* Seat = Snapshot.Seats.FindByPredicate(
		[SeatIndex](const FSGSSeatViewData& Candidate)
		{
			return Candidate.SeatIndex == SeatIndex;
		});
	return Seat != nullptr ? Seat->DisplayName : FString::Printf(TEXT("座位 %d"), SeatIndex);
}
}

FSGSTableFeatureController::FSGSTableFeatureController(
	int32 InViewerSeat,
	FSGSTableFeatureBindings InBindings,
	TSharedRef<FSGSUIContext> InUIContext,
	int32 InInitialMotionSequence)
	: Bindings(MoveTemp(InBindings))
	, UIContext(MoveTemp(InUIContext))
	, State(InViewerSeat, InInitialMotionSequence)
{
}

bool FSGSTableFeatureController::RefreshFromHost()
{
	if (!Bindings.ReadSnapshot)
	{
		return false;
	}
	return State.IngestSnapshot(Bindings.ReadSnapshot());
}

bool FSGSTableFeatureController::SelectCard(int32 CardId)
{
	if (!State.SelectCard(CardId))
	{
		return false;
	}
	FocusConfirmIfReady();
	return true;
}

bool FSGSTableFeatureController::SelectTarget(int32 SeatIndex)
{
	if (!State.SelectTarget(SeatIndex))
	{
		return false;
	}
	FocusConfirmIfReady();
	return true;
}

bool FSGSTableFeatureController::SelectSkill(FName SkillName)
{
	if (!State.SelectSkill(SkillName))
	{
		return false;
	}
	FocusConfirmIfReady();
	return true;
}

bool FSGSTableFeatureController::ReorderHand(TConstArrayView<int32> OrderedCardIds)
{
	return State.ReorderHand(OrderedCardIds);
}

bool FSGSTableFeatureController::Confirm()
{
	if (!IsConfirmEnabled())
	{
		PublishToast(FText::FromString(TEXT("Select a legal card and target first.")), false);
		return false;
	}

	const FSGSTableViewSnapshot& Snapshot = State.GetSnapshot();
	const FSGSTableUIInteractionState& Interaction = State.GetInteractionState();
	const bool bSubmitted = Snapshot.Prompt.bIsResponse
		? Bindings.SubmitResponseCard
			&& Bindings.SubmitResponseCard(
				Interaction.SelectedCardId,
				Interaction.SelectedTargetSeat,
				Interaction.SelectedSkillName)
		: Bindings.SubmitUseCard
			&& Bindings.SubmitUseCard(Interaction.SelectedCardId, Interaction.SelectedTargetSeat);
	if (!bSubmitted)
	{
		PublishToast(FText::FromString(TEXT("The action was rejected.")), false);
		return false;
	}

	State.ClearSelection();
	PublishToast(FText::FromString(TEXT("Action submitted.")), true);
	return true;
}

bool FSGSTableFeatureController::Pass()
{
	const FSGSTableViewSnapshot& Snapshot = State.GetSnapshot();
	if (!Snapshot.Prompt.bHasPrompt || !Snapshot.Prompt.bAllowPass || !Bindings.SubmitPass)
	{
		return false;
	}
	if (!Bindings.SubmitPass())
	{
		PublishToast(FText::FromString(TEXT("Pass was rejected.")), false);
		return false;
	}

	State.ClearSelection();
	PublishToast(FText::FromString(TEXT("Passed.")), true);
	return true;
}

bool FSGSTableFeatureController::IsConfirmEnabled() const
{
	return State.IsSelectionComplete();
}

FString FSGSTableFeatureController::GetPromptTitle() const
{
	const FSGSTableViewSnapshot& Snapshot = State.GetSnapshot();
	if (!Snapshot.Prompt.bHasPrompt)
	{
		return FString();
	}
	return Snapshot.Prompt.bIsResponse ? TEXT("响应请求") : TEXT("出牌阶段");
}

FString FSGSTableFeatureController::GetPromptText() const
{
	const FSGSTableViewSnapshot& Snapshot = State.GetSnapshot();
	if (!Snapshot.Prompt.bHasPrompt)
	{
		return FString();
	}
	if (Snapshot.Prompt.bIsResponse)
	{
		const FString RequiredName = DisplayDecisionName(Snapshot.Prompt.RequiredCardName);
		const FString ContextName = DisplayDecisionName(Snapshot.Prompt.ContextName);
		const bool bUseVerb = Snapshot.Prompt.RequiredCardName == TEXT("Nullification")
			|| Snapshot.Prompt.RequiredCardName == TEXT("Wuxie")
			|| Snapshot.Prompt.ContextName == TEXT("Dying");
		if (!ContextName.IsEmpty())
		{
			return bUseVerb
				? FString::Printf(TEXT("请使用【%s】响应【%s】"), *RequiredName, *ContextName)
				: FString::Printf(TEXT("请打出【%s】响应【%s】"), *RequiredName, *ContextName);
		}
		return bUseVerb
			? FString::Printf(TEXT("请使用【%s】"), *RequiredName)
			: FString::Printf(TEXT("请打出【%s】"), *RequiredName);
	}
	return TEXT("请选择一张可用牌和合法目标");
}

FString FSGSTableFeatureController::GetPromptContextText() const
{
	const FSGSTableViewSnapshot& Snapshot = State.GetSnapshot();
	if (!Snapshot.Prompt.bHasPrompt || !Snapshot.Prompt.bIsResponse)
	{
		return FString();
	}
	TArray<FString> Parts;
	if (Snapshot.Prompt.EffectSourceSeat != INDEX_NONE)
	{
		Parts.Add(FString::Printf(
			TEXT("来源：%s"),
			*SeatDisplayName(Snapshot, Snapshot.Prompt.EffectSourceSeat)));
	}
	if (Snapshot.Prompt.EffectTargetSeat != INDEX_NONE)
	{
		Parts.Add(FString::Printf(
			TEXT("目标：%s"),
			*SeatDisplayName(Snapshot, Snapshot.Prompt.EffectTargetSeat)));
	}
	return FString::Join(Parts, TEXT("　"));
}

FString FSGSTableFeatureController::GetConfirmLabel() const
{
	if (!State.GetInteractionState().SelectedSkillName.IsNone())
	{
		return TEXT("发动技能");
	}
	return State.GetSnapshot().Prompt.bIsResponse ? TEXT("确认响应") : TEXT("使用");
}

FString FSGSTableFeatureController::GetPassLabel() const
{
	return State.GetSnapshot().Prompt.bIsResponse ? TEXT("不响应") : TEXT("结束出牌");
}

void FSGSTableFeatureController::PublishToast(const FText& Message, bool bSuccess)
{
	FSGSUIToastRequest Request;
	Request.Message = Message;
	Request.Tone = bSuccess ? ESGSUIToastTone::Success : ESGSUIToastTone::Warning;
	UIContext->Signals().Toast().Publish(Request);
}

void FSGSTableFeatureController::FocusConfirmIfReady()
{
	if (!IsConfirmEnabled())
	{
		return;
	}

	FSGSUIFocusRequest Request;
	Request.TargetId = ConfirmFocusTarget;
	UIContext->Signals().Focus().Publish(Request);
}
