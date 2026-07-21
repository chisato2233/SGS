#pragma once

// 统一伤害结算帧：扣减体力、派发 DamageAfter、等待可选触发、进入濒死，
// 完成后按父帧 continuation 恢复原卡牌或技能结算。

#include "Server/Rules/Core/SGSRuleTypes.h"
#include "SGSDamageResolution.generated.h"

USTRUCT()
struct SGS_API FSGSDamageResolutionState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SourceSeat = INDEX_NONE;

	UPROPERTY()
	int32 TargetSeat = INDEX_NONE;

	UPROPERTY()
	int32 Amount = 0;

	UPROPERTY()
	int32 CardId = INDEX_NONE;
};

namespace SGSDamageResolution
{
	SGS_API FName ResumeAfterTriggersContinuation();
	SGS_API FName FinishAfterDyingContinuation();
	SGS_API FSGSStatus Start(
		FSGSRuleExecutionContext& Context,
		int32 SourceSeat,
		int32 TargetSeat,
		int32 Amount,
		int32 CardId,
		FName ParentContinuation);
	SGS_API FSGSStatus ContinueAfterTriggers(FSGSRuleExecutionContext& Context);
}
