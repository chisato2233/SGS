#pragma once

// 常见 SGS EffectPipeline 步骤的工厂集合。
// 移牌、摸牌、伤害、回复、判定占位和响应窗口优先复用这里的标准步骤。
// 新卡牌应先组合这些步骤，只有能力不足时再补专门规则代码。

#include "CoreMinimal.h"
#include "Logic/Cards/SGSCardTypes.h"
#include "Logic/Effects/SGSEffectPipeline.h"

class USGSCard;

namespace SGSStandardEffectSteps
{
	SGS_API FSGSEffectStep MakeMoveCardsStep(
		TArray<USGSCard*> Cards,
		FSGSCardZone FromZone,
		int32 FromSeat,
		FSGSCardZone ToZone,
		int32 ToSeat);

	SGS_API FSGSEffectStep MakeDrawCardsStep(int32 SeatIndex, int32 Count);
	SGS_API FSGSEffectStep MakeDamageStep(int32 SourceSeat, int32 TargetSeat, int32 Amount);
	SGS_API FSGSEffectStep MakeHealStep(int32 SeatIndex, int32 Amount);
	SGS_API FSGSEffectStep MakeJudgementPlaceholderStep(int32 SeatIndex, FName Reason);
	SGS_API FSGSEffectStep MakeReactionWindowStep(int32 SeatIndex, FName WindowName);
	SGS_API FSGSEffectStep MakeExpireActiveEffectStep(FSGSStableHandle EffectHandle, FName Reason);
}
