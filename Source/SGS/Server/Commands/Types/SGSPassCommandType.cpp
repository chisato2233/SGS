#include "Server/Commands/Types/SGSPassCommandType.h"

#include "Shared/Commands/SGSCommandPayloadTraits.h"

FGameplayTag FSGSPassCommandType::GetType() const
{
	return TSGSCommandPayloadTraits<FSGSPassCommandPayload>::GetType();
}

FSGSStatus FSGSPassCommandType::ValidateTyped(
	const FSGSCommand& Command,
	const FSGSPassCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	if (Command.CardIds.Num() > 0
		|| Command.TargetSeatIndices.Num() > 0
		|| Command.CardHandles.Num() > 0
		|| Command.TargetHandles.Num() > 0
		|| Command.Parameters.Num() > 0
		|| !Command.SkillName.IsNone())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvalidPayload")),
			TEXT("Pass command must not include cards, targets, skills, or parameters.")));
	}

	return MakeValue();
}
