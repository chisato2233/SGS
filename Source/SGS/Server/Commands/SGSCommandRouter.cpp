#include "Server/Commands/SGSCommandRouter.h"

#include "Server/Engine/SGSGameContext.h"
#include "Shared/Core/SGSLogChannels.h"

FSGSCommandRouter::FSGSCommandRouter() {
  SGSCommandTypes::RegisterDefaultCommandTypes(*this);
}

void FSGSCommandRouter::Reset() {
  LogEntries.Reset();
  NextLogSequence = 0;
  TypeSpecs.Reset();
  SGSCommandTypes::RegisterDefaultCommandTypes(*this);
}

void FSGSCommandRouter::RegisterCommandType(
    TSharedRef<ISGSCommandType> CommandType) {
  if (!CommandType->CheckInvariants()) {
    return;
  }

  const FGameplayTag Type = CommandType->GetType();
  for (int32 Index = TypeSpecs.Num() - 1; Index >= 0; --Index) {
    if (TypeSpecs[Index]->GetType().MatchesTagExact(Type)) {
      ensureMsgf(false, TEXT("Replacing SGS command type for type %s."),
                 *Type.ToString());
      TypeSpecs.RemoveAt(Index);
    }
  }

  TypeSpecs.Add(CommandType);
}

FSGSStatus
FSGSCommandRouter::SubmitCommand(const FSGSCommand &Command,
                                 const FSGSCommandExecutionContext &Context) {
  RecordLifecycle(Command, SGSCommandLifecycle::Created(), true, FSGSError(),
                  TEXT("Command observed by server router."));
  RecordLifecycle(Command, SGSCommandLifecycle::Received(), true);

  if (FSGSStatus Status = ValidateCommon(Command, Context); Status.HasError()) {
    RecordLifecycle(Command, SGSCommandLifecycle::Rejected(), false,
                    Status.GetError());
    return Status;
  }

  const ISGSCommandType *TypeSpec = FindTypeSpec(Command.Type);
  check(TypeSpec != nullptr);

  if (FSGSStatus Status = TypeSpec->Validate(Command, Context);
      Status.HasError()) {
    RecordLifecycle(Command, SGSCommandLifecycle::Rejected(), false,
                    Status.GetError());
    return Status;
  }

  RecordLifecycle(Command, SGSCommandLifecycle::Validated(), true);
  UE_LOG(LogSGSTurn, Verbose,
         TEXT("Validated SGS command: %s PayloadSummary=%s"),
         *Command.ToLogString(), *TypeSpec->FormatPayloadSummary(Command));
  return MakeValue();
}

TSGSResult<FSGSRuleInvocation> FSGSCommandRouter::BuildRuleInvocation(
    const FSGSCommand &Command,
    const FSGSCommandExecutionContext &Context) const {
  const ISGSCommandType *TypeSpec =
      Command.Type.IsValid() ? FindTypeSpec(Command.Type) : nullptr;
  if (TypeSpec == nullptr) {
    return MakeError(FSGSError::Make(
        FName(TEXT("SGS.Command.UnsupportedType")),
        FString::Printf(TEXT("No SGS command type registered for type %s."),
                        *Command.Type.ToString())));
  }

  return TypeSpec->BuildRuleInvocation(Command, Context);
}

void FSGSCommandRouter::RecordLifecycle(const FSGSCommand &Command,
                                        FName Lifecycle, bool bSucceeded,
                                        FSGSError Error, FString Detail) {
  FSGSCommandLogEntry Entry;
  Entry.Sequence = NextLogSequence++;
  Entry.Lifecycle = Lifecycle;
  Entry.Command = Command;
  Entry.bSucceeded = bSucceeded;
  Entry.Error = MoveTemp(Error);
  Entry.Detail = MoveTemp(Detail);
  if (Entry.Detail.IsEmpty()) {
    Entry.Detail = FormatPayloadForLog(Command);
  }

  const FString ErrorSuffix = Entry.Error.IsValid()
                                  ? FString::Printf(
                                        TEXT(" Error=%s"),
                                        *Entry.Error.ToLogString())
                                  : FString();
  const FString LifecycleMessage = FString::Printf(
      TEXT("%s %s Detail=%s%s"), *Entry.Lifecycle.ToString(),
      *Entry.Command.ToLogString(), *Entry.Detail, *ErrorSuffix);
  if (!Entry.bSucceeded) {
    UE_LOG(LogSGSTurn, Warning, TEXT("%s"), *LifecycleMessage);
  } else if (Entry.Lifecycle == SGSCommandLifecycle::Executed()) {
    UE_LOG(LogSGSTurn, Log, TEXT("%s"), *LifecycleMessage);
  } else {
    UE_LOG(LogSGSTurn, Verbose, TEXT("%s"), *LifecycleMessage);
  }
  LogEntries.Add(MoveTemp(Entry));
}

bool FSGSCommandRouter::CheckInvariants() const {
  bool bOk = true;
  for (int32 Index = 0; Index < LogEntries.Num(); ++Index) {
    const FSGSCommandLogEntry &Entry = LogEntries[Index];
    bOk &= ensureMsgf(Entry.Sequence == Index,
                      TEXT("Command log sequence mismatch."));
    bOk &= ensureMsgf(!Entry.Lifecycle.IsNone(),
                      TEXT("Command log lifecycle is empty."));
    bOk &= Entry.Command.CheckInvariants();
    if (Entry.Error.IsValid()) {
      bOk &= Entry.Error.CheckInvariants();
    }
  }

  for (const TSharedRef<ISGSCommandType> &TypeSpec : TypeSpecs) {
    bOk &= TypeSpec->CheckInvariants();
  }

  return bOk;
}

FSGSStatus FSGSCommandRouter::ValidateCommon(
    const FSGSCommand &Command,
    const FSGSCommandExecutionContext &Context) const {
  if (!Command.CheckInvariants()) {
    return MakeError(
        FSGSError::Make(FName(TEXT("SGS.Command.InvariantViolation")),
                        FString::Printf(TEXT("Invalid command shape: %s"),
                                        *Command.ToLogString())));
  }

  if (Context.GameContext == nullptr) {
    return MakeError(
        FSGSError::Make(FName(TEXT("SGS.Command.MissingContext")),
                        TEXT("Command router received a null game context.")));
  }

  if (!Command.CommandId.IsValid() ||
      Command.CommandId != Context.ExpectedCommandId) {
    return MakeError(FSGSError::Make(
        FName(TEXT("SGS.Command.IdMismatch")),
        FString::Printf(
            TEXT("Command id %s does not match expected command id %s."),
            *Command.CommandId.ToLogString(),
            *Context.ExpectedCommandId.ToLogString())));
  }

  if (Command.RequestId != Context.ExpectedRequestId) {
    return MakeError(FSGSError::Make(
        FName(TEXT("SGS.Command.RequestMismatch")),
        FString::Printf(
            TEXT("Command request %d does not match expected request %d."),
            Command.RequestId, Context.ExpectedRequestId)));
  }

  if (Command.SeatIndex != Context.ExpectedSeatIndex ||
      Context.GameContext->GetSeat(Command.SeatIndex) == nullptr) {
    return MakeError(FSGSError::Make(
        FName(TEXT("SGS.Command.InvalidSeat")),
        FString::Printf(
            TEXT("Command seat %d does not match expected seat %d."),
            Command.SeatIndex, Context.ExpectedSeatIndex)));
  }

  if (!Command.Phase.MatchesTagExact(Context.ExpectedPhase)) {
    return MakeError(FSGSError::Make(
        FName(TEXT("SGS.Command.PhaseMismatch")),
        FString::Printf(
            TEXT("Command phase %s does not match expected phase %s."),
            *Command.Phase.ToString(), *Context.ExpectedPhase.ToString())));
  }

  const ISGSCommandType *TypeSpec =
      Command.Type.IsValid() ? FindTypeSpec(Command.Type) : nullptr;
  if (TypeSpec == nullptr) {
    return MakeError(FSGSError::Make(
        FName(TEXT("SGS.Command.UnsupportedType")),
        FString::Printf(TEXT("No SGS command type registered for type %s."),
                        *Command.Type.ToString())));
  }

  const UScriptStruct *ExpectedPayloadStruct = TypeSpec->GetPayloadStruct();
  if (ExpectedPayloadStruct == nullptr) {
    return MakeError(FSGSError::Make(
        FName(TEXT("SGS.Command.InvalidHandler")),
        FString::Printf(
            TEXT("Command type %s does not declare a payload struct."),
            *Command.Type.ToString())));
  }

  const UScriptStruct *ActualPayloadStruct = Command.GetPayloadStruct();
  if (ActualPayloadStruct == nullptr) {
    return MakeError(FSGSError::Make(
        FName(TEXT("SGS.Command.MissingPayload")),
        FString::Printf(TEXT("Command type %s requires payload %s."),
                        *Command.Type.ToString(),
                        *ExpectedPayloadStruct->GetName())));
  }

  if (ActualPayloadStruct != ExpectedPayloadStruct) {
    return MakeError(FSGSError::Make(
        FName(TEXT("SGS.Command.PayloadTypeMismatch")),
        FString::Printf(
            TEXT("Command type %s requires payload %s but received %s."),
            *Command.Type.ToString(), *ExpectedPayloadStruct->GetName(),
            *ActualPayloadStruct->GetName())));
  }

  return MakeValue();
}

const ISGSCommandType *
FSGSCommandRouter::FindTypeSpec(FGameplayTag Type) const {
  for (const TSharedRef<ISGSCommandType> &TypeSpec : TypeSpecs) {
    if (TypeSpec->GetType().MatchesTagExact(Type)) {
      return &TypeSpec.Get();
    }
  }

  return nullptr;
}

FString
FSGSCommandRouter::FormatPayloadForLog(const FSGSCommand &Command) const {
  const ISGSCommandType *TypeSpec =
      Command.Type.IsValid() ? FindTypeSpec(Command.Type) : nullptr;
  if (TypeSpec != nullptr) {
    return TypeSpec->FormatPayloadSummary(Command);
  }

  return Command.HasPayload() ? FString::Printf(TEXT("UnregisteredPayload=%s"),
                                                *Command.GetPayloadTypeName())
                              : TEXT("NoPayload");
}
