#include "Server/Engine/SGSGameContext.h"

#include "Shared/Cards/SGSCard.h"
#include "Server/Players/SGSSeat.h"
#include "Server/Content/SGSStandardGenerals.h"
#include "Shared/Core/SGSLogChannels.h"

void USGSGameContext::Initialize(
	const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents,
	int32 RandomSeed,
	bool bIdentityMode,
	TConstArrayView<FName> GeneralIdsBySeat,
	TConstArrayView<FGameplayTag> FactionsBySeat)
{
	RandomAudit.Initialize(RandomSeed);
	NextCardId = 0;
	AllCards.Reset();
	RegisterCardIndexes();

	TArray<TScriptInterface<ISGSDecisionAgent>> SeatAgents = InAgents;
	TArray<FGameplayTag> Identities;
	if (bIdentityMode)
	{
		check(SeatAgents.Num() == 8);
		Identities = {
			SGSGameplayTags::Identity_Lord.GetTag(),
			SGSGameplayTags::Identity_Loyalist.GetTag(),
			SGSGameplayTags::Identity_Loyalist.GetTag(),
			SGSGameplayTags::Identity_Rebel.GetTag(),
			SGSGameplayTags::Identity_Rebel.GetTag(),
			SGSGameplayTags::Identity_Rebel.GetTag(),
			SGSGameplayTags::Identity_Rebel.GetTag(),
			SGSGameplayTags::Identity_Renegade.GetTag(),
		};
		for (int32 Index = SeatAgents.Num() - 1; Index > 0; --Index)
		{
			SeatAgents.Swap(Index, RandomAudit.RandRange(
				TEXT("SGS.Random.SeatAssignment"), 0, Index, TEXT("IdentityMode")));
			Identities.Swap(Index, RandomAudit.RandRange(
				TEXT("SGS.Random.IdentityAssignment"), 0, Index, TEXT("IdentityMode")));
		}
	}

	Seats.Reset();
	for (int32 Index = 0; Index < SeatAgents.Num(); ++Index)
	{
		USGSSeat* Seat = NewObject<USGSSeat>(this);
		Seat->SeatIndex = Index;
		Seat->DisplayName = FString::Printf(TEXT("Seat%d"), Index);
		Seat->GeneralId = GeneralIdsBySeat.IsValidIndex(Index) ? GeneralIdsBySeat[Index] : NAME_None;
		const FSGSGeneralDefinition* General = SGSStandardGenerals::Find(Seat->GeneralId);
		Seat->Faction = FactionsBySeat.IsValidIndex(Index)
			? FactionsBySeat[Index]
			: General != nullptr ? General->Faction : FGameplayTag();
		Seat->SkillNames = General != nullptr ? General->SkillNames : TArray<FName>();
		Seat->DecisionAgent = SeatAgents[Index];
		Seat->Identity = Identities.IsValidIndex(Index) ? Identities[Index] : FGameplayTag();
		const int32 GeneralMaxHealth = General != nullptr ? General->MaxHealth : DefaultMaxHealth;
		Seat->MaxHealth = Seat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
			? GeneralMaxHealth + 1
			: GeneralMaxHealth;
		Seat->Health = Seat->MaxHealth;
		Seat->bIsAlive = true;
		Seats.Add(Seat);
	}

	RebuildSeatIndexes();
}

USGSSeat* USGSGameContext::GetSeat(int32 Index) const
{
	return Seats.IsValidIndex(Index) ? Seats[Index] : nullptr;
}

USGSCard* USGSGameContext::FindCardById(int32 CardId) const
{
	const FSGSCardState* State = FindCardStateById(CardId);
	return State != nullptr ? State->Card : nullptr;
}

const FSGSCardState* USGSGameContext::FindCardStateById(int32 CardId) const
{
	if (CardId == INDEX_NONE || !CardsById.IsValid())
	{
		return nullptr;
	}

	const FSGSStableHandle Handle = CardsById->Find(CardId);
	return Handle.IsValid() ? CardStore.Find(Handle) : nullptr;
}

TArray<USGSCard*> USGSGameContext::GetCardsInZone(FSGSCardZone Zone, int32 SeatIndex) const
{
	return GetCardsInPile(MakePileKey(Zone, SeatIndex));
}

void USGSGameContext::RegisterCardIndexes()
{
	CardStore.Reset();
	CardsById = CardStore.RegisterUniqueIndex<int32>(
		FName(TEXT("CardId")),
		[](const FSGSCardState& State) { return State.CardId; });
	CardsByOwnerSeat = CardStore.RegisterNonUniqueIndex<int32>(
		FName(TEXT("OwnerSeat")),
		[](const FSGSCardState& State) { return State.OwnerSeat; });
	CardsByZone = CardStore.RegisterNonUniqueIndex<FSGSCardZone>(
		FName(TEXT("Zone")),
		[](const FSGSCardState& State) { return State.Zone; });
	CardsByPile = CardStore.RegisterOrderedIndex<FSGSCardPileKey>(
		FName(TEXT("PileOrder")),
		[](const FSGSCardState& State) { return State.GetPileKey(); },
		[](const FSGSCardState& State) { return State.ZoneOrder; });
	CardsByName = CardStore.RegisterNonUniqueIndex<FName>(
		FName(TEXT("CardName")),
		[](const FSGSCardState& State) { return State.CardName; });
	CardsByType = CardStore.RegisterNonUniqueIndex<FSGSCardType>(
		FName(TEXT("CardType")),
		[](const FSGSCardState& State) { return State.CardType; });
	CardsBySuit = CardStore.RegisterNonUniqueIndex<FSGSSuit>(
		FName(TEXT("Suit")),
		[](const FSGSCardState& State) { return State.Suit; });
	CardsByNumber = CardStore.RegisterNonUniqueIndex<int32>(
		FName(TEXT("Number")),
		[](const FSGSCardState& State) { return State.Number; });
	CardsByOwnerZone = CardStore.RegisterNonUniqueIndex<FSGSCardPileKey>(
		FName(TEXT("OwnerZone")),
		[](const FSGSCardState& State) { return State.GetPileKey(); });
	CardsByZoneName = CardStore.RegisterNonUniqueIndex<FSGSCardZoneNameKey>(
		FName(TEXT("ZoneCardName")),
		[](const FSGSCardState& State) { return FSGSCardZoneNameKey(State.Zone, State.CardName); });
	CardsByZoneType = CardStore.RegisterNonUniqueIndex<FSGSCardZoneTypeKey>(
		FName(TEXT("ZoneCardType")),
		[](const FSGSCardState& State) { return FSGSCardZoneTypeKey(State.Zone, State.CardType); });
}

void USGSGameContext::RebuildSeatIndexes()
{
	AllSeatIndices.Reset(Seats.Num());
	AliveSeatIndices.Reset(Seats.Num());
	SeatIndicesByFaction.Empty();

	for (int32 SeatIndex = 0; SeatIndex < Seats.Num(); ++SeatIndex)
	{
		AllSeatIndices.Add(SeatIndex);

		const USGSSeat* Seat = Seats[SeatIndex];
		if (Seat == nullptr)
		{
			continue;
		}

		if (Seat->bIsAlive)
		{
			AliveSeatIndices.Add(SeatIndex);
		}
		if (Seat->Faction.IsValid())
		{
			SeatIndicesByFaction.Add(Seat->Faction, SeatIndex);
		}
	}
}

FSGSStableHandle USGSGameContext::MakeSeatHandle(int32 SeatIndex) const
{
	return IsValidSeat(SeatIndex) ? FSGSStableHandle(SeatIndex, 1) : FSGSStableHandle();
}

FSGSStableHandle USGSGameContext::MakeCardHandle(const USGSCard* Card) const
{
	if (Card == nullptr || Card->CardId == INDEX_NONE || !CardsById.IsValid())
	{
		return FSGSStableHandle();
	}

	return CardsById->Find(Card->CardId);
}

USGSCard* USGSGameContext::CreateCard(FName CardName, FSGSSuit Suit, int32 Number, FSGSCardType CardType)
{
	USGSCard* Card = NewObject<USGSCard>(this);
	Card->CardId = NextCardId++;
	Card->CardName = CardName;
	Card->Suit = Suit;
	Card->Number = Number;
	Card->CardType = CardType.IsValid() ? CardType : SGSGameplayTags::CardType_None.GetTag();

	AllCards.Add(Card);

	FSGSCardState State;
	State.Card = Card;
	State.CardId = Card->CardId;
	State.CardName = Card->CardName;
	State.CardType = Card->CardType;
	State.Suit = Card->Suit;
	State.Number = Card->Number;
	State.Zone = SGSGameplayTags::CardZone_DrawPile.GetTag();
	State.OwnerSeat = INDEX_NONE;
	State.ZoneOrder = CountCardsInPile(State.GetPileKey());
	CardStore.Add(MoveTemp(State));
	return Card;
}

void USGSGameContext::ShuffleDrawPile()
{
	ShufflePileInStore(
		FSGSCardPileKey(SGSGameplayTags::CardZone_DrawPile.GetTag(), INDEX_NONE),
		TEXT("ShuffleDrawPile"),
		TEXT("DrawPile"));
}

FSGSCardPileKey USGSGameContext::MakePileKey(FSGSCardZone Zone, int32 SeatIndex) const
{
	if (SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_DrawPile)
		|| SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_DiscardPile))
	{
		return FSGSCardPileKey(Zone, INDEX_NONE);
	}

	return FSGSCardPileKey(Zone, SeatIndex);
}

int32 USGSGameContext::CountCardsInPile(FSGSCardPileKey Key) const
{
	return GetCardHandlesInPile(Key).Num();
}

TArray<FSGSStableHandle> USGSGameContext::GetCardHandlesInPile(FSGSCardPileKey Key) const
{
	return CardsByPile.IsValid() ? CardsByPile->FindCopy(Key) : TArray<FSGSStableHandle>();
}

TArray<USGSCard*> USGSGameContext::GetCardsInPile(FSGSCardPileKey Key) const
{
	TArray<USGSCard*> Cards;
	const TArray<FSGSStableHandle> Handles = GetCardHandlesInPile(Key);
	Cards.Reserve(Handles.Num());
	for (const FSGSStableHandle& Handle : Handles)
	{
		const FSGSCardState* State = CardStore.Find(Handle);
		if (State != nullptr && State->Card != nullptr)
		{
			Cards.Add(State->Card);
		}
	}
	return Cards;
}

FSGSStatus USGSGameContext::NormalizePileOrder(FSGSCardPileKey Key)
{
	const TArray<FSGSStableHandle> Handles = GetCardHandlesInPile(Key);
	for (int32 Order = 0; Order < Handles.Num(); ++Order)
	{
		const FSGSStatus Status = CardStore.Modify(Handles[Order], [Order](FSGSCardState& State)
		{
			State.ZoneOrder = Order;
		});
		if (Status.HasError())
		{
			return Status;
		}
	}

	return MakeValue();
}

FSGSStatus USGSGameContext::MoveCardHandle(FSGSStableHandle Handle, FSGSCardZone ToZone, int32 ToSeat, int32 ToOrder)
{
	const FSGSCardPileKey ToKey = MakePileKey(ToZone, ToSeat);
	const FSGSStatus Status = CardStore.Modify(Handle, [ToKey, ToOrder](FSGSCardState& State)
	{
		State.Zone = ToKey.Zone;
		State.OwnerSeat = ToKey.SeatIndex;
		State.ZoneOrder = ToOrder;
	});

	if (Status.HasError())
	{
		return Status;
	}

	return MakeValue();
}

void USGSGameContext::ShufflePileInStore(FSGSCardPileKey Key, FName Purpose, FString Context)
{
	TArray<FSGSStableHandle> Handles = GetCardHandlesInPile(Key);
	const int32 LastIndex = Handles.Num() - 1;
	for (int32 Index = 0; Index < LastIndex; ++Index)
	{
		const int32 SwapIndex = RandomAudit.RandRange(
			Purpose,
			Index,
			LastIndex,
			FString::Printf(TEXT("%s Index=%d"), *Context, Index),
			FSGSCommandId(),
			FName(TEXT("SGS.RandomEvent.ShufflePile")));
		if (SwapIndex != Index)
		{
			Handles.Swap(Index, SwapIndex);
		}
	}

	for (int32 Order = 0; Order < Handles.Num(); ++Order)
	{
		const FSGSStatus Status = CardStore.Modify(Handles[Order], [Order](FSGSCardState& State)
		{
			State.ZoneOrder = Order;
		});
		ensureMsgf(!Status.HasError(), TEXT("ShufflePileInStore failed to update order: %s"),
			Status.HasError() ? *Status.GetError().ToLogString() : TEXT(""));
	}
}

void USGSGameContext::MoveCards(const TArray<USGSCard*>& Cards, FSGSCardZone FromZone, int32 FromSeat, FSGSCardZone ToZone, int32 ToSeat)
{
	if (Cards.Num() == 0)
	{
		return;
	}

	const FSGSCardPileKey FromKey = MakePileKey(FromZone, FromSeat);
	const FSGSCardPileKey ToKey = MakePileKey(ToZone, ToSeat);
	int32 NextToOrder = CountCardsInPile(ToKey);

	FSGSCardMoveInfo Info;
	Info.FromZone = FromZone;
	Info.FromSeat = FromSeat;
	Info.ToZone = ToZone;
	Info.ToSeat = ToSeat;
	Info.Cards.Reserve(Cards.Num());

	for (USGSCard* Card : Cards)
	{
		if (Card == nullptr)
		{
			continue;
		}

		const FSGSStableHandle Handle = MakeCardHandle(Card);
		const FSGSCardState* State = CardStore.Find(Handle);
		if (State == nullptr)
		{
			UE_LOG(LogSGSCard, Warning, TEXT("MoveCards skipped untracked card %d."), Card->CardId);
			continue;
		}

		if ((!FromZone.IsValid() || !State->Zone.MatchesTagExact(FromKey.Zone) || State->OwnerSeat != FromKey.SeatIndex)
			&& !SGSMatchesExactTag(FromZone, SGSGameplayTags::CardZone_None))
		{
			ensureMsgf(false,
				TEXT("MoveCards source mismatch for card %d: expected %s actual %s@%d."),
				Card->CardId,
				*FromKey.ToLogString(),
				*State->Zone.ToString(),
				State->OwnerSeat);
		}

		const FSGSStatus Status = MoveCardHandle(Handle, ToZone, ToSeat, NextToOrder++);
		if (Status.HasError())
		{
			UE_LOG(LogSGSCard, Warning, TEXT("MoveCards failed: %s"), *Status.GetError().ToLogString());
			continue;
		}

		Info.Cards.Add(Card);
	}

	if (SGSMatchesExactTag(FromZone, SGSGameplayTags::CardZone_Equipment)
		&& Seats.IsValidIndex(FromSeat))
	{
		USGSSeat* SourceSeat = Seats[FromSeat];
		for (auto It = SourceSeat->Equipment.CreateIterator(); It; ++It)
		{
			if (Info.Cards.Contains(It.Value().Get()))
			{
				It.RemoveCurrent();
			}
		}
	}

	NormalizePileOrder(FromKey);
	NormalizePileOrder(ToKey);

	UE_LOG(LogSGSCard, Verbose, TEXT("Moved %d card(s) from zone %s (seat %d) to zone %s (seat %d)."),
		Info.Cards.Num(), *FromZone.ToString(), FromSeat, *ToZone.ToString(), ToSeat);

	CardsMovedDelegate.Broadcast(Info);
}

TArray<USGSCard*> USGSGameContext::DrawCards(int32 SeatIndex, int32 Count)
{
	TArray<USGSCard*> Drawn;

	USGSSeat* Seat = GetSeat(SeatIndex);
	if (Seat == nullptr || Count <= 0)
	{
		return Drawn;
	}

	const FSGSCardPileKey DrawKey(SGSGameplayTags::CardZone_DrawPile.GetTag(), INDEX_NONE);
	while (Drawn.Num() < Count)
	{
		if (CountCardsInPile(DrawKey) == 0)
		{
			ReshuffleDiscardIntoDraw();
			if (CountCardsInPile(DrawKey) == 0)
			{
				break; // 牌堆与弃牌堆皆空，无牌可摸。
			}
		}

		const TArray<USGSCard*> DrawPileCards = GetCardsInPile(DrawKey);
		const int32 TakeCount = FMath::Min(Count - Drawn.Num(), DrawPileCards.Num());
		if (TakeCount <= 0)
		{
			break;
		}

		TArray<USGSCard*> Batch;
		Batch.Reserve(TakeCount);
		for (int32 DrawIndex = 0; DrawIndex < TakeCount; ++DrawIndex)
		{
			Batch.Add(DrawPileCards[DrawIndex]);
		}

		MoveCards(Batch, SGSGameplayTags::CardZone_DrawPile.GetTag(), INDEX_NONE, SGSGameplayTags::CardZone_Hand.GetTag(), SeatIndex);
		Drawn.Append(Batch);
	}

	UE_LOG(LogSGSCard, Verbose, TEXT("Seat %d drew %d card(s)."), SeatIndex, Drawn.Num());

	return Drawn;
}

void USGSGameContext::DiscardFromHand(int32 SeatIndex, const TArray<USGSCard*>& Cards)
{
	MoveCards(Cards, SGSGameplayTags::CardZone_Hand.GetTag(), SeatIndex, SGSGameplayTags::CardZone_DiscardPile.GetTag(), INDEX_NONE);
}

void USGSGameContext::ReshuffleDiscardIntoDraw()
{
	const FSGSCardPileKey DiscardKey(SGSGameplayTags::CardZone_DiscardPile.GetTag(), INDEX_NONE);
	const FSGSCardPileKey DrawKey(SGSGameplayTags::CardZone_DrawPile.GetTag(), INDEX_NONE);
	if (CountCardsInPile(DiscardKey) == 0)
	{
		return;
	}

	TArray<USGSCard*> Recycled = GetCardsInPile(DiscardKey);
	MoveCards(Recycled, SGSGameplayTags::CardZone_DiscardPile.GetTag(), INDEX_NONE, SGSGameplayTags::CardZone_DrawPile.GetTag(), INDEX_NONE);
	ShufflePileInStore(
		DrawKey,
		TEXT("ReshuffleDiscardIntoDraw"),
		FString::Printf(TEXT("Recycled=%d"), Recycled.Num()));

	UE_LOG(LogSGSCard, Log, TEXT("Reshuffled %d discard card(s) back into the draw pile."), Recycled.Num());
}

void USGSGameContext::ApplyDamage(int32 SourceSeat, int32 TargetSeat, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	USGSSeat* Target = GetSeat(TargetSeat);
	if (Target == nullptr || !Target->bIsAlive)
	{
		return;
	}

	Target->Health = FMath::Max(0, Target->Health - Amount);

	FSGSDamageInfo Info;
	Info.SourceSeat = SourceSeat;
	Info.TargetSeat = TargetSeat;
	Info.Amount = Amount;
	DamageDelegate.Broadcast(Info);
	HealthChangedDelegate.Broadcast(TargetSeat, Target->Health);

	UE_LOG(LogSGSCombat, Log, TEXT("Seat %d dealt %d damage to seat %d (HP now %d)."),
		SourceSeat, Amount, TargetSeat, Target->Health);

	if (Target->Health <= 0)
	{
		// 体力归零：进入濒死。求桃/真正死亡的结算流程见 Plan 0005；
		// 此处仅广播濒死，不在 0004 直接判定死亡。
		UE_LOG(LogSGSCombat, Log, TEXT("Seat %d is dying."), TargetSeat);
		SeatDyingDelegate.Broadcast(TargetSeat);
	}
}

void USGSGameContext::Heal(int32 SeatIndex, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	USGSSeat* Seat = GetSeat(SeatIndex);
	if (Seat == nullptr || !Seat->bIsAlive)
	{
		return;
	}

	Seat->Health = FMath::Min(Seat->MaxHealth, Seat->Health + Amount);
	HealthChangedDelegate.Broadcast(SeatIndex, Seat->Health);

	UE_LOG(LogSGSCombat, Log, TEXT("Seat %d healed %d (HP now %d)."), SeatIndex, Amount, Seat->Health);
}

void USGSGameContext::EliminateSeat(int32 SeatIndex, int32 SourceSeat, FName Reason)
{
	USGSSeat* Seat = GetSeat(SeatIndex);
	if (Seat == nullptr || !Seat->bIsAlive)
	{
		return;
	}

	Seat->Health = 0;
	Seat->bIsAlive = false;
	RebuildSeatIndexes();
	UE_LOG(LogSGSCombat, Log, TEXT("Seat %d is eliminated. Reason=%s"), SeatIndex, *Reason.ToString());
	HealthChangedDelegate.Broadcast(SeatIndex, Seat->Health);
	SeatEliminatedDelegate.Broadcast(SeatIndex, SourceSeat, Reason);
}

bool USGSGameContext::AssignGeneral(int32 SeatIndex, FName GeneralId)
{
	USGSSeat* Seat = GetSeat(SeatIndex);
	const FSGSGeneralDefinition* General = SGSStandardGenerals::Find(GeneralId);
	if (Seat == nullptr || General == nullptr)
	{
		return false;
	}
	Seat->GeneralId = General->GeneralId;
	Seat->Faction = General->Faction;
	Seat->SkillNames = General->SkillNames;
	Seat->MaxHealth = Seat->Identity.MatchesTagExact(SGSGameplayTags::Identity_Lord.GetTag())
		? General->MaxHealth + 1
		: General->MaxHealth;
	Seat->Health = Seat->MaxHealth;
	return true;
}

void USGSGameContext::EquipCard(int32 SeatIndex, USGSCard* Card, FSGSEquipSlot Slot)
{
	USGSSeat* Seat = GetSeat(SeatIndex);
	if (Seat == nullptr || Card == nullptr || !Slot.IsValid())
	{
		return;
	}

	if (TObjectPtr<USGSCard>* Existing = Seat->Equipment.Find(Slot))
	{
		USGSCard* ReplacedCard = Existing->Get();
		Seat->Equipment.Remove(Slot);
		if (ReplacedCard != nullptr)
		{
			MoveCards(
				{ ReplacedCard },
				SGSGameplayTags::CardZone_Equipment.GetTag(),
				SeatIndex,
				SGSGameplayTags::CardZone_DiscardPile.GetTag(),
				INDEX_NONE);
		}
	}

	MoveCards(
		{ Card },
		SGSGameplayTags::CardZone_Hand.GetTag(),
		SeatIndex,
		SGSGameplayTags::CardZone_Equipment.GetTag(),
		SeatIndex);
	Seat->Equipment.Add(Slot, Card);
}

int32 USGSGameContext::GetDistance(int32 FromSeat, int32 ToSeat) const
{
	if (!IsValidSeat(FromSeat) || !IsValidSeat(ToSeat) || FromSeat == ToSeat)
	{
		return 0;
	}

	const int32 SeatCount = Seats.Num();

	// 沿一个方向计步，只数存活座位；落到目标即停。
	auto CountAliveSteps = [this, SeatCount, FromSeat, ToSeat](int32 Direction) -> int32
	{
		int32 Steps = 0;
		int32 Cursor = FromSeat;
		for (int32 Guard = 0; Guard < SeatCount; ++Guard)
		{
			Cursor = (Cursor + Direction + SeatCount) % SeatCount;
			if (Seats[Cursor] != nullptr && Seats[Cursor]->bIsAlive)
			{
				++Steps;
			}
			if (Cursor == ToSeat)
			{
				return Steps;
			}
		}
		return Steps;
	};

	const int32 Clockwise = CountAliveSteps(+1);
	const int32 CounterClockwise = CountAliveSteps(-1);
	int32 Distance = FMath::Min(Clockwise, CounterClockwise);

	// 坐骑修正：目标的 +1 马增大被算距离；攻击者的 -1 马减小到他人距离。
	const USGSSeat* Target = Seats[ToSeat];
	const USGSSeat* Source = Seats[FromSeat];
	if (Target != nullptr && Target->Equipment.Contains(SGSGameplayTags::EquipSlot_DefenseHorse.GetTag()))
	{
		Distance += 1;
	}
	if (Source != nullptr && Source->Equipment.Contains(SGSGameplayTags::EquipSlot_OffenseHorse.GetTag()))
	{
		Distance -= 1;
	}

	return FMath::Max(1, Distance);
}

FSGSSeatQueryResult USGSGameContext::QuerySeats(const FSGSSeatQuery& Query) const
{
	FSGSSeatQueryResult Result;

	TArray<int32> CandidateSeats;
	if (Query.RequiredFaction.IsValid())
	{
		SeatIndicesByFaction.MultiFind(Query.RequiredFaction, CandidateSeats);
	}
	else
	{
		CandidateSeats = Query.bAliveOnly ? AliveSeatIndices : AllSeatIndices;
	}

	for (int32 SeatIndex : CandidateSeats)
	{
		const USGSSeat* Seat = Seats[SeatIndex];
		const FSGSStableHandle Handle = MakeSeatHandle(SeatIndex);

		auto Reject = [&Result, Handle, SeatIndex](FName Reason, FString Detail)
		{
			FSGSTargetQueryRejection Rejection;
			Rejection.Handle = Handle;
			Rejection.SeatIndex = SeatIndex;
			Rejection.Reason = Reason;
			Rejection.Detail = MoveTemp(Detail);
			Result.Rejections.Add(MoveTemp(Rejection));
		};

		if (Seat == nullptr)
		{
			Reject(TEXT("SGS.TargetQuery.NullSeat"), TEXT("Seat object is missing."));
			continue;
		}

		if (!Query.bIncludeSource && Query.SourceSeat != INDEX_NONE && SeatIndex == Query.SourceSeat)
		{
			Reject(TEXT("SGS.TargetQuery.SourceExcluded"), TEXT("Source seat is excluded."));
			continue;
		}

		if (Query.bAliveOnly && !Seat->bIsAlive)
		{
			Reject(TEXT("SGS.TargetQuery.DeadSeat"), TEXT("Seat is not alive."));
			continue;
		}

		if (Query.RequiredFaction.IsValid() && !Seat->Faction.MatchesTagExact(Query.RequiredFaction))
		{
			Reject(TEXT("SGS.TargetQuery.FactionMismatch"), FString::Printf(TEXT("Required=%s Actual=%s"),
				*Query.RequiredFaction.ToString(), *Seat->Faction.ToString()));
			continue;
		}

		int32 Distance = INDEX_NONE;
		if (Query.SourceSeat != INDEX_NONE)
		{
			Distance = GetDistance(Query.SourceSeat, SeatIndex);
			if (Query.MaxDistance != INDEX_NONE && Distance > Query.MaxDistance)
			{
				Reject(TEXT("SGS.TargetQuery.OutOfDistance"), FString::Printf(TEXT("Distance=%d Max=%d"), Distance, Query.MaxDistance));
				continue;
			}
		}

		FSGSSeatTarget Target;
		Target.Handle = Handle;
		Target.SeatIndex = SeatIndex;
		Target.Distance = Distance;
		Result.Targets.Add(Target);
	}

	return Result;
}

FSGSCardQueryResult USGSGameContext::QueryCards(const FSGSCardQuery& Query) const
{
	FSGSCardQueryResult Result;
	const bool bHasZone = Query.Zone.IsValid() && !SGSMatchesExactTag(Query.Zone, SGSGameplayTags::CardZone_None);

	if (bHasZone && Query.bRequireVisible && !CanViewCardZone(Query.Zone, Query.SeatIndex, Query.ViewerSeatIndex))
	{
		FSGSTargetQueryRejection Rejection;
		Rejection.SeatIndex = Query.SeatIndex;
		Rejection.Reason = TEXT("SGS.TargetQuery.ZoneNotVisible");
		Rejection.Detail = FString::Printf(TEXT("Zone=%s Viewer=%d"), *Query.Zone.ToString(), Query.ViewerSeatIndex);
		Result.Rejections.Add(MoveTemp(Rejection));
		return Result;
	}

	TArray<FSGSStableHandle> CandidateHandles;
	if (bHasZone && !Query.RequiredCardName.IsNone() && CardsByZoneName.IsValid())
	{
		CandidateHandles = CardsByZoneName->FindCopy(FSGSCardZoneNameKey(Query.Zone, Query.RequiredCardName));
	}
	else if (bHasZone && Query.RequiredCardType.IsValid() && CardsByZoneType.IsValid())
	{
		CandidateHandles = CardsByZoneType->FindCopy(FSGSCardZoneTypeKey(Query.Zone, Query.RequiredCardType));
	}
	else if (bHasZone)
	{
		CandidateHandles = GetCardHandlesInPile(MakePileKey(Query.Zone, Query.SeatIndex));
	}
	else if (!Query.RequiredCardName.IsNone() && CardsByName.IsValid())
	{
		CandidateHandles = CardsByName->FindCopy(Query.RequiredCardName);
	}
	else if (Query.RequiredCardType.IsValid() && CardsByType.IsValid())
	{
		CandidateHandles = CardsByType->FindCopy(Query.RequiredCardType);
	}
	else if (Query.RequiredSuit.IsValid() && CardsBySuit.IsValid())
	{
		CandidateHandles = CardsBySuit->FindCopy(Query.RequiredSuit);
	}
	else if (Query.RequiredNumber != INDEX_NONE && CardsByNumber.IsValid())
	{
		CandidateHandles = CardsByNumber->FindCopy(Query.RequiredNumber);
	}
	else
	{
		CardStore.ForEach([&CandidateHandles](FSGSStableHandle Handle, const FSGSCardState&)
		{
			CandidateHandles.Add(Handle);
		});
	}

	for (const FSGSStableHandle& Handle : CandidateHandles)
	{
		const FSGSCardState* State = CardStore.Find(Handle);
		if (State == nullptr || State->Card == nullptr)
		{
			FSGSTargetQueryRejection Rejection;
			Rejection.Handle = Handle;
			Rejection.SeatIndex = Query.SeatIndex;
			Rejection.Reason = TEXT("SGS.TargetQuery.NullCard");
			Rejection.Detail = FString::Printf(TEXT("Handle=%s"), *Handle.ToLogString());
			Result.Rejections.Add(MoveTemp(Rejection));
			continue;
		}

		auto RejectCard = [&Result, Handle, State](FName Reason, FString Detail)
		{
			FSGSTargetQueryRejection Rejection;
			Rejection.Handle = Handle;
			Rejection.SeatIndex = State->OwnerSeat;
			Rejection.CardId = State->CardId;
			Rejection.Reason = Reason;
			Rejection.Detail = MoveTemp(Detail);
			Result.Rejections.Add(MoveTemp(Rejection));
		};

		if (bHasZone && (!State->Zone.MatchesTagExact(Query.Zone) || State->OwnerSeat != MakePileKey(Query.Zone, Query.SeatIndex).SeatIndex))
		{
			RejectCard(TEXT("SGS.TargetQuery.ZoneMismatch"), FString::Printf(TEXT("Expected=%s Actual=%s"),
				*MakePileKey(Query.Zone, Query.SeatIndex).ToLogString(), *State->GetPileKey().ToLogString()));
			continue;
		}

		if (Query.bRequireVisible && !CanViewCardZone(State->Zone, State->OwnerSeat, Query.ViewerSeatIndex))
		{
			RejectCard(TEXT("SGS.TargetQuery.ZoneNotVisible"), FString::Printf(TEXT("Zone=%s Viewer=%d"),
				*State->Zone.ToString(), Query.ViewerSeatIndex));
			continue;
		}

		if (!Query.RequiredCardName.IsNone() && State->CardName != Query.RequiredCardName)
		{
			RejectCard(TEXT("SGS.TargetQuery.CardNameMismatch"), FString::Printf(TEXT("Required=%s Actual=%s"),
				*Query.RequiredCardName.ToString(), *State->CardName.ToString()));
			continue;
		}

		if (Query.RequiredCardType.IsValid() && !State->CardType.MatchesTagExact(Query.RequiredCardType))
		{
			RejectCard(TEXT("SGS.TargetQuery.CardTypeMismatch"), FString::Printf(TEXT("Required=%s Actual=%s"),
				*Query.RequiredCardType.ToString(), *State->CardType.ToString()));
			continue;
		}

		if (Query.RequiredSuit.IsValid() && !State->Suit.MatchesTagExact(Query.RequiredSuit))
		{
			RejectCard(TEXT("SGS.TargetQuery.SuitMismatch"), FString::Printf(TEXT("Required=%s Actual=%s"),
				*Query.RequiredSuit.ToString(), *State->Suit.ToString()));
			continue;
		}

		if (Query.RequiredNumber != INDEX_NONE && State->Number != Query.RequiredNumber)
		{
			RejectCard(TEXT("SGS.TargetQuery.NumberMismatch"), FString::Printf(TEXT("Required=%d Actual=%d"),
				Query.RequiredNumber, State->Number));
			continue;
		}

		FSGSCardTarget Target;
		Target.Handle = Handle;
		Target.CardId = State->CardId;
		Target.Card = State->Card;
		Target.Zone = State->Zone;
		Target.SeatIndex = State->OwnerSeat;
		Result.Targets.Add(Target);
	}

	return Result;
}

bool USGSGameContext::CanViewCardZone(FSGSCardZone Zone, int32 ZoneSeat, int32 ViewerSeat) const
{
	if (ViewerSeat == INDEX_NONE)
	{
		return true;
	}

	if (SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_Hand))
	{
		return ZoneSeat == ViewerSeat;
	}

	if (SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_DrawPile)
		|| SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_Processing))
	{
		return false;
	}

	if (SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_DiscardPile)
		|| SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_Judgement)
		|| SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_Equipment))
	{
		return true;
	}

	return false;
}

bool USGSGameContext::CheckInvariants() const
{
	bool bOk = true;
	bOk &= CardStore.CheckInvariants();

	for (int32 SeatIndex = 0; SeatIndex < Seats.Num(); ++SeatIndex)
	{
		const USGSSeat* Seat = Seats[SeatIndex];
		bOk &= ensureMsgf(Seat != nullptr, TEXT("GameContext has a null seat."));
		if (Seat != nullptr)
		{
			bOk &= ensureMsgf(Seat->SeatIndex == SeatIndex, TEXT("Seat index mismatch."));
			if (Seat->bIsAlive)
			{
				bOk &= ensureMsgf(AliveSeatIndices.Contains(SeatIndex), TEXT("Alive seat index is missing from seat index."));
			}
		}
	}

	bOk &= ensureMsgf(AllSeatIndices.Num() == Seats.Num(), TEXT("All seat index count mismatch."));
	for (int32 SeatIndex : AllSeatIndices)
	{
		bOk &= ensureMsgf(Seats.IsValidIndex(SeatIndex), TEXT("All seat index contains invalid seat."));
	}

	int32 StoreCardCount = 0;
	CardStore.ForEach([this, &bOk, &StoreCardCount](FSGSStableHandle Handle, const FSGSCardState& State)
	{
		++StoreCardCount;
		bOk &= ensureMsgf(State.Card != nullptr, TEXT("CardStore state has null card."));
		bOk &= ensureMsgf(State.CardId != INDEX_NONE, TEXT("CardStore state has invalid card id."));
		bOk &= ensureMsgf(State.Zone.IsValid(), TEXT("CardStore state has invalid zone."));
		bOk &= ensureMsgf(State.ZoneOrder >= 0, TEXT("CardStore state has invalid zone order."));

		if (State.Card != nullptr)
		{
			bOk &= ensureMsgf(State.Card->CardId == State.CardId, TEXT("CardStore card id mismatch."));
			bOk &= ensureMsgf(State.Card->CardName == State.CardName, TEXT("CardStore card name mismatch."));
			bOk &= ensureMsgf(State.Card->Suit.MatchesTagExact(State.Suit), TEXT("CardStore suit mismatch."));
			bOk &= ensureMsgf(State.Card->Number == State.Number, TEXT("CardStore number mismatch."));
			bOk &= ensureMsgf(State.Card->CardType.MatchesTagExact(State.CardType), TEXT("CardStore card type mismatch."));
		}

		if (CardsById.IsValid())
		{
			bOk &= ensureMsgf(CardsById->Find(State.CardId) == Handle, TEXT("CardId index cannot find stored card."));
		}
	});

	bOk &= ensureMsgf(StoreCardCount == AllCards.Num(), TEXT("CardStore/AllCards count mismatch."));
	for (int32 CardIndex = 0; CardIndex < AllCards.Num(); ++CardIndex)
	{
		const USGSCard* Card = AllCards[CardIndex];
		bOk &= ensureMsgf(Card != nullptr, TEXT("GameContext has a null card."));
		if (Card != nullptr)
		{
			bOk &= ensureMsgf(Card->CardId == CardIndex, TEXT("Card id/index mismatch."));
			bOk &= ensureMsgf(MakeCardHandle(Card).IsValid(), TEXT("AllCards contains card missing from CardStore."));
		}
	}

	bOk &= RandomAudit.CheckInvariants();
	return bOk;
}
