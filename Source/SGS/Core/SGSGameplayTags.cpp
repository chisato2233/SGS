#include "Core/SGSGameplayTags.h"

namespace SGSGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Suit_None, "SGS.Suit.None", "No suit or virtual suit not resolved yet.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Suit_Spade, "SGS.Suit.Spade", "Standard spade suit.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Suit_Heart, "SGS.Suit.Heart", "Standard heart suit.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Suit_Club, "SGS.Suit.Club", "Standard club suit.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Suit_Diamond, "SGS.Suit.Diamond", "Standard diamond suit.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardColor_None, "SGS.CardColor.None", "No card color.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardColor_Red, "SGS.CardColor.Red", "Red card color.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardColor_Black, "SGS.CardColor.Black", "Black card color.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardType_None, "SGS.CardType.None", "No card type.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardType_Basic, "SGS.CardType.Basic", "Basic card type.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardType_Trick, "SGS.CardType.Trick", "Trick card type.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardType_Equipment, "SGS.CardType.Equipment", "Equipment card type.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_None, "SGS.Phase.None", "No active phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_RoundStart, "SGS.Phase.RoundStart", "Standard round start phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Judge, "SGS.Phase.Judge", "Standard judge phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Draw, "SGS.Phase.Draw", "Standard draw phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Play, "SGS.Phase.Play", "Standard play phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Discard, "SGS.Phase.Discard", "Standard discard phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_RoundEnd, "SGS.Phase.RoundEnd", "Standard round end phase.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_None, "SGS.Faction.None", "No faction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_Wei, "SGS.Faction.Wei", "Wei faction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_Shu, "SGS.Faction.Shu", "Shu faction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_Wu, "SGS.Faction.Wu", "Wu faction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_Qun, "SGS.Faction.Qun", "Qun faction.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipSlot_None, "SGS.EquipSlot.None", "No equipment slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipSlot_Weapon, "SGS.EquipSlot.Weapon", "Weapon equipment slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipSlot_Armor, "SGS.EquipSlot.Armor", "Armor equipment slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipSlot_DefenseHorse, "SGS.EquipSlot.DefenseHorse", "Defense horse equipment slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(EquipSlot_OffenseHorse, "SGS.EquipSlot.OffenseHorse", "Offense horse equipment slot.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardZone_None, "SGS.CardZone.None", "No card zone.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardZone_DrawPile, "SGS.CardZone.DrawPile", "Draw pile card zone.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardZone_DiscardPile, "SGS.CardZone.DiscardPile", "Discard pile card zone.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardZone_Hand, "SGS.CardZone.Hand", "Hand card zone.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardZone_Equipment, "SGS.CardZone.Equipment", "Equipment card zone.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardZone_Judgement, "SGS.CardZone.Judgement", "Judgement card zone.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CardZone_Processing, "SGS.CardZone.Processing", "Processing card zone.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameEvent_GameStarted, "SGS.GameEvent.GameStarted", "Game started event.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameEvent_GameEnded, "SGS.GameEvent.GameEnded", "Game ended event.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameEvent_TurnBegan, "SGS.GameEvent.TurnBegan", "Turn began event.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameEvent_TurnEnded, "SGS.GameEvent.TurnEnded", "Turn ended event.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameEvent_PhaseBegan, "SGS.GameEvent.PhaseBegan", "Phase began event.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameEvent_PhaseEnded, "SGS.GameEvent.PhaseEnded", "Phase ended event.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(PlayAction_Pass, "SGS.PlayAction.Pass", "Pass the current play action request.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(PlayAction_UseCard, "SGS.PlayAction.UseCard", "Use a card from hand during an active decision.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(PlayAction_RespondCard, "SGS.PlayAction.RespondCard", "Respond to a reaction or rescue window with a card.");
}
