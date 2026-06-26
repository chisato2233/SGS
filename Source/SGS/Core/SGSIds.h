#pragma once

// Command、EffectPipeline、ReplayLog 与索引 Store 共用的小型稳定标识。
// 顺序日志使用值 ID；运行期实体引用使用能承受数组移动的 StableHandle。
// 无效值只有一种规范形态，CheckInvariants 用来暴露过期或半有效 ID。

#include "CoreMinimal.h"

struct SGS_API FSGSCommandId
{
	int32 Value = INDEX_NONE;

	FSGSCommandId() = default;
	explicit FSGSCommandId(int32 InValue) : Value(InValue) {}

	bool IsValid() const { return Value > 0; }

	bool CheckInvariants() const
	{
		return ensureMsgf(Value == INDEX_NONE || Value > 0, TEXT("CommandId must be invalid or positive: %d"), Value);
	}

	void Reset()
	{
		Value = INDEX_NONE;
	}

	FString ToLogString() const
	{
		return IsValid() ? FString::FromInt(Value) : TEXT("InvalidCommandId");
	}

	static FSGSCommandId Invalid()
	{
		return FSGSCommandId();
	}

	friend bool operator==(const FSGSCommandId& Left, const FSGSCommandId& Right)
	{
		return Left.Value == Right.Value;
	}

	friend bool operator!=(const FSGSCommandId& Left, const FSGSCommandId& Right)
	{
		return !(Left == Right);
	}
};

inline uint32 GetTypeHash(const FSGSCommandId& CommandId)
{
	return GetTypeHash(CommandId.Value);
}

inline FString LexToString(const FSGSCommandId& CommandId)
{
	return CommandId.ToLogString();
}

struct SGS_API FSGSEffectStepId
{
	int32 Value = INDEX_NONE;

	FSGSEffectStepId() = default;
	explicit FSGSEffectStepId(int32 InValue) : Value(InValue) {}

	bool IsValid() const { return Value > 0; }

	bool CheckInvariants() const
	{
		return ensureMsgf(Value == INDEX_NONE || Value > 0, TEXT("EffectStepId must be invalid or positive: %d"), Value);
	}

	void Reset()
	{
		Value = INDEX_NONE;
	}

	FString ToLogString() const
	{
		return IsValid() ? FString::FromInt(Value) : TEXT("InvalidEffectStepId");
	}

	friend bool operator==(const FSGSEffectStepId& Left, const FSGSEffectStepId& Right)
	{
		return Left.Value == Right.Value;
	}

	friend bool operator!=(const FSGSEffectStepId& Left, const FSGSEffectStepId& Right)
	{
		return !(Left == Right);
	}
};

inline uint32 GetTypeHash(const FSGSEffectStepId& StepId)
{
	return GetTypeHash(StepId.Value);
}

inline FString LexToString(const FSGSEffectStepId& StepId)
{
	return StepId.ToLogString();
}

struct SGS_API FSGSStableHandle
{
	int32 Index = INDEX_NONE;
	int32 Generation = 0;

	FSGSStableHandle() = default;
	FSGSStableHandle(int32 InIndex, int32 InGeneration)
		: Index(InIndex)
		, Generation(InGeneration)
	{
	}

	bool IsValid() const { return Index != INDEX_NONE && Generation > 0; }

	bool CheckInvariants() const
	{
		const bool bIsExactlyInvalid = Index == INDEX_NONE && Generation == 0;
		const bool bIsValidShape = Index >= 0 && Generation > 0;
		return ensureMsgf(bIsExactlyInvalid || bIsValidShape,
			TEXT("StableHandle must be exactly invalid or have non-negative index and positive generation: index=%d generation=%d"),
			Index,
			Generation);
	}

	void Reset()
	{
		Index = INDEX_NONE;
		Generation = 0;
	}

	FString ToLogString() const
	{
		return IsValid()
			? FString::Printf(TEXT("%d:%d"), Index, Generation)
			: TEXT("InvalidHandle");
	}

	static FSGSStableHandle Invalid()
	{
		return FSGSStableHandle();
	}

	friend bool operator==(const FSGSStableHandle& Left, const FSGSStableHandle& Right)
	{
		return Left.Index == Right.Index && Left.Generation == Right.Generation;
	}

	friend bool operator!=(const FSGSStableHandle& Left, const FSGSStableHandle& Right)
	{
		return !(Left == Right);
	}
};

inline uint32 GetTypeHash(const FSGSStableHandle& Handle)
{
	return HashCombine(GetTypeHash(Handle.Index), GetTypeHash(Handle.Generation));
}

inline FString LexToString(const FSGSStableHandle& Handle)
{
	return Handle.ToLogString();
}
