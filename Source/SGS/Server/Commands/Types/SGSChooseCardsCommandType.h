#pragma once

#include "CoreMinimal.h"
#include "Server/Commands/SGSCommandTypes.h"
#include "Shared/Commands/SGSCommandPayloads.h"

class SGS_API FSGSChooseCardsCommandType final : public TSGSCommandType<FSGSChooseCardsCommandPayload>
{
public:
	virtual FGameplayTag GetType() const override;

protected:
	virtual FSGSStatus ValidateTyped(
		const FSGSCommand& Command,
		const FSGSChooseCardsCommandPayload& Payload,
		const FSGSCommandExecutionContext& Context) const override;
	virtual TSGSResult<FSGSRuleInvocation> BuildRuleInvocationTyped(
		const FSGSCommand& Command,
		const FSGSChooseCardsCommandPayload& Payload,
		const FSGSCommandExecutionContext& Context) const override;
};
