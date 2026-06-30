#include "Shared/Game/SGSPlayerState.h"

#include "Net/UnrealNetwork.h"

void ASGSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASGSPlayerState, SeatIndex);
}
