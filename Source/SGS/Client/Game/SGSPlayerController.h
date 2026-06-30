#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Shared/UI/SGSTableViewTypes.h"
#include "SGSPlayerController.generated.h"

class SSGSTableHudWidget;
class USGSLocalHumanDecisionAgent;

UCLASS()
class SGS_API ASGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void AttachToMatch(
		TFunction<FSGSTableViewSnapshot()> InSnapshotProvider,
		USGSLocalHumanDecisionAgent* InDecisionAgent,
		int32 InViewerSeat);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RemoveTableHud();

	TFunction<FSGSTableViewSnapshot()> SnapshotProvider;
	TWeakObjectPtr<USGSLocalHumanDecisionAgent> DecisionAgent;
	int32 ViewerSeat = INDEX_NONE;
	TSharedPtr<SSGSTableHudWidget> TableHudWidget;
};
