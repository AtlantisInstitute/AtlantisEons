#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/NoExportTypes.h"
#include "DataTableDiagnostic.generated.h"

/**
 * Diagnostic utility to inspect data table structure and content
 * This will help identify why the store system can't read the data table properly
 */
UCLASS(BlueprintType)
class ATLANTISEONS_API UDataTableDiagnostic : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Inspect the data table structure and log all available information
     * This will help us understand what structure the data table is actually using
     */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    static bool InspectItemDataTable();

    /**
     * Get all row names from the data table
     */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    static TArray<FString> GetDataTableRowNames();

    /**
     * Get the structure name used by the data table
     */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    static FString GetDataTableStructureName();

    /**
     * Test reading raw data from a specific row
     */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    static bool TestReadRawRowData(const FString& RowName);

    /**
     * Export data table content to a readable format for debugging
     */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    static bool ExportDataTableContent();
}; 