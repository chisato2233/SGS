#include "Server/Rules/Responses/BasicCards/SGSDodgeRules.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Rules/Actions/BasicCards/SGSSlashRule.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

FName FSGSDodgePassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.DodgePass"));
}

FSGSRuleDescriptor FSGSDodgePassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None,
		SGSBasicCardRuleHelpers::SlashDodgeWindowName(),
		210);
}

bool FSGSDodgePassRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::IsIntent(Context, SGSGameplayTags::PlayAction_Pass)
		&& SGSBasicCardRuleHelpers::IsWindow(Payload.WindowName, SGSBasicCardRuleHelpers::SlashDodgeWindowName());
}

FSGSStatus FSGSDodgePassRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	(void)Payload;
	return SGSBasicCardRuleHelpers::ResolveSlashHit(Context);
}

FName FSGSDodgeResponseRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.DodgeResponse"));
}

FSGSRuleDescriptor FSGSDodgeResponseRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_RespondCard.GetTag(),
		FName(TEXT("Dodge")),
		SGSBasicCardRuleHelpers::SlashDodgeWindowName(),
		200);
}

bool FSGSDodgeResponseRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::IsIntent(Context, SGSGameplayTags::PlayAction_RespondCard)
		&& SGSBasicCardRuleHelpers::IsWindow(Payload.WindowName, SGSBasicCardRuleHelpers::SlashDodgeWindowName())
		&& Payload.CardName == TEXT("Dodge");
}

FSGSStatus FSGSDodgeResponseRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	USGSCard* DodgeCard = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	if (DodgeCard == nullptr || DodgeCard->CardName != TEXT("Dodge"))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidDodge")),
			TEXT("Dodge response requires a Dodge card."));
	}

	return MakeValue();
}

FSGSStatus FSGSDodgeResponseRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const
{
	USGSCard* DodgeCard = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	if (FSGSStatus Status = SGSBasicCardRuleHelpers::DiscardHandCard(Context, DodgeCard, SGSBasicCardRuleHelpers::GetCommandSeat(Context)); Status.HasError())
	{
		return Status;
	}
	FSGSResolutionFrame* SlashFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSSlashResolutionState* SlashState = SlashFrame != nullptr
		? SlashFrame->GetMutableState<FSGSSlashResolutionState>()
		: nullptr;
	if (SlashState != nullptr && ++SlashState->DodgeCount < SlashState->RequiredDodgeCount)
	{
		FSGSRuleResponseWindowSpec WindowSpec;
		WindowSpec.SeatIndex = SlashState->TargetSeat;
		WindowSpec.WindowName = SGSBasicCardRuleHelpers::SlashDodgeWindowName();
		WindowSpec.RequiredCardName = FName(TEXT("Dodge"));
		WindowSpec.AcceptedCardNames.Add(FName(TEXT("Dodge")));
		WindowSpec.ContextName = FName(TEXT("Slash"));
		WindowSpec.EffectSourceSeat = SlashState->SourceSeat;
		WindowSpec.EffectTargetSeat = SlashState->TargetSeat;
		if (Context.Runtime->OpenResponseWindow(WindowSpec))
		{
			return MakeValue();
		}
		return SGSBasicCardRuleHelpers::ResolveSlashHit(Context);
	}
	if (FSGSStatus Status = SGSBasicCardRuleHelpers::DiscardProcessingCard(Context); Status.HasError())
	{
		return Status;
	}

	return SGSBasicCardRuleHelpers::CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.SlashDodged")));
}
