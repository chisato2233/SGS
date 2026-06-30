#include "Client/Game/SGSPlayerController.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Shared/Core/SGSLogChannels.h"
#include "Client/UI/Bridge/SGSLocalHumanDecisionAgent.h"
#include "Client/UI/Widgets/SGSTableHudWidget.h"

void ASGSPlayerController::AttachToMatch(
	TFunction<FSGSTableViewSnapshot()> InSnapshotProvider,
	USGSLocalHumanDecisionAgent* InDecisionAgent,
	int32 InViewerSeat)
{
	SnapshotProvider = MoveTemp(InSnapshotProvider);
	DecisionAgent = InDecisionAgent;
	ViewerSeat = InViewerSeat;

	if (!IsLocalController() || GEngine == nullptr || GEngine->GameViewport == nullptr || !SnapshotProvider)
	{
		return;
	}

	RemoveTableHud();

	SAssignNew(TableHudWidget, SSGSTableHudWidget)
		.SnapshotProvider(SnapshotProvider)
		.DecisionAgent(DecisionAgent)
		.ViewerSeat(ViewerSeat);

	GEngine->GameViewport->AddViewportWidgetContent(TableHudWidget.ToSharedRef(), 10);
	TableHudWidget->SlatePrepass();
	const FVector2D DesiredSize = TableHudWidget->GetDesiredSize();
	UE_LOG(LogSGSUI, Log, TEXT("Local HUD attached. ViewerSeat=%d DesiredSize=%.1fx%.1f"),
		ViewerSeat,
		DesiredSize.X,
		DesiredSize.Y);

	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ASGSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveTableHud();
	Super::EndPlay(EndPlayReason);
}

void ASGSPlayerController::RemoveTableHud()
{
	if (TableHudWidget.IsValid() && GEngine != nullptr && GEngine->GameViewport != nullptr)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(TableHudWidget.ToSharedRef());
	}
	TableHudWidget.Reset();
}
