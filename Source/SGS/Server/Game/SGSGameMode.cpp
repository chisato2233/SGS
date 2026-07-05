#include "Server/Game/SGSGameMode.h"

#include "Server/AI/SGSBasicAIAgent.h"
#include "Shared/Core/SGSLogChannels.h"
#include "Client/Game/SGSPlayerController.h"
#include "Client/Game/SGSTablePawn.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Server/Engine/SGSGameDriver.h"
#include "Server/UI/SGSTableSnapshotBuilder.h"
#include "Shared/Decisions/SGSDecisionAgent.h"
#include "Shared/Game/SGSGameState.h"
#include "Shared/Game/SGSPlayerState.h"
#include "Misc/App.h"
#include "Client/UI/Bridge/SGSLocalHumanDecisionAgent.h"

ASGSGameMode::ASGSGameMode()
{
	PlayerControllerClass = ASGSPlayerController::StaticClass();
	DefaultPawnClass = ASGSTablePawn::StaticClass();
	GameStateClass = ASGSGameState::StaticClass();
	PlayerStateClass = ASGSPlayerState::StaticClass();
}

void ASGSGameMode::SpawnDevelopmentTableScene()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh == nullptr)
	{
		UE_LOG(LogSGSUI, Warning, TEXT("Development table scene skipped: /Engine/BasicShapes/Cube is missing."));
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* TableActor = World->SpawnActor<AStaticMeshActor>(
		FVector(0.0f, 0.0f, -8.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (TableActor == nullptr || TableActor->GetStaticMeshComponent() == nullptr)
	{
		return;
	}

	UStaticMeshComponent* MeshComponent = TableActor->GetStaticMeshComponent();
	MeshComponent->SetStaticMesh(CubeMesh);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TableActor->SetActorScale3D(FVector(12.0f, 7.0f, 0.08f));
#if WITH_EDITOR
	TableActor->SetActorLabel(TEXT("SGS Development Table"));
#endif
}

void ASGSGameMode::RefreshViewSnapshots()
{
	if (ASGSGameState* SGSGameState = GetGameState<ASGSGameState>())
	{
		SGSGameState->PublishPublicSnapshot(FSGSTableSnapshotBuilder::BuildPublicSnapshot(GameDriver));
	}

	if (LocalHumanPlayerController != nullptr)
	{
		LocalHumanPlayerController->RefreshPrivateSnapshotFromServer();
	}
}

void ASGSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 防御性：GameMode 本就只在服务器存在，这里显式表达「权威逻辑只在服务器跑」的约束。
	if (!HasAuthority())
	{
		return;
	}

	SpawnDevelopmentTableScene();

	const int32 SeatCount = FMath::Max(NumSeats, 1);
	LocalHumanPlayerController = Cast<ASGSPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	const bool bUseLocalHuman =
		!FApp::IsUnattended()
		&& !IsRunningCommandlet()
		&& GetNetMode() != NM_DedicatedServer
		&& LocalHumanPlayerController != nullptr
		&& LocalHumanPlayerController->IsLocalController();

	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	Agents.Reserve(SeatCount);
	for (int32 Index = 0; Index < SeatCount; ++Index)
	{
		UObject* AgentObject = nullptr;
		if (Index == 0 && bUseLocalHuman)
		{
			LocalHumanAgent = NewObject<USGSLocalHumanDecisionAgent>(this);
			AgentObject = LocalHumanAgent;
		}
		else
		{
			AgentObject = NewObject<USGSBasicAIAgent>(this);
		}

		TScriptInterface<ISGSDecisionAgent> AgentRef;
		AgentRef.SetObject(AgentObject);
		AgentRef.SetInterface(Cast<ISGSDecisionAgent>(AgentObject));
		Agents.Add(AgentRef);
	}

	UE_LOG(LogSGS, Log, TEXT("SGS match starting with %d seats. LocalHuman=%s"),
		SeatCount,
		bUseLocalHuman ? TEXT("true") : TEXT("false"));

	GameDriver = NewObject<USGSGameDriver>(this);

	if (bUseLocalHuman && LocalHumanPlayerController != nullptr && LocalHumanAgent != nullptr)
	{
		TWeakObjectPtr<USGSGameDriver> WeakGameDriver = GameDriver.Get();
		TWeakObjectPtr<USGSLocalHumanDecisionAgent> WeakLocalHumanAgent = LocalHumanAgent.Get();
		TWeakObjectPtr<ASGSGameMode> WeakGameMode = this;
		LocalHumanPlayerController->AttachToMatch(
			LocalHumanAgent,
			0,
			[WeakGameDriver, WeakLocalHumanAgent]()
			{
				return FSGSTableSnapshotBuilder::BuildPrivateSnapshot(
					WeakGameDriver.Get(),
					WeakLocalHumanAgent.Get(),
					0);
			},
			[WeakGameMode]()
			{
				if (WeakGameMode.IsValid())
				{
					WeakGameMode->RefreshViewSnapshots();
				}
			});
	}

	if (LocalHumanPlayerController != nullptr)
	{
		if (ASGSPlayerState* SGSPlayerState = LocalHumanPlayerController->GetPlayerState<ASGSPlayerState>())
		{
			SGSPlayerState->SetSeatIndex(
				bUseLocalHuman
					? 0
					: INDEX_NONE);
		}
	}

	FSGSGameStartConfig Config;
	Config.RandomSeed = 1;
	Config.InitialDeck = SGSDeckDefinitions::MakePlan0005SmokeDeck(SeatCount);
	Config.bShuffleInitialDeck = false;
	GameDriver->StartGame(Agents, Config);
	RefreshViewSnapshots();
}
