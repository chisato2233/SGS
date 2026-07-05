#pragma once

// RuleDescriptor 是 Registry 的索引事实源：Rule 自报自己处理的 kind / intent / subject / window。
// 它不描述执行逻辑，只提供候选查询、wildcard 匹配和稳定排序所需的元数据。

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct SGS_API FSGSRuleDescriptor
{
	FName RuleName = NAME_None;
	FName RuleKindTag = NAME_None;
	FGameplayTag IntentTag;
	FName SubjectName = NAME_None;
	FName WindowName = NAME_None;
	int32 Priority = 0;
	bool bWildcardIntent = false;
	bool bWildcardSubject = false;
	bool bWildcardWindow = false;

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= ensureMsgf(!RuleName.IsNone(), TEXT("RuleDescriptor requires a rule name."));
		bOk &= ensureMsgf(!RuleKindTag.IsNone(), TEXT("RuleDescriptor requires a rule kind."));
		bOk &= ensureMsgf(bWildcardIntent || IntentTag.IsValid(), TEXT("RuleDescriptor requires an intent tag unless it is wildcard."));
		bOk &= ensureMsgf(Priority > TNumericLimits<int32>::Min(), TEXT("RuleDescriptor priority is invalid."));
		return bOk;
	}

	FString ToLogString() const
	{
		return FString::Printf(
			TEXT("Rule=%s Kind=%s Intent=%s Subject=%s Window=%s Priority=%d WildIntent=%s WildSubject=%s WildWindow=%s"),
			*RuleName.ToString(),
			*RuleKindTag.ToString(),
			*IntentTag.ToString(),
			*SubjectName.ToString(),
			*WindowName.ToString(),
			Priority,
			bWildcardIntent ? TEXT("true") : TEXT("false"),
			bWildcardSubject ? TEXT("true") : TEXT("false"),
			bWildcardWindow ? TEXT("true") : TEXT("false"));
	}
};

struct SGS_API FSGSRuleLookupKey
{
	FName RuleKindTag = NAME_None;
	FGameplayTag IntentTag;
	FName SubjectName = NAME_None;
	FName WindowName = NAME_None;

	friend bool operator==(const FSGSRuleLookupKey& Left, const FSGSRuleLookupKey& Right)
	{
		return Left.RuleKindTag == Right.RuleKindTag
			&& Left.IntentTag == Right.IntentTag
			&& Left.SubjectName == Right.SubjectName
			&& Left.WindowName == Right.WindowName;
	}

	FString ToLogString() const
	{
		return FString::Printf(
			TEXT("Kind=%s Intent=%s Subject=%s Window=%s"),
			*RuleKindTag.ToString(),
			*IntentTag.ToString(),
			*SubjectName.ToString(),
			*WindowName.ToString());
	}
};

inline uint32 GetTypeHash(const FSGSRuleLookupKey& Key)
{
	uint32 Hash = GetTypeHash(Key.RuleKindTag);
	Hash = HashCombine(Hash, GetTypeHash(Key.IntentTag));
	Hash = HashCombine(Hash, GetTypeHash(Key.SubjectName));
	Hash = HashCombine(Hash, GetTypeHash(Key.WindowName));
	return Hash;
}
