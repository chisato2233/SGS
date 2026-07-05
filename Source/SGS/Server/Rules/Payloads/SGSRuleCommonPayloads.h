#pragma once

// Common RuleInvocation payloads shared by action and response rules.

#include "CoreMinimal.h"
#include "SGSRuleCommonPayloads.generated.h"

USTRUCT()
struct SGS_API FSGSPassRulePayload
{
	GENERATED_BODY()

	UPROPERTY()
	FName WindowName = NAME_None;

	UPROPERTY()
	FName RequiredCardName = NAME_None;

	UPROPERTY()
	int32 EffectSourceSeat = INDEX_NONE;

	UPROPERTY()
	int32 EffectTargetSeat = INDEX_NONE;

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("Pass Window=%s Required=%s Source=%d Target=%d"),
			*WindowName.ToString(),
			*RequiredCardName.ToString(),
			EffectSourceSeat,
			EffectTargetSeat);
	}
};
