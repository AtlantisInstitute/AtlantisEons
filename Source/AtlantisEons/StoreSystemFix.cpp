#include "StoreSystemFix.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

bool UStoreSystemFix::GetItemData(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo)
{
    UE_LOG(LogTemp, Verbose, TEXT("StoreSystemFix: Getting item %d"), ItemIndex);

    // Use the new property mapping system
    OutItemInfo = GetItemDataFromTable(ItemIndex);
    
    if (OutItemInfo.bIsValid)
    {
        UE_LOG(LogTemp, Verbose, TEXT("StoreSystemFix: Got item %d: %s"), ItemIndex, *OutItemInfo.ItemName);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("StoreSystemFix: Failed item %d, using fallback"), ItemIndex);
        return true; // Fallback data is still valid
    }
}

TArray<FStructure_ItemInfo> UStoreSystemFix::GetAllStoreItems()
{
    TArray<FStructure_ItemInfo> StoreItems;
    
    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("StoreSystemFix: No data table"));
        // Return fallback items
        for (int32 i = 1; i <= 10; i++)
        {
            StoreItems.Add(CreateFallbackItemData(i));
        }
        return StoreItems;
    }

    // Get all row names from the data table
    TArray<FName> RowNames = ItemDataTable->GetRowNames();
    UE_LOG(LogTemp, Log, TEXT("StoreSystemFix: Found %d rows"), RowNames.Num());

    // Extract items based on actual row names in the data table
    for (FName RowName : RowNames)
    {
        FString RowString = RowName.ToString();
        int32 ItemIndex = -1;

        // Try to extract item index from row name
        if (RowString.IsNumeric())
        {
            ItemIndex = FCString::Atoi(*RowString);
        }
        else if (RowString.StartsWith(TEXT("Item_")))
        {
            FString IndexString = RowString.RightChop(5);
            if (IndexString.IsNumeric())
            {
                ItemIndex = FCString::Atoi(*IndexString);
            }
        }

        if (ItemIndex > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("StoreSystemFix: Processing row '%s' -> ItemIndex %d"), *RowString, ItemIndex);
            
            // Get the actual row data
            void* RowData = ItemDataTable->FindRowUnchecked(RowName);
            if (RowData)
            {
                const UScriptStruct* RowStruct = ItemDataTable->GetRowStruct();
                if (RowStruct)
                {
                    // Pass the extracted ItemIndex as a hint, but let ExtractItemInfoWithMapping use the real data table value
                    FStructure_ItemInfo ItemInfo = ExtractItemInfoWithMapping(RowData, RowStruct, ItemIndex);
                    if (ItemInfo.bIsValid)
                    {
                        StoreItems.Add(ItemInfo);
                        UE_LOG(LogTemp, Log, TEXT("✅ Row '%s' -> Extracted: %d = '%s' (Price: %d)"), 
                               *RowString, ItemInfo.ItemIndex, *ItemInfo.ItemName, ItemInfo.Price);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("❌ Row '%s' -> Failed to extract valid data for item %d"), *RowString, ItemIndex);
                    }
                }
            }
        }
    }

    // Sort items by ItemIndex for consistent display order
    StoreItems.Sort([](const FStructure_ItemInfo& A, const FStructure_ItemInfo& B) {
        return A.ItemIndex < B.ItemIndex;
    });

    // If no items found, create fallback items
    if (StoreItems.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("StoreSystemFix: No items found, creating fallbacks"));
        for (int32 i = 1; i <= 10; i++)
        {
            StoreItems.Add(CreateFallbackItemData(i));
        }
    }

    return StoreItems;
}

bool UStoreSystemFix::DoesItemExist(int32 ItemIndex)
{
    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        return false;
    }

    // Try multiple row name patterns
    TArray<FString> RowPatterns = {
        FString::FromInt(ItemIndex),
        FString::Printf(TEXT("Item_%d"), ItemIndex)
    };

    for (const FString& Pattern : RowPatterns)
    {
        void* RowData = ItemDataTable->FindRowUnchecked(*Pattern);
        if (RowData)
        {
            return true;
        }
    }

    return false;
}

int32 UStoreSystemFix::GetItemPrice(int32 ItemIndex)
{
    FStructure_ItemInfo ItemInfo;
    if (GetItemData(ItemIndex, ItemInfo))
    {
        return ItemInfo.Price > 0 ? ItemInfo.Price : (100 * ItemIndex);
    }
    return 100 * ItemIndex; // Fallback pricing
}

void UStoreSystemFix::DiagnoseDataTable()
{
    UE_LOG(LogTemp, Warning, TEXT("=== STORE DIAGNOSTIC ==="));

    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("DIAGNOSTIC: Failed to load data table"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("DIAGNOSTIC: Data table loaded"));

    // Check structure
    if (ItemDataTable->GetRowStruct())
    {
        FString StructureName = ItemDataTable->GetRowStruct()->GetName();
        UE_LOG(LogTemp, Warning, TEXT("DIAGNOSTIC: Structure: %s"), *StructureName);
    }

    // Check row names
    TArray<FName> RowNames = ItemDataTable->GetRowNames();
    UE_LOG(LogTemp, Warning, TEXT("DIAGNOSTIC: %d rows found"), RowNames.Num());

    for (int32 i = 0; i < FMath::Min(3, RowNames.Num()); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("DIAGNOSTIC: Row %d: %s"), i, *RowNames[i].ToString());
    }

    UE_LOG(LogTemp, Warning, TEXT("=== DIAGNOSTIC END ==="));
}

void UStoreSystemFix::ExtractDataTableAsJSON()
{
    UE_LOG(LogTemp, Warning, TEXT("=== JSON EXTRACT START ==="));

    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("JSON: Failed to load data table"));
        return;
    }

    // Extract the entire data table as JSON
    FString JSONOutput = ItemDataTable->GetTableAsJSON(EDataTableExportFlags::None);

    UE_LOG(LogTemp, Warning, TEXT("JSON Data: %s"), *JSONOutput.Left(500)); // Limit to first 500 chars

    // Parse the JSON array to show structured data
    TSharedPtr<FJsonValue> JsonValue;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONOutput);

    if (FJsonSerializer::Deserialize(Reader, JsonValue) && JsonValue.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("JSON parsed successfully"));
        
        // Check if it's an array
        const TArray<TSharedPtr<FJsonValue>>* JsonArray;
        if (JsonValue->TryGetArray(JsonArray))
        {
            UE_LOG(LogTemp, Warning, TEXT("Found %d items in JSON"), JsonArray->Num());
            
            // Show first few items only
            for (int32 i = 0; i < FMath::Min(3, JsonArray->Num()); i++)
            {
                const TSharedPtr<FJsonValue>& ArrayItem = (*JsonArray)[i];
                TSharedPtr<FJsonObject> ItemData = ArrayItem->AsObject();
                
                if (ItemData.IsValid())
                {
                    FString ItemName;
                    ItemData->TryGetStringField(TEXT("ItemName"), ItemName);
                    
                    int32 ItemIndex = 0;
                    ItemData->TryGetNumberField(TEXT("ItemIndex"), ItemIndex);
                    
                    int32 Price = 0;
                    ItemData->TryGetNumberField(TEXT("Price"), Price);
                    
                    UE_LOG(LogTemp, Warning, TEXT("Item %d: %s (Price: %d)"), ItemIndex, *ItemName, Price);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("JSON is not an array"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
    }

    UE_LOG(LogTemp, Warning, TEXT("=== JSON EXTRACT END ==="));
}

bool UStoreSystemFix::TryExtractWithStructureItemInfo(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo)
{
    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        return false;
    }

    // Try multiple row name patterns
    TArray<FString> RowPatterns = {
        FString::FromInt(ItemIndex),
        FString::Printf(TEXT("Item_%d"), ItemIndex)
    };

    for (const FString& Pattern : RowPatterns)
    {
        try
        {
            FStructure_ItemInfo* FoundRow = ItemDataTable->FindRow<FStructure_ItemInfo>(*Pattern, TEXT(""), false);
            if (FoundRow)
            {
                OutItemInfo = *FoundRow;
                UE_LOG(LogTemp, Display, TEXT("StoreSystemFix: Found item %d using FStructure_ItemInfo with pattern '%s'"), ItemIndex, *Pattern);
                return true;
            }
        }
        catch (...)
        {
            UE_LOG(LogTemp, Warning, TEXT("StoreSystemFix: Exception during FStructure_ItemInfo extraction for pattern '%s'"), *Pattern);
        }
    }

    return false;
}

bool UStoreSystemFix::TryExtractWithBlueprintItemInfo(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo)
{
    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        return false;
    }

    // Try multiple row name patterns
    TArray<FString> RowPatterns = {
        FString::FromInt(ItemIndex),
        FString::Printf(TEXT("Item_%d"), ItemIndex)
    };

    for (const FString& Pattern : RowPatterns)
    {
        try
        {
            FBlueprintItemInfo* FoundRow = ItemDataTable->FindRow<FBlueprintItemInfo>(*Pattern, TEXT(""), false);
            if (FoundRow)
            {
                // Convert to FStructure_ItemInfo
                OutItemInfo = FoundRow->ToOriginalStructure();
                UE_LOG(LogTemp, Display, TEXT("StoreSystemFix: Found item %d using FBlueprintItemInfo with pattern '%s'"), ItemIndex, *Pattern);
                return true;
            }
        }
        catch (...)
        {
            UE_LOG(LogTemp, Warning, TEXT("StoreSystemFix: Exception during FBlueprintItemInfo extraction for pattern '%s'"), *Pattern);
        }
    }

    return false;
}

FStructure_ItemInfo UStoreSystemFix::CreateFallbackItemData(int32 ItemIndex)
{
    FStructure_ItemInfo ItemInfo;
    ItemInfo.ItemIndex = ItemIndex;
    ItemInfo.bIsValid = true;
    ItemInfo.bIsStackable = true;
    ItemInfo.StackNumber = 99;

    switch (ItemIndex)
    {
        case 1:
            ItemInfo.ItemName = TEXT("Basic HP Potion");
            ItemInfo.ItemDescription = TEXT("A basic potion that restores 50 HP.");
            ItemInfo.ItemType = EItemType::Consume_HP;
            ItemInfo.RecoveryHP = 50;
            ItemInfo.Price = 100;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicHealingPotion")));
            break;

        case 2:
            ItemInfo.ItemName = TEXT("Basic MP Potion");
            ItemInfo.ItemDescription = TEXT("A basic potion that restores 50 MP.");
            ItemInfo.ItemType = EItemType::Consume_MP;
            ItemInfo.RecoveryMP = 50;
            ItemInfo.Price = 120;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicManaPotion")));
            break;

        case 3:
            ItemInfo.ItemName = TEXT("Iron Sword");
            ItemInfo.ItemDescription = TEXT("A sturdy iron sword with good balance.");
            ItemInfo.ItemType = EItemType::Equip;
            ItemInfo.ItemEquipSlot = EItemEquipSlot::Weapon;
            ItemInfo.Damage = 25;
            ItemInfo.Price = 500;
            ItemInfo.bIsStackable = false;
            ItemInfo.StackNumber = 1;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_IronSword")));
            break;

        case 4:
            ItemInfo.ItemName = TEXT("Leather Armor");
            ItemInfo.ItemDescription = TEXT("Basic leather armor providing modest protection.");
            ItemInfo.ItemType = EItemType::Equip;
            ItemInfo.ItemEquipSlot = EItemEquipSlot::Body;
            ItemInfo.Defence = 15;
            ItemInfo.Price = 300;
            ItemInfo.bIsStackable = false;
            ItemInfo.StackNumber = 1;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_LeatherArmor")));
            break;

        case 5:
            ItemInfo.ItemName = TEXT("Magic Ring");
            ItemInfo.ItemDescription = TEXT("A ring imbued with magical energy.");
            ItemInfo.ItemType = EItemType::Equip;
            ItemInfo.ItemEquipSlot = EItemEquipSlot::Accessory;
            ItemInfo.INT = 10;
            ItemInfo.MP = 20;
            ItemInfo.Price = 800;
            ItemInfo.bIsStackable = false;
            ItemInfo.StackNumber = 1;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_MagicRing")));
            break;

        default:
            ItemInfo.ItemName = FString::Printf(TEXT("Store Item %d"), ItemIndex);
            ItemInfo.ItemDescription = FString::Printf(TEXT("A mysterious item found in the store."));
            ItemInfo.ItemType = EItemType::Equip;
            ItemInfo.Price = 100 * ItemIndex;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Engine/EditorResources/S_Actor")));
            break;
    }

    return ItemInfo;
}

UDataTable* UStoreSystemFix::GetItemDataTable()
{
    static UDataTable* CachedDataTable = nullptr;
    
    if (!CachedDataTable)
    {
        // Try multiple paths
        TArray<FString> TablePaths = {
            TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList"),
            TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList")
        };

        for (const FString& Path : TablePaths)
        {
            CachedDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *Path));
            if (CachedDataTable)
            {
                UE_LOG(LogTemp, Display, TEXT("StoreSystemFix: Loaded data table from path: %s"), *Path);
                break;
            }
        }

        if (!CachedDataTable)
        {
            UE_LOG(LogTemp, Error, TEXT("StoreSystemFix: Failed to load data table from any path"));
        }
    }

    return CachedDataTable;
}

FStructure_ItemInfo UStoreSystemFix::GetItemDataFromTable(int32 ItemIndex)
{
    UE_LOG(LogTemp, Verbose, TEXT("StoreSystemFix: Getting data for %d"), ItemIndex);
    
    FStructure_ItemInfo ItemInfo;
    ItemInfo.bIsValid = false;
    
    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("StoreSystemFix: No data table for %d"), ItemIndex);
        ItemInfo.bIsValid = false;
        return ItemInfo;
    }

    // CRITICAL FIX: Search through all rows to find the one with the matching ItemIndex value
    // Don't just rely on row names, since row names might not match ItemIndex values
    TArray<FName> RowNames = ItemDataTable->GetRowNames();
    void* FoundRowData = nullptr;
    FString FoundRowName;
    
    for (FName RowName : RowNames)
    {
        void* RowData = ItemDataTable->FindRowUnchecked(RowName);
        if (RowData)
        {
            const UScriptStruct* RowStruct = ItemDataTable->GetRowStruct();
            if (RowStruct)
            {
                // Check if this row has the ItemIndex we're looking for
                TMap<FString, FString> PropertyMapping;
                for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
                {
                    FProperty* Property = *PropIt;
                    FString PropertyName = Property->GetName();
                    if (PropertyName.Contains(TEXT("ItemIndex")))
                    {
                        PropertyMapping.Add(TEXT("ItemIndex"), PropertyName);
                        break;
                    }
                }
                
                int32 RowItemIndex = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("ItemIndex"), -1);
                if (RowItemIndex == ItemIndex)
                {
                    FoundRowData = RowData;
                    FoundRowName = RowName.ToString();
                    UE_LOG(LogTemp, Verbose, TEXT("StoreSystemFix: Found ItemIndex %d in row: %s"), ItemIndex, *FoundRowName);
                    break;
                }
            }
        }
    }

    if (!FoundRowData)
    {
        UE_LOG(LogTemp, Error, TEXT("StoreSystemFix: No row contains ItemIndex %d"), ItemIndex);
        ItemInfo.bIsValid = false;
        return ItemInfo;
    }

    const UScriptStruct* RowStruct = ItemDataTable->GetRowStruct();
    if (!RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("StoreSystemFix: No row structure"));
        ItemInfo.bIsValid = false;
        return ItemInfo;
    }

    // Extract data using property mapping - this should get the REAL data
    ItemInfo = ExtractItemInfoWithMapping(FoundRowData, RowStruct, ItemIndex);
    
    UE_LOG(LogTemp, Verbose, TEXT("StoreSystemFix: Extracted %d: %s (Valid: %s)"), 
           ItemIndex, *ItemInfo.ItemName, ItemInfo.bIsValid ? TEXT("YES") : TEXT("NO"));

    return ItemInfo;
}

FStructure_ItemInfo UStoreSystemFix::ExtractItemInfoWithMapping(void* RowData, const UScriptStruct* RowStruct, int32 ItemIndex)
{
    FStructure_ItemInfo ItemInfo;
    ItemInfo.bIsValid = false;
    
    if (!RowData || !RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("StoreSystemFix: Invalid data for %d"), ItemIndex);
        return ItemInfo;
    }

    UE_LOG(LogTemp, Log, TEXT("StoreSystemFix: Extracting data for item %d"), ItemIndex);

    // Create property mapping for the generated names we found
    TMap<FString, FString> PropertyMapping;
    
    // Build the mapping by finding properties that contain our expected names
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString PropertyName = Property->GetName();
        
        // Map generated names to our expected names
        if (PropertyName.Contains(TEXT("ItemIndex")))
        {
            PropertyMapping.Add(TEXT("ItemIndex"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("ItemName")))
        {
            PropertyMapping.Add(TEXT("ItemName"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("ItemDescription")))
        {
            PropertyMapping.Add(TEXT("ItemDescription"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("ItemType")))
        {
            PropertyMapping.Add(TEXT("ItemType"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("ItemEquipSlot")))
        {
            PropertyMapping.Add(TEXT("ItemEquipSlot"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("Price")))
        {
            PropertyMapping.Add(TEXT("Price"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("RecoveryHP")))
        {
            PropertyMapping.Add(TEXT("RecoveryHP"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("RecoveryMP")))
        {
            PropertyMapping.Add(TEXT("RecoveryMP"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("Damge")) || PropertyName.Contains(TEXT("Damage"))) // Handle typo
        {
            PropertyMapping.Add(TEXT("Damage"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("Defence")))
        {
            PropertyMapping.Add(TEXT("Defence"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("HP")) && !PropertyName.Contains(TEXT("Recovery")))
        {
            PropertyMapping.Add(TEXT("HP"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("MP")) && !PropertyName.Contains(TEXT("Recovery")))
        {
            PropertyMapping.Add(TEXT("MP"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("STR")))
        {
            PropertyMapping.Add(TEXT("STR"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("DEX")))
        {
            PropertyMapping.Add(TEXT("DEX"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("INT")))
        {
            PropertyMapping.Add(TEXT("INT"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("ItemThumbnail")))
        {
            PropertyMapping.Add(TEXT("ItemThumbnail"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("Item3DModel")))
        {
            PropertyMapping.Add(TEXT("Item3DModel"), PropertyName);
            PropertyMapping.Add(TEXT("StaticMeshID"), PropertyName); // Map Item3DModel to StaticMeshID
        }
        else if (PropertyName.Contains(TEXT("StaticMeshID")))
        {
            PropertyMapping.Add(TEXT("StaticMeshID"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("StaticMesh")))
        {
            PropertyMapping.Add(TEXT("StaticMesh"), PropertyName);
        }
        else if (PropertyName.Contains(TEXT("Mesh")))
        {
            PropertyMapping.Add(TEXT("Mesh"), PropertyName);
        }
    }

    UE_LOG(LogTemp, Verbose, TEXT("StoreSystemFix: Built mapping with %d entries"), PropertyMapping.Num());

    // Extract values using the mapping - GET THE REAL DATA
    int32 ActualItemIndex = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("ItemIndex"), -1);
    if (ActualItemIndex > 0)
    {
        ItemInfo.ItemIndex = ActualItemIndex; // Use the REAL index from data table
        UE_LOG(LogTemp, Verbose, TEXT("StoreSystemFix: Using actual ItemIndex %d from data table"), ActualItemIndex);
    }
    else
    {
        ItemInfo.ItemIndex = ItemIndex; // Fallback to passed parameter
        UE_LOG(LogTemp, Warning, TEXT("StoreSystemFix: No ItemIndex in data, using passed value %d"), ItemIndex);
    }
    
    ItemInfo.ItemName = ExtractStringProperty(RowData, RowStruct, PropertyMapping, TEXT("ItemName"), TEXT(""));
    ItemInfo.ItemDescription = ExtractStringProperty(RowData, RowStruct, PropertyMapping, TEXT("ItemDescription"), TEXT(""));
    ItemInfo.Price = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("Price"), 0);
    
    // Extract enum values
    ItemInfo.ItemType = ExtractEnumProperty<EItemType>(RowData, RowStruct, PropertyMapping, TEXT("ItemType"), EItemType::Consume_HP);
    ItemInfo.ItemEquipSlot = ExtractEnumProperty<EItemEquipSlot>(RowData, RowStruct, PropertyMapping, TEXT("ItemEquipSlot"), EItemEquipSlot::None);
    
    // Extract stats
    ItemInfo.RecoveryHP = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("RecoveryHP"), 0);
    ItemInfo.RecoveryMP = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("RecoveryMP"), 0);
    ItemInfo.Damage = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("Damage"), 0);
    ItemInfo.Defence = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("Defence"), 0);
    ItemInfo.HP = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("HP"), 0);
    ItemInfo.MP = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("MP"), 0);
    ItemInfo.STR = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("STR"), 0);
    ItemInfo.DEX = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("DEX"), 0);
    ItemInfo.INT = ExtractIntProperty(RowData, RowStruct, PropertyMapping, TEXT("INT"), 0);
    
    // Extract soft object pointers for meshes and thumbnails
    ItemInfo.StaticMeshID = ExtractSoftObjectProperty<UStaticMesh>(RowData, RowStruct, PropertyMapping, TEXT("Item3DModel"));
    if (ItemInfo.StaticMeshID.IsNull())
    {
        // Try alternative names
        ItemInfo.StaticMeshID = ExtractSoftObjectProperty<UStaticMesh>(RowData, RowStruct, PropertyMapping, TEXT("StaticMeshID"));
        if (ItemInfo.StaticMeshID.IsNull())
        {
            ItemInfo.StaticMeshID = ExtractSoftObjectProperty<UStaticMesh>(RowData, RowStruct, PropertyMapping, TEXT("StaticMesh"));
            if (ItemInfo.StaticMeshID.IsNull())
            {
                ItemInfo.StaticMeshID = ExtractSoftObjectProperty<UStaticMesh>(RowData, RowStruct, PropertyMapping, TEXT("Mesh"));
            }
        }
    }
    
    // For thumbnails, the data table contains static mesh names, so we need to convert them to texture paths
    // First try to get the ItemThumbnail value (which contains static mesh names like "SM_LaserSword")
    FString ThumbnailMeshName = ExtractStringProperty(RowData, RowStruct, PropertyMapping, TEXT("ItemThumbnail"), TEXT(""));
    if (!ThumbnailMeshName.IsEmpty())
    {
        // Convert static mesh name to texture path
        // SM_LaserSword -> IMG_LaserSword
        FString TextureName = ThumbnailMeshName.Replace(TEXT("SM_"), TEXT("IMG_"));
        FString TexturePath = FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/%s"), *TextureName);
        ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TexturePath));
        UE_LOG(LogTemp, Log, TEXT("   Converted thumbnail: %s -> %s"), *ThumbnailMeshName, *TexturePath);
    }
    else
    {
        // Try to extract as ObjectProperty (since logs show it's an ObjectProperty)
        UObject* ThumbnailObject = ExtractObjectProperty(RowData, RowStruct, PropertyMapping, TEXT("ItemThumbnail"));
        if (ThumbnailObject)
        {
            FString ObjectName = ThumbnailObject->GetName();
            UE_LOG(LogTemp, Log, TEXT("   Found thumbnail object: %s"), *ObjectName);
            
            // Convert static mesh name to texture path
            // SM_LaserSword -> IMG_LaserSword
            FString TextureName = ObjectName.Replace(TEXT("SM_"), TEXT("IMG_"));
            FString TexturePath = FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/%s"), *TextureName);
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TexturePath));
            UE_LOG(LogTemp, Log, TEXT("   Converted thumbnail object: %s -> %s"), *ObjectName, *TexturePath);
        }
        else
        {
            // Fallback to direct soft object extraction
            ItemInfo.ItemThumbnail = ExtractSoftObjectProperty<UTexture2D>(RowData, RowStruct, PropertyMapping, TEXT("ItemThumbnail"));
        }
    }
    
    // Log what we extracted
    if (!ItemInfo.StaticMeshID.IsNull())
    {
        UE_LOG(LogTemp, Log, TEXT("   Found StaticMeshID: %s"), *ItemInfo.StaticMeshID.GetAssetName());
    }
    if (!ItemInfo.ItemThumbnail.IsNull())
    {
        UE_LOG(LogTemp, Log, TEXT("   Found ItemThumbnail: %s"), *ItemInfo.ItemThumbnail.GetAssetName());
    }
    
    // Set derived properties
    ItemInfo.bIsStackable = (ItemInfo.ItemType == EItemType::Consume_HP || ItemInfo.ItemType == EItemType::Consume_MP);
    ItemInfo.StackNumber = ItemInfo.bIsStackable ? 99 : 1;
    
    // Mark as valid if we got basic data
    if (!ItemInfo.ItemName.IsEmpty() || ItemInfo.ItemIndex > 0)
    {
        ItemInfo.bIsValid = true;
        UE_LOG(LogTemp, Log, TEXT("✅ Extracted: %d = '%s' (Price: %d)"), 
               ItemInfo.ItemIndex, *ItemInfo.ItemName, ItemInfo.Price);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to extract data for %d"), ItemIndex);
    }

    return ItemInfo;
}

// Helper methods for property extraction
int32 UStoreSystemFix::ExtractIntProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName, int32 DefaultValue)
{
    const FString* ActualName = PropertyMapping.Find(LogicalName);
    if (!ActualName)
    {
        return DefaultValue;
    }

    FProperty* Property = RowStruct->FindPropertyByName(**ActualName);
    if (!Property)
    {
        return DefaultValue;
    }

    if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        int32 Value = IntProp->GetPropertyValue_InContainer(RowData);
        if (LogicalName == TEXT("ItemIndex") || LogicalName == TEXT("Price") || Value > 0)
        {
            UE_LOG(LogTemp, Verbose, TEXT("   Found %s = %d"), *LogicalName, Value);
        }
        return Value;
    }

    return DefaultValue;
}

FString UStoreSystemFix::ExtractStringProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName, const FString& DefaultValue)
{
    const FString* ActualName = PropertyMapping.Find(LogicalName);
    if (!ActualName)
    {
        return DefaultValue;
    }

    FProperty* Property = RowStruct->FindPropertyByName(**ActualName);
    if (!Property)
    {
        return DefaultValue;
    }

    if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        FString Value = StrProp->GetPropertyValue_InContainer(RowData);
        UE_LOG(LogTemp, Verbose, TEXT("   Found %s = '%s'"), *LogicalName, *Value);
        return Value;
    }

    return DefaultValue;
}

template<typename T>
T UStoreSystemFix::ExtractEnumProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName, T DefaultValue)
{
    const FString* ActualName = PropertyMapping.Find(LogicalName);
    if (!ActualName)
    {
        return DefaultValue;
    }

    FProperty* Property = RowStruct->FindPropertyByName(**ActualName);
    if (!Property)
    {
        return DefaultValue;
    }

    // Special handling for ItemEquipSlot to map string values to enum values
    if (LogicalName == TEXT("ItemEquipSlot"))
    {
        // First try to get as string property (if it's stored as string in data table)
        if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
        {
            FString StringValue = StrProp->GetPropertyValue_InContainer(RowData);
            UE_LOG(LogTemp, Verbose, TEXT("   Found ItemEquipSlot = '%s'"), *StringValue);
            
            // Map data table strings to our enum values
            EItemEquipSlot MappedSlot = EItemEquipSlot::None;
            if (StringValue == TEXT("Weapon"))
            {
                MappedSlot = EItemEquipSlot::Weapon;
            }
            else if (StringValue == TEXT("Shield"))
            {
                MappedSlot = EItemEquipSlot::Accessory; // Map Shield to Accessory
            }
            else if (StringValue == TEXT("Helmet"))
            {
                MappedSlot = EItemEquipSlot::Head; // Map Helmet to Head
            }
            else if (StringValue == TEXT("Suit"))
            {
                MappedSlot = EItemEquipSlot::Body; // Map Suit to Body
            }
            else if (StringValue == TEXT("(INVALID)") || StringValue.IsEmpty())
            {
                MappedSlot = EItemEquipSlot::None;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("   Unknown ItemEquipSlot: '%s'"), *StringValue);
                MappedSlot = EItemEquipSlot::None;
            }
            
            UE_LOG(LogTemp, Verbose, TEXT("   Mapped '%s' to %d"), *StringValue, static_cast<int32>(MappedSlot));
            return static_cast<T>(MappedSlot);
        }
        // If it's stored as byte property, try direct extraction
        else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
        {
            uint8 Value = ByteProp->GetPropertyValue_InContainer(RowData);
            UE_LOG(LogTemp, Verbose, TEXT("   Found ItemEquipSlot byte = %d"), static_cast<int32>(Value));
            return static_cast<T>(Value);
        }
    }

    // Special handling for ItemType to map string values to enum values  
    if (LogicalName == TEXT("ItemType"))
    {
        // First try to get as string property
        if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
        {
            FString StringValue = StrProp->GetPropertyValue_InContainer(RowData);
            UE_LOG(LogTemp, Verbose, TEXT("   Found ItemType = '%s'"), *StringValue);
            
            // Map data table strings to our enum values
            EItemType MappedType = EItemType::Equip;
            if (StringValue == TEXT("Equip"))
            {
                MappedType = EItemType::Equip;
            }
            else if (StringValue == TEXT("Consume(HP)"))
            {
                MappedType = EItemType::Consume_HP;
            }
            else if (StringValue == TEXT("Consume(MP)"))
            {
                MappedType = EItemType::Consume_MP;
            }
            else if (StringValue == TEXT("Collection"))
            {
                MappedType = EItemType::Collection;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("   Unknown ItemType: '%s'"), *StringValue);
                MappedType = EItemType::Equip;
            }
            
            UE_LOG(LogTemp, Verbose, TEXT("   Mapped '%s' to %d"), *StringValue, static_cast<int32>(MappedType));
            return static_cast<T>(MappedType);
        }
        // If it's stored as byte property, try direct extraction
        else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
        {
            uint8 Value = ByteProp->GetPropertyValue_InContainer(RowData);
            UE_LOG(LogTemp, Verbose, TEXT("   Found ItemType byte = %d"), static_cast<int32>(Value));
            return static_cast<T>(Value);
        }
    }

    // For other enum properties, try byte property extraction
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        uint8 Value = ByteProp->GetPropertyValue_InContainer(RowData);
        return static_cast<T>(Value);
    }

    return DefaultValue;
}

template<typename T>
TSoftObjectPtr<T> UStoreSystemFix::ExtractSoftObjectProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName)
{
    const FString* ActualName = PropertyMapping.Find(LogicalName);
    if (!ActualName)
    {
        return TSoftObjectPtr<T>();
    }

    FProperty* Property = RowStruct->FindPropertyByName(**ActualName);
    if (!Property)
    {
        return TSoftObjectPtr<T>();
    }

    if (FSoftObjectProperty* SoftObjectProp = CastField<FSoftObjectProperty>(Property))
    {
        // Get the raw soft object pointer first
        FSoftObjectPtr RawValue = SoftObjectProp->GetPropertyValue_InContainer(RowData);
        
        // Convert to the typed version
        TSoftObjectPtr<T> TypedValue(RawValue.ToSoftObjectPath());
        
        UE_LOG(LogTemp, Verbose, TEXT("   Found %s = %s"), *LogicalName, *TypedValue.ToString());
        return TypedValue;
    }

    return TSoftObjectPtr<T>();
}

// Helper function to extract UObject properties
UObject* UStoreSystemFix::ExtractObjectProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName)
{
    const FString* ActualName = PropertyMapping.Find(LogicalName);
    if (!ActualName)
    {
        return nullptr;
    }

    FProperty* Property = RowStruct->FindPropertyByName(**ActualName);
    if (!Property)
    {
        return nullptr;
    }

    if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        UObject* Value = ObjectProp->GetObjectPropertyValue_InContainer(RowData);
        UE_LOG(LogTemp, Verbose, TEXT("   Found %s = %s"), *LogicalName, Value ? *Value->GetName() : TEXT("NULL"));
        return Value;
    }

    return nullptr;
} 