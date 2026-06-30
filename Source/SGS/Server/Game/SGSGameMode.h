#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGSGameMode.generated.h"

class USGSGameDriver;
class USGSLocalHumanDecisionAgent;
class ASGSPlayerController;

// 服务器权威入口：GameMode 只存在于服务器，是对局逻辑的落地点。
// 本地开发运行使用 seat 0 的本地人类决策代理；无人值守与服务器环境使用 AI，避免测试阻塞。
UCLASS()
class SGS_API ASGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASGSGameMode();
	void RefreshViewSnapshots();

protected:
	virtual void BeginPlay() override;

	// 骨架期座位数。
	UPROPERTY(EditDefaultsOnly, Category = "SGS")
	int32 NumSeats = 4;

private:
	UPROPERTY()
	TObjectPtr<USGSGameDriver> GameDriver;

	UPROPERTY()
	TObjectPtr<USGSLocalHumanDecisionAgent> LocalHumanAgent;

	UPROPERTY()
	TObjectPtr<ASGSPlayerController> LocalHumanPlayerController;
};
