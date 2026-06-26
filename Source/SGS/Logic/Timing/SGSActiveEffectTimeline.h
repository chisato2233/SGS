#pragma once

// SGS 持续效果的运行期生命周期注册表。
// 效果写入来源、拥有者、目标和时序后，可按句柄或标签查询，并由规则显式失效。
// 规则真相源属于这条 Timeline；GAS 绑定只是适配层，不反客为主。

#include "CoreMinimal.h"
#include "Core/SGSIndexedStore.h"
#include "Logic/Timing/SGSTimingTypes.h"

namespace SGSActiveEffectStackPolicies
{
	inline FName Independent() { return FName(TEXT("SGS.ActiveEffect.Stack.Independent")); }
	inline FName RefreshDuration() { return FName(TEXT("SGS.ActiveEffect.Stack.RefreshDuration")); }
	inline FName AddStack() { return FName(TEXT("SGS.ActiveEffect.Stack.AddStack")); }
	inline FName Replace() { return FName(TEXT("SGS.ActiveEffect.Stack.Replace")); }
}

namespace SGSActiveEffectExpireReasons
{
	inline FName Manual() { return FName(TEXT("SGS.ActiveEffect.Expire.Manual")); }
	inline FName DurationEnded() { return FName(TEXT("SGS.ActiveEffect.Expire.DurationEnded")); }
	inline FName OwnerLeft() { return FName(TEXT("SGS.ActiveEffect.Expire.OwnerLeft")); }
	inline FName SourceRemoved() { return FName(TEXT("SGS.ActiveEffect.Expire.SourceRemoved")); }
}

struct SGS_API FSGSActiveEffect
{
	FName EffectName = NAME_None;
	FGameplayTag PrimaryTag;
	TArray<FGameplayTag> Tags;
	FSGSStableHandle SourceHandle;
	FSGSStableHandle TargetHandle;
	FSGSCommandId SourceCommandId;
	int32 OwnerSeat = INDEX_NONE;
	int32 TargetSeat = INDEX_NONE;
	int32 Priority = 0;
	int32 StackCount = 1;
	FName StackPolicy = SGSActiveEffectStackPolicies::Independent();
	FSGSDurationSpec Duration;
	FSGSTimingPoint CreatedAt;
	FSGSTimingPoint ExpiredAt;
	FName ExpireReason = NAME_None;
	bool bActive = true;

	bool HasTag(FGameplayTag Tag) const
	{
		if (!Tag.IsValid())
		{
			return false;
		}
		if (PrimaryTag.MatchesTagExact(Tag))
		{
			return true;
		}
		for (const FGameplayTag& OwnedTag : Tags)
		{
			if (OwnedTag.MatchesTagExact(Tag))
			{
				return true;
			}
		}
		return false;
	}

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(!EffectName.IsNone(), TEXT("ActiveEffect requires an effect name."));
		bOk &= ensureMsgf(StackCount > 0, TEXT("ActiveEffect stack count must be positive."));
		bOk &= ensureMsgf(!StackPolicy.IsNone(), TEXT("ActiveEffect stack policy cannot be empty."));
		bOk &= SourceHandle.CheckInvariants();
		bOk &= TargetHandle.CheckInvariants();
		bOk &= SourceCommandId.CheckInvariants();
		bOk &= Duration.CheckInvariants();
		bOk &= CreatedAt.CheckInvariants();
		if (!bActive)
		{
			bOk &= ensureMsgf(!ExpireReason.IsNone(), TEXT("Expired ActiveEffect requires an expire reason."));
			bOk &= ExpiredAt.CheckInvariants();
		}
		return bOk;
	}
};

class SGS_API FSGSActiveEffectTimeline
{
public:
	FSGSActiveEffectTimeline();

	void Reset();
	FSGSStableHandle Add(FSGSActiveEffect Effect);
	FSGSStatus Expire(FSGSStableHandle Handle, FSGSTimingPoint At, FName Reason);

	const FSGSActiveEffect* Find(FSGSStableHandle Handle) const { return Effects.Find(Handle); }
	TArray<FSGSStableHandle> QueryActive() const;
	TArray<FSGSStableHandle> QueryByOwner(int32 OwnerSeat, bool bActiveOnly = true) const;
	TArray<FSGSStableHandle> QueryByTarget(int32 TargetSeat, bool bActiveOnly = true) const;
	TArray<FSGSStableHandle> QueryByTag(FGameplayTag Tag, bool bActiveOnly = true) const;

	bool CheckInvariants() const;

private:
	using FEffectStore = TSGSIndexedStore<FSGSActiveEffect>;

	void RegisterIndexes();
	void RebuildTagIndex();
	TArray<FSGSStableHandle> FilterActive(const TArray<FSGSStableHandle>& Handles, bool bActiveOnly) const;

	FEffectStore Effects;
	TSharedPtr<FEffectStore::TNonUniqueIndex<int32>> EffectsByOwner;
	TSharedPtr<FEffectStore::TNonUniqueIndex<int32>> EffectsByTarget;
	TSharedPtr<FEffectStore::TNonUniqueIndex<FGameplayTag>> EffectsByPrimaryTag;
	TMultiMap<FGameplayTag, FSGSStableHandle> EffectsByTag;
};
