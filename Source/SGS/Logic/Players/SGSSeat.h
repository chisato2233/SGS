#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptInterface.h"
#include "Logic/Players/SGSDecisionAgent.h"
#include "SGSSeat.generated.h"

// 一个座位（一名玩家在牌桌上的占位）。服务器侧权威对象。
// 座位不关心其决策来自真人还是 AI——只持有一个决策代理。
// 字段随玩法推进扩展（血量、势力、身份、手牌引用…）。
UCLASS()
class SGS_API USGSSeat : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 SeatIndex = INDEX_NONE;

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	bool bIsAlive = true;

	// 真人或 AI 决策代理。
	UPROPERTY()
	TScriptInterface<ISGSDecisionAgent> DecisionAgent;
};
