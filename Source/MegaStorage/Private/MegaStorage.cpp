#include "MegaStorage.h"

#include "Buildables/FGBuildableStorage.h"
#include "FGInventoryComponent.h"
#include "FGRecipe.h"
#include "Patching/NativeHookManager.h"

DEFINE_LOG_CATEGORY(LogMegaStorage);

namespace
{
	// The target inventory grid for Mega Storage: 10 columns x 50 rows = 500 slots.
	// A larger size (2000, 8x250) was tested and reliably stalled belt/lift input after a
	// few hundred items despite the inventory itself never filling up -- some vanilla
	// factory-input bookkeeping doesn't scale cleanly with a very large override. 500 was
	// confirmed stable well past that failure point (~2000 items fed in continuously with
	// no stall), so it's the safe ceiling for now.
	constexpr int32 TargetInventorySizeX = 10;
	constexpr int32 TargetInventorySizeY = 50;

	// ContentLib names the generated recipe Blueprint class after the JSON file
	// (Recipe_MegaStorage.json -> Recipe_MegaStorage_C). We match on the short class
	// name rather than a full package path, since we don't control/know exactly which
	// package ContentLib generates it under, and the short name is documented/stable.
	bool IsMegaStorageRecipe(const TSubclassOf<UFGRecipe>& Recipe)
	{
		if (!Recipe)
		{
			return false;
		}

		const FString RecipeClassName = Recipe->GetName();
		return RecipeClassName.Equals(TEXT("Recipe_MegaStorage_C")) ||
			RecipeClassName.Equals(TEXT("Recipe_MegaStorage"));
	}
}

void FMegaStorageModule::StartupModule()
{
	UE_LOG(LogMegaStorage, Log, TEXT("MegaStorage module loaded."));

	// Storage containers create their mStorageInventory component during BeginPlay, sized
	// from mInventorySizeX/mInventorySizeY. We hook BEFORE the vanilla BeginPlay runs so
	// that, for buildables constructed from our recipe, the inventory is created at the
	// larger size from the start (rather than needing a separate resize call afterwards).
	// Every other Industrial Storage Container Mk.2 (built via the vanilla recipe) is left
	// completely untouched.
	InventorySizeHookHandle = SUBSCRIBE_METHOD_VIRTUAL(AFGBuildableStorage::BeginPlay, GetDefault<AFGBuildableStorage>(), [](auto& Scope, AFGBuildableStorage* Self)
	{
		if (!IsValid(Self))
		{
			return;
		}

		if (!IsMegaStorageRecipe(Self->GetBuiltWithRecipe()))
		{
			return;
		}

		Self->mInventorySizeX = TargetInventorySizeX;
		Self->mInventorySizeY = TargetInventorySizeY;

		UE_LOG(LogMegaStorage, Log, TEXT("MegaStorage: expanding inventory for %s to %dx%d (%d slots)"),
			*GetNameSafe(Self),
			TargetInventorySizeX,
			TargetInventorySizeY,
			TargetInventorySizeX * TargetInventorySizeY);
	});

	// Belt-fed items were observed to stop flowing into the container well before it was
	// actually full, even though the inventory UI correctly showed the full 2000-slot grid.
	// That points to some factory-input-side bookkeeping (separate from the inventory
	// component's own slot array) still being sized off whatever mStorageInventory ended up
	// with when Super::BeginPlay() first created it. Rather than rely on exactly how/when
	// vanilla BeginPlay derives that size from mInventorySizeX/Y, explicitly (and
	// redundantly) resize the real component ourselves once BeginPlay has fully run, so the
	// actual usable capacity is guaranteed to match the displayed grid.
	InventoryResizeHookHandle = SUBSCRIBE_METHOD_VIRTUAL_AFTER(AFGBuildableStorage::BeginPlay, GetDefault<AFGBuildableStorage>(), [](AFGBuildableStorage* Self)
	{
		if (!IsValid(Self))
		{
			return;
		}

		if (!IsMegaStorageRecipe(Self->GetBuiltWithRecipe()))
		{
			return;
		}

		UFGInventoryComponent* Inventory = Self->GetStorageInventory();
		if (!IsValid(Inventory))
		{
			return;
		}

		const int32 TargetSlots = TargetInventorySizeX * TargetInventorySizeY;
		if (Inventory->GetSizeLinear() != TargetSlots)
		{
			UE_LOG(LogMegaStorage, Log, TEXT("MegaStorage: forcing inventory resize for %s from %d to %d slots"),
				*GetNameSafe(Self),
				Inventory->GetSizeLinear(),
				TargetSlots);
			Inventory->Resize(TargetSlots);
		}
	});
}

void FMegaStorageModule::ShutdownModule()
{
	if (InventorySizeHookHandle.IsValid())
	{
		UNSUBSCRIBE_METHOD(AFGBuildableStorage::BeginPlay, InventorySizeHookHandle);
	}

	if (InventoryResizeHookHandle.IsValid())
	{
		UNSUBSCRIBE_METHOD(AFGBuildableStorage::BeginPlay, InventoryResizeHookHandle);
	}

	UE_LOG(LogMegaStorage, Log, TEXT("MegaStorage module unloaded."));
}

IMPLEMENT_MODULE(FMegaStorageModule, MegaStorage)
