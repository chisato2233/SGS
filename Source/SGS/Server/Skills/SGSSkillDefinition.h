#pragma once

#include "CoreMinimal.h"
#include "Server/AI/SGSAIEvaluation.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"

struct SGS_API FSGSSkillDefinition
{
	FName SkillName = NAME_None;
	FName DisplayName = NAME_None;
	TArray<TSharedRef<ISGSRule>> Rules;
	TArray<TSharedRef<ISGSAIActionEvaluator>> AIEvaluators;

	void Register(FSGSRuleRegistry& RuleRegistry, FSGSAIEvaluatorRegistry& AIRegistry) const
	{
		for (const TSharedRef<ISGSRule>& Rule : Rules)
		{
			RuleRegistry.RegisterRule(Rule);
		}
		for (const TSharedRef<ISGSAIActionEvaluator>& Evaluator : AIEvaluators)
		{
			AIRegistry.RegisterSkill(SkillName, Evaluator);
		}
	}
};

namespace SGSStandardSkills
{
	SGS_API void Register(FSGSRuleRegistry& RuleRegistry, FSGSAIEvaluatorRegistry& AIRegistry);
}
