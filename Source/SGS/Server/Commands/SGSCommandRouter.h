#pragma once

// 服务器权威的 Command 校验与分发层。
// 每个 Command 标签注册一个 CommandType，原始意图统一提交到这里并记录生命周期审计。
// Router 负责通用信封检查；CommandType 负责具体类型的规则校验与 RuleInvocation 构造。

#include "CoreMinimal.h"
#include "Shared/Core/SGSError.h"
#include "Shared/Commands/SGSCommand.h"
#include "Server/Commands/SGSCommandTypes.h"

class USGSGameContext;

namespace SGSCommandLifecycle
{
	inline FName Created() { return FName(TEXT("SGS.CommandLifecycle.Created")); }
	inline FName Received() { return FName(TEXT("SGS.CommandLifecycle.Received")); }
	inline FName Validated() { return FName(TEXT("SGS.CommandLifecycle.Validated")); }
	inline FName Rejected() { return FName(TEXT("SGS.CommandLifecycle.Rejected")); }
	inline FName Executed() { return FName(TEXT("SGS.CommandLifecycle.Executed")); }
	inline FName Fallbacked() { return FName(TEXT("SGS.CommandLifecycle.Fallbacked")); }
}

struct SGS_API FSGSCommandLogEntry
{
	int32 Sequence = INDEX_NONE;
	FName Lifecycle = NAME_None;
	FSGSCommand Command;
	bool bSucceeded = true;
	FSGSError Error;
	FString Detail;
};

class SGS_API FSGSCommandRouter
{
public:
	FSGSCommandRouter();

	void Reset();
	void RegisterCommandType(TSharedRef<ISGSCommandType> CommandType);

	FSGSStatus SubmitCommand(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context);
	TSGSResult<FSGSRuleInvocation> BuildRuleInvocation(
		const FSGSCommand& Command,
		const FSGSCommandExecutionContext& Context) const;
	void RecordLifecycle(const FSGSCommand& Command, FName Lifecycle, bool bSucceeded, FSGSError Error = FSGSError(), FString Detail = FString());

	const TArray<FSGSCommandLogEntry>& GetLogEntries() const { return LogEntries; }
	bool CheckInvariants() const;

private:
	FSGSStatus ValidateCommon(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const;
	const ISGSCommandType* FindTypeSpec(FGameplayTag Type) const;
	FString FormatPayloadForLog(const FSGSCommand& Command) const;

	TArray<TSharedRef<ISGSCommandType>> TypeSpecs;
	TArray<FSGSCommandLogEntry> LogEntries;
	int32 NextLogSequence = 0;
};
