#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SGSCardPile.generated.h"

class USGSCard;
struct FRandomStream;

// 有序牌区：牌堆、弃牌堆、手牌、判定区均用它。
// 约定：索引 0 为「顶」（摸牌从顶取）。
UCLASS()
class SGS_API USGSCardPile : public UObject
{
	GENERATED_BODY()

public:
	int32 Num() const { return Cards.Num(); }
	bool IsEmpty() const { return Cards.Num() == 0; }
	bool Contains(const USGSCard* Card) const;

	void AddToTop(USGSCard* Card);
	void AddToBottom(USGSCard* Card);
	void AddManyToBottom(const TArray<USGSCard*>& InCards);

	// 从顶部取最多 Count 张（不足取全部），从本区移除并返回。
	TArray<USGSCard*> TakeFromTop(int32 Count);

	// 移除指定牌；返回是否移除成功。
	bool RemoveCard(USGSCard* Card);

	void Shuffle(const FRandomStream& Stream);

	const TArray<TObjectPtr<USGSCard>>& GetCards() const { return Cards; }

private:
	UPROPERTY()
	TArray<TObjectPtr<USGSCard>> Cards;
};
