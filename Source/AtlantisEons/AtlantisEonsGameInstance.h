#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "ItemTypes.h"
#include "BlueprintItemTypes.h"
#include "BP_Item.h"
#include "BP_ItemInfo.h"
#include "AtlantisEonsGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ItemIndex = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StackNumber = 0;
};


/**
 * Game Instance class for managing global game state and item database
 */
UCLASS()
class ATLANTISEONS_API UAtlantisEonsGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UAtlantisEonsGameInstance();

    /** Get item information from the database using C++ structure */
    UFUNCTION(BlueprintCallable, Category = "Items")
    bool GetItemInfo(int32 ItemIndex, FStructure_ItemInfo& OutItemInfo) const;
    
    /** Get item information from the database using Blueprint structure */
    UFUNCTION(BlueprintCallable, Category = "Items", meta = (DisplayName = "Get Item Info Blueprint"))
    bool GetItemInfoBlueprint(int32 ItemIndex, FBlueprintItemInfo& OutItemInfo) const;

    /** Get the item data table */
    UFUNCTION(BlueprintCallable, Category = "Items")
    UDataTable* GetItemDataTable() const { return ItemDataTable; }

    /** Get the inventory slots */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TArray<FInventorySlot>& GetInventorySlots() { return InventorySlots; }

    /** Get a default item texture */
    UFUNCTION(BlueprintCallable, Category = "Items")
    UTexture2D* GetDefaultItemTexture();

    /** Create a colored texture */
    UFUNCTION(BlueprintCallable, Category = "Items")
    UTexture2D* CreateColoredTexture(FLinearColor Color, int32 Width = 64, int32 Height = 64);

protected:
    /** Reference to the item data table asset */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items")
    UDataTable* ItemDataTable;

    /** The player's inventory slots */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
    TArray<FInventorySlot> InventorySlots;

    // Cached default texture reference
    UPROPERTY()
    UTexture2D* DefaultItemTexture;

    virtual void Init() override;
};
