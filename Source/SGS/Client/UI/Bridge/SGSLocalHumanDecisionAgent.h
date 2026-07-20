#pragma once

#include "CoreMinimal.h"
#include "Shared/Decisions/SGSDecisionAgent.h"
#include "Shared/Decisions/SGSDecisionTypes.h"
#include "SGSLocalHumanDecisionAgent.generated.h"

DECLARE_MULTICAST_DELEGATE(FSGSOnLocalDecisionPromptChanged);

enum class ESGSLocalDecisionPromptType : uint8
{
	None,
	Play,
	Response
};

// 本地人类玩家决策通道。
//
// GameDriver 仍然只认识 ISGSDecisionAgent；UI 通过本对象把选择封装成 Command，
// 后续网络化时可把 Submit* 的调用点替换为 RPC，而不改规则层。
UCLASS()
class SGS_API USGSLocalHumanDecisionAgent : public UObject, public ISGSDecisionAgent
{
	GENERATED_BODY()

public:
	virtual void RequestPlayPhaseAction(
		const FSGSPlayPhaseRequest& Request,
		FSGSPlayPhaseDecisionDelegate OnDecided) override;

	virtual void RequestResponseAction(
		const FSGSResponseRequest& Request,
		FSGSResponseDecisionDelegate OnDecided) override;

	bool HasPendingPlayRequest() const { return PromptType == ESGSLocalDecisionPromptType::Play; }
	bool HasPendingResponseRequest() const { return PromptType == ESGSLocalDecisionPromptType::Response; }
	bool IsWaitingForLocalDecision() const { return PromptType != ESGSLocalDecisionPromptType::None; }

	const FSGSPlayPhaseRequest* GetPendingPlayRequest() const;
	const FSGSResponseRequest* GetPendingResponseRequest() const;

	bool SubmitUseCard(int32 CardId, int32 TargetSeatIndex);
	bool SubmitSkill(FName SkillName, TArray<int32> SelectedCardIds, int32 TargetSeatIndex = INDEX_NONE);
	bool SubmitResponseCard(
		int32 CardId,
		int32 TargetSeatIndex = INDEX_NONE,
		FName SkillName = NAME_None);
	bool SubmitPass();

	FSGSOnLocalDecisionPromptChanged& OnPromptChanged() { return PromptChangedDelegate; }

private:
	void ClearPrompt(bool bNotify);
	bool SubmitPassInternal(FName SourceName);

	ESGSLocalDecisionPromptType PromptType = ESGSLocalDecisionPromptType::None;
	FSGSPlayPhaseRequest PendingPlayRequest;
	FSGSResponseRequest PendingResponseRequest;
	FSGSPlayPhaseDecisionDelegate PendingPlayDecisionDelegate;
	FSGSResponseDecisionDelegate PendingResponseDecisionDelegate;
	FSGSOnLocalDecisionPromptChanged PromptChangedDelegate;
};
