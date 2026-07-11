#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Core/Lifetime/SGSUILifetime.h"
#include "Widgets/SCompoundWidget.h"

class FSGSUIContext;

// 可复用的跨树焦点目标。请求来自 typed focus signal，内容仍由调用者组合。
class SGS_API SSGSUIFocusTarget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSUIFocusTarget) {}
		SLATE_ARGUMENT(TSharedPtr<FSGSUIContext>, UIContext)
		SLATE_ARGUMENT(FName, TargetId)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	SSGSUIFocusTarget();
	virtual ~SSGSUIFocusTarget() override;
	void Construct(const FArguments& InArgs);

private:
	void HandleFocusRequest(const struct FSGSUIFocusRequest& Request);

	FName TargetId;
	TSharedPtr<SWidget> FocusWidget;
	FSGSUILifetimeScope Lifetime;
};
