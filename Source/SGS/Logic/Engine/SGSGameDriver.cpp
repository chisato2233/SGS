#include "Logic/Engine/SGSGameDriver.h"

#include "Logic/Engine/SGSGameContext.h"
#include "Logic/Players/SGSSeat.h"
#include "Core/SGSLogChannels.h"

void USGSGameDriver::StartGame(const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents, int32 RandomSeed)
{
	Context = NewObject<USGSGameContext>(this);
	Context->Initialize(InAgents, RandomSeed);

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

	// 发起始手牌（牌堆为空时不发牌；牌库数据接入后自然生效）。
	for (int32 SeatIndex = 0; SeatIndex < Context->NumSeats(); ++SeatIndex)
	{
		Context->DrawCards(SeatIndex, StartingHandSize);
	}

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
		Context->DrawCards(CurrentSeatIndex, DrawCountPerTurn);
		AdvanceAfterPhase();
		return;
	}

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_Play))
	{
		const USGSSeat* Seat = Context->GetSeat(CurrentSeatIndex);
		ISGSDecisionAgent* Agent = Seat != nullptr ? Seat->DecisionAgent.GetInterface() : nullptr;
		if (Agent == nullptr)
		{
			UE_LOG(LogSGSTurn, Warning, TEXT("Seat %d has no decision agent; treating as pass."), CurrentSeatIndex);
			AdvanceAfterPhase();
			return;
		}

		bWaitingForDecision = true;

		FSGSPlayPhaseRequest Request;
		Request.SeatIndex = CurrentSeatIndex;
		Request.RequestId = ++PendingRequestId;

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

	// 骨架期仅支持 Pass。
	UE_LOG(LogSGSTurn, Verbose, TEXT("Seat %d play action resolved (%s)."), CurrentSeatIndex, *Decision.Action.ToString());

	AdvanceAfterPhase();
	Pump();
}

void USGSGameDriver::AdvanceAfterPhase()
{
	Broadcast(SGSGameplayTags::GameEvent_PhaseEnded.GetTag());

	if (SGSMatchesExactTag(CurrentPhase, SGSGameplayTags::Phase_RoundEnd))
	{
		Broadcast(SGSGameplayTags::GameEvent_TurnEnded.GetTag());
		++TurnsPlayed;

		if (TurnsPlayed >= MaxTurnsForSkeleton)
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
