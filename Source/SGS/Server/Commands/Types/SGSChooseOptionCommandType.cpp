#include "Server/Commands/Types/SGSChooseOptionCommandType.h"

#include "Shared/Commands/SGSCommandPayloadTraits.h"
#include "Server/Rules/Payloads/SGSRuleResponsePayloads.h"

FGameplayTag FSGSChooseOptionCommandType::GetType() const
{
	return TSGSCommandPayloadTraits<FSGSChooseOptionCommandPayload>::GetType();
}

FSGSStatus FSGSChooseOptionCommandType::ValidateTyped(
	const FSGSCommand&,
	const FSGSChooseOptionCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	if (Context.bIsOptionChoice
		&& Payload.WindowName == Context.ExpectedWindowName
		&& Payload.ChoiceName == Context.ExpectedChoiceName
		&& Context.SelectableNamedOptions.Contains(Payload.SelectedOption))
	{
		return MakeValue();
	}
	return MakeError(FSGSError::Make(
		FName(TEXT("SGS.Command.InvalidOptionChoice")),
		TEXT("ChooseOption does not match the pending server option set.")));
}

TSGSResult<FSGSRuleInvocation> FSGSChooseOptionCommandType::BuildRuleInvocationTyped(
	const FSGSCommand& Command,
	const FSGSChooseOptionCommandPayload& Payload,
	const FSGSCommandExecutionContext&) const
{
	FSGSChooseOptionRulePayload RulePayload;
	RulePayload.ChoiceName = Payload.ChoiceName;
	RulePayload.WindowName = Payload.WindowName;
	RulePayload.SelectedOption = Payload.SelectedOption;

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
