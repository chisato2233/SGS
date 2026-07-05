#include "Server/Commands/Types/SGSUseCardCommandType.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Commands/SGSCommandPayloadTraits.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Rules/Core/SGSRuleInvocation.h"

namespace
{
const FSGSCardState* FindUseCardHandCardStateById(int32 CardId, const FSGSCommand& Command, const FSGSCommandExecutionContext& Context)
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

FSGSStatus FSGSUseCardCommandType::ValidateTyped(
	const FSGSCommand& Command,
	const FSGSUseCardCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	if (!Context.ExpectedWindowName.IsNone())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.UseCardDuringResponse")),
			TEXT("UseCard is only valid outside response windows.")));
	}

	if (FindUseCardHandCardStateById(Payload.CardId, Command, Context) == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvalidCard")),
			TEXT("UseCard requires exactly one card currently in the acting seat's hand.")));
	}

	return MakeValue();
}

TSGSResult<FSGSRuleInvocation> FSGSUseCardCommandType::BuildRuleInvocationTyped(
	const FSGSCommand& Command,
	const FSGSUseCardCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	const FSGSCardState* State = FindUseCardHandCardStateById(Payload.CardId, Command, Context);
	if (State == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvalidCard")),
			TEXT("Cannot build RuleInvocation for a missing UseCard hand card.")));
	}

	FSGSUseCardRulePayload RulePayload;
	RulePayload.CardId = Payload.CardId;
	RulePayload.CardName = State->CardName;
	RulePayload.TargetSeatIndices = Payload.TargetSeatIndices;
	RulePayload.EffectSourceSeat = Command.SeatIndex;
	RulePayload.EffectTargetSeat = Payload.TargetSeatIndices.Num() > 0 ? Payload.TargetSeatIndices[0] : Command.SeatIndex;

	FSGSRuleInvocation Invocation;
	Invocation.RuleKindTag = SGSRuleKinds::Action();
	Invocation.IntentTag = Command.Type;
	Invocation.SubjectName = State->CardName;
	Invocation.ActorSeat = Command.SeatIndex;
	Invocation.WindowName = Context.ExpectedWindowName;
	Invocation.SourceCommandId = Command.CommandId;
	Invocation.SourceRequestId = Command.RequestId;
	Invocation.Payload = FInstancedStruct::Make(RulePayload);
	return MakeValue(MoveTemp(Invocation));
}
