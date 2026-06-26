#pragma once

// SGS 规则层统一的玩家 / AI / RPC / 回放意图信封。
// Command 字段通过标签、句柄、名称和参数保持开放，不使用封闭内容枚举。
// 所有 Command 在修改权威状态前都应先进入 FSGSCommandRouter。

#include "CoreMinimal.h"
#include "Core/SGSIds.h"
#include "Core/SGSTypes.h"

struct SGS_API FSGSCommand
{
	FSGSCommandId CommandId;
	int32 RequestId = 0;
	int32 SeatIndex = INDEX_NONE;
	FGameplayTag Type;
	FSGSPhase Phase = SGSGameplayTags::Phase_None.GetTag();
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

	FString ToLogString() const
	{
		return FString::Printf(
			TEXT("CommandId=%s RequestId=%d Seat=%d Type=%s Phase=%s Source=%s/%s"),
			*CommandId.ToLogString(),
			RequestId,
			SeatIndex,
			*Type.ToString(),
			*Phase.ToString(),
			*SourceChannel.ToString(),
			*SourceName.ToString());
	}

	static FSGSCommand MakePass(
		FSGSCommandId InCommandId,
		int32 InRequestId,
		int32 InSeatIndex,
		FSGSPhase InPhase,
		FName InSourceChannel = TEXT("Server"),
		FName InSourceName = TEXT("Pass"))
	{
		FSGSCommand Command;
		Command.CommandId = InCommandId;
		Command.RequestId = InRequestId;
		Command.SeatIndex = InSeatIndex;
		Command.Type = SGSGameplayTags::PlayAction_Pass.GetTag();
		Command.Phase = InPhase;
		Command.SourceChannel = InSourceChannel;
		Command.SourceName = InSourceName;
		return Command;
	}
};
