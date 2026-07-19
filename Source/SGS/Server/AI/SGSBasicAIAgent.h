#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Shared/Decisions/SGSDecisionAgent.h"
#include "SGSBasicAIAgent.generated.h"

class USGSGameContext;

// 最小身份局 AI：使用权威座次和真实身份选择基础牌与目标。
// 它仍然通过 ISGSDecisionAgent 返回 Command，不直接修改规则状态。
UCLASS()
class SGS_API USGSBasicAIAgent : public UObject, public ISGSDecisionAgent
{
	GENERATED_BODY()

public:
	void BindToSeat(USGSGameContext* InContext, int32 InSeatIndex);

	virtual void RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided) override;
	virtual void RequestResponseAction(const FSGSResponseRequest& Request, FSGSResponseDecisionDelegate OnDecided) override;

private:
	int32 ChooseTarget(TConstArrayView<int32> CandidateSeatIndices) const;
	bool CanRescue(int32 TargetSeatIndex) const;

	TWeakObjectPtr<USGSGameContext> Context;
	int32 SeatIndex = INDEX_NONE;
};
