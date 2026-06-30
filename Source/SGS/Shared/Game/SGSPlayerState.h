#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "SGSPlayerState.generated.h"

UCLASS()
class SGS_API ASGSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetSeatIndex() const { return SeatIndex; }
	void SetSeatIndex(int32 InSeatIndex) { SeatIndex = InSeatIndex; }

private:
	UPROPERTY(Replicated)
	int32 SeatIndex = INDEX_NONE;
};
