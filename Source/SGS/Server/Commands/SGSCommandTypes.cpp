#include "Server/Commands/SGSCommandTypes.h"

#include "Server/Commands/Types/SGSPassCommandType.h"
#include "Server/Commands/Types/SGSRespondCardCommandType.h"
#include "Server/Commands/Types/SGSUseCardCommandType.h"
#include "Server/Commands/Types/SGSActivateSkillCommandType.h"
#include "Server/Commands/Types/SGSChooseCardsCommandType.h"
#include "Server/Commands/Types/SGSChooseOptionCommandType.h"
#include "Server/Commands/SGSCommandRouter.h"

namespace
{
const TArray<TSharedRef<ISGSCommandType>>& GetDefaultCommandTypes()
{
	static const TArray<TSharedRef<ISGSCommandType>> DefaultTypes = {
		MakeShared<FSGSPassCommandType>(),
		MakeShared<FSGSUseCardCommandType>(),
		MakeShared<FSGSRespondCardCommandType>(),
		MakeShared<FSGSActivateSkillCommandType>(),
		MakeShared<FSGSChooseCardsCommandType>(),
		MakeShared<FSGSChooseOptionCommandType>()
	};
	return DefaultTypes;
}
}

void SGSCommandTypes::RegisterDefaultCommandTypes(FSGSCommandRouter& Router)
{
	for (const TSharedRef<ISGSCommandType>& CommandType : GetDefaultCommandTypes())
	{
		Router.RegisterCommandType(CommandType);
	}
}
