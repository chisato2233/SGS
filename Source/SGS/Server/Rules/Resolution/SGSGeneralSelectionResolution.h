#pragma once

#include "CoreMinimal.h"
#include "SGSGeneralSelectionResolution.generated.h"

USTRUCT()
struct SGS_API FSGSGeneralSelectionResolutionState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int32> SeatOrder;

	UPROPERTY()
	int32 NextSeatIndex = 0;

	UPROPERTY()
	TArray<FName> AvailableGeneralIds;
};

namespace SGSGeneralSelectionResolution
{
	inline FName FrameRuleName() { return FName(TEXT("SGS.Rule.GeneralSelection.Frame")); }
	inline FName WindowName() { return FName(TEXT("GameSetup.GeneralSelection")); }
	inline FName ChoiceName() { return FName(TEXT("GeneralSelection")); }
}
