#include "Server/Commands/Types/SGSRespondCardCommandType.h"

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

FGameplayTag FSGSRespondCardCommandType::GetType() const
{
	return TSGSCommandPayloadTraits<FSGSRespondCardCommandPayload>::GetType();
}

void FSGSRespondCardCommandType::SyncLegacyMirror(FSGSCommand& Command) const
{
	TSGSCommandPayloadTraits<FSGSRespondCardCommandPayload>::SyncLegacyMirror(Command);
}

FSGSStatus FSGSRespondCardCommandType::ValidateTyped(
	const FSGSCommand& Command,
	const FSGSRespondCardCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	if (Context.ExpectedWindowName.IsNone())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.NoResponseWindow")),
			TEXT("RespondCard is only valid while a response window is pending.")));
	}

	if (Payload.WindowName != Context.ExpectedWindowName)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.ResponseWindowMismatch")),
			FString::Printf(TEXT("Command window does not match expected window %s."), *Context.ExpectedWindowName.ToString())));
	}

	const FSGSCardState* State = FindHandCardStateById(Payload.CardId, Command, Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvalidCard")),
			TEXT("RespondCard requires exactly one card currently in the responding seat's hand.")));
	}

	if (!Context.RequiredCardName.IsNone() && State->CardName != Context.RequiredCardName)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.CardNameMismatch")),
			FString::Printf(TEXT("Required=%s Actual=%s"), *Context.RequiredCardName.ToString(), *State->CardName.ToString())));
	}

	if (Command.CardHandles.Num() > 0 || Command.TargetHandles.Num() > 0)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.UnsupportedHandlePayload")),
			TEXT("Plan0005 commands use CardIds / TargetSeatIndices; handle payloads are reserved for later Client/UI/network paths.")));
	}

	return MakeValue();
}
