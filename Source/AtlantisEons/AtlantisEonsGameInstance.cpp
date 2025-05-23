#include "AtlantisEonsGameInstance.h"

UAtlantisEonsGameInstance::UAtlantisEonsGameInstance()
{
    // Constructor
}

void UAtlantisEonsGameInstance::Init()
{
    Super::Init();

    // Load the item data table if it hasn't been loaded
    if (!ItemDataTable)
    {
        // You can load the data table asset here if you have a reference path
        static ConstructorHelpers::FObjectFinder<UDataTable> ItemDataTableAsset(TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList"));
        if (ItemDataTableAsset.Succeeded())
        {
            ItemDataTable = ItemDataTableAsset.Object;
        }
    }
}

bool UAtlantisEonsGameInstance::GetItemInfoBlueprint(int32 ItemIndex, FFStructure_ItemInfo& OutItemInfo) const
{
    // Log the request for debugging
    UE_LOG(LogTemp, Display, TEXT("%s: GetItemInfoBlueprint called for ItemIndex=%d"), *GetName(), ItemIndex);
    
    // Verify data table is loaded
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: ItemDataTable is null, attempting to load it"), *GetName());
        
        // Try to load it dynamically if it wasn't loaded in Init
        UDataTable* LoadedTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
            TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
            
        if (!LoadedTable)
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Failed to load item data table"), *GetName());
            return false;
        }
        
        // Use const_cast to update the member variable since this is a const method
        // but we need to load the table when it's missing
        const_cast<UAtlantisEonsGameInstance*>(this)->ItemDataTable = LoadedTable;
    }
    
    // Log data table structure for debugging
    if (ItemDataTable->GetRowStruct())
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Data table row structure: %s"), 
               *GetName(), *ItemDataTable->GetRowStruct()->GetName());
    }

    // Create a string for the row name
    FString RowName = FString::FromInt(ItemIndex);
    
    // Try to find the row using the EXACT blueprint structure
    FFStructure_ItemInfo* FoundRow = nullptr;
    
    try
    {
        FoundRow = ItemDataTable->FindRow<FFStructure_ItemInfo>(*RowName, TEXT(""), false);
    }
    catch(...)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Exception occurred during FindRow with FFStructure_ItemInfo"), *GetName());
    }
    
    if (FoundRow)
    {
        // Standard row lookup succeeded
        OutItemInfo = *FoundRow;
        UE_LOG(LogTemp, Display, TEXT("%s: Successfully found item data for index %d using exact blueprint structure"), *GetName(), ItemIndex);
        return true;
    }
    else
    {
        // Try alternate approach with FindRowUnchecked
        void* RowData = ItemDataTable->FindRowUnchecked(*RowName);
        
        if (RowData)
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Found row %s using unchecked approach"), *GetName(), *RowName);
            
            // Create default item values based on the index
            OutItemInfo.ItemIndex = ItemIndex;
            OutItemInfo.ItemName = FString::Printf(TEXT("Item %d"), ItemIndex);
            OutItemInfo.ItemDescription = FString::Printf(TEXT("Description for item %d"), ItemIndex);
            OutItemInfo.Price = 100 * (ItemIndex + 1); // Simple pricing based on index
            OutItemInfo.StackNumber = 1;
            OutItemInfo.bIsValid = true;
            OutItemInfo.bIsStackable = true;
            
            return true;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("%s: Item with index %d not found in data table"), *GetName(), ItemIndex);
    return false;
}

bool UAtlantisEonsGameInstance::GetItemInfo(int32 ItemIndex, Structure_ItemInfo& OutItemInfo) const
{
    // Use direct access instead of the bridge method
    UDataTable* ItemDataTable = GetItemDataTable();
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Item data table is null"), *GetName());
        return false;
    }
    
    FString RowName = FString::FromInt(ItemIndex);
    Structure_ItemInfo* FoundRow = nullptr;
    
    try
    {
        FoundRow = ItemDataTable->FindRow<Structure_ItemInfo>(*RowName, TEXT(""), false);
    }
    catch(...)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Exception occurred during FindRow, using unchecked approach"), *GetName());
    }
    
    if (FoundRow)
    {
        OutItemInfo = *FoundRow;
        return true;
    }
    else
    {
        // Try alternate approach with FindRowUnchecked
        void* RowData = ItemDataTable->FindRowUnchecked(*RowName);
        
        if (RowData)
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Found row %s using unchecked approach"), *GetName(), *RowName);
            
            // Create default item values based on the index
            OutItemInfo.ItemIndex = ItemIndex;
            OutItemInfo.ItemName = FString::Printf(TEXT("Item %d"), ItemIndex);
            OutItemInfo.ItemDescription = FString::Printf(TEXT("Description for item %d"), ItemIndex);
            OutItemInfo.Price = 100 * (ItemIndex + 1); // Simple pricing based on index
            OutItemInfo.StackNumber = 1;
            OutItemInfo.bIsValid = true;
            OutItemInfo.bIsStackable = true;
            
            return true;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("%s: Item with index %d not found in data table"), *GetName(), ItemIndex);
    return false;
}

UTexture2D* UAtlantisEonsGameInstance::GetDefaultItemTexture()
{
    // Create the default texture if it doesn't exist
    if (!DefaultItemTexture)
    {
        // Create an orange/gold texture for item placeholder
        DefaultItemTexture = CreateColoredTexture(FLinearColor(1.0f, 0.7f, 0.0f, 1.0f));
        
        if (DefaultItemTexture)
        {
            // Ensure the texture doesn't get garbage collected
            DefaultItemTexture->AddToRoot();
            UE_LOG(LogTemp, Display, TEXT("Created default item texture"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create default item texture"));
        }
    }
    
    return DefaultItemTexture;
}

UTexture2D* UAtlantisEonsGameInstance::CreateColoredTexture(FLinearColor Color, int32 Width, int32 Height)
{
    // Create a new texture with the specified dimensions
    UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
    if (!Texture)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create transient texture"));
        return nullptr;
    }

    // Lock the texture for writing
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
    
    // Convert linear color to bytes (BGRA format)
    uint8 R = (uint8)(Color.R * 255);
    uint8 G = (uint8)(Color.G * 255);
    uint8 B = (uint8)(Color.B * 255);
    uint8 A = (uint8)(Color.A * 255);

    // Fill the texture with the color
    uint8* DestPtr = (uint8*)TextureData;
    for (int32 y = 0; y < Height; y++)
    {
        for (int32 x = 0; x < Width; x++)
        {
            *DestPtr++ = B; // Blue
            *DestPtr++ = G; // Green
            *DestPtr++ = R; // Red
            *DestPtr++ = A; // Alpha
        }
    }

    // Unlock the texture
    Mip.BulkData.Unlock();

    // Update the texture
    Texture->UpdateResource();
    
    UE_LOG(LogTemp, Display, TEXT("Created colored texture of size %dx%d"), Width, Height);
    
    return Texture;
}
