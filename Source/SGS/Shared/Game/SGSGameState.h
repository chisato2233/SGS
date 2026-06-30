#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Shared/UI/SGSTableViewTypes.h"

#include "SGSGameState.generated.h"

DECLARE_MULTICAST_DELEGATE(FSGSOnPublicSnapshotChanged);

UCLASS()
class SGS_API ASGSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	const FSGSTablePublicSnapshot& GetPublicSnapshot() const { return PublicSnapshot; }
	void PublishPublicSnapshot(FSGSTablePublicSnapshot InSnapshot);

	FSGSOnPublicSnapshotChanged& OnPublicSnapshotChanged() { return PublicSnapshotChanged; }

private:
	UFUNCTION()
	void OnRep_PublicSnapshot();

	UPROPERTY(ReplicatedUsing = OnRep_PublicSnapshot)
	FSGSTablePublicSnapshot PublicSnapshot;

	FSGSOnPublicSnapshotChanged PublicSnapshotChanged;
};
