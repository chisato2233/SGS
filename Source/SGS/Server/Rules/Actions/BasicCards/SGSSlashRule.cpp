#include "Server/Rules/Actions/BasicCards/SGSSlashRule.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

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

FName FSGSSlashRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Slash"));
}

FSGSRuleDescriptor FSGSSlashRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_UseCard.GetTag(),
		FName(TEXT("Slash")),
		NAME_None,
		100);
}

bool FSGSSlashRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::IsIntent(Context, SGSGameplayTags::PlayAction_UseCard)
		&& Context.RuleInvocation.WindowName.IsNone()
		&& Payload.CardName == TEXT("Slash");
}

FSGSStatus FSGSSlashRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	if (Payload.TargetSeatIndices.Num() != 1)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidTarget")),
			TEXT("Slash requires exactly one target."));
	}

	const int32 SourceSeat = SGSBasicCardRuleHelpers::GetCommandSeat(Context);
	const int32 TargetSeat = Payload.TargetSeatIndices[0];
	USGSCard* SlashCard = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	if (SlashCard == nullptr || SlashCard->CardName != TEXT("Slash") || Context.GameContext->GetSeat(TargetSeat) == nullptr || TargetSeat == SourceSeat)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidSlash")),
			TEXT("Slash requires a Slash card and a valid non-self target."));
	}

	if (Context.GameContext->GetDistance(SourceSeat, TargetSeat) > 1)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.TargetOutOfDistance")),
			TEXT("Slash target is out of distance."));
	}

	return MakeValue();
}

FSGSStatus FSGSSlashRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	USGSCard* SlashCard = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	const int32 SourceSeat = SGSBasicCardRuleHelpers::GetCommandSeat(Context);
	const int32 TargetSeat = Payload.TargetSeatIndices[0];

	if (FSGSStatus Status = Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ SlashCard },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		SourceSeat,
		SGSGameplayTags::CardZone_Processing.GetTag(),
		INDEX_NONE),
		SGSBasicCardRuleHelpers::GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}

	FSGSSlashResolutionState SlashState;
	SlashState.SlashCardId = SlashCard->CardId;
	SlashState.SourceSeat = SourceSeat;
	SlashState.TargetSeat = TargetSeat;

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = GetRuleName();
	Frame.SourceCommandId = SGSBasicCardRuleHelpers::GetCommandId(Context);
	Frame.ActorSeat = SourceSeat;
	Frame.SourceSeat = SourceSeat;
	Frame.TargetSeat = TargetSeat;
	Frame.ProcessingCardId = SlashCard->CardId;
	Frame.OnChildCompletedContinuation = SGSResolutionContinuations::FinishParentCardResolution();
	Frame.FrameState = FInstancedStruct::Make(SlashState);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	FSGSRuleResponseWindowSpec WindowSpec;
	WindowSpec.SeatIndex = TargetSeat;
	WindowSpec.WindowName = SGSBasicCardRuleHelpers::SlashDodgeWindowName();
	WindowSpec.RequiredCardName = FName(TEXT("Dodge"));
	WindowSpec.EffectSourceSeat = SourceSeat;
	WindowSpec.EffectTargetSeat = TargetSeat;
	if (Context.Runtime->OpenResponseWindow(WindowSpec))
	{
		return MakeValue();
	}

	return SGSBasicCardRuleHelpers::ResolveSlashHit(Context);
}
