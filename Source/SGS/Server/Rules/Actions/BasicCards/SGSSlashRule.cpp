#include "Server/Rules/Actions/BasicCards/SGSSlashRule.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

FString FSGSSlashResolutionState::ToLogString() const
{
	return FString::Printf(TEXT("SlashCard=%d Virtual=%s IgnoreArmor=%s Source=%d Target=%d"),
		SlashCardId,
		bVirtualSlash ? TEXT("true") : TEXT("false"),
		bIgnoreArmor ? TEXT("true") : TEXT("false"),
		SourceSeat,
		TargetSeat);
}

bool FSGSSlashResolutionState::CheckInvariants() const
{
	bool bOk = true;
	bOk &= ensureMsgf(bVirtualSlash || SlashCardId != INDEX_NONE, TEXT("Physical Slash resolution requires a card id."));
	bOk &= ensureMsgf(SourceSeat != INDEX_NONE, TEXT("SlashResolutionState requires a source seat."));
	bOk &= ensureMsgf(TargetSeat != INDEX_NONE, TEXT("SlashResolutionState requires a target seat."));
	bOk &= ensureMsgf(DamageAmount > 0, TEXT("SlashResolutionState requires positive damage."));
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
	USGSCard* SlashCard = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	if (SlashCard == nullptr || SlashCard->CardName != TEXT("Slash"))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidSlash")),
			TEXT("Slash requires a Slash card and a valid non-self target."));
	}

	return SGSBasicCardRuleHelpers::ValidateSlashUse(Context, SlashCard, Payload.TargetSeatIndices);
}

FSGSStatus FSGSSlashRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	USGSCard* SlashCard = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	return SGSBasicCardRuleHelpers::ExecuteSlashUse(Context, SlashCard, Payload.TargetSeatIndices, GetRuleName());
}
