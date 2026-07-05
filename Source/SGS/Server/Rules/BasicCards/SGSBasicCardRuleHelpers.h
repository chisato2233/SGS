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
	FSGSStatus DiscardHandCard(FSGSRuleExecutionContext& Context, USGSCard* Card, int32 SeatIndex);
	FSGSStatus DiscardProcessingCard(FSGSRuleExecutionContext& Context);
	FSGSStatus CompleteCurrentFrame(FSGSRuleExecutionContext& Context, FName Reason);
	int32 GetCurrentEffectSourceSeat(const FSGSRuleExecutionContext& Context);
	int32 GetCurrentEffectTargetSeat(const FSGSRuleExecutionContext& Context);
	int32 GetPeachHealTarget(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload);
	FSGSStatus ExecutePeachHeal(FSGSRuleExecutionContext& Context, int32 PeachCardId, int32 HealTargetSeat);
	FSGSStatus ContinueDyingPeachOrEliminate(FSGSRuleExecutionContext& Context);
	FSGSStatus StartDyingPeachResolution(FSGSRuleExecutionContext& Context, int32 DyingSeatIndex);
	FSGSStatus ResolveSlashHit(FSGSRuleExecutionContext& Context);
}
