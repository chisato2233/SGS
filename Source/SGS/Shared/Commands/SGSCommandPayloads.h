#pragma once

// Shared command payloads are protocol data. They are visible to client input,
// server validation, replay, and tests, but contain no server-only rule behavior.

#include "CoreMinimal.h"

#include "SGSCommandPayloads.generated.h"

namespace SGSCommandPayloadLog
{
	inline FString JoinIntArray(const TArray<int32>& Values)
	{
		TArray<FString> Parts;
		Parts.Reserve(Values.Num());
		for (const int32 Value : Values)
		{
			Parts.Add(FString::FromInt(Value));
		}
		return FString::Join(Parts, TEXT(","));
	}
}

USTRUCT()
struct SGS_API FSGSPassCommandPayload
{
	GENERATED_BODY()

	FString ToPayloadLogString() const
	{
		return TEXT("Pass");
	}
};

USTRUCT()
struct SGS_API FSGSUseCardCommandPayload
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CardId = INDEX_NONE;

	UPROPERTY()
	TArray<int32> TargetSeatIndices;

	FSGSUseCardCommandPayload() = default;

	FSGSUseCardCommandPayload(int32 InCardId, TArray<int32> InTargetSeatIndices)
		: CardId(InCardId)
		, TargetSeatIndices(MoveTemp(InTargetSeatIndices))
	{
	}

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("CardId=%d Targets=[%s]"),
			CardId,
			*SGSCommandPayloadLog::JoinIntArray(TargetSeatIndices));
	}
};

USTRUCT()
struct SGS_API FSGSRespondCardCommandPayload
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CardId = INDEX_NONE;

	UPROPERTY()
	TArray<int32> TargetSeatIndices;

	UPROPERTY()
	FName WindowName = NAME_None;

	FSGSRespondCardCommandPayload() = default;

	FSGSRespondCardCommandPayload(int32 InCardId, TArray<int32> InTargetSeatIndices, FName InWindowName)
		: CardId(InCardId)
		, TargetSeatIndices(MoveTemp(InTargetSeatIndices))
		, WindowName(InWindowName)
	{
	}

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("CardId=%d Targets=[%s] Window=%s"),
			CardId,
			*SGSCommandPayloadLog::JoinIntArray(TargetSeatIndices),
			*WindowName.ToString());
	}
};
