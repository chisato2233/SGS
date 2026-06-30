#pragma once

// Command type 是 SGS 命令协议的唯一事实源：一个类型同时声明 GameplayTag、
// PayloadStruct、payload 日志、legacy mirror 同步，以及上下文相关校验 / 执行。
// FSGSCommand 只保存公共头和 typed payload；构造与分发都回到这里查类型定义。

#include "CoreMinimal.h"
#include "Shared/Commands/SGSCommand.h"
#include "Shared/Core/SGSError.h"

class FSGSCommandRouter;
class USGSGameContext;

struct SGS_API FSGSCommandExecutionContext
{
	USGSGameContext* GameContext = nullptr;
	FSGSCommandId ExpectedCommandId;
	int32 ExpectedRequestId = 0;
	int32 ExpectedSeatIndex = INDEX_NONE;
	FSGSPhase ExpectedPhase = SGSGameplayTags::Phase_None.GetTag();
	FName ExpectedWindowName = NAME_None;
	FName RequiredCardName = NAME_None;
	int32 EffectTargetSeatIndex = INDEX_NONE;
};

class SGS_API ISGSCommandType
{
public:
	virtual ~ISGSCommandType() = default;

	virtual FGameplayTag GetType() const = 0;
	virtual const UScriptStruct* GetPayloadStruct() const = 0;
	virtual FString FormatPayloadSummary(const FSGSCommand& Command) const = 0;
	virtual void SyncLegacyMirror(FSGSCommand& Command) const {}
	virtual FSGSStatus Validate(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const = 0;
	virtual FSGSStatus Execute(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const = 0;

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(GetType().IsValid(), TEXT("Command type has invalid GameplayTag."));
		bOk &= ensureMsgf(GetPayloadStruct() != nullptr, TEXT("Command type %s has no payload struct."), *GetType().ToString());
		return bOk;
	}
};

template <typename TPayload>
class TSGSCommandType : public ISGSCommandType
{
public:
	virtual const UScriptStruct* GetPayloadStruct() const override
	{
		return TPayload::StaticStruct();
	}

	virtual FString FormatPayloadSummary(const FSGSCommand& Command) const override
	{
		if (const TPayload* Payload = Command.GetPayload<TPayload>())
		{
			return Payload->ToPayloadLogString();
		}

		return Command.HasPayload()
			? FString::Printf(TEXT("Expected=%s Actual=%s"), *TPayload::StaticStruct()->GetName(), *Command.GetPayloadTypeName())
			: TEXT("NoPayload");
	}

	virtual FSGSStatus Validate(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const override
	{
		const TPayload* Payload = Command.GetPayload<TPayload>();
		if (Payload == nullptr)
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.Command.InvalidPayload")),
				FString::Printf(TEXT("Command type %s requires payload %s."), *GetType().ToString(), *TPayload::StaticStruct()->GetName())));
		}

		return ValidateTyped(Command, *Payload, Context);
	}

	virtual FSGSStatus Execute(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const override
	{
		const TPayload* Payload = Command.GetPayload<TPayload>();
		if (Payload == nullptr)
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.Command.InvalidPayload")),
				FString::Printf(TEXT("Command type %s requires payload %s."), *GetType().ToString(), *TPayload::StaticStruct()->GetName())));
		}

		return ExecuteTyped(Command, *Payload, Context);
	}

protected:
	virtual FSGSStatus ValidateTyped(const FSGSCommand& Command, const TPayload& Payload, const FSGSCommandExecutionContext& Context) const = 0;

	virtual FSGSStatus ExecuteTyped(const FSGSCommand& Command, const TPayload& Payload, const FSGSCommandExecutionContext& Context) const
	{
		return MakeValue();
	}
};

namespace SGSCommandTypes
{
	SGS_API void RegisterDefaultCommandTypes(FSGSCommandRouter& Router);
}
