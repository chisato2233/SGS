#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

// 装备栏与牌区是开放规则概念：标准槽位/区域由 SGSGameplayTags 声明，
// 后续模式可以通过 GameplayTag / 注册表追加。
using FSGSEquipSlot = FGameplayTag;
using FSGSCardZone = FGameplayTag;
