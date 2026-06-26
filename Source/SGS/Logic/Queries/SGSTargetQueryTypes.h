#pragma once

// 规则、AI 与 UI 提示共用的目标查询契约。
// GameContext 通过索引 Store 解析请求，同时返回可用目标与拒绝原因。
// 目标语义在这里保持数据化，避免卡牌效果、UI、AI 各自实现一套筛选。

#include "CoreMinimal.h"
#include "Core/SGSIds.h"
#include "Core/SGSTypes.h"
#include "Logic/Cards/SGSCardTypes.h"

class USGSCard;

struct SGS_API FSGSTargetQueryRejection
{
	FSGSStableHandle Handle;
	int32 SeatIndex = INDEX_NONE;
	int32 CardId = INDEX_NONE;
	FName Reason = NAME_None;
	FString Detail;
};

struct SGS_API FSGSSeatTarget
{
	FSGSStableHandle Handle;
	int32 SeatIndex = INDEX_NONE;
	int32 Distance = INDEX_NONE;
};

struct SGS_API FSGSSeatQuery
{
	int32 SourceSeat = INDEX_NONE;
	bool bIncludeSource = false;
	bool bAliveOnly = true;
	int32 MaxDistance = INDEX_NONE;
	FGameplayTag RequiredFaction;
};

struct SGS_API FSGSSeatQueryResult
{
	TArray<FSGSSeatTarget> Targets;
	TArray<FSGSTargetQueryRejection> Rejections;
};

struct SGS_API FSGSCardTarget
{
	FSGSStableHandle Handle;
	int32 CardId = INDEX_NONE;
	TObjectPtr<USGSCard> Card = nullptr;
	FSGSCardZone Zone = SGSGameplayTags::CardZone_None.GetTag();
	int32 SeatIndex = INDEX_NONE;
};

struct SGS_API FSGSCardQuery
{
	FSGSCardZone Zone = SGSGameplayTags::CardZone_None.GetTag();
	int32 SeatIndex = INDEX_NONE;
	FName RequiredCardName = NAME_None;
	FSGSCardType RequiredCardType;
	FSGSSuit RequiredSuit;
	int32 RequiredNumber = INDEX_NONE;

	// INDEX_NONE means server-authority/full visibility. A concrete seat means player-view visibility.
	int32 ViewerSeatIndex = INDEX_NONE;
	bool bRequireVisible = true;
};

struct SGS_API FSGSCardQueryResult
{
	TArray<FSGSCardTarget> Targets;
	TArray<FSGSTargetQueryRejection> Rejections;
};
