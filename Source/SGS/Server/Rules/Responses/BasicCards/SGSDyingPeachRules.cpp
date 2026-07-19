#include "Server/Rules/Responses/BasicCards/SGSDyingPeachRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

namespace
{
FSGSStatus ExecuteDyingHeal(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload,
	FName CardName)
{
	FSGSResolutionFrame* DyingFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSDyingPeachResolutionState* DyingState = DyingFrame != nullptr
		? DyingFrame->GetState<FSGSDyingPeachResolutionState>()
		: nullptr;
	if (DyingState == nullptr)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.MissingResolutionFrame")),
			TEXT("Dying heal response requires the current DyingPeach frame."));
	}

	const int32 DyingSeatIndex = DyingState->DyingSeat;
	if (FSGSStatus Status = SGSBasicCardRuleHelpers::ExecuteHealCard(
		Context,
		Payload.CardId,
		CardName,
		DyingSeatIndex);
		Status.HasError())
	{
		return Status;
	}

	const USGSSeat* DyingSeat = Context.GameContext->GetSeat(DyingSeatIndex);
	if (DyingSeat != nullptr && DyingSeat->Health > 0)
	{
		return SGSBasicCardRuleHelpers::CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.DyingPeachResolved")));
	}

	return SGSBasicCardRuleHelpers::ContinueDyingPeachOrEliminate(Context);
}
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

FName FSGSDyingPeachPassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.DyingPeachPass"));
}

FSGSRuleDescriptor FSGSDyingPeachPassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None,
		SGSBasicCardRuleHelpers::DyingPeachWindowName(),
		210);
}

bool FSGSDyingPeachPassRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::IsIntent(Context, SGSGameplayTags::PlayAction_Pass)
		&& SGSBasicCardRuleHelpers::IsWindow(Payload.WindowName, SGSBasicCardRuleHelpers::DyingPeachWindowName());
}

FSGSStatus FSGSDyingPeachPassRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	(void)Payload;
	return SGSBasicCardRuleHelpers::ContinueDyingPeachOrEliminate(Context);
}

FName FSGSDyingPeachRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.DyingPeach"));
}

FSGSRuleDescriptor FSGSDyingPeachRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Peach")),
		SGSBasicCardRuleHelpers::DyingPeachWindowName(),
		200);
}

bool FSGSDyingPeachRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::IsIntent(Context, SGSGameplayTags::PlayAction_RespondCard)
		&& SGSBasicCardRuleHelpers::IsWindow(Payload.WindowName, SGSBasicCardRuleHelpers::DyingPeachWindowName())
		&& Payload.CardName == TEXT("Peach");
}

FSGSStatus FSGSDyingPeachRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	USGSCard* PeachCard = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	if (PeachCard == nullptr || PeachCard->CardName != TEXT("Peach"))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidDyingPeach")),
			TEXT("Dying.Peach response requires a Peach card."));
	}

	return MakeValue();
}

FSGSStatus FSGSDyingPeachRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	return ExecuteDyingHeal(Context, Payload, FName(TEXT("Peach")));
}

FName FSGSDyingAnalepticRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.DyingAnaleptic"));
}

FSGSRuleDescriptor FSGSDyingAnalepticRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Analeptic")),
		SGSBasicCardRuleHelpers::DyingPeachWindowName(),
		200);
}

bool FSGSDyingAnalepticRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::IsIntent(Context, SGSGameplayTags::PlayAction_RespondCard)
		&& SGSBasicCardRuleHelpers::IsWindow(Payload.WindowName, SGSBasicCardRuleHelpers::DyingPeachWindowName())
		&& Payload.CardName == TEXT("Analeptic");
}

FSGSStatus FSGSDyingAnalepticRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	USGSCard* Card = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	const FSGSResolutionFrame* DyingFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSDyingPeachResolutionState* DyingState = DyingFrame != nullptr
		? DyingFrame->GetState<FSGSDyingPeachResolutionState>()
		: nullptr;
	if (Card == nullptr
		|| Card->CardName != TEXT("Analeptic")
		|| DyingState == nullptr
		|| SGSBasicCardRuleHelpers::GetCommandSeat(Context) != DyingState->DyingSeat)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidDyingAnaleptic")),
			TEXT("Only the dying seat may respond with Analeptic."));
	}

	return MakeValue();
}

FSGSStatus FSGSDyingAnalepticRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSRespondCardRulePayload& Payload) const
{
	return ExecuteDyingHeal(Context, Payload, FName(TEXT("Analeptic")));
}
