#pragma once

#include "CoreMinimal.h"
#include "Core/SGSIndexedStore.h"
#include "UObject/Object.h"
#include "UObject/ScriptInterface.h"
#include "Core/SGSRandomAudit.h"
#include "Core/SGSTypes.h"
#include "Logic/Cards/SGSCardTypes.h"
#include "Logic/Queries/SGSTargetQueryTypes.h"
#include "SGSGameContext.generated.h"

class USGSSeat;
class USGSCard;
class ISGSDecisionAgent;

// ---- 事件载荷（C++ 多播，不暴露蓝图）----

// 一次换区移动。
struct FSGSCardMoveInfo
{
	TArray<TObjectPtr<USGSCard>> Cards;
	FSGSCardZone FromZone = SGSGameplayTags::CardZone_None.GetTag();
	int32 FromSeat = INDEX_NONE;
	FSGSCardZone ToZone = SGSGameplayTags::CardZone_None.GetTag();
	int32 ToSeat = INDEX_NONE;
};

struct FSGSDamageInfo
{
	int32 SourceSeat = INDEX_NONE;
	int32 TargetSeat = INDEX_NONE;
	int32 Amount = 0;
};

struct SGS_API FSGSCardState
{
	TObjectPtr<USGSCard> Card = nullptr;
	int32 CardId = INDEX_NONE;
	FName CardName = NAME_None;
	FSGSCardType CardType = SGSGameplayTags::CardType_None.GetTag();
	FSGSSuit Suit = SGSGameplayTags::Suit_None.GetTag();
	int32 Number = 0;
	FSGSCardZone Zone = SGSGameplayTags::CardZone_None.GetTag();
	int32 OwnerSeat = INDEX_NONE;
	int32 ZoneOrder = INDEX_NONE;

	FSGSCardPileKey GetPileKey() const
	{
		return FSGSCardPileKey(Zone, OwnerSeat);
	}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FSGSOnCardsMoved, const FSGSCardMoveInfo& /*Move*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FSGSOnDamage, const FSGSDamageInfo& /*Damage*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSGSOnHealthChanged, int32 /*SeatIndex*/, int32 /*NewHealth*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FSGSOnSeatDying, int32 /*SeatIndex*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSGSOnSeatEliminated, int32 /*SeatIndex*/, FName /*Reason*/);

// 对局权威数据模型 + 基础操作原语（服务器侧）。
// 持有牌堆/弃牌堆/座位与全部牌；提供移牌/摸牌/弃牌/伤害/回复/距离等原语并广播事件。
// 流程控制在 USGSGameDriver；本类不关心回合推进，只负责「状态 + 改状态的最小操作」。
UCLASS()
class SGS_API USGSGameContext : public UObject
{
	GENERATED_BODY()

public:
	// 建座位（与决策代理一一对应）、空牌堆、随机流。
	void Initialize(const TArray<TScriptInterface<ISGSDecisionAgent>>& InAgents, int32 RandomSeed);

	int32 NumSeats() const { return Seats.Num(); }
	USGSSeat* GetSeat(int32 Index) const;
	USGSCard* FindCardById(int32 CardId) const;
	const FSGSCardState* FindCardStateById(int32 CardId) const;
	TArray<USGSCard*> GetCardsInZone(FSGSCardZone Zone, int32 SeatIndex = INDEX_NONE) const;
	const FSGSRandomAudit& GetRandomAudit() const { return RandomAudit; }

	// 造一张牌并放入牌堆底。CardId 自增分配。完整牌库应由 DataTable 驱动批量建库。
	USGSCard* CreateCard(FName CardName, FSGSSuit Suit, int32 Number, FSGSCardType CardType = FGameplayTag());
	void ShuffleDrawPile();

	// 通用移动原语：所有换区都应走这里（装备区除外，见 Plan 0008）。广播 OnCardsMoved。
	void MoveCards(const TArray<USGSCard*>& Cards, FSGSCardZone FromZone, int32 FromSeat, FSGSCardZone ToZone, int32 ToSeat);

	// 摸牌：牌堆顶 → 座位手牌；牌堆空时把弃牌堆洗回再摸。返回实际摸到的牌。
	TArray<USGSCard*> DrawCards(int32 SeatIndex, int32 Count);

	// 弃牌：座位手牌 → 弃牌堆。
	void DiscardFromHand(int32 SeatIndex, const TArray<USGSCard*>& Cards);

	// 伤害：扣体力 + 广播。濒死/求桃在 Plan 0005；此处仅在体力归零时广播 OnSeatDying。
	void ApplyDamage(int32 SourceSeat, int32 TargetSeat, int32 Amount);

	// 回复体力（不超过上限）。
	void Heal(int32 SeatIndex, int32 Amount);

	// 濒死求桃失败后的最小出局处理；完整胜负判定进入后续计划。
	void EliminateSeat(int32 SeatIndex, FName Reason);

	// 攻击者 FromSeat 到 ToSeat 的距离：存活座位环形最短 + 坐骑修正，最小为 1。
	int32 GetDistance(int32 FromSeat, int32 ToSeat) const;
	FSGSSeatQueryResult QuerySeats(const FSGSSeatQuery& Query) const;
	FSGSCardQueryResult QueryCards(const FSGSCardQuery& Query) const;
	bool CheckInvariants() const;

	FSGSOnCardsMoved& OnCardsMoved() { return CardsMovedDelegate; }
	FSGSOnDamage& OnDamage() { return DamageDelegate; }
	FSGSOnHealthChanged& OnHealthChanged() { return HealthChangedDelegate; }
	FSGSOnSeatDying& OnSeatDying() { return SeatDyingDelegate; }
	FSGSOnSeatEliminated& OnSeatEliminated() { return SeatEliminatedDelegate; }

private:
	using FCardStore = TSGSIndexedStore<FSGSCardState>;

	bool IsValidSeat(int32 Index) const { return Seats.IsValidIndex(Index); }
	void RegisterCardIndexes();
	void RebuildSeatIndexes();
	FSGSStableHandle MakeSeatHandle(int32 SeatIndex) const;
	FSGSStableHandle MakeCardHandle(const USGSCard* Card) const;
	bool CanViewCardZone(FSGSCardZone Zone, int32 ZoneSeat, int32 ViewerSeat) const;
	FSGSCardPileKey MakePileKey(FSGSCardZone Zone, int32 SeatIndex) const;
	int32 CountCardsInPile(FSGSCardPileKey Key) const;
	TArray<FSGSStableHandle> GetCardHandlesInPile(FSGSCardPileKey Key) const;
	TArray<USGSCard*> GetCardsInPile(FSGSCardPileKey Key) const;
	FSGSStatus NormalizePileOrder(FSGSCardPileKey Key);
	FSGSStatus MoveCardHandle(FSGSStableHandle Handle, FSGSCardZone ToZone, int32 ToSeat, int32 ToOrder);
	void ShufflePileInStore(FSGSCardPileKey Key, FName Purpose, FString Context);
	void ReshuffleDiscardIntoDraw();

	UPROPERTY()
	TArray<TObjectPtr<USGSSeat>> Seats;

	// 持有全部牌，避免被 GC，并作为牌的全集。
	UPROPERTY()
	TArray<TObjectPtr<USGSCard>> AllCards;

	int32 NextCardId = 0;
	FCardStore CardStore;
	TSharedPtr<FCardStore::TUniqueIndex<int32>> CardsById;
	TSharedPtr<FCardStore::TNonUniqueIndex<int32>> CardsByOwnerSeat;
	TSharedPtr<FCardStore::TNonUniqueIndex<FSGSCardZone>> CardsByZone;
	TSharedPtr<FCardStore::TOrderedIndex<FSGSCardPileKey>> CardsByPile;
	TSharedPtr<FCardStore::TNonUniqueIndex<FName>> CardsByName;
	TSharedPtr<FCardStore::TNonUniqueIndex<FSGSCardType>> CardsByType;
	TSharedPtr<FCardStore::TNonUniqueIndex<FSGSSuit>> CardsBySuit;
	TSharedPtr<FCardStore::TNonUniqueIndex<int32>> CardsByNumber;
	TSharedPtr<FCardStore::TNonUniqueIndex<FSGSCardPileKey>> CardsByOwnerZone;
	TSharedPtr<FCardStore::TNonUniqueIndex<FSGSCardZoneNameKey>> CardsByZoneName;
	TSharedPtr<FCardStore::TNonUniqueIndex<FSGSCardZoneTypeKey>> CardsByZoneType;

	TArray<int32> AllSeatIndices;
	TArray<int32> AliveSeatIndices;
	TMultiMap<FGameplayTag, int32> SeatIndicesByFaction;

	FSGSRandomAudit RandomAudit;

	FSGSOnCardsMoved CardsMovedDelegate;
	FSGSOnDamage DamageDelegate;
	FSGSOnHealthChanged HealthChangedDelegate;
	FSGSOnSeatDying SeatDyingDelegate;
	FSGSOnSeatEliminated SeatEliminatedDelegate;

	// 骨架期默认体力上限（真实值由武将决定，见 Plan 0009）。
	static constexpr int32 DefaultMaxHealth = 4;
};
