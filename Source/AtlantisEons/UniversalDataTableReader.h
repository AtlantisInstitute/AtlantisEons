#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemTypes.h"
#include "UniversalDataTableReader.generated.h"

/**
 * Universal data table reader that can extract item data regardless of structure type
 * This solves the structure mismatch issues by using reflection to read properties by name
 */
UCLASS(BlueprintType)
class ATLANTISEONS_API UUniversalDataTableReader : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Read item data from the data table using reflection
     * This method can read any data table structure as long as it has the expected property names
     */
    UFUNCTION(BlueprintCallable, Category = "DataTable")
    static bool ReadItemData(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo);

    /**
     * Read item data from a specific row name
     */
    UFUNCTION(BlueprintCallable, Category = "DataTable")
    static bool ReadItemDataByRowName(const FString& RowName, FStructure_ItemInfo& OutItemInfo);

    /**
     * Get all available item indices from the data table
     */
    UFUNCTION(BlueprintCallable, Category = "DataTable")
    static TArray<int32> GetAllItemIndices();

    /**
     * Check if an item exists in the data table
     */
    UFUNCTION(BlueprintCallable, Category = "DataTable")
    static bool DoesItemExist(int32 ItemIndex);

private:
    /**
     * Extract data from raw row data using reflection
     */
    static bool ExtractItemDataFromRow(void* RowData, UScriptStruct* RowStruct, FStructure_ItemInfo& OutItemInfo);

    /**
     * Get property value by name using reflection
     */
    template<typename T>
    static bool GetPropertyValue(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, T& OutValue);

    /**
     * Get string property value
     */
    static bool GetStringProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, FString& OutValue);

    /**
     * Get integer property value
     */
    static bool GetIntProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, int32& OutValue);

    /**
     * Get boolean property value
     */
    static bool GetBoolProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, bool& OutValue);

    /**
     * Get enum property value
     */
    template<typename TEnum>
    static bool GetEnumProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, TEnum& OutValue);

    /**
     * Get soft object property value
     */
    template<typename T>
    static bool GetSoftObjectProperty(void* RowData, UScriptStruct* RowStruct, const FString& PropertyName, TSoftObjectPtr<T>& OutValue);
}; 