#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemTypes.h"
#include "BlueprintItemTypes.h"
#include "StoreSystemFix.generated.h"

/**
 * Comprehensive store system fix that handles data table structure mismatches
 * and provides proper item data extraction
 */
UCLASS(BlueprintType)
class ATLANTISEONS_API UStoreSystemFix : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Get item data with automatic structure detection and fallback
     */
    UFUNCTION(BlueprintCallable, Category = "Store")
    static bool GetItemData(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo);

    /**
     * Get all available items from the data table
     */
    UFUNCTION(BlueprintCallable, Category = "Store")
    static TArray<FStructure_ItemInfo> GetAllStoreItems();

    /**
     * Check if item exists in data table
     */
    UFUNCTION(BlueprintCallable, Category = "Store")
    static bool DoesItemExist(int32 ItemIndex);

    /**
     * Get item price with fallback calculation
     */
    UFUNCTION(BlueprintCallable, Category = "Store")
    static int32 GetItemPrice(int32 ItemIndex);

    /**
     * Diagnose data table structure and log findings
     */
    UFUNCTION(BlueprintCallable, Category = "Store")
    static void DiagnoseDataTable();

    /**
     * Extract data table as JSON and parse real item data
     */
    UFUNCTION(BlueprintCallable, Category = "Store")
    static void ExtractDataTableAsJSON();

    /**
     * Get item data from table using property mapping
     */
    static FStructure_ItemInfo GetItemDataFromTable(int32 ItemIndex);

    /**
     * Get the data table reference with caching (made public for UniversalItemLoader)
     */
    static UDataTable* GetItemDataTable();

private:
    /**
     * Try to extract data using FStructure_ItemInfo
     */
    static bool TryExtractWithStructureItemInfo(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo);

    /**
     * Try to extract data using FBlueprintItemInfo
     */
    static bool TryExtractWithBlueprintItemInfo(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo);

    /**
     * Create fallback item data when data table fails
     */
    static FStructure_ItemInfo CreateFallbackItemData(int32 ItemIndex);

    /**
     * Extract item info using property name mapping for generated suffixes
     */
    static FStructure_ItemInfo ExtractItemInfoWithMapping(void* RowData, const UScriptStruct* RowStruct, int32 ItemIndex);

    /**
     * Helper methods for property extraction with mapping
     */
    static int32 ExtractIntProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName, int32 DefaultValue);
    static FString ExtractStringProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName, const FString& DefaultValue);
    
    template<typename T>
    static T ExtractEnumProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName, T DefaultValue);

    template<typename T>
    static TSoftObjectPtr<T> ExtractSoftObjectProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName);

    static UObject* ExtractObjectProperty(void* RowData, const UScriptStruct* RowStruct, const TMap<FString, FString>& PropertyMapping, const FString& LogicalName);
}; 