#pragma once

// 统一判定结算帧。判定牌进入处理区后发布可改判时机，待触发队列完成再把
// 最终牌交还父结算；具体牌或技能负责解释花色并清理判定牌。

#include "Server/Rules/Core/SGSRuleTypes.h"
#include "SGSJudgementResolution.generated.h"

USTRUCT()
struct SGS_API FSGSJudgementResolutionState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 JudgedSeat = INDEX_NONE;

	UPROPERTY()
	FName ReasonName = NAME_None;

	UPROPERTY()
	int32 ResultCardId = INDEX_NONE;
};

namespace SGSJudgementResolution
{
	SGS_API FName ResumeAfterTriggersContinuation();
	SGS_API FSGSStatus Start(
		FSGSRuleExecutionContext& Context,
		int32 JudgedSeat,
		FName ReasonName,
		FName ParentContinuation);
	SGS_API FSGSStatus ContinueAfterTriggers(FSGSRuleExecutionContext& Context);
	SGS_API FSGSJudgementResolutionState* FindActiveState(FSGSResolutionStack& Stack);
}
