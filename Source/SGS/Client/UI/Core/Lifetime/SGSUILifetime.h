#pragma once

#include "CoreMinimal.h"

class SGS_API FSGSUISubscription
{
public:
	FSGSUISubscription() = default;
	explicit FSGSUISubscription(TFunction<void()> InUnsubscribe);
	~FSGSUISubscription();

	FSGSUISubscription(FSGSUISubscription&& Other) noexcept;
	FSGSUISubscription& operator=(FSGSUISubscription&& Other) noexcept;
	FSGSUISubscription(const FSGSUISubscription&) = delete;
	FSGSUISubscription& operator=(const FSGSUISubscription&) = delete;

	void Reset();
	bool IsActive() const { return static_cast<bool>(Unsubscribe); }

private:
	TFunction<void()> Unsubscribe;
};

// 任意 UI owner 可组合的生命周期容器；销毁或 Reset 时按 RAII 解除全部订阅。
class SGS_API FSGSUILifetimeScope
{
public:
	explicit FSGSUILifetimeScope(FName InDebugName = NAME_None);
	~FSGSUILifetimeScope();

	FSGSUILifetimeScope(const FSGSUILifetimeScope&) = delete;
	FSGSUILifetimeScope& operator=(const FSGSUILifetimeScope&) = delete;

	void Add(FSGSUISubscription&& Subscription);
	void Reset();
	FName GetDebugName() const { return DebugName; }
	int32 NumSubscriptions() const { return Subscriptions.Num(); }

private:
	FName DebugName;
	TArray<FSGSUISubscription> Subscriptions;
};
