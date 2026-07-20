#pragma once

// 护驾、激将共享的同势力依次响应帧。它只负责征询合法响应和记录结果，
// 原始杀/锦囊响应仍由父结算解释成功或失败。

#include "Server/Rules/Core/SGSRuleTypes.h"
#include "SGSLordAssistResolution.generated.h"

USTRUCT()
struct SGS_API FSGSLordAssistResolutionState
{
	GENERATED_BODY()

	UPROPERTY()
	FName SkillName = NAME_None;

	UPROPERTY()
	int32 LordSeat = INDEX_NONE;

	UPROPERTY()
	FName RequiredCardName = NAME_None;

	UPROPERTY()
	int32 EffectTargetSeat = INDEX_NONE;

	UPROPERTY()
	FName OriginWindowName = NAME_None;

	UPROPERTY()
	TArray<int32> ProviderSeats;

	UPROPERTY()
	int32 NextProviderIndex = 0;

	UPROPERTY()
	bool bSucceeded = false;

	UPROPERTY()
	bool bPlaySlash = false;
};

namespace SGSLordAssistResolution
{
	SGS_API FName WindowName();
	SGS_API FName ResumeParentContinuation();
	SGS_API FSGSStatus Start(
		FSGSRuleExecutionContext& Context,
		FName SkillName,
		int32 LordSeat,
		FName RequiredCardName,
		int32 EffectTargetSeat,
		FGameplayTag ProviderFaction,
		bool bPlaySlash);
	SGS_API FSGSStatus Continue(FSGSRuleExecutionContext& Context, bool bAccepted);
}
