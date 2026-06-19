#pragma once

#include "CoreMinimal.h"
#include "SGSTypes.generated.h"

// 三国杀核心通用类型。仅放跨层、长期稳定的基础枚举/常量；
// 子系统专属类型放各自层目录。BlueprintReadOnly 风格见 Rulers.md 2.3。

// 花色
UENUM(BlueprintType)
enum class ESGSSuit : uint8
{
	None    UMETA(DisplayName = "无"),
	Spade   UMETA(DisplayName = "黑桃"),
	Heart   UMETA(DisplayName = "红桃"),
	Club    UMETA(DisplayName = "梅花"),
	Diamond UMETA(DisplayName = "方块"),
};

// 牌面颜色（由花色推导，见 SGSSuitToColor）
UENUM(BlueprintType)
enum class ESGSCardColor : uint8
{
	None  UMETA(DisplayName = "无"),
	Red   UMETA(DisplayName = "红"),
	Black UMETA(DisplayName = "黑"),
};

// 牌的大类
UENUM(BlueprintType)
enum class ESGSCardType : uint8
{
	None      UMETA(DisplayName = "无"),
	Basic     UMETA(DisplayName = "基本牌"),
	Trick     UMETA(DisplayName = "锦囊牌"),
	Equipment UMETA(DisplayName = "装备牌"),
};

// 回合阶段（标准回合推进顺序）
UENUM(BlueprintType)
enum class ESGSPhase : uint8
{
	None       UMETA(DisplayName = "无"),
	RoundStart UMETA(DisplayName = "回合开始"),
	Judge      UMETA(DisplayName = "判定阶段"),
	Draw       UMETA(DisplayName = "摸牌阶段"),
	Play       UMETA(DisplayName = "出牌阶段"),
	Discard    UMETA(DisplayName = "弃牌阶段"),
	RoundEnd   UMETA(DisplayName = "回合结束"),
};

// 势力
UENUM(BlueprintType)
enum class ESGSFaction : uint8
{
	None UMETA(DisplayName = "无"),
	Wei  UMETA(DisplayName = "魏"),
	Shu  UMETA(DisplayName = "蜀"),
	Wu   UMETA(DisplayName = "吴"),
	Qun  UMETA(DisplayName = "群"),
};

// 黑桃/梅花为黑，红桃/方块为红。无花色（如无花色转化牌）返回 None。
inline ESGSCardColor SGSSuitToColor(ESGSSuit Suit)
{
	switch (Suit)
	{
	case ESGSSuit::Spade:
	case ESGSSuit::Club:
		return ESGSCardColor::Black;
	case ESGSSuit::Heart:
	case ESGSSuit::Diamond:
		return ESGSCardColor::Red;
	default:
		return ESGSCardColor::None;
	}
}
