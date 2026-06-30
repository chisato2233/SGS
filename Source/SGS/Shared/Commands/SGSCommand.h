#pragma once

// SGS 规则层统一的玩家 / AI / RPC / 回放意图信封。
// Command 使用公共命令头 + typed payload；legacy mirror 字段仅用于兼容过渡。
// 所有 Command 在修改权威状态前都应先进入 FSGSCommandRouter。

#include "CoreMinimal.h"
#include "Shared/Core/SGSIds.h"
#include "Shared/Core/SGSTypes.h"
#include "StructUtils/InstancedStruct.h"

struct SGS_API FSGSCommand
{
	FSGSCommandId CommandId;
	int32 RequestId = 0;
	int32 SeatIndex = INDEX_NONE;
	FGameplayTag Type;
	FSGSPhase Phase = SGSGameplayTags::Phase_None.GetTag();
	FInstancedStruct Payload;

	// Legacy mirror fields retained for one migration iteration. New validation
	// and resolution code must read Payload instead of these fields.
	TArray<int32> CardIds;
	TArray<int32> TargetSeatIndices;
	TArray<FSGSStableHandle> CardHandles;
	TArray<FSGSStableHandle> TargetHandles;
	FName SkillName = NAME_None;
	FName SourceName = NAME_None;
	FName SourceChannel = NAME_None;
	TMap<FName, FString> Parameters;

	bool IsType(const FNativeGameplayTag& ExpectedType) const
	{
		return SGSMatchesExactTag(Type, ExpectedType);
	}

	bool HasPayload() const
	{
		return Payload.IsValid();
	}

	const UScriptStruct* GetPayloadStruct() const
	{
		return Payload.GetScriptStruct();
	}

	template <typename TPayload>
	const TPayload* GetPayload() const
	{
		return Payload.GetPtr<TPayload>();
	}

	template <typename TPayload>
	TPayload* GetMutablePayload()
	{
		return Payload.GetMutablePtr<TPayload>();
	}

	bool CheckInvariants() const
	{
		bool bOk = true;
		bOk &= CommandId.CheckInvariants();
		for (const FSGSStableHandle& Handle : CardHandles)
		{
			bOk &= Handle.CheckInvariants();
		}
		for (const FSGSStableHandle& Handle : TargetHandles)
		{
			bOk &= Handle.CheckInvariants();
		}
		return bOk;
	}

	FString GetPayloadTypeName() const
	{
		const UScriptStruct* PayloadStruct = GetPayloadStruct();
		return PayloadStruct != nullptr ? PayloadStruct->GetName() : TEXT("None");
	}

	FString ToLogString() const
	{
		return FString::Printf(
			TEXT("CommandId=%s RequestId=%d Seat=%d Type=%s Phase=%s Source=%s/%s Payload=%s"),
			*CommandId.ToLogString(),
			RequestId,
			SeatIndex,
			*Type.ToString(),
			*Phase.ToString(),
			*SourceChannel.ToString(),
			*SourceName.ToString(),
			*GetPayloadTypeName());
	}
};
