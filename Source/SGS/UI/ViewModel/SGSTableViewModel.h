#pragma once

#include "CoreMinimal.h"
#include "Core/SGSTypes.h"

class USGSGameDriver;
class USGSLocalHumanDecisionAgent;

struct SGS_API FSGSCardViewData
{
	int32 CardId = INDEX_NONE;
	FName CardName = NAME_None;
	FString DisplayText;
	FSGSSuit Suit = SGSGameplayTags::Suit_None.GetTag();
	int32 Number = 0;
	bool bSelectable = false;
};

struct SGS_API FSGSSeatViewData
{
	int32 SeatIndex = INDEX_NONE;
	FString DisplayName;
	int32 Health = 0;
	int32 MaxHealth = 0;
	int32 HandCount = 0;
	bool bIsAlive = false;
	bool bIsCurrent = false;
	bool bIsSelectableTarget = false;
};

struct SGS_API FSGSDecisionPromptViewData
{
	bool bHasPrompt = false;
	bool bIsResponse = false;
	FName WindowName = NAME_None;
	FName RequiredCardName = NAME_None;
	bool bAllowPass = true;
	TArray<int32> SelectableCardIds;
	TArray<int32> SelectableTargetSeatIndices;
	TMap<int32, TArray<int32>> TargetSeatIndicesByCardId;
};

struct SGS_API FSGSTableViewSnapshot
{
	int32 ViewerSeat = INDEX_NONE;
	FSGSPhase CurrentPhase = SGSGameplayTags::Phase_None.GetTag();
	int32 CurrentSeatIndex = INDEX_NONE;
	int32 DrawPileCount = 0;
	int32 DiscardPileCount = 0;
	bool bGameOver = false;
	TArray<FSGSSeatViewData> Seats;
	TArray<FSGSCardViewData> HandCards;
	FSGSDecisionPromptViewData Prompt;
	FString LastCommand;
};

class SGS_API FSGSTableViewModel
{
public:
	static FSGSTableViewSnapshot Build(
		const USGSGameDriver* Driver,
		const USGSLocalHumanDecisionAgent* DecisionAgent,
		int32 ViewerSeat);
};
