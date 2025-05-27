#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "ItemTypes.h"
#include "UniversalItemLoader.generated.h"

/**
 * Universal item loader that can handle any new items added to the data table
 * without requiring code changes. This system is fully data-driven and extensible.
 */
UCLASS(BlueprintType)
class ATLANTISEONS_API UUniversalItemLoader : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Load item data for any item index from the data table
     * This method works with any number of items and any data table structure
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static bool LoadItemData(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo);

    /**
     * Load texture for any item using intelligent path resolution
     * Supports multiple naming conventions and fallback patterns
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static UTexture2D* LoadItemTexture(const FStructure_ItemInfo& ItemInfo);

    /**
     * Load static mesh for any item using intelligent path resolution
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static UStaticMesh* LoadItemMesh(const FStructure_ItemInfo& ItemInfo);

    /**
     * Get all available items from the data table
     * Returns all valid items regardless of how many are in the table
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static TArray<FStructure_ItemInfo> GetAllItems();

    /**
     * Check if an item exists in the data table
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static bool DoesItemExist(int32 ItemIndex);

    /**
     * Get the maximum item index in the data table
     * Useful for determining the range of available items
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static int32 GetMaxItemIndex();

    /**
     * Get exact texture paths from the data table for a specific item
     * This uses the actual texture names stored in the data table
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static TArray<FString> GetExactTexturePathsFromDataTable(int32 ItemIndex);

    /**
     * Generate intelligent texture path variations for any item
     * This handles various naming conventions automatically
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static TArray<FString> GenerateTexturePaths(const FString& ItemName, int32 ItemIndex);

    /**
     * Generate intelligent mesh path variations for any item
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static TArray<FString> GenerateMeshPaths(const FString& ItemName, int32 ItemIndex);

    /**
     * Validate that all items in the data table have proper assets
     * Useful for debugging and ensuring data integrity
     */
    UFUNCTION(BlueprintCallable, Category = "Universal Item Loader")
    static void ValidateAllItems();

private:
    /**
     * Get the data table with caching
     */
    static UDataTable* GetItemDataTable();

    /**
     * Extract item data from raw row data using reflection
     * This works with any data table structure that has the expected properties
     */
    static bool ExtractItemDataFromRow(void* RowData, UScriptStruct* RowStruct, FStructure_ItemInfo& OutItemInfo);

    /**
     * Clean item name for path generation
     */
    static FString CleanItemName(const FString& ItemName);

    /**
     * Generate base asset paths
     */
    static FString GetTextureBasePath();
    static FString GetMeshBasePath();

    /**
     * Try to load asset from multiple path variations
     */
    template<typename T>
    static T* LoadAssetFromPaths(const TArray<FString>& Paths);

    /**
     * Extract property value using reflection
     */
    template<typename T>
    static bool GetPropertyValue(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, T& OutValue);

    /**
     * Cached data table reference
     */
    static UDataTable* CachedDataTable;
}; 