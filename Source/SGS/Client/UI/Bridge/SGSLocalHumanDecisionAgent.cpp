#include "Client/UI/Bridge/SGSLocalHumanDecisionAgent.h"

#include "Shared/Core/SGSLogChannels.h"
#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"

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
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("LocalUI")), CardName),
		FSGSUseCardCommandPayload(CardId, MoveTemp(Targets)));

	UE_LOG(LogSGSUI, Log, TEXT("LocalUI submitted UseCard command: CardId=%d Target=%d."), CardId, TargetSeatIndex);
	Delegate.ExecuteIfBound(Decision);
	return true;
}

bool USGSLocalHumanDecisionAgent::SubmitResponseCard(
	int32 CardId,
	int32 TargetSeatIndex,
	FName SkillName)
{
	if (!HasPendingResponseRequest())
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard ignored: no local response prompt is pending."));
		return false;
	}

	const FSGSDecisionSkillOption* SkillOption = SkillName.IsNone()
		? nullptr
		: PendingResponseRequest.SkillOptions.FindByPredicate(
			[SkillName](const FSGSDecisionSkillOption& Candidate)
			{
				return Candidate.SkillName == SkillName;
			});
	if (!SkillName.IsNone() && SkillOption == nullptr)
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard rejected: skill %s is not legal for the pending response."), *SkillName.ToString());
		return false;
	}
	if (SkillOption == nullptr && !PendingResponseRequest.ResponseCardIds.Contains(CardId))
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard rejected: card %d is not legal for the pending response."), CardId);
		return false;
	}
	if (SkillOption != nullptr
		&& SkillOption->bRequiresCard
		&& !SkillOption->SelectableCardIds.Contains(CardId))
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard rejected: card %d is not a legal cost for skill %s."), CardId, *SkillName.ToString());
		return false;
	}
	if (SkillOption != nullptr && !SkillOption->bRequiresCard)
	{
		CardId = INDEX_NONE;
	}

	TArray<int32> Targets;
	TArray<int32> LegalTargets;
	if (SkillOption != nullptr)
	{
		LegalTargets = SkillOption->TargetSeatIndices;
	}
	if (LegalTargets.IsEmpty() && PendingResponseRequest.EffectTargetSeat != INDEX_NONE)
	{
		LegalTargets.Add(PendingResponseRequest.EffectTargetSeat);
	}
	if (!LegalTargets.IsEmpty())
	{
		int32 FinalTarget = TargetSeatIndex;
		if (FinalTarget == INDEX_NONE && LegalTargets.Num() == 1)
		{
			FinalTarget = LegalTargets[0];
		}
		if (!LegalTargets.Contains(FinalTarget))
		{
			UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard rejected: target %d is not legal for the pending response."), FinalTarget);
			return false;
		}
		Targets.Add(FinalTarget);
	}

	const FSGSResponseRequest Request = PendingResponseRequest;
	FSGSResponseDecisionDelegate Delegate = MoveTemp(PendingResponseDecisionDelegate);
	ClearPrompt(true);

	FSGSResponseDecision Decision;
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(
			Request,
			FName(TEXT("LocalUI")),
			SkillName.IsNone() ? Request.RequiredCardName : SkillName),
		FSGSRespondCardCommandPayload(CardId, MoveTemp(Targets), Request.WindowName, SkillName));

	UE_LOG(LogSGSUI, Log, TEXT("LocalUI submitted response command: CardId=%d Skill=%s Window=%s."),
		CardId,
		*SkillName.ToString(),
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
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("LocalUI")), SourceName),
			FSGSPassCommandPayload());
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
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("LocalUI")), SourceName),
			FSGSPassCommandPayload());
		UE_LOG(LogSGSUI, Log, TEXT("LocalUI submitted response pass for window %s."), *Request.WindowName.ToString());
		Delegate.ExecuteIfBound(Decision);
		return true;
	}

	UE_LOG(LogSGSUI, Warning, TEXT("SubmitPass ignored: no local prompt is pending."));
	return false;
}
