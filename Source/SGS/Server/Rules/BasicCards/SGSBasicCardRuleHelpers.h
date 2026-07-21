#pragma once

#include "CoreMinimal.h"
#include "Server/Rules/Core/SGSRuleDescriptor.h"
#include "Server/Rules/Core/SGSRuleTypes.h"
#include "Server/Rules/Payloads/SGSRuleActionPayloads.h"
#include "Shared/Core/SGSGameplayTags.h"

class USGSCard;

namespace SGSBasicCardRuleHelpers
{
	FName SlashDodgeWindowName();
	FName DyingPeachWindowName();
	FName SlashDamageTriggerContinuation();
	FName AxeWindowName();
	FName AxeChoiceName();
	FName BladeWindowName();
	FName KylinBowWindowName();
	FName KylinBowChoiceName();

	FSGSRuleDescriptor MakeBasicRuleDescriptor(
		FName RuleName,
		FName RuleKindTag,
		FGameplayTag IntentTag,
		FName SubjectName,
		FName WindowName,
		int32 Priority,
		bool bWildcardIntent = false,
		bool bWildcardSubject = false,
		bool bWildcardWindow = false);

	bool IsIntent(const FSGSRuleExecutionContext& Context, const FNativeGameplayTag& Type);
	bool IsWindow(FName ActualWindowName, FName ExpectedWindowName);
	FSGSStatus MakeRuleError(FName Code, FString Message);
	USGSCard* FindPayloadCard(const FSGSRuleExecutionContext& Context, int32 CardId);
	int32 GetCommandSeat(const FSGSRuleExecutionContext& Context);
	FSGSCommandId GetCommandId(const FSGSRuleExecutionContext& Context);
	bool HasStatus(const FSGSRuleExecutionContext& Context, int32 SeatIndex, FGameplayTag StatusTag);
	int32 CountStatus(const FSGSRuleExecutionContext& Context, int32 SeatIndex, FGameplayTag StatusTag);
	void AddStatus(
		FSGSRuleExecutionContext& Context,
		int32 SeatIndex,
		FName EffectName,
		FGameplayTag StatusTag,
		FSGSDurationSpec Duration);
	bool ConsumeStatus(FSGSRuleExecutionContext& Context, int32 SeatIndex, FGameplayTag StatusTag);
	FSGSStatus DiscardHandCard(FSGSRuleExecutionContext& Context, USGSCard* Card, int32 SeatIndex);
	FSGSStatus DiscardProcessingCard(FSGSRuleExecutionContext& Context);
	FSGSStatus CompleteCurrentFrame(FSGSRuleExecutionContext& Context, FName Reason);
	int32 GetCurrentEffectSourceSeat(const FSGSRuleExecutionContext& Context);
	int32 GetCurrentEffectTargetSeat(const FSGSRuleExecutionContext& Context);
	int32 GetPeachHealTarget(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload);
	FSGSStatus ExecuteHealCard(
		FSGSRuleExecutionContext& Context,
		int32 CardId,
		FName CardName,
		int32 HealTargetSeat,
		int32 HealAmount = 1);
	FSGSStatus ContinueDyingPeachOrEliminate(FSGSRuleExecutionContext& Context);
	FSGSStatus StartDyingPeachResolution(FSGSRuleExecutionContext& Context, int32 DyingSeatIndex);
	FSGSStatus ResolveSlashHit(FSGSRuleExecutionContext& Context);
	FSGSStatus ContinueSlashAfterDamageTriggers(FSGSRuleExecutionContext& Context);
	FSGSStatus ResolveSuccessfulSlashDodge(FSGSRuleExecutionContext& Context);
	FSGSStatus FinishDodgedSlash(FSGSRuleExecutionContext& Context);
	FSGSStatus RestartSlashAfterBlade(
		FSGSRuleExecutionContext& Context,
		TConstArrayView<int32> MaterialCardIds,
		int32 SlashCardId,
		bool bVirtualSlash);
	FSGSStatus ValidateSlashUse(
		FSGSRuleExecutionContext& Context,
		USGSCard* MaterialCard,
		TConstArrayView<int32> TargetSeatIndices);
	FSGSStatus ValidateVirtualSlashUse(
		FSGSRuleExecutionContext& Context,
		int32 SourceSeat,
		TConstArrayView<int32> TargetSeatIndices);
	FSGSStatus ExecuteSlashUse(
		FSGSRuleExecutionContext& Context,
		USGSCard* MaterialCard,
		TConstArrayView<int32> TargetSeatIndices,
		FName SourceRuleName);
	FSGSStatus ExecuteVirtualSlashUse(
		FSGSRuleExecutionContext& Context,
		TConstArrayView<int32> MaterialCardIds,
		TConstArrayView<int32> TargetSeatIndices,
		FName SourceRuleName);
	FSGSStatus StartSlashResolution(
		FSGSRuleExecutionContext& Context,
		int32 SourceSeat,
		int32 TargetSeat,
		int32 SlashCardId,
		bool bVirtualSlash,
		FName SourceRuleName,
		TConstArrayView<int32> MaterialCardIds = {});
	FSGSStatus StartSlashResolution(
		FSGSRuleExecutionContext& Context,
		int32 SourceSeat,
		TConstArrayView<int32> TargetSeatIndices,
		int32 SlashCardId,
		bool bVirtualSlash,
		FName SourceRuleName,
		TConstArrayView<int32> MaterialCardIds = {});
}
