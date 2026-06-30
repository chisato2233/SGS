#pragma once

// RespondCard CommandType 是 server-only 行为定义；payload 在 Shared/Commands。

#include "CoreMinimal.h"
#include "Server/Commands/SGSCommandTypes.h"
#include "Shared/Commands/SGSCommandPayloads.h"

class SGS_API FSGSRespondCardCommandType final : public TSGSCommandType<FSGSRespondCardCommandPayload>
{
public:
	virtual FGameplayTag GetType() const override;
	virtual void SyncLegacyMirror(FSGSCommand& Command) const override;

protected:
	virtual FSGSStatus ValidateTyped(
		const FSGSCommand& Command,
		const FSGSRespondCardCommandPayload& Payload,
		const FSGSCommandExecutionContext& Context) const override;
};
