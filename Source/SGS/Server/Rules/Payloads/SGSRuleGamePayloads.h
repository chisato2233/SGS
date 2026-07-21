#pragma once

#include "CoreMinimal.h"
#include "SGSRuleGamePayloads.generated.h"

USTRUCT()
struct SGS_API FSGSJudgementRulePayload
{
	GENERATED_BODY()

	UPROPERTY()
	int32 DelayedTrickCardId = INDEX_NONE;

	UPROPERTY()
	int32 SeatIndex = INDEX_NONE;

	FString ToPayloadLogString() const
	{
		return FString::Printf(TEXT("Judgement CardId=%d Seat=%d"), DelayedTrickCardId, SeatIndex);
	}
};
