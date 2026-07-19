#pragma once

// Response RuleInvocation payloads describe cards submitted into an open response window.

#include "CoreMinimal.h"
#include "Server/Rules/Payloads/SGSRulePayloadLog.h"
#include "SGSRuleResponsePayloads.generated.h"

USTRUCT()
struct SGS_API FSGSRespondCardRulePayload
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CardId = INDEX_NONE;

	UPROPERTY()
	FName CardName = NAME_None;

	UPROPERTY()
	TArray<int32> TargetSeatIndices;

	UPROPERTY()
	FName WindowName = NAME_None;

	UPROPERTY()
	FName RequiredCardName = NAME_None;

	UPROPERTY()
	TArray<FName> AcceptedCardNames;

	UPROPERTY()
	int32 EffectSourceSeat = INDEX_NONE;

	UPROPERTY()
	int32 EffectTargetSeat = INDEX_NONE;

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("RespondCard CardId=%d Card=%s Targets=[%s] Window=%s Required=%s Source=%d Target=%d"),
			CardId,
			*CardName.ToString(),
			*SGSRulePayloadLog::JoinIntArray(TargetSeatIndices),
			*WindowName.ToString(),
			*RequiredCardName.ToString(),
			EffectSourceSeat,
			EffectTargetSeat);
	}
};
