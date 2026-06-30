#include "Game/SGSPlayerController.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Core/SGSLogChannels.h"
#include "Logic/Engine/SGSGameDriver.h"
#include "UI/Bridge/SGSLocalHumanDecisionAgent.h"
#include "UI/Widgets/SGSTableHudWidget.h"

void ASGSPlayerController::AttachToMatch(
	USGSGameDriver* InGameDriver,
	USGSLocalHumanDecisionAgent* InDecisionAgent,
	int32 InViewerSeat)
{
	GameDriver = InGameDriver;
	DecisionAgent = InDecisionAgent;
	ViewerSeat = InViewerSeat;

	if (!IsLocalController() || GEngine == nullptr || GEngine->GameViewport == nullptr || InGameDriver == nullptr)
	{
		return;
	}

	RemoveTableHud();

	SAssignNew(TableHudWidget, SSGSTableHudWidget)
		.GameDriver(GameDriver)
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
