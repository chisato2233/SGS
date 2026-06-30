#pragma once

// SGS ActiveEffect 句柄与 GAS 运行期产物之间的所有权桥。
// 记录持续效果失效时必须清理的 GameplayEffect、授予标签或属性绑定。
// Adapter 只记录所有权；SGS 时序和规则决策仍留在 ActiveEffectTimeline。

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Shared/Core/SGSError.h"
#include "Shared/Core/SGSIds.h"

struct SGS_API FSGSGASActiveEffectBinding
{
	FSGSStableHandle ActiveEffectHandle;
	FActiveGameplayEffectHandle GameplayEffectHandle;
	TArray<FGameplayTag> GrantedTags;
	TArray<FName> AttributeNames;
	FString Context;

	bool CheckInvariants() const
	{
		bool bOk = ActiveEffectHandle.CheckInvariants();
		bOk &= ensureMsgf(ActiveEffectHandle.IsValid(), TEXT("GAS binding requires a valid SGS ActiveEffect handle."));
		bOk &= ensureMsgf(GameplayEffectHandle.IsValid() || GrantedTags.Num() > 0 || AttributeNames.Num() > 0,
			TEXT("GAS binding must own a GameplayEffect, GameplayTag, or Attribute modification."));
		return bOk;
	}
};

class SGS_API FSGSGASActiveEffectAdapter
{
public:
	void Reset();

	FSGSStatus AddBinding(FSGSGASActiveEffectBinding Binding);
	int32 RemoveBindingsForEffect(FSGSStableHandle ActiveEffectHandle);
	int32 RemoveBindingsForGameplayEffect(FActiveGameplayEffectHandle GameplayEffectHandle);

	TArray<FSGSGASActiveEffectBinding> QueryByEffect(FSGSStableHandle ActiveEffectHandle) const;
	const TArray<FSGSGASActiveEffectBinding>& GetBindings() const { return Bindings; }
	bool CheckInvariants() const;

private:
	TArray<FSGSGASActiveEffectBinding> Bindings;
};
