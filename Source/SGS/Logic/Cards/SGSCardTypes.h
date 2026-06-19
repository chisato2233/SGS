#pragma once

#include "CoreMinimal.h"
#include "SGSCardTypes.generated.h"

// 装备栏。一名角色每栏至多一张。
UENUM(BlueprintType)
enum class ESGSEquipSlot : uint8
{
	None         UMETA(DisplayName = "无"),
	Weapon       UMETA(DisplayName = "武器"),
	Armor        UMETA(DisplayName = "防具"),
	DefenseHorse UMETA(DisplayName = "+1马"),
	OffenseHorse UMETA(DisplayName = "-1马"),
};

// 牌所在的区域。通用移动原语据此定位目标牌区。
UENUM(BlueprintType)
enum class ESGSCardZone : uint8
{
	None        UMETA(DisplayName = "无"),
	DrawPile    UMETA(DisplayName = "牌堆"),
	DiscardPile UMETA(DisplayName = "弃牌堆"),
	Hand        UMETA(DisplayName = "手牌"),
	Equipment   UMETA(DisplayName = "装备区"),
	Judgement   UMETA(DisplayName = "判定区"),
	Processing  UMETA(DisplayName = "处理区"),
};
