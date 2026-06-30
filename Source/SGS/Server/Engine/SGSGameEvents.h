#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/SGSTypes.h"

// 事件总线的最小形态：对局关键节点广播，技能/观察者按时机介入。
// 触发技的细粒度时机（出牌前/受伤后…）随技能系统在后续 Plan 扩展。

using FSGSGameEvent = FSGSRuleTag;

struct FSGSEventContext
{
	FSGSGameEvent Event = SGSGameplayTags::GameEvent_GameStarted.GetTag();

	// 事件相关的座位（当前行动者）；无关时为 INDEX_NONE。
	int32 SeatIndex = INDEX_NONE;

	// 事件相关的阶段；无关时为 None。
	FSGSPhase Phase = SGSGameplayTags::Phase_None.GetTag();
};

// C++ 多播委托（不暴露蓝图，符合纯 C++ 约束）。
DECLARE_MULTICAST_DELEGATE_OneParam(FSGSOnGameEvent, const FSGSEventContext& /*Context*/);
