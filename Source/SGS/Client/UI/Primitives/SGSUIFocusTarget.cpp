#include "Client/UI/Primitives/SGSUIFocusTarget.h"

#include "Client/UI/Core/Context/SGSUIContext.h"
#include "Framework/Application/SlateApplication.h"

SSGSUIFocusTarget::SSGSUIFocusTarget()
	: Lifetime(TEXT("UIFocusTarget"))
{
}

SSGSUIFocusTarget::~SSGSUIFocusTarget()
{
	Lifetime.Reset();
}

void SSGSUIFocusTarget::Construct(const FArguments& InArgs)
{
	TargetId = InArgs._TargetId;
	FocusWidget = InArgs._Content.Widget;
	ChildSlot
	[
		FocusWidget.ToSharedRef()
	];

	if (InArgs._UIContext.IsValid())
	{
		TWeakPtr<SSGSUIFocusTarget> WeakOwner = SharedThis(this);
		InArgs._UIContext->Signals().Focus().SubscribeWeak<SSGSUIFocusTarget>(
			Lifetime,
			WeakOwner,
			[](SSGSUIFocusTarget& Owner, const FSGSUIFocusRequest& Request)
			{
				Owner.HandleFocusRequest(Request);
			});
	}
}

void SSGSUIFocusTarget::HandleFocusRequest(const FSGSUIFocusRequest& Request)
{
	if (Request.TargetId == TargetId && FocusWidget.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(FocusWidget, EFocusCause::SetDirectly);
	}
}
