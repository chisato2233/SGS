#pragma once

// RuleInvocation 是 CommandRouter 之后的内部规则调用信封。
// 它用开放 FName / GameplayTag 描述 kind、intent、subject、window，并用 FInstancedStruct 承载 typed payload。

#include "CoreMinimal.h"
#include "Server/Rules/Payloads/SGSRulePayloads.h"
#include "Shared/Core/SGSIds.h"
#include "StructUtils/InstancedStruct.h"

namespace SGSRuleKinds
{
	inline FName Action() { return FName(TEXT("SGS.RuleKind.Action")); }
	inline FName Response() { return FName(TEXT("SGS.RuleKind.Response")); }
	inline FName GameRule() { return FName(TEXT("SGS.RuleKind.GameRule")); }
	inline FName Trigger() { return FName(TEXT("SGS.RuleKind.Trigger")); }
	inline FName Modifier() { return FName(TEXT("SGS.RuleKind.Modifier")); }
	inline FName ViewAs() { return FName(TEXT("SGS.RuleKind.ViewAs")); }
}

struct SGS_API FSGSRuleInvocation
{
	FName RuleKindTag = NAME_None;
	FGameplayTag IntentTag;
	FName SubjectName = NAME_None;
	int32 ActorSeat = INDEX_NONE;
	FName WindowName = NAME_None;
	FSGSCommandId SourceCommandId;
	int32 SourceRequestId = 0;
	FInstancedStruct Payload;

	bool HasPayload() const
	{
		return Payload.IsValid();
	}

	const UScriptStruct* GetPayloadStruct() const
	{
		return Payload.GetScriptStruct();
	}

	template <typename TPayload>
	const TPayload* GetPayload() const
	{
		return Payload.GetPtr<TPayload>();
	}

	template <typename TPayload>
	TPayload* GetMutablePayload()
	{
		return Payload.GetMutablePtr<TPayload>();
	}

	FString GetPayloadTypeName() const
	{
		const UScriptStruct* PayloadStruct = GetPayloadStruct();
		return PayloadStruct != nullptr ? PayloadStruct->GetName() : TEXT("None");
	}

	FString GetPayloadLogString() const
	{
		if (const FSGSPassRulePayload* PassPayload = GetPayload<FSGSPassRulePayload>())
		{
			return PassPayload->ToPayloadLogString();
		}
		if (const FSGSUseCardRulePayload* UseCardPayload = GetPayload<FSGSUseCardRulePayload>())
		{
			return UseCardPayload->ToPayloadLogString();
		}
		if (const FSGSActivateSkillRulePayload* SkillPayload = GetPayload<FSGSActivateSkillRulePayload>())
		{
			return SkillPayload->ToPayloadLogString();
		}
		if (const FSGSRespondCardRulePayload* RespondCardPayload = GetPayload<FSGSRespondCardRulePayload>())
		{
			return RespondCardPayload->ToPayloadLogString();
		}
		if (const FSGSRuleEventPayload* EventPayload = GetPayload<FSGSRuleEventPayload>())
		{
			return EventPayload->ToPayloadLogString();
		}
		if (const FSGSJudgementRulePayload* JudgementPayload = GetPayload<FSGSJudgementRulePayload>())
		{
			return JudgementPayload->ToPayloadLogString();
		}

		return HasPayload()
			? FString::Printf(TEXT("Payload=%s"), *GetPayloadTypeName())
			: TEXT("NoPayload");
	}

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(!RuleKindTag.IsNone(), TEXT("RuleInvocation requires a rule kind."));
		bOk &= ensureMsgf(IntentTag.IsValid(), TEXT("RuleInvocation requires an intent tag."));
		bOk &= ensureMsgf(ActorSeat != INDEX_NONE, TEXT("RuleInvocation requires an actor seat."));
		bOk &= SourceCommandId.CheckInvariants();
		bOk &= ensureMsgf(SourceCommandId.IsValid(), TEXT("RuleInvocation requires a valid source command id."));
		bOk &= ensureMsgf(SourceRequestId > 0, TEXT("RuleInvocation requires a positive source request id."));
		bOk &= ensureMsgf(HasPayload(), TEXT("RuleInvocation requires a typed payload."));
		return bOk;
	}

	FString ToLogString() const
	{
		return FString::Printf(
			TEXT("Kind=%s Intent=%s Subject=%s Actor=%d Window=%s CommandId=%s RequestId=%d PayloadType=%s %s"),
			*RuleKindTag.ToString(),
			*IntentTag.ToString(),
			*SubjectName.ToString(),
			ActorSeat,
			*WindowName.ToString(),
			*SourceCommandId.ToLogString(),
			SourceRequestId,
			*GetPayloadTypeName(),
			*GetPayloadLogString());
	}
};
