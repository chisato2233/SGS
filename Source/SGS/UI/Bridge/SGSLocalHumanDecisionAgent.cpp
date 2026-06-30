#include "UI/Bridge/SGSLocalHumanDecisionAgent.h"

#include "Core/SGSLogChannels.h"

void USGSLocalHumanDecisionAgent::RequestPlayPhaseAction(
	const FSGSPlayPhaseRequest& Request,
	FSGSPlayPhaseDecisionDelegate OnDecided)
{
	if (IsWaitingForLocalDecision())
	{
		UE_LOG(LogSGSUI, Warning, TEXT("Replacing an unresolved local decision prompt with a play-phase prompt."));
		SubmitPassInternal(TEXT("ReplacedPrompt"));
	}

	PendingPlayRequest = Request;
	PendingPlayDecisionDelegate = MoveTemp(OnDecided);
	PromptType = ESGSLocalDecisionPromptType::Play;
	PromptChangedDelegate.Broadcast();
}

void USGSLocalHumanDecisionAgent::RequestResponseAction(
	const FSGSResponseRequest& Request,
	FSGSResponseDecisionDelegate OnDecided)
{
	if (IsWaitingForLocalDecision())
	{
		UE_LOG(LogSGSUI, Warning, TEXT("Replacing an unresolved local decision prompt with a response prompt."));
		SubmitPassInternal(TEXT("ReplacedPrompt"));
	}

	PendingResponseRequest = Request;
	PendingResponseDecisionDelegate = MoveTemp(OnDecided);
	PromptType = ESGSLocalDecisionPromptType::Response;
	PromptChangedDelegate.Broadcast();
}

const FSGSPlayPhaseRequest* USGSLocalHumanDecisionAgent::GetPendingPlayRequest() const
{
	return HasPendingPlayRequest() ? &PendingPlayRequest : nullptr;
}

const FSGSResponseRequest* USGSLocalHumanDecisionAgent::GetPendingResponseRequest() const
{
	return HasPendingResponseRequest() ? &PendingResponseRequest : nullptr;
}

bool USGSLocalHumanDecisionAgent::SubmitUseCard(int32 CardId, int32 TargetSeatIndex)
{
	if (!HasPendingPlayRequest())
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitUseCard ignored: no local play prompt is pending."));
		return false;
	}

	const FSGSCardActionOption* Option = PendingPlayRequest.Options.FindByPredicate(
		[CardId](const FSGSCardActionOption& Candidate)
		{
			return Candidate.CardId == CardId;
		});

	if (Option == nullptr)
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitUseCard rejected: card %d is not a legal play option."), CardId);
		return false;
	}

	const FName CardName = Option->CardName;
	TArray<int32> Targets;
	if (Option->TargetSeatIndices.Num() > 0)
	{
		int32 FinalTarget = TargetSeatIndex;
		if (FinalTarget == INDEX_NONE && Option->TargetSeatIndices.Num() == 1)
		{
			FinalTarget = Option->TargetSeatIndices[0];
		}

		if (!Option->TargetSeatIndices.Contains(FinalTarget))
		{
			UE_LOG(LogSGSUI, Warning, TEXT("SubmitUseCard rejected: target %d is not legal for card %d."), FinalTarget, CardId);
			return false;
		}

		Targets.Add(FinalTarget);
	}

	const FSGSPlayPhaseRequest Request = PendingPlayRequest;
	FSGSPlayPhaseDecisionDelegate Delegate = MoveTemp(PendingPlayDecisionDelegate);
	ClearPrompt(true);

	FSGSPlayPhaseDecision Decision;
	Decision.Command = FSGSCommand::MakeUseCard(
		Request.CommandId,
		Request.RequestId,
		Request.SeatIndex,
		Request.Phase,
		CardId,
		MoveTemp(Targets),
		TEXT("LocalUI"),
		CardName);

	UE_LOG(LogSGSUI, Log, TEXT("LocalUI submitted UseCard command: CardId=%d Target=%d."), CardId, TargetSeatIndex);
	Delegate.ExecuteIfBound(Decision);
	return true;
}

bool USGSLocalHumanDecisionAgent::SubmitResponseCard(int32 CardId, int32 TargetSeatIndex)
{
	if (!HasPendingResponseRequest())
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard ignored: no local response prompt is pending."));
		return false;
	}

	if (!PendingResponseRequest.ResponseCardIds.Contains(CardId))
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard rejected: card %d is not legal for the pending response."), CardId);
		return false;
	}

	TArray<int32> Targets;
	if (PendingResponseRequest.EffectTargetSeat != INDEX_NONE)
	{
		const int32 FinalTarget = TargetSeatIndex != INDEX_NONE
			? TargetSeatIndex
			: PendingResponseRequest.EffectTargetSeat;
		Targets.Add(FinalTarget);
	}

	const FSGSResponseRequest Request = PendingResponseRequest;
	FSGSResponseDecisionDelegate Delegate = MoveTemp(PendingResponseDecisionDelegate);
	ClearPrompt(true);

	FSGSResponseDecision Decision;
	Decision.Command = FSGSCommand::MakeRespondCard(
		Request.CommandId,
		Request.RequestId,
		Request.SeatIndex,
		Request.Phase,
		CardId,
		MoveTemp(Targets),
		Request.WindowName,
		TEXT("LocalUI"),
		Request.RequiredCardName);

	UE_LOG(LogSGSUI, Log, TEXT("LocalUI submitted RespondCard command: CardId=%d Window=%s."),
		CardId,
		*Request.WindowName.ToString());
	Delegate.ExecuteIfBound(Decision);
	return true;
}

bool USGSLocalHumanDecisionAgent::SubmitPass()
{
	return SubmitPassInternal(TEXT("Pass"));
}

void USGSLocalHumanDecisionAgent::ClearPrompt(bool bNotify)
{
	PromptType = ESGSLocalDecisionPromptType::None;
	PendingPlayRequest = FSGSPlayPhaseRequest();
	PendingResponseRequest = FSGSResponseRequest();
	PendingPlayDecisionDelegate.Unbind();
	PendingResponseDecisionDelegate.Unbind();

	if (bNotify)
	{
		PromptChangedDelegate.Broadcast();
	}
}

bool USGSLocalHumanDecisionAgent::SubmitPassInternal(FName SourceName)
{
	if (HasPendingPlayRequest())
	{
		const FSGSPlayPhaseRequest Request = PendingPlayRequest;
		FSGSPlayPhaseDecisionDelegate Delegate = MoveTemp(PendingPlayDecisionDelegate);
		ClearPrompt(true);

		FSGSPlayPhaseDecision Decision;
		Decision.Command = FSGSCommand::MakePass(
			Request.CommandId,
			Request.RequestId,
			Request.SeatIndex,
			Request.Phase,
			TEXT("LocalUI"),
			SourceName);
		UE_LOG(LogSGSUI, Log, TEXT("LocalUI submitted play pass."));
		Delegate.ExecuteIfBound(Decision);
		return true;
	}

	if (HasPendingResponseRequest())
	{
		const FSGSResponseRequest Request = PendingResponseRequest;
		FSGSResponseDecisionDelegate Delegate = MoveTemp(PendingResponseDecisionDelegate);
		ClearPrompt(true);

		FSGSResponseDecision Decision;
		Decision.Command = FSGSCommand::MakePass(
			Request.CommandId,
			Request.RequestId,
			Request.SeatIndex,
			Request.Phase,
			TEXT("LocalUI"),
			SourceName);
		UE_LOG(LogSGSUI, Log, TEXT("LocalUI submitted response pass for window %s."), *Request.WindowName.ToString());
		Delegate.ExecuteIfBound(Decision);
		return true;
	}

	UE_LOG(LogSGSUI, Warning, TEXT("SubmitPass ignored: no local prompt is pending."));
	return false;
}
