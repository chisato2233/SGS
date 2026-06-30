#include "Logic/Engine/SGSGameDriver.h"

#include "Logic/Cards/SGSCard.h"
#include "Logic/Engine/SGSGameContext.h"
#include "Logic/Players/SGSSeat.h"
#include "Core/SGSLogChannels.h"
#include "Logic/Effects/SGSStandardEffectSteps.h"

void USGSGameDriver::StartGame(const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents, int32 RandomSeed)
{
	FSGSGameStartConfig Config;
	Config.RandomSeed = RandomSeed;
	StartGame(InAgents, Config);
}

void USGSGameDriver::StartGame(const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents, const FSGSGameStartConfig& Config)
{
	Context = NewObject<USGSGameContext>(this);
	Context->Initialize(InAgents, Config.RandomSeed);

	if (Context->NumSeats() == 0)
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("StartGame: no seats provided; nothing to run."));
		bGameOver = true;
		return;
	}

	bGameOver = false;
	bWaitingForDecision = false;
	TurnsPlayed = 0;
	CurrentSeatIndex = 0;
	PendingRequestId = 0;
	PendingCommandId = FSGSCommandId();
	NextCommandIdValue = 0;
	NextTimingSequence = 0;
	PendingWindowName = NAME_None;
	PendingRequiredCardName = NAME_None;
	PendingEffectSourceSeat = INDEX_NONE;
	PendingEffectTargetSeat = INDEX_NONE;
	PendingSlashCard = nullptr;
	PendingDyingResponders.Reset();
	PendingDyingResponderIndex = INDEX_NONE;
	CurrentMaxTurns = FMath::Max(Config.MaxTurns, 1);
	CurrentStartingHandSize = FMath::Max(Config.StartingHandSize, 0);
	CommandRouter.Reset();
	EffectPipeline.Reset();
	ActiveEffectTimeline.Reset();
	ReplayLog.Reset();

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
		EffectPipeline.Enqueue(SGSStandardEffectSteps::MakeDrawCardsStep(SeatIndex, CurrentStartingHandSize));
		FSGSEffectContext EffectContext = MakeEffectContext();
		if (FSGSStatus Status = EffectPipeline.Run(EffectContext); Status.HasError())
		{
			UE_LOG(LogSGSTurn, Error, TEXT("Starting hand effect pipeline failed: %s"), *Status.GetError().ToLogString());
		}
		SyncReplayLog();
	}
	CurrentSeatIndex = 0;

	Broadcast(SGSGameplayTags::GameEvent_GameStarted.GetTag());
	BeginTurn();
	Pump();
}

void USGSGameDriver::BeginTurn()
{
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
		PendingWindowName = NAME_None;
		PendingRequiredCardName = NAME_None;
		PendingEffectSourceSeat = INDEX_NONE;
		PendingEffectTargetSeat = INDEX_NONE;

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
			SubmitCommandWithFallback(FallbackCommand);
			PendingCommandId = FSGSCommandId();
			AdvanceAfterPhase();
			return;
		}

		bWaitingForDecision = true;

		FSGSPlayPhaseDecisionDelegate OnDecided;
		OnDecided.BindUObject(this, &USGSGameDriver::OnPlayActionDecided);

		// 异步：代理可能同步（AI）或跨帧（真人）回调 OnPlayActionDecided。
		Agent->RequestPlayPhaseAction(Request, OnDecided);
		return;
	}

	// 其余阶段（回合开始/判定/弃牌/回合结束）在本期无结算内容，立即结束并推进。
	// 判定/弃牌限制等将在后续 Plan 接入。
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
	if (SubmitCommandWithFallback(Decision.Command))
	{
		ResolvePlayPhaseCommand(Decision.Command);
	}
	else
	{
		PendingCommandId = FSGSCommandId();
		AdvanceAfterPhase();
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
	if (SubmitCommandWithFallback(Decision.Command))
	{
		ResolveResponseCommand(Decision.Command);
	}
	else
	{
		ResolveResponseCommand(MakeFallbackPassCommand(TEXT("InvalidResponseCommand")));
	}
	Pump();
}

bool USGSGameDriver::SubmitCommandWithFallback(const FSGSCommand& Command)
{
	const FSGSCommandExecutionContext ExecutionContext = MakeCommandExecutionContext();
	if (FSGSStatus Status = CommandRouter.SubmitCommand(Command, ExecutionContext); Status.HasError())
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("Invalid play command: %s. Falling back to pass."),
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
		}
		SyncReplayLog();
		return false;
	}
	SyncReplayLog();
	return true;
}

FSGSCommandExecutionContext USGSGameDriver::MakeCommandExecutionContext() const
{
	FSGSCommandExecutionContext ExecutionContext;
	ExecutionContext.GameContext = Context;
	ExecutionContext.ExpectedCommandId = PendingCommandId;
	ExecutionContext.ExpectedRequestId = PendingRequestId;
	ExecutionContext.ExpectedSeatIndex = CurrentSeatIndex;
	ExecutionContext.ExpectedPhase = CurrentPhase;
	ExecutionContext.ExpectedWindowName = PendingWindowName;
	ExecutionContext.RequiredCardName = PendingRequiredCardName;
	ExecutionContext.EffectTargetSeatIndex = PendingEffectTargetSeat;
	return ExecutionContext;
}

FSGSCommand USGSGameDriver::MakeFallbackPassCommand(FName Reason) const
{
	return FSGSCommand::MakePass(
		PendingCommandId,
		PendingRequestId,
		CurrentSeatIndex,
		CurrentPhase,
		FName(TEXT("ServerFallback")),
		Reason);
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

	for (const FSGSCardTarget& CardTarget : HandCards.Targets)
	{
		if (CardTarget.Card == nullptr)
		{
			continue;
		}

		FSGSCardActionOption Option;
		Option.CardId = CardTarget.CardId;
		Option.CardName = CardTarget.Card->CardName;
		if (Option.CardName == TEXT("Slash"))
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

		if (Option.CardName == TEXT("Slash") || Option.CardName == TEXT("Peach"))
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
	int32 EffectSourceSeat,
	int32 EffectTargetSeat)
{
	FSGSResponseRequest Request;
	Request.CommandId = AllocateCommandId();
	Request.SeatIndex = SeatIndex;
	Request.RequestId = ++PendingRequestId;
	Request.Phase = CurrentPhase;
	Request.WindowName = WindowName;
	Request.RequiredCardName = RequiredCardName;
	Request.EffectSourceSeat = EffectSourceSeat;
	Request.EffectTargetSeat = EffectTargetSeat;

	FSGSCardQuery Query;
	Query.Zone = SGSGameplayTags::CardZone_Hand.GetTag();
	Query.SeatIndex = SeatIndex;
	Query.ViewerSeatIndex = SeatIndex;
	Query.RequiredCardName = RequiredCardName;
	const FSGSCardQueryResult Cards = Context->QueryCards(Query);
	for (const FSGSCardTarget& Target : Cards.Targets)
	{
		Request.ResponseCardIds.Add(Target.CardId);
	}

	return Request;
}

void USGSGameDriver::ResolvePlayPhaseCommand(const FSGSCommand& Command)
{
	PendingEffectSourceSeat = Command.SeatIndex;

	if (Command.IsType(SGSGameplayTags::PlayAction_Pass))
	{
		PendingCommandId = FSGSCommandId();
		AdvanceAfterPhase();
		return;
	}

	if (!Command.IsType(SGSGameplayTags::PlayAction_UseCard))
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("Unsupported play command after router validation: %s"), *Command.ToLogString());
		PendingCommandId = FSGSCommandId();
		AdvanceAfterPhase();
		return;
	}

	const FSGSCardState* State = Context->FindCardStateById(Command.CardIds.Num() > 0 ? Command.CardIds[0] : INDEX_NONE);
	if (State == nullptr)
	{
		PendingCommandId = FSGSCommandId();
		AdvanceAfterPhase();
		return;
	}

	if (State->CardName == TEXT("Slash"))
	{
		ResolveSlashCommand(Command);
		return;
	}

	if (State->CardName == TEXT("Peach"))
	{
		const int32 HealTarget = Command.TargetSeatIndices.Num() > 0 ? Command.TargetSeatIndices[0] : Command.SeatIndex;
		ResolvePeachCommand(Command, HealTarget);
		FinishPendingCardResolution();
		return;
	}

	UE_LOG(LogSGSTurn, Warning, TEXT("Unsupported basic card command for card %s."), *State->CardName.ToString());
	FinishPendingCardResolution();
}

void USGSGameDriver::ResolveResponseCommand(const FSGSCommand& Command)
{
	if (PendingWindowName == TEXT("Slash.Dodge"))
	{
		ResolveSlashDodgeResponse(Command);
		return;
	}

	if (PendingWindowName == TEXT("Dying.Peach"))
	{
		ResolveDyingPeachResponse(Command);
		return;
	}

	UE_LOG(LogSGSTurn, Warning, TEXT("No pending response window for command: %s"), *Command.ToLogString());
	FinishPendingCardResolution();
}

void USGSGameDriver::ResolveSlashCommand(const FSGSCommand& Command)
{
	if (Command.TargetSeatIndices.Num() != 1)
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("Slash command requires exactly one target."));
		FinishPendingCardResolution();
		return;
	}

	USGSCard* SlashCard = Context->FindCardById(Command.CardIds[0]);
	const int32 TargetSeat = Command.TargetSeatIndices[0];
	if (SlashCard == nullptr || Context->GetSeat(TargetSeat) == nullptr || TargetSeat == Command.SeatIndex)
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("Slash command has invalid card or target."));
		FinishPendingCardResolution();
		return;
	}

	if (Context->GetDistance(Command.SeatIndex, TargetSeat) > 1)
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("Slash target is out of distance."));
		FinishPendingCardResolution();
		return;
	}

	PendingSlashCard = SlashCard;
	PendingEffectSourceSeat = Command.SeatIndex;
	PendingEffectTargetSeat = TargetSeat;
	RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ SlashCard },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		Command.SeatIndex,
		SGSGameplayTags::CardZone_Processing.GetTag(),
		INDEX_NONE),
		Command.CommandId);
	RequestSlashDodge(Command.SeatIndex, TargetSeat);
}

void USGSGameDriver::ResolvePeachCommand(const FSGSCommand& Command, int32 HealTargetSeat)
{
	USGSCard* PeachCard = Context->FindCardById(Command.CardIds.Num() > 0 ? Command.CardIds[0] : INDEX_NONE);
	if (PeachCard == nullptr || PeachCard->CardName != TEXT("Peach") || Context->GetSeat(HealTargetSeat) == nullptr)
	{
		UE_LOG(LogSGSTurn, Warning, TEXT("Invalid Peach command."));
		return;
	}

	RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
		TArray<USGSCard*>{ PeachCard },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		Command.SeatIndex,
		SGSGameplayTags::CardZone_DiscardPile.GetTag(),
		INDEX_NONE),
		Command.CommandId);
	RunEffectStep(SGSStandardEffectSteps::MakeHealStep(HealTargetSeat, 1), Command.CommandId);
}

void USGSGameDriver::RequestSlashDodge(int32 SourceSeat, int32 TargetSeat)
{
	USGSSeat* Target = Context->GetSeat(TargetSeat);
	ISGSDecisionAgent* Agent = Target != nullptr ? Target->DecisionAgent.GetInterface() : nullptr;
	if (Agent == nullptr)
	{
		ApplySlashDamageOrDying();
		return;
	}

	FSGSResponseRequest Request = MakeResponseRequest(TargetSeat, TEXT("Slash.Dodge"), TEXT("Dodge"), SourceSeat, TargetSeat);
	PendingCommandId = Request.CommandId;
	PendingWindowName = Request.WindowName;
	PendingRequiredCardName = Request.RequiredCardName;
	PendingEffectSourceSeat = SourceSeat;
	PendingEffectTargetSeat = TargetSeat;
	CurrentSeatIndex = TargetSeat;
	bWaitingForDecision = true;

	FSGSResponseDecisionDelegate OnDecided;
	OnDecided.BindUObject(this, &USGSGameDriver::OnResponseActionDecided);
	Agent->RequestResponseAction(Request, OnDecided);
}

void USGSGameDriver::ResolveSlashDodgeResponse(const FSGSCommand& Command)
{
	if (Command.IsType(SGSGameplayTags::PlayAction_RespondCard))
	{
		USGSCard* DodgeCard = Context->FindCardById(Command.CardIds.Num() > 0 ? Command.CardIds[0] : INDEX_NONE);
		if (DodgeCard != nullptr && DodgeCard->CardName == TEXT("Dodge"))
		{
			RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
				TArray<USGSCard*>{ DodgeCard },
				SGSGameplayTags::CardZone_Hand.GetTag(),
				Command.SeatIndex,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE),
				Command.CommandId);
			if (PendingSlashCard != nullptr)
			{
				RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
					TArray<USGSCard*>{ PendingSlashCard.Get() },
					SGSGameplayTags::CardZone_Processing.GetTag(),
					INDEX_NONE,
					SGSGameplayTags::CardZone_DiscardPile.GetTag(),
					INDEX_NONE),
					Command.CommandId);
			}
			FinishPendingCardResolution();
			return;
		}
	}

	ApplySlashDamageOrDying();
}

void USGSGameDriver::ApplySlashDamageOrDying()
{
	if (PendingSlashCard != nullptr)
	{
		RunEffectStep(SGSStandardEffectSteps::MakeMoveCardsStep(
			TArray<USGSCard*>{ PendingSlashCard.Get() },
			SGSGameplayTags::CardZone_Processing.GetTag(),
			INDEX_NONE,
			SGSGameplayTags::CardZone_DiscardPile.GetTag(),
			INDEX_NONE),
			PendingCommandId);
	}

	RunEffectStep(SGSStandardEffectSteps::MakeDamageStep(PendingEffectSourceSeat, PendingEffectTargetSeat, 1), PendingCommandId);
	const USGSSeat* DamagedSeat = Context->GetSeat(PendingEffectTargetSeat);
	if (DamagedSeat != nullptr && DamagedSeat->bIsAlive && DamagedSeat->Health <= 0)
	{
		BeginDyingPeachWindow(PendingEffectTargetSeat);
		return;
	}

	FinishPendingCardResolution();
}

void USGSGameDriver::BeginDyingPeachWindow(int32 DyingSeat)
{
	PendingWindowName = TEXT("Dying.Peach");
	PendingRequiredCardName = TEXT("Peach");
	PendingEffectTargetSeat = DyingSeat;
	PendingDyingResponders.Reset();
	PendingDyingResponderIndex = 0;

	if (Context->GetSeat(DyingSeat) != nullptr)
	{
		PendingDyingResponders.Add(DyingSeat);
	}
	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		if (SeatIndex != DyingSeat)
		{
			const USGSSeat* Seat = Context->GetSeat(SeatIndex);
			if (Seat != nullptr && Seat->bIsAlive)
			{
				PendingDyingResponders.Add(SeatIndex);
			}
		}
	}

	RequestNextDyingPeachResponder();
}

void USGSGameDriver::RequestNextDyingPeachResponder()
{
	const USGSSeat* DyingSeat = Context->GetSeat(PendingEffectTargetSeat);
	if (DyingSeat == nullptr || !DyingSeat->bIsAlive || DyingSeat->Health > 0)
	{
		FinishPendingCardResolution();
		return;
	}

	while (PendingDyingResponders.IsValidIndex(PendingDyingResponderIndex))
	{
		const int32 ResponderSeat = PendingDyingResponders[PendingDyingResponderIndex++];
		USGSSeat* Responder = Context->GetSeat(ResponderSeat);
		ISGSDecisionAgent* Agent = Responder != nullptr ? Responder->DecisionAgent.GetInterface() : nullptr;
		if (Responder == nullptr || !Responder->bIsAlive || Agent == nullptr)
		{
			continue;
		}

		FSGSResponseRequest Request = MakeResponseRequest(ResponderSeat, TEXT("Dying.Peach"), TEXT("Peach"), PendingEffectSourceSeat, PendingEffectTargetSeat);
		PendingCommandId = Request.CommandId;
		PendingWindowName = Request.WindowName;
		PendingRequiredCardName = Request.RequiredCardName;
		CurrentSeatIndex = ResponderSeat;
		bWaitingForDecision = true;

		FSGSResponseDecisionDelegate OnDecided;
		OnDecided.BindUObject(this, &USGSGameDriver::OnResponseActionDecided);
		Agent->RequestResponseAction(Request, OnDecided);
		return;
	}

	Context->EliminateSeat(PendingEffectTargetSeat, TEXT("NoPeach"));
	FinishPendingCardResolution();
}

void USGSGameDriver::ResolveDyingPeachResponse(const FSGSCommand& Command)
{
	if (Command.IsType(SGSGameplayTags::PlayAction_RespondCard))
	{
		ResolvePeachCommand(Command, PendingEffectTargetSeat);
		const USGSSeat* DyingSeat = Context->GetSeat(PendingEffectTargetSeat);
		if (DyingSeat != nullptr && DyingSeat->Health > 0)
		{
			FinishPendingCardResolution();
			return;
		}
	}

	RequestNextDyingPeachResponder();
}

void USGSGameDriver::FinishPendingCardResolution()
{
	const int32 ActionSeat = PendingEffectSourceSeat != INDEX_NONE ? PendingEffectSourceSeat : CurrentSeatIndex;
	PendingCommandId = FSGSCommandId();
	PendingWindowName = NAME_None;
	PendingRequiredCardName = NAME_None;
	PendingEffectSourceSeat = INDEX_NONE;
	PendingEffectTargetSeat = INDEX_NONE;
	PendingSlashCard = nullptr;
	PendingDyingResponders.Reset();
	PendingDyingResponderIndex = INDEX_NONE;
	CurrentSeatIndex = ActionSeat;
	if (!bGameOver)
	{
		AdvanceAfterPhase();
	}
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

void USGSGameDriver::RunEffectStep(FSGSEffectStep Step, FSGSCommandId CommandId)
{
	EffectPipeline.Reset();
	EffectPipeline.Enqueue(MoveTemp(Step));
	FSGSEffectContext EffectContext = MakeEffectContext(CommandId);
	if (FSGSStatus Status = EffectPipeline.Run(EffectContext); Status.HasError())
	{
		UE_LOG(LogSGSTurn, Error, TEXT("Effect step failed: %s"), *Status.GetError().ToLogString());
	}
	SyncReplayLog();
}

void USGSGameDriver::SyncReplayLog()
{
	ReplayLog.SetCommandLog(CommandRouter.GetLogEntries());
	if (Context != nullptr)
	{
		ReplayLog.SetRandomLog(Context->GetRandomAudit().GetEntries());
	}
}

void USGSGameDriver::AdvanceAfterPhase()
{
	Broadcast(SGSGameplayTags::GameEvent_PhaseEnded.GetTag());

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_RoundEnd))
	{
		Broadcast(SGSGameplayTags::GameEvent_TurnEnded.GetTag());
		++TurnsPlayed;

		if (TurnsPlayed >= CurrentMaxTurns)
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

void USGSGameDriver::EndGame()
{
	bGameOver = true;
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
