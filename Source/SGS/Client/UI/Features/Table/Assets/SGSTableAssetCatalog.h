#pragma once

#include "CoreMinimal.h"

class FDeferredCleanupSlateBrush;
struct FSlateBrush;

// Table feature 的懒加载展示资源。资源路径和 fallback 规则不泄漏给组件或 HUD。
class SGS_API FSGSTableAssetCatalog
{
public:
	static FVector2D BackgroundImageSize();

	const FSlateBrush* GetBackgroundBrush();
	const FSlateBrush* GetGeneralPortraitBrush(FName GeneralId);
	const FSlateBrush* GetCardFaceBrush(FName CardName);

private:
	TSharedPtr<FDeferredCleanupSlateBrush> BackgroundBrush;
	TMap<FName, TSharedPtr<FDeferredCleanupSlateBrush>> GeneralPortraitBrushes;
	TMap<FName, TSharedPtr<FDeferredCleanupSlateBrush>> CardFaceBrushes;
};
