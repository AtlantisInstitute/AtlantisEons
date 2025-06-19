#include "BP_ItemInfo.h"
#include "BlueprintItemTypes.h"
#include "StoreSystemFix.h"

UBP_ItemInfo::UBP_ItemInfo()
{
    // Initialize basic properties
    ItemIndex = 0;
    ItemName = TEXT("");
    ItemDescription = TEXT("");
    StackNumber = 1;
    Equipped = false;
    bIsValid = false;
    bIsStackable = false;
    ItemType = EItemType::Equip;
    ItemEquipSlot = EItemEquipSlot::Consumable;
    RecoveryHP = 0;
    RecoveryMP = 0;

    // Don't load data table in constructor - this can cause initialization order issues
    ItemDataTable = nullptr;
}

void UBP_ItemInfo::PostInitProperties()
{
    Super::PostInitProperties();
    
    // Load the item data table after object initialization is complete
    if (!ItemDataTable)
    {
        LoadItemDataTable();
    }
}

void UBP_ItemInfo::LoadItemDataTable()
{
    // Try to load the data table synchronously
    UDataTable* LoadedTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
        
    if (LoadedTable)
    {
        ItemDataTable = LoadedTable;
        
        // Verify the table structure - accept either structure name to be more resilient
        if (ItemDataTable->GetRowStruct())
        {
            FString StructureName = ItemDataTable->GetRowStruct()->GetName();
            if (ItemDataTable->GetRowStruct()->IsChildOf(Structure_ItemInfo::StaticStruct()) || 
                StructureName.Contains(TEXT("Structure_ItemInfo")))
            {
                UE_LOG(LogTemp, Log, TEXT("Successfully loaded item data table with structure: %s"), *StructureName);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Data table structure mismatch. Expected Structure_ItemInfo, got: %s"), 
                    *StructureName);
                // Don't set ItemDataTable to nullptr - try to use it anyway
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ItemDataTable has no row structure!"));
            // Still keep the table reference
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load ItemDataTable"));
    }
}

FStructure_ItemInfo UBP_ItemInfo::CreateHardcodedItemData(int32 InItemIndex)
{
    FStructure_ItemInfo ItemInfo;
    ItemInfo.ItemIndex = InItemIndex;
    
    switch (InItemIndex)
    {
        case 1: // Basic HP Potion
            ItemInfo.ItemName = TEXT("Basic HP Potion");
            ItemInfo.ItemDescription = TEXT("A basic potion that restores a small amount of HP.");
            ItemInfo.ItemType = EItemType::Consume_HP;
            ItemInfo.bIsStackable = true;
            ItemInfo.StackNumber = 99;
            ItemInfo.RecoveryHP = 20;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicHealingPotion")));
            break;
            
        case 2: // Basic MP Potion
            ItemInfo.ItemName = TEXT("Basic MP Potion");
            ItemInfo.ItemDescription = TEXT("A basic potion that restores a small amount of MP.");
            ItemInfo.ItemType = EItemType::Consume_MP;
            ItemInfo.bIsStackable = true;
            ItemInfo.StackNumber = 99;
            ItemInfo.RecoveryMP = 20;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicManaPotion")));
            break;
            
        default:
            ItemInfo.ItemName = FString::Printf(TEXT("Unknown Item %d"), InItemIndex);
            ItemInfo.ItemDescription = TEXT("Unknown item.");
            ItemInfo.ItemType = EItemType::Equip;
            ItemInfo.bIsStackable = false;
            ItemInfo.StackNumber = 1;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Engine/EditorResources/S_Actor")));
            break;
    }
    
    return ItemInfo;
}

void UBP_ItemInfo::GetItemTableRow(bool& Find, Structure_ItemInfo& ItemInfoStructure)
{
    // Use the robust StoreSystemFix to get item data - this handles the ItemIndex mapping correctly
    FStructure_ItemInfo StoreItemInfo;
    bool bStoreFound = UStoreSystemFix::GetItemData(ItemIndex, StoreItemInfo);
    
    if (bStoreFound && StoreItemInfo.bIsValid)
    {
        ItemInfoStructure = StoreItemInfo;
        Find = true;
        UE_LOG(LogTemp, Display, TEXT("✅ GetItemTableRow: Successfully found item %d using StoreSystemFix: '%s'"), ItemIndex, *StoreItemInfo.ItemName);
        return;
    }
    
    // Fallback to the original method if StoreSystemFix fails
    UE_LOG(LogTemp, Warning, TEXT("GetItemTableRow: StoreSystemFix failed for item %d, trying fallback method"), ItemIndex);
    
    // Get the data table
    static const FString TablePath = TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList");
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
    
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("GetItemTableRow: Failed to load item data table from path: %s"), *TablePath);
        Find = false;
        ItemInfoStructure = CreateHardcodedItemData(ItemIndex);
        return;
    }

    // Try multiple row name formats to find the correct one
    TArray<FString> RowNamePatterns = {
        FString::FromInt(ItemIndex),                    // "7" (numeric format used by store system)
        FString::Printf(TEXT("Item_%d"), ItemIndex),    // "Item_7" (prefixed format)
        FString::Printf(TEXT("%d"), ItemIndex)          // Explicit numeric format
    };
    
    FStructure_ItemInfo* Row = nullptr;
    FString FoundRowName;
    
    // Try each pattern until we find a match
    for (const FString& RowName : RowNamePatterns)
    {
        static const FString ContextString(TEXT("UBP_ItemInfo::GetItemTableRow"));
        Row = ItemDataTable->FindRow<FStructure_ItemInfo>(*RowName, ContextString, false);
        if (Row)
        {
            FoundRowName = RowName;
            UE_LOG(LogTemp, Display, TEXT("GetItemTableRow: Found item %d using row name pattern: %s"), ItemIndex, *RowName);
            break;
        }
    }
    
    if (Row)
    {
        ItemInfoStructure = *Row;
        Find = true;
        UE_LOG(LogTemp, Display, TEXT("✅ Successfully found item data for index %d using row name: %s"), ItemIndex, *FoundRowName);
        return;
    }
    
    // If not found in data table, log the attempted patterns and use hardcoded data
    FString AttemptedPatterns;
    for (int32 i = 0; i < RowNamePatterns.Num(); i++)
    {
        AttemptedPatterns += RowNamePatterns[i];
        if (i < RowNamePatterns.Num() - 1)
        {
            AttemptedPatterns += TEXT(", ");
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("❌ Item %d not found in data table. Tried patterns: [%s]. Using hardcoded data."), ItemIndex, *AttemptedPatterns);
    ItemInfoStructure = CreateHardcodedItemData(ItemIndex);
    Find = true;
}

UDataTable* UBP_ItemInfo::GetItemDataTable() const
{
    return ItemDataTable;
}

void UBP_ItemInfo::SetItemDataTable(UDataTable* NewDataTable)
{
    ItemDataTable = NewDataTable;
}
