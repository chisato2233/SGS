#pragma once

// 常见 SGS EffectPipeline 步骤的工厂集合。
// 移牌、摸牌、伤害、回复、判定占位和响应窗口优先复用这里的标准步骤。
// 新卡牌应先组合这些步骤，只有能力不足时再补专门规则代码。

#include "CoreMinimal.h"
#include "Shared/Cards/SGSCardTypes.h"
#include "Server/Effects/SGSEffectPipeline.h"

class USGSCard;

namespace SGSCardMoveReasons
{
	SGS_API FName InitialDeal();
	SGS_API FName Draw();
	SGS_API FName RewardDraw();
	SGS_API FName Use();
	SGS_API FName Respond();
	SGS_API FName Discard();
	SGS_API FName Cleanup();
	SGS_API FName Gain();
}

struct SGS_API FSGSCardMoveEventMetadata
{
	FName Reason = NAME_None;
	TArray<int32> RelatedTargetSeatIndices;
};

namespace SGSStandardEffectSteps
{
	SGS_API FSGSEffectStep MakeMoveCardsStep(
		TArray<USGSCard*> Cards,
		FSGSCardZone FromZone,
		int32 FromSeat,
		FSGSCardZone ToZone,
		int32 ToSeat,
		FSGSCardMoveEventMetadata Metadata = {});

	SGS_API FSGSEffectStep MakeDrawCardsStep(
		int32 SeatIndex,
		int32 Count,
		FSGSCardMoveEventMetadata Metadata = {});
	SGS_API FSGSEffectStep MakeRevealTopCardsStep(
		int32 TemporarySeatIndex,
		int32 Count,
		TSharedRef<TArray<TObjectPtr<USGSCard>>> OutCards,
		TArray<int32> RelatedTargetSeatIndices = {});
	SGS_API FSGSEffectStep MakeDamageStep(int32 SourceSeat, int32 TargetSeat, int32 Amount);
	SGS_API FSGSEffectStep MakeHealStep(int32 SeatIndex, int32 Amount);
	SGS_API FSGSEffectStep MakeEquipCardStep(int32 SeatIndex, USGSCard* Card, FSGSEquipSlot Slot);
	SGS_API FSGSEffectStep MakeJudgementDrawStep(
		int32 SeatIndex,
		TSharedRef<TObjectPtr<USGSCard>> OutJudgementCard);
	SGS_API FSGSEffectStep MakeEliminateSeatStep(int32 SeatIndex, int32 SourceSeat, FName Reason);
	SGS_API FSGSEffectStep MakeJudgementPlaceholderStep(int32 SeatIndex, FName Reason);
	SGS_API FSGSEffectStep MakeReactionWindowStep(int32 SeatIndex, FName WindowName);
	SGS_API FSGSEffectStep MakeExpireActiveEffectStep(FSGSStableHandle EffectHandle, FName Reason);
	SGS_API FSGSEffectStep MakeRuleOutcomeStep(FName EventName, FString Payload);
}
