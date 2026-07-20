#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Rules/Actions/BasicCards/SGSSlashRule.h"
#include "Server/Rules/Responses/BasicCards/SGSDyingPeachRules.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"

namespace
{
FSGSStatus EliminateDyingSeatAndFinish(FSGSRuleExecutionContext& Context, int32 DyingSeatIndex)
{
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeEliminateSeatStep(
			DyingSeatIndex,
			SGSBasicCardRuleHelpers::GetCurrentEffectSourceSeat(Context),
			FName(TEXT("NoPeach"))),
		SGSBasicCardRuleHelpers::GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}

	return SGSBasicCardRuleHelpers::CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.DyingPeachNoPeach")));
}

bool OpenNextDyingPeachResponseWindow(FSGSRuleExecutionContext& Context, FSGSResolutionFrame& DyingFrame)
{
	FSGSDyingPeachResolutionState* DyingState = DyingFrame.GetMutableState<FSGSDyingPeachResolutionState>();
	if (DyingState == nullptr)
	{
		return false;
	}

	const USGSSeat* DyingSeat = Context.GameContext->GetSeat(DyingState->DyingSeat);
	if (DyingSeat == nullptr || !DyingSeat->bIsAlive || DyingSeat->Health > 0)
	{
		return false;
	}

	DyingState->bNeedsHealthRecheck = false;
	while (DyingState->ResponderSeatIndices.IsValidIndex(DyingState->NextResponderIndex))
	{
		const int32 ResponderSeat = DyingState->ResponderSeatIndices[DyingState->NextResponderIndex++];
		const USGSSeat* Responder = Context.GameContext->GetSeat(ResponderSeat);
		if (Responder == nullptr || !Responder->bIsAlive)
		{
			continue;
		}

		FSGSRuleResponseWindowSpec WindowSpec;
		WindowSpec.SeatIndex = ResponderSeat;
		WindowSpec.WindowName = SGSBasicCardRuleHelpers::DyingPeachWindowName();
		WindowSpec.RequiredCardName = FName(TEXT("Peach"));
		WindowSpec.AcceptedCardNames.Add(FName(TEXT("Peach")));
		if (ResponderSeat == DyingState->DyingSeat)
		{
			WindowSpec.AcceptedCardNames.Add(FName(TEXT("Analeptic")));
		}
		WindowSpec.ContextName = FName(TEXT("Dying"));
		WindowSpec.EffectSourceSeat = DyingFrame.SourceSeat;
		WindowSpec.EffectTargetSeat = DyingFrame.TargetSeat;
		if (Context.Runtime->OpenResponseWindow(WindowSpec))
		{
			return true;
		}
	}

	return false;
}

FSGSResolutionFrame* FindLatestDyingFrameForSeat(FSGSResolutionStack& Stack, int32 DyingSeatIndex)
{
	return Stack.FindLatestFrameByPredicate([DyingSeatIndex](FSGSResolutionFrame& Frame)
	{
		const FSGSDyingPeachResolutionState* DyingState = Frame.GetState<FSGSDyingPeachResolutionState>();
		return DyingState != nullptr && DyingState->DyingSeat == DyingSeatIndex;
	});
}
}

FName SGSBasicCardRuleHelpers::SlashDodgeWindowName()
{
	return FName(TEXT("Slash.Dodge"));
}

FName SGSBasicCardRuleHelpers::DyingPeachWindowName()
{
	return FName(TEXT("Dying.Peach"));
}

FSGSRuleDescriptor SGSBasicCardRuleHelpers::MakeBasicRuleDescriptor(
	FName RuleName,
	FName RuleKindTag,
	FGameplayTag IntentTag,
	FName SubjectName,
	FName WindowName,
	int32 Priority,
	bool bWildcardIntent,
	bool bWildcardSubject,
	bool bWildcardWindow)
{
	FSGSRuleDescriptor Descriptor;
	Descriptor.RuleName = RuleName;
	Descriptor.RuleKindTag = RuleKindTag;
	Descriptor.IntentTag = IntentTag;
	Descriptor.SubjectName = SubjectName;
	Descriptor.WindowName = WindowName;
	Descriptor.Priority = Priority;
	Descriptor.bWildcardIntent = bWildcardIntent;
	Descriptor.bWildcardSubject = bWildcardSubject;
	Descriptor.bWildcardWindow = bWildcardWindow;
	return Descriptor;
}

bool SGSBasicCardRuleHelpers::IsIntent(const FSGSRuleExecutionContext& Context, const FNativeGameplayTag& Type)
{
	return Context.RuleInvocation.IntentTag.MatchesTagExact(Type.GetTag());
}

bool SGSBasicCardRuleHelpers::IsWindow(FName ActualWindowName, FName ExpectedWindowName)
{
	return ActualWindowName == ExpectedWindowName;
}

FSGSStatus SGSBasicCardRuleHelpers::MakeRuleError(FName Code, FString Message)
{
	return MakeError(FSGSError::Make(Code, MoveTemp(Message)));
}

USGSCard* SGSBasicCardRuleHelpers::FindPayloadCard(const FSGSRuleExecutionContext& Context, int32 CardId)
{
	return Context.GameContext != nullptr ? Context.GameContext->FindCardById(CardId) : nullptr;
}

int32 SGSBasicCardRuleHelpers::GetCommandSeat(const FSGSRuleExecutionContext& Context)
{
	return Context.Command != nullptr ? Context.Command->SeatIndex : INDEX_NONE;
}

FSGSCommandId SGSBasicCardRuleHelpers::GetCommandId(const FSGSRuleExecutionContext& Context)
{
	return Context.Command != nullptr ? Context.Command->CommandId : FSGSCommandId();
}

bool SGSBasicCardRuleHelpers::HasStatus(const FSGSRuleExecutionContext& Context, int32 SeatIndex, FGameplayTag StatusTag)
{
	for (const FSGSStableHandle Handle : Context.ActiveEffects->QueryByTag(StatusTag))
	{
		const FSGSActiveEffect* Effect = Context.ActiveEffects->Find(Handle);
		if (Effect != nullptr && Effect->OwnerSeat == SeatIndex)
		{
			return true;
		}
	}

	return false;
}

int32 SGSBasicCardRuleHelpers::CountStatus(const FSGSRuleExecutionContext& Context, int32 SeatIndex, FGameplayTag StatusTag)
{
	int32 Count = 0;
	for (const FSGSStableHandle Handle : Context.ActiveEffects->QueryByTag(StatusTag))
	{
		const FSGSActiveEffect* Effect = Context.ActiveEffects->Find(Handle);
		if (Effect != nullptr && Effect->OwnerSeat == SeatIndex)
		{
			++Count;
		}
	}
	return Count;
}

void SGSBasicCardRuleHelpers::AddStatus(
	FSGSRuleExecutionContext& Context,
	int32 SeatIndex,
	FName EffectName,
	FGameplayTag StatusTag,
	FSGSDurationSpec Duration)
{
	FSGSActiveEffect Effect;
	Effect.EffectName = EffectName;
	Effect.PrimaryTag = StatusTag;
	Effect.SourceCommandId = GetCommandId(Context);
	Effect.OwnerSeat = SeatIndex;
	Effect.TargetSeat = SeatIndex;
	Effect.Duration = MoveTemp(Duration);
	Effect.CreatedAt = Context.TimingPoint;
	Context.ActiveEffects->Add(MoveTemp(Effect));
}

bool SGSBasicCardRuleHelpers::ConsumeStatus(FSGSRuleExecutionContext& Context, int32 SeatIndex, FGameplayTag StatusTag)
{
	for (const FSGSStableHandle Handle : Context.ActiveEffects->QueryByTag(StatusTag))
	{
		const FSGSActiveEffect* Effect = Context.ActiveEffects->Find(Handle);
		if (Effect != nullptr && Effect->OwnerSeat == SeatIndex)
		{
			Context.ActiveEffects->Expire(Handle, Context.TimingPoint, SGSActiveEffectExpireReasons::Manual());
			return true;
		}
	}

	return false;
}

FSGSStatus SGSBasicCardRuleHelpers::DiscardHandCard(FSGSRuleExecutionContext& Context, USGSCard* Card, int32 SeatIndex)
{
	if (Card == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidCard")),
			TEXT("Expected a valid hand card to discard."));
	}

	FSGSCardMoveEventMetadata Metadata;
	Metadata.Reason = Context.RuleInvocation.IntentTag.MatchesTagExact(SGSGameplayTags::PlayAction_RespondCard.GetTag())
		? SGSCardMoveReasons::Respond()
		: SGSCardMoveReasons::Use();
	if (const FSGSUseCardRulePayload* UsePayload = Context.RuleInvocation.GetPayload<FSGSUseCardRulePayload>())
	{
		Metadata.RelatedTargetSeatIndices = UsePayload->TargetSeatIndices;
	}
	else if (const FSGSRespondCardRulePayload* ResponsePayload = Context.RuleInvocation.GetPayload<FSGSRespondCardRulePayload>())
	{
		if (ResponsePayload->EffectSourceSeat != INDEX_NONE)
		{
			Metadata.RelatedTargetSeatIndices.Add(ResponsePayload->EffectSourceSeat);
		}
		if (ResponsePayload->EffectTargetSeat != INDEX_NONE
			&& ResponsePayload->EffectTargetSeat != ResponsePayload->EffectSourceSeat)
		{
			Metadata.RelatedTargetSeatIndices.Add(ResponsePayload->EffectTargetSeat);
		}
	}

	return Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ Card },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		SeatIndex,
		SGSGameplayTags::CardZone_DiscardPile.GetTag(),
		INDEX_NONE,
		MoveTemp(Metadata)),
		GetCommandId(Context));
}

FSGSStatus SGSBasicCardRuleHelpers::DiscardProcessingCard(FSGSRuleExecutionContext& Context)
{
	const int32 ProcessingCardId = Context.Runtime->GetResolutionStack().FindLatestProcessingCardId();
	USGSCard* ProcessingCard = ProcessingCardId != INDEX_NONE ? Context.GameContext->FindCardById(ProcessingCardId) : nullptr;
	const FSGSCardState* ProcessingState = ProcessingCard != nullptr
		? Context.GameContext->FindCardStateById(ProcessingCardId)
		: nullptr;
	if (ProcessingCard == nullptr
		|| ProcessingState == nullptr
		|| !ProcessingState->Zone.MatchesTagExact(SGSGameplayTags::CardZone_Processing.GetTag()))
	{
		return MakeValue();
	}

	return Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ ProcessingCard },
		SGSGameplayTags::CardZone_Processing.GetTag(),
		INDEX_NONE,
		SGSGameplayTags::CardZone_DiscardPile.GetTag(),
		INDEX_NONE,
		{ SGSCardMoveReasons::Cleanup(), {} }),
		GetCommandId(Context));
}

FSGSStatus SGSBasicCardRuleHelpers::CompleteCurrentFrame(FSGSRuleExecutionContext& Context, FName Reason)
{
	return Context.Runtime->CompleteCurrentFrame(Reason);
}

int32 SGSBasicCardRuleHelpers::GetCurrentEffectSourceSeat(const FSGSRuleExecutionContext& Context)
{
	return Context.Runtime->GetResolutionStack().FindLatestEffectSourceSeat();
}

int32 SGSBasicCardRuleHelpers::GetCurrentEffectTargetSeat(const FSGSRuleExecutionContext& Context)
{
	return Context.Runtime->GetResolutionStack().FindLatestEffectTargetSeat();
}

int32 SGSBasicCardRuleHelpers::GetPeachHealTarget(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload)
{
	if (Payload.EffectTargetSeat != INDEX_NONE)
	{
		return Payload.EffectTargetSeat;
	}
	if (Payload.TargetSeatIndices.Num() > 0)
	{
		return Payload.TargetSeatIndices[0];
	}
	return GetCommandSeat(Context);
}

FSGSStatus SGSBasicCardRuleHelpers::ExecuteHealCard(
	FSGSRuleExecutionContext& Context,
	int32 CardId,
	FName CardName,
	int32 HealTargetSeat)
{
	USGSCard* Card = FindPayloadCard(Context, CardId);
	if (Card == nullptr || Card->CardName != CardName || Context.GameContext->GetSeat(HealTargetSeat) == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.InvalidHealCard")),
			TEXT("Heal rule requires the expected card and a valid target."));
	}

	if (FSGSStatus Status = DiscardHandCard(Context, Card, GetCommandSeat(Context)); Status.HasError())
	{
		return Status;
	}

	return Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeHealStep(HealTargetSeat, 1),
		GetCommandId(Context));
}

FSGSStatus SGSBasicCardRuleHelpers::ContinueDyingPeachOrEliminate(FSGSRuleExecutionContext& Context)
{
	FSGSResolutionFrame* DyingFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	FSGSDyingPeachResolutionState* DyingState = DyingFrame != nullptr ? DyingFrame->GetMutableState<FSGSDyingPeachResolutionState>() : nullptr;
	if (DyingFrame == nullptr || DyingState == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.MissingResolutionFrame")),
			TEXT("Dying.Peach requires the current resolution frame to carry DyingPeach state."));
	}

	const int32 DyingSeatIndex = DyingState->DyingSeat;
	const USGSSeat* DyingSeat = Context.GameContext->GetSeat(DyingSeatIndex);
	if (DyingSeat == nullptr || !DyingSeat->bIsAlive || DyingSeat->Health > 0)
	{
		return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.DyingPeachResolved")));
	}

	if (OpenNextDyingPeachResponseWindow(Context, *DyingFrame))
	{
		return MakeValue();
	}

	return EliminateDyingSeatAndFinish(Context, DyingSeatIndex);
}

FSGSStatus SGSBasicCardRuleHelpers::StartDyingPeachResolution(FSGSRuleExecutionContext& Context, int32 DyingSeatIndex)
{
	FSGSResolutionStack& Stack = Context.Runtime->GetResolutionStack();
	if (FSGSResolutionFrame* ExistingDyingFrame = FindLatestDyingFrameForSeat(Stack, DyingSeatIndex))
	{
		if (FSGSDyingPeachResolutionState* ExistingDyingState = ExistingDyingFrame->GetMutableState<FSGSDyingPeachResolutionState>())
		{
			ExistingDyingState->bNeedsHealthRecheck = true;
		}

		return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.DuplicateDyingMarked")));
	}

	FSGSDyingPeachResolutionState DyingState;
	DyingState.DyingSeat = DyingSeatIndex;
	DyingState.NextResponderIndex = 0;

	if (Context.GameContext->GetSeat(DyingSeatIndex) != nullptr)
	{
		DyingState.ResponderSeatIndices.Add(DyingSeatIndex);
	}
	for (int32 SeatIndex = 0; SeatIndex < Context.GameContext->NumSeats(); ++SeatIndex)
	{
		if (SeatIndex == DyingSeatIndex)
		{
			continue;
		}

		const USGSSeat* Seat = Context.GameContext->GetSeat(SeatIndex);
		if (Seat != nullptr && Seat->bIsAlive)
		{
			DyingState.ResponderSeatIndices.Add(SeatIndex);
		}
	}

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = FName(TEXT("SGS.Rule.DyingPeach"));
	Frame.SourceCommandId = GetCommandId(Context);
	Frame.ActorSeat = DyingSeatIndex;
	Frame.SourceSeat = GetCurrentEffectSourceSeat(Context);
	Frame.TargetSeat = DyingSeatIndex;
	Frame.WindowName = DyingPeachWindowName();
	Frame.RequiredCardName = FName(TEXT("Peach"));
	Frame.OnChildCompletedContinuation = SGSResolutionContinuations::ResumeDyingPeach();
	Frame.FrameState = FInstancedStruct::Make(DyingState);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	return ContinueDyingPeachOrEliminate(Context);
}

FSGSStatus SGSBasicCardRuleHelpers::ResolveSlashHit(FSGSRuleExecutionContext& Context)
{
	const FSGSResolutionFrame* SlashFrame = Context.Runtime->GetResolutionStack().GetCurrentFrame();
	const FSGSSlashResolutionState* SlashState = SlashFrame != nullptr
		? SlashFrame->GetState<FSGSSlashResolutionState>()
		: nullptr;
	if (SlashState == nullptr)
	{
		return MakeRuleError(
			FName(TEXT("SGS.Rule.MissingSlashResolution")),
			TEXT("Slash hit requires the current Slash resolution state."));
	}

	const int32 SourceSeat = SlashState->SourceSeat;
	const int32 TargetSeat = SlashState->TargetSeat;
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(
		SGSStandardEffectSteps::MakeDamageStep(SourceSeat, TargetSeat, SlashState->DamageAmount),
		GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}

	FSGSRuleEventPayload DamageAfter;
	DamageAfter.EventTag = SGSGameplayTags::GameEvent_DamageAfter.GetTag();
	DamageAfter.EventName = FName(TEXT("DamageAfter"));
	DamageAfter.SourceSeat = SourceSeat;
	DamageAfter.TargetSeat = TargetSeat;
	DamageAfter.SourceCommandId = GetCommandId(Context);
	DamageAfter.TimingPoint = Context.TimingPoint;
	DamageAfter.TimingPoint.Step = SGSTimingSteps::After();
	DamageAfter.TimingPoint.SubOrder += 1;
	FSGSDamageEventData DamageData;
	DamageData.CardId = SlashState->SlashCardId;
	DamageData.Amount = SlashState->DamageAmount;
	DamageAfter.EventData = FInstancedStruct::Make(DamageData);
	if (FSGSStatus Status = Context.Runtime->PublishTimingEvent(DamageAfter); Status.HasError())
	{
		return Status;
	}

	if (FSGSStatus Status = DiscardProcessingCard(Context); Status.HasError())
	{
		return Status;
	}

	const USGSSeat* DamagedSeat = Context.GameContext->GetSeat(TargetSeat);
	if (DamagedSeat != nullptr && DamagedSeat->bIsAlive && DamagedSeat->Health <= 0)
	{
		return StartDyingPeachResolution(Context, TargetSeat);
	}

	return CompleteCurrentFrame(Context, FName(TEXT("SGS.Resolution.SlashHitResolved")));
}

FSGSStatus SGSBasicCardRuleHelpers::ValidateSlashUse(
	FSGSRuleExecutionContext& Context,
	USGSCard* MaterialCard,
	TConstArrayView<int32> TargetSeatIndices)
{
	if (MaterialCard == nullptr || TargetSeatIndices.Num() != 1)
	{
		return MakeRuleError(FName(TEXT("SGS.Rule.InvalidSlash")), TEXT("Slash requires one material card and one target."));
	}

	const int32 SourceSeat = GetCommandSeat(Context);
	const int32 TargetSeat = TargetSeatIndices[0];
	const USGSSeat* Target = Context.GameContext->GetSeat(TargetSeat);
	if (Target == nullptr || !Target->bIsAlive || TargetSeat == SourceSeat)
	{
		return MakeRuleError(FName(TEXT("SGS.Rule.InvalidTarget")), TEXT("Slash requires one living non-self target."));
	}

	FSGSNumericRuleQuery LimitQuery;
	LimitQuery.QueryName = SGSRuleQueries::SlashUseLimit();
	LimitQuery.ActorSeat = SourceSeat;
	LimitQuery.TargetSeat = TargetSeat;
	LimitQuery.CardName = FName(TEXT("Slash"));
	LimitQuery.BaseValue = 1;
	FSGSRuleQueryContext QueryContext;
	QueryContext.GameContext = Context.GameContext;
	QueryContext.ActiveEffects = Context.ActiveEffects;
	QueryContext.RuleRegistry = Context.RuleRegistry;
	QueryContext.Phase = Context.TimingPoint.Phase;
	QueryContext.ActorSeat = SourceSeat;
	const int32 SlashLimit = Context.RuleRegistry != nullptr
		? Context.RuleRegistry->ApplyNumericModifiers(QueryContext, LimitQuery)
		: LimitQuery.BaseValue;
	if (CountStatus(Context, SourceSeat, SGSGameplayTags::Status_SlashUsed.GetTag()) >= SlashLimit)
	{
		return MakeRuleError(FName(TEXT("SGS.Rule.SlashAlreadyUsed")), TEXT("The acting seat has reached its Slash limit."));
	}

	FSGSNumericRuleQuery DistanceQuery;
	DistanceQuery.QueryName = SGSRuleQueries::SlashTargetDistance();
	DistanceQuery.ActorSeat = SourceSeat;
	DistanceQuery.TargetSeat = TargetSeat;
	DistanceQuery.CardName = FName(TEXT("Slash"));
	DistanceQuery.BaseValue = 1;
	const int32 SlashDistance = Context.RuleRegistry != nullptr
		? Context.RuleRegistry->ApplyNumericModifiers(QueryContext, DistanceQuery)
		: DistanceQuery.BaseValue;
	if (Context.GameContext->GetDistance(SourceSeat, TargetSeat) > SlashDistance)
	{
		return MakeRuleError(FName(TEXT("SGS.Rule.TargetOutOfDistance")), TEXT("Slash target is out of distance."));
	}

	return MakeValue();
}

FSGSStatus SGSBasicCardRuleHelpers::ExecuteSlashUse(
	FSGSRuleExecutionContext& Context,
	USGSCard* MaterialCard,
	TConstArrayView<int32> TargetSeatIndices,
	FName SourceRuleName)
{
	const int32 SourceSeat = GetCommandSeat(Context);
	const int32 TargetSeat = TargetSeatIndices[0];
	if (FSGSStatus Status = Context.Runtime->RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ MaterialCard },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		SourceSeat,
		SGSGameplayTags::CardZone_Processing.GetTag(),
		INDEX_NONE,
		{ SGSCardMoveReasons::Use(), { TargetSeat } }),
		GetCommandId(Context));
		Status.HasError())
	{
		return Status;
	}

	FSGSSlashResolutionState SlashState;
	SlashState.SlashCardId = MaterialCard->CardId;
	SlashState.SourceSeat = SourceSeat;
	SlashState.TargetSeat = TargetSeat;
	SlashState.DamageAmount = ConsumeStatus(Context, SourceSeat, SGSGameplayTags::Status_AnalepticBoost.GetTag()) ? 2 : 1;
	FSGSNumericRuleQuery ResponseCountQuery;
	ResponseCountQuery.QueryName = SGSRuleQueries::RequiredResponseCount();
	ResponseCountQuery.ActorSeat = SourceSeat;
	ResponseCountQuery.TargetSeat = TargetSeat;
	ResponseCountQuery.CardName = FName(TEXT("Dodge"));
	ResponseCountQuery.BaseValue = 1;
	FSGSRuleQueryContext ResponseQueryContext;
	ResponseQueryContext.GameContext = Context.GameContext;
	ResponseQueryContext.ActiveEffects = Context.ActiveEffects;
	ResponseQueryContext.RuleRegistry = Context.RuleRegistry;
	ResponseQueryContext.Phase = Context.TimingPoint.Phase;
	ResponseQueryContext.ActorSeat = SourceSeat;
	SlashState.RequiredDodgeCount = Context.RuleRegistry != nullptr
		? Context.RuleRegistry->ApplyNumericModifiers(ResponseQueryContext, ResponseCountQuery)
		: 1;
	AddStatus(
		Context,
		SourceSeat,
		FName(TEXT("SGS.ActiveEffect.SlashUsed")),
		SGSGameplayTags::Status_SlashUsed.GetTag(),
		FSGSDurationSpec::ThisPhase(SourceSeat, Context.TimingPoint));

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = SourceRuleName;
	Frame.SourceCommandId = GetCommandId(Context);
	Frame.ActorSeat = SourceSeat;
	Frame.SourceSeat = SourceSeat;
	Frame.TargetSeat = TargetSeat;
	Frame.ProcessingCardId = MaterialCard->CardId;
	Frame.OnChildCompletedContinuation = SGSResolutionContinuations::FinishParentCardResolution();
	Frame.FrameState = FInstancedStruct::Make(SlashState);
	Context.Runtime->PushResolutionFrame(MoveTemp(Frame));

	FSGSRuleResponseWindowSpec WindowSpec;
	WindowSpec.SeatIndex = TargetSeat;
	WindowSpec.WindowName = SlashDodgeWindowName();
	WindowSpec.RequiredCardName = FName(TEXT("Dodge"));
	WindowSpec.AcceptedCardNames.Add(FName(TEXT("Dodge")));
	WindowSpec.ContextName = FName(TEXT("Slash"));
	WindowSpec.EffectSourceSeat = SourceSeat;
	WindowSpec.EffectTargetSeat = TargetSeat;
	if (Context.Runtime->OpenResponseWindow(WindowSpec))
	{
		return MakeValue();
	}

	return ResolveSlashHit(Context);
}
