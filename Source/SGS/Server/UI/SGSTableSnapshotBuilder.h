#pragma once

#include "CoreMinimal.h"
#include "Shared/UI/SGSTableViewTypes.h"

class USGSGameDriver;

class SGS_API FSGSTableSnapshotBuilder
{
public:
	static FSGSTableViewSnapshot Build(const USGSGameDriver* Driver, int32 ViewerSeat);
};
