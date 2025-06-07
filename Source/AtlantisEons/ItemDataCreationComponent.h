#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "ItemDataCreationComponent.generated.h"

class UBP_ItemInfo;
class UAtlantisEonsGameInstance;

/**
 * ItemDataCreationComponent
 * 
 * Centralizes item data creation, validation, and asset loading logic.
 * Eliminates code duplication across multiple classes and provides a single
 * source of truth for item data generation.
 * 
 * Features:
 * - Basic hardcoded item data for testing/fallback
 * - Intelligent asset path resolution and loading
 * - Data table integration with fallback mechanisms
 * - Asset validation and error handling
 * - Blueprint can manage item variations via properties and data tables
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UItemDataCreationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UItemDataCreationComponent();

    // ========== CORE ITEM CREATION FUNCTIONS ==========
    
    /** 
     * Main item data creation function with basic hardcoded fallback
     * Blueprint and DataTable manage actual item variations
     */
    UFUNCTION(BlueprintCallable, Category = "Item Creation")
    FStructure_ItemInfo CreateItemData(int32 ItemIndex);

    /** 
     * Create item data with custom stack number override
     */
    UFUNCTION(BlueprintCallable, Category = "Item Creation")
    FStructure_ItemInfo CreateItemDataWithStack(int32 ItemIndex, int32 StackNumber);

    /** 
     * Batch create multiple items at once for inventory initialization
     */
    UFUNCTION(BlueprintCallable, Category = "Item Creation")
    TArray<FStructure_ItemInfo> CreateMultipleItems(const TArray<int32>& ItemIndices);

    // ========== ASSET LOADING AND VALIDATION ==========
    
    /** 
     * Load and validate texture assets for items with intelligent path resolution
     */
    UFUNCTION(BlueprintCallable, Category = "Asset Loading")
    UTexture2D* LoadItemTexture(int32 ItemIndex, const FString& ItemName = TEXT(""));

    /** 
     * Load and validate static mesh assets for equipment items
     */
    UFUNCTION(BlueprintCallable, Category = "Asset Loading")
    UStaticMesh* LoadItemMesh(int32 ItemIndex, const FString& ItemName = TEXT(""));

    /** 
     * Validate that an item's assets exist and are loadable
     */
    UFUNCTION(BlueprintCallable, Category = "Asset Loading")
    bool ValidateItemAssets(const FStructure_ItemInfo& ItemInfo);

    // ========== DATA TABLE INTEGRATION ==========
    
    /** 
     * Attempt to load item data from data table with multiple fallback patterns
     */
    UFUNCTION(BlueprintCallable, Category = "Data Table")
    bool LoadFromDataTable(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo);

    /** 
     * Get item data with data table priority and hardcoded fallback
     */
    UFUNCTION(BlueprintCallable, Category = "Data Table")
    bool GetItemDataWithFallback(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo);

    // ========== ITEM VALIDATION AND UTILITIES ==========
    
    /** 
     * Check if an item index has valid data available
     */
    UFUNCTION(BlueprintCallable, Category = "Item Validation")
    bool IsValidItemIndex(int32 ItemIndex);

    /** 
     * Get all available item indices in the hardcoded database
     */
    UFUNCTION(BlueprintCallable, Category = "Item Validation")
    TArray<int32> GetAllAvailableItemIndices();

    /** 
     * Get items by category (weapons, armor, consumables, etc.)
     */
    UFUNCTION(BlueprintCallable, Category = "Item Validation")
    TArray<FStructure_ItemInfo> GetItemsByType(EItemType ItemType);

    /** 
     * Get items by equipment slot
     */
    UFUNCTION(BlueprintCallable, Category = "Item Validation")
    TArray<FStructure_ItemInfo> GetItemsByEquipSlot(EItemEquipSlot EquipSlot);

    // ========== BLUEPRINT ITEM INFO CREATION ==========
    
    /** 
     * Create a UBP_ItemInfo object from item data
     */
    UFUNCTION(BlueprintCallable, Category = "Blueprint Integration")
    UBP_ItemInfo* CreateBlueprintItemInfo(int32 ItemIndex, int32 StackNumber = 1);

    // ========== DEBUGGING AND LOGGING ==========
    
    /** 
     * Log detailed information about an item for debugging
     */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void LogItemInfo(const FStructure_ItemInfo& ItemInfo);

    /** 
     * Get statistics about the item database
     */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    FString GetDatabaseStatistics();

protected:
    // ========== CORE FUNCTIONS ==========
    
    /** Create basic hardcoded item data (fallback for testing) */
    FStructure_ItemInfo CreateHardcodedItemData(int32 ItemIndex);

    /** Generate intelligent asset paths for textures */
    TArray<FString> GenerateTexturePaths(int32 ItemIndex, const FString& ItemName);

    /** Generate intelligent asset paths for meshes */
    TArray<FString> GenerateMeshPaths(int32 ItemIndex, const FString& ItemName);

    /** Attempt to load asset from multiple possible paths */
    template<typename T>
    T* LoadAssetFromPaths(const TArray<FString>& AssetPaths);

    /** Validate and log asset loading results */
    void LogAssetLoadResult(const FString& AssetType, const FString& AssetPath, bool bSuccess);

private:
    /** Cached reference to data table for optimization */
    UPROPERTY()
    UDataTable* CachedDataTable;

    /** Statistics tracking */
    mutable int32 DataTableHits = 0;
    mutable int32 HardcodedFallbacks = 0;
    mutable int32 AssetLoadAttempts = 0;
    mutable int32 AssetLoadSuccesses = 0;
    mutable int32 ItemsCreated = 0;
    mutable int32 LastCreatedItemIndex = 0;
}; 