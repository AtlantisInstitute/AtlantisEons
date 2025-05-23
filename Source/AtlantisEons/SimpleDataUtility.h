#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SimpleDataUtility.generated.h"

/**
 * Simple utility class to help identify data structure issues
 */
UCLASS()
class ATLANTISEONS_API USimpleDataUtility : public UObject
{
    GENERATED_BODY()

public:
    // Get the name of the row structure for the ItemList data table
    UFUNCTION(BlueprintCallable, Category = "Data")
    static FString GetItemTableStructureName();
    
    // Check if an ItemList row exists in the data table
    UFUNCTION(BlueprintCallable, Category = "Data")
    static bool DoesItemExist(int32 ItemIndex);
};
