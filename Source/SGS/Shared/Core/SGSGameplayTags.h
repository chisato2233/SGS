#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// SGS 内置规则标签。这里列出的是标准模式的默认事实，不是封闭枚举。
// 玩法扩展、武将技能和模式规则可以继续增加 GameplayTag / DataTable RowName / 注册表项。
namespace SGSGameplayTags
{
	// Suits
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Suit_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Suit_Spade);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Suit_Heart);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Suit_Club);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Suit_Diamond);

	// Card colors derived from suits or explicit virtual suits.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardColor_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardColor_Red);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardColor_Black);

	// Card definition categories.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardType_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardType_Basic);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardType_Trick);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardType_Equipment);

	// Standard turn phases.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_RoundStart);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Judge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Draw);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Play);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Discard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_RoundEnd);

	// Standard factions.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Faction_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Faction_Wei);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Faction_Shu);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Faction_Wu);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Faction_Qun);

	// Standard identity mode roles.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Identity_Lord);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Identity_Loyalist);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Identity_Rebel);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Identity_Renegade);

	// Minimal basic-card runtime states.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_SlashUsed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_AnalepticUsed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_AnalepticBoost);

	// Equipment slots.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipSlot_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipSlot_Weapon);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipSlot_Armor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipSlot_DefenseHorse);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipSlot_OffenseHorse);

	// Card zones.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardZone_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardZone_DrawPile);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardZone_DiscardPile);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardZone_Hand);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardZone_Equipment);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardZone_Judgement);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardZone_Processing);

	// Game lifecycle events.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameEvent_GameStarted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameEvent_GameEnded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameEvent_TurnBegan);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameEvent_TurnEnded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameEvent_PhaseBegan);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameEvent_PhaseEnded);

	// Command / decision actions.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlayAction_Pass);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlayAction_UseCard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlayAction_RespondCard);
}
