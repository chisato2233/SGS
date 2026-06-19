#pragma once

#include "CoreMinimal.h"
#include "Core/SGSTypes.h"

// 事件总线的最小形态：对局关键节点广播，技能/观察者按时机介入。
// 触发技的细粒度时机（出牌前/受伤后…）随技能系统在后续 Plan 扩展。

enum class ESGSGameEvent : uint8
{
	GameStarted,
	GameEnded,
	TurnBegan,
	TurnEnded,
	PhaseBegan,
	PhaseEnded,
};

struct FSGSEventContext
{
	ESGSGameEvent Event = ESGSGameEvent::GameStarted;

	// 事件相关的座位（当前行动者）；无关时为 INDEX_NONE。
	int32 SeatIndex = INDEX_NONE;

	// 事件相关的阶段；无关时为 None。
	ESGSPhase Phase = ESGSPhase::None;
};

// C++ 多播委托（不暴露蓝图，符合纯 C++ 约束）。
DECLARE_MULTICAST_DELEGATE_OneParam(FSGSOnGameEvent, const FSGSEventContext& /*Context*/);
