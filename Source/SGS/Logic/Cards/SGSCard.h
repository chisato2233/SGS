#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/SGSTypes.h"
#include "SGSCard.generated.h"

// 一张物理牌的运行态实例。CardId 在一局内唯一，用于稳定标识与（未来）网络同步。
UCLASS()
class SGS_API USGSCard : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 CardId = INDEX_NONE;

	// 牌名标识，关联 FSGSCardDef。
	UPROPERTY()
	FName CardName;

	UPROPERTY(meta = (Categories = "SGS.Suit"))
	FGameplayTag Suit;

	// 点数 1-13（A=1，J=11，Q=12，K=13）。
	UPROPERTY()
	int32 Number = 0;

	UPROPERTY(meta = (Categories = "SGS.CardType"))
	FGameplayTag CardType = SGSGameplayTags::CardType_None.GetTag();

	FSGSCardColor GetColor() const { return SGSSuitToColor(Suit); }
};
