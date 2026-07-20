#pragma once

#include "Server/Rules/Core/SGSTypedRule.h"
#include "Shared/Cards/SGSStandardCardDefinitions.h"

class SGS_API FSGSEquipmentUseRule final : public FSGSCardActionRuleBase<FSGSUseCardRulePayload>
{
public:
	explicit FSGSEquipmentUseRule(FSGSStandardCardDefinition InDefinition)
		: Definition(MoveTemp(InDefinition))
	{
	}

	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSUseCardRulePayload& Payload) const override;

private:
	FSGSStandardCardDefinition Definition;
};

class SGS_API FSGSEquipmentModifierRule final : public FSGSQueryRuleBase
{
public:
	explicit FSGSEquipmentModifierRule(FName InQueryName)
		: QueryName(InQueryName)
	{
	}

	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void ModifyNumericQuery(const FSGSRuleQueryContext& Context, FSGSNumericRuleQuery& Query) const override;

private:
	FName QueryName;
};

class SGS_API FSGSBaguaOptionRule final : public FSGSQueryRuleBase
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void CollectSkillOptions(const FSGSRuleQueryContext& Context, FSGSSkillOptionQuery& Query) const override;
};

class SGS_API FSGSBaguaResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

namespace SGSEquipmentRules
{
	SGS_API void Register(FSGSRuleRegistry& Registry);
}
