#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Core/Signals/SGSUISignal.h"

enum class ESGSUIToastTone : uint8
{
	Neutral,
	Success,
	Warning,
	Error
};

struct SGS_API FSGSUIToastRequest
{
	FText Message;
	ESGSUIToastTone Tone = ESGSUIToastTone::Neutral;
	float DurationSeconds = 2.5f;
};

struct SGS_API FSGSUIFocusRequest
{
	FName TargetId = NAME_None;
};

class SGS_API FSGSUISignalService
{
public:
	FSGSUISignalService();

	TSGSUITypedSignal<FSGSUIToastRequest>& Toast() { return ToastSignal; }
	TSGSUITypedSignal<FSGSUIFocusRequest>& Focus() { return FocusSignal; }

private:
	TSGSUITypedSignal<FSGSUIToastRequest> ToastSignal;
	TSGSUITypedSignal<FSGSUIFocusRequest> FocusSignal;
};

// 每个 LocalPlayer 独享一个 Context；这里只放业务无关的横切 UI 能力。
class SGS_API FSGSUIContext
{
public:
	FSGSUIContext();

	FSGSUISignalService& Signals() const { return *SignalService; }

private:
	TSharedRef<FSGSUISignalService> SignalService;
};
