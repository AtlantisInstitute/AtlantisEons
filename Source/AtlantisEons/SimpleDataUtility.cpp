#include "SimpleDataUtility.h"

FString USimpleDataUtility::GetItemTableStructureName()
{
    // Load the item data table
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (ItemDataTable && ItemDataTable->GetRowStruct())
    {
        // Return the exact name of the structure used by the data table
        return ItemDataTable->GetRowStruct()->GetName();
    }
    
    return TEXT("StructureNotFound");
}

bool USimpleDataUtility::DoesItemExist(int32 ItemIndex)
{
    // Load the item data table
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (ItemDataTable)
    {
        // Convert ItemIndex to row name string
        FString RowName = FString::FromInt(ItemIndex);
        
        // Check if row exists using generic row finding (avoids structure type issues)
        void* RowData = ItemDataTable->FindRowUnchecked(*RowName);
        return (RowData != nullptr);
    }
    
    return false;
}
