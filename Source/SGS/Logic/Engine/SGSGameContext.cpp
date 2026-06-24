#include "Logic/Engine/SGSGameContext.h"

#include "Logic/Cards/SGSCard.h"
#include "Logic/Cards/SGSCardPile.h"
#include "Logic/Players/SGSSeat.h"
#include "Core/SGSLogChannels.h"

void USGSGameContext::Initialize(const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents, int32 RandomSeed)
{
	Rng.Initialize(RandomSeed);

	DrawPile = NewObject<USGSCardPile>(this);
	DiscardPile = NewObject<USGSCardPile>(this);

	Seats.Reset();
	for (int32 Index = 0; Index < InAgents.Num(); ++Index)
	{
		USGSSeat* Seat = NewObject<USGSSeat>(this);
		Seat->SeatIndex = Index;
		Seat->DisplayName = FString::Printf(TEXT("Seat%d"), Index);
		Seat->DecisionAgent = InAgents[Index];
		Seat->MaxHealth = DefaultMaxHealth;
		Seat->Health = DefaultMaxHealth;
		Seat->bIsAlive = true;
		Seat->Hand = NewObject<USGSCardPile>(this);
		Seat->JudgementZone = NewObject<USGSCardPile>(this);
		Seats.Add(Seat);
	}
}

USGSSeat* USGSGameContext::GetSeat(int32 Index) const
{
	return Seats.IsValidIndex(Index) ? Seats[Index] : nullptr;
}

USGSCard* USGSGameContext::CreateCard(FName CardName, FSGSSuit Suit, int32 Number)
{
	USGSCard* Card = NewObject<USGSCard>(this);
	Card->CardId = NextCardId++;
	Card->CardName = CardName;
	Card->Suit = Suit;
	Card->Number = Number;

	AllCards.Add(Card);
	DrawPile->AddToBottom(Card);
	return Card;
}

void USGSGameContext::ShuffleDrawPile()
{
	DrawPile->Shuffle(Rng);
}

USGSCardPile* USGSGameContext::GetPileForZone(FSGSCardZone Zone, int32 SeatIndex) const
{
	if (SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_DrawPile))
	{
		return DrawPile;
	}
	if (SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_DiscardPile))
	{
		return DiscardPile;
	}
	if (SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_Hand))
	{
		if (const USGSSeat* Seat = GetSeat(SeatIndex))
		{
			return Seat->Hand;
		}
		return nullptr;
	}
	if (SGSMatchesExactTag(Zone, SGSGameplayTags::CardZone_Judgement))
	{
		if (const USGSSeat* Seat = GetSeat(SeatIndex))
		{
			return Seat->JudgementZone;
		}
		return nullptr;
	}

	// Equipment（见 Plan 0008）、Processing、None 和扩展区默认不由通用移动原语处理。
	return nullptr;
}

void USGSGameContext::MoveCards(const TArray<USGSCard*>& Cards, FSGSCardZone FromZone, int32 FromSeat, FSGSCardZone ToZone, int32 ToSeat)
{
	if (Cards.Num() == 0)
	{
		return;
	}

	USGSCardPile* From = GetPileForZone(FromZone, FromSeat);
	USGSCardPile* To = GetPileForZone(ToZone, ToSeat);

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
		if (From != nullptr)
		{
			From->RemoveCard(Card);
		}
		if (To != nullptr)
		{
			To->AddToBottom(Card);
		}
		Info.Cards.Add(Card);
	}

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

	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (DrawPile->IsEmpty())
		{
			ReshuffleDiscardIntoDraw();
			if (DrawPile->IsEmpty())
			{
				break; // 牌堆与弃牌堆皆空，无牌可摸。
			}
		}

		TArray<USGSCard*> One = DrawPile->TakeFromTop(1);
		if (One.Num() == 0)
		{
			break;
		}
		Drawn.Add(One[0]);
	}

	if (Drawn.Num() > 0)
	{
		FSGSCardMoveInfo Info;
		Info.FromZone = SGSGameplayTags::CardZone_DrawPile.GetTag();
		Info.FromSeat = INDEX_NONE;
		Info.ToZone = SGSGameplayTags::CardZone_Hand.GetTag();
		Info.ToSeat = SeatIndex;
		Info.Cards.Reserve(Drawn.Num());

		for (USGSCard* Card : Drawn)
		{
			Seat->Hand->AddToBottom(Card);
			Info.Cards.Add(Card);
		}

		CardsMovedDelegate.Broadcast(Info);
		UE_LOG(LogSGSCard, Verbose, TEXT("Seat %d drew %d card(s)."), SeatIndex, Drawn.Num());
	}

	return Drawn;
}

void USGSGameContext::DiscardFromHand(int32 SeatIndex, const TArray<USGSCard*>& Cards)
{
	MoveCards(Cards, SGSGameplayTags::CardZone_Hand.GetTag(), SeatIndex, SGSGameplayTags::CardZone_DiscardPile.GetTag(), INDEX_NONE);
}

void USGSGameContext::ReshuffleDiscardIntoDraw()
{
	if (DiscardPile->IsEmpty())
	{
		return;
	}

	TArray<USGSCard*> Recycled = DiscardPile->TakeFromTop(DiscardPile->Num());
	DrawPile->AddManyToBottom(Recycled);
	DrawPile->Shuffle(Rng);

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
