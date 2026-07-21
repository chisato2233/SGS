#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Shared/UI/SGSTableViewTypes.h"
#include "SGSPlayerController.generated.h"

class SSGSTableHudWidget;
class SSGSUIToastLayerWidget;
class FSGSTableFeatureController;
class FSGSUIContext;
class USGSLocalHumanDecisionAgent;
class ASGSGameState;
class ULocalPlayer;
class FLifetimeProperty;

UCLASS()
class SGS_API ASGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AttachToMatch(
		USGSLocalHumanDecisionAgent* InDecisionAgent,
		int32 InViewerSeat,
		TFunction<FSGSPlayerPrivateSnapshot()> InPrivateSnapshotProvider,
		TFunction<void()> InServerViewRefreshHandler,
		int32 InInitialMotionSequence = INDEX_NONE);

	void RefreshPrivateSnapshotFromServer();
	FSGSTableViewSnapshot BuildTableViewSnapshot() const;

	bool SubmitUseCard(int32 CardId, int32 TargetSeatIndex);
	bool SubmitUseCard(int32 CardId, TArray<int32> TargetSeatIndices);
	bool SubmitSkill(FName SkillName, TArray<int32> CardIds, int32 TargetSeatIndex);
	bool SubmitResponseCard(int32 CardId, int32 TargetSeatIndex, FName SkillName = NAME_None);
	bool SubmitResponseCards(TArray<int32> CardIds, int32 TargetSeatIndex, FName SkillName = NAME_None);
	bool SubmitPass();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION(Client, Reliable)
	void ClientAttachLocalHud(int32 InViewerSeat, int32 InInitialMotionSequence);

	UFUNCTION(Server, Reliable)
	void ServerSubmitUseCard(int32 CardId, int32 TargetSeatIndex);

	UFUNCTION(Server, Reliable)
	void ServerSubmitUseCardTargets(int32 CardId, const TArray<int32>& TargetSeatIndices);

	UFUNCTION(Server, Reliable)
	void ServerSubmitSkill(FName SkillName, const TArray<int32>& CardIds, int32 TargetSeatIndex);

	UFUNCTION(Server, Reliable)
	void ServerSubmitResponseCards(const TArray<int32>& CardIds, int32 TargetSeatIndex, FName SkillName);

	UFUNCTION(Server, Reliable)
	void ServerSubmitPass();

	UFUNCTION()
	void OnRep_PrivateSnapshot();

	void AttachLocalHud();
	void RemoveTableHud();
	void BindTableSnapshotSource();
	void UnbindTableSnapshotSource();
	void HandlePublicSnapshotChanged();
	void RefreshTableFeature();
	bool SubmitUseCardOnServer(int32 CardId, int32 TargetSeatIndex);
	bool SubmitUseCardOnServer(int32 CardId, TArray<int32> TargetSeatIndices);
	bool SubmitSkillOnServer(FName SkillName, TArray<int32> CardIds, int32 TargetSeatIndex);
	bool SubmitResponseCardsOnServer(TArray<int32> CardIds, int32 TargetSeatIndex, FName SkillName);
	bool SubmitPassOnServer();
	void RefreshAfterServerDecision();

	UPROPERTY(ReplicatedUsing = OnRep_PrivateSnapshot)
	FSGSPlayerPrivateSnapshot PrivateSnapshot;

	UPROPERTY(Replicated)
	int32 ViewerSeat = INDEX_NONE;

	UPROPERTY(Replicated)
	int32 InitialMotionSequence = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<USGSLocalHumanDecisionAgent> DecisionAgent;

	TFunction<FSGSPlayerPrivateSnapshot()> PrivateSnapshotProvider;
	TFunction<void()> ServerViewRefreshHandler;
	TWeakObjectPtr<ULocalPlayer> HudLocalPlayer;
	TWeakObjectPtr<ASGSGameState> ObservedGameState;
	FDelegateHandle PublicSnapshotChangedHandle;
	TSharedPtr<FSGSUIContext> UIContext;
	TSharedPtr<FSGSTableFeatureController> TableFeatureController;
	TSharedPtr<SSGSTableHudWidget> TableHudWidget;
	TSharedPtr<SSGSUIToastLayerWidget> ToastLayerWidget;
};
