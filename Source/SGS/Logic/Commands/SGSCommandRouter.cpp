#include "Logic/Commands/SGSCommandRouter.h"

#include "Logic/Engine/SGSGameContext.h"
#include "Core/SGSLogChannels.h"

namespace
{
class FSGSPassCommandHandler final : public ISGSCommandHandler
{
public:
	virtual FGameplayTag GetType() const override
	{
		return SGSGameplayTags::PlayAction_Pass.GetTag();
	}

	virtual FSGSStatus Validate(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const override
	{
		if (!Command.IsType(SGSGameplayTags::PlayAction_Pass))
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.Command.UnsupportedType")),
				FString::Printf(TEXT("Pass handler received unsupported command type %s."), *Command.Type.ToString())));
		}

		if (Command.CardIds.Num() > 0
			|| Command.TargetSeatIndices.Num() > 0
			|| Command.CardHandles.Num() > 0
			|| Command.TargetHandles.Num() > 0
			|| Command.Parameters.Num() > 0
			|| !Command.SkillName.IsNone())
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.Command.InvalidPayload")),
				TEXT("Pass command must not include cards, targets, skills, or parameters.")));
		}

		return MakeValue();
	}

	virtual FSGSStatus Execute(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const override
	{
		return MakeValue();
	}
};
}

FSGSCommandRouter::FSGSCommandRouter()
{
	RegisterHandler(MakeShared<FSGSPassCommandHandler>());
}

void FSGSCommandRouter::Reset()
{
	LogEntries.Reset();
	NextLogSequence = 0;
	Handlers.Reset();
	RegisterHandler(MakeShared<FSGSPassCommandHandler>());
}

void FSGSCommandRouter::RegisterHandler(TSharedRef<ISGSCommandHandler> Handler)
{
	const FGameplayTag Type = Handler.Get().GetType();
	if (!ensureMsgf(Type.IsValid(), TEXT("Cannot register SGS command handler with invalid type.")))
	{
		return;
	}

	for (int32 Index = Handlers.Num() - 1; Index >= 0; --Index)
	{
		if (Handlers[Index].Get().GetType().MatchesTagExact(Type))
		{
			ensureMsgf(false, TEXT("Replacing SGS command handler for type %s."), *Type.ToString());
			Handlers.RemoveAt(Index);
		}
	}

	Handlers.Add(Handler);
}

FSGSStatus FSGSCommandRouter::SubmitCommand(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context)
{
	RecordLifecycle(Command, SGSCommandLifecycle::Created(), true, FSGSError(), TEXT("Command observed by server router."));
	RecordLifecycle(Command, SGSCommandLifecycle::Received(), true);

	if (FSGSStatus Status = ValidateCommon(Command, Context); Status.HasError())
	{
		RecordLifecycle(Command, SGSCommandLifecycle::Rejected(), false, Status.GetError());
		return Status;
	}

	const ISGSCommandHandler* Handler = FindHandler(Command.Type);
	check(Handler != nullptr);

	if (FSGSStatus Status = Handler->Validate(Command, Context); Status.HasError())
	{
		RecordLifecycle(Command, SGSCommandLifecycle::Rejected(), false, Status.GetError());
		return Status;
	}

	RecordLifecycle(Command, SGSCommandLifecycle::Validated(), true);

	if (FSGSStatus Status = Handler->Execute(Command, Context); Status.HasError())
	{
		RecordLifecycle(Command, SGSCommandLifecycle::Rejected(), false, Status.GetError(), TEXT("Execution failed."));
		return Status;
	}

	RecordLifecycle(Command, SGSCommandLifecycle::Executed(), true);
	UE_LOG(LogSGSTurn, Verbose, TEXT("Executed SGS command: %s"), *Command.ToLogString());
	return MakeValue();
}

void FSGSCommandRouter::RecordLifecycle(
	const FSGSCommand& Command,
	FName Lifecycle,
	bool bSucceeded,
	FSGSError Error,
	FString Detail)
{
	FSGSCommandLogEntry Entry;
	Entry.Sequence = NextLogSequence++;
	Entry.Lifecycle = Lifecycle;
	Entry.Command = Command;
	Entry.bSucceeded = bSucceeded;
	Entry.Error = MoveTemp(Error);
	Entry.Detail = MoveTemp(Detail);
	LogEntries.Add(MoveTemp(Entry));
}

bool FSGSCommandRouter::CheckInvariants() const
{
	bool bOk = true;
	for (int32 Index = 0; Index < LogEntries.Num(); ++Index)
	{
		const FSGSCommandLogEntry& Entry = LogEntries[Index];
		bOk &= ensureMsgf(Entry.Sequence == Index, TEXT("Command log sequence mismatch."));
		bOk &= ensureMsgf(!Entry.Lifecycle.IsNone(), TEXT("Command log lifecycle is empty."));
		bOk &= Entry.Command.CheckInvariants();
		if (Entry.Error.IsValid())
		{
			bOk &= Entry.Error.CheckInvariants();
		}
	}

	for (const TSharedRef<ISGSCommandHandler>& Handler : Handlers)
	{
		bOk &= ensureMsgf(Handler.Get().GetType().IsValid(), TEXT("Command router has invalid handler type."));
	}

	return bOk;
}

FSGSStatus FSGSCommandRouter::ValidateCommon(const FSGSCommand& Command, const FSGSCommandExecutionContext& Context) const
{
	if (!Command.CheckInvariants())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvariantViolation")),
			FString::Printf(TEXT("Invalid command shape: %s"), *Command.ToLogString())));
	}

	if (Context.GameContext == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.MissingContext")),
			TEXT("Command router received a null game context.")));
	}

	if (!Command.CommandId.IsValid() || Command.CommandId != Context.ExpectedCommandId)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.IdMismatch")),
			FString::Printf(
				TEXT("Command id %s does not match expected command id %s."),
				*Command.CommandId.ToLogString(),
				*Context.ExpectedCommandId.ToLogString())));
	}

	if (Command.RequestId != Context.ExpectedRequestId)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.RequestMismatch")),
			FString::Printf(
				TEXT("Command request %d does not match expected request %d."),
				Command.RequestId,
				Context.ExpectedRequestId)));
	}

	if (Command.SeatIndex != Context.ExpectedSeatIndex || Context.GameContext->GetSeat(Command.SeatIndex) == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvalidSeat")),
			FString::Printf(
				TEXT("Command seat %d does not match expected seat %d."),
				Command.SeatIndex,
				Context.ExpectedSeatIndex)));
	}

	if (!Command.Phase.MatchesTagExact(Context.ExpectedPhase))
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.PhaseMismatch")),
			FString::Printf(
				TEXT("Command phase %s does not match expected phase %s."),
				*Command.Phase.ToString(),
				*Context.ExpectedPhase.ToString())));
	}

	if (!Command.Type.IsValid() || FindHandler(Command.Type) == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.UnsupportedType")),
			FString::Printf(TEXT("No SGS command handler registered for type %s."), *Command.Type.ToString())));
	}

	return MakeValue();
}

const ISGSCommandHandler* FSGSCommandRouter::FindHandler(FGameplayTag Type) const
{
	for (const TSharedRef<ISGSCommandHandler>& Handler : Handlers)
	{
		if (Handler.Get().GetType().MatchesTagExact(Type))
		{
			return &Handler.Get();
		}
	}

	return nullptr;
}
