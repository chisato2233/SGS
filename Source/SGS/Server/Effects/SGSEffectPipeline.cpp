#include "Server/Effects/SGSEffectPipeline.h"

FSGSEffectResult FSGSEffectResult::Success()
{
	return FSGSEffectResult();
}

FSGSEffectResult FSGSEffectResult::Suspend()
{
	FSGSEffectResult Result;
	Result.bSuspend = true;
	return Result;
}

FSGSEffectResult FSGSEffectResult::Failure(FSGSError InError)
{
	FSGSEffectResult Result;
	Result.Error = MoveTemp(InError);
	return Result;
}

void FSGSEffectQueue::Reset()
{
	Steps.Reset();
}

void FSGSEffectQueue::Enqueue(FSGSEffectStep Step)
{
	Steps.Add(MoveTemp(Step));
}

void FSGSEffectQueue::EnqueueFront(FSGSEffectStep Step)
{
	Steps.Insert(MoveTemp(Step), 0);
}

void FSGSEffectQueue::EnqueueManyFront(TArray<FSGSEffectStep> InSteps)
{
	for (int32 Index = InSteps.Num() - 1; Index >= 0; --Index)
	{
		Steps.Insert(MoveTemp(InSteps[Index]), 0);
	}
}

FSGSEffectStep FSGSEffectQueue::PopFront()
{
	check(Steps.Num() > 0);
	FSGSEffectStep Step = MoveTemp(Steps[0]);
	Steps.RemoveAt(0);
	return Step;
}

bool FSGSEffectQueue::CheckInvariants() const
{
	bool bOk = true;
	for (const FSGSEffectStep& Step : Steps)
	{
		bOk &= Step.CheckInvariants();
	}
	return bOk;
}

void FSGSEffectPipeline::Reset()
{
	Queue.Reset();
	NextStepIdValue = 0;
	bSuspended = false;
}

FSGSEffectStepId FSGSEffectPipeline::AllocateStepId()
{
	return FSGSEffectStepId(++NextStepIdValue);
}

void FSGSEffectPipeline::Enqueue(FSGSEffectStep Step)
{
	if (!Step.StepId.IsValid())
	{
		Step.StepId = AllocateStepId();
	}
	Queue.Enqueue(MoveTemp(Step));
}

void FSGSEffectPipeline::EnqueueFront(FSGSEffectStep Step)
{
	if (!Step.StepId.IsValid())
	{
		Step.StepId = AllocateStepId();
	}
	Queue.EnqueueFront(MoveTemp(Step));
}

FSGSStatus FSGSEffectPipeline::Run(FSGSEffectContext& Context)
{
	bSuspended = false;
	return RunInternal(Context);
}

FSGSStatus FSGSEffectPipeline::Resume(FSGSEffectContext& Context)
{
	bSuspended = false;
	return RunInternal(Context);
}

bool FSGSEffectPipeline::CheckInvariants() const
{
	return Queue.CheckInvariants()
		&& ensureMsgf(NextStepIdValue >= 0, TEXT("EffectPipeline next step id is invalid."));
}

FSGSStatus FSGSEffectPipeline::RunInternal(FSGSEffectContext& Context)
{
	if (!Context.CheckInvariants())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.EffectPipeline.InvalidContext")),
			TEXT("EffectPipeline received an invalid context.")));
	}

	while (!Queue.IsEmpty())
	{
		FSGSEffectStep Step = Queue.PopFront();
		if (!Step.CheckInvariants())
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.EffectPipeline.InvalidStep")),
				FString::Printf(TEXT("Step=%s"), *Step.StepName.ToString())));
		}

		Context.CurrentStepId = Step.StepId;
		AppendStepEvent(Context, FName(TEXT("SGS.Event.EffectStepStarted")), Step, TEXT("Started"));

		FSGSEffectResult Result = Step.Execute(Context);
		if (Result.HasError())
		{
			AppendStepEvent(Context, FName(TEXT("SGS.Event.EffectStepFailed")), Step, Result.Error.ToLogString());
			return MakeError(Result.Error);
		}

		if (Result.FollowUpSteps.Num() > 0)
		{
			for (FSGSEffectStep& FollowUp : Result.FollowUpSteps)
			{
				if (!FollowUp.StepId.IsValid())
				{
					FollowUp.StepId = AllocateStepId();
				}
			}
			Queue.EnqueueManyFront(MoveTemp(Result.FollowUpSteps));
			AppendStepEvent(Context, FName(TEXT("SGS.Event.EffectStepInsertedFollowUps")), Step, TEXT("Follow-up steps inserted."));
		}

		if (Result.bSuspend)
		{
			bSuspended = true;
			AppendStepEvent(Context, FName(TEXT("SGS.Event.EffectStepSuspended")), Step, TEXT("Suspended"));
			return MakeValue();
		}

		AppendStepEvent(Context, FName(TEXT("SGS.Event.EffectStepFinished")), Step, TEXT("Finished"));
	}

	return MakeValue();
}

void FSGSEffectPipeline::AppendStepEvent(FSGSEffectContext& Context, FName EventName, const FSGSEffectStep& Step, FString Payload)
{
	if (Context.ReplayLog != nullptr)
	{
		Context.ReplayLog->AppendEvent(
			EventName,
			Context.CommandId,
			Step.StepId,
			Context.TimingPoint,
			FString::Printf(TEXT("%s | %s"), *Step.StepName.ToString(), *Payload));
	}
}
