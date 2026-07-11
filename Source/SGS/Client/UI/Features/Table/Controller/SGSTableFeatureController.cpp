#include "Client/UI/Features/Table/Controller/SGSTableFeatureController.h"

#include "Client/UI/Core/Context/SGSUIContext.h"

namespace
{
const FName ConfirmFocusTarget(TEXT("Table.Confirm"));
}

FSGSTableFeatureController::FSGSTableFeatureController(
	int32 InViewerSeat,
	FSGSTableFeatureBindings InBindings,
	TSharedRef<FSGSUIContext> InUIContext)
	: Bindings(MoveTemp(InBindings))
	, UIContext(MoveTemp(InUIContext))
	, State(InViewerSeat)
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
			&& Bindings.SubmitResponseCard(Interaction.SelectedCardId, Interaction.SelectedTargetSeat)
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
	const FSGSTableViewSnapshot& Snapshot = State.GetSnapshot();
	const FSGSTableUIInteractionState& Interaction = State.GetInteractionState();
	if (!Snapshot.Prompt.bHasPrompt
		|| !Snapshot.Prompt.SelectableCardIds.Contains(Interaction.SelectedCardId))
	{
		return false;
	}

	const TArray<int32> Targets = GetTargetsForCard(Interaction.SelectedCardId);
	return Targets.Num() == 0
		|| Targets.Num() == 1
		|| Targets.Contains(Interaction.SelectedTargetSeat);
}

FString FSGSTableFeatureController::GetPromptText() const
{
	const FSGSTableViewSnapshot& Snapshot = State.GetSnapshot();
	if (!Snapshot.Prompt.bHasPrompt)
	{
		return TEXT("No local decision pending.");
	}
	if (Snapshot.Prompt.bIsResponse)
	{
		return FString::Printf(
			TEXT("Response window: %s | Need: %s | Select a card, then Confirm or Pass."),
			*Snapshot.Prompt.WindowName.ToString(),
			*Snapshot.Prompt.RequiredCardName.ToString());
	}
	return TEXT("Play phase: select a legal card, choose a highlighted target if needed, then Confirm or Pass.");
}

TArray<int32> FSGSTableFeatureController::GetTargetsForCard(int32 CardId) const
{
	if (const TArray<int32>* Targets = State.GetSnapshot().Prompt.FindTargetSeatIndicesForCard(CardId))
	{
		return *Targets;
	}
	return TArray<int32>();
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
