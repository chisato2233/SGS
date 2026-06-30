#pragma once

// 服务器权威的 Command 校验与分发层。
// 每个 Command 标签注册一个 handler，原始意图统一提交到这里并记录生命周期审计。
// Router 负责通用信封检查；handler 负责具体类型的规则校验与执行。

#include "CoreMinimal.h"
#include "Core/SGSError.h"
#include "Logic/Commands/SGSCommand.h"

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

struct SGS_API FSGSCommandLogEntry
{
	int32 Sequence = INDEX_NONE;
	FName Lifecycle = NAME_None;
	FSGSCommand Command;
	bool bSucceeded = true;
	FSGSError Error;
	FString Detail;
};

class SGS_API ISGSCommandHandler
{
public:
	virtual ~ISGSCommandHandler() = default;

	virtual FGameplayTag GetType() const = 0;
	virtual FSGSStatus Validate(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const = 0;
	virtual FSGSStatus Execute(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const = 0;
};

class SGS_API FSGSCommandRouter
{
public:
	FSGSCommandRouter();

	void Reset();
	void RegisterHandler(TSharedRef<ISGSCommandHandler> Handler);

	FSGSStatus SubmitCommand(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context);
	void RecordLifecycle(const FSGSCommand& Command, FName Lifecycle, bool bSucceeded, FSGSError Error = FSGSError(), FString Detail = FString());

	const TArray<FSGSCommandLogEntry>& GetLogEntries() const { return LogEntries; }
	bool CheckInvariants() const;

private:
	FSGSStatus ValidateCommon(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const;
	const ISGSCommandHandler* FindHandler(FGameplayTag Type) const;

	TArray<TSharedRef<ISGSCommandHandler>> Handlers;
	TArray<FSGSCommandLogEntry> LogEntries;
	int32 NextLogSequence = 0;
};
