#include "Shared/Core/SGSRandomAudit.h"

void FSGSRandomAudit::Initialize(int32 InSeed)
{
	Seed = InSeed;
	NextStep = 0;
	bReplayMode = false;
	Entries.Reset();
	Stream.Initialize(Seed);
}

void FSGSRandomAudit::InitializeReplay(int32 InSeed, TArray<FSGSRandomLogEntry> InReplayEntries)
{
	Seed = InSeed;
	NextStep = 0;
	bReplayMode = true;
	Entries = MoveTemp(InReplayEntries);
	Stream.Initialize(Seed);
}

TSGSResult<int32> FSGSRandomAudit::TryRandRange(
	FName Purpose,
	int32 MinInclusive,
	int32 MaxInclusive,
	FString Context,
	FSGSCommandId CommandId,
	FName EventName)
{
	if (Purpose.IsNone())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.RandomAudit.MissingPurpose")),
			MoveTemp(Context)));
	}

	if (MinInclusive > MaxInclusive)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.RandomAudit.InvalidRange")),
			FString::Printf(TEXT("Purpose=%s Range=[%d,%d] Context=%s"), *Purpose.ToString(), MinInclusive, MaxInclusive, *Context)));
	}

	if (bReplayMode)
	{
		if (!Entries.IsValidIndex(NextStep))
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.RandomAudit.ReplayExhausted")),
				FString::Printf(TEXT("Purpose=%s Step=%d Context=%s"), *Purpose.ToString(), NextStep, *Context)));
		}

		const FSGSRandomLogEntry& Entry = Entries[NextStep];
		if (Entry.Seed != Seed
			|| Entry.Step != NextStep
			|| Entry.Purpose != Purpose
			|| Entry.MinInclusive != MinInclusive
			|| Entry.MaxInclusive != MaxInclusive)
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.RandomAudit.ReplayMismatch")),
				FString::Printf(
					TEXT("Expected Step=%d Seed=%d Purpose=%s Range=[%d,%d]; got Step=%d Seed=%d Purpose=%s Range=[%d,%d]. Context=%s"),
					NextStep,
					Seed,
					*Purpose.ToString(),
					MinInclusive,
					MaxInclusive,
					Entry.Step,
					Entry.Seed,
					*Entry.Purpose.ToString(),
					Entry.MinInclusive,
					Entry.MaxInclusive,
					*Context)));
		}

		++NextStep;
		return MakeValue(Entry.Output);
	}

	const int32 Output = Stream.RandRange(MinInclusive, MaxInclusive);

	FSGSRandomLogEntry Entry;
	Entry.Seed = Seed;
	Entry.Step = NextStep++;
	Entry.Purpose = Purpose;
	Entry.MinInclusive = MinInclusive;
	Entry.MaxInclusive = MaxInclusive;
	Entry.Output = Output;
	Entry.CommandId = CommandId;
	Entry.EventName = EventName;
	Entry.Context = MoveTemp(Context);
	Entries.Add(MoveTemp(Entry));

	return MakeValue(Output);
}

int32 FSGSRandomAudit::RandRange(
	FName Purpose,
	int32 MinInclusive,
	int32 MaxInclusive,
	FString Context,
	FSGSCommandId CommandId,
	FName EventName)
{
	TSGSResult<int32> Result = TryRandRange(Purpose, MinInclusive, MaxInclusive, MoveTemp(Context), CommandId, EventName);
	if (Result.HasValue())
	{
		return Result.GetValue();
	}

	ensureMsgf(false, TEXT("RandomAudit failed: %s"), *Result.GetError().ToLogString());
	return MinInclusive;
}

TSGSResult<int32> FSGSRandomAudit::TryRandIndex(
	FName Purpose,
	int32 NumOptions,
	FString Context,
	FSGSCommandId CommandId,
	FName EventName)
{
	if (NumOptions <= 0)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.RandomAudit.EmptyOptions")),
			FString::Printf(TEXT("Purpose=%s NumOptions=%d Context=%s"), *Purpose.ToString(), NumOptions, *Context)));
	}

	return TryRandRange(Purpose, 0, NumOptions - 1, MoveTemp(Context), CommandId, EventName);
}

int32 FSGSRandomAudit::RandIndex(
	FName Purpose,
	int32 NumOptions,
	FString Context,
	FSGSCommandId CommandId,
	FName EventName)
{
	TSGSResult<int32> Result = TryRandIndex(Purpose, NumOptions, MoveTemp(Context), CommandId, EventName);
	if (Result.HasValue())
	{
		return Result.GetValue();
	}

	ensureMsgf(false, TEXT("RandomAudit failed: %s"), *Result.GetError().ToLogString());
	return 0;
}

bool FSGSRandomAudit::CheckInvariants() const
{
	bool bOk = true;
	if (bReplayMode)
	{
		bOk &= ensureMsgf(NextStep <= Entries.Num(), TEXT("Random audit replay step is beyond log length."));
	}
	else
	{
		bOk &= ensureMsgf(NextStep == Entries.Num(), TEXT("Random audit step count mismatch."));
	}

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FSGSRandomLogEntry& Entry = Entries[Index];
		bOk &= ensureMsgf(Entry.Seed == Seed, TEXT("Random audit step %d seed mismatch."), Entry.Step);
		bOk &= ensureMsgf(Entry.Step == Index, TEXT("Random audit step %d is stored at index %d."), Entry.Step, Index);
		bOk &= ensureMsgf(!Entry.Purpose.IsNone(), TEXT("Random audit step %d has no purpose."), Entry.Step);
		bOk &= ensureMsgf(Entry.MinInclusive <= Entry.MaxInclusive, TEXT("Random audit step %d has invalid range."), Entry.Step);
		bOk &= ensureMsgf(Entry.Output >= Entry.MinInclusive && Entry.Output <= Entry.MaxInclusive,
			TEXT("Random audit step %d output is outside its range."), Entry.Step);
		bOk &= Entry.CommandId.CheckInvariants();
	}

	return bOk;
}
