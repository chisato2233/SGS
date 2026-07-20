#include "Server/Commands/Types/SGSChooseCardsCommandType.h"

#include "Shared/Commands/SGSCommandPayloadTraits.h"
#include "Server/Rules/Payloads/SGSRuleResponsePayloads.h"

FGameplayTag FSGSChooseCardsCommandType::GetType() const
{
	return TSGSCommandPayloadTraits<FSGSChooseCardsCommandPayload>::GetType();
}

FSGSStatus FSGSChooseCardsCommandType::ValidateTyped(
	const FSGSCommand&,
	const FSGSChooseCardsCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	if (!Context.bIsCardChoice
		|| Payload.WindowName != Context.ExpectedWindowName
		|| Payload.ChoiceName != Context.ExpectedChoiceName)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.CardChoiceMismatch")),
			TEXT("ChooseCards does not match the pending server choice window.")));
	}
	if (Payload.SelectedCardIds.Num() < Context.MinChoiceCount
		|| Payload.SelectedCardIds.Num() > Context.MaxChoiceCount)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.CardChoiceCount")),
			TEXT("ChooseCards selection count is outside the server-authorized range.")));
	}

	TSet<int32> UniqueCards;
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		if (UniqueCards.Contains(CardId) || !Context.SelectableChoiceCardIds.Contains(CardId))
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.Command.InvalidCardChoice")),
				TEXT("ChooseCards contains a duplicate or unauthorized card.")));
		}
		UniqueCards.Add(CardId);
	}
	return MakeValue();
}

TSGSResult<FSGSRuleInvocation> FSGSChooseCardsCommandType::BuildRuleInvocationTyped(
	const FSGSCommand& Command,
	const FSGSChooseCardsCommandPayload& Payload,
	const FSGSCommandExecutionContext&) const
{
	FSGSChooseCardsRulePayload RulePayload;
	RulePayload.ChoiceName = Payload.ChoiceName;
	RulePayload.WindowName = Payload.WindowName;
	RulePayload.SelectedCardIds = Payload.SelectedCardIds;

	FSGSRuleInvocation Invocation;
	Invocation.RuleKindTag = SGSRuleKinds::Response();
	Invocation.IntentTag = Command.Type;
	Invocation.SubjectName = Payload.ChoiceName;
	Invocation.ActorSeat = Command.SeatIndex;
	Invocation.WindowName = Payload.WindowName;
	Invocation.SourceCommandId = Command.CommandId;
	Invocation.SourceRequestId = Command.RequestId;
	Invocation.Payload = FInstancedStruct::Make(RulePayload);
	return MakeValue(MoveTemp(Invocation));
}
