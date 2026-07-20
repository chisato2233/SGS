#include "Server/Skills/SGSSkillDefinition.h"

#include "Server/Skills/SGSStandardSkillRules.h"

namespace
{
class FSGSZhihengAIEvaluator final : public ISGSAIActionEvaluator
{
public:
	virtual FName GetEvaluatorId() const override { return FName(TEXT("SGS.AI.Skill.Zhiheng")); }
	virtual double Evaluate(const FSGSAIEvaluationContext&, const FSGSAIActionCandidate& Candidate) const override
	{
		return Candidate.SkillName == TEXT("Zhiheng")
			? 8.0 + 3.0 * static_cast<double>(Candidate.SelectedCardIds.Num())
			: 0.0;
	}
};

TArray<FSGSSkillDefinition> MakeDefinitions()
{
	FSGSSkillDefinition Paoxiao;
	Paoxiao.SkillName = FName(TEXT("Paoxiao"));
	Paoxiao.DisplayName = FName(TEXT("咆哮"));
	Paoxiao.Rules.Add(MakeShared<FSGSPaoxiaoModifierRule>());

	FSGSSkillDefinition Wusheng;
	Wusheng.SkillName = FName(TEXT("Wusheng"));
	Wusheng.DisplayName = FName(TEXT("武圣"));
	Wusheng.Rules.Add(MakeShared<FSGSWushengViewAsRule>());
	Wusheng.Rules.Add(MakeShared<FSGSWushengResponseRule>());

	FSGSSkillDefinition Jianxiong;
	Jianxiong.SkillName = FName(TEXT("Jianxiong"));
	Jianxiong.DisplayName = FName(TEXT("奸雄"));
	Jianxiong.Rules.Add(MakeShared<FSGSJianxiongTriggerRule>());

	FSGSSkillDefinition Zhiheng;
	Zhiheng.SkillName = FName(TEXT("Zhiheng"));
	Zhiheng.DisplayName = FName(TEXT("制衡"));
	Zhiheng.Rules.Add(MakeShared<FSGSZhihengActionRule>());
	Zhiheng.AIEvaluators.Add(MakeShared<FSGSZhihengAIEvaluator>());

	FSGSSkillDefinition Wushuang;
	Wushuang.SkillName = FName(TEXT("Wushuang"));
	Wushuang.DisplayName = FName(TEXT("无双"));
	Wushuang.Rules.Add(MakeShared<FSGSWushuangModifierRule>());

	FSGSSkillDefinition Rende;
	Rende.SkillName = FName(TEXT("Rende"));
	Rende.DisplayName = FName(TEXT("仁德"));
	Rende.Rules.Add(MakeShared<FSGSRendeActionRule>());

	FSGSSkillDefinition Fankui;
	Fankui.SkillName = FName(TEXT("Fankui"));
	Fankui.DisplayName = FName(TEXT("反馈"));
	Fankui.Rules.Add(MakeShared<FSGSFankuiTriggerRule>());

	return {
		MoveTemp(Paoxiao), MoveTemp(Wusheng), MoveTemp(Jianxiong), MoveTemp(Zhiheng),
		MoveTemp(Wushuang), MoveTemp(Rende), MoveTemp(Fankui)
	};
}
}

void SGSStandardSkills::Register(FSGSRuleRegistry& RuleRegistry, FSGSAIEvaluatorRegistry& AIRegistry)
{
	for (const FSGSSkillDefinition& Definition : MakeDefinitions())
	{
		Definition.Register(RuleRegistry, AIRegistry);
	}
}
