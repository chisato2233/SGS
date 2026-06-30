#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Logic/Players/SGSDecisionAgent.h"
#include "SGSScriptedDecisionAgent.generated.h"

struct FSGSScriptedDecisionStep
{
	bool bPass = true;
	bool bUseInvalidCard = false;
	FName CardName = NAME_None;
	int32 TargetSeatIndex = INDEX_NONE;
};

// Deterministic decision agent for automation and developer smoke paths.
// It exercises the same Command boundary as AI/UI/RPC instead of editing game state directly.
UCLASS()
class SGS_API USGSScriptedDecisionAgent : public UObject, public ISGSDecisionAgent
{
	GENERATED_BODY()

public:
	void EnqueuePlayCard(FName CardName, int32 TargetSeatIndex = INDEX_NONE);
	void EnqueuePlayPass();
	void EnqueueInvalidPlayCard(int32 TargetSeatIndex = INDEX_NONE);
	void EnqueueResponseCard(FName CardName, int32 TargetSeatIndex = INDEX_NONE);
	void EnqueueResponsePass();

	virtual void RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided) override;
	virtual void RequestResponseAction(const FSGSResponseRequest& Request, FSGSResponseDecisionDelegate OnDecided) override;

private:
	static FSGSScriptedDecisionStep MakeCardStep(FName CardName, int32 TargetSeatIndex);
	FSGSScriptedDecisionStep PopPlayStep();
	FSGSScriptedDecisionStep PopResponseStep();

	TArray<FSGSScriptedDecisionStep> PlaySteps;
	TArray<FSGSScriptedDecisionStep> ResponseSteps;
};
