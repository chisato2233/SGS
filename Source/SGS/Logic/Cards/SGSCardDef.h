#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Core/SGSTypes.h"
#include "Logic/Cards/SGSCardTypes.h"
#include "SGSCardDef.generated.h"

// 牌的静态定义（DataTable 行）。一张「牌名」对应一条定义；牌堆里多张物理牌可共享同名定义。
// 数据驱动：完整标准牌库以 DataTable 资产维护（见 Plan 0004 说明），代码不硬编码全套。
USTRUCT(BlueprintType)
struct SGS_API FSGSCardDef : public FTableRowBase
{
	GENERATED_BODY()

	// 牌名标识（如 Slash / Dodge / Peach / Dismantle …）。运行态牌据此关联本定义。
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName CardName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "SGS.CardType"))
	FGameplayTag CardType;

	// 装备牌专用：占用的装备栏。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "SGS.EquipSlot"))
	FGameplayTag EquipSlot;

	// 武器专用：攻击范围（非武器忽略）。
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 AttackRange = 0;

	// 坐骑专用：距离修正（-1 马使自己计算到他人距离 -1；+1 马使他人到自己距离 +1）。
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 DistanceModifier = 0;
};
