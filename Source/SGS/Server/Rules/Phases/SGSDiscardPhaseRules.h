#pragma once

#include "Server/Rules/Core/SGSTypedRule.h"
#include "SGSDiscardPhaseRules.generated.h"

USTRUCT()
struct SGS_API FSGSDiscardPhaseResolutionState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SeatIndex = INDEX_NONE;

	UPROPERTY()
	int32 RequiredDiscardCount = 0;
};

class SGS_API FSGSDiscardPhaseResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

namespace SGSDiscardPhaseRules
{
	SGS_API FName WindowName();
	SGS_API void Register(FSGSRuleRegistry& Registry);
}
