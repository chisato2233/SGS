#pragma once

// UseCard CommandType 是 server-only 行为定义；payload 在 Shared/Commands。

#include "CoreMinimal.h"
#include "Server/Commands/SGSCommandTypes.h"
#include "Shared/Commands/SGSCommandPayloads.h"

class SGS_API FSGSUseCardCommandType final : public TSGSCommandType<FSGSUseCardCommandPayload>
{
public:
	virtual FGameplayTag GetType() const override;

protected:
	virtual FSGSStatus ValidateTyped(
		const FSGSCommand& Command,
		const FSGSUseCardCommandPayload& Payload,
		const FSGSCommandExecutionContext& Context) const override;

	virtual TSGSResult<FSGSRuleInvocation> BuildRuleInvocationTyped(
		const FSGSCommand& Command,
		const FSGSUseCardCommandPayload& Payload,
		const FSGSCommandExecutionContext& Context) const override;
};
