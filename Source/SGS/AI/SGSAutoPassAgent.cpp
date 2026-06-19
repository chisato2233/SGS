#include "AI/SGSAutoPassAgent.h"

void USGSAutoPassAgent::RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided)
{
	FSGSPlayPhaseDecision Decision;
	Decision.Action = ESGSPlayActionType::Pass;
	OnDecided.ExecuteIfBound(Decision);
}
