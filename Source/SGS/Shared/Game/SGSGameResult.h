#pragma once

// 身份局的最小终局事实；空结果表示对局仍在进行。
// 胜方座次包含同身份的已死亡玩家，供 UI 直接判断本地胜负。

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SGSGameResult.generated.h"

USTRUCT()
struct SGS_API FSGSGameResult
{
	GENERATED_BODY()

	UPROPERTY()
	FName EndReason = NAME_None;

	UPROPERTY()
	TArray<FGameplayTag> WinningIdentities;

	UPROPERTY()
	TArray<int32> WinningSeatIndices;

	bool IsFinished() const { return !EndReason.IsNone(); }
};
