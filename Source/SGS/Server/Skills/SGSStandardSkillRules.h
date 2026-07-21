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

class SGS_API FSGSJiuyuanModifierRule final : public FSGSQueryRuleBase
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void ModifyNumericQuery(const FSGSRuleQueryContext& Context, FSGSNumericRuleQuery& Query) const override;
};

class SGS_API FSGSHujiaOptionRule final : public FSGSQueryRuleBase
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void CollectSkillOptions(const FSGSRuleQueryContext& Context, FSGSSkillOptionQuery& Query) const override;
};

class SGS_API FSGSJijiangOptionRule final : public FSGSQueryRuleBase
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;
	virtual void CollectSkillOptions(const FSGSRuleQueryContext& Context, FSGSSkillOptionQuery& Query) const override;
};

class SGS_API FSGSHujiaResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSJijiangActionRule final : public FSGSActionRuleBase<FSGSActivateSkillRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSActivateSkillRulePayload& Payload) const override;
};

class SGS_API FSGSJijiangResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSLordAssistCardRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSLordAssistPassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
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

class SGS_API FSGSJianxiongResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSJianxiongPassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
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

class SGS_API FSGSFankuiResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSFankuiPassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
};

class SGS_API FSGSFankuiCardChoiceRule final : public FSGSResponseRuleBase<FSGSChooseCardsRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSChooseCardsRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSChooseCardsRulePayload& Payload) const override;
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

class SGS_API FSGSGanglieTriggerRule final : public FSGSTriggerRuleBase<FSGSRuleEventPayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRuleEventPayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRuleEventPayload& Payload) const override;
};

class SGS_API FSGSGanglieInvokeRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSGanglieDiscardRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSGangliePassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
};

class SGS_API FSGSGuicaiTriggerRule final : public FSGSTriggerRuleBase<FSGSRuleEventPayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRuleEventPayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRuleEventPayload& Payload) const override;
};

class SGS_API FSGSGuicaiResponseRule final : public FSGSResponseRuleBase<FSGSRespondCardRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual bool CanHandlePayload(const FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ValidatePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSRespondCardRulePayload& Payload) const override;
};

class SGS_API FSGSGuicaiPassRule final : public FSGSResponseRuleBase<FSGSPassRulePayload>
{
public:
	virtual FName GetRuleName() const override;
	virtual FSGSRuleDescriptor GetDescriptor() const override;

protected:
	virtual FSGSStatus ExecutePayload(FSGSRuleExecutionContext& Context, const FSGSPassRulePayload& Payload) const override;
};

namespace SGSStandardSkillRules
{
	SGS_API FName GanglieAfterJudgementContinuation();
	SGS_API FSGSStatus ContinueGanglieAfterJudgement(
		FSGSRuleExecutionContext& Context,
		int32 ResultCardId);
}
