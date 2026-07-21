#include "Shared/Cards/SGSStandardCardDefinitions.h"

namespace
{
const TArray<FSGSStandardCardDefinition>& Definitions()
{
	static const TArray<FSGSStandardCardDefinition> Values = {
		{ TEXT("Slash"), SGSGameplayTags::CardType_Basic.GetTag(), SGSCardTargetModes::Other(), 1 },
		{ TEXT("Dodge"), SGSGameplayTags::CardType_Basic.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, {}, 1, false },
		{ TEXT("Peach"), SGSGameplayTags::CardType_Basic.GetTag(), SGSCardTargetModes::Self() },
		{ TEXT("Analeptic"), SGSGameplayTags::CardType_Basic.GetTag(), SGSCardTargetModes::None() },
		{ TEXT("ExNihilo"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::Self() },
		{ TEXT("Dismantlement"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::Other() },
		{ TEXT("Snatch"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::Other(), 1 },
		{ TEXT("Duel"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::Other() },
		{ TEXT("SavageAssault"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::AllOthers() },
		{ TEXT("ArcheryAttack"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::AllOthers() },
		{ TEXT("GodSalvation"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::AllAlive() },
		{ TEXT("AmazingGrace"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::AllAlive() },
		{ TEXT("Nullification"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, {}, 1, false },
		{ TEXT("Indulgence"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::Other(), INDEX_NONE, {}, 1, true, true },
		{ TEXT("SupplyShortage"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::Other(), 1, {}, 1, true, true },
		{ TEXT("Lightning"), SGSGameplayTags::CardType_Trick.GetTag(), SGSCardTargetModes::Self(), INDEX_NONE, {}, 1, true, true },
		{ TEXT("Crossbow"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Weapon.GetTag(), 1 },
		{ TEXT("DoubleSword"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Weapon.GetTag(), 2 },
		{ TEXT("QinggangSword"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Weapon.GetTag(), 2 },
		{ TEXT("Blade"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Weapon.GetTag(), 3 },
		{ TEXT("Spear"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Weapon.GetTag(), 3 },
		{ TEXT("Axe"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Weapon.GetTag(), 3 },
		{ TEXT("Halberd"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Weapon.GetTag(), 4 },
		{ TEXT("KylinBow"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Weapon.GetTag(), 5 },
		{ TEXT("BaguaArmor"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Armor.GetTag() },
		{ TEXT("RenwangShield"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_Armor.GetTag() },
		{ TEXT("Jueying"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_DefenseHorse.GetTag() },
		{ TEXT("Dilu"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_DefenseHorse.GetTag() },
		{ TEXT("ZhuahuangFeidian"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_DefenseHorse.GetTag() },
		{ TEXT("Chitu"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_OffenseHorse.GetTag() },
		{ TEXT("Dayuan"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_OffenseHorse.GetTag() },
		{ TEXT("Zixing"), SGSGameplayTags::CardType_Equipment.GetTag(), SGSCardTargetModes::None(), INDEX_NONE, SGSGameplayTags::EquipSlot_OffenseHorse.GetTag() },
	};
	return Values;
}
}

const FSGSStandardCardDefinition* SGSStandardCardDefinitions::Find(FName CardName)
{
	return Definitions().FindByPredicate(
		[CardName](const FSGSStandardCardDefinition& Definition)
		{
			return Definition.CardName == CardName;
		});
}
