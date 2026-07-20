#pragma once

// 汇合 Command、Random 与 Event 三条流的回放地基。
// CommandLog 记录意图生命周期，RandomLog 记录确定性随机选择，EventLog 记录规则事实。
// CommandId、EffectStepId 与 TimingPoint 一起保存因果顺序，供后续回放校验使用。

#include "CoreMinimal.h"
#include "Shared/Core/SGSRandomAudit.h"
#include "Shared/Cards/SGSCardTypes.h"
#include "Server/Commands/SGSCommandRouter.h"
#include "Server/Timing/SGSTimingTypes.h"

// 服务器权威的牌移动事实。Reason 是开放标识，规则层可扩展而无需让表现层依赖规则类。
struct SGS_API FSGSCardMoveEventPayload
{
	int32 MatchEpoch = 0;
	int32 Sequence = INDEX_NONE;
	FName Reason = NAME_None;
	TArray<int32> CardIds;
	FSGSCardZone FromZone = SGSGameplayTags::CardZone_None.GetTag();
	int32 FromSeat = INDEX_NONE;
	FSGSCardZone ToZone = SGSGameplayTags::CardZone_None.GetTag();
	int32 ToSeat = INDEX_NONE;
	TArray<int32> RelatedTargetSeatIndices;

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(MatchEpoch >= 0, TEXT("Card move match epoch must be non-negative."));
		bOk &= ensureMsgf(Sequence >= 0, TEXT("Card move sequence must be non-negative."));
		bOk &= ensureMsgf(!Reason.IsNone(), TEXT("Card move reason cannot be empty."));
		bOk &= ensureMsgf(FromZone.IsValid() && ToZone.IsValid(), TEXT("Card move zones must be valid."));
		for (const int32 CardId : CardIds)
		{
			bOk &= ensureMsgf(CardId >= 0, TEXT("Card move CardId must be non-negative."));
		}
		return bOk;
	}
};

struct SGS_API FSGSEventLogEntry
{
	int32 Sequence = INDEX_NONE;
	FName EventName = NAME_None;
	FSGSCommandId CommandId;
	FSGSEffectStepId EffectStepId;
	FSGSTimingPoint TimingPoint;
	FString Payload;
	TOptional<FSGSCardMoveEventPayload> CardMove;

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(Sequence >= 0, TEXT("EventLog sequence must be non-negative."));
		bOk &= ensureMsgf(!EventName.IsNone(), TEXT("EventLog event name cannot be empty."));
		bOk &= CommandId.CheckInvariants();
		bOk &= EffectStepId.CheckInvariants();
		bOk &= TimingPoint.CheckInvariants();
		if (CardMove.IsSet())
		{
			bOk &= CardMove->CheckInvariants();
		}
		return bOk;
	}
};

class SGS_API FSGSReplayLog
{
public:
	void Reset(int32 InMatchEpoch = 0);

	void SetCommandLog(const TArray<FSGSCommandLogEntry>& InCommandLog);
	void SetRandomLog(const TArray<FSGSRandomLogEntry>& InRandomLog);
	void AppendEvent(FName EventName, FSGSCommandId CommandId, FSGSEffectStepId EffectStepId, FSGSTimingPoint TimingPoint, FString Payload);
	void AppendCardMoveEvent(
		FSGSCommandId CommandId,
		FSGSEffectStepId EffectStepId,
		FSGSTimingPoint TimingPoint,
		FSGSCardMoveEventPayload Payload);

	const TArray<FSGSCommandLogEntry>& GetCommandLog() const { return CommandLog; }
	const TArray<FSGSRandomLogEntry>& GetRandomLog() const { return RandomLog; }
	const TArray<FSGSEventLogEntry>& GetEventLog() const { return EventLog; }

	bool CheckInvariants() const;

private:
	TArray<FSGSCommandLogEntry> CommandLog;
	TArray<FSGSRandomLogEntry> RandomLog;
	TArray<FSGSEventLogEntry> EventLog;
	int32 NextEventSequence = 0;
	int32 MatchEpoch = 0;
	int32 NextCardMoveSequence = 0;
};
