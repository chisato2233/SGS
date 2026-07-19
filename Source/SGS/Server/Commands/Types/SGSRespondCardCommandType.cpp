#include "Server/Commands/Types/SGSRespondCardCommandType.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Commands/SGSCommandPayloadTraits.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Rules/Core/SGSRuleInvocation.h"

namespace
{
const FSGSCardState* FindRespondCardHandCardStateById(int32 CardId, const FSGSCommand& Command, const FSGSCommandExecutionContext& Context)
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

	const FSGSCardState* State = FindRespondCardHandCardStateById(Payload.CardId, Command, Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvalidCard")),
			TEXT("RespondCard requires exactly one card currently in the responding seat's hand.")));
	}

	const bool bCardNameAccepted = Context.AcceptedCardNames.IsEmpty()
		? Context.RequiredCardName.IsNone() || State->CardName == Context.RequiredCardName
		: Context.AcceptedCardNames.Contains(State->CardName);
	if (!bCardNameAccepted)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.CardNameMismatch")),
			FString::Printf(TEXT("Required=%s Actual=%s"), *Context.RequiredCardName.ToString(), *State->CardName.ToString())));
	}

	return MakeValue();
}

TSGSResult<FSGSRuleInvocation> FSGSRespondCardCommandType::BuildRuleInvocationTyped(
	const FSGSCommand& Command,
	const FSGSRespondCardCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	const FSGSCardState* State = FindRespondCardHandCardStateById(Payload.CardId, Command, Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvalidCard")),
			TEXT("Cannot build RuleInvocation for a missing RespondCard hand card.")));
	}

	FSGSRespondCardRulePayload RulePayload;
	RulePayload.CardId = Payload.CardId;
	RulePayload.CardName = State->CardName;
	RulePayload.TargetSeatIndices = Payload.TargetSeatIndices;
	RulePayload.WindowName = Payload.WindowName;
	RulePayload.RequiredCardName = Context.RequiredCardName;
	RulePayload.AcceptedCardNames = Context.AcceptedCardNames;
	RulePayload.EffectSourceSeat = Context.EffectSourceSeatIndex;
	RulePayload.EffectTargetSeat = Context.EffectTargetSeatIndex;

	FSGSRuleInvocation Invocation;
	Invocation.RuleKindTag = SGSRuleKinds::Response();
	Invocation.IntentTag = Command.Type;
	Invocation.SubjectName = State->CardName;
	Invocation.ActorSeat = Command.SeatIndex;
	Invocation.WindowName = Payload.WindowName;
	Invocation.SourceCommandId = Command.CommandId;
	Invocation.SourceRequestId = Command.RequestId;
	Invocation.Payload = FInstancedStruct::Make(RulePayload);
	return MakeValue(MoveTemp(Invocation));
}
