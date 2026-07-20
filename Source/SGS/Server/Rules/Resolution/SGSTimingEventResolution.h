#pragma once

// TimingEvent 触发队列的跨决策状态。RuleRegistry 先给出稳定规则顺序，
// Driver 逐条派发；可选触发打开响应窗口后保留游标，选择完成再继续。

#include "CoreMinimal.h"
#include "Server/Rules/Payloads/SGSRuleEventPayloads.h"
#include "SGSTimingEventResolution.generated.h"

USTRUCT()
struct SGS_API FSGSTimingEventResolutionState
{
	GENERATED_BODY()

	FSGSRuleEventPayload EventPayload;

	UPROPERTY()
	TArray<FName> RuleNames;

	UPROPERTY()
	int32 NextRuleIndex = 0;
};

namespace SGSTimingEventResolution
{
	inline FName FrameRuleName() { return FName(TEXT("SGS.Rule.TimingEvent.Dispatch")); }
	inline FName ResumeDispatchAfterChild() { return FName(TEXT("SGS.Resolution.Continuation.TimingEventAfterChild")); }
}
