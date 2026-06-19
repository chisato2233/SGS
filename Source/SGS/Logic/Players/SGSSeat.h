#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptInterface.h"
#include "Core/SGSTypes.h"
#include "Logic/Cards/SGSCardTypes.h"
#include "Logic/Players/SGSDecisionAgent.h"
#include "SGSSeat.generated.h"

class USGSCard;
class USGSCardPile;

// 一个座位（一名玩家在牌桌上的占位）。服务器侧权威对象。
// 座位不关心其决策来自真人还是 AI——只持有一个决策代理。
UCLASS()
class SGS_API USGSSeat : public UObject
{
	GENERATED_BODY()

public:
	// ---- 身份 ----
	UPROPERTY()
	int32 SeatIndex = INDEX_NONE;

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	ESGSFaction Faction = ESGSFaction::None;

	// ---- 生命 ----
	// bIsAlive 由对局逻辑在体力归零并完成濒死结算后置否（濒死/求桃见 Plan 0005）。
	UPROPERTY()
	bool bIsAlive = true;

	UPROPERTY()
	int32 MaxHealth = 0;

	UPROPERTY()
	int32 Health = 0;

	// ---- 牌区 ----
	UPROPERTY()
	TObjectPtr<USGSCardPile> Hand;

	UPROPERTY()
	TObjectPtr<USGSCardPile> JudgementZone;

	// 装备区：每栏至多一张。
	UPROPERTY()
	TMap<ESGSEquipSlot, TObjectPtr<USGSCard>> Equipment;

	// ---- 决策 ----
	UPROPERTY()
	TScriptInterface<ISGSDecisionAgent> DecisionAgent;
};
