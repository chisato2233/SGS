#include "AI/SGSAutoPassAgent.h"

void USGSAutoPassAgent::RequestPlayPhaseAction(const FSGSPlayPhaseRequest& Request, FSGSPlayPhaseDecisionDelegate OnDecided)
{
	FSGSPlayPhaseDecision Decision;
	Decision.Command = FSGSCommand::MakePass(
		Request.CommandId,
		Request.RequestId,
		Request.SeatIndex,
		Request.Phase,
		FName(TEXT("AI")),
		FName(TEXT("AutoPass")));
	OnDecided.ExecuteIfBound(Decision);
}
