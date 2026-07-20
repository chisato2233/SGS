#include "Server/Effects/SGSReplayLog.h"

void FSGSReplayLog::Reset(int32 InMatchEpoch)
{
	CommandLog.Reset();
	RandomLog.Reset();
	EventLog.Reset();
	NextEventSequence = 0;
	MatchEpoch = FMath::Max(InMatchEpoch, 0);
	NextCardMoveSequence = 0;
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

void FSGSReplayLog::AppendCardMoveEvent(
	FSGSCommandId CommandId,
	FSGSEffectStepId EffectStepId,
	FSGSTimingPoint TimingPoint,
	FSGSCardMoveEventPayload Payload)
{
	Payload.MatchEpoch = MatchEpoch;
	Payload.Sequence = NextCardMoveSequence++;
	FSGSEventLogEntry Entry;
	Entry.Sequence = NextEventSequence++;
	Entry.EventName = FName(TEXT("SGS.Event.CardMove"));
	Entry.CommandId = CommandId;
	Entry.EffectStepId = EffectStepId;
	Entry.TimingPoint = TimingPoint;
	Entry.Payload = FString::Printf(
		TEXT("Reason=%s Count=%d From=%s@%d To=%s@%d"),
		*Payload.Reason.ToString(),
		Payload.CardIds.Num(),
		*Payload.FromZone.ToString(),
		Payload.FromSeat,
		*Payload.ToZone.ToString(),
		Payload.ToSeat);
	Entry.CardMove = MoveTemp(Payload);
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
	int32 ExpectedCardMoveSequence = 0;
	for (int32 Index = 0; Index < EventLog.Num(); ++Index)
	{
		bOk &= ensureMsgf(EventLog[Index].Sequence == Index, TEXT("Replay EventLog sequence mismatch."));
		bOk &= EventLog[Index].CheckInvariants();
		if (EventLog[Index].CardMove.IsSet())
		{
			bOk &= ensureMsgf(
				EventLog[Index].CardMove->Sequence == ExpectedCardMoveSequence++,
				TEXT("Replay card move sequence mismatch."));
			bOk &= ensureMsgf(
				EventLog[Index].CardMove->MatchEpoch == MatchEpoch,
				TEXT("Replay card move epoch mismatch."));
		}
	}
	return bOk;
}
