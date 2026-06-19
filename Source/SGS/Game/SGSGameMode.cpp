#include "Game/SGSGameMode.h"

#include "Logic/Engine/SGSGameDriver.h"
#include "Logic/Players/SGSDecisionAgent.h"
#include "AI/SGSAutoPassAgent.h"
#include "Core/SGSLogChannels.h"

void ASGSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 防御性：GameMode 本就只在服务器存在，这里显式表达「权威逻辑只在服务器跑」的约束。
	if (!HasAuthority())
	{
		return;
	}

	const int32 SeatCount = FMath::Max(NumSeats, 1);

	TArray<TScriptInterface<ISGSDecisionAgent>> Agents;
	Agents.Reserve(SeatCount);
	for (int32 Index = 0; Index < SeatCount; ++Index)
	{
		USGSAutoPassAgent* Agent = NewObject<USGSAutoPassAgent>(this);

		TScriptInterface<ISGSDecisionAgent> AgentRef;
		AgentRef.SetObject(Agent);
		AgentRef.SetInterface(Cast<ISGSDecisionAgent>(Agent));
		Agents.Add(AgentRef);
	}

	UE_LOG(LogSGS, Log, TEXT("SGS skeleton match starting with %d AI seats."), SeatCount);

	GameDriver = NewObject<USGSGameDriver>(this);
	GameDriver->StartGame(Agents);
}
