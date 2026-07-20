#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Shared/Decisions/SGSDecisionAgent.h"
#include "SGSBasicAIAgent.generated.h"

class USGSGameContext;
class FSGSAIEvaluatorRegistry;

// 服务器 AI 适配器：从受限 WorldView 评价规则层提供的合法候选。
UCLASS()
class SGS_API USGSBasicAIAgent : public UObject, public ISGSDecisionAgent
{
	GENERATED_BODY()

public:
	void BindToSeat(USGSGameContext* InContext, int32 InSeatIndex, const FSGSAIEvaluatorRegistry* InEvaluatorRegistry);

	virtual void RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided) override;
	virtual void RequestResponseAction(const FSGSResponseRequest& Request, FSGSResponseDecisionDelegate OnDecided) override;

private:
	TWeakObjectPtr<USGSGameContext> Context;
	const FSGSAIEvaluatorRegistry* EvaluatorRegistry = nullptr;
	int32 SeatIndex = INDEX_NONE;
};
