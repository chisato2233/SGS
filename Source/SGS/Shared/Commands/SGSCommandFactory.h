#pragma once

// FSGSCommandFactory 是 shared 命令构造门面。调用方提供公共构造请求与
// typed payload；GameplayTag 和 legacy mirror 由 shared payload traits 决定。

#include "CoreMinimal.h"
#include "Shared/Commands/SGSCommandPayloadTraits.h"

struct SGS_API FSGSCommandBuildRequest
{
	FSGSCommandId CommandId;
	int32 RequestId = 0;
	int32 SeatIndex = INDEX_NONE;
	FSGSPhase Phase = SGSGameplayTags::Phase_None.GetTag();
	FName SourceChannel = NAME_None;
	FName SourceName = NAME_None;

	template <typename TDecisionRequest>
	static FSGSCommandBuildRequest FromDecisionRequest(
		const TDecisionRequest& DecisionRequest,
		FName InSourceChannel,
		FName InSourceName)
	{
		FSGSCommandBuildRequest Request;
		Request.CommandId = DecisionRequest.CommandId;
		Request.RequestId = DecisionRequest.RequestId;
		Request.SeatIndex = DecisionRequest.SeatIndex;
		Request.Phase = DecisionRequest.Phase;
		Request.SourceChannel = InSourceChannel;
		Request.SourceName = InSourceName;
		return Request;
	}
};

class SGS_API FSGSCommandFactory
{
public:
	template <typename TPayload>
	static FSGSCommand Make(
		const FSGSCommandBuildRequest& Request,
		const TPayload& Payload)
	{
		FSGSCommand Command;
		Command.CommandId = Request.CommandId;
		Command.RequestId = Request.RequestId;
		Command.SeatIndex = Request.SeatIndex;
		Command.Type = TSGSCommandPayloadTraits<TPayload>::GetType();
		Command.Phase = Request.Phase;
		Command.Payload = FInstancedStruct::Make(Payload);
		Command.SourceChannel = Request.SourceChannel;
		Command.SourceName = Request.SourceName;
		TSGSCommandPayloadTraits<TPayload>::SyncLegacyMirror(Command);
		return Command;
	}
};
