#pragma once

// Event RuleInvocation payloads are the typed bridge for controlled TriggerRule dispatch.

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Server/Timing/SGSTimingTypes.h"
#include "Shared/Core/SGSIds.h"
#include "StructUtils/InstancedStruct.h"
#include "SGSRuleEventPayloads.generated.h"

USTRUCT()
struct SGS_API FSGSDamageEventData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CardId = INDEX_NONE;

	UPROPERTY()
	int32 Amount = 0;
};

USTRUCT()
struct SGS_API FSGSJudgementEventData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 JudgedSeat = INDEX_NONE;

	UPROPERTY()
	FName ReasonName = NAME_None;

	UPROPERTY()
	int32 ResultCardId = INDEX_NONE;
};

USTRUCT()
struct SGS_API FSGSRuleEventPayload
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag EventTag;

	UPROPERTY()
	FName EventName = NAME_None;

	UPROPERTY()
	int32 SourceSeat = INDEX_NONE;

	UPROPERTY()
	int32 TargetSeat = INDEX_NONE;

	FSGSCommandId SourceCommandId;

	FSGSTimingPoint TimingPoint;

	FInstancedStruct EventData;

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(EventTag.IsValid(), TEXT("RuleEventPayload requires a valid event tag."));
		bOk &= ensureMsgf(SourceSeat == INDEX_NONE || SourceSeat >= 0, TEXT("RuleEventPayload source seat must be invalid or non-negative."));
		bOk &= ensureMsgf(TargetSeat == INDEX_NONE || TargetSeat >= 0, TEXT("RuleEventPayload target seat must be invalid or non-negative."));
		bOk &= SourceCommandId.CheckInvariants();
		bOk &= TimingPoint.CheckInvariants();
		bOk &= ensureMsgf(TimingPoint.IsValid(), TEXT("RuleEventPayload requires a valid timing point."));
		return bOk;
	}

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("Event Tag=%s Name=%s Source=%d Target=%d CommandId=%s Timing={%s} Data=%s"),
			*EventTag.ToString(),
			*EventName.ToString(),
			SourceSeat,
			TargetSeat,
			*SourceCommandId.ToLogString(),
			*TimingPoint.ToLogString(),
			EventData.IsValid() ? *EventData.GetScriptStruct()->GetName() : TEXT("None"));
	}
};
