#pragma once

#include "Server/Rules/Core/SGSTypedRule.h"

class SGS_API FSGSDelayedTrickJudgementRule final : public TSGSTypedRule<FSGSJudgementRulePayload>
{
public:
	explicit FSGSDelayedTrickJudgementRule(FName InCardName)
		: CardName(InCardName)
	{
	}

	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSJudgementRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSJudgementRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSJudgementRulePayload& Payload) const override;

private:
	FName CardName;
};

namespace SGSDelayedTrickRules
{
	SGS_API void Register(FSGSRuleRegistry& Registry);
}
