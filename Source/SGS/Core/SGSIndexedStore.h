#pragma once

// 规则实体的运行期多索引 Store，服务卡牌、座位、持续效果等对象。
// Context 先注册自己拥有的索引，再只通过 Add / Remove / Modify 改实体并查询索引。
// StableHandle 是唯一稳定引用；CheckInvariants 负责校验 Store 与索引一致。

#include "CoreMinimal.h"
#include "Core/SGSError.h"
#include "Core/SGSIds.h"
#include "Misc/Optional.h"

template <typename ItemType>
class TSGSIndexedStore
{
public:
	class IIndex
	{
	public:
		virtual ~IIndex() = default;

		virtual FName GetName() const = 0;
		virtual void Rebuild(const TSGSIndexedStore<ItemType>& Store) = 0;
		virtual bool CheckInvariants(const TSGSIndexedStore<ItemType>& Store) const = 0;
	};

	template <typename KeyType>
	class TUniqueIndex final : public IIndex
	{
	public:
		TUniqueIndex(FName InName, TFunction<KeyType(const ItemType&)> InKeySelector)
			: Name(InName)
			, KeySelector(MoveTemp(InKeySelector))
		{
		}

		virtual FName GetName() const override { return Name; }

		virtual void Rebuild(const TSGSIndexedStore<ItemType>& Store) override
		{
			HandlesByKey.Reset();
			bHadCollision = false;
			Store.ForEach([this](FSGSStableHandle Handle, const ItemType& Item)
			{
				const KeyType Key = KeySelector(Item);
				if (HandlesByKey.Contains(Key))
				{
					bHadCollision = true;
					ensureMsgf(false, TEXT("Unique index %s key collision."), *Name.ToString());
					return;
				}
				HandlesByKey.Add(Key, Handle);
			});
		}

		FSGSStableHandle Find(const KeyType& Key) const
		{
			if (const FSGSStableHandle* Handle = HandlesByKey.Find(Key))
			{
				return *Handle;
			}
			return FSGSStableHandle();
		}

		bool Contains(const KeyType& Key) const
		{
			return HandlesByKey.Contains(Key);
		}

		virtual bool CheckInvariants(const TSGSIndexedStore<ItemType>& Store) const override
		{
			bool bOk = true;
			int32 Count = 0;
			Store.ForEach([this, &bOk, &Count](FSGSStableHandle Handle, const ItemType& Item)
			{
				++Count;
				const KeyType Key = KeySelector(Item);
				const FSGSStableHandle* IndexedHandle = HandlesByKey.Find(Key);
				bOk &= ensureMsgf(IndexedHandle != nullptr && *IndexedHandle == Handle,
					TEXT("Unique index %s is stale."), *Name.ToString());
			});
			bOk &= ensureMsgf(!bHadCollision, TEXT("Unique index %s has a recorded key collision."), *Name.ToString());
			bOk &= ensureMsgf(Count == HandlesByKey.Num(), TEXT("Unique index %s count mismatch."), *Name.ToString());
			return bOk;
		}

	private:
		FName Name;
		TFunction<KeyType(const ItemType&)> KeySelector;
		TMap<KeyType, FSGSStableHandle> HandlesByKey;
		bool bHadCollision = false;
	};

	template <typename KeyType>
	class TNonUniqueIndex final : public IIndex
	{
	public:
		TNonUniqueIndex(FName InName, TFunction<KeyType(const ItemType&)> InKeySelector)
			: Name(InName)
			, KeySelector(MoveTemp(InKeySelector))
		{
		}

		virtual FName GetName() const override { return Name; }

		virtual void Rebuild(const TSGSIndexedStore<ItemType>& Store) override
		{
			HandlesByKey.Reset();
			Store.ForEach([this](FSGSStableHandle Handle, const ItemType& Item)
			{
				HandlesByKey.FindOrAdd(KeySelector(Item)).Add(Handle);
			});
		}

		const TArray<FSGSStableHandle>* Find(const KeyType& Key) const
		{
			return HandlesByKey.Find(Key);
		}

		TArray<FSGSStableHandle> FindCopy(const KeyType& Key) const
		{
			if (const TArray<FSGSStableHandle>* Handles = HandlesByKey.Find(Key))
			{
				return *Handles;
			}
			return TArray<FSGSStableHandle>();
		}

		virtual bool CheckInvariants(const TSGSIndexedStore<ItemType>& Store) const override
		{
			bool bOk = true;
			int32 Count = 0;
			Store.ForEach([this, &bOk, &Count](FSGSStableHandle Handle, const ItemType& Item)
			{
				++Count;
				const TArray<FSGSStableHandle>* Handles = HandlesByKey.Find(KeySelector(Item));
				bOk &= ensureMsgf(Handles != nullptr && Handles->Contains(Handle),
					TEXT("NonUnique index %s is stale."), *Name.ToString());
			});

			int32 IndexedCount = 0;
			for (const TPair<KeyType, TArray<FSGSStableHandle>>& Pair : HandlesByKey)
			{
				IndexedCount += Pair.Value.Num();
			}
			bOk &= ensureMsgf(Count == IndexedCount, TEXT("NonUnique index %s count mismatch."), *Name.ToString());
			return bOk;
		}

	private:
		FName Name;
		TFunction<KeyType(const ItemType&)> KeySelector;
		TMap<KeyType, TArray<FSGSStableHandle>> HandlesByKey;
	};

	template <typename KeyType>
	class TOrderedIndex final : public IIndex
	{
	public:
		TOrderedIndex(
			FName InName,
			TFunction<KeyType(const ItemType&)> InKeySelector,
			TFunction<int32(const ItemType&)> InOrderSelector)
			: Name(InName)
			, KeySelector(MoveTemp(InKeySelector))
			, OrderSelector(MoveTemp(InOrderSelector))
		{
		}

		virtual FName GetName() const override { return Name; }

		virtual void Rebuild(const TSGSIndexedStore<ItemType>& Store) override
		{
			TMap<KeyType, TArray<FEntry>> EntriesByKey;
			int32 TieBreaker = 0;
			Store.ForEach([this, &EntriesByKey, &TieBreaker](FSGSStableHandle Handle, const ItemType& Item)
			{
				FEntry Entry;
				Entry.Handle = Handle;
				Entry.Order = OrderSelector(Item);
				Entry.TieBreaker = TieBreaker++;
				EntriesByKey.FindOrAdd(KeySelector(Item)).Add(Entry);
			});

			HandlesByKey.Reset();
			for (TPair<KeyType, TArray<FEntry>>& Pair : EntriesByKey)
			{
				Pair.Value.Sort([](const FEntry& Left, const FEntry& Right)
				{
					if (Left.Order == Right.Order)
					{
						return Left.TieBreaker < Right.TieBreaker;
					}
					return Left.Order < Right.Order;
				});

				TArray<FSGSStableHandle>& Handles = HandlesByKey.FindOrAdd(Pair.Key);
				Handles.Reserve(Pair.Value.Num());
				for (const FEntry& Entry : Pair.Value)
				{
					Handles.Add(Entry.Handle);
				}
			}
		}

		const TArray<FSGSStableHandle>* Find(const KeyType& Key) const
		{
			return HandlesByKey.Find(Key);
		}

		TArray<FSGSStableHandle> FindCopy(const KeyType& Key) const
		{
			if (const TArray<FSGSStableHandle>* Handles = HandlesByKey.Find(Key))
			{
				return *Handles;
			}
			return TArray<FSGSStableHandle>();
		}

		virtual bool CheckInvariants(const TSGSIndexedStore<ItemType>& Store) const override
		{
			bool bOk = true;
			int32 Count = 0;
			Store.ForEach([this, &bOk, &Count](FSGSStableHandle Handle, const ItemType& Item)
			{
				++Count;
				const TArray<FSGSStableHandle>* Handles = HandlesByKey.Find(KeySelector(Item));
				bOk &= ensureMsgf(Handles != nullptr && Handles->Contains(Handle),
					TEXT("Ordered index %s is stale."), *Name.ToString());
			});

			int32 IndexedCount = 0;
			for (const TPair<KeyType, TArray<FSGSStableHandle>>& Pair : HandlesByKey)
			{
				IndexedCount += Pair.Value.Num();
			}
			bOk &= ensureMsgf(Count == IndexedCount, TEXT("Ordered index %s count mismatch."), *Name.ToString());
			return bOk;
		}

	private:
		struct FEntry
		{
			FSGSStableHandle Handle;
			int32 Order = 0;
			int32 TieBreaker = 0;
		};

		FName Name;
		TFunction<KeyType(const ItemType&)> KeySelector;
		TFunction<int32(const ItemType&)> OrderSelector;
		TMap<KeyType, TArray<FSGSStableHandle>> HandlesByKey;
	};

	void Reset()
	{
		Slots.Reset();
		FreeIndices.Reset();
		Indexes.Reset();
	}

	void ResetItems()
	{
		Slots.Reset();
		FreeIndices.Reset();
		const FSGSStatus Status = RebuildIndexes();
		ensureMsgf(!Status.HasError(), TEXT("IndexedStore ResetItems failed: %s"),
			Status.HasError() ? *Status.GetError().ToLogString() : TEXT(""));
	}

	template <typename KeyType>
	TSharedRef<TUniqueIndex<KeyType>> RegisterUniqueIndex(FName Name, TFunction<KeyType(const ItemType&)> KeySelector)
	{
		TSharedRef<TUniqueIndex<KeyType>> Index = MakeShared<TUniqueIndex<KeyType>>(Name, MoveTemp(KeySelector));
		Indexes.Add(StaticCastSharedRef<IIndex>(Index));
		Index->Rebuild(*this);
		return Index;
	}

	template <typename KeyType>
	TSharedRef<TNonUniqueIndex<KeyType>> RegisterNonUniqueIndex(FName Name, TFunction<KeyType(const ItemType&)> KeySelector)
	{
		TSharedRef<TNonUniqueIndex<KeyType>> Index = MakeShared<TNonUniqueIndex<KeyType>>(Name, MoveTemp(KeySelector));
		Indexes.Add(StaticCastSharedRef<IIndex>(Index));
		Index->Rebuild(*this);
		return Index;
	}

	template <typename KeyType>
	TSharedRef<TOrderedIndex<KeyType>> RegisterOrderedIndex(
		FName Name,
		TFunction<KeyType(const ItemType&)> KeySelector,
		TFunction<int32(const ItemType&)> OrderSelector)
	{
		TSharedRef<TOrderedIndex<KeyType>> Index = MakeShared<TOrderedIndex<KeyType>>(
			Name,
			MoveTemp(KeySelector),
			MoveTemp(OrderSelector));
		Indexes.Add(StaticCastSharedRef<IIndex>(Index));
		Index->Rebuild(*this);
		return Index;
	}

	FSGSStableHandle Add(ItemType Item)
	{
		int32 Index = INDEX_NONE;
		if (FreeIndices.Num() > 0)
		{
			Index = FreeIndices.Pop(EAllowShrinking::No);
			FSlot& Slot = Slots[Index];
			Slot.Value.Emplace(MoveTemp(Item));
			const FSGSStableHandle Handle(Index, Slot.Generation);
			const FSGSStatus Status = RebuildIndexes();
			ensureMsgf(!Status.HasError(), TEXT("IndexedStore Add rebuild failed: %s"),
				Status.HasError() ? *Status.GetError().ToLogString() : TEXT(""));
			return Handle;
		}

		Index = Slots.AddDefaulted();
		FSlot& Slot = Slots[Index];
		Slot.Generation = 1;
		Slot.Value.Emplace(MoveTemp(Item));
		const FSGSStableHandle Handle(Index, Slot.Generation);
		const FSGSStatus Status = RebuildIndexes();
		ensureMsgf(!Status.HasError(), TEXT("IndexedStore Add rebuild failed: %s"),
			Status.HasError() ? *Status.GetError().ToLogString() : TEXT(""));
		return Handle;
	}

	FSGSStatus Remove(FSGSStableHandle Handle)
	{
		if (!Handle.CheckInvariants() || !Handle.IsValid())
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.IndexedStore.InvalidHandle")),
				FString::Printf(TEXT("Remove called with invalid handle %s."), *Handle.ToLogString())));
		}

		FSlot* Slot = FindSlot(Handle);
		if (Slot == nullptr)
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.IndexedStore.NotFound")),
				FString::Printf(TEXT("Remove could not find handle %s."), *Handle.ToLogString())));
		}

		Slot->Value.Reset();
		++Slot->Generation;
		FreeIndices.Add(Handle.Index);
		return RebuildIndexes();
	}

	ItemType* Find(FSGSStableHandle Handle)
	{
		FSlot* Slot = FindSlot(Handle);
		return Slot != nullptr ? Slot->Value.GetPtrOrNull() : nullptr;
	}

	const ItemType* Find(FSGSStableHandle Handle) const
	{
		const FSlot* Slot = FindSlot(Handle);
		return Slot != nullptr ? Slot->Value.GetPtrOrNull() : nullptr;
	}

	template <typename MutatorType>
	FSGSStatus Modify(FSGSStableHandle Handle, MutatorType&& Mutator)
	{
		if (!Handle.CheckInvariants() || !Handle.IsValid())
		{
			return MakeError(FSGSError::Make(
				FName(TEXT("SGS.IndexedStore.InvalidHandle")),
				FString::Printf(TEXT("Modify called with invalid handle %s."), *Handle.ToLogString())));
		}

		if (ItemType* Item = Find(Handle))
		{
			Mutator(*Item);
			return RebuildIndexes();
		}

		return MakeError(FSGSError::Make(
			FName(TEXT("SGS.IndexedStore.NotFound")),
			FString::Printf(TEXT("Modify could not find handle %s."), *Handle.ToLogString())));
	}

	template <typename VisitorType>
	void ForEach(VisitorType&& Visitor) const
	{
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			const FSlot& Slot = Slots[Index];
			if (Slot.Value.IsSet())
			{
				Visitor(FSGSStableHandle(Index, Slot.Generation), Slot.Value.GetValue());
			}
		}
	}

	FSGSStatus RebuildIndexes() const
	{
		for (const TSharedRef<IIndex>& Index : Indexes)
		{
			Index->Rebuild(*this);
			if (!Index->CheckInvariants(*this))
			{
				return MakeError(FSGSError::InvariantViolation(
					FName(TEXT("IndexedStore")),
					FString::Printf(TEXT("Index %s is inconsistent."), *Index->GetName().ToString())));
			}
		}

		if (!CheckStorageInvariants())
		{
			return MakeError(FSGSError::InvariantViolation(
				FName(TEXT("IndexedStore")),
				TEXT("Store slots or free list are inconsistent.")));
		}

		return MakeValue();
	}

	bool CheckInvariants() const
	{
		bool bOk = CheckStorageInvariants();
		for (const TSharedRef<IIndex>& Index : Indexes)
		{
			bOk &= Index->CheckInvariants(*this);
		}
		return bOk;
	}

	int32 Num() const
	{
		return Slots.Num() - FreeIndices.Num();
	}

private:
	struct FSlot
	{
		TOptional<ItemType> Value;
		int32 Generation = 1;
	};

	FSlot* FindSlot(FSGSStableHandle Handle)
	{
		if (!Slots.IsValidIndex(Handle.Index))
		{
			return nullptr;
		}

		FSlot& Slot = Slots[Handle.Index];
		if (!Slot.Value.IsSet() || Slot.Generation != Handle.Generation)
		{
			return nullptr;
		}

		return &Slot;
	}

	const FSlot* FindSlot(FSGSStableHandle Handle) const
	{
		if (!Slots.IsValidIndex(Handle.Index))
		{
			return nullptr;
		}

		const FSlot& Slot = Slots[Handle.Index];
		if (!Slot.Value.IsSet() || Slot.Generation != Handle.Generation)
		{
			return nullptr;
		}

		return &Slot;
	}

	bool CheckStorageInvariants() const
	{
		bool bOk = true;
		TSet<int32> SeenFreeIndices;

		for (int32 FreeIndex : FreeIndices)
		{
			bOk &= ensureMsgf(Slots.IsValidIndex(FreeIndex), TEXT("IndexedStore free index is out of range."));
			bOk &= ensureMsgf(!SeenFreeIndices.Contains(FreeIndex), TEXT("IndexedStore duplicate free index."));
			SeenFreeIndices.Add(FreeIndex);
			if (Slots.IsValidIndex(FreeIndex))
			{
				bOk &= ensureMsgf(!Slots[FreeIndex].Value.IsSet(), TEXT("IndexedStore free slot still has a value."));
			}
		}

		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			const FSlot& Slot = Slots[Index];
			bOk &= ensureMsgf(Slot.Generation > 0, TEXT("IndexedStore slot has invalid generation."));
			if (!Slot.Value.IsSet())
			{
				bOk &= ensureMsgf(SeenFreeIndices.Contains(Index), TEXT("IndexedStore empty slot is not in free list."));
			}
		}

		return bOk;
	}

	TArray<FSlot> Slots;
	TArray<int32> FreeIndices;
	TArray<TSharedRef<IIndex>> Indexes;
};
