#include "Server/Rules/Actions/BasicCards/SGSAnalepticRule.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

FName FSGSAnalepticRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.Analeptic"));
}

FSGSRuleDescriptor FSGSAnalepticRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Action(),
		SGSGameplayTags::PlayAction_UseCard.GetTag(),
		FName(TEXT("Analeptic")),
		NAME_None,
		100);
}

bool FSGSAnalepticRule::CanHandlePayload(
	const FSGSRuleExecutionContext& Context,
	const FSGSUseCardRulePayload& Payload) const
{
	return SGSBasicCardRuleHelpers::IsIntent(Context, SGSGameplayTags::PlayAction_UseCard)
		&& Context.RuleInvocation.WindowName.IsNone()
		&& Payload.CardName == TEXT("Analeptic");
}

FSGSStatus FSGSAnalepticRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSUseCardRulePayload& Payload) const
{
	const int32 SourceSeat = SGSBasicCardRuleHelpers::GetCommandSeat(Context);
	USGSCard* Card = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);
	if (Card == nullptr || Card->CardName != TEXT("Analeptic") || !Payload.TargetSeatIndices.IsEmpty())
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidAnaleptic")),
			TEXT("Analeptic requires an Analeptic card and no target."));
	}

	if (SGSBasicCardRuleHelpers::HasStatus(Context, SourceSeat, SGSGameplayTags::Status_AnalepticUsed.GetTag()))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.Rule.AnalepticAlreadyUsed")),
			TEXT("Analeptic can only be used once per turn."));
	}

	return MakeValue();
}

FSGSStatus FSGSAnalepticRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSUseCardRulePayload& Payload) const
{
	const int32 SourceSeat = SGSBasicCardRuleHelpers::GetCommandSeat(Context);
	USGSCard* Card = SGSBasicCardRuleHelpers::FindPayloadCard(Context, Payload.CardId);

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = GetRuleName();
	Frame.SourceCommandId = SGSBasicCardRuleHelpers::GetCommandId(Context);
	Frame.ActorSeat = SourceSeat;
	Frame.SourceSeat = SourceSeat;
	Frame.TargetSeat = SourceSeat;
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	if (FSGSStatus Status = SGSBasicCardRuleHelpers::DiscardHandCard(Context, Card, SourceSeat); Status.HasError())
	{
		Context.Runtime->AbortAllFrames(FName(TEXT("SGS.Resolution.AnalepticFailed")));
		return Status;
	}

	SGSBasicCardRuleHelpers::AddStatus(
		Context,
		SourceSeat,
		FName(TEXT("SGS.ActiveEffect.AnalepticUsed")),
		SGSGameplayTags::Status_AnalepticUsed.GetTag(),
		FSGSDurationSpec::ThisTurn(SourceSeat, Context.TimingPoint));
	SGSBasicCardRuleHelpers::AddStatus(
		Context,
		SourceSeat,
		FName(TEXT("SGS.ActiveEffect.AnalepticBoost")),
		SGSGameplayTags::Status_AnalepticBoost.GetTag(),
		FSGSDurationSpec::ThisTurn(SourceSeat, Context.TimingPoint));

	return SGSBasicCardRuleHelpers::CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.AnalepticResolved")));
}
