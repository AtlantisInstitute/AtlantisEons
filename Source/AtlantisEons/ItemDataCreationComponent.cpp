#include "ItemDataCreationComponent.h"
#include "BP_ItemInfo.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "BP_ItemInfo.h"
#include "AtlantisEonsGameInstance.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"

UItemDataCreationComponent::UItemDataCreationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CachedDataTable = nullptr;
}

// ========== CORE ITEM CREATION FUNCTIONS ==========

FStructure_ItemInfo UItemDataCreationComponent::CreateItemData(int32 ItemIndex)
{
    // Delegate to hardcoded data creation or DataTable
    return CreateHardcodedItemData(ItemIndex);
}

FStructure_ItemInfo UItemDataCreationComponent::CreateItemDataWithStack(int32 ItemIndex, int32 StackNumber)
{
    FStructure_ItemInfo ItemInfo = CreateItemData(ItemIndex);
    ItemInfo.StackNumber = StackNumber;
    return ItemInfo;
}

TArray<FStructure_ItemInfo> UItemDataCreationComponent::CreateMultipleItems(const TArray<int32>& ItemIndices)
{
    TArray<FStructure_ItemInfo> Items;
    Items.Reserve(ItemIndices.Num());

    for (int32 ItemIndex : ItemIndices)
    {
        Items.Add(CreateItemData(ItemIndex));
    }

    return Items;
}

// ========== HELPER FUNCTIONS (Simplified) ==========

// ========== ASSET LOADING AND VALIDATION ==========

UTexture2D* UItemDataCreationComponent::LoadItemTexture(int32 ItemIndex, const FString& ItemName)
{
    AssetLoadAttempts++;
    
    // First try the exact path from item data
    FStructure_ItemInfo ItemInfo = CreateItemData(ItemIndex);
    if (!ItemInfo.ItemThumbnail.IsNull())
    {
        UTexture2D* LoadedTexture = ItemInfo.ItemThumbnail.LoadSynchronous();
        if (LoadedTexture)
        {
            AssetLoadSuccesses++;
            LogAssetLoadResult(TEXT("Texture"), ItemInfo.ItemThumbnail.ToString(), true);
            return LoadedTexture;
        }
    }

    // Generate alternative paths
    TArray<FString> TexturePaths = GenerateTexturePaths(ItemIndex, ItemName.IsEmpty() ? ItemInfo.ItemName : ItemName);
    UTexture2D* LoadedTexture = LoadAssetFromPaths<UTexture2D>(TexturePaths);
    
    if (LoadedTexture)
    {
        AssetLoadSuccesses++;
        return LoadedTexture;
    }

    // Return default texture
    LogAssetLoadResult(TEXT("Texture"), TEXT("Multiple paths"), false);
    return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Engine/EditorResources/S_Actor")));
}

UStaticMesh* UItemDataCreationComponent::LoadItemMesh(int32 ItemIndex, const FString& ItemName)
{
    AssetLoadAttempts++;
    
    // First try the exact path from item data
    FStructure_ItemInfo ItemInfo = CreateItemData(ItemIndex);
    if (!ItemInfo.StaticMeshID.IsNull())
    {
        UStaticMesh* LoadedMesh = ItemInfo.StaticMeshID.LoadSynchronous();
        if (LoadedMesh)
        {
            AssetLoadSuccesses++;
            LogAssetLoadResult(TEXT("Mesh"), ItemInfo.StaticMeshID.ToString(), true);
            return LoadedMesh;
        }
    }

    // Generate alternative paths
    TArray<FString> MeshPaths = GenerateMeshPaths(ItemIndex, ItemName.IsEmpty() ? ItemInfo.ItemName : ItemName);
    UStaticMesh* LoadedMesh = LoadAssetFromPaths<UStaticMesh>(MeshPaths);
    
    if (LoadedMesh)
    {
        AssetLoadSuccesses++;
        return LoadedMesh;
    }

    LogAssetLoadResult(TEXT("Mesh"), TEXT("Multiple paths"), false);
    return nullptr;
}

bool UItemDataCreationComponent::ValidateItemAssets(const FStructure_ItemInfo& ItemInfo)
{
    bool bAllValid = true;

    // Check thumbnail
    if (!ItemInfo.ItemThumbnail.IsNull())
    {
        UTexture2D* Texture = ItemInfo.ItemThumbnail.LoadSynchronous();
        if (!Texture)
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ Invalid thumbnail for item %d: %s"), ItemInfo.ItemIndex, *ItemInfo.ItemThumbnail.ToString());
            bAllValid = false;
        }
    }

    // Check mesh (for equipment items)
    if (ItemInfo.ItemType == EItemType::Equip && !ItemInfo.StaticMeshID.IsNull())
    {
        UStaticMesh* Mesh = ItemInfo.StaticMeshID.LoadSynchronous();
        if (!Mesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ Invalid mesh for item %d: %s"), ItemInfo.ItemIndex, *ItemInfo.StaticMeshID.ToString());
            bAllValid = false;
        }
    }

    return bAllValid;
}

// ========== DATA TABLE INTEGRATION ==========

bool UItemDataCreationComponent::LoadFromDataTable(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo)
{
    if (!CachedDataTable)
    {
        CachedDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
            TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    }

    if (!CachedDataTable)
    {
        return false;
    }

    // Try multiple row name patterns
    TArray<FString> RowPatterns = {
        FString::FromInt(ItemIndex),
        FString::Printf(TEXT("Item_%d"), ItemIndex),
        FString::Printf(TEXT("item_%d"), ItemIndex),
        FString::Printf(TEXT("Row_%d"), ItemIndex)
    };

    for (const FString& RowPattern : RowPatterns)
    {
        FStructure_ItemInfo* Row = CachedDataTable->FindRow<FStructure_ItemInfo>(*RowPattern, TEXT("ItemDataCreationComponent"), false);
        if (Row)
        {
            OutItemInfo = *Row;
            DataTableHits++;
            UE_LOG(LogTemp, Log, TEXT("✅ Found item %d in data table using pattern: %s"), ItemIndex, *RowPattern);
            return true;
        }
    }

    return false;
}

bool UItemDataCreationComponent::GetItemDataWithFallback(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo)
{
    // Try data table first
    if (LoadFromDataTable(ItemIndex, OutItemInfo))
    {
        return true;
    }

    // Fallback to hardcoded data
    OutItemInfo = CreateItemData(ItemIndex);
    HardcodedFallbacks++;
    return OutItemInfo.bIsValid;
}

// ========== ITEM VALIDATION AND UTILITIES ==========

bool UItemDataCreationComponent::IsValidItemIndex(int32 ItemIndex)
{
    return (ItemIndex >= 1 && ItemIndex <= 40);
}

TArray<int32> UItemDataCreationComponent::GetAllAvailableItemIndices()
{
    TArray<int32> Indices;
    for (int32 i = 1; i <= 40; ++i)
    {
        Indices.Add(i);
    }
    return Indices;
}

TArray<FStructure_ItemInfo> UItemDataCreationComponent::GetItemsByType(EItemType ItemType)
{
    TArray<FStructure_ItemInfo> FilteredItems;
    
    for (int32 i = 1; i <= 40; ++i)
    {
        FStructure_ItemInfo ItemInfo = CreateItemData(i);
        if (ItemInfo.ItemType == ItemType)
        {
            FilteredItems.Add(ItemInfo);
        }
    }
    
    return FilteredItems;
}

TArray<FStructure_ItemInfo> UItemDataCreationComponent::GetItemsByEquipSlot(EItemEquipSlot EquipSlot)
{
    TArray<FStructure_ItemInfo> FilteredItems;
    
    for (int32 i = 1; i <= 40; ++i)
    {
        FStructure_ItemInfo ItemInfo = CreateItemData(i);
        if (ItemInfo.ItemEquipSlot == EquipSlot)
        {
            FilteredItems.Add(ItemInfo);
        }
    }
    
    return FilteredItems;
}

// ========== BLUEPRINT ITEM INFO CREATION ==========

UBP_ItemInfo* UItemDataCreationComponent::CreateBlueprintItemInfo(int32 ItemIndex, int32 StackNumber)
{
    UBP_ItemInfo* NewItemInfo = NewObject<UBP_ItemInfo>(this);
    if (!NewItemInfo)
    {
        return nullptr;
    }

    FStructure_ItemInfo ItemData = CreateItemData(ItemIndex);
    ItemData.StackNumber = StackNumber;
    NewItemInfo->CopyFrom(ItemData);

    return NewItemInfo;
}

// ========== DEBUGGING AND LOGGING ==========

void UItemDataCreationComponent::LogItemInfo(const FStructure_ItemInfo& ItemInfo)
{
    UE_LOG(LogTemp, Log, TEXT("=== ITEM INFO [%d] ==="), ItemInfo.ItemIndex);
    UE_LOG(LogTemp, Log, TEXT("Name: %s"), *ItemInfo.ItemName);
    UE_LOG(LogTemp, Log, TEXT("Description: %s"), *ItemInfo.ItemDescription);
    UE_LOG(LogTemp, Log, TEXT("Type: %d, Slot: %d"), (int32)ItemInfo.ItemType, (int32)ItemInfo.ItemEquipSlot);
    UE_LOG(LogTemp, Log, TEXT("Stackable: %s, Stack: %d"), ItemInfo.bIsStackable ? TEXT("Yes") : TEXT("No"), ItemInfo.StackNumber);
    UE_LOG(LogTemp, Log, TEXT("Price: %d"), ItemInfo.Price);
    UE_LOG(LogTemp, Log, TEXT("Stats - DMG:%d DEF:%d HP:%d MP:%d STR:%d DEX:%d INT:%d"), 
        ItemInfo.Damage, ItemInfo.Defence, ItemInfo.HP, ItemInfo.MP, ItemInfo.STR, ItemInfo.DEX, ItemInfo.INT);
    UE_LOG(LogTemp, Log, TEXT("Recovery - HP:%d MP:%d"), ItemInfo.RecoveryHP, ItemInfo.RecoveryMP);
    UE_LOG(LogTemp, Log, TEXT("========================"));
}

FString UItemDataCreationComponent::GetDatabaseStatistics()
{
    float SuccessRate = (AssetLoadAttempts > 0) ? (float(AssetLoadSuccesses) / float(AssetLoadAttempts)) * 100.0f : 0.0f;
    
    return FString::Printf(TEXT("📊 Item Database Statistics:\n"
        "Items Available: 40\n"
        "Data Table Hits: %d\n"
        "Hardcoded Fallbacks: %d\n"
        "Asset Load Attempts: %d\n"
        "Asset Load Successes: %d\n"
        "Asset Success Rate: %.1f%%"), 
        DataTableHits, HardcodedFallbacks, AssetLoadAttempts, AssetLoadSuccesses, SuccessRate);
}

// ========== HELPER FUNCTIONS ==========

TArray<FString> UItemDataCreationComponent::GenerateTexturePaths(int32 ItemIndex, const FString& ItemName)
{
    TArray<FString> Paths;
    
    // Clean item name for path generation
    FString CleanName = ItemName.Replace(TEXT(" "), TEXT(""));
    
    // Generate various path patterns
    Paths.Add(FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_%s"), *CleanName));
    Paths.Add(FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_Item_%d"), ItemIndex));
    Paths.Add(FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_%d"), ItemIndex));
    Paths.Add(FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/%s"), *CleanName));
    Paths.Add(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicHealingPotion"));
    Paths.Add(TEXT("/Engine/EditorResources/S_Actor"));
    
    return Paths;
}

TArray<FString> UItemDataCreationComponent::GenerateMeshPaths(int32 ItemIndex, const FString& ItemName)
{
    TArray<FString> Paths;
    
    // Clean item name for path generation
    FString CleanName = ItemName.Replace(TEXT(" "), TEXT(""));
    
    // Generate various path patterns based on item type
    Paths.Add(FString::Printf(TEXT("/Game/AtlantisEons/Sources/Models/Items/SM_%s"), *CleanName));
    Paths.Add(FString::Printf(TEXT("/Game/AtlantisEons/Sources/Models/Weapons/SM_%s"), *CleanName));
    Paths.Add(FString::Printf(TEXT("/Game/AtlantisEons/Sources/Models/Armor/SM_%s"), *CleanName));
    Paths.Add(FString::Printf(TEXT("/Game/AtlantisEons/Sources/Models/SM_Item_%d"), ItemIndex));
    Paths.Add(FString::Printf(TEXT("/Game/AtlantisEons/Sources/Models/SM_%d"), ItemIndex));
    
    return Paths;
}

template<typename T>
T* UItemDataCreationComponent::LoadAssetFromPaths(const TArray<FString>& AssetPaths)
{
    for (const FString& Path : AssetPaths)
    {
        T* LoadedAsset = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *Path));
        if (LoadedAsset)
        {
            LogAssetLoadResult(T::StaticClass()->GetName(), Path, true);
            return LoadedAsset;
        }
    }
    
    LogAssetLoadResult(T::StaticClass()->GetName(), TEXT("All paths"), false);
    return nullptr;
}

void UItemDataCreationComponent::LogAssetLoadResult(const FString& AssetType, const FString& AssetPath, bool bSuccess)
{
    if (bSuccess)
    {
        UE_LOG(LogTemp, Verbose, TEXT("✅ Loaded %s: %s"), *AssetType, *AssetPath);
    }
    else
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("❌ Failed to load %s: %s"), *AssetType, *AssetPath);
    }
}

FStructure_ItemInfo UItemDataCreationComponent::CreateHardcodedItemData(int32 ItemIndex)
{
    FStructure_ItemInfo ItemInfo;
    
    // Create only 1 basic item - Blueprint will handle variations via properties and data table
    if (ItemIndex == 1)
    {
        // Basic Test Item
        ItemInfo.ItemIndex = ItemIndex;
        ItemInfo.ItemName = TEXT("Basic Item");
        ItemInfo.ItemType = EItemType::Consume_HP;
        ItemInfo.ItemEquipSlot = EItemEquipSlot::Consumable;
        ItemInfo.bIsValid = true;
        ItemInfo.bIsStackable = true;
        ItemInfo.StackNumber = 1;
        ItemInfo.ItemDescription = TEXT("A basic test item");
        ItemInfo.Price = 10;
        
        // Basic stats
        ItemInfo.STR = 0;
        ItemInfo.DEX = 0;
        ItemInfo.INT = 0;
        ItemInfo.Defence = 0;
        ItemInfo.Damage = 0;
        ItemInfo.RecoveryHP = 10;
        ItemInfo.RecoveryMP = 0;
        
        // Try to load texture with fallback
        ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(LoadItemTexture(ItemIndex, TEXT("Basic_Item")));
        
        // Try to load mesh if needed
        ItemInfo.StaticMeshID = TSoftObjectPtr<UStaticMesh>(LoadItemMesh(ItemIndex, TEXT("Basic_Item")));
        
        UE_LOG(LogTemp, Warning, TEXT("ItemDataCreation: Created basic item (ID: %d)"), ItemIndex);
    }
    else
    {
        // For any other index, return empty item info (Blueprint/DataTable will handle)
        UE_LOG(LogTemp, Warning, TEXT("ItemDataCreation: Item ID %d not found in hardcoded data - Blueprint should handle via DataTable"), ItemIndex);
    }
    
    // Update statistics
    if (ItemInfo.ItemName != TEXT(""))
    {
        ItemsCreated++;
        LastCreatedItemIndex = ItemIndex;
    }
    
    return ItemInfo;
} 