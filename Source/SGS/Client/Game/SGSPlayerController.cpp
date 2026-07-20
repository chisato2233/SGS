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
	DOREPLIFETIME(ASGSPlayerController, InitialMotionSequence);
}

void ASGSPlayerController::AttachToMatch(
	USGSLocalHumanDecisionAgent* InDecisionAgent,
	int32 InViewerSeat,
	TFunction<FSGSPlayerPrivateSnapshot()> InPrivateSnapshotProvider,
	TFunction<void()> InServerViewRefreshHandler,
	int32 InInitialMotionSequence)
{
	DecisionAgent = InDecisionAgent;
	ViewerSeat = InViewerSeat;
	InitialMotionSequence = InInitialMotionSequence;
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
		ClientAttachLocalHud(ViewerSeat, InitialMotionSequence);
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
	return SubmitUseCard(
		CardId,
		TargetSeatIndex == INDEX_NONE ? TArray<int32>() : TArray<int32>{ TargetSeatIndex });
}

bool ASGSPlayerController::SubmitUseCard(int32 CardId, TArray<int32> TargetSeatIndices)
{
	if (HasAuthority())
	{
		return SubmitUseCardOnServer(CardId, MoveTemp(TargetSeatIndices));
	}

	ServerSubmitUseCardTargets(CardId, TargetSeatIndices);
	return true;
}

bool ASGSPlayerController::SubmitSkill(FName SkillName, TArray<int32> CardIds, int32 TargetSeatIndex)
{
	if (HasAuthority())
	{
		return SubmitSkillOnServer(SkillName, MoveTemp(CardIds), TargetSeatIndex);
	}

	ServerSubmitSkill(SkillName, CardIds, TargetSeatIndex);
	return true;
}

bool ASGSPlayerController::SubmitResponseCard(int32 CardId, int32 TargetSeatIndex, FName SkillName)
{
	TArray<int32> CardIds;
	if (CardId != INDEX_NONE)
	{
		CardIds.Add(CardId);
	}
	return SubmitResponseCards(MoveTemp(CardIds), TargetSeatIndex, SkillName);
}

bool ASGSPlayerController::SubmitResponseCards(TArray<int32> CardIds, int32 TargetSeatIndex, FName SkillName)
{
	if (HasAuthority())
	{
		return SubmitResponseCardsOnServer(MoveTemp(CardIds), TargetSeatIndex, SkillName);
	}

	ServerSubmitResponseCards(CardIds, TargetSeatIndex, SkillName);
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

void ASGSPlayerController::ClientAttachLocalHud_Implementation(int32 InViewerSeat, int32 InInitialMotionSequence)
{
	ViewerSeat = InViewerSeat;
	InitialMotionSequence = InInitialMotionSequence;
	AttachLocalHud();
}

void ASGSPlayerController::ServerSubmitUseCard_Implementation(int32 CardId, int32 TargetSeatIndex)
{
	SubmitUseCardOnServer(CardId, TargetSeatIndex);
}

void ASGSPlayerController::ServerSubmitUseCardTargets_Implementation(
	int32 CardId,
	const TArray<int32>& TargetSeatIndices)
{
	SubmitUseCardOnServer(CardId, TargetSeatIndices);
}

void ASGSPlayerController::ServerSubmitSkill_Implementation(
	FName SkillName,
	const TArray<int32>& CardIds,
	int32 TargetSeatIndex)
{
	SubmitSkillOnServer(SkillName, CardIds, TargetSeatIndex);
}

void ASGSPlayerController::ServerSubmitResponseCards_Implementation(
	const TArray<int32>& CardIds,
	int32 TargetSeatIndex,
	FName SkillName)
{
	SubmitResponseCardsOnServer(CardIds, TargetSeatIndex, SkillName);
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
	Bindings.SubmitUseCard = [WeakThis](int32 CardId, TArray<int32> TargetSeats)
	{
		return WeakThis.IsValid() && WeakThis->SubmitUseCard(CardId, MoveTemp(TargetSeats));
	};
	Bindings.SubmitSkill = [WeakThis](FName SkillName, TArray<int32> CardIds, int32 TargetSeat)
	{
		return WeakThis.IsValid() && WeakThis->SubmitSkill(SkillName, MoveTemp(CardIds), TargetSeat);
	};
	Bindings.SubmitResponseCards = [WeakThis](TArray<int32> CardIds, int32 TargetSeat, FName SkillName)
	{
		return WeakThis.IsValid() && WeakThis->SubmitResponseCards(MoveTemp(CardIds), TargetSeat, SkillName);
	};
	Bindings.SubmitPass = [WeakThis]()
	{
		return WeakThis.IsValid() && WeakThis->SubmitPass();
	};
	TableFeatureController = MakeShared<FSGSTableFeatureController>(
		ViewerSeat,
		MoveTemp(Bindings),
		UIContext.ToSharedRef(),
		InitialMotionSequence);
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
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetWidgetToFocus(TableHudWidget);
	SetInputMode(InputMode);
}

bool ASGSPlayerController::SubmitUseCardOnServer(int32 CardId, int32 TargetSeatIndex)
{
	return SubmitUseCardOnServer(
		CardId,
		TargetSeatIndex == INDEX_NONE ? TArray<int32>() : TArray<int32>{ TargetSeatIndex });
}

bool ASGSPlayerController::SubmitUseCardOnServer(int32 CardId, TArray<int32> TargetSeatIndices)
{
	if (DecisionAgent == nullptr)
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitUseCard ignored: no server decision agent is bound."));
		return false;
	}

	const bool bSubmitted = DecisionAgent->SubmitUseCard(CardId, MoveTemp(TargetSeatIndices));
	if (bSubmitted)
	{
		RefreshAfterServerDecision();
	}
	return bSubmitted;
}

bool ASGSPlayerController::SubmitSkillOnServer(
	FName SkillName,
	TArray<int32> CardIds,
	int32 TargetSeatIndex)
{
	if (DecisionAgent == nullptr)
	{
		return false;
	}

	const bool bSubmitted = DecisionAgent->SubmitSkill(SkillName, MoveTemp(CardIds), TargetSeatIndex);
	if (bSubmitted)
	{
		RefreshAfterServerDecision();
	}
	return bSubmitted;
}

bool ASGSPlayerController::SubmitResponseCardsOnServer(
	TArray<int32> CardIds,
	int32 TargetSeatIndex,
	FName SkillName)
{
	if (DecisionAgent == nullptr)
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard ignored: no server decision agent is bound."));
		return false;
	}

	const bool bSubmitted = DecisionAgent->SubmitResponseCards(MoveTemp(CardIds), TargetSeatIndex, SkillName);
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
