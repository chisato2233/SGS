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
	TArray<int32> SelectedCardIds;

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

	// 响应技能可消耗多张素材牌；物理牌响应仍保持单张，并同步写入 CardId。
	UPROPERTY()
	TArray<int32> SelectedCardIds;

	UPROPERTY()
	TArray<int32> TargetSeatIndices;

	UPROPERTY()
	FName WindowName = NAME_None;

	// 非空时表示由服务器列出的响应技能产生该动作；物理牌响应保持 None。
	UPROPERTY()
	FName SkillName = NAME_None;

	UPROPERTY()
	FName ResultCardName = NAME_None;

	FSGSRespondCardCommandPayload() = default;

	FSGSRespondCardCommandPayload(
		int32 InCardId,
		TArray<int32> InTargetSeatIndices,
		FName InWindowName,
		FName InSkillName = NAME_None,
		FName InResultCardName = NAME_None)
		: CardId(InCardId)
		, TargetSeatIndices(MoveTemp(InTargetSeatIndices))
		, WindowName(InWindowName)
		, SkillName(InSkillName)
		, ResultCardName(InResultCardName)
	{
		if (InCardId != INDEX_NONE)
		{
			SelectedCardIds.Add(InCardId);
		}
	}

	FSGSRespondCardCommandPayload(
		TArray<int32> InSelectedCardIds,
		TArray<int32> InTargetSeatIndices,
		FName InWindowName,
		FName InSkillName,
		FName InResultCardName = NAME_None)
		: CardId(InSelectedCardIds.IsEmpty() ? INDEX_NONE : InSelectedCardIds[0])
		, SelectedCardIds(MoveTemp(InSelectedCardIds))
		, TargetSeatIndices(MoveTemp(InTargetSeatIndices))
		, WindowName(InWindowName)
		, SkillName(InSkillName)
		, ResultCardName(InResultCardName)
	{
	}

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("CardId=%d Cards=[%s] Targets=[%s] Window=%s Skill=%s Result=%s"),
			CardId,
			*SGSCommandPayloadLog::JoinIntArray(SelectedCardIds),
			*SGSCommandPayloadLog::JoinIntArray(TargetSeatIndices),
			*WindowName.ToString(),
			*SkillName.ToString(),
			*ResultCardName.ToString());
	}
};

USTRUCT()
struct SGS_API FSGSActivateSkillCommandPayload
{
	GENERATED_BODY()

	UPROPERTY()
	FName SkillName = NAME_None;

	UPROPERTY()
	FName RuleKindTag = NAME_None;

	UPROPERTY()
	FName ResultCardName = NAME_None;

	UPROPERTY()
	TArray<int32> SelectedCardIds;

	UPROPERTY()
	TArray<int32> TargetSeatIndices;

	FSGSActivateSkillCommandPayload() = default;

	FSGSActivateSkillCommandPayload(
		FName InSkillName,
		FName InRuleKindTag,
		FName InResultCardName,
		TArray<int32> InSelectedCardIds,
		TArray<int32> InTargetSeatIndices)
		: SkillName(InSkillName)
		, RuleKindTag(InRuleKindTag)
		, ResultCardName(InResultCardName)
		, SelectedCardIds(MoveTemp(InSelectedCardIds))
		, TargetSeatIndices(MoveTemp(InTargetSeatIndices))
	{
	}

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("Skill=%s Kind=%s ResultCard=%s Cards=[%s] Targets=[%s]"),
			*SkillName.ToString(),
			*RuleKindTag.ToString(),
			*ResultCardName.ToString(),
			*SGSCommandPayloadLog::JoinIntArray(SelectedCardIds),
			*SGSCommandPayloadLog::JoinIntArray(TargetSeatIndices));
	}
};

USTRUCT()
struct SGS_API FSGSChooseCardsCommandPayload
{
	GENERATED_BODY()

	UPROPERTY()
	FName ChoiceName = NAME_None;

	UPROPERTY()
	FName WindowName = NAME_None;

	UPROPERTY()
	TArray<int32> SelectedCardIds;

	FSGSChooseCardsCommandPayload() = default;

	FSGSChooseCardsCommandPayload(
		FName InChoiceName,
		FName InWindowName,
		TArray<int32> InSelectedCardIds)
		: ChoiceName(InChoiceName)
		, WindowName(InWindowName)
		, SelectedCardIds(MoveTemp(InSelectedCardIds))
	{
	}

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("Choice=%s Window=%s Cards=[%s]"),
			*ChoiceName.ToString(),
			*WindowName.ToString(),
			*SGSCommandPayloadLog::JoinIntArray(SelectedCardIds));
	}
};

USTRUCT()
struct SGS_API FSGSChooseOptionCommandPayload
{
	GENERATED_BODY()

	UPROPERTY()
	FName ChoiceName = NAME_None;

	UPROPERTY()
	FName WindowName = NAME_None;

	UPROPERTY()
	FName SelectedOption = NAME_None;

	FSGSChooseOptionCommandPayload() = default;
	FSGSChooseOptionCommandPayload(FName InChoiceName, FName InWindowName, FName InSelectedOption)
		: ChoiceName(InChoiceName), WindowName(InWindowName), SelectedOption(InSelectedOption) {}

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("Choice=%s Window=%s Option=%s"),
			*ChoiceName.ToString(), *WindowName.ToString(), *SelectedOption.ToString());
	}
};
