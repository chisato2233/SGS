#include "Server/Rules/Responses/BasicCards/SGSDodgeRules.h"

#include "Shared/Cards/SGSCard.h"
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
	return SGSBasicCardRuleHelpers::ResolveSuccessfulSlashDodge(Context);
}
