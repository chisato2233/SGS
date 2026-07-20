#pragma once

// Action RuleInvocation payloads describe server-translated active intents.

#include "CoreMinimal.h"
#include "Server/Rules/Payloads/SGSRulePayloadLog.h"
#include "SGSRuleActionPayloads.generated.h"

USTRUCT()
struct SGS_API FSGSUseCardRulePayload
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CardId = INDEX_NONE;

	UPROPERTY()
	FName CardName = NAME_None;

	UPROPERTY()
	TArray<int32> TargetSeatIndices;

	UPROPERTY()
	int32 EffectSourceSeat = INDEX_NONE;

	UPROPERTY()
	int32 EffectTargetSeat = INDEX_NONE;

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("UseCard CardId=%d Card=%s Targets=[%s] Source=%d Target=%d"),
			CardId,
			*CardName.ToString(),
			*SGSRulePayloadLog::JoinIntArray(TargetSeatIndices),
			EffectSourceSeat,
			EffectTargetSeat);
	}
};

USTRUCT()
struct SGS_API FSGSActivateSkillRulePayload
{
	GENERATED_BODY()

	UPROPERTY()
	FName SkillName = NAME_None;

	UPROPERTY()
	FName ResultCardName = NAME_None;

	UPROPERTY()
	TArray<int32> SelectedCardIds;

	UPROPERTY()
	TArray<int32> TargetSeatIndices;

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("ActivateSkill Skill=%s ResultCard=%s Cards=[%s] Targets=[%s]"),
			*SkillName.ToString(),
			*ResultCardName.ToString(),
			*SGSRulePayloadLog::JoinIntArray(SelectedCardIds),
			*SGSRulePayloadLog::JoinIntArray(TargetSeatIndices));
	}
};
