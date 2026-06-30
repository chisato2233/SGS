#pragma once

// SGS 规则状态变更的有序执行管线。
// 将命名 EffectStep 放入队列，用 follow-up step 和挂起机制表达连锁与响应。
// 权威移牌、伤害、回复和响应窗口都应走这条可回放审计路径。

#include "CoreMinimal.h"
#include "Shared/Core/SGSError.h"
#include "Shared/Commands/SGSCommand.h"
#include "Server/Effects/SGSReplayLog.h"
#include "Server/Timing/SGSActiveEffectTimeline.h"

class USGSGameContext;

struct SGS_API FSGSEffectContext
{
	USGSGameContext* GameContext = nullptr;
	FSGSRandomAudit* RandomAudit = nullptr;
	FSGSReplayLog* ReplayLog = nullptr;
	FSGSActiveEffectTimeline* ActiveEffects = nullptr;
	const FSGSCommand* Command = nullptr;
	FSGSCommandId CommandId;
	FSGSEffectStepId CurrentStepId;
	FSGSTimingPoint TimingPoint;

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(GameContext != nullptr, TEXT("EffectContext requires a GameContext."));
		bOk &= ensureMsgf(ReplayLog != nullptr, TEXT("EffectContext requires a ReplayLog."));
		bOk &= CommandId.CheckInvariants();
		bOk &= CurrentStepId.CheckInvariants();
		bOk &= TimingPoint.CheckInvariants();
		return bOk;
	}
};

struct FSGSEffectStep;

struct SGS_API FSGSEffectResult
{
	bool bSuspend = false;
	FSGSError Error;
	TArray<FSGSEffectStep> FollowUpSteps;

	bool HasError() const { return Error.IsValid(); }

	static FSGSEffectResult Success();
	static FSGSEffectResult Suspend();
	static FSGSEffectResult Failure(FSGSError InError);
};

struct SGS_API FSGSEffectStep
{
	FSGSEffectStepId StepId;
	FName StepName = NAME_None;
	FName SourceName = NAME_None;
	bool bCanSuspend = false;
	TFunction<FSGSEffectResult(FSGSEffectContext&)> Execute;

	bool CheckInvariants() const
	{
		bool bOk = StepId.CheckInvariants();
		bOk &= ensureMsgf(!StepName.IsNone(), TEXT("EffectStep requires a name."));
		bOk &= ensureMsgf(Execute != nullptr, TEXT("EffectStep requires an execute function."));
		return bOk;
	}
};

class SGS_API FSGSEffectQueue
{
public:
	void Reset();
	void Enqueue(FSGSEffectStep Step);
	void EnqueueFront(FSGSEffectStep Step);
	void EnqueueManyFront(TArray<FSGSEffectStep> Steps);
	bool IsEmpty() const { return Steps.Num() == 0; }
	int32 Num() const { return Steps.Num(); }
	FSGSEffectStep PopFront();
	bool CheckInvariants() const;

private:
	TArray<FSGSEffectStep> Steps;
};

class SGS_API FSGSEffectPipeline
{
public:
	void Reset();
	FSGSEffectStepId AllocateStepId();
	void Enqueue(FSGSEffectStep Step);
	void EnqueueFront(FSGSEffectStep Step);
	FSGSStatus Run(FSGSEffectContext& Context);
	FSGSStatus Resume(FSGSEffectContext& Context);

	bool IsSuspended() const { return bSuspended; }
	bool CheckInvariants() const;

private:
	FSGSStatus RunInternal(FSGSEffectContext& Context);
	void AppendStepEvent(FSGSEffectContext& Context, FName EventName, const FSGSEffectStep& Step, FString Payload);

	FSGSEffectQueue Queue;
	int32 NextStepIdValue = 0;
	bool bSuspended = false;
};
