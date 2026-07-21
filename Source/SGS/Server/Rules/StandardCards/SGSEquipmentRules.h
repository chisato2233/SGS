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

class SGS_API FSGSSpearOptionRule final : public FSGSQueryRuleBase
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void CollectSkillOptions(const FSGSRuleQueryContext& Context, FSGSSkillOptionQuery& Query) const override;
};

class SGS_API FSGSSpearActionRule final : public FSGSActionRuleBase<FSGSActivateSkillRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
};

class SGS_API FSGSSpearResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSEquipmentCardChoiceRule final : public FSGSResponseRuleBase<FSGSChooseCardsRulePayload>
{
public:
	FSGSEquipmentCardChoiceRule(FName InChoiceName, FName InWindowName)
		: ChoiceName(InChoiceName), WindowName(InWindowName) {}
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSChooseCardsRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSChooseCardsRulePayload& Payload) const override;

private:
	FName ChoiceName;
	FName WindowName;
};

class SGS_API FSGSEquipmentChoicePassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	explicit FSGSEquipmentChoicePassRule(FName InWindowName) : WindowName(InWindowName) {}
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;

private:
	FName WindowName;
};

class SGS_API FSGSBladeSlashRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSBladePassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
};

namespace SGSEquipmentRules
{
	SGS_API FName BaguaAfterJudgementContinuation();
	SGS_API FSGSStatus ContinueBaguaAfterJudgement(
		FSGSRuleExecutionContext& Context,
		int32 ResultCardId);
	SGS_API void Register(FSGSRuleRegistry& Registry);
}
