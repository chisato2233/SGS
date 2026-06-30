#pragma once

// 规则层所有随机性的审计入口。
// 生成模式记录 seed、step、purpose、range、command 和 event；重放模式校验同一条随机流。
// 业务代码应从这里取随机值，不应自己持有 FRandomStream。

#include "CoreMinimal.h"
#include "Shared/Core/SGSError.h"
#include "Shared/Core/SGSIds.h"
#include "Math/RandomStream.h"

struct SGS_API FSGSRandomLogEntry
{
	int32 Seed = 0;
	int32 Step = INDEX_NONE;
	FName Purpose = NAME_None;
	int32 MinInclusive = 0;
	int32 MaxInclusive = 0;
	int32 Output = 0;
	FSGSCommandId CommandId;
	FName EventName = NAME_None;
	FString Context;
};

struct SGS_API FSGSRandomAudit
{
	void Initialize(int32 InSeed);
	void InitializeReplay(int32 InSeed, TArray<FSGSRandomLogEntry> InReplayEntries);

	TSGSResult<int32> TryRandRange(
		FName Purpose,
		int32 MinInclusive,
		int32 MaxInclusive,
		FString Context,
		FSGSCommandId CommandId = FSGSCommandId(),
		FName EventName = NAME_None);
	int32 RandRange(
		FName Purpose,
		int32 MinInclusive,
		int32 MaxInclusive,
		FString Context,
		FSGSCommandId CommandId = FSGSCommandId(),
		FName EventName = NAME_None);
	TSGSResult<int32> TryRandIndex(
		FName Purpose,
		int32 NumOptions,
		FString Context,
		FSGSCommandId CommandId = FSGSCommandId(),
		FName EventName = NAME_None);
	int32 RandIndex(
		FName Purpose,
		int32 NumOptions,
		FString Context,
		FSGSCommandId CommandId = FSGSCommandId(),
		FName EventName = NAME_None);

	const TArray<FSGSRandomLogEntry>& GetEntries() const { return Entries; }
	bool CheckInvariants() const;
	int32 GetSeed() const { return Seed; }
	int32 GetNextStep() const { return NextStep; }
	bool IsReplayMode() const { return bReplayMode; }

private:
	int32 Seed = 0;
	int32 NextStep = 0;
	bool bReplayMode = false;
	FRandomStream Stream;
	TArray<FSGSRandomLogEntry> Entries;
};
