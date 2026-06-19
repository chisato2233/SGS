#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGSGameMode.generated.h"

class USGSGameDriver;

// 服务器权威入口：GameMode 只存在于服务器，是对局逻辑的落地点。
// 骨架期用纯 AI 座位跑通一局空对局；真人座位 / 复制 / RPC 决策通道见后续网络 Plan。
UCLASS()
class SGS_API ASGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	// 骨架期座位数（全部由占位 AI 代理控制）。
	UPROPERTY(EditDefaultsOnly, Category = "SGS")
	int32 NumSeats = 4;

private:
	UPROPERTY()
	TObjectPtr<USGSGameDriver> GameDriver;
};
