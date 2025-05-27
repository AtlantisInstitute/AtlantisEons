#include "UniversalDataTableReader.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"

bool UUniversalDataTableReader::ReadItemData(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo)
{
    // Load the data table
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("UniversalReader: Failed to load item data table"));
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
        if (ReadItemDataByRowName(RowPattern, OutItemInfo))
        {
            UE_LOG(LogTemp, Log, TEXT("UniversalReader: Successfully read item %d using pattern '%s'"), 
                   ItemIndex, *RowPattern);
            return true;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UniversalReader: Item %d not found in data table"), ItemIndex);
    return false;
}

bool UUniversalDataTableReader::ReadItemDataByRowName(const FString& RowName, FStructure_ItemInfo& OutItemInfo)
{
    // Load the data table
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (!ItemDataTable)
    {
        return false;
    }
    
    // Get raw row data
    void* RowData = ItemDataTable->FindRowUnchecked(*RowName);
    if (!RowData)
    {
        return false;
    }

    const UScriptStruct* RowStruct = ItemDataTable->GetRowStruct();
    if (!RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("UniversalReader: No row structure found"));
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("UniversalReader: Reading row with structure"));
    
    return ExtractItemDataFromRow(RowData, const_cast<UScriptStruct*>(RowStruct), OutItemInfo);
}

TArray<int32> UUniversalDataTableReader::GetAllItemIndices()
{
    TArray<int32> ItemIndices;
    
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (!ItemDataTable)
    {
        return ItemIndices;
    }
    
    TArray<FName> RowNames = ItemDataTable->GetRowNames();
    for (FName RowName : RowNames)
    {
        FString RowString = RowName.ToString();
        
        // Try to extract index from row name
        int32 Index = -1;
        if (RowString.StartsWith(TEXT("Item_")))
        {
            FString IndexString = RowString.RightChop(5);
            Index = FCString::Atoi(*IndexString);
        }
        else if (RowString.IsNumeric())
        {
            Index = FCString::Atoi(*RowString);
        }
        
        if (Index >= 0)
        {
            ItemIndices.Add(Index);
        }
    }
    
    return ItemIndices;
}

bool UUniversalDataTableReader::DoesItemExist(int32 ItemIndex)
{
    FStructure_ItemInfo DummyItemInfo;
    return ReadItemData(ItemIndex, DummyItemInfo);
}

bool UUniversalDataTableReader::ExtractItemDataFromRow(void* RowData, UScriptStruct* RowStruct, FStructure_ItemInfo& OutItemInfo)
{
    // Initialize with defaults
    OutItemInfo = FStructure_ItemInfo();
    
    // Extract properties using reflection
    bool bFoundAnyData = false;
    
    // Try to get ItemIndex
    int32 ItemIndex = -1;
    if (GetIntProperty(RowData, RowStruct, TEXT("ItemIndex"), ItemIndex))
    {
        OutItemInfo.ItemIndex = ItemIndex;
        bFoundAnyData = true;
        UE_LOG(LogTemp, Log, TEXT("UniversalReader: Found ItemIndex: %d"), ItemIndex);
    }
    
    // Try to get ItemName
    FString ItemName;
    if (GetStringProperty(RowData, RowStruct, TEXT("ItemName"), ItemName))
    {
        OutItemInfo.ItemName = ItemName;
        bFoundAnyData = true;
        UE_LOG(LogTemp, Log, TEXT("UniversalReader: Found ItemName: %s"), *ItemName);
    }
    
    // Try to get ItemDescription
    FString ItemDescription;
    if (GetStringProperty(RowData, RowStruct, TEXT("ItemDescription"), ItemDescription))
    {
        OutItemInfo.ItemDescription = ItemDescription;
        bFoundAnyData = true;
    }
    
    // Try to get Price
    int32 Price = 0;
    if (GetIntProperty(RowData, RowStruct, TEXT("Price"), Price))
    {
        OutItemInfo.Price = Price;
        bFoundAnyData = true;
        UE_LOG(LogTemp, Log, TEXT("UniversalReader: Found Price: %d"), Price);
    }
    
    // Try to get bIsValid
    bool bIsValid = false;
    if (GetBoolProperty(RowData, RowStruct, TEXT("bIsValid"), bIsValid))
    {
        OutItemInfo.bIsValid = bIsValid;
        bFoundAnyData = true;
    }
    else
    {
        OutItemInfo.bIsValid = true; // Default to valid if not specified
    }
    
    // Try to get bIsStackable
    bool bIsStackable = false;
    if (GetBoolProperty(RowData, RowStruct, TEXT("bIsStackable"), bIsStackable))
    {
        OutItemInfo.bIsStackable = bIsStackable;
        bFoundAnyData = true;
    }
    
    // Try to get StackNumber
    int32 StackNumber = 1;
    if (GetIntProperty(RowData, RowStruct, TEXT("StackNumber"), StackNumber))
    {
        OutItemInfo.StackNumber = StackNumber;
        bFoundAnyData = true;
    }
    
    // Try to get ItemType (as enum)
    if (GetEnumProperty(RowData, RowStruct, TEXT("ItemType"), OutItemInfo.ItemType))
    {
        bFoundAnyData = true;
        UE_LOG(LogTemp, Log, TEXT("UniversalReader: Found ItemType: %d"), (int32)OutItemInfo.ItemType);
    }
    
    // Try to get ItemEquipSlot (as enum)
    if (GetEnumProperty(RowData, RowStruct, TEXT("ItemEquipSlot"), OutItemInfo.ItemEquipSlot))
    {
        bFoundAnyData = true;
        UE_LOG(LogTemp, Log, TEXT("UniversalReader: Found ItemEquipSlot: %d"), (int32)OutItemInfo.ItemEquipSlot);
    }
    
    // Try to get stat values
    GetIntProperty(RowData, RowStruct, TEXT("RecoveryHP"), OutItemInfo.RecoveryHP);
    GetIntProperty(RowData, RowStruct, TEXT("RecoveryMP"), OutItemInfo.RecoveryMP);
    GetIntProperty(RowData, RowStruct, TEXT("Damage"), OutItemInfo.Damage);
    GetIntProperty(RowData, RowStruct, TEXT("Defence"), OutItemInfo.Defence);
    GetIntProperty(RowData, RowStruct, TEXT("HP"), OutItemInfo.HP);
    GetIntProperty(RowData, RowStruct, TEXT("MP"), OutItemInfo.MP);
    GetIntProperty(RowData, RowStruct, TEXT("STR"), OutItemInfo.STR);
    GetIntProperty(RowData, RowStruct, TEXT("DEX"), OutItemInfo.DEX);
    GetIntProperty(RowData, RowStruct, TEXT("INT"), OutItemInfo.INT);
    
    // Try to get soft object references
    GetSoftObjectProperty(RowData, RowStruct, TEXT("ItemThumbnail"), OutItemInfo.ItemThumbnail);
    GetSoftObjectProperty(RowData, RowStruct, TEXT("StaticMeshID"), OutItemInfo.StaticMeshID);
    
    if (bFoundAnyData)
    {
        UE_LOG(LogTemp, Log, TEXT("UniversalReader: Successfully extracted item data for '%s'"), 
               *OutItemInfo.ItemName);
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UniversalReader: No compatible data found in row"));
    return false;
}

bool UUniversalDataTableReader::GetStringProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, FString& OutValue)
{
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->GetName() == PropertyName)
        {
            if (FStrProperty* StringProp = CastField<FStrProperty>(Property))
            {
                OutValue = StringProp->GetPropertyValue_InContainer(RowData);
                return true;
            }
        }
    }
    return false;
}

bool UUniversalDataTableReader::GetIntProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, int32& OutValue)
{
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->GetName() == PropertyName)
        {
            if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
            {
                OutValue = IntProp->GetPropertyValue_InContainer(RowData);
                return true;
            }
        }
    }
    return false;
}

bool UUniversalDataTableReader::GetBoolProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, bool& OutValue)
{
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->GetName() == PropertyName)
        {
            if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
            {
                OutValue = BoolProp->GetPropertyValue_InContainer(RowData);
                return true;
            }
        }
    }
    return false;
}

template<typename TEnum>
bool UUniversalDataTableReader::GetEnumProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, TEnum& OutValue)
{
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->GetName() == PropertyName)
        {
            if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
            {
                int64 EnumValue = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(RowData));
                OutValue = static_cast<TEnum>(EnumValue);
                return true;
            }
            else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
            {
                if (ByteProp->Enum)
                {
                    uint8 ByteValue = ByteProp->GetPropertyValue_InContainer(RowData);
                    OutValue = static_cast<TEnum>(ByteValue);
                    return true;
                }
            }
        }
    }
    return false;
}

template<typename T>
bool UUniversalDataTableReader::GetSoftObjectProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, TSoftObjectPtr<T>& OutValue)
{
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->GetName() == PropertyName)
        {
            if (FSoftObjectProperty* SoftObjectProp = CastField<FSoftObjectProperty>(Property))
            {
                FSoftObjectPtr* SoftObjectPtr = SoftObjectProp->GetPropertyValuePtr_InContainer(RowData);
                if (SoftObjectPtr)
                {
                    OutValue = TSoftObjectPtr<T>(SoftObjectPtr->GetUniqueID());
                    return true;
                }
            }
        }
    }
    return false;
}

// Explicit template instantiations
template bool UUniversalDataTableReader::GetEnumProperty<EItemType>(void*, UScriptStruct*, const FString&, EItemType&);
template bool UUniversalDataTableReader::GetEnumProperty<EItemEquipSlot>(void*, UScriptStruct*, const FString&, EItemEquipSlot&);
template bool UUniversalDataTableReader::GetSoftObjectProperty<UTexture2D>(void*, UScriptStruct*, const FString&, TSoftObjectPtr<UTexture2D>&);
template bool UUniversalDataTableReader::GetSoftObjectProperty<UStaticMesh>(void*, UScriptStruct*, const FString&, TSoftObjectPtr<UStaticMesh>&); 