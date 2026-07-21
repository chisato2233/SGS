#include "Client/UI/Features/Table/Assets/SGSTableAssetCatalog.h"

#include "Engine/Texture2D.h"
#include "Shared/Core/SGSGameplayTags.h"
#include "Slate/DeferredCleanupSlateBrush.h"

namespace
{
const FName FallbackGeneralId(TEXT("anjiang"));

TSharedPtr<FDeferredCleanupSlateBrush> LoadTextureBrush(const FString& AssetPath, FVector2D ImageSize)
{
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath))
	{
		return FDeferredCleanupSlateBrush::CreateBrush(Texture, ImageSize);
	}
	return nullptr;
}

TSharedPtr<FDeferredCleanupSlateBrush> LoadGeneralPortraitBrush(FName GeneralId)
{
	const FString GeneralName = GeneralId.ToString();
	const FString NoNameAssetPath = FString::Printf(
		TEXT("/Game/ImportedAssets/NoName/image/character/%s.%s"),
		*GeneralName,
		*GeneralName);
	if (TSharedPtr<FDeferredCleanupSlateBrush> Brush = LoadTextureBrush(
		NoNameAssetPath,
		FVector2D(200.0f, 290.0f)))
	{
		return Brush;
	}

	const FString QSanguoshaAssetPath = FString::Printf(
		TEXT("/Game/ImportedAssets/QSanguosha/image/generals/card/%s.%s"),
		*GeneralName,
		*GeneralName);
	return LoadTextureBrush(QSanguoshaAssetPath, FVector2D(200.0f, 290.0f));
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

const FSlateBrush* FSGSTableAssetCatalog::GetSeatFrameBrush(const FGameplayTag& Faction)
{
	const TCHAR* FactionStem = nullptr;
	if (Faction.MatchesTagExact(SGSGameplayTags::Faction_Wei.GetTag()))
	{
		FactionStem = TEXT("wei");
	}
	else if (Faction.MatchesTagExact(SGSGameplayTags::Faction_Shu.GetTag()))
	{
		FactionStem = TEXT("shu");
	}
	else if (Faction.MatchesTagExact(SGSGameplayTags::Faction_Wu.GetTag()))
	{
		FactionStem = TEXT("wu");
	}
	else if (Faction.MatchesTagExact(SGSGameplayTags::Faction_Qun.GetTag()))
	{
		FactionStem = TEXT("qun");
	}
	if (FactionStem == nullptr)
	{
		return nullptr;
	}

	TSharedPtr<FDeferredCleanupSlateBrush>& Brush = SeatFrameBrushes.FindOrAdd(Faction);
	if (!Brush.IsValid())
	{
		Brush = LoadTextureBrush(
			FString::Printf(
				TEXT("/Game/ImportedAssets/NoName/DecadeUI/Decoration/name2_%s.name2_%s"),
				FactionStem,
				FactionStem),
			FVector2D(441.0f, 591.0f));
	}
	return FDeferredCleanupSlateBrush::TrySlateBrush(Brush);
}

const FSlateBrush* FSGSTableAssetCatalog::GetSeatHealthBrush(ESGSSeatHealthVisual Visual)
{
	TSharedPtr<FDeferredCleanupSlateBrush>& Brush = SeatHealthBrushes.FindOrAdd(Visual);
	if (!Brush.IsValid())
	{
		const TCHAR* FileStem = TEXT("glass4");
		switch (Visual)
		{
		case ESGSSeatHealthVisual::High:
			FileStem = TEXT("glass1");
			break;
		case ESGSSeatHealthVisual::Mid:
			FileStem = TEXT("glass2");
			break;
		case ESGSSeatHealthVisual::Low:
			FileStem = TEXT("glass3");
			break;
		case ESGSSeatHealthVisual::Lost:
			break;
		}
		Brush = LoadTextureBrush(
			FString::Printf(
				TEXT("/Game/ImportedAssets/NoName/theme/style/hp/image/%s.%s"),
				FileStem,
				FileStem),
			FVector2D(44.0f, 44.0f));
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

const FSlateBrush* FSGSTableAssetCatalog::GetCardBackBrush()
{
	if (!CardBackBrush.IsValid())
	{
		CardBackBrush = LoadTextureBrush(
			TEXT("/Game/ImportedAssets/QSanguosha/image/system/card-back.card-back"),
			FVector2D(94.0f, 132.0f));
	}
	return FDeferredCleanupSlateBrush::TrySlateBrush(CardBackBrush);
}
