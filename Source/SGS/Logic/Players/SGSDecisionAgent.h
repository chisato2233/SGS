#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Logic/Players/SGSDecisionTypes.h"
#include "SGSDecisionAgent.generated.h"

// 决策代理接口：服务器逻辑层需要某座位的输入时只面向本接口。
// 真人由网络/UI 应答，AI 由服务器侧计算；逻辑层不感知对端是人还是 AI。
// 约定：请求一律「异步」——代理在拿到结果时调用 OnDecided，可同步（AI）或跨帧（真人）回调。
UINTERFACE(MinimalAPI)
class USGSDecisionAgent : public UInterface
{
	GENERATED_BODY()
};

class SGS_API ISGSDecisionAgent
{
	GENERATED_BODY()

public:
	// 请求该座位在出牌阶段的一个动作。完成时调用 OnDecided 回传结果。
	virtual void RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided) = 0;

	// 请求该座位响应一个规则窗口（如出闪 / 求桃）。默认 Pass，便于旧代理保持安全行为。
	virtual void RequestResponseAction(const FSGSResponseRequest& Request, FSGSResponseDecisionDelegate OnDecided)
	{
		FSGSResponseDecision Decision;
		Decision.Command = FSGSCommand::MakePass(
			Request.CommandId,
			Request.RequestId,
			Request.SeatIndex,
			Request.Phase,
			FName(TEXT("AgentDefault")),
			Request.WindowName.IsNone() ? FName(TEXT("ResponsePass")) : Request.WindowName);
		OnDecided.ExecuteIfBound(Decision);
	}
};
