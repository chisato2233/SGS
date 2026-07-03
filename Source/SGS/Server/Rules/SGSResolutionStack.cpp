#include "Server/Rules/SGSResolutionStack.h"

FString FSGSSlashResolutionState::ToLogString() const
{
	return FString::Printf(TEXT("SlashCard=%d Source=%d Target=%d"), SlashCardId, SourceSeat, TargetSeat);
}

bool FSGSSlashResolutionState::CheckInvariants() const
{
	bool bOk = true;
	bOk &= ensureMsgf(SlashCardId != INDEX_NONE, TEXT("SlashResolutionState requires a Slash card id."));
	bOk &= ensureMsgf(SourceSeat != INDEX_NONE, TEXT("SlashResolutionState requires a source seat."));
	bOk &= ensureMsgf(TargetSeat != INDEX_NONE, TEXT("SlashResolutionState requires a target seat."));
	return bOk;
}

FString FSGSDyingPeachResolutionState::ToLogString() const
{
	return FString::Printf(
		TEXT("DyingSeat=%d Responders=%d Next=%d Recheck=%s"),
		DyingSeat,
		ResponderSeatIndices.Num(),
		NextResponderIndex,
		bNeedsHealthRecheck ? TEXT("true") : TEXT("false"));
}

bool FSGSDyingPeachResolutionState::CheckInvariants() const
{
	bool bOk = true;
	bOk &= ensureMsgf(DyingSeat != INDEX_NONE, TEXT("DyingPeachResolutionState requires a dying seat."));
	bOk &= ensureMsgf(NextResponderIndex >= 0, TEXT("DyingPeachResolutionState requires a non-negative responder index."));
	bOk &= ensureMsgf(
		NextResponderIndex <= ResponderSeatIndices.Num(),
		TEXT("DyingPeachResolutionState responder index is out of range."));
	for (int32 SeatIndex : ResponderSeatIndices)
	{
		bOk &= ensureMsgf(SeatIndex != INDEX_NONE, TEXT("DyingPeachResolutionState contains an invalid responder."));
	}
	return bOk;
}

FString FSGSResolutionFrame::GetStateTypeName() const
{
	const UScriptStruct* StateStruct = GetStateStruct();
	return StateStruct != nullptr ? StateStruct->GetName() : TEXT("None");
}

FString FSGSResolutionFrame::ToLogString() const
{
	return FString::Printf(
		TEXT("Rule=%s CommandId=%s Actor=%d Source=%d Target=%d Window=%s Required=%s ProcessingCard=%d Seq=%d Continuation=%s State=%s"),
		*SourceRuleName.ToString(),
		*SourceCommandId.ToLogString(),
		ActorSeat,
		SourceSeat,
		TargetSeat,
		*WindowName.ToString(),
		*RequiredCardName.ToString(),
		ProcessingCardId,
		StackSequence,
		*OnChildCompletedContinuation.ToString(),
		*GetStateTypeName());
}

bool FSGSResolutionFrame::CheckInvariants() const
{
	bool bOk = true;
	bOk &= ParentFrameHandle.CheckInvariants();
	bOk &= ensureMsgf(!SourceRuleName.IsNone(), TEXT("ResolutionFrame requires a source rule name."));
	bOk &= SourceCommandId.CheckInvariants();
	bOk &= ensureMsgf(SourceCommandId.IsValid(), TEXT("ResolutionFrame requires a source command id."));
	bOk &= ensureMsgf(ActorSeat != INDEX_NONE, TEXT("ResolutionFrame requires an actor seat."));
	bOk &= ensureMsgf(StackSequence >= 0, TEXT("ResolutionFrame requires a non-negative stack sequence."));
	if (const FSGSSlashResolutionState* SlashState = GetState<FSGSSlashResolutionState>())
	{
		bOk &= SlashState->CheckInvariants();
	}
	if (const FSGSDyingPeachResolutionState* DyingPeachState = GetState<FSGSDyingPeachResolutionState>())
	{
		bOk &= DyingPeachState->CheckInvariants();
	}
	return bOk;
}

FSGSResolutionStack::FSGSResolutionStack()
{
	InitializeStore();
}

void FSGSResolutionStack::InitializeStore()
{
	Frames.Reset();
	FramesByWindow = Frames.RegisterNonUniqueIndex<FName>(
		FName(TEXT("ByWindow")),
		[](const FSGSResolutionFrame& Frame)
		{
			return Frame.WindowName;
		});
	FramesByCommandId = Frames.RegisterNonUniqueIndex<FSGSCommandId>(
		FName(TEXT("ByCommandId")),
		[](const FSGSResolutionFrame& Frame)
		{
			return Frame.SourceCommandId;
		});
	FramesByActorSeat = Frames.RegisterNonUniqueIndex<int32>(
		FName(TEXT("ByActorSeat")),
		[](const FSGSResolutionFrame& Frame)
		{
			return Frame.ActorSeat;
		});
}

void FSGSResolutionStack::Reset()
{
	StackOrder.Reset();
	NextStackSequence = 0;
	Frames.ResetItems();
}

void FSGSResolutionStack::Clear()
{
	Reset();
}

FSGSStableHandle FSGSResolutionStack::PushFrame(FSGSResolutionFrame Frame)
{
	Frame.ParentFrameHandle = GetCurrentFrameHandle();
	Frame.StackSequence = NextStackSequence++;

	FSGSStableHandle Handle = Frames.Add(MoveTemp(Frame));
	StackOrder.Add(Handle);
	return Handle;
}

FSGSStatus FSGSResolutionStack::PopFrame(FSGSStableHandle FrameHandle)
{
	if (!FrameHandle.IsValid() || StackOrder.Num() == 0 || StackOrder.Last() != FrameHandle)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.ResolutionStack.InvalidPop")),
			FString::Printf(TEXT("ResolutionStack can only pop the current frame, received %s."), *FrameHandle.ToLogString())));
	}

	StackOrder.Pop(EAllowShrinking::No);
	return Frames.Remove(FrameHandle);
}

TSGSResult<FSGSResolutionFrame> FSGSResolutionStack::CompleteCurrentFrame(FName Reason)
{
	const FSGSStableHandle CurrentHandle = GetCurrentFrameHandle();
	if (!CurrentHandle.IsValid())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.ResolutionStack.NoCurrentFrame")),
			FString::Printf(TEXT("Cannot complete a resolution frame for reason %s because the stack is empty."), *Reason.ToString())));
	}

	const FSGSResolutionFrame* CurrentFrame = FindFrame(CurrentHandle);
	if (CurrentFrame == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.ResolutionStack.StaleCurrentFrame")),
			FString::Printf(TEXT("Cannot complete stale current frame %s."), *CurrentHandle.ToLogString())));
	}

	FSGSResolutionFrame CompletedFrame = *CurrentFrame;
	if (FSGSStatus Status = PopFrame(CurrentHandle); Status.HasError())
	{
		return MakeError(Status.GetError());
	}

	return MakeValue(MoveTemp(CompletedFrame));
}

FSGSStatus FSGSResolutionStack::AbortAllFrames(FName Reason)
{
	(void)Reason;
	Reset();
	return MakeValue();
}

FSGSStableHandle FSGSResolutionStack::GetCurrentFrameHandle() const
{
	return StackOrder.Num() > 0 ? StackOrder.Last() : FSGSStableHandle();
}

FSGSResolutionFrame* FSGSResolutionStack::GetCurrentFrame()
{
	return FindFrame(GetCurrentFrameHandle());
}

const FSGSResolutionFrame* FSGSResolutionStack::GetCurrentFrame() const
{
	return FindFrame(GetCurrentFrameHandle());
}

FSGSResolutionFrame* FSGSResolutionStack::FindFrame(FSGSStableHandle FrameHandle)
{
	return FrameHandle.IsValid() ? Frames.Find(FrameHandle) : nullptr;
}

const FSGSResolutionFrame* FSGSResolutionStack::FindFrame(FSGSStableHandle FrameHandle) const
{
	return FrameHandle.IsValid() ? Frames.Find(FrameHandle) : nullptr;
}

FSGSStableHandle FSGSResolutionStack::GetParentFrameHandle(FSGSStableHandle FrameHandle) const
{
	const FSGSResolutionFrame* Frame = FindFrame(FrameHandle);
	return Frame != nullptr ? Frame->ParentFrameHandle : FSGSStableHandle();
}

FSGSResolutionFrame* FSGSResolutionStack::GetParentFrame(FSGSStableHandle FrameHandle)
{
	return FindFrame(GetParentFrameHandle(FrameHandle));
}

const FSGSResolutionFrame* FSGSResolutionStack::GetParentFrame(FSGSStableHandle FrameHandle) const
{
	return FindFrame(GetParentFrameHandle(FrameHandle));
}

FSGSStatus FSGSResolutionStack::OpenResponseWindowOnCurrent(FName WindowName, FName RequiredCardName, int32 EffectSourceSeat, int32 EffectTargetSeat)
{
	const FSGSStableHandle CurrentHandle = GetCurrentFrameHandle();
	if (!CurrentHandle.IsValid())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.ResolutionStack.NoCurrentFrame")),
			TEXT("Cannot open a response window without a resolution frame.")));
	}

	return Frames.Modify(CurrentHandle, [WindowName, RequiredCardName, EffectSourceSeat, EffectTargetSeat](FSGSResolutionFrame& Frame)
	{
		Frame.WindowName = WindowName;
		Frame.RequiredCardName = RequiredCardName;
		Frame.SourceSeat = EffectSourceSeat;
		Frame.TargetSeat = EffectTargetSeat;
	});
}

FSGSStatus FSGSResolutionStack::ClearResponseWindowOnCurrent()
{
	const FSGSStableHandle CurrentHandle = GetCurrentFrameHandle();
	if (!CurrentHandle.IsValid())
	{
		return MakeValue();
	}

	return Frames.Modify(CurrentHandle, [](FSGSResolutionFrame& Frame)
	{
		Frame.WindowName = NAME_None;
		Frame.RequiredCardName = NAME_None;
	});
}

FSGSResolutionFrame* FSGSResolutionStack::FindLatestDyingFrameForSeat(int32 DyingSeat)
{
	for (int32 Index = StackOrder.Num() - 1; Index >= 0; --Index)
	{
		FSGSResolutionFrame* Frame = Frames.Find(StackOrder[Index]);
		FSGSDyingPeachResolutionState* DyingState = Frame != nullptr ? Frame->GetMutableState<FSGSDyingPeachResolutionState>() : nullptr;
		if (DyingState != nullptr && DyingState->DyingSeat == DyingSeat)
		{
			return Frame;
		}
	}

	return nullptr;
}

const FSGSResolutionFrame* FSGSResolutionStack::FindLatestDyingFrameForSeat(int32 DyingSeat) const
{
	for (int32 Index = StackOrder.Num() - 1; Index >= 0; --Index)
	{
		const FSGSResolutionFrame* Frame = Frames.Find(StackOrder[Index]);
		const FSGSDyingPeachResolutionState* DyingState = Frame != nullptr ? Frame->GetState<FSGSDyingPeachResolutionState>() : nullptr;
		if (DyingState != nullptr && DyingState->DyingSeat == DyingSeat)
		{
			return Frame;
		}
	}

	return nullptr;
}

int32 FSGSResolutionStack::FindLatestProcessingCardId() const
{
	for (int32 Index = StackOrder.Num() - 1; Index >= 0; --Index)
	{
		const FSGSResolutionFrame* Frame = Frames.Find(StackOrder[Index]);
		if (Frame != nullptr && Frame->ProcessingCardId != INDEX_NONE)
		{
			return Frame->ProcessingCardId;
		}
	}

	return INDEX_NONE;
}

int32 FSGSResolutionStack::FindLatestEffectSourceSeat() const
{
	for (int32 Index = StackOrder.Num() - 1; Index >= 0; --Index)
	{
		const FSGSResolutionFrame* Frame = Frames.Find(StackOrder[Index]);
		if (Frame != nullptr && Frame->SourceSeat != INDEX_NONE)
		{
			return Frame->SourceSeat;
		}
	}

	return INDEX_NONE;
}

int32 FSGSResolutionStack::FindLatestEffectTargetSeat() const
{
	for (int32 Index = StackOrder.Num() - 1; Index >= 0; --Index)
	{
		const FSGSResolutionFrame* Frame = Frames.Find(StackOrder[Index]);
		if (Frame != nullptr && Frame->TargetSeat != INDEX_NONE)
		{
			return Frame->TargetSeat;
		}
	}

	return INDEX_NONE;
}

bool FSGSResolutionStack::CheckInvariants() const
{
	bool bOk = true;
	bOk &= Frames.CheckInvariants();
	int32 PreviousStackSequence = INDEX_NONE;
	for (int32 Index = 0; Index < StackOrder.Num(); ++Index)
	{
		const FSGSStableHandle Handle = StackOrder[Index];
		const FSGSResolutionFrame* Frame = Frames.Find(Handle);
		bOk &= ensureMsgf(Frame != nullptr, TEXT("ResolutionStack contains a stale frame handle."));
		if (Frame != nullptr)
		{
			bOk &= Frame->CheckInvariants();
			bOk &= ensureMsgf(
				PreviousStackSequence == INDEX_NONE || PreviousStackSequence < Frame->StackSequence,
				TEXT("ResolutionStack frame sequence must be monotonic in stack order."));
			PreviousStackSequence = Frame->StackSequence;
		}
	}
	return bOk;
}
