#pragma once

#include "CoreMinimal.h"

// 决策代理的请求/应答数据。纯 C++（无反射）：仅在服务器逻辑层与代理之间传递。
// 随玩法推进逐步扩展（出牌、用技、选目标、弃牌…）。

// 出牌阶段可选的动作类型。骨架期仅 Pass。
enum class ESGSPlayActionType : uint8
{
	Pass,
	// 后续：PlayCard, UseSkill, UseEquipmentActive, ...
};

// 服务器向某座位发起的「出牌阶段动作」请求。
struct FSGSPlayPhaseRequest
{
	// 被询问的座位。
	int32 SeatIndex = INDEX_NONE;

	// 单调递增的请求号，用于丢弃过期/重复应答。
	int32 RequestId = 0;
};

// 座位对「出牌阶段动作」请求的应答。
struct FSGSPlayPhaseDecision
{
	ESGSPlayActionType Action = ESGSPlayActionType::Pass;
};

// 异步应答回调：代理决定后调用，回传决策结果。
DECLARE_DELEGATE_OneParam(FSGSPlayPhaseDecisionDelegate, const FSGSPlayPhaseDecision& /*Decision*/);
