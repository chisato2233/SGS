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

class FSGSOptionalTriggerAIEvaluator final : public ISGSAIActionEvaluator
{
public:
	explicit FSGSOptionalTriggerAIEvaluator(FName InSkillName, double InScore)
		: SkillName(InSkillName), Score(InScore) {}

	virtual FName GetEvaluatorId() const override
	{
		return FName(*FString::Printf(TEXT("SGS.AI.Skill.%s.OptionalTrigger"), *SkillName.ToString()));
	}

	virtual double Evaluate(const FSGSAIEvaluationContext&, const FSGSAIActionCandidate& Candidate) const override
	{
		return Candidate.SkillName == SkillName ? Score : 0.0;
	}

private:
	FName SkillName;
	double Score = 0.0;
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
	Jianxiong.Rules.Add(MakeShared<FSGSJianxiongResponseRule>());
	Jianxiong.Rules.Add(MakeShared<FSGSJianxiongPassRule>());
	Jianxiong.AIEvaluators.Add(MakeShared<FSGSOptionalTriggerAIEvaluator>(FName(TEXT("Jianxiong")), 80.0));

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
	Fankui.Rules.Add(MakeShared<FSGSFankuiResponseRule>());
	Fankui.Rules.Add(MakeShared<FSGSFankuiPassRule>());
	Fankui.Rules.Add(MakeShared<FSGSFankuiCardChoiceRule>());
	Fankui.AIEvaluators.Add(MakeShared<FSGSOptionalTriggerAIEvaluator>(FName(TEXT("Fankui")), 70.0));

	FSGSSkillDefinition Ganglie;
	Ganglie.SkillName = FName(TEXT("Ganglie"));
	Ganglie.DisplayName = FName(TEXT("刚烈"));
	Ganglie.Rules.Add(MakeShared<FSGSGanglieTriggerRule>());
	Ganglie.Rules.Add(MakeShared<FSGSGanglieInvokeRule>());
	Ganglie.Rules.Add(MakeShared<FSGSGanglieDiscardRule>());
	Ganglie.Rules.Add(MakeShared<FSGSGangliePassRule>());
	Ganglie.AIEvaluators.Add(MakeShared<FSGSOptionalTriggerAIEvaluator>(FName(TEXT("Ganglie")), 60.0));
	Ganglie.AIEvaluators.Add(MakeShared<FSGSOptionalTriggerAIEvaluator>(FName(TEXT("GanglieDiscard")), 40.0));

	FSGSSkillDefinition Guicai;
	Guicai.SkillName = FName(TEXT("Guicai"));
	Guicai.DisplayName = FName(TEXT("鬼才"));
	Guicai.Rules.Add(MakeShared<FSGSGuicaiTriggerRule>());
	Guicai.Rules.Add(MakeShared<FSGSGuicaiResponseRule>());
	Guicai.Rules.Add(MakeShared<FSGSGuicaiPassRule>());
	Guicai.AIEvaluators.Add(MakeShared<FSGSOptionalTriggerAIEvaluator>(FName(TEXT("Guicai")), 20.0));

	FSGSSkillDefinition Jiuyuan;
	Jiuyuan.SkillName = FName(TEXT("Jiuyuan"));
	Jiuyuan.DisplayName = FName(TEXT("救援"));
	Jiuyuan.Rules.Add(MakeShared<FSGSJiuyuanModifierRule>());

	FSGSSkillDefinition Hujia;
	Hujia.SkillName = FName(TEXT("Hujia"));
	Hujia.DisplayName = FName(TEXT("护驾"));
	Hujia.Rules.Add(MakeShared<FSGSHujiaOptionRule>());
	Hujia.Rules.Add(MakeShared<FSGSHujiaResponseRule>());
	Hujia.Rules.Add(MakeShared<FSGSLordAssistCardRule>());
	Hujia.Rules.Add(MakeShared<FSGSLordAssistPassRule>());
	Hujia.AIEvaluators.Add(MakeShared<FSGSOptionalTriggerAIEvaluator>(FName(TEXT("Hujia")), 90.0));

	FSGSSkillDefinition Jijiang;
	Jijiang.SkillName = FName(TEXT("Jijiang"));
	Jijiang.DisplayName = FName(TEXT("激将"));
	Jijiang.Rules.Add(MakeShared<FSGSJijiangOptionRule>());
	Jijiang.Rules.Add(MakeShared<FSGSJijiangActionRule>());
	Jijiang.Rules.Add(MakeShared<FSGSJijiangResponseRule>());
	Jijiang.AIEvaluators.Add(MakeShared<FSGSOptionalTriggerAIEvaluator>(FName(TEXT("Jijiang")), 50.0));

	return {
		MoveTemp(Paoxiao), MoveTemp(Wusheng), MoveTemp(Jianxiong), MoveTemp(Zhiheng),
		MoveTemp(Wushuang), MoveTemp(Rende), MoveTemp(Fankui), MoveTemp(Ganglie),
		MoveTemp(Guicai), MoveTemp(Jiuyuan), MoveTemp(Hujia), MoveTemp(Jijiang)
	};
}
}

void SGSStandardSkills::Register(FSGSRuleRegistry& RuleRegistry, FSGSAIEvaluatorRegistry& AIRegistry)
{
	for (const FSGSSkillDefinition& Definition : MakeDefinitions())
	{
		Definition.Register(RuleRegistry, AIRegistry);
	}
	AIRegistry.RegisterSkillSemantics(
		FName(TEXT("Rende")),
		{ 14.0, -15.0, 35.0, 4.0, SGSAIEffectAttitudes::Helpful() });
}
