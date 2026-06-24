#include "AI/SGSAutoPassAgent.h"

void USGSAutoPassAgent::RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided)
{
	FSGSPlayPhaseDecision Decision;
	Decision.Action = SGSGameplayTags::PlayAction_Pass.GetTag();
	OnDecided.ExecuteIfBound(Decision);
}
