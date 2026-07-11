#include "Client/UI/Core/Context/SGSUIContext.h"

FSGSUISignalService::FSGSUISignalService()
	: ToastSignal(TEXT("UI.Toast"))
	, FocusSignal(TEXT("UI.Focus"))
{
}

FSGSUIContext::FSGSUIContext()
	: SignalService(MakeShared<FSGSUISignalService>())
{
}
