#pragma once

#include "CoreMinimal.h"
#include "Shared/UI/SGSTableViewTypes.h"

class USGSGameDriver;
class USGSLocalHumanDecisionAgent;

class SGS_API FSGSTableSnapshotBuilder
{
public:
	static FSGSTablePublicSnapshot BuildPublicSnapshot(const USGSGameDriver* Driver);
	static FSGSPlayerPrivateSnapshot BuildPrivateSnapshot(
		const USGSGameDriver* Driver,
		const USGSLocalHumanDecisionAgent* DecisionAgent,
		int32 ViewerSeat);
	static FSGSTableViewSnapshot Build(const USGSGameDriver* Driver, int32 ViewerSeat);
	static FSGSTableViewSnapshot Build(
		const USGSGameDriver* Driver,
		const USGSLocalHumanDecisionAgent* DecisionAgent,
		int32 ViewerSeat);
};
