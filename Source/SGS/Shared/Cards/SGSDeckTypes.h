#pragma once

// 牌库定义的最小运行时入口。GameDriver 只消费这里的牌规格，不硬编码具体牌库。
// 正式内容后续可由 DataTable / DataAsset 生成这些规格；测试可构造隔离的确定性牌库。

#include "CoreMinimal.h"
#include "Shared/Core/SGSTypes.h"

struct SGS_API FSGSDeckCardSpec
{
	FName CardName = NAME_None;
	FSGSSuit Suit = SGSGameplayTags::Suit_None.GetTag();
	int32 Number = 0;
	FSGSCardType CardType = SGSGameplayTags::CardType_None.GetTag();
	int32 Count = 1;
};

namespace SGSDeckDefinitions
{
	SGS_API TArray<FSGSDeckCardSpec> MakePlan0005SmokeDeck(int32 SeatCount);
	SGS_API TArray<FSGSDeckCardSpec> MakeMinimalIdentityDeck();
}
