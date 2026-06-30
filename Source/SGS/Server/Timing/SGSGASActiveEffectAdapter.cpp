#include "Server/Timing/SGSGASActiveEffectAdapter.h"

#include "Shared/Core/SGSError.h"

void FSGSGASActiveEffectAdapter::Reset()
{
	Bindings.Reset();
}

FSGSStatus FSGSGASActiveEffectAdapter::AddBinding(FSGSGASActiveEffectBinding Binding)
{
	if (!Binding.CheckInvariants())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.GASAdapter.InvalidBinding")),
			FString::Printf(TEXT("ActiveEffect=%s Context=%s"), *Binding.ActiveEffectHandle.ToLogString(), *Binding.Context)));
	}

	Bindings.Add(MoveTemp(Binding));
	return MakeValue();
}

int32 FSGSGASActiveEffectAdapter::RemoveBindingsForEffect(FSGSStableHandle ActiveEffectHandle)
{
	return Bindings.RemoveAll([ActiveEffectHandle](const FSGSGASActiveEffectBinding& Binding)
	{
		return Binding.ActiveEffectHandle == ActiveEffectHandle;
	});
}

int32 FSGSGASActiveEffectAdapter::RemoveBindingsForGameplayEffect(FActiveGameplayEffectHandle GameplayEffectHandle)
{
	return Bindings.RemoveAll([GameplayEffectHandle](const FSGSGASActiveEffectBinding& Binding)
	{
		return Binding.GameplayEffectHandle == GameplayEffectHandle;
	});
}

TArray<FSGSGASActiveEffectBinding> FSGSGASActiveEffectAdapter::QueryByEffect(FSGSStableHandle ActiveEffectHandle) const
{
	TArray<FSGSGASActiveEffectBinding> Result;
	for (const FSGSGASActiveEffectBinding& Binding : Bindings)
	{
		if (Binding.ActiveEffectHandle == ActiveEffectHandle)
		{
			Result.Add(Binding);
		}
	}
	return Result;
}

bool FSGSGASActiveEffectAdapter::CheckInvariants() const
{
	bool bOk = true;
	for (const FSGSGASActiveEffectBinding& Binding : Bindings)
	{
		bOk &= Binding.CheckInvariants();
	}
	return bOk;
}
