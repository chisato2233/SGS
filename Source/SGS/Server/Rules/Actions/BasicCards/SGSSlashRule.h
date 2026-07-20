#pragma once

#include "CoreMinimal.h"
#include "Server/Rules/Core/SGSTypedRule.h"
#include "SGSSlashRule.generated.h"

USTRUCT()
struct SGS_API FSGSSlashResolutionState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SlashCardId = INDEX_NONE;

	UPROPERTY()
	int32 SourceSeat = INDEX_NONE;

	UPROPERTY()
	int32 TargetSeat = INDEX_NONE;

	UPROPERTY()
	int32 DamageAmount = 1;

	UPROPERTY()
	int32 RequiredDodgeCount = 1;

	UPROPERTY()
	int32 DodgeCount = 0;

	FString ToLogString() const;
	bool CheckInvariants() const;
};

class SGS_API FSGSSlashRule final : public FSGSCardActionRuleBase<FSGSUseCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const override;
};
