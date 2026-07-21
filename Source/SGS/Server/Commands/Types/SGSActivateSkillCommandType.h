#pragma once

#include "Server/Commands/SGSCommandTypes.h"
#include "Shared/Commands/SGSCommandPayloads.h"

class SGS_API FSGSActivateSkillCommandType final : public TSGSCommandType<FSGSActivateSkillCommandPayload>
{
public:
	virtual FGameplayTag GetType() const override;

protected:
	virtual FSGSStatus ValidateTyped(
		const FSGSCommand& Command,
		const FSGSActivateSkillCommandPayload& Payload,
		const FSGSCommandExecutionContext& Context) const override;

	virtual TSGSResult<FSGSRuleInvocation> BuildRuleInvocationTyped(
		const FSGSCommand& Command,
		const FSGSActivateSkillCommandPayload& Payload,
		const FSGSCommandExecutionContext& Context) const override;
};
