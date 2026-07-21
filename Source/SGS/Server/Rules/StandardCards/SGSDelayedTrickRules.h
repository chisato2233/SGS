#pragma once

#include "Server/Rules/Core/SGSTypedRule.h"
#include "SGSDelayedTrickRules.generated.h"

USTRUCT()
struct SGS_API FSGSDelayedTrickResolutionState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 JudgedSeat = INDEX_NONE;

	UPROPERTY()
	int32 DelayedTrickCardId = INDEX_NONE;

	UPROPERTY()
	FName CardName = NAME_None;
};

class SGS_API FSGSDelayedTrickJudgementRule final : public TSGSTypedRule<FSGSJudgementRulePayload>
{
public:
	explicit FSGSDelayedTrickJudgementRule(FName InCardName)
		: CardName(InCardName)
	{
	}

	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSJudgementRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSJudgementRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSJudgementRulePayload& Payload) const override;

private:
	FName CardName;
};

namespace SGSDelayedTrickRules
{
	SGS_API FName AfterJudgementContinuation();
	SGS_API FSGSStatus ContinueAfterJudgement(
		FSGSRuleExecutionContext& Context,
		int32 ResultCardId);
	SGS_API void Register(FSGSRuleRegistry& Registry);
}
