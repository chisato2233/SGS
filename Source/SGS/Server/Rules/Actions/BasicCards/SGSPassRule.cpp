#include "Server/Rules/Actions/BasicCards/SGSPassRule.h"

#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

FName FSGSPassRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Pass"));
}

FSGSRuleDescriptor FSGSPassRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_Pass.GetTag(),
		NAME_None,
		NAME_None,
		-1000);
}

bool FSGSPassRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::IsIntent(Context, SGSGameplayTags::PlayAction_Pass) && Payload.WindowName.IsNone();
}

FSGSStatus FSGSPassRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const
{
	(void)Payload;
	Context.Runtime->AdvanceAfterPhase();
	return MakeValue();
}
