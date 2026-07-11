#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Core/Lifetime/SGSUILifetime.h"
#include "Widgets/SCompoundWidget.h"

class FSGSUIContext;
struct FSGSUIToastRequest;

// 独立于 Table 的真实第二 feature：显示当前 LocalPlayer Context 上的 Toast。
class SGS_API SSGSUIToastLayerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSUIToastLayerWidget) {}
		SLATE_ARGUMENT(TSharedPtr<FSGSUIContext>, UIContext)
	SLATE_END_ARGS()

	SSGSUIToastLayerWidget();
	virtual ~SSGSUIToastLayerWidget() override;
	void Construct(const FArguments& InArgs);

private:
	void HandleToast(const FSGSUIToastRequest& Request);
	EActiveTimerReturnType HandleExpiryTimer(double CurrentTime, float DeltaTime);

	FSGSUILifetimeScope Lifetime;
	double ExpiryTime = 0.0;
};
