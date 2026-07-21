#pragma once

#include "CoreMinimal.h"
#include "Shared/Cards/SGSCardTypes.h"

namespace SGSCardTargetModes
{
	inline FName None() { return FName(TEXT("SGS.CardTarget.None")); }
	inline FName Self() { return FName(TEXT("SGS.CardTarget.Self")); }
	inline FName Other() { return FName(TEXT("SGS.CardTarget.Other")); }
	inline FName AllOthers() { return FName(TEXT("SGS.CardTarget.AllOthers")); }
	inline FName AllAlive() { return FName(TEXT("SGS.CardTarget.AllAlive")); }
}

struct SGS_API FSGSStandardCardDefinition
{
	FName CardName = NAME_None;
	FSGSCardType CardType = SGSGameplayTags::CardType_None.GetTag();
	FName TargetMode = SGSCardTargetModes::None();
	int32 MaxDistance = INDEX_NONE;
	FSGSEquipSlot EquipSlot = SGSGameplayTags::EquipSlot_None.GetTag();
	int32 AttackRange = 1;
	bool bPlayable = true;
	bool bDelayedTrick = false;
};

namespace SGSStandardCardDefinitions
{
	SGS_API const FSGSStandardCardDefinition* Find(FName CardName);
}
