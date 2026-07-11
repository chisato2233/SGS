#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Core/Lifetime/SGSUILifetime.h"

template<typename PayloadType>
class TSGSUITypedSignal
{
public:
	using FCallback = TFunction<void(const PayloadType&)>;

	explicit TSGSUITypedSignal(FName InDebugName = NAME_None)
		: State(MakeShared<FState>(InDebugName))
	{
	}

	void Subscribe(FSGSUILifetimeScope& Scope, FCallback Callback)
	{
		check(IsInGameThread());
		const uint64 SubscriptionId = State->NextSubscriptionId++;
		State->Subscribers.Add(SubscriptionId, MoveTemp(Callback));
		TWeakPtr<FState> WeakState = State;
		FSGSUISubscription Subscription(
			[WeakState, SubscriptionId]()
			{
				if (const TSharedPtr<FState> Pinned = WeakState.Pin())
				{
					Pinned->Subscribers.Remove(SubscriptionId);
				}
			});
		Scope.Add(MoveTemp(Subscription));
	}

	template<typename OwnerType>
	void SubscribeWeak(
		FSGSUILifetimeScope& Scope,
		TWeakPtr<OwnerType> Owner,
		TFunction<void(OwnerType&, const PayloadType&)> Callback)
	{
		Subscribe(
			Scope,
			[Owner, Callback = MoveTemp(Callback)](const PayloadType& Payload)
			{
				if (const TSharedPtr<OwnerType> Pinned = Owner.Pin())
				{
					Callback(*Pinned, Payload);
				}
			});
	}

	bool Publish(const PayloadType& Payload)
	{
		if (!ensureMsgf(IsInGameThread(), TEXT("UI signals must be published on the game thread.")))
		{
			++State->DroppedPublishCount;
			return false;
		}

		TArray<uint64> SubscriptionIds;
		State->Subscribers.GenerateKeyArray(SubscriptionIds);
		for (uint64 SubscriptionId : SubscriptionIds)
		{
			if (FCallback* Callback = State->Subscribers.Find(SubscriptionId))
			{
				FCallback SafeCallback = *Callback;
				SafeCallback(Payload);
			}
		}
		return true;
	}

	FName GetDebugName() const { return State->DebugName; }
	int32 NumSubscribers() const { return State->Subscribers.Num(); }
	uint64 GetDroppedPublishCount() const { return State->DroppedPublishCount; }

private:
	struct FState
	{
		explicit FState(FName InDebugName)
			: DebugName(InDebugName)
		{
		}

		FName DebugName;
		TMap<uint64, FCallback> Subscribers;
		uint64 NextSubscriptionId = 1;
		uint64 DroppedPublishCount = 0;
	};

	TSharedRef<FState> State;
};
