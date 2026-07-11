#include "Client/Game/SGSPlayerController.h"

#include "Client/UI/Bridge/SGSLocalHumanDecisionAgent.h"
#include "Client/UI/Core/Context/SGSUIContext.h"
#include "Client/UI/Features/Table/Controller/SGSTableFeatureController.h"
#include "Client/UI/Features/Toast/SGSUIToastLayerWidget.h"
#include "Client/UI/Widgets/SGSTableHudWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Net/UnrealNetwork.h"
#include "Shared/Game/SGSGameState.h"
#include "Shared/Game/SGSPlayerState.h"
#include "Shared/Core/SGSLogChannels.h"

void ASGSPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASGSPlayerController, PrivateSnapshot);
	DOREPLIFETIME(ASGSPlayerController, ViewerSeat);
}

void ASGSPlayerController::AttachToMatch(
	USGSLocalHumanDecisionAgent* InDecisionAgent,
	int32 InViewerSeat,
	TFunction<FSGSPlayerPrivateSnapshot()> InPrivateSnapshotProvider,
	TFunction<void()> InServerViewRefreshHandler)
{
	DecisionAgent = InDecisionAgent;
	ViewerSeat = InViewerSeat;
	PrivateSnapshotProvider = MoveTemp(InPrivateSnapshotProvider);
	ServerViewRefreshHandler = MoveTemp(InServerViewRefreshHandler);

	if (ASGSPlayerState* SGSPlayerState = GetPlayerState<ASGSPlayerState>())
	{
		SGSPlayerState->SetSeatIndex(ViewerSeat);
	}

	RefreshPrivateSnapshotFromServer();

	if (IsLocalController())
	{
		AttachLocalHud();
	}
	else
	{
		ClientAttachLocalHud(ViewerSeat);
	}
}

void ASGSPlayerController::RefreshPrivateSnapshotFromServer()
{
	if (!HasAuthority())
	{
		return;
	}

	FSGSPlayerPrivateSnapshot NewSnapshot = PrivateSnapshotProvider
		? PrivateSnapshotProvider()
		: FSGSPlayerPrivateSnapshot();
	NewSnapshot.Revision = PrivateSnapshot.Revision + 1;
	NewSnapshot.ViewerSeat = ViewerSeat;
	PrivateSnapshot = MoveTemp(NewSnapshot);
	RefreshTableFeature();
}

FSGSTableViewSnapshot ASGSPlayerController::BuildTableViewSnapshot() const
{
	FSGSTablePublicSnapshot PublicSnapshot;
	if (const UWorld* World = GetWorld())
	{
		if (const ASGSGameState* SGSGameState = World->GetGameState<ASGSGameState>())
		{
			PublicSnapshot = SGSGameState->GetPublicSnapshot();
		}
	}

	FSGSPlayerPrivateSnapshot LocalPrivateSnapshot = PrivateSnapshot;
	if (LocalPrivateSnapshot.ViewerSeat == INDEX_NONE)
	{
		LocalPrivateSnapshot.ViewerSeat = ViewerSeat;
	}
	return SGSComposeTableViewSnapshot(PublicSnapshot, LocalPrivateSnapshot);
}

bool ASGSPlayerController::SubmitUseCard(int32 CardId, int32 TargetSeatIndex)
{
	if (HasAuthority())
	{
		return SubmitUseCardOnServer(CardId, TargetSeatIndex);
	}

	ServerSubmitUseCard(CardId, TargetSeatIndex);
	return true;
}

bool ASGSPlayerController::SubmitResponseCard(int32 CardId, int32 TargetSeatIndex)
{
	if (HasAuthority())
	{
		return SubmitResponseCardOnServer(CardId, TargetSeatIndex);
	}

	ServerSubmitResponseCard(CardId, TargetSeatIndex);
	return true;
}

bool ASGSPlayerController::SubmitPass()
{
	if (HasAuthority())
	{
		return SubmitPassOnServer();
	}

	ServerSubmitPass();
	return true;
}

void ASGSPlayerController::ClientAttachLocalHud_Implementation(int32 InViewerSeat)
{
	ViewerSeat = InViewerSeat;
	AttachLocalHud();
}

void ASGSPlayerController::ServerSubmitUseCard_Implementation(int32 CardId, int32 TargetSeatIndex)
{
	SubmitUseCardOnServer(CardId, TargetSeatIndex);
}

void ASGSPlayerController::ServerSubmitResponseCard_Implementation(int32 CardId, int32 TargetSeatIndex)
{
	SubmitResponseCardOnServer(CardId, TargetSeatIndex);
}

void ASGSPlayerController::ServerSubmitPass_Implementation()
{
	SubmitPassOnServer();
}

void ASGSPlayerController::OnRep_PrivateSnapshot()
{
	RefreshTableFeature();
}

void ASGSPlayerController::AttachLocalHud()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UGameViewportClient* ViewportClient = LocalPlayer != nullptr
		? LocalPlayer->ViewportClient
		: nullptr;
	if (!IsLocalController() || LocalPlayer == nullptr || ViewportClient == nullptr)
	{
		return;
	}

	RemoveTableHud();
	UIContext = MakeShared<FSGSUIContext>();
	FSGSTableFeatureBindings Bindings;
	const TWeakObjectPtr<ASGSPlayerController> WeakThis(this);
	Bindings.ReadSnapshot = [WeakThis]()
	{
		return WeakThis.IsValid()
			? WeakThis->BuildTableViewSnapshot()
			: FSGSTableViewSnapshot();
	};
	Bindings.SubmitUseCard = [WeakThis](int32 CardId, int32 TargetSeat)
	{
		return WeakThis.IsValid() && WeakThis->SubmitUseCard(CardId, TargetSeat);
	};
	Bindings.SubmitResponseCard = [WeakThis](int32 CardId, int32 TargetSeat)
	{
		return WeakThis.IsValid() && WeakThis->SubmitResponseCard(CardId, TargetSeat);
	};
	Bindings.SubmitPass = [WeakThis]()
	{
		return WeakThis.IsValid() && WeakThis->SubmitPass();
	};
	TableFeatureController = MakeShared<FSGSTableFeatureController>(
		ViewerSeat,
		MoveTemp(Bindings),
		UIContext.ToSharedRef());
	TableFeatureController->RefreshFromHost();

	SAssignNew(TableHudWidget, SSGSTableHudWidget)
		.Controller(TableFeatureController);
	SAssignNew(ToastLayerWidget, SSGSUIToastLayerWidget)
		.UIContext(UIContext);

	ViewportClient->AddViewportWidgetForPlayer(LocalPlayer, TableHudWidget.ToSharedRef(), 10);
	ViewportClient->AddViewportWidgetForPlayer(LocalPlayer, ToastLayerWidget.ToSharedRef(), 20);
	HudLocalPlayer = LocalPlayer;
	BindTableSnapshotSource();
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

bool ASGSPlayerController::SubmitUseCardOnServer(int32 CardId, int32 TargetSeatIndex)
{
	if (DecisionAgent == nullptr)
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitUseCard ignored: no server decision agent is bound."));
		return false;
	}

	const bool bSubmitted = DecisionAgent->SubmitUseCard(CardId, TargetSeatIndex);
	if (bSubmitted)
	{
		RefreshAfterServerDecision();
	}
	return bSubmitted;
}

bool ASGSPlayerController::SubmitResponseCardOnServer(int32 CardId, int32 TargetSeatIndex)
{
	if (DecisionAgent == nullptr)
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard ignored: no server decision agent is bound."));
		return false;
	}

	const bool bSubmitted = DecisionAgent->SubmitResponseCard(CardId, TargetSeatIndex);
	if (bSubmitted)
	{
		RefreshAfterServerDecision();
	}
	return bSubmitted;
}

bool ASGSPlayerController::SubmitPassOnServer()
{
	if (DecisionAgent == nullptr)
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitPass ignored: no server decision agent is bound."));
		return false;
	}

	const bool bSubmitted = DecisionAgent->SubmitPass();
	if (bSubmitted)
	{
		RefreshAfterServerDecision();
	}
	return bSubmitted;
}

void ASGSPlayerController::RefreshAfterServerDecision()
{
	RefreshPrivateSnapshotFromServer();
	if (ServerViewRefreshHandler)
	{
		ServerViewRefreshHandler();
	}
}

void ASGSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveTableHud();
	Super::EndPlay(EndPlayReason);
}

void ASGSPlayerController::RemoveTableHud()
{
	UnbindTableSnapshotSource();
	ULocalPlayer* LocalPlayer = HudLocalPlayer.Get();
	if (LocalPlayer == nullptr)
	{
		LocalPlayer = GetLocalPlayer();
	}
	UGameViewportClient* ViewportClient = LocalPlayer != nullptr
		? LocalPlayer->ViewportClient
		: nullptr;
	if (TableHudWidget.IsValid() && LocalPlayer != nullptr && ViewportClient != nullptr)
	{
		ViewportClient->RemoveViewportWidgetForPlayer(LocalPlayer, TableHudWidget.ToSharedRef());
	}
	if (ToastLayerWidget.IsValid() && LocalPlayer != nullptr && ViewportClient != nullptr)
	{
		ViewportClient->RemoveViewportWidgetForPlayer(LocalPlayer, ToastLayerWidget.ToSharedRef());
	}
	HudLocalPlayer.Reset();
	TableHudWidget.Reset();
	ToastLayerWidget.Reset();
	TableFeatureController.Reset();
	UIContext.Reset();
}

void ASGSPlayerController::BindTableSnapshotSource()
{
	UnbindTableSnapshotSource();
	ASGSGameState* GameState = GetWorld() != nullptr
		? GetWorld()->GetGameState<ASGSGameState>()
		: nullptr;
	if (GameState == nullptr)
	{
		return;
	}

	ObservedGameState = GameState;
	PublicSnapshotChangedHandle = GameState->OnPublicSnapshotChanged().AddUObject(
		this,
		&ASGSPlayerController::HandlePublicSnapshotChanged);
}

void ASGSPlayerController::UnbindTableSnapshotSource()
{
	if (ASGSGameState* GameState = ObservedGameState.Get();
		GameState != nullptr && PublicSnapshotChangedHandle.IsValid())
	{
		GameState->OnPublicSnapshotChanged().Remove(PublicSnapshotChangedHandle);
	}
	ObservedGameState.Reset();
	PublicSnapshotChangedHandle.Reset();
}

void ASGSPlayerController::HandlePublicSnapshotChanged()
{
	RefreshTableFeature();
}

void ASGSPlayerController::RefreshTableFeature()
{
	if (TableFeatureController.IsValid())
	{
		TableFeatureController->RefreshFromHost();
	}
}
