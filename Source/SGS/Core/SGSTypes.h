#pragma once

#include "CoreMinimal.h"
#include "Core/SGSGameplayTags.h"

// 三国杀核心通用类型。规则概念必须保持开放扩展：阶段、花色、势力、牌类等
// 使用 GameplayTag / FName / RowName / 注册表，而不是封闭 enum。
// 子系统专属类型放各自层目录。BlueprintReadOnly 风格见 Rulers.md 2.3。

using FSGSRuleTag = FGameplayTag;

// 标准花色/阶段等只作为内置标签存在，不代表全集。
using FSGSSuit = FSGSRuleTag;
using FSGSCardColor = FSGSRuleTag;
using FSGSCardType = FSGSRuleTag;
using FSGSPhase = FSGSRuleTag;
using FSGSFaction = FSGSRuleTag;

inline bool SGSMatchesExactTag(const FGameplayTag& Value, const FNativeGameplayTag& Expected)
{
	return Value.MatchesTagExact(Expected.GetTag());
}

// 黑桃/梅花为黑，红桃/方块为红。无花色（如无花色转化牌）返回 None。
inline FSGSCardColor SGSSuitToColor(const FSGSSuit& Suit)
{
	if (SGSMatchesExactTag(Suit, SGSGameplayTags::Suit_Spade)
		|| SGSMatchesExactTag(Suit, SGSGameplayTags::Suit_Club))
	{
		return SGSGameplayTags::CardColor_Black.GetTag();
	}

	if (SGSMatchesExactTag(Suit, SGSGameplayTags::Suit_Heart)
		|| SGSMatchesExactTag(Suit, SGSGameplayTags::Suit_Diamond))
	{
		return SGSGameplayTags::CardColor_Red.GetTag();
	}

	return SGSGameplayTags::CardColor_None.GetTag();
}
