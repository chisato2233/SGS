#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Logic/Players/SGSDecisionAgent.h"
#include "SGSAutoPassAgent.generated.h"

// 骨架期占位 AI：出牌阶段一律 Pass，立即（同步）应答。
// Plan 0010 起被参考 QSanguosha 的真正 AI 代理替换/继承。
UCLASS()
class SGS_API USGSAutoPassAgent : public UObject, public ISGSDecisionAgent
{
	GENERATED_BODY()

public:
	virtual void RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided) override;
};
