#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class FDeferredCleanupSlateBrush;
struct FSlateBrush;

enum class ESGSSeatHealthVisual : uint8
{
	High,
	Mid,
	Low,
	Lost
};

// Table feature 的懒加载展示资源。资源路径和 fallback 规则不泄漏给组件或 HUD。
class SGS_API FSGSTableAssetCatalog
{
public:
	static FVector2D BackgroundImageSize();

	const FSlateBrush* GetBackgroundBrush();
	const FSlateBrush* GetGeneralPortraitBrush(FName GeneralId);
	const FSlateBrush* GetSeatFrameBrush(const FGameplayTag& Faction);
	const FSlateBrush* GetSeatHealthBrush(ESGSSeatHealthVisual Visual);
	const FSlateBrush* GetCardFaceBrush(FName CardName);
	const FSlateBrush* GetCardBackBrush();

private:
	TSharedPtr<FDeferredCleanupSlateBrush> BackgroundBrush;
	TMap<FName, TSharedPtr<FDeferredCleanupSlateBrush>> GeneralPortraitBrushes;
	TMap<FGameplayTag, TSharedPtr<FDeferredCleanupSlateBrush>> SeatFrameBrushes;
	TMap<ESGSSeatHealthVisual, TSharedPtr<FDeferredCleanupSlateBrush>> SeatHealthBrushes;
	TMap<FName, TSharedPtr<FDeferredCleanupSlateBrush>> CardFaceBrushes;
	TSharedPtr<FDeferredCleanupSlateBrush> CardBackBrush;
};
