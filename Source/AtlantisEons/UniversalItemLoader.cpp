#include "UniversalItemLoader.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"
#include "StoreSystemFix.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

// Initialize static member
UDataTable* UUniversalItemLoader::CachedDataTable = nullptr;

bool UUniversalItemLoader::LoadItemData(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo)
{
    // Use the established StoreSystemFix method which handles JSON conversion and property mapping
    return UStoreSystemFix::GetItemData(ItemIndex, OutItemInfo);
}

UTexture2D* UUniversalItemLoader::LoadItemTexture(const FStructure_ItemInfo& ItemInfo)
{
    // First try the data table soft object pointer
    if (!ItemInfo.ItemThumbnail.IsNull())
    {
        UTexture2D* LoadedTexture = ItemInfo.ItemThumbnail.LoadSynchronous();
        if (LoadedTexture)
        {
            UE_LOG(LogTemp, Log, TEXT("UniversalItemLoader: ✅ Loaded texture from data table for %s"), *ItemInfo.ItemName);
            return LoadedTexture;
        }
    }

    // CRITICAL: Use the exact texture names from the data table first
    TArray<FString> ExactTexturePaths = GetExactTexturePathsFromDataTable(ItemInfo.ItemIndex);
    
    // Try exact paths from data table first
    for (const FString& ExactPath : ExactTexturePaths)
    {
        UTexture2D* LoadedTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *ExactPath));
        if (LoadedTexture)
        {
            UE_LOG(LogTemp, Log, TEXT("UniversalItemLoader: ✅ Loaded texture from exact data table path: %s"), *ExactPath);
            return LoadedTexture;
        }
    }

    // Generate intelligent path variations as fallback
    TArray<FString> TexturePaths = GenerateTexturePaths(ItemInfo.ItemName, ItemInfo.ItemIndex);
    
    // Try to load from generated paths
    UTexture2D* LoadedTexture = LoadAssetFromPaths<UTexture2D>(TexturePaths);
    if (LoadedTexture)
    {
        UE_LOG(LogTemp, Log, TEXT("UniversalItemLoader: ✅ Loaded texture from generated path for %s"), *ItemInfo.ItemName);
        return LoadedTexture;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UniversalItemLoader: ❌ No texture found for %s"), *ItemInfo.ItemName);
    
    // Return default texture
    return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Engine/EditorResources/S_Actor")));
}

UStaticMesh* UUniversalItemLoader::LoadItemMesh(const FStructure_ItemInfo& ItemInfo)
{
    // First try the data table soft object pointer
    if (!ItemInfo.StaticMeshID.IsNull())
    {
        UStaticMesh* LoadedMesh = ItemInfo.StaticMeshID.LoadSynchronous();
        if (LoadedMesh)
        {
            return LoadedMesh;
        }
    }

    // Generate intelligent path variations
    TArray<FString> MeshPaths = GenerateMeshPaths(ItemInfo.ItemName, ItemInfo.ItemIndex);
    
    // Try to load from generated paths
    UStaticMesh* LoadedMesh = LoadAssetFromPaths<UStaticMesh>(MeshPaths);
    if (LoadedMesh)
    {
        return LoadedMesh;
    }

    return nullptr;
}

TArray<FStructure_ItemInfo> UUniversalItemLoader::GetAllItems()
{
    // Use the established StoreSystemFix method which handles JSON conversion and property mapping
    return UStoreSystemFix::GetAllStoreItems();
}

bool UUniversalItemLoader::DoesItemExist(int32 ItemIndex)
{
    // Use the established StoreSystemFix method
    return UStoreSystemFix::DoesItemExist(ItemIndex);
}

int32 UUniversalItemLoader::GetMaxItemIndex()
{
    TArray<FStructure_ItemInfo> AllItems = GetAllItems();
    int32 MaxIndex = 0;
    
    for (const FStructure_ItemInfo& Item : AllItems)
    {
        if (Item.ItemIndex > MaxIndex)
        {
            MaxIndex = Item.ItemIndex;
        }
    }
    
    return MaxIndex;
}

TArray<FString> UUniversalItemLoader::GetExactTexturePathsFromDataTable(int32 ItemIndex)
{
    TArray<FString> ExactPaths;
    
    // Get the item data to extract the exact texture name
    FStructure_ItemInfo ItemInfo;
    if (UStoreSystemFix::GetItemData(ItemIndex, ItemInfo))
    {
        // The StoreSystemFix extracts the exact texture object name from the data table
        // We need to get this exact name and build the full path
        
        // Use StoreSystemFix to get the raw data table row and extract the exact texture name
        UDataTable* DataTable = UStoreSystemFix::GetItemDataTable();
        if (DataTable)
        {
            // Get the row name for this item index
            FString RowName = FString::Printf(TEXT("%d"), ItemIndex);
            
            // Find the row
            uint8* RowData = DataTable->FindRowUnchecked(*RowName);
            if (RowData)
            {
                const UScriptStruct* RowStruct = DataTable->GetRowStruct();
                if (RowStruct)
                {
                    // Look for thumbnail object property
                    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
                    {
                        FProperty* Property = *PropIt;
                        FString PropertyName = Property->GetName();
                        
                        if (PropertyName.Contains(TEXT("ItemThumbnail")) || PropertyName.Contains(TEXT("Thumbnail")))
                        {
                            if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
                            {
                                const FSoftObjectPtr* SoftTexturePtr = SoftObjProp->ContainerPtrToValuePtr<FSoftObjectPtr>(RowData);
                                if (SoftTexturePtr && !SoftTexturePtr->IsNull())
                                {
                                    FString TexturePath = SoftTexturePtr->ToSoftObjectPath().ToString();
                                    if (!TexturePath.IsEmpty())
                                    {
                                        ExactPaths.Add(TexturePath);
                                        UE_LOG(LogTemp, Log, TEXT("UniversalItemLoader: Found exact texture path for item %d"), ItemIndex);
                                    }
                                }
                            }
                            // Also check for object property that might contain the texture name
                            else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
                            {
                                UObject* const* ObjectPtr = ObjProp->ContainerPtrToValuePtr<UObject*>(RowData);
                                if (ObjectPtr && *ObjectPtr)
                                {
                                    FString ObjectName = (*ObjectPtr)->GetName();
                                    FString TexturePath = GetTextureBasePath() + ObjectName;
                                    ExactPaths.Add(TexturePath);
                                    UE_LOG(LogTemp, Log, TEXT("UniversalItemLoader: Found exact texture name for item %d"), ItemIndex);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return ExactPaths;
}

TArray<FString> UUniversalItemLoader::GenerateTexturePaths(const FString& ItemName, int32 ItemIndex)
{
    TArray<FString> Paths;
    FString BasePath = GetTextureBasePath();
    FString CleanName = CleanItemName(ItemName);
    
    // Primary patterns based on item name
    Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *CleanName));
    Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *ItemName.Replace(TEXT(" "), TEXT("-"))));
    Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *ItemName.Replace(TEXT(" "), TEXT("_"))));
    
    // Common suffix patterns
    Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s_105"), *CleanName));
    Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s_01"), *CleanName));
    Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s_1"), *CleanName));
    
    // Handle common word replacements
    if (ItemName.Contains(TEXT("HP")) || ItemName.Contains(TEXT("Health")))
    {
        FString HealingName = CleanName.Replace(TEXT("HP"), TEXT("Healing"));
        Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *HealingName));
        // Handle common typos
        FString TypoName = HealingName.Replace(TEXT("Potion"), TEXT("Ption"));
        Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *TypoName));
    }
    
    if (ItemName.Contains(TEXT("MP")) || ItemName.Contains(TEXT("Mana")))
    {
        FString ManaName = CleanName.Replace(TEXT("MP"), TEXT("Mana"));
        Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *ManaName));
        // Handle common typos
        FString TypoName = ManaName.Replace(TEXT("Potion"), TEXT("Ption"));
        Paths.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *TypoName));
    }
    
    // Index-based fallbacks
    Paths.Add(BasePath + FString::Printf(TEXT("IMG_Item_%d"), ItemIndex));
    Paths.Add(BasePath + FString::Printf(TEXT("Item_%d"), ItemIndex));
    Paths.Add(BasePath + FString::Printf(TEXT("IMG_%d"), ItemIndex));
    
    // Generic patterns
    Paths.Add(BasePath + CleanName);
    Paths.Add(BasePath + ItemName);
    
    return Paths;
}

TArray<FString> UUniversalItemLoader::GenerateMeshPaths(const FString& ItemName, int32 ItemIndex)
{
    TArray<FString> Paths;
    FString BasePath = GetMeshBasePath();
    FString CleanName = CleanItemName(ItemName);
    
    // Primary patterns based on item name
    Paths.Add(BasePath + FString::Printf(TEXT("SM_%s"), *CleanName));
    Paths.Add(BasePath + FString::Printf(TEXT("SM_%s"), *ItemName.Replace(TEXT(" "), TEXT("-"))));
    Paths.Add(BasePath + FString::Printf(TEXT("SM_%s"), *ItemName.Replace(TEXT(" "), TEXT("_"))));
    
    // Common suffix patterns
    Paths.Add(BasePath + FString::Printf(TEXT("SM_%s_105"), *CleanName));
    Paths.Add(BasePath + FString::Printf(TEXT("SM_%s_01"), *CleanName));
    Paths.Add(BasePath + FString::Printf(TEXT("SM_%s_1"), *CleanName));
    
    // Index-based fallbacks
    Paths.Add(BasePath + FString::Printf(TEXT("SM_Item_%d"), ItemIndex));
    Paths.Add(BasePath + FString::Printf(TEXT("Item_%d"), ItemIndex));
    Paths.Add(BasePath + FString::Printf(TEXT("SM_%d"), ItemIndex));
    
    // Generic patterns
    Paths.Add(BasePath + CleanName);
    Paths.Add(BasePath + ItemName);
    
    return Paths;
}

void UUniversalItemLoader::ValidateAllItems()
{
    UE_LOG(LogTemp, Warning, TEXT("=== UNIVERSAL ITEM LOADER VALIDATION ==="));
    
    TArray<FStructure_ItemInfo> AllItems = GetAllItems();
    int32 ValidTextures = 0;
    int32 ValidMeshes = 0;
    int32 TotalItems = AllItems.Num();
    
    for (const FStructure_ItemInfo& Item : AllItems)
    {
        UE_LOG(LogTemp, Log, TEXT("Validating Item %d: %s"), Item.ItemIndex, *Item.ItemName);
        
        // Check texture
        UTexture2D* Texture = LoadItemTexture(Item);
        if (Texture && Texture->GetName() != TEXT("S_Actor"))
        {
            ValidTextures++;
            UE_LOG(LogTemp, Log, TEXT("  ✅ Texture: %s"), *Texture->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("  ❌ No texture found"));
        }
        
        // Check mesh
        UStaticMesh* Mesh = LoadItemMesh(Item);
        if (Mesh)
        {
            ValidMeshes++;
            UE_LOG(LogTemp, Log, TEXT("  ✅ Mesh: %s"), *Mesh->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("  ❌ No mesh found"));
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Validation Results:"));
    UE_LOG(LogTemp, Warning, TEXT("  Total Items: %d"), TotalItems);
    UE_LOG(LogTemp, Warning, TEXT("  Valid Textures: %d/%d (%.1f%%)"), ValidTextures, TotalItems, TotalItems > 0 ? (float)ValidTextures / TotalItems * 100.0f : 0.0f);
    UE_LOG(LogTemp, Warning, TEXT("  Valid Meshes: %d/%d (%.1f%%)"), ValidMeshes, TotalItems, TotalItems > 0 ? (float)ValidMeshes / TotalItems * 100.0f : 0.0f);
    UE_LOG(LogTemp, Warning, TEXT("=== VALIDATION COMPLETE ==="));
}

UDataTable* UUniversalItemLoader::GetItemDataTable()
{
    // Use the established StoreSystemFix method to get the data table
    // This ensures consistency with the rest of the system
    return UStoreSystemFix::GetItemDataTable();
}

bool UUniversalItemLoader::ExtractItemDataFromRow(void* RowData, UScriptStruct* RowStruct, FStructure_ItemInfo& OutItemInfo)
{
    if (!RowData || !RowStruct)
    {
        return false;
    }

    // Initialize with defaults
    OutItemInfo = FStructure_ItemInfo();
    OutItemInfo.bIsValid = true;

    // Extract properties using reflection
    for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString PropertyName = Property->GetName();

        if (PropertyName.Contains(TEXT("ItemIndex")))
        {
            if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
            {
                OutItemInfo.ItemIndex = IntProp->GetPropertyValue_InContainer(RowData);
            }
        }
        else if (PropertyName.Contains(TEXT("ItemName")))
        {
            if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
            {
                OutItemInfo.ItemName = StrProp->GetPropertyValue_InContainer(RowData);
            }
        }
        else if (PropertyName.Contains(TEXT("ItemDescription")))
        {
            if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
            {
                OutItemInfo.ItemDescription = StrProp->GetPropertyValue_InContainer(RowData);
            }
        }
        else if (PropertyName.Contains(TEXT("Price")))
        {
            if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
            {
                OutItemInfo.Price = IntProp->GetPropertyValue_InContainer(RowData);
            }
        }
        else if (PropertyName.Contains(TEXT("StaticMeshID")))
        {
            if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
            {
                const FSoftObjectPtr* SoftMeshPtr = SoftObjProp->ContainerPtrToValuePtr<FSoftObjectPtr>(RowData);
                if (SoftMeshPtr && !SoftMeshPtr->IsNull())
                {
                    OutItemInfo.StaticMeshID = TSoftObjectPtr<UStaticMesh>(SoftMeshPtr->ToSoftObjectPath());
                }
            }
        }
        else if (PropertyName.Contains(TEXT("ItemThumbnail")))
        {
            if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
            {
                const FSoftObjectPtr* SoftTexturePtr = SoftObjProp->ContainerPtrToValuePtr<FSoftObjectPtr>(RowData);
                if (SoftTexturePtr && !SoftTexturePtr->IsNull())
                {
                    OutItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(SoftTexturePtr->ToSoftObjectPath());
                }
            }
        }
        // Add more property extractions as needed for other fields
    }

    return OutItemInfo.ItemIndex > 0;
}

FString UUniversalItemLoader::CleanItemName(const FString& ItemName)
{
    return ItemName.Replace(TEXT(" "), TEXT("")).Replace(TEXT("-"), TEXT("")).Replace(TEXT("_"), TEXT(""));
}

FString UUniversalItemLoader::GetTextureBasePath()
{
    return TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/");
}

FString UUniversalItemLoader::GetMeshBasePath()
{
    return TEXT("/Game/AtlantisEons/Sources/Meshes/Items/");
}

template<typename T>
T* UUniversalItemLoader::LoadAssetFromPaths(const TArray<FString>& Paths)
{
    for (const FString& Path : Paths)
    {
        T* LoadedAsset = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *Path));
        if (LoadedAsset)
        {
            return LoadedAsset;
        }
    }
    return nullptr;
} 