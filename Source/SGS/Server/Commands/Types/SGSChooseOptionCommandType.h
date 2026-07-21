#pragma once

#include "CoreMinimal.h"
#include "Server/Commands/SGSCommandTypes.h"
#include "Shared/Commands/SGSCommandPayloads.h"

class SGS_API FSGSChooseOptionCommandType final : public TSGSCommandType<FSGSChooseOptionCommandPayload>
{
public:
	virtual FGameplayTag GetType() const override;

protected:
	virtual FSGSStatus ValidateTyped(
		const FSGSCommand& Command,
		const FSGSChooseOptionCommandPayload& Payload,
		const FSGSCommandExecutionContext& Context) const override;
	virtual TSGSResult<FSGSRuleInvocation> BuildRuleInvocationTyped(
		const FSGSCommand& Command,
		const FSGSChooseOptionCommandPayload& Payload,
		const FSGSCommandExecutionContext& Context) const override;
};
