#pragma once

#include "Server/Rules/Core/SGSTypedRule.h"

class SGS_API FSGSPaoxiaoModifierRule final : public FSGSQueryRuleBase
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void ModifyNumericQuery(const FSGSRuleQueryContext& Context, FSGSNumericRuleQuery& Query) const override;
};

class SGS_API FSGSWushuangModifierRule final : public FSGSQueryRuleBase
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void ModifyNumericQuery(const FSGSRuleQueryContext& Context, FSGSNumericRuleQuery& Query) const override;
};

class SGS_API FSGSWushengViewAsRule final : public FSGSActionRuleBase<FSGSActivateSkillRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void CollectSkillOptions(const FSGSRuleQueryContext& Context, FSGSSkillOptionQuery& Query) const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
};

class SGS_API FSGSJianxiongTriggerRule final : public FSGSTriggerRuleBase<FSGSRuleEventPayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRuleEventPayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRuleEventPayload& Payload) const override;
};

class SGS_API FSGSFankuiTriggerRule final : public FSGSTriggerRuleBase<FSGSRuleEventPayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRuleEventPayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRuleEventPayload& Payload) const override;
};

class SGS_API FSGSWushengResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSZhihengActionRule final : public FSGSActionRuleBase<FSGSActivateSkillRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void CollectSkillOptions(const FSGSRuleQueryContext& Context, FSGSSkillOptionQuery& Query) const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
};

class SGS_API FSGSRendeActionRule final : public FSGSActionRuleBase<FSGSActivateSkillRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void CollectSkillOptions(const FSGSRuleQueryContext& Context, FSGSSkillOptionQuery& Query) const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
};
