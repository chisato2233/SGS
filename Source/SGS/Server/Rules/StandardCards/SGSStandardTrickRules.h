#pragma once

#include "Server/Rules/Core/SGSTypedRule.h"
#include "SGSStandardTrickRules.generated.h"

USTRUCT()
struct SGS_API FSGSStandardTrickResolutionState
{
	GENERATED_BODY()

	UPROPERTY()
	FName CardName = NAME_None;

	UPROPERTY()
	int32 SourceSeat = INDEX_NONE;

	UPROPERTY()
	TArray<int32> TargetSeatIndices;

	UPROPERTY()
	int32 TargetIndex = 0;

	UPROPERTY()
	FName Stage = NAME_None;

	UPROPERTY()
	int32 NullificationNextSeat = INDEX_NONE;

	UPROPERTY()
	int32 NullificationVisited = 0;

	UPROPERTY()
	bool bNullified = false;

	UPROPERTY()
	FName RequiredResponseCard = NAME_None;

	UPROPERTY()
	int32 DuelResponderSeat = INDEX_NONE;

	UPROPERTY()
	int32 DuelOpponentSeat = INDEX_NONE;

	UPROPERTY()
	int32 RequiredResponseCount = 1;

	UPROPERTY()
	int32 ResponseCount = 0;
};

class SGS_API FSGSStandardTrickUseRule final : public FSGSCardActionRuleBase<FSGSUseCardRulePayload>
{
public:
	explicit FSGSStandardTrickUseRule(FName InCardName)
		: CardName(InCardName)
	{
	}

	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const override;

private:
	FName CardName;
};

class SGS_API FSGSTrickNullificationPassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
};

class SGS_API FSGSTrickNullificationResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSTrickEffectPassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
};

class SGS_API FSGSTrickEffectResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

namespace SGSStandardTrickRules
{
	SGS_API FName NullificationWindow();
	SGS_API FName EffectResponseWindow();
	SGS_API FName ResumeContinuation();
	SGS_API FSGSStatus Continue(FSGSRuleExecutionContext& Context);
	SGS_API FSGSStatus ContinueAfterAcceptedResponse(FSGSRuleExecutionContext& Context);
	SGS_API FSGSStatus ContinueAfterDeclinedResponse(FSGSRuleExecutionContext& Context);
	SGS_API void Register(FSGSRuleRegistry& Registry);
}
