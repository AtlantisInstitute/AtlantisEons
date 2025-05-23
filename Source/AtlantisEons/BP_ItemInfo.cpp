#include "BP_ItemInfo.h"
#include "BlueprintItemTypes.h"

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
    ItemEquipSlot = EItemEquipSlot::None;
    RecoveryHP = 0;
    RecoveryMP = 0;

    // Load the default item data table
    static ConstructorHelpers::FObjectFinder<UDataTable> ItemDataTableObj(TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList"));
    if (ItemDataTableObj.Succeeded())
    {
        ItemDataTable = ItemDataTableObj.Object;
        if (ItemDataTable)
        {
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
            UE_LOG(LogTemp, Error, TEXT("ItemDataTable is null after loading"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load item data table at path: /Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList"));
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

    // Create the row name with proper format and ensure it's not empty
    FString RowName = FString::Printf(TEXT("Item_%d"), FMath::Max(0, ItemIndex));
    
    // Get the row structure with proper context
    static const FString ContextString(TEXT("UBP_ItemInfo::GetItemTableRow"));
    FStructure_ItemInfo* Row = ItemDataTable->FindRow<FStructure_ItemInfo>(*RowName, ContextString, true);
    
    if (Row)
    {
        ItemInfoStructure = *Row;
        Find = true;
        UE_LOG(LogTemp, Display, TEXT("Successfully found item data for index %d in data table"), ItemIndex);
        return;
    }
    
    // If not found in data table, use hardcoded data
    ItemInfoStructure = CreateHardcodedItemData(ItemIndex);
    Find = true;
    UE_LOG(LogTemp, Warning, TEXT("Using hardcoded data for item index: %d"), ItemIndex);
}

UDataTable* UBP_ItemInfo::GetItemDataTable() const
{
    return ItemDataTable;
}

void UBP_ItemInfo::SetItemDataTable(UDataTable* NewDataTable)
{
    ItemDataTable = NewDataTable;
}
