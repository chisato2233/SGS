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
	TArray<int32> SelectedCardIds;

	UPROPERTY()
	FName CardName = NAME_None;

	UPROPERTY()
	FName MaterialCardName = NAME_None;

	UPROPERTY()
	FName SkillName = NAME_None;

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
			TEXT("RespondCard CardId=%d Cards=[%s] Card=%s Targets=[%s] Window=%s Required=%s Source=%d Target=%d"),
			CardId,
			*SGSRulePayloadLog::JoinIntArray(SelectedCardIds),
			*CardName.ToString(),
			*SGSRulePayloadLog::JoinIntArray(TargetSeatIndices),
			*WindowName.ToString(),
			*RequiredCardName.ToString(),
			EffectSourceSeat,
			EffectTargetSeat);
	}
};

USTRUCT()
struct SGS_API FSGSChooseCardsRulePayload
{
	GENERATED_BODY()

	UPROPERTY()
	FName ChoiceName = NAME_None;

	UPROPERTY()
	FName WindowName = NAME_None;

	UPROPERTY()
	TArray<int32> SelectedCardIds;

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("ChooseCards Choice=%s Window=%s Cards=[%s]"),
			*ChoiceName.ToString(),
			*WindowName.ToString(),
			*SGSRulePayloadLog::JoinIntArray(SelectedCardIds));
	}
};

USTRUCT()
struct SGS_API FSGSChooseOptionRulePayload
{
	GENERATED_BODY()

	UPROPERTY()
	FName ChoiceName = NAME_None;

	UPROPERTY()
	FName WindowName = NAME_None;

	UPROPERTY()
	FName SelectedOption = NAME_None;

	FString ToPayloadLogString() const
	{
		return FString::Printf(
			TEXT("ChooseOption Choice=%s Window=%s Option=%s"),
			*ChoiceName.ToString(), *WindowName.ToString(), *SelectedOption.ToString());
	}
};
