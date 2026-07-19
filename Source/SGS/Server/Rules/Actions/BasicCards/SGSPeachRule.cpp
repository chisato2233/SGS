#include "Server/Rules/Actions/BasicCards/SGSPeachRule.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

FName FSGSPeachRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Peach"));
}

FSGSRuleDescriptor FSGSPeachRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_UseCard.GetTag(),
		FName(TEXT("Peach")),
		NAME_None,
		100);
}

bool FSGSPeachRule::CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::IsIntent(Context, SGSGameplayTags::PlayAction_UseCard)
		&& Context.RuleInvocation.WindowName.IsNone()
		&& Payload.CardName == TEXT("Peach");
}

FSGSStatus FSGSPeachRule::ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	const int32 HealTargetSeat = SGSBasicCardRuleHelpers::GetPeachHealTarget(Context, Payload);
	USGSCard* PeachCard = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	if (PeachCard == nullptr || PeachCard->CardName != TEXT("Peach") || Context.GameContext->GetSeat(HealTargetSeat) == nullptr)
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidPeach")),
			TEXT("Peach requires a Peach card and a valid heal target."));
	}

	return MakeValue();
}

FSGSStatus FSGSPeachRule::ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const
{
	const int32 HealTargetSeat = SGSBasicCardRuleHelpers::GetPeachHealTarget(Context, Payload);
	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = GetRuleName();
	Frame.SourceCommandId = SGSBasicCardRuleHelpers::GetCommandId(Context);
	Frame.ActorSeat = SGSBasicCardRuleHelpers::GetCommandSeat(Context);
	Frame.SourceSeat = SGSBasicCardRuleHelpers::GetCommandSeat(Context);
	Frame.TargetSeat = HealTargetSeat;
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	if (FSGSStatus Status = SGSBasicCardRuleHelpers::ExecuteHealCard(
		Context,
		Payload.CardId,
		FName(TEXT("Peach")),
		HealTargetSeat);
		Status.HasError())
	{
		Context.Runtime->AbortAllFrames(FName(TEXT("SGS.Resolution.PeachFailed")));
		return Status;
	}

	return SGSBasicCardRuleHelpers::CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.PeachResolved")));
}
