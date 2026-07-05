#pragma once

#include "Server/Rules/Core/SGSTypedRule.h"

class SGS_API FSGSPassRule final : public FSGSActionRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
};
