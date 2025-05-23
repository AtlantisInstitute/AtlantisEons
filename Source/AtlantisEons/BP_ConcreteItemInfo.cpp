#include "BP_ConcreteItemInfo.h"

void UBP_ConcreteItemInfo::GetItemTableRow(bool& Find, FStructure_ItemInfo& ItemInfoStructure)
{
    // Get the data table
    static const FString TablePath = TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList");
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
    
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load item data table from path: %s"), *TablePath);
        Find = false;
        return;
    }

    // Create the row name with proper format
    FString RowName = FString::Printf(TEXT("Item_%d"), FMath::Max(0, ItemIndex));
    
    UE_LOG(LogTemp, Display, TEXT("Looking up item with row name: %s in table: %s"), *RowName, *TablePath);
    
    // Get the row from the data table
    FStructure_ItemInfo* FoundRow = ItemDataTable->FindRow<FStructure_ItemInfo>(*RowName, TEXT(""), true);
    if (FoundRow)
    {
        ItemInfoStructure = *FoundRow;
        Find = true;
        UE_LOG(LogTemp, Display, TEXT("Found item data for row: %s"), *RowName);
    }
    else
    {
        Find = false;
        UE_LOG(LogTemp, Warning, TEXT("Failed to find item data for row: %s"), *RowName);
    }
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