#include "Server/Rules/Phases/SGSGeneralSelectionRules.h"

#include "Server/Engine/SGSGameContext.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"
#include "Server/Rules/Resolution/SGSGeneralSelectionResolution.h"

FName FSGSGeneralSelectionChoiceRule::GetRuleName() const
{
	return FName(TEXT("SGS.Rule.GeneralSelection.Choose"));
}

FSGSRuleDescriptor FSGSGeneralSelectionChoiceRule::GetDescriptor() const
{
	return SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
		GetRuleName(),
		SGSRuleKinds::Response(),
		SGSGameplayTags::PlayAction_ChooseOption.GetTag(),
		SGSGeneralSelectionResolution::ChoiceName(),
		SGSGeneralSelectionResolution::WindowName(),
		300);
}

FSGSStatus FSGSGeneralSelectionChoiceRule::ValidatePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSChooseOptionRulePayload& Payload) const
{
	const FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSGeneralSelectionResolutionState* State = Frame != nullptr
		? Frame->GetState<FSGSGeneralSelectionResolutionState>()
		: nullptr;
	return State != nullptr
		&& State->SeatOrder.IsValidIndex(State->NextSeatIndex)
		&& State->SeatOrder[State->NextSeatIndex] == Context.RuleInvocation.ActorSeat
		&& State->AvailableGeneralIds.Contains(Payload.SelectedOption)
		? MakeValue()
		: SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.GeneralSelection.InvalidChoice")),
			TEXT("The chosen general is not available to the current seat."));
}

FSGSStatus FSGSGeneralSelectionChoiceRule::ExecutePayload(
	FSGSRuleExecutionContext& Context,
	const FSGSChooseOptionRulePayload& Payload) const
{
	FSGSResolutionFrame* Frame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSGeneralSelectionResolutionState* State = Frame != nullptr
		? Frame->GetMutableState<FSGSGeneralSelectionResolutionState>()
		: nullptr;
	check(State != nullptr);
	if (!Context.GameContext->AssignGeneral(Context.RuleInvocation.ActorSeat, Payload.SelectedOption))
	{
		return SGSBasicCardRuleHelpers::MakeRuleError(
			FName(TEXT("SGS.GeneralSelection.AssignFailed")),
			TEXT("The selected general definition is unavailable."));
	}
	State->AvailableGeneralIds.Remove(Payload.SelectedOption);
	++State->NextSeatIndex;
	return Context.Runtime->ContinueGeneralSelection();
}

void SGSGeneralSelectionRules::Register(FSGSRuleRegistry& Registry)
{
	Registry.RegisterRule(MakeShared<FSGSGeneralSelectionChoiceRule>());
}
