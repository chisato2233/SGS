#pragma once

#include "CoreMinimal.h"
#include "Server/Rules/Core/SGSTypedRule.h"
#include "SGSDyingPeachRules.generated.h"

USTRUCT()
struct SGS_API FSGSDyingPeachResolutionState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 DyingSeat = INDEX_NONE;

	UPROPERTY()
	TArray<int32> ResponderSeatIndices;

	UPROPERTY()
	int32 NextResponderIndex = 0;

	UPROPERTY()
	bool bNeedsHealthRecheck = false;

	FString ToLogString() const;
	bool CheckInvariants() const;
};

class SGS_API FSGSDyingPeachPassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
};

class SGS_API FSGSDyingPeachRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};
