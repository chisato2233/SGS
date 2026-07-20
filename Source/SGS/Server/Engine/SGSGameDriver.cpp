#include "Server/Engine/SGSGameDriver.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Cards/SGSStandardCardDefinitions.h"
#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/AI/SGSBasicAIAgent.h"
#include "Shared/Core/SGSLogChannels.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Rules/BasicCards/SGSBasicCardRuleHelpers.h"
#include "Server/Rules/Actions/BasicCards/SGSSlashRule.h"
#include "Server/Rules/Responses/BasicCards/SGSDyingPeachRules.h"
#include "Server/Rules/Resolution/SGSDamageResolution.h"
#include "Server/Rules/Resolution/SGSJudgementResolution.h"
#include "Server/Rules/Resolution/SGSLordAssistResolution.h"
#include "Server/Rules/Phases/SGSDiscardPhaseRules.h"
#include "Server/Rules/Resolution/SGSGeneralSelectionResolution.h"
#include "Server/Rules/Resolution/SGSTimingEventResolution.h"
#include "Server/Rules/StandardCards/SGSDelayedTrickRules.h"
#include "Server/Rules/StandardCards/SGSEquipmentRules.h"
#include "Server/Rules/StandardCards/SGSStandardTrickRules.h"
#include "Server/Skills/SGSSkillDefinition.h"
#include "Server/Skills/SGSStandardSkillRules.h"
#include "Server/Content/SGSStandardGenerals.h"

class FSGSDriverRuleRuntime final : public ISGSRuleRuntime
{
public:
	explicit FSGSDriverRuleRuntime(USGSGameDriver& InDriver)
		: Driver(InDriver)
	{
	}

	virtual FSGSStatus RunEffectStep(FSGSEffectStep Step, FSGSCommandId CommandId) override
	{
		return Driver.RunEffectStep(MoveTemp(Step), CommandId);
	}

	virtual bool OpenResponseWindow(const FSGSRuleResponseWindowSpec& Spec) override
	{
		USGSSeat* Target = Driver.Context != nullptr ? Driver.Context->GetSeat(Spec.SeatIndex) : nullptr;
		ISGSDecisionAgent* Agent = Target != nullptr ? Target->DecisionAgent.GetInterface() : nullptr;
		if (Agent == nullptr)
		{
			return false;
		}

		if (FSGSStatus Status = Driver.ResolutionStack.OpenResponseWindowOnCurrent(
			Spec.WindowName,
			Spec.RequiredCardName,
			Spec.AcceptedCardNames,
			Spec.EffectSourceSeat,
			Spec.EffectTargetSeat);
			Status.HasError())
		{
			UE_LOG(LogSGSTurn, Error, TEXT("Open response window failed: %s"), *Status.GetError().ToLogString());
			return false;
		}
		if (FSGSResolutionFrame* Frame = Driver.ResolutionStack.GetCurrentFrame())
		{
			Frame->bIsCardChoice = Spec.bIsCardChoice;
			Frame->ChoiceName = Spec.ChoiceName;
			Frame->MinChoiceCount = Spec.MinChoiceCount;
			Frame->MaxChoiceCount = Spec.MaxChoiceCount;
			Frame->CardChoiceOptions = Spec.CardChoiceOptions;
			Frame->CandidateCardSelections = Spec.CandidateCardSelections;
			Frame->bIsOptionChoice = Spec.bIsOptionChoice;
			Frame->NamedOptions = Spec.NamedOptions;
		}

		FSGSResponseRequest Request = Driver.MakeResponseRequest(
			Spec.SeatIndex,
			Spec.WindowName,
			Spec.RequiredCardName,
			Spec.ContextName,
			Spec.EffectSourceSeat,
			Spec.EffectTargetSeat,
			Spec.SkillOptions,
			Spec.AcceptedCardNames,
			Spec.bAllowPass);
		Request.bIsCardChoice = Spec.bIsCardChoice;
		Request.ChoiceName = Spec.ChoiceName;
		Request.MinChoiceCount = Spec.MinChoiceCount;
		Request.MaxChoiceCount = Spec.MaxChoiceCount;
		Request.CardChoiceOptions = Spec.CardChoiceOptions;
		Request.CandidateCardSelections = Spec.CandidateCardSelections;
		Request.bIsOptionChoice = Spec.bIsOptionChoice;
		Request.NamedOptions = Spec.NamedOptions;
		Driver.PendingCommandId = Request.CommandId;
		Driver.CurrentSeatIndex = Spec.SeatIndex;
		Driver.bWaitingForDecision = true;
		Driver.DeferResponseRequest(Request, Target->DecisionAgent);
		return true;
	}

	virtual void AdvanceAfterPhase() override
	{
		Driver.PendingCommandId = FSGSCommandId();
		Driver.AdvanceAfterPhase();
	}

	virtual FSGSStableHandle PushResolutionFrame(FSGSResolutionFrame Frame) override
	{
		return Driver.ResolutionStack.PushFrame(MoveTemp(Frame));
	}

	virtual FSGSStatus CompleteCurrentFrame(FName Reason) override
	{
		return Driver.FinishCurrentResolution(Reason);
	}

	virtual FSGSStatus AbortAllFrames(FName Reason) override
	{
		Driver.PendingCommandId = FSGSCommandId();
		Driver.ClearDeferredResponseRequest();
		return Driver.ResolutionStack.AbortAllFrames(Reason);
	}

	virtual FSGSResolutionStack& GetResolutionStack() override
	{
		return Driver.ResolutionStack;
	}

	virtual const FSGSResolutionStack& GetResolutionStack() const override
	{
		return Driver.ResolutionStack;
	}

	virtual FSGSStatus PublishTimingEvent(const FSGSRuleEventPayload& Payload) override
	{
		return Driver.PublishTimingEvent(Payload);
	}

	virtual FSGSStatus ContinueTimingEventDispatch() override
	{
		return Driver.ContinueTimingEventDispatch(true);
	}

	virtual FSGSStatus ContinueGeneralSelection() override
	{
		return Driver.ContinueGeneralSelection();
	}

	virtual void RequestCurrentPhaseResume() override
	{
		Driver.bResumeCurrentPhaseAfterResolution = true;
	}

private:
	USGSGameDriver& Driver;
};

void USGSGameDriver::StartGame(const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents, int32 RandomSeed)
{
	FSGSGameStartConfig Config;
	Config.RandomSeed = RandomSeed;
	StartGame(InAgents, Config);
}

void USGSGameDriver::StartGame(const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents, const FSGSGameStartConfig& Config)
{
	++MotionEpoch;
	ReplayLog.Reset(MotionEpoch);
	Context = NewObject<USGSGameContext>(this);
	Context->Initialize(
		InAgents,
		Config.RandomSeed,
		Config.bIdentityMode,
		Config.GeneralIdsBySeat,
		Config.FactionsBySeat);

	if (Context->NumSeats() == 0)
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("StartGame: no seats provided; nothing to run."));
		bGameOver = true;
		return;
	}

	bGameOver = false;
	bWaitingForDecision = false;
	PhaseProgress = ESGSPhaseProgress::Before;
	bResumeCurrentPhaseAfterResolution = false;
	bIdentityMode = Config.bIdentityMode;
	GameResult = FSGSGameResult();
	TurnsPlayed = 0;
	CurrentSeatIndex = 0;
	TurnSeatIndex = INDEX_NONE;
	PendingRequestId = 0;
	PendingCommandId = FSGSCommandId();
	NextCommandIdValue = 0;
	NextTimingSequence = 0;
	DeferredResponseRequest = FSGSResponseRequest();
	DeferredResponseAgent = TScriptInterface<ISGSDecisionAgent>();
	bHasDeferredResponseRequest = false;
	CurrentMaxTurns = bIdentityMode ? 0 : FMath::Max(Config.MaxTurns, 1);
	CurrentStartingHandSize = FMath::Max(Config.StartingHandSize, 0);
	PendingInitialDeck = Config.InitialDeck;
	bPendingShuffleInitialDeck = Config.bShuffleInitialDeck;
	PendingInitialHealthBySeat = Config.InitialHealthBySeat;
	CommandRouter.Reset();
	AIEvaluatorRegistry.Reset();
	SGSBasicAIEvaluators::Register(AIEvaluatorRegistry);
	SGSStandardAISemantics::Register(AIEvaluatorRegistry);
	RuleRegistry.Reset();
	SGSStandardSkills::Register(RuleRegistry, AIEvaluatorRegistry);
	ResolutionStack.Reset();
	EffectPipeline.Reset();
	ActiveEffectTimeline.Reset();
	Context->OnSeatEliminated().AddUObject(this, &USGSGameDriver::HandleSeatEliminated);
	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		USGSSeat* Seat = Context->GetSeat(SeatIndex);
		if (USGSBasicAIAgent* Agent = Cast<USGSBasicAIAgent>(Seat->DecisionAgent.GetObject()))
		{
			Agent->BindToSeat(Context, SeatIndex, &AIEvaluatorRegistry, Config.BasicAIThinkDelaySeconds);
		}
	}

	if (Config.bChooseGenerals && Config.GeneralIdsBySeat.IsEmpty())
	{
		BeginGeneralSelection();
		return;
	}

	CompleteInitialSetup();
	Pump();
}

void USGSGameDriver::BeginGeneralSelection()
{
	FSGSGeneralSelectionResolutionState State;
	State.AvailableGeneralIds = SGSStandardGenerals::FirstIdentityRoster();

	int32 LordSeatIndex = 0;
	if (bIdentityMode)
	{
		for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
		{
			if (Context->GetSeat(SeatIndex)->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag()))
			{
				LordSeatIndex = SeatIndex;
				break;
			}
		}
	}
	for (int32 Offset = 0; Offset < Context->NumSeats(); ++Offset)
	{
		State.SeatOrder.Add((LordSeatIndex + Offset) % Context->NumSeats());
	}

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = SGSGeneralSelectionResolution::FrameRuleName();
	Frame.SourceCommandId = AllocateCommandId();
	Frame.ActorSeat = LordSeatIndex;
	Frame.SourceSeat = LordSeatIndex;
	Frame.TargetSeat = LordSeatIndex;
	Frame.FrameState.InitializeAs<FSGSGeneralSelectionResolutionState>(MoveTemp(State));
	ResolutionStack.PushFrame(MoveTemp(Frame));
	CurrentPhase = SGSGameplayTags::Phase_None.GetTag();
	CurrentSeatIndex = LordSeatIndex;

	if (FSGSStatus Status = ContinueGeneralSelection(); Status.HasError())
	{
		UE_LOG(LogSGSTurn, Error, TEXT("General selection failed: %s"), *Status.GetError().ToLogString());
		return;
	}
	DispatchDeferredResponseRequest();
}

FSGSStatus USGSGameDriver::ContinueGeneralSelection()
{
	FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame();
	FSGSGeneralSelectionResolutionState* State = Frame != nullptr
		? Frame->GetMutableState<FSGSGeneralSelectionResolutionState>()
		: nullptr;
	check(State != nullptr);

	if (!State->SeatOrder.IsValidIndex(State->NextSeatIndex))
	{
		TSGSResult<FSGSResolutionFrame> Completed = ResolutionStack.CompleteCurrentFrame(
			FName(TEXT("SGS.GeneralSelection.Complete")));
		if (!Completed.HasValue())
		{
			return MakeError(Completed.GetError());
		}
		PendingCommandId = FSGSCommandId();
		ClearDeferredResponseRequest();
		CompleteInitialSetup();
		return MakeValue();
	}

	const int32 SeatIndex = State->SeatOrder[State->NextSeatIndex];
	FSGSRuleResponseWindowSpec Spec;
	Spec.SeatIndex = SeatIndex;
	Spec.WindowName = SGSGeneralSelectionResolution::WindowName();
	Spec.ContextName = SGSGeneralSelectionResolution::ChoiceName();
	Spec.EffectSourceSeat = SeatIndex;
	Spec.EffectTargetSeat = SeatIndex;
	Spec.bAllowPass = false;
	Spec.bIsOptionChoice = true;
	Spec.ChoiceName = SGSGeneralSelectionResolution::ChoiceName();
	for (const FName GeneralId : State->AvailableGeneralIds)
	{
		const FSGSGeneralDefinition* Definition = SGSStandardGenerals::Find(GeneralId);
		if (Definition != nullptr)
		{
			FSGSDecisionNamedOption& Option = Spec.NamedOptions.AddDefaulted_GetRef();
			Option.OptionName = GeneralId;
			Option.DisplayName = Definition->DisplayName;
		}
	}

	FSGSDriverRuleRuntime Runtime(*this);
	if (Runtime.OpenResponseWindow(Spec))
	{
		return MakeValue();
	}
	return MakeError(FSGSError::Make(
		FName(TEXT("SGS.GeneralSelection.MissingAgent")),
		TEXT("The current general-selection seat has no decision agent.")));
}

void USGSGameDriver::CompleteInitialSetup()
{
	for (const TPair<int32, int32>& HealthOverride : PendingInitialHealthBySeat)
	{
		if (USGSSeat* Seat = Context->GetSeat(HealthOverride.Key))
		{
			Seat->Health = FMath::Clamp(HealthOverride.Value, 0, Seat->MaxHealth);
		}
	}

	BuildInitialDeck(PendingInitialDeck, bPendingShuffleInitialDeck);
	PendingInitialDeck.Reset();
	PendingInitialHealthBySeat.Reset();

	// 发起始手牌（牌堆为空时不发牌；牌库数据接入后自然生效）。
	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		CurrentSeatIndex = SeatIndex;
		EffectPipeline.Reset();
		FSGSCardMoveEventMetadata Metadata;
		Metadata.Reason = SGSCardMoveReasons::InitialDeal();
		EffectPipeline.Enqueue(SGSStandardEffectSteps::MakeDrawCardsStep(SeatIndex, CurrentStartingHandSize, MoveTemp(Metadata)));
		FSGSEffectContext EffectContext = MakeEffectContext();
		if (FSGSStatus Status = EffectPipeline.Run(EffectContext); Status.HasError())
		{
			UE_LOG(LogSGSTurn, Error, TEXT("Starting hand effect pipeline failed: %s"), *Status.GetError().ToLogString());
		}
		SyncReplayLog();
	}
	CurrentSeatIndex = 0;
	if (bIdentityMode)
	{
		for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
		{
			if (Context->GetSeat(SeatIndex)->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag()))
			{
				CurrentSeatIndex = SeatIndex;
				break;
			}
		}
	}

	Broadcast(SGSGameplayTags::GameEvent_GameStarted.GetTag());
	BeginTurn();
}

void USGSGameDriver::BeginTurn()
{
	TurnSeatIndex = CurrentSeatIndex;
	CurrentPhase = SGSGameplayTags::Phase_RoundStart.GetTag();
	PhaseProgress = ESGSPhaseProgress::Before;
	UE_LOG(LogSGSTurn, Log, TEXT("Turn %d begins for seat %d."), TurnsPlayed, CurrentSeatIndex);
	Broadcast(SGSGameplayTags::GameEvent_TurnBegan.GetTag());
}

void USGSGameDriver::Pump()
{
	// 重入保护：同步应答会在 OnPlayActionDecided 内再次调用 Pump；此时直接返回，
	// 让最外层的 Pump 循环继续推进。
	if (bPumping)
	{
		return;
	}

	bPumping = true;
	while (!bGameOver && !bWaitingForDecision)
	{
		EnterCurrentPhase();
	}
	bPumping = false;
}

void USGSGameDriver::EnterCurrentPhase()
{
	if (PhaseProgress == ESGSPhaseProgress::Before)
	{
		PhaseProgress = ESGSPhaseProgress::Begin;
		PublishPhaseTiming(SGSGameplayTags::GameEvent_PhaseBefore.GetTag(), SGSTimingSteps::Before());
		return;
	}
	if (PhaseProgress == ESGSPhaseProgress::Begin)
	{
		PhaseProgress = ESGSPhaseProgress::BroadcastBegin;
		PublishPhaseTiming(SGSGameplayTags::GameEvent_PhaseBegin.GetTag(), SGSTimingSteps::Begin());
		return;
	}
	if (PhaseProgress == ESGSPhaseProgress::BroadcastBegin)
	{
		Broadcast(SGSGameplayTags::GameEvent_PhaseBegan.GetTag());
		PhaseProgress = ESGSPhaseProgress::Content;
		return;
	}
	if (PhaseProgress == ESGSPhaseProgress::End)
	{
		PhaseProgress = ESGSPhaseProgress::BroadcastEnd;
		PublishPhaseTiming(SGSGameplayTags::GameEvent_PhaseEnd.GetTag(), SGSTimingSteps::End());
		return;
	}
	if (PhaseProgress == ESGSPhaseProgress::BroadcastEnd)
	{
		Broadcast(SGSGameplayTags::GameEvent_PhaseEnded.GetTag());
		PhaseProgress = ESGSPhaseProgress::After;
		return;
	}
	if (PhaseProgress == ESGSPhaseProgress::After)
	{
		PhaseProgress = ESGSPhaseProgress::Commit;
		PublishPhaseTiming(SGSGameplayTags::GameEvent_PhaseAfter.GetTag(), SGSTimingSteps::After());
		return;
	}
	if (PhaseProgress == ESGSPhaseProgress::Commit)
	{
		CommitPhaseAdvance();
		return;
	}

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Draw))
	{
		if (HasStatusEffect(CurrentSeatIndex, SGSGameplayTags::Status_SkipDrawPhase.GetTag()))
		{
			ExpireStatusEffects(CurrentSeatIndex, SGSGameplayTags::Status_SkipDrawPhase.GetTag());
			AdvanceAfterPhase();
			return;
		}
		ExecuteDrawPhaseThroughPipeline();
		AdvanceAfterPhase();
		return;
	}

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Play))
	{
		if (HasStatusEffect(CurrentSeatIndex, SGSGameplayTags::Status_SkipPlayPhase.GetTag()))
		{
			ExpireStatusEffects(CurrentSeatIndex, SGSGameplayTags::Status_SkipPlayPhase.GetTag());
			AdvanceAfterPhase();
			return;
		}
		FSGSPlayPhaseRequest Request = MakePlayPhaseRequest();
		PendingCommandId = Request.CommandId;
		ResolutionStack.Reset();

		const USGSSeat* Seat = Context->GetSeat(CurrentSeatIndex);
		ISGSDecisionAgent* Agent = Seat != nullptr ? Seat->DecisionAgent.GetInterface() : nullptr;
		if (Agent == nullptr)
		{
			UE_LOG(LogSGSTurn, Warning, TEXT("Seat %d has no decision agent; treating as pass."), CurrentSeatIndex);
			const FSGSCommand FallbackCommand = MakeFallbackPassCommand(TEXT("NoDecisionAgent"));
			CommandRouter.RecordLifecycle(
				FallbackCommand,
				SGSCommandLifecycle::Fallbacked(),
				true,
				FSGSError(),
				TEXT("Seat has no decision agent."));
			if (!ResolveCommandThroughRules(FallbackCommand))
			{
				PendingCommandId = FSGSCommandId();
				AdvanceAfterPhase();
			}
			return;
		}

		bWaitingForDecision = true;

		FSGSPlayPhaseDecisionDelegate OnDecided;
		OnDecided.BindUObject(this, &USGSGameDriver::OnPlayActionDecided);

		// 异步：代理可能同步（AI）或跨帧（真人）回调 OnPlayActionDecided。
		Agent->RequestPlayPhaseAction(Request, OnDecided);
		return;
	}

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Judge))
	{
		ExecuteJudgementPhaseThroughRules();
		return;
	}

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Discard))
	{
		ExecuteDiscardPhase();
		return;
	}

	// 准备和结束阶段的内容由稳定阶段时机上的 TriggerRule 扩展。
	AdvanceAfterPhase();
}

void USGSGameDriver::OnPlayActionDecided(const FSGSPlayPhaseDecision& Decision)
{
	if (!bWaitingForDecision)
	{
		// 过期/重复应答（例如超时已托管后迟到的真人应答）。
		UE_LOG(LogSGSNet, Warning, TEXT("Ignoring stray play-phase decision."));
		return;
	}

	bWaitingForDecision = false;
	if (!ResolveCommandThroughRules(Decision.Command))
	{
		const FSGSCommand FallbackCommand = MakeFallbackPassCommand(TEXT("RuleFailure"));
		if (!ResolveCommandThroughRules(FallbackCommand))
		{
			PendingCommandId = FSGSCommandId();
			AdvanceAfterPhase();
		}
	}
	Pump();
}

void USGSGameDriver::OnResponseActionDecided(const FSGSResponseDecision& Decision)
{
	if (!bWaitingForDecision)
	{
		UE_LOG(LogSGSNet, Warning, TEXT("Ignoring stray response decision."));
		return;
	}

	bWaitingForDecision = false;
	if (!ResolveCommandThroughRules(Decision.Command))
	{
		if (!ResolveCommandThroughRules(MakeFallbackPassCommand(TEXT("InvalidResponseCommand"))))
		{
			FinishCurrentResolution();
		}
	}
	Pump();
}

bool USGSGameDriver::ResolveCommandThroughRules(const FSGSCommand& Command)
{
	const FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	TSGSResult<FSGSCommand> CommandResult = SubmitCommandWithFallback(Command, ExecutionContext);
	if (!CommandResult.HasValue())
	{
		return false;
	}
	const FSGSCommand ValidatedCommand = CommandResult.GetValue();

	TSGSResult<FSGSRuleInvocation> RuleInvocationResult = CommandRouter.BuildRuleInvocation(ValidatedCommand, ExecutionContext);
	if (!RuleInvocationResult.HasValue())
	{
		CommandRouter.RecordLifecycle(
			ValidatedCommand,
			SGSCommandLifecycle::Rejected(),
			false,
			RuleInvocationResult.GetError(),
			TEXT("Rule invocation build failed."));
		SyncReplayLog();
		return false;
	}

	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &ValidatedCommand;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::Resolve());
	RuleContext.RuleInvocation = RuleInvocationResult.GetValue();
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;

	if (FSGSStatus Status = RuleRegistry.Resolve(RuleContext); Status.HasError())
	{
		CommandRouter.RecordLifecycle(
			ValidatedCommand,
			SGSCommandLifecycle::Rejected(),
			false,
			Status.GetError(),
			TEXT("Rule execution failed."));
		SyncReplayLog();
		return false;
	}
	PublishAIObservation(RuleContext.RuleInvocation);

	CommandRouter.RecordLifecycle(
		ValidatedCommand,
		SGSCommandLifecycle::Executed(),
		true,
		FSGSError(),
		RuleContext.RuleInvocation.ToLogString());
	SyncReplayLog();
	DispatchDeferredResponseRequest();
	return true;
}

TSGSResult<FSGSCommand> USGSGameDriver::SubmitCommandWithFallback(
	const FSGSCommand& Command,
	const FSGSCommandExecutionContext& ExecutionContext)
{
	if (FSGSStatus Status = CommandRouter.SubmitCommand(Command, ExecutionContext); Status.HasError())
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("Invalid command: %s. Falling back to pass."),
			*Status.GetError().ToLogString());

		const FSGSCommand FallbackCommand = MakeFallbackPassCommand(TEXT("InvalidCommand"));
		CommandRouter.RecordLifecycle(
			FallbackCommand,
			SGSCommandLifecycle::Fallbacked(),
			true,
			Status.GetError(),
			TEXT("Server-authority fallback pass."));
		if (FSGSStatus FallbackStatus = CommandRouter.SubmitCommand(FallbackCommand, ExecutionContext); FallbackStatus.HasError())
		{
			UE_LOG(LogSGSTurn, Error, TEXT("Fallback pass command failed: %s"), *FallbackStatus.GetError().ToLogString());
			SyncReplayLog();
			return MakeError(FallbackStatus.GetError());
		}
		SyncReplayLog();
		return MakeValue(FallbackCommand);
	}

	SyncReplayLog();
	return MakeValue(Command);
}

FSGSCommandExecutionContext USGSGameDriver::MakeCommandExecutionContext() const
{
	FSGSCommandExecutionContext ExecutionContext;
	ExecutionContext.GameContext = Context;
	ExecutionContext.ExpectedCommandId = PendingCommandId;
	ExecutionContext.ExpectedRequestId = PendingRequestId;
	ExecutionContext.ExpectedSeatIndex = CurrentSeatIndex;
	ExecutionContext.ExpectedPhase = CurrentPhase;
	if (const FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame())
	{
		ExecutionContext.ExpectedWindowName = Frame->WindowName;
		ExecutionContext.RequiredCardName = Frame->RequiredCardName;
		ExecutionContext.AcceptedCardNames = Frame->AcceptedCardNames;
		ExecutionContext.EffectSourceSeatIndex = Frame->SourceSeat;
		ExecutionContext.EffectTargetSeatIndex = Frame->TargetSeat;
		ExecutionContext.bIsCardChoice = Frame->bIsCardChoice;
		ExecutionContext.ExpectedChoiceName = Frame->ChoiceName;
		ExecutionContext.MinChoiceCount = Frame->MinChoiceCount;
		ExecutionContext.MaxChoiceCount = Frame->MaxChoiceCount;
		for (const FSGSDecisionCardChoiceOption& Option : Frame->CardChoiceOptions)
		{
			ExecutionContext.SelectableChoiceCardIds.Add(Option.CardId);
		}
		ExecutionContext.bIsOptionChoice = Frame->bIsOptionChoice;
		for (const FSGSDecisionNamedOption& Option : Frame->NamedOptions)
		{
			ExecutionContext.SelectableNamedOptions.Add(Option.OptionName);
		}
	}
	return ExecutionContext;
}

FSGSCommand USGSGameDriver::MakeFallbackPassCommand(FName Reason) const
{
	FSGSCommandBuildRequest Request;
	Request.CommandId = PendingCommandId;
	Request.RequestId = PendingRequestId;
	Request.SeatIndex = CurrentSeatIndex;
	Request.Phase = CurrentPhase;
	Request.SourceChannel = FName(TEXT("ServerFallback"));
	Request.SourceName = Reason;
	return FSGSCommandFactory::Make(Request, FSGSPassCommandPayload());
}

FSGSEffectContext USGSGameDriver::MakeEffectContext(FSGSCommandId CommandId)
{
	FSGSEffectContext EffectContext;
	EffectContext.GameContext = Context;
	EffectContext.ReplayLog = &ReplayLog;
	EffectContext.ActiveEffects = &ActiveEffectTimeline;
	EffectContext.CommandId = CommandId;
	EffectContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::Resolve());
	return EffectContext;
}

FSGSTimingPoint USGSGameDriver::MakeCurrentTimingPoint(FName Step)
{
	return FSGSTimingPoint::Make(
		NextTimingSequence++,
		TurnsPlayed,
		CurrentSeatIndex,
		TurnsPlayed * 100 + CurrentSeatIndex,
		CurrentPhase,
		Step);
}

void USGSGameDriver::BuildInitialDeck(const TArray<FSGSDeckCardSpec>& InitialDeck, bool bShuffle)
{
	if (Context == nullptr)
	{
		return;
	}

	for (const FSGSDeckCardSpec& Spec : InitialDeck)
	{
		const int32 Count = FMath::Max(Spec.Count, 0);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Context->CreateCard(Spec.CardName, Spec.Suit, Spec.Number, Spec.CardType);
		}
	}

	if (bShuffle && InitialDeck.Num() > 0)
	{
		Context->ShuffleDrawPile();
	}
}

FSGSPlayPhaseRequest USGSGameDriver::MakePlayPhaseRequest()
{
	FSGSPlayPhaseRequest Request;
	Request.CommandId = AllocateCommandId();
	Request.SeatIndex = CurrentSeatIndex;
	Request.RequestId = ++PendingRequestId;
	Request.Phase = CurrentPhase;
	Request.bAllowPass = true;

	FSGSCardQuery HandQuery;
	HandQuery.Zone = SGSGameplayTags::CardZone_Hand.GetTag();
	HandQuery.SeatIndex = CurrentSeatIndex;
	HandQuery.ViewerSeatIndex = CurrentSeatIndex;
	const FSGSCardQueryResult HandCards = Context->QueryCards(HandQuery);

	FSGSSeatQuery SlashTargetQuery;
	SlashTargetQuery.SourceSeat = CurrentSeatIndex;
	FSGSRuleQueryContext RuleQueryContext;
	RuleQueryContext.GameContext = Context;
	RuleQueryContext.ActiveEffects = &ActiveEffectTimeline;
	RuleQueryContext.RuleRegistry = &RuleRegistry;
	RuleQueryContext.Phase = CurrentPhase;
	RuleQueryContext.ActorSeat = CurrentSeatIndex;
	FSGSNumericRuleQuery SlashDistanceQuery;
	SlashDistanceQuery.QueryName = SGSRuleQueries::SlashTargetDistance();
	SlashDistanceQuery.ActorSeat = CurrentSeatIndex;
	SlashDistanceQuery.CardName = FName(TEXT("Slash"));
	SlashDistanceQuery.BaseValue = 1;
	SlashTargetQuery.MaxDistance = RuleRegistry.ApplyNumericModifiers(RuleQueryContext, SlashDistanceQuery);
	const FSGSSeatQueryResult SlashTargets = Context->QuerySeats(SlashTargetQuery);
	auto HasStatus = [this](FGameplayTag StatusTag)
	{
		for (const FSGSStableHandle Handle : ActiveEffectTimeline.QueryByTag(StatusTag))
		{
			const FSGSActiveEffect* Effect = ActiveEffectTimeline.Find(Handle);
			if (Effect != nullptr && Effect->OwnerSeat == CurrentSeatIndex)
			{
				return true;
			}
		}
		return false;
	};
	const int32 SlashUsedCount = ActiveEffectTimeline.QueryByTag(SGSGameplayTags::Status_SlashUsed.GetTag()).FilterByPredicate(
		[this](FSGSStableHandle Handle)
		{
			const FSGSActiveEffect* Effect = ActiveEffectTimeline.Find(Handle);
			return Effect != nullptr && Effect->OwnerSeat == CurrentSeatIndex;
		}).Num();
	FSGSNumericRuleQuery SlashLimitQuery;
	SlashLimitQuery.QueryName = SGSRuleQueries::SlashUseLimit();
	SlashLimitQuery.ActorSeat = CurrentSeatIndex;
	SlashLimitQuery.CardName = FName(TEXT("Slash"));
	SlashLimitQuery.BaseValue = 1;
	const int32 SlashUseLimit = RuleRegistry.ApplyNumericModifiers(RuleQueryContext, SlashLimitQuery);
	const bool bCanUseSlash = SlashUsedCount < SlashUseLimit;
	FSGSNumericRuleQuery SlashTargetCountQuery;
	SlashTargetCountQuery.QueryName = SGSRuleQueries::SlashTargetCount();
	SlashTargetCountQuery.ActorSeat = CurrentSeatIndex;
	SlashTargetCountQuery.CardName = FName(TEXT("Slash"));
	SlashTargetCountQuery.BaseValue = 1;
	const int32 SlashTargetCount = RuleRegistry.ApplyNumericModifiers(RuleQueryContext, SlashTargetCountQuery);
	const bool bAnalepticUsed = HasStatus(SGSGameplayTags::Status_AnalepticUsed.GetTag());

	for (const FSGSCardTarget& CardTarget : HandCards.Targets)
	{
		if (CardTarget.Card == nullptr)
		{
			continue;
		}

		FSGSCardActionOption Option;
		Option.CardId = CardTarget.CardId;
		Option.CardName = CardTarget.Card->CardName;
		if (Option.CardName == TEXT("Slash") && bCanUseSlash)
		{
			for (const FSGSSeatTarget& SeatTarget : SlashTargets.Targets)
			{
				Option.TargetSeatIndices.Add(SeatTarget.SeatIndex);
			}
		}
		else if (Option.CardName == TEXT("Peach"))
		{
			const USGSSeat* Seat = Context->GetSeat(CurrentSeatIndex);
			if (Seat != nullptr && Seat->Health < Seat->MaxHealth)
			{
				Option.TargetSeatIndices.Add(CurrentSeatIndex);
			}
		}
		bool bAddOption = (Option.CardName == TEXT("Slash") && bCanUseSlash)
			|| (Option.CardName == TEXT("Peach") && !Option.TargetSeatIndices.IsEmpty())
			|| (Option.CardName == TEXT("Analeptic") && !bAnalepticUsed);
		if (!bAddOption)
		{
			const FSGSStandardCardDefinition* Definition = SGSStandardCardDefinitions::Find(Option.CardName);
			if (Definition != nullptr
				&& Definition->bPlayable
				&& (Definition->CardType.MatchesTagExact(SGSGameplayTags::CardType_Trick.GetTag())
					|| Definition->CardType.MatchesTagExact(SGSGameplayTags::CardType_Equipment.GetTag())))
			{
				if (Definition->TargetMode == SGSCardTargetModes::Self())
				{
					if (!Definition->bDelayedTrick
						|| SGSStandardTrickRules::IsLegalExplicitTarget(
							*Context, CurrentSeatIndex, Option.CardName, CurrentSeatIndex))
					{
						Option.TargetSeatIndices.Add(CurrentSeatIndex);
					}
				}
				else if (Definition->TargetMode == SGSCardTargetModes::Other())
				{
					FSGSSeatQuery TargetQuery;
					TargetQuery.SourceSeat = CurrentSeatIndex;
					TargetQuery.MaxDistance = Definition->MaxDistance;
					for (const FSGSSeatTarget& Target : Context->QuerySeats(TargetQuery).Targets)
					{
						if (SGSStandardTrickRules::IsLegalExplicitTarget(
							*Context, CurrentSeatIndex, Option.CardName, Target.SeatIndex))
						{
							Option.TargetSeatIndices.Add(Target.SeatIndex);
						}
					}
				}
				bAddOption = (Definition->TargetMode != SGSCardTargetModes::Other()
					&& Definition->TargetMode != SGSCardTargetModes::Self())
					|| !Option.TargetSeatIndices.IsEmpty();
			}
		}
		if (bAddOption)
		{
			if (Option.CardName == TEXT("Slash") && !Option.TargetSeatIndices.IsEmpty())
			{
				Option.MinTargetCount = 1;
				Option.MaxTargetCount = FMath::Min(SlashTargetCount, Option.TargetSeatIndices.Num());
				for (int32 First = 0; First < Option.TargetSeatIndices.Num(); ++First)
				{
					Option.CandidateTargetSelections.Add({ Option.TargetSeatIndices[First] });
					if (Option.MaxTargetCount >= 2)
					{
						for (int32 Second = First + 1; Second < Option.TargetSeatIndices.Num(); ++Second)
						{
							Option.CandidateTargetSelections.Add({
								Option.TargetSeatIndices[First], Option.TargetSeatIndices[Second] });
							if (Option.MaxTargetCount >= 3)
							{
								for (int32 Third = Second + 1; Third < Option.TargetSeatIndices.Num(); ++Third)
								{
									Option.CandidateTargetSelections.Add({
										Option.TargetSeatIndices[First],
										Option.TargetSeatIndices[Second],
										Option.TargetSeatIndices[Third] });
								}
							}
						}
					}
				}
			}
			else if (!Option.TargetSeatIndices.IsEmpty())
			{
				Option.MinTargetCount = 1;
				Option.MaxTargetCount = 1;
			}
			Request.Options.Add(MoveTemp(Option));
		}
	}

	FSGSSkillOptionQuery SkillQuery;
	SkillQuery.QueryName = SGSRuleQueries::PlaySkillOptions();
	SkillQuery.ActorSeat = CurrentSeatIndex;
	Request.SkillOptions = RuleRegistry.CollectSkillOptions(RuleQueryContext, MoveTemp(SkillQuery));

	return Request;
}

FSGSResponseRequest USGSGameDriver::MakeResponseRequest(
	int32 SeatIndex,
	FName WindowName,
	FName RequiredCardName,
	FName ContextName,
	int32 EffectSourceSeat,
	int32 EffectTargetSeat,
	TConstArrayView<FSGSDecisionSkillOption> SkillOptions,
	TConstArrayView<FName> AcceptedCardNames,
	bool bAllowPass)
{
	FSGSResponseRequest Request;
	Request.CommandId = AllocateCommandId();
	Request.SeatIndex = SeatIndex;
	Request.RequestId = ++PendingRequestId;
	Request.Phase = CurrentPhase;
	Request.WindowName = WindowName;
	Request.RequiredCardName = RequiredCardName;
	Request.AcceptedCardNames.Append(AcceptedCardNames.GetData(), AcceptedCardNames.Num());
	if (Request.AcceptedCardNames.IsEmpty() && !RequiredCardName.IsNone())
	{
		Request.AcceptedCardNames.Add(RequiredCardName);
	}
	Request.ContextName = ContextName;
	Request.EffectSourceSeat = EffectSourceSeat;
	Request.EffectTargetSeat = EffectTargetSeat;
	Request.bAllowPass = bAllowPass;
	Request.SkillOptions.Append(SkillOptions.GetData(), SkillOptions.Num());
	FSGSRuleQueryContext RuleQueryContext;
	RuleQueryContext.GameContext = Context;
	RuleQueryContext.ActiveEffects = &ActiveEffectTimeline;
	RuleQueryContext.RuleRegistry = &RuleRegistry;
	RuleQueryContext.Phase = CurrentPhase;
	RuleQueryContext.ActorSeat = SeatIndex;
	RuleQueryContext.WindowName = WindowName;
	RuleQueryContext.RequiredCardName = RequiredCardName;
	RuleQueryContext.AcceptedCardNames = Request.AcceptedCardNames;
	if (WindowName == SGSBasicCardRuleHelpers::SlashDodgeWindowName())
	{
		if (const FSGSResolutionFrame* SlashFrame = ResolutionStack.FindLatestFrameWithState<FSGSSlashResolutionState>())
		{
			const FSGSSlashResolutionState* SlashState = SlashFrame->GetState<FSGSSlashResolutionState>();
			RuleQueryContext.bArmorIgnored = SlashState != nullptr && SlashState->bIgnoreArmor;
		}
	}
	FSGSSkillOptionQuery ResponseSkillQuery;
	ResponseSkillQuery.QueryName = SGSRuleQueries::ResponseSkillOptions();
	ResponseSkillQuery.ActorSeat = SeatIndex;
	ResponseSkillQuery.WindowName = WindowName;
	ResponseSkillQuery.RequiredCardName = RequiredCardName;
	ResponseSkillQuery.AcceptedCardNames = Request.AcceptedCardNames;
	for (FSGSDecisionSkillOption& Option : RuleRegistry.CollectSkillOptions(RuleQueryContext, MoveTemp(ResponseSkillQuery)))
	{
		if (!Request.SkillOptions.ContainsByPredicate(
			[&Option](const FSGSDecisionSkillOption& Existing)
			{
				return Existing.SkillName == Option.SkillName;
			}))
		{
			Request.SkillOptions.Add(MoveTemp(Option));
		}
	}

	FSGSCardQuery Query;
	Query.Zone = SGSGameplayTags::CardZone_Hand.GetTag();
	Query.SeatIndex = SeatIndex;
	Query.ViewerSeatIndex = SeatIndex;
	const FSGSCardQueryResult Cards = Context->QueryCards(Query);
	for (const FSGSCardTarget& Target : Cards.Targets)
	{
		if (Target.Card != nullptr && Request.AcceptedCardNames.Contains(Target.Card->CardName))
		{
			Request.ResponseCardIds.Add(Target.CardId);
		}
	}

	return Request;
}

void USGSGameDriver::DeferResponseRequest(
	const FSGSResponseRequest& Request,
	const TScriptInterface<ISGSDecisionAgent>& Agent)
{
	DeferredResponseRequest = Request;
	DeferredResponseAgent = Agent;
	bHasDeferredResponseRequest = true;
}

void USGSGameDriver::ClearDeferredResponseRequest()
{
	DeferredResponseRequest = FSGSResponseRequest();
	DeferredResponseAgent = TScriptInterface<ISGSDecisionAgent>();
	bHasDeferredResponseRequest = false;
}

void USGSGameDriver::DispatchDeferredResponseRequest()
{
	if (!bHasDeferredResponseRequest)
	{
		return;
	}

	const FSGSResponseRequest Request = DeferredResponseRequest;
	const TScriptInterface<ISGSDecisionAgent> AgentRef = DeferredResponseAgent;
	DeferredResponseRequest = FSGSResponseRequest();
	DeferredResponseAgent = TScriptInterface<ISGSDecisionAgent>();
	bHasDeferredResponseRequest = false;

	ISGSDecisionAgent* Agent = AgentRef.GetInterface();
	if (Agent == nullptr)
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("Deferred response request has no decision agent; using fallback pass."));
		bWaitingForDecision = false;
		if (!ResolveCommandThroughRules(MakeFallbackPassCommand(TEXT("MissingResponseAgent"))))
		{
			FinishCurrentResolution();
		}
		return;
	}

	FSGSResponseDecisionDelegate OnDecided;
	OnDecided.BindUObject(this, &USGSGameDriver::OnResponseActionDecided);
	Agent->RequestResponseAction(Request, OnDecided);
}

FSGSStatus USGSGameDriver::FinishCurrentResolution(FName Reason)
{
	const int32 EffectSourceSeat = ResolutionStack.FindLatestEffectSourceSeat();
	const int32 ActionSeat = EffectSourceSeat != INDEX_NONE ? EffectSourceSeat : CurrentSeatIndex;
	TSGSResult<FSGSResolutionFrame> CompletedResult = ResolutionStack.CompleteCurrentFrame(Reason);
	if (!CompletedResult.HasValue())
	{
		UE_LOG(LogSGSTurn, Error, TEXT("FinishCurrentResolution failed: %s"), *CompletedResult.GetError().ToLogString());
		return MakeError(CompletedResult.GetError());
	}

	PendingCommandId = FSGSCommandId();
	ClearDeferredResponseRequest();
	if (ResolutionStack.GetCurrentFrame() != nullptr)
	{
		return ResumeResolutionParentAfterChild(CompletedResult.GetValue());
	}
	if (bResumeCurrentPhaseAfterResolution)
	{
		bResumeCurrentPhaseAfterResolution = false;
		return MakeValue();
	}

	CurrentSeatIndex = ActionSeat;
	if (!bGameOver && !SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Play))
	{
		AdvanceAfterPhase();
	}

	return MakeValue();
}

FSGSStatus USGSGameDriver::ResumeResolutionParentAfterChild(const FSGSResolutionFrame& CompletedFrame)
{
	FSGSResolutionFrame* ParentFrame = ResolutionStack.GetCurrentFrame();
	if (ParentFrame == nullptr)
	{
		return MakeValue();
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSResolutionContinuations::FinishParentCardResolution())
	{
		return FinishCurrentResolution(SGSResolutionContinuations::FinishParentCardResolution());
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSResolutionContinuations::ResumeDyingPeach())
	{
		return ContinueDyingPeachFrame(*ParentFrame);
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSDamageResolution::ResumeAfterTriggersContinuation())
	{
		return ResumeDamageAfterTriggers();
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSDamageResolution::FinishAfterDyingContinuation())
	{
		return FinishCurrentResolution(FName(TEXT("SGS.Resolution.DamageComplete")));
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSJudgementResolution::ResumeAfterTriggersContinuation())
	{
		return ResumeJudgementAfterTriggers();
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSStandardSkillRules::GanglieAfterJudgementContinuation())
	{
		const FSGSJudgementResolutionState* Judgement = CompletedFrame.GetState<FSGSJudgementResolutionState>();
		check(Judgement != nullptr);
		ParentFrame->OnChildCompletedContinuation = NAME_None;
		return ResumeGanglieAfterJudgement(Judgement->ResultCardId);
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSEquipmentRules::BaguaAfterJudgementContinuation())
	{
		const FSGSJudgementResolutionState* Judgement = CompletedFrame.GetState<FSGSJudgementResolutionState>();
		check(Judgement != nullptr);
		ParentFrame->OnChildCompletedContinuation = NAME_None;
		return ResumeBaguaAfterJudgement(Judgement->ResultCardId);
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSDelayedTrickRules::AfterJudgementContinuation())
	{
		const FSGSJudgementResolutionState* Judgement = CompletedFrame.GetState<FSGSJudgementResolutionState>();
		check(Judgement != nullptr);
		ParentFrame->OnChildCompletedContinuation = NAME_None;
		return ResumeDelayedTrickAfterJudgement(Judgement->ResultCardId);
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSLordAssistResolution::ResumeParentContinuation())
	{
		const FSGSLordAssistResolutionState* Assist = CompletedFrame.GetState<FSGSLordAssistResolutionState>();
		check(Assist != nullptr);
		ParentFrame->OnChildCompletedContinuation = NAME_None;
		return ResumeLordAssistParent(*Assist);
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSTimingEventResolution::ResumeDispatchAfterChild())
	{
		ParentFrame->OnChildCompletedContinuation = NAME_None;
		return ContinueTimingEventDispatch(true);
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSBasicCardRuleHelpers::SlashDamageTriggerContinuation())
	{
		return ResumeSlashAfterDamageTriggers();
	}

	if (ParentFrame->OnChildCompletedContinuation == SGSStandardTrickRules::ResumeContinuation())
	{
		return ResumeStandardTrickResolution();
	}

	return MakeValue();
}

FSGSStatus USGSGameDriver::ResumeDamageAfterTriggers()
{
	FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame();
	if (Frame == nullptr)
	{
		return MakeValue();
	}
	Frame->OnChildCompletedContinuation = NAME_None;

	FSGSCommand Command;
	Command.CommandId = Frame->SourceCommandId;
	Command.RequestId = PendingRequestId > 0 ? PendingRequestId : 1;
	Command.SeatIndex = Frame->ActorSeat;
	Command.Type = SGSGameplayTags::GameEvent_DamageAfter.GetTag();
	Command.Phase = CurrentPhase;
	Command.Payload = FInstancedStruct::Make(FSGSPassCommandPayload());
	Command.SourceChannel = FName(TEXT("ResolutionContinuation"));
	Command.SourceName = SGSDamageResolution::ResumeAfterTriggersContinuation();

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &Command;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::After());
	RuleContext.RuleInvocation.RuleKindTag = SGSRuleKinds::GameRule();
	RuleContext.RuleInvocation.IntentTag = SGSGameplayTags::GameEvent_DamageAfter.GetTag();
	RuleContext.RuleInvocation.SubjectName = SGSDamageResolution::ResumeAfterTriggersContinuation();
	RuleContext.RuleInvocation.ActorSeat = Frame->ActorSeat;
	RuleContext.RuleInvocation.SourceCommandId = Frame->SourceCommandId;
	RuleContext.RuleInvocation.SourceRequestId = Command.RequestId;
	RuleContext.RuleInvocation.Payload = FInstancedStruct::Make(FSGSPassRulePayload());
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;
	return SGSDamageResolution::ContinueAfterTriggers(RuleContext);
}

FSGSStatus USGSGameDriver::ResumeJudgementAfterTriggers()
{
	FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame();
	if (Frame == nullptr)
	{
		return MakeValue();
	}
	Frame->OnChildCompletedContinuation = NAME_None;

	FSGSCommand Command;
	Command.CommandId = Frame->SourceCommandId;
	Command.RequestId = PendingRequestId > 0 ? PendingRequestId : 1;
	Command.SeatIndex = Frame->ActorSeat;
	Command.Type = SGSGameplayTags::GameEvent_JudgementRevealed.GetTag();
	Command.Phase = CurrentPhase;
	Command.Payload = FInstancedStruct::Make(FSGSPassCommandPayload());
	Command.SourceChannel = FName(TEXT("ResolutionContinuation"));
	Command.SourceName = SGSJudgementResolution::ResumeAfterTriggersContinuation();

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &Command;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::After());
	RuleContext.RuleInvocation.RuleKindTag = SGSRuleKinds::GameRule();
	RuleContext.RuleInvocation.IntentTag = SGSGameplayTags::GameEvent_JudgementRevealed.GetTag();
	RuleContext.RuleInvocation.SubjectName = SGSJudgementResolution::ResumeAfterTriggersContinuation();
	RuleContext.RuleInvocation.ActorSeat = Frame->ActorSeat;
	RuleContext.RuleInvocation.SourceCommandId = Frame->SourceCommandId;
	RuleContext.RuleInvocation.SourceRequestId = Command.RequestId;
	RuleContext.RuleInvocation.Payload = FInstancedStruct::Make(FSGSPassRulePayload());
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;
	return SGSJudgementResolution::ContinueAfterTriggers(RuleContext);
}

FSGSStatus USGSGameDriver::ResumeGanglieAfterJudgement(int32 ResultCardId)
{
	FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame();
	check(Frame != nullptr);
	FSGSCommand Command;
	Command.CommandId = Frame->SourceCommandId;
	Command.RequestId = PendingRequestId > 0 ? PendingRequestId : 1;
	Command.SeatIndex = Frame->ActorSeat;
	Command.Type = SGSGameplayTags::GameEvent_DamageAfter.GetTag();
	Command.Phase = CurrentPhase;
	Command.Payload = FInstancedStruct::Make(FSGSPassCommandPayload());
	Command.SourceChannel = FName(TEXT("ResolutionContinuation"));
	Command.SourceName = SGSStandardSkillRules::GanglieAfterJudgementContinuation();

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &Command;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::After());
	RuleContext.RuleInvocation.RuleKindTag = SGSRuleKinds::Trigger();
	RuleContext.RuleInvocation.IntentTag = SGSGameplayTags::GameEvent_DamageAfter.GetTag();
	RuleContext.RuleInvocation.SubjectName = FName(TEXT("Ganglie"));
	RuleContext.RuleInvocation.ActorSeat = Frame->ActorSeat;
	RuleContext.RuleInvocation.SourceCommandId = Frame->SourceCommandId;
	RuleContext.RuleInvocation.SourceRequestId = Command.RequestId;
	RuleContext.RuleInvocation.Payload = FInstancedStruct::Make(FSGSPassRulePayload());
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;
	return SGSStandardSkillRules::ContinueGanglieAfterJudgement(RuleContext, ResultCardId);
}

FSGSStatus USGSGameDriver::ResumeBaguaAfterJudgement(int32 ResultCardId)
{
	FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame();
	check(Frame != nullptr);
	FSGSCommand Command;
	Command.CommandId = Frame->SourceCommandId;
	Command.RequestId = PendingRequestId > 0 ? PendingRequestId : 1;
	Command.SeatIndex = Frame->ActorSeat;
	Command.Type = SGSGameplayTags::PlayAction_RespondCard.GetTag();
	Command.Phase = CurrentPhase;
	Command.Payload = FInstancedStruct::Make(FSGSPassCommandPayload());
	Command.SourceChannel = FName(TEXT("ResolutionContinuation"));
	Command.SourceName = SGSEquipmentRules::BaguaAfterJudgementContinuation();

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &Command;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::Resolve());
	RuleContext.RuleInvocation.RuleKindTag = SGSRuleKinds::Response();
	RuleContext.RuleInvocation.IntentTag = SGSGameplayTags::PlayAction_RespondCard.GetTag();
	RuleContext.RuleInvocation.SubjectName = FName(TEXT("BaguaArmor"));
	RuleContext.RuleInvocation.ActorSeat = Frame->ActorSeat;
	RuleContext.RuleInvocation.WindowName = Frame->WindowName;
	RuleContext.RuleInvocation.SourceCommandId = Frame->SourceCommandId;
	RuleContext.RuleInvocation.SourceRequestId = Command.RequestId;
	RuleContext.RuleInvocation.Payload = FInstancedStruct::Make(FSGSPassRulePayload());
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;
	return SGSEquipmentRules::ContinueBaguaAfterJudgement(RuleContext, ResultCardId);
}

FSGSStatus USGSGameDriver::ResumeDelayedTrickAfterJudgement(int32 ResultCardId)
{
	FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame();
	check(Frame != nullptr);
	FSGSCommand Command;
	Command.CommandId = Frame->SourceCommandId;
	Command.RequestId = PendingRequestId > 0 ? PendingRequestId : 1;
	Command.SeatIndex = Frame->ActorSeat;
	Command.Type = SGSGameplayTags::GameEvent_PhaseBegin.GetTag();
	Command.Phase = CurrentPhase;
	Command.Payload = FInstancedStruct::Make(FSGSPassCommandPayload());
	Command.SourceChannel = FName(TEXT("ResolutionContinuation"));
	Command.SourceName = SGSDelayedTrickRules::AfterJudgementContinuation();

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &Command;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::Resolve());
	RuleContext.RuleInvocation.RuleKindTag = SGSRuleKinds::GameRule();
	RuleContext.RuleInvocation.IntentTag = SGSGameplayTags::GameEvent_PhaseBegin.GetTag();
	RuleContext.RuleInvocation.SubjectName = SGSDelayedTrickRules::AfterJudgementContinuation();
	RuleContext.RuleInvocation.ActorSeat = Frame->ActorSeat;
	RuleContext.RuleInvocation.SourceCommandId = Frame->SourceCommandId;
	RuleContext.RuleInvocation.SourceRequestId = Command.RequestId;
	RuleContext.RuleInvocation.Payload = FInstancedStruct::Make(FSGSPassRulePayload());
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;
	return SGSDelayedTrickRules::ContinueAfterJudgement(RuleContext, ResultCardId);
}

FSGSStatus USGSGameDriver::ResumeLordAssistParent(const FSGSLordAssistResolutionState& State)
{
	FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame();
	check(Frame != nullptr);
	FSGSCommand Command;
	Command.CommandId = Frame->SourceCommandId;
	Command.RequestId = PendingRequestId > 0 ? PendingRequestId : 1;
	Command.SeatIndex = State.LordSeat;
	Command.Type = SGSGameplayTags::PlayAction_RespondCard.GetTag();
	Command.Phase = CurrentPhase;
	Command.Payload = FInstancedStruct::Make(FSGSPassCommandPayload());
	Command.SourceChannel = FName(TEXT("ResolutionContinuation"));
	Command.SourceName = SGSLordAssistResolution::ResumeParentContinuation();

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &Command;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::Resolve());
	RuleContext.RuleInvocation.RuleKindTag = SGSRuleKinds::Response();
	RuleContext.RuleInvocation.IntentTag = SGSGameplayTags::PlayAction_RespondCard.GetTag();
	RuleContext.RuleInvocation.SubjectName = State.RequiredCardName;
	RuleContext.RuleInvocation.ActorSeat = State.LordSeat;
	RuleContext.RuleInvocation.WindowName = State.OriginWindowName;
	RuleContext.RuleInvocation.SourceCommandId = Frame->SourceCommandId;
	RuleContext.RuleInvocation.SourceRequestId = Command.RequestId;
	RuleContext.RuleInvocation.Payload = FInstancedStruct::Make(FSGSPassRulePayload());
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;

	if (State.OriginWindowName == SGSBasicCardRuleHelpers::SlashDodgeWindowName())
	{
		return State.bSucceeded
			? SGSBasicCardRuleHelpers::ResolveSuccessfulSlashDodge(RuleContext)
			: SGSBasicCardRuleHelpers::ResolveSlashHit(RuleContext);
	}
	check(State.OriginWindowName == SGSStandardTrickRules::EffectResponseWindow());
	return State.bSucceeded
		? SGSStandardTrickRules::ContinueAfterAcceptedResponse(RuleContext)
		: SGSStandardTrickRules::ContinueAfterDeclinedResponse(RuleContext);
}

FSGSStatus USGSGameDriver::ResumeSlashAfterDamageTriggers()
{
	FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame();
	if (Frame == nullptr)
	{
		return MakeValue();
	}
	Frame->OnChildCompletedContinuation = NAME_None;

	FSGSCommand Command;
	Command.CommandId = Frame->SourceCommandId;
	Command.RequestId = PendingRequestId > 0 ? PendingRequestId : 1;
	Command.SeatIndex = Frame->ActorSeat;
	Command.Type = SGSGameplayTags::PlayAction_UseCard.GetTag();
	Command.Phase = CurrentPhase;
	Command.Payload = FInstancedStruct::Make(FSGSPassCommandPayload());
	Command.SourceChannel = FName(TEXT("ResolutionContinuation"));
	Command.SourceName = SGSBasicCardRuleHelpers::SlashDamageTriggerContinuation();

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &Command;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::Resolve());
	RuleContext.RuleInvocation.RuleKindTag = SGSRuleKinds::GameRule();
	RuleContext.RuleInvocation.IntentTag = SGSGameplayTags::PlayAction_UseCard.GetTag();
	RuleContext.RuleInvocation.SubjectName = SGSBasicCardRuleHelpers::SlashDamageTriggerContinuation();
	RuleContext.RuleInvocation.ActorSeat = Frame->ActorSeat;
	RuleContext.RuleInvocation.SourceCommandId = Frame->SourceCommandId;
	RuleContext.RuleInvocation.SourceRequestId = Command.RequestId;
	RuleContext.RuleInvocation.Payload = FInstancedStruct::Make(FSGSPassRulePayload());
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;
	return SGSBasicCardRuleHelpers::ContinueSlashAfterDamageTriggers(RuleContext);
}

FSGSStatus USGSGameDriver::ResumeStandardTrickResolution()
{
	FSGSResolutionFrame* Frame = ResolutionStack.GetCurrentFrame();
	if (Frame == nullptr)
	{
		return MakeValue();
	}
	Frame->OnChildCompletedContinuation = NAME_None;

	FSGSCommand Command;
	Command.CommandId = Frame->SourceCommandId;
	Command.RequestId = PendingRequestId > 0 ? PendingRequestId : 1;
	Command.SeatIndex = Frame->ActorSeat;
	Command.Type = SGSGameplayTags::PlayAction_UseCard.GetTag();
	Command.Phase = CurrentPhase;
	Command.Payload = FInstancedStruct::Make(FSGSPassCommandPayload());
	Command.SourceChannel = FName(TEXT("ResolutionContinuation"));
	Command.SourceName = SGSStandardTrickRules::ResumeContinuation();

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &Command;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::Resolve());
	RuleContext.RuleInvocation.RuleKindTag = SGSRuleKinds::GameRule();
	RuleContext.RuleInvocation.IntentTag = SGSGameplayTags::PlayAction_UseCard.GetTag();
	RuleContext.RuleInvocation.SubjectName = SGSStandardTrickRules::ResumeContinuation();
	RuleContext.RuleInvocation.ActorSeat = Frame->ActorSeat;
	RuleContext.RuleInvocation.SourceCommandId = Frame->SourceCommandId;
	RuleContext.RuleInvocation.SourceRequestId = Command.RequestId;
	RuleContext.RuleInvocation.Payload = FInstancedStruct::Make(FSGSPassRulePayload());
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;
	return SGSStandardTrickRules::Continue(RuleContext);
}

bool USGSGameDriver::OpenNextDyingPeachResponseWindow(FSGSResolutionFrame& DyingFrame)
{
	FSGSDyingPeachResolutionState* DyingState = DyingFrame.GetMutableState<FSGSDyingPeachResolutionState>();
	if (Context == nullptr || DyingState == nullptr)
	{
		return false;
	}

	const USGSSeat* DyingSeat = Context->GetSeat(DyingState->DyingSeat);
	if (DyingSeat == nullptr || !DyingSeat->bIsAlive || DyingSeat->Health > 0)
	{
		return false;
	}

	DyingState->bNeedsHealthRecheck = false;
	while (DyingState->ResponderSeatIndices.IsValidIndex(DyingState->NextResponderIndex))
	{
		const int32 ResponderSeat = DyingState->ResponderSeatIndices[DyingState->NextResponderIndex++];
		USGSSeat* Responder = Context->GetSeat(ResponderSeat);
		ISGSDecisionAgent* Agent = Responder != nullptr ? Responder->DecisionAgent.GetInterface() : nullptr;
		if (Responder == nullptr || !Responder->bIsAlive || Agent == nullptr)
		{
			continue;
		}

		TArray<FName> AcceptedCardNames = { FName(TEXT("Peach")) };
		if (ResponderSeat == DyingState->DyingSeat)
		{
			AcceptedCardNames.Add(FName(TEXT("Analeptic")));
		}
		ResolutionStack.OpenResponseWindowOnCurrent(
			FName(TEXT("Dying.Peach")),
			FName(TEXT("Peach")),
			AcceptedCardNames,
			DyingFrame.SourceSeat,
			DyingFrame.TargetSeat);

		FSGSResponseRequest Request = MakeResponseRequest(
			ResponderSeat,
			FName(TEXT("Dying.Peach")),
			FName(TEXT("Peach")),
			FName(TEXT("Dying")),
			DyingFrame.SourceSeat,
			DyingFrame.TargetSeat,
			{},
			AcceptedCardNames);
		PendingCommandId = Request.CommandId;
		CurrentSeatIndex = ResponderSeat;
		bWaitingForDecision = true;
		DeferResponseRequest(Request, Responder->DecisionAgent);
		return true;
	}

	return false;
}

FSGSStatus USGSGameDriver::ContinueDyingPeachFrame(FSGSResolutionFrame& DyingFrame)
{
	FSGSDyingPeachResolutionState* DyingState = DyingFrame.GetMutableState<FSGSDyingPeachResolutionState>();
	if (DyingState == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Rule.FrameStateMismatch")),
			TEXT("DyingPeach continuation requires a DyingPeach frame state.")));
	}

	const USGSSeat* DyingSeat = Context != nullptr ? Context->GetSeat(DyingState->DyingSeat) : nullptr;
	if (DyingSeat == nullptr || !DyingSeat->bIsAlive || DyingSeat->Health > 0)
	{
		return FinishCurrentResolution(FName(TEXT("SGS.Resolution.DyingPeachResolved")));
	}

	if (OpenNextDyingPeachResponseWindow(DyingFrame))
	{
		return MakeValue();
	}

	const FSGSCommandId CommandId = PendingCommandId.IsValid() ? PendingCommandId : DyingFrame.SourceCommandId;
	if (FSGSStatus Status = RunEffectStep(
		SGSStandardEffectSteps::MakeEliminateSeatStep(
			DyingState->DyingSeat,
			DyingFrame.SourceSeat,
			FName(TEXT("NoPeach"))),
		CommandId);
		Status.HasError())
	{
		return Status;
	}

	return FinishCurrentResolution(FName(TEXT("SGS.Resolution.DyingPeachNoPeach")));
}

FSGSStatus USGSGameDriver::PublishTimingEvent(const FSGSRuleEventPayload& Payload)
{
	if (Context == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Rule.MissingContext")),
			TEXT("Cannot publish a timing event without a game context.")));
	}

	FSGSRuleEventPayload DispatchPayload = Payload;
	if (!DispatchPayload.CheckInvariants())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Rule.InvalidTimingEvent")),
			FString::Printf(TEXT("Invalid timing event payload: %s"), *DispatchPayload.ToPayloadLogString())));
	}

	const int32 ActorSeat = DispatchPayload.SourceSeat != INDEX_NONE
		? DispatchPayload.SourceSeat
		: (DispatchPayload.TargetSeat != INDEX_NONE ? DispatchPayload.TargetSeat : CurrentSeatIndex);
	if (ActorSeat == INDEX_NONE)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Rule.InvalidTimingEvent")),
			FString::Printf(TEXT("Timing event has no actor seat: %s"), *DispatchPayload.ToPayloadLogString())));
	}

	if (!DispatchPayload.SourceCommandId.IsValid())
	{
		DispatchPayload.SourceCommandId = AllocateCommandId();
	}
	const int32 SourceRequestId = PendingRequestId > 0 ? PendingRequestId : 1;

	FSGSRuleInvocation Invocation;
	Invocation.RuleKindTag = SGSRuleKinds::Trigger();
	Invocation.IntentTag = DispatchPayload.EventTag;
	Invocation.SubjectName = NAME_None;
	Invocation.ActorSeat = ActorSeat;
	Invocation.WindowName = DispatchPayload.TimingPoint.Step;
	Invocation.SourceCommandId = DispatchPayload.SourceCommandId;
	Invocation.SourceRequestId = SourceRequestId;
	Invocation.Payload = FInstancedStruct::Make(DispatchPayload);

	FSGSCommand EventCommand;
	EventCommand.CommandId = DispatchPayload.SourceCommandId;
	EventCommand.RequestId = SourceRequestId;
	EventCommand.SeatIndex = ActorSeat;
	EventCommand.Type = DispatchPayload.EventTag;
	EventCommand.Phase = CurrentPhase;
	EventCommand.Payload = FInstancedStruct::Make(DispatchPayload);
	EventCommand.SourceChannel = FName(TEXT("TimingEvent"));
	EventCommand.SourceName = DispatchPayload.EventName;

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	ExecutionContext.ExpectedCommandId = EventCommand.CommandId;
	ExecutionContext.ExpectedRequestId = EventCommand.RequestId;
	ExecutionContext.ExpectedSeatIndex = EventCommand.SeatIndex;
	ExecutionContext.ExpectedPhase = CurrentPhase;

	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &EventCommand;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = DispatchPayload.TimingPoint;
	RuleContext.RuleInvocation = MoveTemp(Invocation);
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;

	FSGSTimingEventResolutionState State;
	State.EventPayload = DispatchPayload;
	State.RuleNames = RuleRegistry.FindMatchingTriggerRuleNames(RuleContext);
	if (State.RuleNames.IsEmpty())
	{
		return MakeValue();
	}

	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = SGSTimingEventResolution::FrameRuleName();
	Frame.SourceCommandId = DispatchPayload.SourceCommandId;
	Frame.ActorSeat = ActorSeat;
	Frame.SourceSeat = DispatchPayload.SourceSeat;
	Frame.TargetSeat = DispatchPayload.TargetSeat;
	Frame.FrameState = FInstancedStruct::Make(State);
	ResolutionStack.PushFrame(MoveTemp(Frame));
	return ContinueTimingEventDispatch(false);
}

FSGSStatus USGSGameDriver::ContinueTimingEventDispatch(bool bResumeParentOnComplete)
{
	FSGSResolutionFrame* TimingFrame = ResolutionStack.GetCurrentFrame();
	FSGSTimingEventResolutionState* State = TimingFrame != nullptr
		? TimingFrame->GetMutableState<FSGSTimingEventResolutionState>()
		: nullptr;
	if (TimingFrame == nullptr || State == nullptr)
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Rule.MissingTimingEventState")),
			TEXT("Timing event continuation requires the current timing event frame.")));
	}

	const FSGSStableHandle TimingFrameHandle = ResolutionStack.GetCurrentFrameHandle();
	if (FSGSStatus Status = ResolutionStack.ClearResponseWindowOnCurrent(); Status.HasError())
	{
		return Status;
	}

	while (State->RuleNames.IsValidIndex(State->NextRuleIndex))
	{
		const FName RuleName = State->RuleNames[State->NextRuleIndex++];
		const FSGSRuleEventPayload DispatchPayload = State->EventPayload;
		const int32 ActorSeat = TimingFrame->ActorSeat;
		const int32 SourceRequestId = PendingRequestId > 0 ? PendingRequestId : 1;

		FSGSRuleInvocation Invocation;
		Invocation.RuleKindTag = SGSRuleKinds::Trigger();
		Invocation.IntentTag = DispatchPayload.EventTag;
		Invocation.SubjectName = NAME_None;
		Invocation.ActorSeat = ActorSeat;
		Invocation.WindowName = DispatchPayload.TimingPoint.Step;
		Invocation.SourceCommandId = DispatchPayload.SourceCommandId;
		Invocation.SourceRequestId = SourceRequestId;
		Invocation.Payload = FInstancedStruct::Make(DispatchPayload);

		FSGSCommand EventCommand;
		EventCommand.CommandId = DispatchPayload.SourceCommandId;
		EventCommand.RequestId = SourceRequestId;
		EventCommand.SeatIndex = ActorSeat;
		EventCommand.Type = DispatchPayload.EventTag;
		EventCommand.Phase = CurrentPhase;
		EventCommand.Payload = FInstancedStruct::Make(DispatchPayload);
		EventCommand.SourceChannel = FName(TEXT("TimingEvent"));
		EventCommand.SourceName = RuleName;

		FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
		ExecutionContext.ExpectedCommandId = EventCommand.CommandId;
		ExecutionContext.ExpectedRequestId = EventCommand.RequestId;
		ExecutionContext.ExpectedSeatIndex = EventCommand.SeatIndex;
		ExecutionContext.ExpectedPhase = CurrentPhase;

		FSGSDriverRuleRuntime Runtime(*this);
		FSGSRuleExecutionContext RuleContext;
		RuleContext.GameContext = Context;
		RuleContext.Command = &EventCommand;
		RuleContext.CommandExecutionContext = &ExecutionContext;
		RuleContext.ReplayLog = &ReplayLog;
		RuleContext.ActiveEffects = &ActiveEffectTimeline;
		RuleContext.TimingPoint = DispatchPayload.TimingPoint;
		RuleContext.RuleInvocation = MoveTemp(Invocation);
		RuleContext.Runtime = &Runtime;
		RuleContext.RuleRegistry = &RuleRegistry;
		if (FSGSStatus Status = RuleRegistry.DispatchTriggerByName(RuleContext, RuleName); Status.HasError())
		{
			return Status;
		}
		SyncReplayLog();

		TimingFrame = ResolutionStack.FindFrame(TimingFrameHandle);
		if (ResolutionStack.GetCurrentFrameHandle() != TimingFrameHandle
			|| TimingFrame == nullptr
			|| bWaitingForDecision
			|| !TimingFrame->WindowName.IsNone())
		{
			return MakeValue();
		}
		State = TimingFrame->GetMutableState<FSGSTimingEventResolutionState>();
	}

	TSGSResult<FSGSResolutionFrame> CompletedResult = ResolutionStack.CompleteCurrentFrame(
		FName(TEXT("SGS.Resolution.TimingEventComplete")));
	if (!CompletedResult.HasValue())
	{
		return MakeError(CompletedResult.GetError());
	}
	SyncReplayLog();
	if (bResumeParentOnComplete && ResolutionStack.GetCurrentFrame() != nullptr)
	{
		return ResumeResolutionParentAfterChild(CompletedResult.GetValue());
	}
	return MakeValue();
}

void USGSGameDriver::ExecuteDrawPhaseThroughPipeline()
{
	EffectPipeline.Reset();
	EffectPipeline.Enqueue(SGSStandardEffectSteps::MakeDrawCardsStep(CurrentSeatIndex, DrawCountPerTurn));
	FSGSEffectContext EffectContext = MakeEffectContext();
	if (FSGSStatus Status = EffectPipeline.Run(EffectContext); Status.HasError())
	{
		UE_LOG(LogSGSTurn, Error, TEXT("Draw phase effect pipeline failed: %s"), *Status.GetError().ToLogString());
	}
	SyncReplayLog();
}

void USGSGameDriver::ExecuteJudgementPhaseThroughRules()
{
	TArray<USGSCard*> DelayedTricks = Context->GetCardsInZone(
		SGSGameplayTags::CardZone_Judgement.GetTag(), CurrentSeatIndex);
	if (DelayedTricks.IsEmpty())
	{
		AdvanceAfterPhase();
		return;
	}
	ResolveJudgementCard(DelayedTricks[0]);
}

bool USGSGameDriver::ResolveJudgementCard(USGSCard* DelayedTrickCard)
{
	if (DelayedTrickCard == nullptr)
	{
		return false;
	}

	FSGSCommand Command;
	Command.CommandId = AllocateCommandId();
	Command.RequestId = ++PendingRequestId;
	Command.SeatIndex = CurrentSeatIndex;
	Command.Type = SGSGameplayTags::GameEvent_PhaseBegin.GetTag();
	Command.Phase = CurrentPhase;
	FSGSJudgementRulePayload Payload;
	Payload.DelayedTrickCardId = DelayedTrickCard->CardId;
	Payload.SeatIndex = CurrentSeatIndex;
	Command.Payload = FInstancedStruct::Make(Payload);
	Command.SourceChannel = FName(TEXT("JudgementPhase"));
	Command.SourceName = DelayedTrickCard->CardName;

	FSGSRuleInvocation Invocation;
	Invocation.RuleKindTag = SGSRuleKinds::GameRule();
	Invocation.IntentTag = Command.Type;
	Invocation.SubjectName = DelayedTrickCard->CardName;
	Invocation.ActorSeat = CurrentSeatIndex;
	Invocation.WindowName = SGSTimingSteps::Resolve();
	Invocation.SourceCommandId = Command.CommandId;
	Invocation.SourceRequestId = Command.RequestId;
	Invocation.Payload = FInstancedStruct::Make(Payload);

	FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	ExecutionContext.ExpectedCommandId = Command.CommandId;
	ExecutionContext.ExpectedRequestId = Command.RequestId;
	FSGSDriverRuleRuntime Runtime(*this);
	FSGSRuleExecutionContext RuleContext;
	RuleContext.GameContext = Context;
	RuleContext.Command = &Command;
	RuleContext.CommandExecutionContext = &ExecutionContext;
	RuleContext.ReplayLog = &ReplayLog;
	RuleContext.ActiveEffects = &ActiveEffectTimeline;
	RuleContext.TimingPoint = MakeCurrentTimingPoint(SGSTimingSteps::Resolve());
	RuleContext.RuleInvocation = MoveTemp(Invocation);
	RuleContext.Runtime = &Runtime;
	RuleContext.RuleRegistry = &RuleRegistry;
	const FSGSStatus Status = RuleRegistry.Resolve(RuleContext);
	SyncReplayLog();
	DispatchDeferredResponseRequest();
	return !Status.HasError();
}

bool USGSGameDriver::HasStatusEffect(int32 SeatIndex, FGameplayTag StatusTag) const
{
	for (const FSGSStableHandle Handle : ActiveEffectTimeline.QueryByTag(StatusTag))
	{
		const FSGSActiveEffect* Effect = ActiveEffectTimeline.Find(Handle);
		if (Effect != nullptr && Effect->OwnerSeat == SeatIndex)
		{
			return true;
		}
	}
	return false;
}

FSGSStatus USGSGameDriver::RunEffectStep(FSGSEffectStep Step, FSGSCommandId CommandId)
{
	EffectPipeline.Reset();
	EffectPipeline.Enqueue(MoveTemp(Step));
	FSGSEffectContext EffectContext = MakeEffectContext(CommandId);
	if (FSGSStatus Status = EffectPipeline.Run(EffectContext); Status.HasError())
	{
		UE_LOG(LogSGSTurn, Error, TEXT("Effect step failed: %s"), *Status.GetError().ToLogString());
		SyncReplayLog();
		return Status;
	}
	SyncReplayLog();
	return MakeValue();
}

void USGSGameDriver::SyncReplayLog()
{
	ReplayLog.SetCommandLog(CommandRouter.GetLogEntries());
	if (Context != nullptr)
	{
		ReplayLog.SetRandomLog(Context->GetRandomAudit().GetEntries());
	}
	ViewStateInvalidatedDelegate.Broadcast();
}

void USGSGameDriver::PublishAIObservation(const FSGSRuleInvocation& Invocation)
{
	FSGSAIActionCandidate Candidate;
	Candidate.CardName = Invocation.SubjectName;
	Candidate.SkillName = NAME_None;
	if (const FSGSUseCardRulePayload* UseCardPayload = Invocation.GetPayload<FSGSUseCardRulePayload>())
	{
		Candidate.CardName = UseCardPayload->CardName;
		Candidate.TargetSeatIndices = UseCardPayload->TargetSeatIndices;
		if (Candidate.TargetSeatIndices.IsEmpty())
		{
			const FSGSStandardCardDefinition* Definition = SGSStandardCardDefinitions::Find(Candidate.CardName);
			if (Definition != nullptr
				&& (Definition->TargetMode == SGSCardTargetModes::AllAlive()
					|| Definition->TargetMode == SGSCardTargetModes::AllOthers()))
			{
				for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
				{
					const USGSSeat* Seat = Context->GetSeat(SeatIndex);
					if (Seat != nullptr && Seat->bIsAlive
						&& (Definition->TargetMode == SGSCardTargetModes::AllAlive()
							|| SeatIndex != Invocation.ActorSeat))
					{
						Candidate.TargetSeatIndices.Add(SeatIndex);
					}
				}
			}
		}
	}
	else if (const FSGSActivateSkillRulePayload* SkillPayload = Invocation.GetPayload<FSGSActivateSkillRulePayload>())
	{
		Candidate.SkillName = SkillPayload->SkillName;
		Candidate.CardName = SkillPayload->ResultCardName;
		Candidate.TargetSeatIndices = SkillPayload->TargetSeatIndices;
	}
	else if (const FSGSRespondCardRulePayload* ResponsePayload = Invocation.GetPayload<FSGSRespondCardRulePayload>())
	{
		Candidate.CardName = ResponsePayload->CardName;
		Candidate.SkillName = ResponsePayload->SkillName;
		if (ResponsePayload->EffectTargetSeat != INDEX_NONE)
		{
			Candidate.TargetSeatIndices.Add(ResponsePayload->EffectTargetSeat);
		}
	}
	else
	{
		return;
	}

	const FSGSAIBehaviorSemantics* Semantics = AIEvaluatorRegistry.FindSemantics(Candidate);
	if (Semantics == nullptr || Semantics->TargetAttitude.IsNone())
	{
		return;
	}

	FSGSAIPublicActionObservation Observation;
	Observation.ActorSeat = Invocation.ActorSeat;
	Observation.TargetSeatIndices = Candidate.TargetSeatIndices;
	Observation.TargetAttitude = Semantics->TargetAttitude;
	Observation.Strength = FMath::Max(5.0, FMath::Abs(Semantics->Effect) + Semantics->Threat * 0.5);
	int32 LordSeat = INDEX_NONE;
	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		const USGSSeat* Seat = Context->GetSeat(SeatIndex);
		if (Seat != nullptr && Seat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag()))
		{
			LordSeat = SeatIndex;
			break;
		}
	}
	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		USGSSeat* Seat = Context->GetSeat(SeatIndex);
		if (USGSBasicAIAgent* Agent = Seat != nullptr ? Cast<USGSBasicAIAgent>(Seat->DecisionAgent.GetObject()) : nullptr)
		{
			Agent->ObservePublicAction(Observation, LordSeat);
		}
	}
}

void USGSGameDriver::AdvanceAfterPhase()
{
	PhaseProgress = ESGSPhaseProgress::End;
}

void USGSGameDriver::CommitPhaseAdvance()
{
	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Play))
	{
		ExpireStatusEffects(CurrentSeatIndex, SGSGameplayTags::Status_SlashUsed.GetTag());
		ExpireStatusEffects(CurrentSeatIndex, SGSGameplayTags::Status_ZhihengUsed.GetTag());
		ExpireStatusEffects(CurrentSeatIndex, SGSGameplayTags::Status_JijiangFailed.GetTag());
	}

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_RoundEnd))
	{
		if (USGSSeat* EndingSeat = Context->GetSeat(CurrentSeatIndex))
		{
			EndingSeat->ResetTurnSkillState();
		}
		ExpireStatusEffects(CurrentSeatIndex, SGSGameplayTags::Status_AnalepticUsed.GetTag());
		ExpireStatusEffects(CurrentSeatIndex, SGSGameplayTags::Status_AnalepticBoost.GetTag());
		Broadcast(SGSGameplayTags::GameEvent_TurnEnded.GetTag());
		++TurnsPlayed;

		if (CurrentMaxTurns > 0 && TurnsPlayed >= CurrentMaxTurns)
		{
			EndGame();
			return;
		}

		// 推进到下一名存活座位。
		const int32 SeatCount = Context->NumSeats();
		for (int32 Step = 1; Step <= SeatCount; ++Step)
		{
			const int32 Candidate = (CurrentSeatIndex + Step) % SeatCount;
			const USGSSeat* CandidateSeat = Context->GetSeat(Candidate);
			if (CandidateSeat != nullptr && CandidateSeat->bIsAlive)
			{
				CurrentSeatIndex = Candidate;
				BeginTurn();
				return;
			}
		}

		// 无存活座位可行动：结束。
		EndGame();
		return;
	}

	CurrentPhase = NextPhase(CurrentPhase);
	PhaseProgress = ESGSPhaseProgress::Before;
}

FSGSStatus USGSGameDriver::PublishPhaseTiming(FGameplayTag EventTag, FName Step)
{
	FSGSRuleEventPayload Payload;
	Payload.EventTag = EventTag;
	Payload.EventName = FName(*FString::Printf(TEXT("Phase.%s"), *Step.ToString()));
	Payload.SourceSeat = CurrentSeatIndex;
	Payload.TargetSeat = CurrentSeatIndex;
	Payload.TimingPoint = MakeCurrentTimingPoint(Step);
	FSGSStatus Status = PublishTimingEvent(Payload);
	DispatchDeferredResponseRequest();
	return Status;
}

void USGSGameDriver::DiscardAllCards(int32 SeatIndex)
{
	TArray<USGSCard*> Hand = Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), SeatIndex);
	TArray<USGSCard*> Judgement = Context->GetCardsInZone(SGSGameplayTags::CardZone_Judgement.GetTag(), SeatIndex);
	USGSSeat* Seat = Context->GetSeat(SeatIndex);
	TArray<USGSCard*> Equipment;
	for (const TPair<FGameplayTag, TObjectPtr<USGSCard>>& Slot : Seat->Equipment)
	{
		Equipment.Add(Slot.Value.Get());
	}

	// EliminateSeat 的回调可能发生在主 EffectPipeline 正在运行时；使用独立管线记录
	// 后续清牌，避免重置正在执行的队列，同时仍保持所有权威移动可审计。
	FSGSEffectPipeline CleanupPipeline;
	FSGSCardMoveEventMetadata Metadata;
	Metadata.Reason = SGSCardMoveReasons::Discard();
	if (!Hand.IsEmpty())
	{
		CleanupPipeline.Enqueue(SGSStandardEffectSteps::MakeMoveCardsStep(
			MoveTemp(Hand),
			SGSGameplayTags::CardZone_Hand.GetTag(),
			SeatIndex,
			SGSGameplayTags::CardZone_DiscardPile.GetTag(),
			INDEX_NONE,
			Metadata));
	}
	if (!Equipment.IsEmpty())
	{
		CleanupPipeline.Enqueue(SGSStandardEffectSteps::MakeMoveCardsStep(
			MoveTemp(Equipment),
			SGSGameplayTags::CardZone_Equipment.GetTag(),
			SeatIndex,
			SGSGameplayTags::CardZone_DiscardPile.GetTag(),
			INDEX_NONE,
			Metadata));
	}
	if (!Judgement.IsEmpty())
	{
		CleanupPipeline.Enqueue(SGSStandardEffectSteps::MakeMoveCardsStep(
			MoveTemp(Judgement),
			SGSGameplayTags::CardZone_Judgement.GetTag(),
			SeatIndex,
			SGSGameplayTags::CardZone_DiscardPile.GetTag(),
			INDEX_NONE,
			Metadata));
	}
	FSGSEffectContext EffectContext = MakeEffectContext();
	if (FSGSStatus Status = CleanupPipeline.Run(EffectContext); Status.HasError())
	{
		UE_LOG(LogSGSTurn, Error, TEXT("Discard-all effect pipeline failed: %s"), *Status.GetError().ToLogString());
	}
	Seat->Equipment.Reset();
	SyncReplayLog();
}

void USGSGameDriver::HandleSeatEliminated(int32 SeatIndex, int32 SourceSeat, FName)
{
	DiscardAllCards(SeatIndex);

	USGSSeat* EliminatedSeat = Context->GetSeat(SeatIndex);
	USGSSeat* KillerSeat = Context->GetSeat(SourceSeat);
	if (KillerSeat != nullptr && KillerSeat->bIsAlive)
	{
		if (EliminatedSeat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag()))
		{
			FSGSEffectPipeline RewardPipeline;
			FSGSCardMoveEventMetadata Metadata;
			Metadata.Reason = SGSCardMoveReasons::RewardDraw();
			RewardPipeline.Enqueue(SGSStandardEffectSteps::MakeDrawCardsStep(SourceSeat, 3, MoveTemp(Metadata)));
			FSGSEffectContext EffectContext = MakeEffectContext();
			if (FSGSStatus Status = RewardPipeline.Run(EffectContext); Status.HasError())
			{
				UE_LOG(LogSGSTurn, Error, TEXT("Elimination reward draw failed: %s"), *Status.GetError().ToLogString());
			}
			SyncReplayLog();
		}
		else if (KillerSeat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
			&& EliminatedSeat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Loyalist.GetTag()))
		{
			DiscardAllCards(SourceSeat);
		}
	}

	EvaluateIdentityVictory();
}

void USGSGameDriver::EvaluateIdentityVictory()
{
	if (!bIdentityMode || bGameOver)
	{
		return;
	}

	USGSSeat* Lord = nullptr;
	int32 AliveNonLordCount = 0;
	int32 AliveRenegadeSeat = INDEX_NONE;
	bool bEnemyOfLordAlive = false;
	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		USGSSeat* Seat = Context->GetSeat(SeatIndex);
		if (Seat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag()))
		{
			Lord = Seat;
			continue;
		}
		if (!Seat->bIsAlive)
		{
			continue;
		}

		++AliveNonLordCount;
		if (Seat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Renegade.GetTag()))
		{
			AliveRenegadeSeat = SeatIndex;
		}
		if (Seat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Rebel.GetTag())
			|| Seat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Renegade.GetTag()))
		{
			bEnemyOfLordAlive = true;
		}
	}

	check(Lord != nullptr);
	if (Lord->bIsAlive && bEnemyOfLordAlive)
	{
		return;
	}

	if (Lord->bIsAlive)
	{
		GameResult.EndReason = FName(TEXT("SGS.GameEnd.LordSideVictory"));
		GameResult.WinningIdentities = {
			SGSGameplayTags::Identity_Lord.GetTag(),
			SGSGameplayTags::Identity_Loyalist.GetTag(),
		};
	}
	else if (AliveNonLordCount == 1 && AliveRenegadeSeat != INDEX_NONE)
	{
		GameResult.EndReason = FName(TEXT("SGS.GameEnd.RenegadeVictory"));
		GameResult.WinningIdentities = { SGSGameplayTags::Identity_Renegade.GetTag() };
	}
	else
	{
		GameResult.EndReason = FName(TEXT("SGS.GameEnd.RebelVictory"));
		GameResult.WinningIdentities = { SGSGameplayTags::Identity_Rebel.GetTag() };
	}

	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		const FGameplayTag Identity = Context->GetSeat(SeatIndex)->Identity;
		if (GameResult.WinningIdentities.ContainsByPredicate([Identity](const FGameplayTag& Winner)
		{
			return Identity.MatchesTagExact(Winner);
		}))
		{
			GameResult.WinningSeatIndices.Add(SeatIndex);
		}
	}
	EndGame();
}

void USGSGameDriver::ExecuteDiscardPhase()
{
	const USGSSeat* Seat = Context->GetSeat(CurrentSeatIndex);
	TArray<USGSCard*> Hand = Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), CurrentSeatIndex);
	const int32 ExcessCount = Hand.Num() - FMath::Max(Seat->Health, 0);
	if (ExcessCount <= 0)
	{
		AdvanceAfterPhase();
		return;
	}

	FSGSDecisionSkillOption Option;
	Option.SkillName = FName(TEXT("DiscardPhase"));
	Option.DisplayName = FName(TEXT("弃牌"));
	Option.RuleKindTag = SGSRuleKinds::GameRule();
	Option.MinCardCount = ExcessCount;
	Option.MaxCardCount = ExcessCount;
	TArray<int32> AICandidate;
	for (USGSCard* Card : Hand)
	{
		if (Card != nullptr)
		{
			Option.SelectableCardIds.Add(Card->CardId);
		}
	}
	for (int32 Index = Hand.Num() - ExcessCount; Index < Hand.Num(); ++Index)
	{
		if (Hand[Index] != nullptr)
		{
			AICandidate.Add(Hand[Index]->CardId);
		}
	}
	Option.CandidateCardSelections.Add(MoveTemp(AICandidate));
	TArray<FSGSDecisionSkillOption> Options;
	Options.Add(MoveTemp(Option));
	const FSGSResponseRequest Request = MakeResponseRequest(
		CurrentSeatIndex,
		SGSDiscardPhaseRules::WindowName(),
		NAME_None,
		FName(TEXT("DiscardPhase")),
		CurrentSeatIndex,
		CurrentSeatIndex,
		Options,
		{},
		false);

	FSGSDiscardPhaseResolutionState State;
	State.SeatIndex = CurrentSeatIndex;
	State.RequiredDiscardCount = ExcessCount;
	FSGSResolutionFrame Frame;
	Frame.SourceRuleName = FName(TEXT("SGS.Rule.Phase.Discard"));
	Frame.SourceCommandId = Request.CommandId;
	Frame.ActorSeat = CurrentSeatIndex;
	Frame.SourceSeat = CurrentSeatIndex;
	Frame.TargetSeat = CurrentSeatIndex;
	Frame.FrameState = FInstancedStruct::Make(State);
	ResolutionStack.PushFrame(MoveTemp(Frame));
	ResolutionStack.OpenResponseWindowOnCurrent(
		Request.WindowName, NAME_None, {}, CurrentSeatIndex, CurrentSeatIndex);

	PendingCommandId = Request.CommandId;
	bWaitingForDecision = true;
	DeferResponseRequest(Request, Seat->DecisionAgent);
	DispatchDeferredResponseRequest();
}

void USGSGameDriver::ExpireStatusEffects(int32 SeatIndex, FGameplayTag StatusTag)
{
	const TArray<FSGSStableHandle> Handles = ActiveEffectTimeline.QueryByTag(StatusTag);
	for (const FSGSStableHandle Handle : Handles)
	{
		const FSGSActiveEffect* Effect = ActiveEffectTimeline.Find(Handle);
		if (Effect != nullptr && Effect->OwnerSeat == SeatIndex)
		{
			ActiveEffectTimeline.Expire(
				Handle,
				MakeCurrentTimingPoint(SGSTimingSteps::End()),
				SGSActiveEffectExpireReasons::DurationEnded());
		}
	}
}

void USGSGameDriver::EndGame()
{
	if (!GameResult.IsFinished())
	{
		GameResult.EndReason = FName(TEXT("SGS.GameEnd.TurnLimit"));
	}
	bGameOver = true;
	bWaitingForDecision = false;
	TurnSeatIndex = INDEX_NONE;
	UE_LOG(LogSGSTurn, Log, TEXT("Game over after %d turns."), TurnsPlayed);
	Broadcast(SGSGameplayTags::GameEvent_GameEnded.GetTag());
}

void USGSGameDriver::Broadcast(FSGSGameEvent Event)
{
	FSGSEventContext EventContext;
	EventContext.Event = Event;
	EventContext.SeatIndex = CurrentSeatIndex;
	EventContext.Phase = CurrentPhase;
	UE_LOG(LogSGSTurn, Log, TEXT("SGS event: %s seat=%d phase=%s"),
		*Event.ToString(),
		CurrentSeatIndex,
		*CurrentPhase.ToString());
	GameEventDelegate.Broadcast(EventContext);
}

FSGSPhase USGSGameDriver::NextPhase(FSGSPhase Phase)
{
	if (SGSMatchesExactTag(Phase, SGSGameplayTags::Phase_RoundStart))
	{
		return SGSGameplayTags::Phase_Judge.GetTag();
	}
	if (SGSMatchesExactTag(Phase, SGSGameplayTags::Phase_Judge))
	{
		return SGSGameplayTags::Phase_Draw.GetTag();
	}
	if (SGSMatchesExactTag(Phase, SGSGameplayTags::Phase_Draw))
	{
		return SGSGameplayTags::Phase_Play.GetTag();
	}
	if (SGSMatchesExactTag(Phase, SGSGameplayTags::Phase_Play))
	{
		return SGSGameplayTags::Phase_Discard.GetTag();
	}
	if (SGSMatchesExactTag(Phase, SGSGameplayTags::Phase_Discard))
	{
		return SGSGameplayTags::Phase_RoundEnd.GetTag();
	}

	return SGSGameplayTags::Phase_RoundEnd.GetTag();
}

FSGSCommandId USGSGameDriver::AllocateCommandId()
{
	return FSGSCommandId(++NextCommandIdValue);
}
