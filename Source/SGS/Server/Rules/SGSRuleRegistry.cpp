#include "Server/Rules/SGSRuleRegistry.h"

#include "Server/Rules/SGSBasicCardRules.h"

namespace
{
FSGSRuleLookupKey MakeRuleLookupKey(const FSGSRuleDescriptor& Descriptor)
{
	FSGSRuleLookupKey Key;
	Key.RuleKindTag = Descriptor.RuleKindTag;
	Key.IntentTag = Descriptor.bWildcardIntent ? FGameplayTag() : Descriptor.IntentTag;
	Key.SubjectName = Descriptor.bWildcardSubject ? NAME_None : Descriptor.SubjectName;
	Key.WindowName = Descriptor.bWildcardWindow ? NAME_None : Descriptor.WindowName;
	return Key;
}

TArray<FSGSRuleLookupKey> MakeInvocationLookupKeys(const FSGSRuleInvocation& Invocation)
{
	TArray<FSGSRuleLookupKey> Keys;
	TSet<FSGSRuleLookupKey> SeenKeys;

	TArray<FGameplayTag> IntentKeys;
	IntentKeys.Add(Invocation.IntentTag);
	IntentKeys.Add(FGameplayTag());

	TArray<FName> SubjectKeys;
	SubjectKeys.Add(Invocation.SubjectName);
	SubjectKeys.Add(NAME_None);

	TArray<FName> WindowKeys;
	WindowKeys.Add(Invocation.WindowName);
	WindowKeys.Add(NAME_None);

	for (const FGameplayTag& IntentKey : IntentKeys)
	{
		for (const FName& SubjectKey : SubjectKeys)
		{
			for (const FName& WindowKey : WindowKeys)
			{
				FSGSRuleLookupKey Key;
				Key.RuleKindTag = Invocation.RuleKindTag;
				Key.IntentTag = IntentKey;
				Key.SubjectName = SubjectKey;
				Key.WindowName = WindowKey;
				if (!SeenKeys.Contains(Key))
				{
					SeenKeys.Add(Key);
					Keys.Add(Key);
				}
			}
		}
	}

	return Keys;
}

bool DescriptorMatchesInvocation(const FSGSRuleDescriptor& Descriptor, const FSGSRuleInvocation& Invocation)
{
	if (Descriptor.RuleKindTag != Invocation.RuleKindTag)
	{
		return false;
	}
	if (!Descriptor.bWildcardIntent && !Descriptor.IntentTag.MatchesTagExact(Invocation.IntentTag))
	{
		return false;
	}
	if (!Descriptor.bWildcardSubject && Descriptor.SubjectName != Invocation.SubjectName)
	{
		return false;
	}
	if (!Descriptor.bWildcardWindow && Descriptor.WindowName != Invocation.WindowName)
	{
		return false;
	}
	return true;
}
}

FSGSRuleRegistry::FSGSRuleRegistry()
{
	InitializeStore();
	SGSRules::RegisterDefaultRules(*this);
}

void FSGSRuleRegistry::Reset()
{
	Rules.Reset();
	InitializeStore();
	NextRegistrationOrder = 0;
	SGSRules::RegisterDefaultRules(*this);
}

void FSGSRuleRegistry::RegisterRule(TSharedRef<ISGSRule> Rule)
{
	if (!Rule->CheckInvariants())
	{
		return;
	}

	FSGSRuleDescriptor Descriptor = Rule->GetDescriptor();
	if (!Descriptor.CheckInvariants())
	{
		return;
	}

	if (RulesByName.IsValid() && RulesByName->Contains(Descriptor.RuleName))
	{
		ensureMsgf(false, TEXT("SGS rule %s is already registered."), *Descriptor.RuleName.ToString());
		return;
	}

	Rules.Add(FSGSRuleEntry(MoveTemp(Descriptor), Rule, NextRegistrationOrder++));
}

TArray<FName> FSGSRuleRegistry::FindCandidateRuleNames(const FSGSRuleInvocation& Invocation) const
{
	TArray<FName> Names;
	for (const FSGSRuleEntry* Entry : FindCandidateEntries(Invocation))
	{
		if (Entry != nullptr)
		{
			Names.Add(Entry->Descriptor.RuleName);
		}
	}
	return Names;
}

FSGSStatus FSGSRuleRegistry::Resolve(FSGSRuleExecutionContext& Context) const
{
	if (!Context.CheckInvariants())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Rule.InvariantViolation")),
			TEXT("Rule execution context failed invariants.")));
	}

	const TArray<const FSGSRuleEntry*> CandidateEntries = FindCandidateEntries(Context.RuleInvocation);
	for (const FSGSRuleEntry* Entry : CandidateEntries)
	{
		if (Entry == nullptr)
		{
			continue;
		}

		const TSharedRef<ISGSRule>& Rule = Entry->Rule;
		if (!Rule->CanHandle(Context))
		{
			continue;
		}

		if (FSGSStatus Status = Rule->Validate(Context); Status.HasError())
		{
			return Status;
		}

		return Rule->Execute(Context);
	}

	return MakeError(FSGSError::Make(
		FName(TEXT("SGS.Rule.NotFound")),
		FString::Printf(TEXT("No SGS rule registered for invocation: %s"), *Context.RuleInvocation.ToLogString())));
}

bool FSGSRuleRegistry::CheckInvariants() const
{
	bool bOk = true;
	bOk &= Rules.CheckInvariants();
	Rules.ForEach([&bOk](FSGSStableHandle Handle, const FSGSRuleEntry& Entry)
	{
		bOk &= Handle.CheckInvariants();
		bOk &= Entry.CheckInvariants();
	});
	return bOk;
}

void FSGSRuleRegistry::InitializeStore()
{
	RulesByName = Rules.RegisterUniqueIndex<FName>(
		FName(TEXT("SGS.RuleIndex.ByRuleName")),
		[](const FSGSRuleEntry& Entry)
		{
			return Entry.Descriptor.RuleName;
		});
	RulesByKind = Rules.RegisterNonUniqueIndex<FName>(
		FName(TEXT("SGS.RuleIndex.ByKind")),
		[](const FSGSRuleEntry& Entry)
		{
			return Entry.Descriptor.RuleKindTag;
		});
	RulesByIntent = Rules.RegisterNonUniqueIndex<FGameplayTag>(
		FName(TEXT("SGS.RuleIndex.ByIntent")),
		[](const FSGSRuleEntry& Entry)
		{
			return Entry.Descriptor.bWildcardIntent ? FGameplayTag() : Entry.Descriptor.IntentTag;
		});
	RulesBySubject = Rules.RegisterNonUniqueIndex<FName>(
		FName(TEXT("SGS.RuleIndex.BySubject")),
		[](const FSGSRuleEntry& Entry)
		{
			return Entry.Descriptor.bWildcardSubject ? NAME_None : Entry.Descriptor.SubjectName;
		});
	RulesByWindow = Rules.RegisterNonUniqueIndex<FName>(
		FName(TEXT("SGS.RuleIndex.ByWindow")),
		[](const FSGSRuleEntry& Entry)
		{
			return Entry.Descriptor.bWildcardWindow ? NAME_None : Entry.Descriptor.WindowName;
		});
	RulesByLookupKey = Rules.RegisterNonUniqueIndex<FSGSRuleLookupKey>(
		FName(TEXT("SGS.RuleIndex.ByLookupKey")),
		[](const FSGSRuleEntry& Entry)
		{
			return MakeRuleLookupKey(Entry.Descriptor);
		});
}

TArray<FSGSStableHandle> FSGSRuleRegistry::QueryCandidateHandles(const FSGSRuleInvocation& Invocation) const
{
	TArray<FSGSStableHandle> Handles;
	TSet<FSGSStableHandle> SeenHandles;

	if (!RulesByLookupKey.IsValid())
	{
		return Handles;
	}

	for (const FSGSRuleLookupKey& Key : MakeInvocationLookupKeys(Invocation))
	{
		const TArray<FSGSStableHandle> FoundHandles = RulesByLookupKey->FindCopy(Key);
		for (const FSGSStableHandle& Handle : FoundHandles)
		{
			if (!SeenHandles.Contains(Handle))
			{
				SeenHandles.Add(Handle);
				Handles.Add(Handle);
			}
		}
	}

	return Handles;
}

TArray<const FSGSRuleEntry*> FSGSRuleRegistry::FindCandidateEntries(const FSGSRuleInvocation& Invocation) const
{
	TArray<const FSGSRuleEntry*> Entries;
	if (!Invocation.CheckInvariants())
	{
		return Entries;
	}

	for (const FSGSStableHandle& Handle : QueryCandidateHandles(Invocation))
	{
		const FSGSRuleEntry* Entry = Rules.Find(Handle);
		if (Entry == nullptr || !DescriptorMatchesInvocation(Entry->Descriptor, Invocation))
		{
			continue;
		}
		Entries.Add(Entry);
	}

	Entries.Sort([](const FSGSRuleEntry& Left, const FSGSRuleEntry& Right)
	{
		if (Left.Descriptor.Priority != Right.Descriptor.Priority)
		{
			return Left.Descriptor.Priority > Right.Descriptor.Priority;
		}
		return Left.RegistrationOrder < Right.RegistrationOrder;
	});

	return Entries;
}

void SGSRules::RegisterDefaultRules(FSGSRuleRegistry& Registry)
{
	Registry.RegisterRule(MakeShared<FSGSDodgeResponseRule>());
	Registry.RegisterRule(MakeShared<FSGSDodgePassRule>());
	Registry.RegisterRule(MakeShared<FSGSDyingPeachRule>());
	Registry.RegisterRule(MakeShared<FSGSDyingPeachPassRule>());
	Registry.RegisterRule(MakeShared<FSGSSlashRule>());
	Registry.RegisterRule(MakeShared<FSGSPeachRule>());
	Registry.RegisterRule(MakeShared<FSGSPassRule>());
}
