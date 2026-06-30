#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Shared/Decisions/SGSDecisionAgent.h"
#include "SGSBasicAIAgent.generated.h"

// Plan0005 最小规则 smoke AI：只理解基础牌纵切。
// 它仍然通过 ISGSDecisionAgent 返回 Command，不直接修改规则状态。
UCLASS()
class SGS_API USGSBasicAIAgent : public UObject, public ISGSDecisionAgent
{
	GENERATED_BODY()

public:
	virtual void RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided) override;
	virtual void RequestResponseAction(const FSGSResponseRequest& Request, FSGSResponseDecisionDelegate OnDecided) override;
};
