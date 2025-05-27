#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemTypes.h"
#include "BlueprintItemTypes.h"
#include "StoreDataTableInspector.generated.h"

/**
 * Comprehensive data table inspector to examine actual table contents
 */
UCLASS(BlueprintType)
class ATLANTISEONS_API UStoreDataTableInspector : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Inspect the data table and log all findings
     */
    UFUNCTION(BlueprintCallable, Category = "Inspector")
    static void InspectDataTable();

    /**
     * Get the actual row structure name
     */
    UFUNCTION(BlueprintCallable, Category = "Inspector")
    static FString GetDataTableStructureName();

    /**
     * List all row names in the data table
     */
    UFUNCTION(BlueprintCallable, Category = "Inspector")
    static TArray<FString> GetAllRowNames();

    /**
     * Try to read a specific row and show what's actually in it
     */
    UFUNCTION(BlueprintCallable, Category = "Inspector")
    static void InspectSpecificRow(const FString& RowName);

    /**
     * Test different structure types against the data table
     */
    UFUNCTION(BlueprintCallable, Category = "Inspector")
    static void TestStructureCompatibility();

private:
    /**
     * Get the data table reference
     */
    static UDataTable* GetItemDataTable();

    /**
     * Log detailed information about a row
     */
    static void LogRowDetails(void* RowData, UScriptStruct* RowStruct, const FString& RowName);
}; 