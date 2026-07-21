#pragma once

#include "Server/Rules/Core/SGSRuleTypes.h"

template <typename TPayload>
class TSGSTypedRule : public ISGSRule
{
public:
	virtual const UScriptStruct* GetExpectedPayloadStruct() const override
	{
		return TPayload::StaticStruct();
	}

	virtual bool CanHandle(const FSGSRuleExecutionContext& Context) const override
	{
		const TPayload* Payload = Context.RuleInvocation.GetPayload<TPayload>();
		return Payload != nullptr && CanHandlePayload(Context, *Payload);
	}

	virtual FSGSStatus Validate(FSGSRuleExecutionContext& Context) const override
	{
		const TPayload* Payload = Context.RuleInvocation.GetPayload<TPayload>();
		if (Payload == nullptr)
		{
			return MakePayloadMismatchError(Context);
		}

		return ValidatePayload(Context, *Payload);
	}

	virtual FSGSStatus Execute(FSGSRuleExecutionContext& Context) const override
	{
		const TPayload* Payload = Context.RuleInvocation.GetPayload<TPayload>();
		if (Payload == nullptr)
		{
			return MakePayloadMismatchError(Context);
		}

		return ExecutePayload(Context, *Payload);
	}

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const TPayload& Payload) const
	{
		return true;
	}

	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const TPayload& Payload) const
	{
		return MakeValue();
	}

	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const TPayload& Payload) const = 0;

	FSGSStatus MakePayloadMismatchError(const FSGSRuleExecutionContext& Context) const
	{
		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.Rule.PayloadTypeMismatch")),
			FString::Printf(
				TEXT("Rule %s requires payload %s but received %s."),
				*GetRuleName().ToString(),
				*TPayload::StaticStruct()->GetName(),
				*Context.RuleInvocation.GetPayloadTypeName())));
	}
};

template <typename TPayload>
class FSGSActionRuleBase : public TSGSTypedRule<TPayload>
{
};

template <typename TPayload>
class FSGSCardActionRuleBase : public FSGSActionRuleBase<TPayload>
{
};

template <typename TPayload>
class FSGSResponseRuleBase : public TSGSTypedRule<TPayload>
{
};

template <typename TPayload>
class FSGSTriggerRuleBase : public TSGSTypedRule<TPayload>
{
};

class SGS_API FSGSQueryRuleBase : public ISGSRule
{
public:
	virtual const UScriptStruct* GetExpectedPayloadStruct() const override { return nullptr; }
	virtual bool CanHandle(const FSGSRuleExecutionContext& Context) const override
	{
		(void)Context;
		return false;
	}
	virtual FSGSStatus Validate(FSGSRuleExecutionContext& Context) const override
	{
		(void)Context;
		return MakeValue();
	}
	virtual FSGSStatus Execute(FSGSRuleExecutionContext& Context) const override
	{
		(void)Context;
		return MakeValue();
	}
};
