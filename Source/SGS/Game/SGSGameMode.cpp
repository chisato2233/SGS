#include "Game/SGSGameMode.h"

#include "AI/SGSBasicAIAgent.h"
#include "Core/SGSLogChannels.h"
#include "Game/SGSPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Logic/Engine/SGSGameDriver.h"
#include "Logic/Players/SGSDecisionAgent.h"
#include "Misc/App.h"
#include "UI/Bridge/SGSLocalHumanDecisionAgent.h"

ASGSGameMode::ASGSGameMode()
{
	PlayerControllerClass = ASGSPlayerController::StaticClass();
}

void ASGSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 防御性：GameMode 本就只在服务器存在，这里显式表达「权威逻辑只在服务器跑」的约束。
	if (!HasAuthority())
	{
		return;
	}

	const int32 SeatCount = FMath::Max(NumSeats, 1);
	ASGSPlayerController* LocalPlayerController = Cast<ASGSPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	const bool bUseLocalHuman =
		!FApp::IsUnattended()
		&& !IsRunningCommandlet()
		&& GetNetMode() != NM_DedicatedServer
		&& LocalPlayerController != nullptr
		&& LocalPlayerController->IsLocalController();

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

	if (bUseLocalHuman && LocalPlayerController != nullptr && LocalHumanAgent != nullptr)
	{
		LocalPlayerController->AttachToMatch(GameDriver, LocalHumanAgent, 0);
	}

	FSGSGameStartConfig Config;
	Config.RandomSeed = 1;
	Config.InitialDeck = SGSDeckDefinitions::MakePlan0005SmokeDeck(SeatCount);
	Config.bShuffleInitialDeck = false;
	GameDriver->StartGame(Agents, Config);
}
