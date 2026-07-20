#include "Server/Commands/Types/SGSActivateSkillCommandType.h"

#include "Shared/Commands/SGSCommandPayloadTraits.h"
#include "Server/Engine/SGSGameContext.h"

namespace
{
bool IsOwnedHandCard(const USGSGameContext& Context, int32 SeatIndex, int32 CardId)
{
	const FSGSCardState* State = Context.FindCardStateById(CardId);
	return State != nullptr
		&& State->OwnerSeat == SeatIndex
		&& State->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Hand.GetTag());
}
}

FGameplayTag FSGSActivateSkillCommandType::GetType() const
{
	return TSGSCommandPayloadTraits<FSGSActivateSkillCommandPayload>::GetType();
}

FSGSStatus FSGSActivateSkillCommandType::ValidateTyped(
	const FSGSCommand& Command,
	const FSGSActivateSkillCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	if (!Context.ExpectedWindowName.IsNone())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.SkillDuringResponse")),
			TEXT("ActivateSkill is only valid during a play-phase decision.")));
	}

	if (Payload.SkillName.IsNone()
		|| (Payload.RuleKindTag != SGSRuleKinds::Action()
			&& Payload.RuleKindTag != SGSRuleKinds::ViewAs()))
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Command.InvalidSkill")),
			TEXT("ActivateSkill requires a skill name and an action or view-as rule kind.")));
	}

	TSet<int32> UniqueCards;
	for (const int32 CardId : Payload.SelectedCardIds)
	{
		if (UniqueCards.Contains(CardId)
			|| Context.GameContext == nullptr
			|| !IsOwnedHandCard(*Context.GameContext, Command.SeatIndex, CardId))
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.Command.InvalidSkillCard")),
				TEXT("Every selected skill card must be a distinct card in the acting seat's hand.")));
		}
		UniqueCards.Add(CardId);
	}

	return MakeValue();
}

TSGSResult<FSGSRuleInvocation> FSGSActivateSkillCommandType::BuildRuleInvocationTyped(
	const FSGSCommand& Command,
	const FSGSActivateSkillCommandPayload& Payload,
	const FSGSCommandExecutionContext& Context) const
{
	(void)Context;

	FSGSActivateSkillRulePayload RulePayload;
	RulePayload.SkillName = Payload.SkillName;
	RulePayload.ResultCardName = Payload.ResultCardName;
	RulePayload.SelectedCardIds = Payload.SelectedCardIds;
	RulePayload.TargetSeatIndices = Payload.TargetSeatIndices;

	FSGSRuleInvocation Invocation;
	Invocation.RuleKindTag = Payload.RuleKindTag;
	Invocation.IntentTag = Command.Type;
	Invocation.SubjectName = Payload.SkillName;
	Invocation.ActorSeat = Command.SeatIndex;
	Invocation.SourceCommandId = Command.CommandId;
	Invocation.SourceRequestId = Command.RequestId;
	Invocation.Payload = FInstancedStruct::Make(RulePayload);
	return MakeValue(MoveTemp(Invocation));
}
