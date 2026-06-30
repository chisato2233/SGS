#pragma once

#include "CoreMinimal.h"
#include "Shared/UI/SGSTableViewTypes.h"

class USGSLocalHumanDecisionAgent;

class SGS_API FSGSLocalDecisionPromptViewModel
{
public:
	static void Apply(FSGSTableViewSnapshot& Snapshot, const USGSLocalHumanDecisionAgent* DecisionAgent);
};
