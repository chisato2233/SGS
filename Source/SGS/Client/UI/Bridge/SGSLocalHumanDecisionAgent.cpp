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
	return SubmitUseCard(
		CardId,
		TargetSeatIndex == INDEX_NONE ? TArray<int32>() : TArray<int32>{ TargetSeatIndex });
}

bool USGSLocalHumanDecisionAgent::SubmitUseCard(int32 CardId, TArray<int32> TargetSeatIndices)
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
	if (TargetSeatIndices.Num() < Option->MinTargetCount
		|| TargetSeatIndices.Num() > Option->MaxTargetCount)
	{
		return false;
	}
	TSet<int32> UniqueTargets;
	for (const int32 TargetSeat : TargetSeatIndices)
	{
		if (UniqueTargets.Contains(TargetSeat) || !Option->TargetSeatIndices.Contains(TargetSeat))
		{
			UE_LOG(LogSGSUI, Warning, TEXT("SubmitUseCard rejected: target %d is not legal for card %d."), TargetSeat, CardId);
			return false;
		}
		UniqueTargets.Add(TargetSeat);
	}

	const FSGSPlayPhaseRequest Request = PendingPlayRequest;
	FSGSPlayPhaseDecisionDelegate Delegate = MoveTemp(PendingPlayDecisionDelegate);
	ClearPrompt(true);

	FSGSPlayPhaseDecision Decision;
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("LocalUI")), CardName),
		FSGSUseCardCommandPayload(CardId, MoveTemp(TargetSeatIndices)));

	UE_LOG(LogSGSUI, Log, TEXT("LocalUI submitted UseCard command: CardId=%d."), CardId);
	Delegate.ExecuteIfBound(Decision);
	return true;
}

bool USGSLocalHumanDecisionAgent::SubmitSkill(
	FName SkillName,
	TArray<int32> SelectedCardIds,
	int32 TargetSeatIndex)
{
	if (!HasPendingPlayRequest())
	{
		return false;
	}

	const FSGSDecisionSkillOption* Option = PendingPlayRequest.SkillOptions.FindByPredicate(
		[SkillName](const FSGSDecisionSkillOption& Candidate)
		{
			return Candidate.SkillName == SkillName;
		});
	if (Option == nullptr
		|| SelectedCardIds.Num() < Option->MinCardCount
		|| SelectedCardIds.Num() > Option->MaxCardCount)
	{
		return false;
	}
	for (const int32 CardId : SelectedCardIds)
	{
		if (!Option->SelectableCardIds.Contains(CardId))
		{
			return false;
		}
	}

	TArray<int32> Targets;
	if (Option->MinTargetCount > 0)
	{
		if (!Option->TargetSeatIndices.Contains(TargetSeatIndex))
		{
			return false;
		}
		Targets.Add(TargetSeatIndex);
	}

	const FSGSPlayPhaseRequest Request = PendingPlayRequest;
	const FName RuleKindTag = Option->RuleKindTag;
	const FName ResultCardName = Option->ResultCardName;
	FSGSPlayPhaseDecisionDelegate Delegate = MoveTemp(PendingPlayDecisionDelegate);
	ClearPrompt(true);

	FSGSPlayPhaseDecision Decision;
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("LocalUI")), SkillName),
		FSGSActivateSkillCommandPayload(
			SkillName,
			RuleKindTag,
			ResultCardName,
			MoveTemp(SelectedCardIds),
			MoveTemp(Targets)));
	Delegate.ExecuteIfBound(Decision);
	return true;
}

bool USGSLocalHumanDecisionAgent::SubmitResponseCard(
	int32 CardId,
	int32 TargetSeatIndex,
	FName SkillName)
{
	TArray<int32> SelectedCardIds;
	if (CardId != INDEX_NONE)
	{
		SelectedCardIds.Add(CardId);
	}
	return SubmitResponseCards(MoveTemp(SelectedCardIds), TargetSeatIndex, SkillName);
}

bool USGSLocalHumanDecisionAgent::SubmitResponseCards(
	TArray<int32> SelectedCardIds,
	int32 TargetSeatIndex,
	FName SkillName)
{
	if (!HasPendingResponseRequest())
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard ignored: no local response prompt is pending."));
		return false;
	}
	if (PendingResponseRequest.bIsOptionChoice)
	{
		if (SkillName.IsNone()
			|| !SelectedCardIds.IsEmpty()
			|| !PendingResponseRequest.NamedOptions.ContainsByPredicate(
				[SkillName](const FSGSDecisionNamedOption& Option) { return Option.OptionName == SkillName; }))
		{
			return false;
		}
		const FSGSResponseRequest Request = PendingResponseRequest;
		FSGSResponseDecisionDelegate Delegate = MoveTemp(PendingResponseDecisionDelegate);
		ClearPrompt(true);
		FSGSResponseDecision Decision;
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("LocalUI")), Request.ChoiceName),
			FSGSChooseOptionCommandPayload(Request.ChoiceName, Request.WindowName, SkillName));
		Delegate.ExecuteIfBound(Decision);
		return true;
	}
	if (PendingResponseRequest.bIsCardChoice)
	{
		if (!SkillName.IsNone()
			|| SelectedCardIds.Num() < PendingResponseRequest.MinChoiceCount
			|| SelectedCardIds.Num() > PendingResponseRequest.MaxChoiceCount)
		{
			return false;
		}
		TSet<int32> UniqueCards;
		for (const int32 CardId : SelectedCardIds)
		{
			if (UniqueCards.Contains(CardId)
				|| !PendingResponseRequest.CardChoiceOptions.ContainsByPredicate(
					[CardId](const FSGSDecisionCardChoiceOption& Option) { return Option.CardId == CardId; }))
			{
				return false;
			}
			UniqueCards.Add(CardId);
		}

		const FSGSResponseRequest Request = PendingResponseRequest;
		FSGSResponseDecisionDelegate Delegate = MoveTemp(PendingResponseDecisionDelegate);
		ClearPrompt(true);
		FSGSResponseDecision Decision;
		Decision.Command = FSGSCommandFactory::Make(
			FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("LocalUI")), Request.ChoiceName),
			FSGSChooseCardsCommandPayload(Request.ChoiceName, Request.WindowName, MoveTemp(SelectedCardIds)));
		Delegate.ExecuteIfBound(Decision);
		return true;
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
	if (SkillOption == nullptr
		&& (SelectedCardIds.Num() != 1
			|| !PendingResponseRequest.ResponseCardIds.Contains(SelectedCardIds[0])))
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard rejected: the selected physical response card is not legal."));
		return false;
	}
	if (SkillOption != nullptr
		&& (SelectedCardIds.Num() < SkillOption->MinCardCount
			|| SelectedCardIds.Num() > SkillOption->MaxCardCount))
	{
		UE_LOG(LogSGSUI, Warning, TEXT("SubmitResponseCard rejected: the card count is not legal for skill %s."), *SkillName.ToString());
		return false;
	}
	if (SkillOption != nullptr)
	{
		TSet<int32> UniqueCards;
		for (const int32 CardId : SelectedCardIds)
		{
			if (UniqueCards.Contains(CardId) || !SkillOption->SelectableCardIds.Contains(CardId))
			{
				return false;
			}
			UniqueCards.Add(CardId);
		}
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
	const FName ResultCardName = SkillOption != nullptr ? SkillOption->ResultCardName : NAME_None;
	FSGSResponseDecisionDelegate Delegate = MoveTemp(PendingResponseDecisionDelegate);
	ClearPrompt(true);

	FSGSResponseDecision Decision;
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(
			Request,
			FName(TEXT("LocalUI")),
			SkillName.IsNone() ? Request.RequiredCardName : SkillName),
		FSGSRespondCardCommandPayload(MoveTemp(SelectedCardIds), MoveTemp(Targets), Request.WindowName, SkillName, ResultCardName));

	UE_LOG(LogSGSUI, Log, TEXT("LocalUI submitted response command: Skill=%s Window=%s."),
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
