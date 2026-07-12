#pragma once

#include "CoreMinimal.h"
#include "Client/UI/Core/Lifetime/SGSUILifetime.h"
#include "Widgets/SCompoundWidget.h"

class FSGSTableAssetCatalog;
class FSGSTableFeatureController;
class SSGSTableShellWidget;
struct FSGSTableHandPresentationState;
struct FSGSTableUIInteractionState;
enum class ESGSTableViewChange : uint8;

// Table Container：只绑定 Feature Controller 状态、响应 viewport 尺寸并生成
// Shell props。业务协调、合法选择和外部提交全部属于 Controller。
class SGS_API SSGSTableHudWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSGSTableHudWidget) {}
		SLATE_ARGUMENT(TSharedPtr<FSGSTableFeatureController>, Controller)
	SLATE_END_ARGS()

	SSGSTableHudWidget();
	virtual ~SSGSTableHudWidget() override;
	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

private:
	FVector2D GetLayoutViewSize() const;
	TSharedRef<SWidget> BuildContent();
	void UpdateShell(ESGSTableViewChange Change);
	void HandlePublicRevision(int32 Revision);
	void HandlePrivateRevision(int32 Revision);
	void HandleInteraction(const FSGSTableUIInteractionState& Interaction);
	void HandleHandPresentation(const FSGSTableHandPresentationState& Presentation);

	FReply OnCardClicked(int32 CardId);
	bool OnHandReordered(const TArray<int32>& OrderedCardIds);
	FReply OnSeatClicked(int32 SeatIndex);
	FReply OnSkillClicked(FName SkillName);
	FReply OnConfirmClicked();
	FReply OnPassClicked();

	TSharedPtr<FSGSTableFeatureController> Controller;
	TSharedPtr<FSGSTableAssetCatalog> AssetCatalog;
	TSharedPtr<SSGSTableShellWidget> ShellWidget;
	FSGSUILifetimeScope Lifetime;
	FVector2D CurrentViewSize = FVector2D::ZeroVector;
	FVector2D ArrangedViewSize = FVector2D::ZeroVector;
};
