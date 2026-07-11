#include "Client/UI/Features/Table/Assets/SGSTableAssetCatalog.h"

#include "Engine/Texture2D.h"
#include "Slate/DeferredCleanupSlateBrush.h"

namespace
{
const TCHAR* GeneralPortraitIds[] = {
	TEXT("caocao"),
	TEXT("liubei"),
	TEXT("sunquan"),
	TEXT("guanyu"),
	TEXT("zhangfei"),
	TEXT("zhaoyun"),
	TEXT("simayi"),
	TEXT("diaochan"),
};
}

FVector2D FSGSTableAssetCatalog::BackgroundImageSize()
{
	return FVector2D(1334.0f, 750.0f);
}

const FSlateBrush* FSGSTableAssetCatalog::GetBackgroundBrush()
{
	if (!BackgroundBrush.IsValid())
	{
		if (UTexture2D* Texture = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/Game/ImportedAssets/NoName/Background/ol_bg.ol_bg")))
		{
			BackgroundBrush = FDeferredCleanupSlateBrush::CreateBrush(Texture, BackgroundImageSize());
		}
	}
	return FDeferredCleanupSlateBrush::TrySlateBrush(BackgroundBrush);
}

const FSlateBrush* FSGSTableAssetCatalog::GetSeatPortraitBrush(int32 SeatIndex)
{
	if (SeatIndex == INDEX_NONE)
	{
		return nullptr;
	}

	const int32 PortraitIndex = FMath::Abs(SeatIndex) % UE_ARRAY_COUNT(GeneralPortraitIds);
	TSharedPtr<FDeferredCleanupSlateBrush>& Brush = GeneralPortraitBrushes.FindOrAdd(PortraitIndex);
	if (!Brush.IsValid())
	{
		const FString AssetPath = FString::Printf(
			TEXT("/Game/ImportedAssets/QSanguosha/image/generals/card/%s.%s"),
			GeneralPortraitIds[PortraitIndex],
			GeneralPortraitIds[PortraitIndex]);
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath))
		{
			Brush = FDeferredCleanupSlateBrush::CreateBrush(Texture, FVector2D(200.0f, 290.0f));
		}
	}
	return FDeferredCleanupSlateBrush::TrySlateBrush(Brush);
}

const FSlateBrush* FSGSTableAssetCatalog::GetCardFaceBrush(FName CardName)
{
	FString FileStem;
	if (CardName == FName(TEXT("Slash")))
	{
		FileStem = TEXT("slash");
	}
	else if (CardName == FName(TEXT("Dodge")))
	{
		FileStem = TEXT("jink");
	}
	else if (CardName == FName(TEXT("Peach")))
	{
		FileStem = TEXT("peach");
	}
	else
	{
		return nullptr;
	}

	TSharedPtr<FDeferredCleanupSlateBrush>& Brush = CardFaceBrushes.FindOrAdd(CardName);
	if (!Brush.IsValid())
	{
		const FString AssetPath = FString::Printf(
			TEXT("/Game/ImportedAssets/QSanguosha/image/card/%s.%s"),
			*FileStem,
			*FileStem);
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath))
		{
			Brush = FDeferredCleanupSlateBrush::CreateBrush(Texture, FVector2D(94.0f, 132.0f));
		}
	}
	return FDeferredCleanupSlateBrush::TrySlateBrush(Brush);
}
