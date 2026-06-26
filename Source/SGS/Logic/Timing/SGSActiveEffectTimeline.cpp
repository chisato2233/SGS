#include "Logic/Timing/SGSActiveEffectTimeline.h"

FSGSActiveEffectTimeline::FSGSActiveEffectTimeline()
{
	RegisterIndexes();
}

void FSGSActiveEffectTimeline::Reset()
{
	Effects.Reset();
	EffectsByTag.Empty();
	RegisterIndexes();
}

FSGSStableHandle FSGSActiveEffectTimeline::Add(FSGSActiveEffect Effect)
{
	Effect.bActive = true;
	Effect.ExpireReason = NAME_None;
	const FSGSStableHandle Handle = Effects.Add(MoveTemp(Effect));
	RebuildTagIndex();
	return Handle;
}

FSGSStatus FSGSActiveEffectTimeline::Expire(FSGSStableHandle Handle, FSGSTimingPoint At, FName Reason)
{
	if (Reason.IsNone())
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.ActiveEffect.MissingExpireReason")),
			FString::Printf(TEXT("Handle=%s"), *Handle.ToLogString())));
	}

	const FSGSStatus Status = Effects.Modify(Handle, [At, Reason](FSGSActiveEffect& Effect)
	{
		Effect.bActive = false;
		Effect.ExpiredAt = At;
		Effect.ExpireReason = Reason;
	});
	if (Status.HasError())
	{
		return Status;
	}

	RebuildTagIndex();
	return MakeValue();
}

TArray<FSGSStableHandle> FSGSActiveEffectTimeline::QueryActive() const
{
	TArray<FSGSStableHandle> Handles;
	Effects.ForEach([&Handles](FSGSStableHandle Handle, const FSGSActiveEffect& Effect)
	{
		if (Effect.bActive)
		{
			Handles.Add(Handle);
		}
	});
	return Handles;
}

TArray<FSGSStableHandle> FSGSActiveEffectTimeline::QueryByOwner(int32 OwnerSeat, bool bActiveOnly) const
{
	return EffectsByOwner.IsValid()
		? FilterActive(EffectsByOwner->FindCopy(OwnerSeat), bActiveOnly)
		: TArray<FSGSStableHandle>();
}

TArray<FSGSStableHandle> FSGSActiveEffectTimeline::QueryByTarget(int32 TargetSeat, bool bActiveOnly) const
{
	return EffectsByTarget.IsValid()
		? FilterActive(EffectsByTarget->FindCopy(TargetSeat), bActiveOnly)
		: TArray<FSGSStableHandle>();
}

TArray<FSGSStableHandle> FSGSActiveEffectTimeline::QueryByTag(FGameplayTag Tag, bool bActiveOnly) const
{
	TArray<FSGSStableHandle> Handles;
	EffectsByTag.MultiFind(Tag, Handles);
	return FilterActive(Handles, bActiveOnly);
}

bool FSGSActiveEffectTimeline::CheckInvariants() const
{
	bool bOk = Effects.CheckInvariants();
	Effects.ForEach([this, &bOk](FSGSStableHandle Handle, const FSGSActiveEffect& Effect)
	{
		bOk &= Effect.CheckInvariants();
		if (Effect.PrimaryTag.IsValid())
		{
			TArray<FSGSStableHandle> Handles;
			EffectsByTag.MultiFind(Effect.PrimaryTag, Handles);
			bOk &= ensureMsgf(Handles.Contains(Handle), TEXT("ActiveEffect tag index missing primary tag."));
		}
		for (const FGameplayTag& Tag : Effect.Tags)
		{
			TArray<FSGSStableHandle> Handles;
			EffectsByTag.MultiFind(Tag, Handles);
			bOk &= ensureMsgf(Handles.Contains(Handle), TEXT("ActiveEffect tag index missing secondary tag."));
		}
	});
	return bOk;
}

void FSGSActiveEffectTimeline::RegisterIndexes()
{
	EffectsByOwner = Effects.RegisterNonUniqueIndex<int32>(
		FName(TEXT("ActiveEffectOwner")),
		[](const FSGSActiveEffect& Effect) { return Effect.OwnerSeat; });
	EffectsByTarget = Effects.RegisterNonUniqueIndex<int32>(
		FName(TEXT("ActiveEffectTarget")),
		[](const FSGSActiveEffect& Effect) { return Effect.TargetSeat; });
	EffectsByPrimaryTag = Effects.RegisterNonUniqueIndex<FGameplayTag>(
		FName(TEXT("ActiveEffectPrimaryTag")),
		[](const FSGSActiveEffect& Effect) { return Effect.PrimaryTag; });
}

void FSGSActiveEffectTimeline::RebuildTagIndex()
{
	EffectsByTag.Empty();
	Effects.ForEach([this](FSGSStableHandle Handle, const FSGSActiveEffect& Effect)
	{
		if (Effect.PrimaryTag.IsValid())
		{
			EffectsByTag.Add(Effect.PrimaryTag, Handle);
		}
		for (const FGameplayTag& Tag : Effect.Tags)
		{
			if (Tag.IsValid())
			{
				EffectsByTag.Add(Tag, Handle);
			}
		}
	});
}

TArray<FSGSStableHandle> FSGSActiveEffectTimeline::FilterActive(const TArray<FSGSStableHandle>& Handles, bool bActiveOnly) const
{
	TArray<FSGSStableHandle> Result;
	Result.Reserve(Handles.Num());
	for (const FSGSStableHandle& Handle : Handles)
	{
		const FSGSActiveEffect* Effect = Effects.Find(Handle);
		if (Effect != nullptr && (!bActiveOnly || Effect->bActive))
		{
			Result.Add(Handle);
		}
	}
	return Result;
}
