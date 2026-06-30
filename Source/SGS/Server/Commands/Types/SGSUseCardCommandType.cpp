#include "Server/Commands/Types/SGSUseCardCommandType.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Commands/SGSCommandPayloadTraits.h"
#include "Server/Engine/SGSGameContext.h"

namespace
{
const FSGSCardState* FindHandCardStateById(int32 CardId, const FSGSCommand& Command, const FSGSCommandExecutionContext& Context)
{
	if (CardId == INDEX_NONE || Context.GameContext == nullptr)
	{
		return nullptr;
	}

	const FSGSCardState* State = Context.GameContext->FindCardStateById(CardId);
	if (State == nullptr
		|| !State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag())
		|| State->OwnerSeat != Command.SeatIndex)
	{
		return nullptr;
	}

	return State;
}
}

FGameplayTag FSGSUseCardCommandType::GetType() const
{
	return TSGSCommandPayloadTraits<FSGSUseCardCommandPayload>::GetType();
}

void FSGSUseCardCommandType::SyncLegacyMirror(FSGSCommand& Command) const
{
	TSGSCommandPayloadTraits<FSGSUseCardCommandPayload>::SyncLegacyMirror(Command);
}

FSGSStatus FSGSUseCardCommandType::ValidateTyped(
	const FSGSCommand& Command,
	const FSGSUseCardCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	if (FindHandCardStateById(Payload.CardId, Command, Context) == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvalidCard")),
			TEXT("UseCard requires exactly one card currently in the acting seat's hand.")));
	}

	if (Command.CardHandles.Num() > 0 || Command.TargetHandles.Num() > 0)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.UnsupportedHandlePayload")),
			TEXT("Plan0005 commands use CardIds / TargetSeatIndices; handle payloads are reserved for later Client/UI/network paths.")));
	}

	return MakeValue();
}
