#include "Server/Content/SGSStandardGenerals.h"

#include "Shared/Core/SGSGameplayTags.h"

namespace
{
const TArray<FSGSGeneralDefinition>& Definitions()
{
	static const TArray<FSGSGeneralDefinition> Values = {
		{ TEXT("caocao"), TEXT("曹操"), SGSGameplayTags::Faction_Wei.GetTag(), 4, { TEXT("Jianxiong"), TEXT("Hujia") } },
		{ TEXT("simayi"), TEXT("司马懿"), SGSGameplayTags::Faction_Wei.GetTag(), 3, { TEXT("Fankui"), TEXT("Guicai") } },
		{ TEXT("xiahoudun"), TEXT("夏侯惇"), SGSGameplayTags::Faction_Wei.GetTag(), 4, { TEXT("Ganglie") } },
		{ TEXT("liubei"), TEXT("刘备"), SGSGameplayTags::Faction_Shu.GetTag(), 4, { TEXT("Rende"), TEXT("Jijiang") } },
		{ TEXT("guanyu"), TEXT("关羽"), SGSGameplayTags::Faction_Shu.GetTag(), 4, { TEXT("Wusheng") } },
		{ TEXT("zhangfei"), TEXT("张飞"), SGSGameplayTags::Faction_Shu.GetTag(), 4, { TEXT("Paoxiao") } },
		{ TEXT("sunquan"), TEXT("孙权"), SGSGameplayTags::Faction_Wu.GetTag(), 4, { TEXT("Zhiheng"), TEXT("Jiuyuan") } },
		{ TEXT("lvbu"), TEXT("吕布"), SGSGameplayTags::Faction_Qun.GetTag(), 4, { TEXT("Wushuang") } },
	};
	return Values;
}
}

const FSGSGeneralDefinition* SGSStandardGenerals::Find(FName GeneralId)
{
	return Definitions().FindByPredicate(
		[GeneralId](const FSGSGeneralDefinition& Definition)
		{
			return Definition.GeneralId == GeneralId;
		});
}

TArray<FName> SGSStandardGenerals::FirstIdentityRoster()
{
	TArray<FName> GeneralIds;
	GeneralIds.Reserve(Definitions().Num());
	for (const FSGSGeneralDefinition& Definition : Definitions())
	{
		GeneralIds.Add(Definition.GeneralId);
	}
	return GeneralIds;
}
