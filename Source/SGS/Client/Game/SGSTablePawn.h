#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SGSTablePawn.generated.h"

class UCameraComponent;
class USceneComponent;

// 最小牌桌观察 Pawn：只提供固定正交摄像机，不承载移动、输入或规则状态。
// 牌桌交互仍由 PlayerController 挂载的 Slate HUD 收集并提交到服务器权威逻辑。
UCLASS()
class SGS_API ASGSTablePawn : public APawn
{
	GENERATED_BODY()

public:
	ASGSTablePawn();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "SGS")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "SGS")
	TObjectPtr<UCameraComponent> TableCamera;
};
