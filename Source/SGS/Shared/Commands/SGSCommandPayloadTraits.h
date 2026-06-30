#pragma once

// Protocol-level mapping from typed payload to command tag and legacy mirrors.
// Server CommandType classes reuse these traits so client/shared construction and
// server registration do not drift apart.

#include "CoreMinimal.h"
#include "Shared/Commands/SGSCommand.h"
#include "Shared/Commands/SGSCommandPayloads.h"

template <typename TPayload>
struct TSGSCommandPayloadTraits;

template <>
struct SGS_API TSGSCommandPayloadTraits<FSGSPassCommandPayload>
{
	static FGameplayTag GetType()
	{
		return SGSGameplayTags::PlayAction_Pass.GetTag();
	}

	static void SyncLegacyMirror(FSGSCommand& Command)
	{
	}
};

template <>
struct SGS_API TSGSCommandPayloadTraits<FSGSUseCardCommandPayload>
{
	static FGameplayTag GetType()
	{
		return SGSGameplayTags::PlayAction_UseCard.GetTag();
	}

	static void SyncLegacyMirror(FSGSCommand& Command)
	{
		const FSGSUseCardCommandPayload* Payload = Command.GetPayload<FSGSUseCardCommandPayload>();
		if (Payload == nullptr)
		{
			return;
		}

		Command.CardIds.Reset();
		Command.CardIds.Add(Payload->CardId);
		Command.TargetSeatIndices = Payload->TargetSeatIndices;
	}
};

template <>
struct SGS_API TSGSCommandPayloadTraits<FSGSRespondCardCommandPayload>
{
	static FGameplayTag GetType()
	{
		return SGSGameplayTags::PlayAction_RespondCard.GetTag();
	}

	static void SyncLegacyMirror(FSGSCommand& Command)
	{
		const FSGSRespondCardCommandPayload* Payload = Command.GetPayload<FSGSRespondCardCommandPayload>();
		if (Payload == nullptr)
		{
			return;
		}

		Command.CardIds.Reset();
		Command.CardIds.Add(Payload->CardId);
		Command.TargetSeatIndices = Payload->TargetSeatIndices;
		Command.Parameters.Reset();
		Command.Parameters.Add(FName(TEXT("WindowName")), Payload->WindowName.ToString());
	}
};
