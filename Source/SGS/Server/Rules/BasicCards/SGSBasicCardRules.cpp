#include "Server/Rules/BasicCards/SGSBasicCardRules.h"

#include "Server/Rules/Actions/BasicCards/SGSPassRule.h"
#include "Server/Rules/Actions/BasicCards/SGSPeachRule.h"
#include "Server/Rules/Actions/BasicCards/SGSSlashRule.h"
#include "Server/Rules/Core/SGSRuleRegistry.h"
#include "Server/Rules/Responses/BasicCards/SGSDodgeRules.h"
#include "Server/Rules/Responses/BasicCards/SGSDyingPeachRules.h"

void SGSBasicCardRules::Register(FSGSRuleRegistry& Registry)
{
	Registry.RegisterRule(MakeShared<FSGSDodgeResponseRule>());
	Registry.RegisterRule(MakeShared<FSGSDodgePassRule>());
	Registry.RegisterRule(MakeShared<FSGSDyingPeachRule>());
	Registry.RegisterRule(MakeShared<FSGSDyingPeachPassRule>());
	Registry.RegisterRule(MakeShared<FSGSSlashRule>());
	Registry.RegisterRule(MakeShared<FSGSPeachRule>());
	Registry.RegisterRule(MakeShared<FSGSPassRule>());
}
