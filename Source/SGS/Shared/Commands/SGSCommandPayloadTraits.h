#pragma once

// Protocol-level mapping from typed payload to command tag.
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
};

template <>
struct SGS_API TSGSCommandPayloadTraits<FSGSUseCardCommandPayload>
{
	static FGameplayTag GetType()
	{
		return SGSGameplayTags::PlayAction_UseCard.GetTag();
	}
};

template <>
struct SGS_API TSGSCommandPayloadTraits<FSGSRespondCardCommandPayload>
{
	static FGameplayTag GetType()
	{
		return SGSGameplayTags::PlayAction_RespondCard.GetTag();
	}
};

template <>
struct SGS_API TSGSCommandPayloadTraits<FSGSActivateSkillCommandPayload>
{
	static FGameplayTag GetType()
	{
		return SGSGameplayTags::PlayAction_ActivateSkill.GetTag();
	}
};

template <>
struct SGS_API TSGSCommandPayloadTraits<FSGSChooseCardsCommandPayload>
{
	static FGameplayTag GetType()
	{
		return SGSGameplayTags::PlayAction_ChooseCards.GetTag();
	}
};

template <>
struct SGS_API TSGSCommandPayloadTraits<FSGSChooseOptionCommandPayload>
{
	static FGameplayTag GetType()
	{
		return SGSGameplayTags::PlayAction_ChooseOption.GetTag();
	}
};
