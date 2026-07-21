#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGSGameMode.generated.h"

class USGSGameDriver;
class USGSLocalHumanDecisionAgent;
class ASGSPlayerController;

// 服务器权威入口：GameMode 只存在于服务器，是对局逻辑的落地点。
// 本地开发运行使用随机座次的本地人类代理；无人值守与服务器环境由 AI 推进。
UCLASS()
class SGS_API ASGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASGSGameMode();
	void RefreshViewSnapshots();

protected:
	virtual void BeginPlay() override;

private:
	void SpawnDevelopmentTableScene();
	void ScheduleViewSnapshotRefresh();

	UPROPERTY()
	TObjectPtr<USGSGameDriver> GameDriver;

	UPROPERTY()
	TObjectPtr<USGSLocalHumanDecisionAgent> LocalHumanAgent;

	UPROPERTY()
	TObjectPtr<ASGSPlayerController> LocalHumanPlayerController;

	FTimerHandle ViewSnapshotRefreshTimer;
	bool bViewSnapshotRefreshScheduled = false;
};
