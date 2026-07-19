#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/SGSError.h"
#include "Shared/Core/SGSIds.h"
#include "Shared/Core/SGSIndexedStore.h"
#include "StructUtils/InstancedStruct.h"

namespace SGSResolutionContinuations
{
	inline FName FinishParentCardResolution() { return FName(TEXT("SGS.Resolution.Continuation.FinishParentCardResolution")); }
	inline FName ResumeDyingPeach() { return FName(TEXT("SGS.Resolution.Continuation.ResumeDyingPeach")); }
}

struct SGS_API FSGSResolutionFrame
{
	FSGSStableHandle ParentFrameHandle;
	FName SourceRuleName = NAME_None;
	FSGSCommandId SourceCommandId;
	int32 ActorSeat = INDEX_NONE;
	int32 SourceSeat = INDEX_NONE;
	int32 TargetSeat = INDEX_NONE;
	FName WindowName = NAME_None;
	FName RequiredCardName = NAME_None;
	TArray<FName> AcceptedCardNames;
	int32 ProcessingCardId = INDEX_NONE;
	int32 StackSequence = INDEX_NONE;
	FName OnChildCompletedContinuation = NAME_None;
	FInstancedStruct FrameState;

	bool HasState() const { return FrameState.IsValid(); }
	const UScriptStruct* GetStateStruct() const { return FrameState.GetScriptStruct(); }

	template <typename TState>
	const TState* GetState() const
	{
		return FrameState.GetPtr<TState>();
	}

	template <typename TState>
	TState* GetMutableState()
	{
		return FrameState.GetMutablePtr<TState>();
	}

	FString GetStateTypeName() const;
	FString ToLogString() const;
	bool CheckInvariants() const;
};

class SGS_API FSGSResolutionStack
{
public:
	FSGSResolutionStack();

	void Reset();
	FSGSStableHandle PushFrame(FSGSResolutionFrame Frame);
	FSGSStatus PopFrame(FSGSStableHandle FrameHandle);
	TSGSResult<FSGSResolutionFrame> CompleteCurrentFrame(FName Reason);
	FSGSStatus AbortAllFrames(FName Reason);
	void Clear();

	FSGSStableHandle GetCurrentFrameHandle() const;
	FSGSResolutionFrame* GetCurrentFrame();
	const FSGSResolutionFrame* GetCurrentFrame() const;
	FSGSResolutionFrame* FindFrame(FSGSStableHandle FrameHandle);
	const FSGSResolutionFrame* FindFrame(FSGSStableHandle FrameHandle) const;
	FSGSStableHandle GetParentFrameHandle(FSGSStableHandle FrameHandle) const;
	FSGSResolutionFrame* GetParentFrame(FSGSStableHandle FrameHandle);
	const FSGSResolutionFrame* GetParentFrame(FSGSStableHandle FrameHandle) const;

	FSGSStatus OpenResponseWindowOnCurrent(
		FName WindowName,
		FName RequiredCardName,
		TConstArrayView<FName> AcceptedCardNames,
		int32 EffectSourceSeat,
		int32 EffectTargetSeat);
	FSGSStatus ClearResponseWindowOnCurrent();

	template <typename TState>
	const FSGSResolutionFrame* FindLatestFrameWithState() const
	{
		for (int32 Index = StackOrder.Num() - 1; Index >= 0; --Index)
		{
			const FSGSResolutionFrame* Frame = Frames.Find(StackOrder[Index]);
			if (Frame != nullptr && Frame->GetState<TState>() != nullptr)
			{
				return Frame;
			}
		}

		return nullptr;
	}

	template <typename TState>
	FSGSResolutionFrame* FindLatestFrameWithState()
	{
		for (int32 Index = StackOrder.Num() - 1; Index >= 0; --Index)
		{
			FSGSResolutionFrame* Frame = Frames.Find(StackOrder[Index]);
			if (Frame != nullptr && Frame->GetMutableState<TState>() != nullptr)
			{
				return Frame;
			}
		}

		return nullptr;
	}

	template <typename Predicate>
	FSGSResolutionFrame* FindLatestFrameByPredicate(Predicate&& Matches)
	{
		for (int32 Index = StackOrder.Num() - 1; Index >= 0; --Index)
		{
			FSGSResolutionFrame* Frame = Frames.Find(StackOrder[Index]);
			if (Frame != nullptr && Matches(*Frame))
			{
				return Frame;
			}
		}

		return nullptr;
	}

	template <typename Predicate>
	const FSGSResolutionFrame* FindLatestFrameByPredicate(Predicate&& Matches) const
	{
		for (int32 Index = StackOrder.Num() - 1; Index >= 0; --Index)
		{
			const FSGSResolutionFrame* Frame = Frames.Find(StackOrder[Index]);
			if (Frame != nullptr && Matches(*Frame))
			{
				return Frame;
			}
		}

		return nullptr;
	}

	int32 FindLatestProcessingCardId() const;
	int32 FindLatestEffectSourceSeat() const;
	int32 FindLatestEffectTargetSeat() const;
	bool CheckInvariants() const;

private:
	void InitializeStore();

	TSGSIndexedStore<FSGSResolutionFrame> Frames;
	TArray<FSGSStableHandle> StackOrder;
	int32 NextStackSequence = 0;

	TSharedPtr<TSGSIndexedStore<FSGSResolutionFrame>::TNonUniqueIndex<FName>> FramesByWindow;
	TSharedPtr<TSGSIndexedStore<FSGSResolutionFrame>::TNonUniqueIndex<FSGSCommandId>> FramesByCommandId;
	TSharedPtr<TSGSIndexedStore<FSGSResolutionFrame>::TNonUniqueIndex<int32>> FramesByActorSeat;
};
