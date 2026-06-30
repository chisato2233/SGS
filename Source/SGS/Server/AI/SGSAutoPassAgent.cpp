#include "Server/AI/SGSAutoPassAgent.h"

#include "Shared/Commands/SGSCommandFactory.h"
#include "Shared/Commands/SGSCommandPayloads.h"

void USGSAutoPassAgent::RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided)
{
	FSGSPlayPhaseDecision Decision;
	Decision.Command = FSGSCommandFactory::Make(
		FSGSCommandBuildRequest::FromDecisionRequest(Request, FName(TEXT("AI")), FName(TEXT("AutoPass"))),
		FSGSPassCommandPayload());
	OnDecided.ExecuteIfBound(Decision);
}
