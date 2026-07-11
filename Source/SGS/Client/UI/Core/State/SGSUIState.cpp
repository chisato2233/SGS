#include "Client/UI/Core/State/SGSUIState.h"

namespace
{
struct FBatchState
{
	int32 Depth = 0;
	bool bFlushing = false;
	TArray<TFunction<void()>> Notifications;
};

thread_local FBatchState GBatchState;

void FlushNotifications()
{
	GBatchState.bFlushing = true;
	while (!GBatchState.Notifications.IsEmpty())
	{
		TArray<TFunction<void()>> Pending = MoveTemp(GBatchState.Notifications);
		GBatchState.Notifications.Reset();
		for (TFunction<void()>& Notification : Pending)
		{
			Notification();
		}
	}
	GBatchState.bFlushing = false;
}
}

FSGSUIStateBatch::FSGSUIStateBatch()
{
	check(IsInGameThread());
	++GBatchState.Depth;
}

FSGSUIStateBatch::~FSGSUIStateBatch()
{
	check(IsInGameThread());
	check(GBatchState.Depth > 0);
	--GBatchState.Depth;
	if (GBatchState.Depth == 0)
	{
		FlushNotifications();
	}
}

bool FSGSUIStateBatch::IsBatching()
{
	return GBatchState.Depth > 0 || GBatchState.bFlushing;
}

void FSGSUIStateBatch::Enqueue(TFunction<void()> Notification)
{
	check(IsInGameThread());
	if (IsBatching())
	{
		GBatchState.Notifications.Add(MoveTemp(Notification));
	}
	else
	{
		Notification();
	}
}
