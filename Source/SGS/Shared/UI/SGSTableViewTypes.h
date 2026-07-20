#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/SGSTypes.h"
#include "Shared/Game/SGSGameResult.h"

#include "SGSTableViewTypes.generated.h"

USTRUCT(BlueprintType)
struct SGS_API FSGSCardViewData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CardId = INDEX_NONE;

	UPROPERTY()
	FName CardName = NAME_None;

	UPROPERTY()
	FString DisplayText;

	UPROPERTY()
	FGameplayTag Suit = SGSGameplayTags::Suit_None.GetTag();

	UPROPERTY()
	int32 Number = 0;

	UPROPERTY()
	bool bSelectable = false;

	UPROPERTY()
	bool bFaceDown = false;
};

// 已经过观看者权限裁剪的表现提示。VisibleCards 为空而 CardCount > 0 时必须绘制牌背，
// 不能从缺省值推断服务器牌 ID。
USTRUCT(BlueprintType)
struct SGS_API FSGSTableCardMotionCueViewData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Sequence = INDEX_NONE;

	UPROPERTY()
	FName Reason = NAME_None;

	UPROPERTY()
	FGameplayTag FromZone = SGSGameplayTags::CardZone_None.GetTag();

	UPROPERTY()
	int32 FromSeat = INDEX_NONE;

	UPROPERTY()
	FGameplayTag ToZone = SGSGameplayTags::CardZone_None.GetTag();

	UPROPERTY()
	int32 ToSeat = INDEX_NONE;

	UPROPERTY()
	int32 CardCount = 0;

	UPROPERTY()
	TArray<FSGSCardViewData> VisibleCards;

	UPROPERTY()
	TArray<int32> RelatedTargetSeatIndices;

	UPROPERTY()
	bool bCleanup = false;
};

USTRUCT(BlueprintType)
struct SGS_API FSGSSeatViewData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SeatIndex = INDEX_NONE;

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	FName GeneralId = NAME_None;

	UPROPERTY()
	FGameplayTag Faction;

	UPROPERTY()
	int32 Health = 0;

	UPROPERTY()
	int32 MaxHealth = 0;

	UPROPERTY()
	int32 HandCount = 0;

	UPROPERTY()
	TArray<FSGSCardViewData> EquipmentCards;

	UPROPERTY()
	TArray<FSGSCardViewData> JudgementCards;

	UPROPERTY()
	bool bIsAlive = false;

	UPROPERTY()
	bool bIsCurrent = false;

	UPROPERTY()
	bool bIsSelectableTarget = false;

	UPROPERTY()
	FGameplayTag Identity;
};

USTRUCT(BlueprintType)
struct SGS_API FSGSCardTargetViewData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CardId = INDEX_NONE;

	UPROPERTY()
	TArray<int32> TargetSeatIndices;

	UPROPERTY()
	int32 MinTargetCount = 0;

	UPROPERTY()
	int32 MaxTargetCount = 0;
};

USTRUCT(BlueprintType)
struct SGS_API FSGSDecisionSkillViewData
{
	GENERATED_BODY()

	UPROPERTY()
	FName SkillName = NAME_None;

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	bool bRequiresCard = false;

	UPROPERTY()
	FName RuleKindTag = NAME_None;

	UPROPERTY()
	FName ResultCardName = NAME_None;

	UPROPERTY()
	int32 MinCardCount = 0;

	UPROPERTY()
	int32 MaxCardCount = 0;

	UPROPERTY()
	int32 MinTargetCount = 0;

	UPROPERTY()
	int32 MaxTargetCount = 0;

	UPROPERTY()
	TArray<int32> SelectableCardIds;

	UPROPERTY()
	TArray<int32> SelectableTargetSeatIndices;
};

USTRUCT(BlueprintType)
struct SGS_API FSGSDecisionPromptViewData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHasPrompt = false;

	UPROPERTY()
	bool bIsResponse = false;

	UPROPERTY()
	FName WindowName = NAME_None;

	UPROPERTY()
	FName RequiredCardName = NAME_None;

	UPROPERTY()
	TArray<FName> AcceptedCardNames;

	UPROPERTY()
	FName ContextName = NAME_None;

	UPROPERTY()
	int32 EffectSourceSeat = INDEX_NONE;

	UPROPERTY()
	int32 EffectTargetSeat = INDEX_NONE;

	UPROPERTY()
	bool bAllowPass = true;

	UPROPERTY()
	bool bIsCardChoice = false;

	UPROPERTY()
	bool bIsOptionChoice = false;

	UPROPERTY()
	FName ChoiceName = NAME_None;

	UPROPERTY()
	int32 MinChoiceCount = 0;

	UPROPERTY()
	int32 MaxChoiceCount = 0;

	UPROPERTY()
	TArray<FSGSCardViewData> ChoiceCards;

	UPROPERTY()
	TArray<int32> SelectableCardIds;

	UPROPERTY()
	TArray<int32> SelectableTargetSeatIndices;

	UPROPERTY()
	TArray<FSGSCardTargetViewData> TargetSeatOptions;

	UPROPERTY()
	TArray<FSGSDecisionSkillViewData> SkillOptions;

	const FSGSDecisionSkillViewData* FindSkillOption(FName SkillName) const
	{
		return SkillOptions.FindByPredicate(
			[SkillName](const FSGSDecisionSkillViewData& Candidate)
			{
				return Candidate.SkillName == SkillName;
			});
	}

	void SetTargetSeatIndicesForCard(
		int32 CardId,
		const TArray<int32>& TargetSeatIndices,
		int32 MinTargetCount = 0,
		int32 MaxTargetCount = 0)
	{
		FSGSCardTargetViewData* Existing = TargetSeatOptions.FindByPredicate(
			[CardId](const FSGSCardTargetViewData& Candidate)
			{
				return Candidate.CardId == CardId;
			});
		if (Existing != nullptr)
		{
			Existing->TargetSeatIndices = TargetSeatIndices;
			Existing->MinTargetCount = MinTargetCount;
			Existing->MaxTargetCount = MaxTargetCount;
			return;
		}

		FSGSCardTargetViewData Option;
		Option.CardId = CardId;
		Option.TargetSeatIndices = TargetSeatIndices;
		Option.MinTargetCount = MinTargetCount;
		Option.MaxTargetCount = MaxTargetCount;
		TargetSeatOptions.Add(MoveTemp(Option));
	}

	const TArray<int32>* FindTargetSeatIndicesForCard(int32 CardId) const
	{
		const FSGSCardTargetViewData* Existing = TargetSeatOptions.FindByPredicate(
			[CardId](const FSGSCardTargetViewData& Candidate)
			{
				return Candidate.CardId == CardId;
			});
		return Existing != nullptr ? &Existing->TargetSeatIndices : nullptr;
	}
};

USTRUCT(BlueprintType)
struct SGS_API FSGSTablePublicSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Revision = 0;

	UPROPERTY()
	FGameplayTag CurrentPhase = SGSGameplayTags::Phase_None.GetTag();

	UPROPERTY()
	int32 CurrentSeatIndex = INDEX_NONE;

	UPROPERTY()
	int32 DrawPileCount = 0;

	UPROPERTY()
	int32 DiscardPileCount = 0;

	UPROPERTY()
	bool bHasDiscardTopCard = false;

	UPROPERTY()
	FSGSCardViewData DiscardTopCard;

	UPROPERTY()
	int32 MotionEpoch = 0;

	UPROPERTY()
	int32 MotionWindowStartSequence = 0;

	UPROPERTY()
	int32 MotionLatestSequence = INDEX_NONE;

	UPROPERTY()
	TArray<FSGSTableCardMotionCueViewData> CardMotionCues;

	UPROPERTY()
	bool bGameOver = false;

	UPROPERTY()
	FSGSGameResult GameResult;

	UPROPERTY()
	TArray<FSGSSeatViewData> Seats;

};

USTRUCT(BlueprintType)
struct SGS_API FSGSPlayerPrivateSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Revision = 0;

	UPROPERTY()
	int32 ViewerSeat = INDEX_NONE;

	UPROPERTY()
	FGameplayTag ViewerIdentity;

	UPROPERTY()
	TArray<FSGSCardViewData> HandCards;

	UPROPERTY()
	FSGSDecisionPromptViewData Prompt;

	UPROPERTY()
	int32 MotionEpoch = 0;

	// 仅包含该观看者有权看到的同序号覆盖，不单独构成事件流。
	UPROPERTY()
	TArray<FSGSTableCardMotionCueViewData> CardMotionCueOverrides;
};

USTRUCT(BlueprintType)
struct SGS_API FSGSTableViewSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	int32 PublicRevision = 0;

	UPROPERTY()
	int32 PrivateRevision = 0;

	UPROPERTY()
	int32 ViewerSeat = INDEX_NONE;

	UPROPERTY()
	FGameplayTag CurrentPhase = SGSGameplayTags::Phase_None.GetTag();

	UPROPERTY()
	int32 CurrentSeatIndex = INDEX_NONE;

	UPROPERTY()
	int32 DrawPileCount = 0;

	UPROPERTY()
	int32 DiscardPileCount = 0;

	UPROPERTY()
	bool bHasDiscardTopCard = false;

	UPROPERTY()
	FSGSCardViewData DiscardTopCard;

	UPROPERTY()
	int32 MotionEpoch = 0;

	UPROPERTY()
	int32 MotionWindowStartSequence = 0;

	UPROPERTY()
	int32 MotionLatestSequence = INDEX_NONE;

	UPROPERTY()
	TArray<FSGSTableCardMotionCueViewData> CardMotionCues;

	UPROPERTY()
	bool bGameOver = false;

	UPROPERTY()
	FSGSGameResult GameResult;

	UPROPERTY()
	TArray<FSGSSeatViewData> Seats;

	UPROPERTY()
	TArray<FSGSCardViewData> HandCards;

	UPROPERTY()
	FSGSDecisionPromptViewData Prompt;

};

inline FSGSTableViewSnapshot SGSComposeTableViewSnapshot(
	const FSGSTablePublicSnapshot& PublicSnapshot,
	const FSGSPlayerPrivateSnapshot& PrivateSnapshot)
{
	FSGSTableViewSnapshot Snapshot;
	Snapshot.PublicRevision = PublicSnapshot.Revision;
	Snapshot.PrivateRevision = PrivateSnapshot.Revision;
	Snapshot.ViewerSeat = PrivateSnapshot.ViewerSeat;
	Snapshot.CurrentPhase = PublicSnapshot.CurrentPhase;
	Snapshot.CurrentSeatIndex = PublicSnapshot.CurrentSeatIndex;
	Snapshot.DrawPileCount = PublicSnapshot.DrawPileCount;
	Snapshot.DiscardPileCount = PublicSnapshot.DiscardPileCount;
	Snapshot.bHasDiscardTopCard = PublicSnapshot.bHasDiscardTopCard;
	Snapshot.DiscardTopCard = PublicSnapshot.DiscardTopCard;
	Snapshot.MotionEpoch = PublicSnapshot.MotionEpoch;
	Snapshot.MotionWindowStartSequence = PublicSnapshot.MotionWindowStartSequence;
	Snapshot.MotionLatestSequence = PublicSnapshot.MotionLatestSequence;
	Snapshot.CardMotionCues = PublicSnapshot.CardMotionCues;
	Snapshot.bGameOver = PublicSnapshot.bGameOver;
	Snapshot.GameResult = PublicSnapshot.GameResult;
	Snapshot.Seats = PublicSnapshot.Seats;
	Snapshot.HandCards = PrivateSnapshot.HandCards;
	Snapshot.Prompt = PrivateSnapshot.Prompt;
	if (PrivateSnapshot.MotionEpoch == PublicSnapshot.MotionEpoch)
	{
		for (const FSGSTableCardMotionCueViewData& Override : PrivateSnapshot.CardMotionCueOverrides)
		{
			if (FSGSTableCardMotionCueViewData* Existing = Snapshot.CardMotionCues.FindByPredicate(
				[Sequence = Override.Sequence](const FSGSTableCardMotionCueViewData& Cue)
				{
					return Cue.Sequence == Sequence;
				}))
			{
				*Existing = Override;
			}
		}
	}
	Snapshot.CardMotionCues.Sort([](const FSGSTableCardMotionCueViewData& Left, const FSGSTableCardMotionCueViewData& Right)
	{
		return Left.Sequence < Right.Sequence;
	});
	for (FSGSSeatViewData& Seat : Snapshot.Seats)
	{
		if (Seat.SeatIndex == Snapshot.ViewerSeat && PrivateSnapshot.ViewerIdentity.IsValid())
		{
			Seat.Identity = PrivateSnapshot.ViewerIdentity;
		}
		Seat.bIsSelectableTarget = Snapshot.Prompt.SelectableTargetSeatIndices.Contains(Seat.SeatIndex);
	}

	for (FSGSCardViewData& Card : Snapshot.HandCards)
	{
		Card.bSelectable = Snapshot.Prompt.SelectableCardIds.Contains(Card.CardId);
	}

	return Snapshot;
}
