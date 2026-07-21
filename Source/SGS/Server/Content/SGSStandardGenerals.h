#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct SGS_API FSGSGeneralDefinition
{
	FName GeneralId = NAME_None;
	FName DisplayName = NAME_None;
	FGameplayTag Faction;
	int32 MaxHealth = 4;
	TArray<FName> SkillNames;
};

namespace SGSStandardGenerals
{
	SGS_API const FSGSGeneralDefinition* Find(FName GeneralId);
	SGS_API TArray<FName> FirstIdentityRoster();
}
