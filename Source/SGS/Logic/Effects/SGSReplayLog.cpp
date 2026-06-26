#include "Logic/Effects/SGSReplayLog.h"

void FSGSReplayLog::Reset()
{
	CommandLog.Reset();
	RandomLog.Reset();
	EventLog.Reset();
	NextEventSequence = 0;
}

void FSGSReplayLog::SetCommandLog(const TArray<FSGSCommandLogEntry>& InCommandLog)
{
	CommandLog = InCommandLog;
}

void FSGSReplayLog::SetRandomLog(const TArray<FSGSRandomLogEntry>& InRandomLog)
{
	RandomLog = InRandomLog;
}

void FSGSReplayLog::AppendEvent(
	FName EventName,
	FSGSCommandId CommandId,
	FSGSEffectStepId EffectStepId,
	FSGSTimingPoint TimingPoint,
	FString Payload)
{
	FSGSEventLogEntry Entry;
	Entry.Sequence = NextEventSequence++;
	Entry.EventName = EventName;
	Entry.CommandId = CommandId;
	Entry.EffectStepId = EffectStepId;
	Entry.TimingPoint = TimingPoint;
	Entry.Payload = MoveTemp(Payload);
	EventLog.Add(MoveTemp(Entry));
}

bool FSGSReplayLog::CheckInvariants() const
{
	bool bOk = true;
	for (int32 Index = 0; Index < CommandLog.Num(); ++Index)
	{
		bOk &= ensureMsgf(CommandLog[Index].Sequence == Index, TEXT("Replay CommandLog sequence mismatch."));
		bOk &= CommandLog[Index].Command.CheckInvariants();
		if (CommandLog[Index].Error.IsValid())
		{
			bOk &= CommandLog[Index].Error.CheckInvariants();
		}
	}
	for (int32 Index = 0; Index < RandomLog.Num(); ++Index)
	{
		bOk &= ensureMsgf(RandomLog[Index].Step == Index, TEXT("Replay RandomLog sequence mismatch."));
		bOk &= RandomLog[Index].CommandId.CheckInvariants();
	}
	for (int32 Index = 0; Index < EventLog.Num(); ++Index)
	{
		bOk &= ensureMsgf(EventLog[Index].Sequence == Index, TEXT("Replay EventLog sequence mismatch."));
		bOk &= EventLog[Index].CheckInvariants();
	}
	return bOk;
}
