#include "Client/UI/Features/Table/Assets/SGSTableAssetCatalog.h"

#include "Engine/Texture2D.h"
#include "Slate/DeferredCleanupSlateBrush.h"

namespace
{
const FName FallbackGeneralId(TEXT("anjiang"));

TSharedPtr<FDeferredCleanupSlateBrush> LoadGeneralPortraitBrush(FName GeneralId)
{
	const FString GeneralName = GeneralId.ToString();
	const FString AssetPath = FString::Printf(
		TEXT("/Game/ImportedAssets/QSanguosha/image/generals/card/%s.%s"),
		*GeneralName,
		*GeneralName);
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath))
	{
		return FDeferredCleanupSlateBrush::CreateBrush(Texture, FVector2D(200.0f, 290.0f));
	}
	return nullptr;
}
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

const FSlateBrush* FSGSTableAssetCatalog::GetGeneralPortraitBrush(FName GeneralId)
{
	const FName RequestedId = GeneralId.IsNone() ? FallbackGeneralId : GeneralId;
	if (const TSharedPtr<FDeferredCleanupSlateBrush>* Existing = GeneralPortraitBrushes.Find(RequestedId))
	{
		return FDeferredCleanupSlateBrush::TrySlateBrush(*Existing);
	}

	TSharedPtr<FDeferredCleanupSlateBrush> Brush = LoadGeneralPortraitBrush(RequestedId);
	if (!Brush.IsValid() && RequestedId != FallbackGeneralId)
	{
		if (const TSharedPtr<FDeferredCleanupSlateBrush>* ExistingFallback =
			GeneralPortraitBrushes.Find(FallbackGeneralId))
		{
			Brush = *ExistingFallback;
		}
		else
		{
			Brush = LoadGeneralPortraitBrush(FallbackGeneralId);
			GeneralPortraitBrushes.Add(FallbackGeneralId, Brush);
		}
	}
	GeneralPortraitBrushes.Add(RequestedId, Brush);
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
	else if (CardName == FName(TEXT("Analeptic")))
	{
		FileStem = TEXT("analeptic");
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
