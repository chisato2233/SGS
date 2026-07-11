#include "Client/UI/Core/Lifetime/SGSUILifetime.h"

FSGSUISubscription::FSGSUISubscription(TFunction<void()> InUnsubscribe)
	: Unsubscribe(MoveTemp(InUnsubscribe))
{
}

FSGSUISubscription::~FSGSUISubscription()
{
	Reset();
}

FSGSUISubscription::FSGSUISubscription(FSGSUISubscription&& Other) noexcept
	: Unsubscribe(MoveTemp(Other.Unsubscribe))
{
	Other.Unsubscribe = nullptr;
}

FSGSUISubscription& FSGSUISubscription::operator=(FSGSUISubscription&& Other) noexcept
{
	if (this != &Other)
	{
		Reset();
		Unsubscribe = MoveTemp(Other.Unsubscribe);
		Other.Unsubscribe = nullptr;
	}
	return *this;
}

void FSGSUISubscription::Reset()
{
	if (Unsubscribe)
	{
		TFunction<void()> Pending = MoveTemp(Unsubscribe);
		Unsubscribe = nullptr;
		Pending();
	}
}

FSGSUILifetimeScope::FSGSUILifetimeScope(FName InDebugName)
	: DebugName(InDebugName)
{
}

FSGSUILifetimeScope::~FSGSUILifetimeScope()
{
	Reset();
}

void FSGSUILifetimeScope::Add(FSGSUISubscription&& Subscription)
{
	if (Subscription.IsActive())
	{
		Subscriptions.Emplace(MoveTemp(Subscription));
	}
}

void FSGSUILifetimeScope::Reset()
{
	Subscriptions.Empty();
}
