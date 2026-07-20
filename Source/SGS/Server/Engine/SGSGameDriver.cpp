#include "Server/Engine/SGSGameDriver.h"

#include "Shared/Cards/SGSCard.h"
#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"
#include "Server/Engine/SGSGameContext.h"
#include "Server/Players/SGSSeat.h"
#include "Server/AI/SGSBasicAIAgent.h"
#include "Shared/Core/SGSLogChannels.h"
#include "Server/Effects/SGSStandardEffectSteps.h"
#include "Server/Rules/Responses/BasicCards/SGSDyingPeachRules.h"

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

		FSGSResponseRequest Request = Driver.MakeResponseRequest(
			Spec.SeatIndex,
			Spec.WindowName,
			Spec.RequiredCardName,
			Spec.ContextName,
			Spec.EffectSourceSeat,
			Spec.EffectTargetSeat,
			Spec.SkillOptions,
			Spec.AcceptedCardNames);
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
	CommandRouter.Reset();
	AIEvaluatorRegistry.Reset();
	SGSBasicAIEvaluators::Register(AIEvaluatorRegistry);
	RuleRegistry.Reset();
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

	for (const TPair<int32, int32>& HealthOverride : Config.InitialHealthBySeat)
	{
		if (USGSSeat* Seat = Context->GetSeat(HealthOverride.Key))
		{
			Seat->Health = FMath::Clamp(HealthOverride.Value, 0, Seat->MaxHealth);
		}
	}

	BuildInitialDeck(Config.InitialDeck, Config.bShuffleInitialDeck);

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
	Pump();
}

void USGSGameDriver::BeginTurn()
{
	TurnSeatIndex = CurrentSeatIndex;
	CurrentPhase = SGSGameplayTags::Phase_RoundStart.GetTag();
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
	Broadcast(SGSGameplayTags::GameEvent_PhaseBegan.GetTag());

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Draw))
	{
		ExecuteDrawPhaseThroughPipeline();
		AdvanceAfterPhase();
		return;
	}

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Play))
	{
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

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Discard))
	{
		ExecuteDiscardPhase();
		AdvanceAfterPhase();
		return;
	}

	// 回合开始、判定和回合结束在最小 Demo 中无额外结算。
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
	HandQuery.RequiredCardType = SGSGameplayTags::CardType_Basic.GetTag();
	const FSGSCardQueryResult HandCards = Context->QueryCards(HandQuery);

	FSGSSeatQuery SlashTargetQuery;
	SlashTargetQuery.SourceSeat = CurrentSeatIndex;
	SlashTargetQuery.MaxDistance = 1;
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
	const bool bSlashUsed = HasStatus(SGSGameplayTags::Status_SlashUsed.GetTag());
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
		if (Option.CardName == TEXT("Slash") && !bSlashUsed)
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
		if ((Option.CardName == TEXT("Slash") && !bSlashUsed)
			|| (Option.CardName == TEXT("Peach") && !Option.TargetSeatIndices.IsEmpty())
			|| (Option.CardName == TEXT("Analeptic") && !bAnalepticUsed))
		{
			Request.Options.Add(MoveTemp(Option));
		}
	}

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
	TConstArrayView<FName> AcceptedCardNames)
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
	Request.SkillOptions.Append(SkillOptions.GetData(), SkillOptions.Num());

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

	CurrentSeatIndex = ActionSeat;
	if (!bGameOver && !SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Play))
	{
		AdvanceAfterPhase();
	}

	return MakeValue();
}

FSGSStatus USGSGameDriver::ResumeResolutionParentAfterChild(const FSGSResolutionFrame& CompletedFrame)
{
	(void)CompletedFrame;

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

	return MakeValue();
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

	FSGSStatus Status = RuleRegistry.DispatchAll(RuleContext);
	SyncReplayLog();
	return Status;
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

void USGSGameDriver::AdvanceAfterPhase()
{
	Broadcast(SGSGameplayTags::GameEvent_PhaseEnded.GetTag());
	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Play))
	{
		ExpireStatusEffects(CurrentSeatIndex, SGSGameplayTags::Status_SlashUsed.GetTag());
	}

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_RoundEnd))
	{
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
}

void USGSGameDriver::DiscardAllCards(int32 SeatIndex)
{
	TArray<USGSCard*> Hand = Context->GetCardsInZone(SGSGameplayTags::CardZone_Hand.GetTag(), SeatIndex);
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
		return;
	}

	TArray<USGSCard*> Discarded;
	Discarded.Append(Hand.GetData() + Hand.Num() - ExcessCount, ExcessCount);
	RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		MoveTemp(Discarded),
		SGSGameplayTags::CardZone_Hand.GetTag(),
		CurrentSeatIndex,
		SGSGameplayTags::CardZone_DiscardPile.GetTag(),
		INDEX_NONE,
		{ SGSCardMoveReasons::Discard(), {} }));
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
