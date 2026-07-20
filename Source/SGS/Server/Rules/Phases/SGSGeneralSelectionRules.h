#pragma once

#include "Server/Rules/Core/SGSTypedRule.h"

class SGS_API FSGSGeneralSelectionChoiceRule final : public FSGSResponseRuleBase<FSGSChooseOptionRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSChooseOptionRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSChooseOptionRulePayload& Payload) const override;
};

namespace SGSGeneralSelectionRules
{
	SGS_API void Register(FSGSRuleRegistry& Registry);
}
