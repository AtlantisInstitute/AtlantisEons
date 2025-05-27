#include "StoreDataTableInspector.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"

void UStoreDataTableInspector::InspectDataTable()
{
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("COMPREHENSIVE DATA TABLE INSPECTION"));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));

    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("INSPECTOR: Failed to load data table"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ Data table loaded successfully"));

    // Get structure information
    FString StructureName = GetDataTableStructureName();
    UE_LOG(LogTemp, Warning, TEXT("📋 Row Structure: %s"), *StructureName);

    // Get all row names
    TArray<FString> RowNames = GetAllRowNames();
    UE_LOG(LogTemp, Warning, TEXT("📊 Total Rows: %d"), RowNames.Num());

    // List first 10 row names
    UE_LOG(LogTemp, Warning, TEXT("📝 Row Names:"));
    for (int32 i = 0; i < FMath::Min(10, RowNames.Num()); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("   %d: %s"), i + 1, *RowNames[i]);
    }

    if (RowNames.Num() > 10)
    {
        UE_LOG(LogTemp, Warning, TEXT("   ... and %d more rows"), RowNames.Num() - 10);
    }

    // Test structure compatibility
    TestStructureCompatibility();

    // Inspect first few rows in detail
    UE_LOG(LogTemp, Warning, TEXT("🔍 Detailed Row Inspection:"));
    for (int32 i = 0; i < FMath::Min(3, RowNames.Num()); i++)
    {
        InspectSpecificRow(RowNames[i]);
    }

    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("INSPECTION COMPLETE"));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

FString UStoreDataTableInspector::GetDataTableStructureName()
{
    UDataTable* ItemDataTable = GetItemDataTable();
    if (ItemDataTable && ItemDataTable->GetRowStruct())
    {
        return ItemDataTable->GetRowStruct()->GetName();
    }
    return TEXT("Unknown");
}

TArray<FString> UStoreDataTableInspector::GetAllRowNames()
{
    TArray<FString> RowNameStrings;
    
    UDataTable* ItemDataTable = GetItemDataTable();
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

void UStoreDataTableInspector::InspectSpecificRow(const FString& RowName)
{
    UE_LOG(LogTemp, Warning, TEXT("🔎 Inspecting Row: %s"), *RowName);

    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("   ❌ Data table not available"));
        return;
    }

    // Get raw row data
    void* RowData = ItemDataTable->FindRowUnchecked(*RowName);
    if (!RowData)
    {
        UE_LOG(LogTemp, Warning, TEXT("   ❌ Row not found: %s"), *RowName);
        return;
    }

    const UScriptStruct* RowStruct = ItemDataTable->GetRowStruct();
    if (!RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("   ❌ No row structure available"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("   ✅ Row found with structure: %s"), *RowStruct->GetName());
    LogRowDetails(RowData, const_cast<UScriptStruct*>(RowStruct), RowName);
}

void UStoreDataTableInspector::TestStructureCompatibility()
{
    UE_LOG(LogTemp, Warning, TEXT("🧪 Testing Structure Compatibility:"));

    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("   ❌ Data table not available"));
        return;
    }

    TArray<FString> RowNames = GetAllRowNames();
    if (RowNames.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("   ⚠️ No rows to test"));
        return;
    }

    FString TestRowName = RowNames[0];
    UE_LOG(LogTemp, Warning, TEXT("   🎯 Testing with row: %s"), *TestRowName);

    // Test FStructure_ItemInfo
    try
    {
        FStructure_ItemInfo* Row1 = ItemDataTable->FindRow<FStructure_ItemInfo>(*TestRowName, TEXT(""), false);
        if (Row1)
        {
            UE_LOG(LogTemp, Warning, TEXT("   ✅ FStructure_ItemInfo: COMPATIBLE"));
            UE_LOG(LogTemp, Warning, TEXT("      ItemIndex: %d"), Row1->ItemIndex);
            UE_LOG(LogTemp, Warning, TEXT("      ItemName: %s"), *Row1->ItemName);
            UE_LOG(LogTemp, Warning, TEXT("      Price: %d"), Row1->Price);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("   ❌ FStructure_ItemInfo: NOT COMPATIBLE"));
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Warning, TEXT("   ❌ FStructure_ItemInfo: EXCEPTION THROWN"));
    }

    // Test FBlueprintItemInfo
    try
    {
        FBlueprintItemInfo* Row2 = ItemDataTable->FindRow<FBlueprintItemInfo>(*TestRowName, TEXT(""), false);
        if (Row2)
        {
            UE_LOG(LogTemp, Warning, TEXT("   ✅ FBlueprintItemInfo: COMPATIBLE"));
            UE_LOG(LogTemp, Warning, TEXT("      ItemIndex: %d"), Row2->ItemIndex);
            UE_LOG(LogTemp, Warning, TEXT("      ItemName: %s"), *Row2->ItemName);
            UE_LOG(LogTemp, Warning, TEXT("      Price: %d"), Row2->Price);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("   ❌ FBlueprintItemInfo: NOT COMPATIBLE"));
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Warning, TEXT("   ❌ FBlueprintItemInfo: EXCEPTION THROWN"));
    }
}

UDataTable* UStoreDataTableInspector::GetItemDataTable()
{
    static UDataTable* CachedDataTable = nullptr;
    
    if (!CachedDataTable)
    {
        // Try multiple paths
        TArray<FString> TablePaths = {
            TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList"),
            TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList"),
            TEXT("/Game/AtlantisEons/Table_ItemList"),
            TEXT("/Game/Table_ItemList")
        };

        for (const FString& Path : TablePaths)
        {
            UE_LOG(LogTemp, Log, TEXT("INSPECTOR: Trying to load data table from: %s"), *Path);
            CachedDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *Path));
            if (CachedDataTable)
            {
                UE_LOG(LogTemp, Warning, TEXT("INSPECTOR: ✅ Loaded data table from: %s"), *Path);
                break;
            }
        }

        if (!CachedDataTable)
        {
            UE_LOG(LogTemp, Error, TEXT("INSPECTOR: ❌ Failed to load data table from any path"));
        }
    }

    return CachedDataTable;
}

void UStoreDataTableInspector::LogRowDetails(void* RowData, UScriptStruct* RowStruct, const FString& RowName)
{
    if (!RowData || !RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("   ❌ Invalid row data or structure"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("   📋 Properties found in row:"));

    // Iterate through all properties in the structure
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString PropertyName = Property->GetName();
        FString PropertyType = Property->GetClass()->GetName();

        UE_LOG(LogTemp, Warning, TEXT("      %s (%s)"), *PropertyName, *PropertyType);

        // Try to get some specific values for common properties
        if (PropertyName == TEXT("ItemIndex"))
        {
            if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
            {
                int32 Value = IntProp->GetPropertyValue_InContainer(RowData);
                UE_LOG(LogTemp, Warning, TEXT("         Value: %d"), Value);
            }
        }
        else if (PropertyName == TEXT("ItemName"))
        {
            if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
            {
                FString Value = StrProp->GetPropertyValue_InContainer(RowData);
                UE_LOG(LogTemp, Warning, TEXT("         Value: \"%s\""), *Value);
            }
        }
        else if (PropertyName == TEXT("Price"))
        {
            if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
            {
                int32 Value = IntProp->GetPropertyValue_InContainer(RowData);
                UE_LOG(LogTemp, Warning, TEXT("         Value: %d"), Value);
            }
        }
        else if (PropertyName == TEXT("ItemDescription"))
        {
            if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
            {
                FString Value = StrProp->GetPropertyValue_InContainer(RowData);
                UE_LOG(LogTemp, Warning, TEXT("         Value: \"%s\""), *Value);
            }
        }
    }
} 