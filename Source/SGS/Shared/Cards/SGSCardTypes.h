#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Shared/Core/SGSTypes.h"
#include "GameplayTagContainer.h"

// 装备栏与牌区是开放规则概念：标准槽位/区域由 SGSGameplayTags 声明，
// 后续模式可以通过 GameplayTag / 注册表追加。
using FSGSEquipSlot = FGameplayTag;
using FSGSCardZone = FGameplayTag;

struct SGS_API FSGSCardPileKey
{
	FSGSCardZone Zone = SGSGameplayTags::CardZone_None.GetTag();
	int32 SeatIndex = INDEX_NONE;

	FSGSCardPileKey() = default;
	FSGSCardPileKey(FSGSCardZone InZone, int32 InSeatIndex)
		: Zone(InZone)
		, SeatIndex(InSeatIndex)
	{
	}

	bool IsValid() const
	{
		return Zone.IsValid();
	}

	FString ToLogString() const
	{
		return FString::Printf(TEXT("%s@%d"), *Zone.ToString(), SeatIndex);
	}

	friend bool operator==(const FSGSCardPileKey& Left, const FSGSCardPileKey& Right)
	{
		return Left.Zone.MatchesTagExact(Right.Zone) && Left.SeatIndex == Right.SeatIndex;
	}

	friend bool operator!=(const FSGSCardPileKey& Left, const FSGSCardPileKey& Right)
	{
		return !(Left == Right);
	}
};

inline uint32 GetTypeHash(const FSGSCardPileKey& Key)
{
	return HashCombine(GetTypeHash(Key.Zone), GetTypeHash(Key.SeatIndex));
}

struct SGS_API FSGSCardZoneNameKey
{
	FSGSCardZone Zone = SGSGameplayTags::CardZone_None.GetTag();
	FName CardName = NAME_None;

	FSGSCardZoneNameKey() = default;
	FSGSCardZoneNameKey(FSGSCardZone InZone, FName InCardName)
		: Zone(InZone)
		, CardName(InCardName)
	{
	}

	friend bool operator==(const FSGSCardZoneNameKey& Left, const FSGSCardZoneNameKey& Right)
	{
		return Left.Zone.MatchesTagExact(Right.Zone) && Left.CardName == Right.CardName;
	}
};

inline uint32 GetTypeHash(const FSGSCardZoneNameKey& Key)
{
	return HashCombine(GetTypeHash(Key.Zone), GetTypeHash(Key.CardName));
}

struct SGS_API FSGSCardZoneTypeKey
{
	FSGSCardZone Zone = SGSGameplayTags::CardZone_None.GetTag();
	FSGSCardType CardType = SGSGameplayTags::CardType_None.GetTag();

	FSGSCardZoneTypeKey() = default;
	FSGSCardZoneTypeKey(FSGSCardZone InZone, FSGSCardType InCardType)
		: Zone(InZone)
		, CardType(InCardType)
	{
	}

	friend bool operator==(const FSGSCardZoneTypeKey& Left, const FSGSCardZoneTypeKey& Right)
	{
		return Left.Zone.MatchesTagExact(Right.Zone) && Left.CardType.MatchesTagExact(Right.CardType);
	}
};

inline uint32 GetTypeHash(const FSGSCardZoneTypeKey& Key)
{
	return HashCombine(GetTypeHash(Key.Zone), GetTypeHash(Key.CardType));
}
