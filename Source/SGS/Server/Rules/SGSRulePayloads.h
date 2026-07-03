#pragma once

// RuleInvocation payload 是服务器内部规则调用数据，不等同于 shared Command payload。
// Command payload 描述外部输入；这里保存经过服务器上下文翻译后的卡牌、窗口和来源事实。
// 第一阶段只覆盖 Pass / UseCard / RespondCard，后续 Trigger / Modifier / ViewAs 再继续扩展。

#include "CoreMinimal.h"
#include "Server/Timing/SGSTimingTypes.h"
#include "Shared/Core/SGSIds.h"
#include "StructUtils/InstancedStruct.h"

#include "SGSRulePayloads.generated.h"

namespace SGSRulePayloadLog
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
