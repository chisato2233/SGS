#pragma once

// 汇合 Command、Random 与 Event 三条流的回放地基。
// CommandLog 记录意图生命周期，RandomLog 记录确定性随机选择，EventLog 记录规则事实。
// CommandId、EffectStepId 与 TimingPoint 一起保存因果顺序，供后续回放校验使用。

#include "CoreMinimal.h"
#include "Shared/Core/SGSRandomAudit.h"
#include "Server/Commands/SGSCommandRouter.h"
#include "Server/Timing/SGSTimingTypes.h"

struct SGS_API FSGSEventLogEntry
{
	int32 Sequence = INDEX_NONE;
	FName EventName = NAME_None;
	FSGSCommandId CommandId;
	FSGSEffectStepId EffectStepId;
	FSGSTimingPoint TimingPoint;
	FString Payload;

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(Sequence >= 0, TEXT("EventLog sequence must be non-negative."));
		bOk &= ensureMsgf(!EventName.IsNone(), TEXT("EventLog event name cannot be empty."));
		bOk &= CommandId.CheckInvariants();
		bOk &= EffectStepId.CheckInvariants();
		bOk &= TimingPoint.CheckInvariants();
		return bOk;
	}
};

class SGS_API FSGSReplayLog
{
public:
	void Reset();

	void SetCommandLog(const TArray<FSGSCommandLogEntry>& InCommandLog);
	void SetRandomLog(const TArray<FSGSRandomLogEntry>& InRandomLog);
	void AppendEvent(FName EventName, FSGSCommandId CommandId, FSGSEffectStepId EffectStepId, FSGSTimingPoint TimingPoint, FString Payload);

	const TArray<FSGSCommandLogEntry>& GetCommandLog() const { return CommandLog; }
	const TArray<FSGSRandomLogEntry>& GetRandomLog() const { return RandomLog; }
	const TArray<FSGSEventLogEntry>& GetEventLog() const { return EventLog; }

	bool CheckInvariants() const;

private:
	TArray<FSGSCommandLogEntry> CommandLog;
	TArray<FSGSRandomLogEntry> RandomLog;
	TArray<FSGSEventLogEntry> EventLog;
	int32 NextEventSequence = 0;
};
