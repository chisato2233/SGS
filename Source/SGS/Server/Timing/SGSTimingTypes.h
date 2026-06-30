#pragma once

// SGS 回合、阶段、响应与插入步骤使用的开放式时序 / 持续时间词汇。
// TimingPoint 为日志和持续效果提供稳定排序键，但不把阶段建模成封闭枚举。
// DurationSpec 覆盖常见卡牌游戏生命周期，同时允许用 FName 扩展新持续类型。

#include "CoreMinimal.h"
#include "Shared/Core/SGSIds.h"
#include "Shared/Core/SGSTypes.h"

namespace SGSTimingSteps
{
	inline FName Begin() { return FName(TEXT("Begin")); }
	inline FName Resolve() { return FName(TEXT("Resolve")); }
	inline FName End() { return FName(TEXT("End")); }
	inline FName After() { return FName(TEXT("After")); }
	inline FName ReactionWindow() { return FName(TEXT("ReactionWindow")); }
}

namespace SGSDurationKinds
{
	inline FName ThisTurn() { return FName(TEXT("SGS.Duration.ThisTurn")); }
	inline FName ThisPhase() { return FName(TEXT("SGS.Duration.ThisPhase")); }
	inline FName TargetNextTurn() { return FName(TEXT("SGS.Duration.TargetNextTurn")); }
	inline FName ThisRound() { return FName(TEXT("SGS.Duration.ThisRound")); }
	inline FName EventCount() { return FName(TEXT("SGS.Duration.EventCount")); }
	inline FName UntilTimingPoint() { return FName(TEXT("SGS.Duration.UntilTimingPoint")); }
	inline FName Manual() { return FName(TEXT("SGS.Duration.Manual")); }
}

struct SGS_API FSGSTimingPoint
{
	int64 Sequence = INDEX_NONE;
	int32 TurnSerial = INDEX_NONE;
	int32 TurnOwnerSeat = INDEX_NONE;
	int32 PhaseSerial = INDEX_NONE;
	FSGSPhase Phase = SGSGameplayTags::Phase_None.GetTag();
	FName Step = NAME_None;
	int32 SubOrder = 0;

	bool IsValid() const
	{
		return Sequence >= 0 && TurnSerial >= 0 && Step != NAME_None;
	}

	bool CheckInvariants() const
	{
		return ensureMsgf(Sequence == INDEX_NONE || Sequence >= 0, TEXT("TimingPoint sequence must be invalid or non-negative."))
			&& ensureMsgf(TurnSerial == INDEX_NONE || TurnSerial >= 0, TEXT("TimingPoint turn serial must be invalid or non-negative."))
			&& ensureMsgf(PhaseSerial == INDEX_NONE || PhaseSerial >= 0, TEXT("TimingPoint phase serial must be invalid or non-negative."));
	}

	FString ToLogString() const
	{
		return FString::Printf(
			TEXT("Seq=%lld Turn=%d Owner=%d PhaseSerial=%d Phase=%s Step=%s Sub=%d"),
			Sequence,
			TurnSerial,
			TurnOwnerSeat,
			PhaseSerial,
			*Phase.ToString(),
			*Step.ToString(),
			SubOrder);
	}

	static FSGSTimingPoint Make(
		int64 InSequence,
		int32 InTurnSerial,
		int32 InTurnOwnerSeat,
		int32 InPhaseSerial,
		FSGSPhase InPhase,
		FName InStep,
		int32 InSubOrder = 0)
	{
		FSGSTimingPoint Point;
		Point.Sequence = InSequence;
		Point.TurnSerial = InTurnSerial;
		Point.TurnOwnerSeat = InTurnOwnerSeat;
		Point.PhaseSerial = InPhaseSerial;
		Point.Phase = InPhase;
		Point.Step = InStep;
		Point.SubOrder = InSubOrder;
		return Point;
	}

	friend bool operator==(const FSGSTimingPoint& Left, const FSGSTimingPoint& Right)
	{
		return Left.Sequence == Right.Sequence
			&& Left.TurnSerial == Right.TurnSerial
			&& Left.TurnOwnerSeat == Right.TurnOwnerSeat
			&& Left.PhaseSerial == Right.PhaseSerial
			&& Left.Phase.MatchesTagExact(Right.Phase)
			&& Left.Step == Right.Step
			&& Left.SubOrder == Right.SubOrder;
	}

	friend bool operator!=(const FSGSTimingPoint& Left, const FSGSTimingPoint& Right)
	{
		return !(Left == Right);
	}

	friend bool operator<(const FSGSTimingPoint& Left, const FSGSTimingPoint& Right)
	{
		if (Left.Sequence != Right.Sequence)
		{
			return Left.Sequence < Right.Sequence;
		}
		if (Left.TurnSerial != Right.TurnSerial)
		{
			return Left.TurnSerial < Right.TurnSerial;
		}
		if (Left.PhaseSerial != Right.PhaseSerial)
		{
			return Left.PhaseSerial < Right.PhaseSerial;
		}
		return Left.SubOrder < Right.SubOrder;
	}
};

inline uint32 GetTypeHash(const FSGSTimingPoint& Point)
{
	uint32 Hash = GetTypeHash(Point.Sequence);
	Hash = HashCombine(Hash, GetTypeHash(Point.TurnSerial));
	Hash = HashCombine(Hash, GetTypeHash(Point.TurnOwnerSeat));
	Hash = HashCombine(Hash, GetTypeHash(Point.PhaseSerial));
	Hash = HashCombine(Hash, GetTypeHash(Point.Phase));
	Hash = HashCombine(Hash, GetTypeHash(Point.Step));
	Hash = HashCombine(Hash, GetTypeHash(Point.SubOrder));
	return Hash;
}

struct SGS_API FSGSDurationSpec
{
	FName Kind = SGSDurationKinds::Manual();
	int32 OwnerSeat = INDEX_NONE;
	int32 TargetSeat = INDEX_NONE;
	FSGSTimingPoint StartPoint;
	FSGSTimingPoint EndPoint;
	int32 RemainingEventCount = 0;
	FGameplayTag EventTag;

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(!Kind.IsNone(), TEXT("Duration kind cannot be empty."));
		bOk &= StartPoint.CheckInvariants();
		if (Kind == SGSDurationKinds::UntilTimingPoint())
		{
			bOk &= ensureMsgf(EndPoint.IsValid(), TEXT("UntilTimingPoint duration requires a valid end point."));
		}
		if (Kind == SGSDurationKinds::EventCount())
		{
			bOk &= ensureMsgf(RemainingEventCount > 0, TEXT("EventCount duration requires a positive count."));
			bOk &= ensureMsgf(EventTag.IsValid(), TEXT("EventCount duration requires an event tag."));
		}
		return bOk;
	}

	FString ToLogString() const
	{
		return FString::Printf(
			TEXT("Kind=%s Owner=%d Target=%d Events=%d EventTag=%s Start={%s} End={%s}"),
			*Kind.ToString(),
			OwnerSeat,
			TargetSeat,
			RemainingEventCount,
			*EventTag.ToString(),
			*StartPoint.ToLogString(),
			*EndPoint.ToLogString());
	}

	static FSGSDurationSpec ThisTurn(int32 InOwnerSeat, FSGSTimingPoint Start)
	{
		FSGSDurationSpec Spec;
		Spec.Kind = SGSDurationKinds::ThisTurn();
		Spec.OwnerSeat = InOwnerSeat;
		Spec.StartPoint = Start;
		return Spec;
	}

	static FSGSDurationSpec ThisPhase(int32 InOwnerSeat, FSGSTimingPoint Start)
	{
		FSGSDurationSpec Spec;
		Spec.Kind = SGSDurationKinds::ThisPhase();
		Spec.OwnerSeat = InOwnerSeat;
		Spec.StartPoint = Start;
		return Spec;
	}

	static FSGSDurationSpec TargetNextTurn(int32 InOwnerSeat, int32 InTargetSeat, FSGSTimingPoint Start)
	{
		FSGSDurationSpec Spec;
		Spec.Kind = SGSDurationKinds::TargetNextTurn();
		Spec.OwnerSeat = InOwnerSeat;
		Spec.TargetSeat = InTargetSeat;
		Spec.StartPoint = Start;
		return Spec;
	}

	static FSGSDurationSpec ThisRound(int32 InOwnerSeat, FSGSTimingPoint Start)
	{
		FSGSDurationSpec Spec;
		Spec.Kind = SGSDurationKinds::ThisRound();
		Spec.OwnerSeat = InOwnerSeat;
		Spec.StartPoint = Start;
		return Spec;
	}

	static FSGSDurationSpec NextEvents(int32 InOwnerSeat, FGameplayTag InEventTag, int32 InCount, FSGSTimingPoint Start)
	{
		FSGSDurationSpec Spec;
		Spec.Kind = SGSDurationKinds::EventCount();
		Spec.OwnerSeat = InOwnerSeat;
		Spec.EventTag = InEventTag;
		Spec.RemainingEventCount = InCount;
		Spec.StartPoint = Start;
		return Spec;
	}

	static FSGSDurationSpec Until(int32 InOwnerSeat, FSGSTimingPoint Start, FSGSTimingPoint End)
	{
		FSGSDurationSpec Spec;
		Spec.Kind = SGSDurationKinds::UntilTimingPoint();
		Spec.OwnerSeat = InOwnerSeat;
		Spec.StartPoint = Start;
		Spec.EndPoint = End;
		return Spec;
	}

	static FSGSDurationSpec Manual(int32 InOwnerSeat, FSGSTimingPoint Start)
	{
		FSGSDurationSpec Spec;
		Spec.Kind = SGSDurationKinds::Manual();
		Spec.OwnerSeat = InOwnerSeat;
		Spec.StartPoint = Start;
		return Spec;
	}
};
