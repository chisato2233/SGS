#include "Server/Commands/Types/SGSPassCommandType.h"

#include "Shared/Commands/SGSCommandPayloadTraits.h"
#include "Server/Rules/Core/SGSRuleInvocation.h"

FGameplayTag FSGSPassCommandType::GetType() const
{
	return TSGSCommandPayloadTraits<FSGSPassCommandPayload>::GetType();
}

FSGSStatus FSGSPassCommandType::ValidateTyped(
	const FSGSCommand& Command,
	const FSGSPassCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	return MakeValue();
}

TSGSResult<FSGSRuleInvocation> FSGSPassCommandType::BuildRuleInvocationTyped(
	const FSGSCommand& Command,
	const FSGSPassCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	FSGSPassRulePayload RulePayload;
	RulePayload.WindowName = Context.ExpectedWindowName;
	RulePayload.RequiredCardName = Context.RequiredCardName;
	RulePayload.EffectSourceSeat = Context.EffectSourceSeatIndex;
	RulePayload.EffectTargetSeat = Context.EffectTargetSeatIndex;

	FSGSRuleInvocation Invocation;
	Invocation.RuleKindTag = Context.ExpectedWindowName.IsNone() ? SGSRuleKinds::Action() : SGSRuleKinds::Response();
	Invocation.IntentTag = Command.Type;
	Invocation.SubjectName = NAME_None;
	Invocation.ActorSeat = Command.SeatIndex;
	Invocation.WindowName = Context.ExpectedWindowName;
	Invocation.SourceCommandId = Command.CommandId;
	Invocation.SourceRequestId = Command.RequestId;
	Invocation.Payload = FInstancedStruct::Make(RulePayload);
	return MakeValue(MoveTemp(Invocation));
}
