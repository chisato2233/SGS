#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SGSPlayerController.generated.h"

class SSGSTableHudWidget;
class USGSGameDriver;
class USGSLocalHumanDecisionAgent;

UCLASS()
class SGS_API ASGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void AttachToMatch(USGSGameDriver* InGameDriver, USGSLocalHumanDecisionAgent* InDecisionAgent, int32 InViewerSeat);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RemoveTableHud();

	TWeakObjectPtr<USGSGameDriver> GameDriver;
	TWeakObjectPtr<USGSLocalHumanDecisionAgent> DecisionAgent;
	int32 ViewerSeat = INDEX_NONE;
	TSharedPtr<SSGSTableHudWidget> TableHudWidget;
};
