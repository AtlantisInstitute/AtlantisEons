#include "DataTableDiagnostic.h"
#include "ItemTypes.h"
#include "BlueprintItemTypes.h"

bool UDataTableDiagnostic::InspectItemDataTable()
{
    UE_LOG(LogTemp, Warning, TEXT("=== DATA TABLE DIAGNOSTIC START ==="));
    
    // Load the data table
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("DIAGNOSTIC: Failed to load data table"));
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("DIAGNOSTIC: Successfully loaded data table"));
    
    // Get row names
    TArray<FName> RowNames = ItemDataTable->GetRowNames();
    UE_LOG(LogTemp, Warning, TEXT("DIAGNOSTIC: Found rows in data table"));
    
    UE_LOG(LogTemp, Warning, TEXT("=== DATA TABLE DIAGNOSTIC END ==="));
    return true;
}

TArray<FString> UDataTableDiagnostic::GetDataTableRowNames()
{
    TArray<FString> RowNameStrings;
    
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (ItemDataTable)
    {
        TArray<FName> RowNames = ItemDataTable->GetRowNames();
        for (FName RowName : RowNames)
        {
            RowNameStrings.Add(RowName.ToString());
        }
    }
    
    return RowNameStrings;
}

FString UDataTableDiagnostic::GetDataTableStructureName()
{
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (ItemDataTable && ItemDataTable->GetRowStruct())
    {
        return ItemDataTable->GetRowStruct()->GetName();
    }
    
    return TEXT("Unknown");
}

bool UDataTableDiagnostic::TestReadRawRowData(const FString& RowName)
{
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("DIAGNOSTIC: Failed to load data table for row test"));
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("DIAGNOSTIC: Testing row"));
    
    // Test unchecked access
    void* RowData = ItemDataTable->FindRowUnchecked(*RowName);
    if (RowData)
    {
        UE_LOG(LogTemp, Warning, TEXT("DIAGNOSTIC: Row found via unchecked access"));
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DIAGNOSTIC: Row not found"));
        return false;
    }
}

bool UDataTableDiagnostic::ExportDataTableContent()
{
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("DIAGNOSTIC: Failed to load data table for export"));
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== DATA TABLE CONTENT EXPORT ==="));
    
    TArray<FName> RowNames = ItemDataTable->GetRowNames();
    for (FName RowName : RowNames)
    {
        UE_LOG(LogTemp, Warning, TEXT("EXPORT: Row found"));
        
        void* RowData = ItemDataTable->FindRowUnchecked(RowName);
        if (RowData)
        {
            UE_LOG(LogTemp, Warning, TEXT("EXPORT: Data found"));
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== DATA TABLE CONTENT EXPORT END ==="));
    return true;
} 