#pragma once

// Registered server rules are the only execution entry after CommandRouter
// validation. Driver owns the registry; rules mutate state only through Runtime.

#include "CoreMinimal.h"
#include "Server/Rules/Core/SGSRuleTypes.h"
#include "Shared/Core/SGSIndexedStore.h"

struct SGS_API FSGSRuleEntry
{
	FSGSRuleDescriptor Descriptor;
	TSharedRef<ISGSRule> Rule;
	int32 RegistrationOrder = INDEX_NONE;

	FSGSRuleEntry(FSGSRuleDescriptor InDescriptor, TSharedRef<ISGSRule> InRule, int32 InRegistrationOrder)
		: Descriptor(MoveTemp(InDescriptor))
		, Rule(InRule)
		, RegistrationOrder(InRegistrationOrder)
	{
	}

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= Descriptor.CheckInvariants();
		bOk &= ensureMsgf(RegistrationOrder >= 0, TEXT("RuleEntry requires a non-negative registration order."));
		bOk &= Rule->CheckInvariants();
		return bOk;
	}
};

class SGS_API FSGSRuleRegistry
{
public:
	FSGSRuleRegistry();

	void Reset();
	void RegisterRule(TSharedRef<ISGSRule> Rule);
	TArray<FName> FindCandidateRuleNames(const FSGSRuleInvocation& Invocation) const;
	FSGSStatus Resolve(FSGSRuleExecutionContext& Context) const;
	FSGSStatus DispatchAll(FSGSRuleExecutionContext& Context) const;
	bool CheckInvariants() const;

private:
	using FRuleStore = TSGSIndexedStore<FSGSRuleEntry>;

	void InitializeStore();
	TArray<FSGSStableHandle> QueryCandidateHandles(const FSGSRuleInvocation& Invocation) const;
	TArray<const FSGSRuleEntry*> FindCandidateEntries(const FSGSRuleInvocation& Invocation) const;

	FRuleStore Rules;
	TSharedPtr<FRuleStore::TUniqueIndex<FName>> RulesByName;
	TSharedPtr<FRuleStore::TNonUniqueIndex<FName>> RulesByKind;
	TSharedPtr<FRuleStore::TNonUniqueIndex<FGameplayTag>> RulesByIntent;
	TSharedPtr<FRuleStore::TNonUniqueIndex<FName>> RulesBySubject;
	TSharedPtr<FRuleStore::TNonUniqueIndex<FName>> RulesByWindow;
	TSharedPtr<FRuleStore::TNonUniqueIndex<FSGSRuleLookupKey>> RulesByLookupKey;
	int32 NextRegistrationOrder = 0;
};

namespace SGSRules
{
	SGS_API void RegisterDefaultRules(FSGSRuleRegistry& Registry);
}
