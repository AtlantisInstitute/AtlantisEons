#include "BP_ConcreteItemInfo.h"
#include "StoreSystemFix.h"

void UBP_ConcreteItemInfo::GetItemTableRow(bool& Find, FStructure_ItemInfo& ItemInfoStructure)
{
    // Use the robust StoreSystemFix to get item data - this handles the ItemIndex mapping correctly
    FStructure_ItemInfo StoreItemInfo;
    bool bStoreFound = UStoreSystemFix::GetItemData(ItemIndex, StoreItemInfo);
    
    if (bStoreFound && StoreItemInfo.bIsValid)
    {
        ItemInfoStructure = StoreItemInfo;
        Find = true;
        UE_LOG(LogTemp, Display, TEXT("✅ BP_ConcreteItemInfo: Successfully found item %d using StoreSystemFix: '%s'"), ItemIndex, *StoreItemInfo.ItemName);
        return;
    }
    
    // Fallback to the original method if StoreSystemFix fails
    UE_LOG(LogTemp, Warning, TEXT("BP_ConcreteItemInfo: StoreSystemFix failed for item %d, trying fallback method"), ItemIndex);
    
    // Get the data table
    static const FString TablePath = TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList");
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
    
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("BP_ConcreteItemInfo: Failed to load item data table from path: %s"), *TablePath);
        Find = false;
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
        static const FString ContextString(TEXT("UBP_ConcreteItemInfo::GetItemTableRow"));
        Row = ItemDataTable->FindRow<FStructure_ItemInfo>(*RowName, ContextString, false);
        if (Row)
        {
            FoundRowName = RowName;
            UE_LOG(LogTemp, Display, TEXT("BP_ConcreteItemInfo: Found item %d using row name pattern: %s"), ItemIndex, *RowName);
            break;
        }
    }
    
    if (Row)
    {
        ItemInfoStructure = *Row;
        Find = true;
        UE_LOG(LogTemp, Display, TEXT("✅ BP_ConcreteItemInfo: Successfully found item data for index %d using row name: %s"), ItemIndex, *FoundRowName);
        return;
    }
    
    // If not found in data table, log the attempted patterns
    FString AttemptedPatterns;
    for (int32 i = 0; i < RowNamePatterns.Num(); i++)
    {
        AttemptedPatterns += RowNamePatterns[i];
        if (i < RowNamePatterns.Num() - 1)
        {
            AttemptedPatterns += TEXT(", ");
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("❌ BP_ConcreteItemInfo: Item %d not found in data table. Tried patterns: [%s]"), ItemIndex, *AttemptedPatterns);
    Find = false;
}

void UBP_ConcreteItemInfo::CopyFromStructure(const FStructure_ItemInfo& ItemInfoStructure)
{
    ItemIndex = ItemInfoStructure.ItemIndex;
    ItemName = ItemInfoStructure.ItemName;
    ItemDescription = ItemInfoStructure.ItemDescription;
    MeshID = ItemInfoStructure.StaticMeshID;
    Thumbnail = ItemInfoStructure.ItemThumbnail;
    bIsValid = ItemInfoStructure.bIsValid;
    bIsStackable = ItemInfoStructure.bIsStackable;
    ItemType = ItemInfoStructure.ItemType;
    ItemEquipSlot = ItemInfoStructure.ItemEquipSlot;
    RecoveryHP = ItemInfoStructure.RecoveryHP;
    RecoveryMP = ItemInfoStructure.RecoveryMP;
} 