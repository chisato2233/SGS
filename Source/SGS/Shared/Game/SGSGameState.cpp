#include "Shared/Game/SGSGameState.h"

#include "Net/UnrealNetwork.h"

void ASGSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASGSGameState, PublicSnapshot);
}

void ASGSGameState::PublishPublicSnapshot(FSGSTablePublicSnapshot InSnapshot)
{
	InSnapshot.Revision = PublicSnapshot.Revision + 1;
	PublicSnapshot = MoveTemp(InSnapshot);
	PublicSnapshotChanged.Broadcast();
}

void ASGSGameState::OnRep_PublicSnapshot()
{
	PublicSnapshotChanged.Broadcast();
}
