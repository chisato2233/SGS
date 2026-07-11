#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Core/Lifetime/SGSUILifetime.h"

// 同一游戏线程调用栈内合并状态通知。嵌套 batch 合并到最外层；不实现跨线程队列。
class SGS_API FSGSUIStateBatch
{
public:
	FSGSUIStateBatch();
	~FSGSUIStateBatch();

	FSGSUIStateBatch(const FSGSUIStateBatch&) = delete;
	FSGSUIStateBatch& operator=(const FSGSUIStateBatch&) = delete;

	static bool IsBatching();
	static void Enqueue(TFunction<void()> Notification);
};

template<typename ValueType>
class TSGSUIObservable
{
public:
	using FEquality = TFunction<bool(const ValueType&, const ValueType&)>;
	using FCallback = TFunction<void(const ValueType&)>;

	explicit TSGSUIObservable(ValueType InitialValue)
		: TSGSUIObservable(
			MoveTemp(InitialValue),
			[](const ValueType& A, const ValueType& B) { return A == B; })
	{
	}

	TSGSUIObservable(ValueType InitialValue, FEquality InEquality)
		: State(MakeShared<FState>(MoveTemp(InitialValue), MoveTemp(InEquality)))
	{
	}

	const ValueType& Get() const
	{
		return State->Value;
	}

	bool Set(ValueType NewValue)
	{
		if (!ensureMsgf(IsInGameThread(), TEXT("UI observable values must be changed on the game thread.")))
		{
			return false;
		}
		if (State->Equality(State->Value, NewValue))
		{
			return false;
		}

		State->Value = MoveTemp(NewValue);
		State->RequestBroadcast();
		return true;
	}

	void Subscribe(
		FSGSUILifetimeScope& Scope,
		FCallback Callback,
		bool bNotifyImmediately = true) const
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
		if (bNotifyImmediately)
		{
			if (FCallback* StoredCallback = State->Subscribers.Find(SubscriptionId))
			{
				FCallback SafeCallback = *StoredCallback;
				SafeCallback(State->Value);
			}
		}
	}

	int32 NumSubscribers() const
	{
		return State->Subscribers.Num();
	}

private:
	struct FState : public TSharedFromThis<FState>
	{
		FState(ValueType InValue, FEquality InEquality)
			: Value(MoveTemp(InValue))
			, Equality(MoveTemp(InEquality))
		{
		}

		void RequestBroadcast()
		{
			if (!FSGSUIStateBatch::IsBatching())
			{
				Broadcast();
				return;
			}
			if (bBroadcastQueued)
			{
				return;
			}

			bBroadcastQueued = true;
			TWeakPtr<FState> WeakThis = this->AsShared();
			FSGSUIStateBatch::Enqueue(
				[WeakThis]()
				{
					if (const TSharedPtr<FState> Pinned = WeakThis.Pin())
					{
						Pinned->bBroadcastQueued = false;
						Pinned->Broadcast();
					}
				});
		}

		void Broadcast()
		{
			TArray<uint64> SubscriptionIds;
			Subscribers.GenerateKeyArray(SubscriptionIds);
			for (uint64 SubscriptionId : SubscriptionIds)
			{
				if (FCallback* Callback = Subscribers.Find(SubscriptionId))
				{
					FCallback SafeCallback = *Callback;
					SafeCallback(Value);
				}
			}
		}

		ValueType Value;
		FEquality Equality;
		TMap<uint64, FCallback> Subscribers;
		uint64 NextSubscriptionId = 1;
		bool bBroadcastQueued = false;
	};

	TSharedRef<FState> State;
};

template<typename SourceType, typename SelectedType>
class TSGSUISelector
{
public:
	using FSelect = TFunction<SelectedType(const SourceType&)>;
	using FEquality = typename TSGSUIObservable<SelectedType>::FEquality;

	TSGSUISelector(
		const TSGSUIObservable<SourceType>& Source,
		FSelect InSelect,
		FEquality InEquality)
		: Impl(MakeShared<FImpl>(InSelect(Source.Get()), MoveTemp(InSelect), MoveTemp(InEquality)))
	{
		TWeakPtr<FImpl> WeakImpl = Impl;
		Source.Subscribe(
			Impl->Lifetime,
			[WeakImpl](const SourceType& SourceValue)
			{
				if (const TSharedPtr<FImpl> Pinned = WeakImpl.Pin())
				{
					Pinned->Selected.Set(Pinned->Select(SourceValue));
				}
			},
			false);
	}

	const TSGSUIObservable<SelectedType>& GetObservable() const
	{
		return Impl->Selected;
	}

	const SelectedType& Get() const
	{
		return Impl->Selected.Get();
	}

private:
	struct FImpl
	{
		FImpl(SelectedType InitialValue, FSelect InSelect, FEquality InEquality)
			: Lifetime(TEXT("UISelector"))
			, Selected(MoveTemp(InitialValue), MoveTemp(InEquality))
			, Select(MoveTemp(InSelect))
		{
		}

		FSGSUILifetimeScope Lifetime;
		TSGSUIObservable<SelectedType> Selected;
		FSelect Select;
	};

	TSharedRef<FImpl> Impl;
};
