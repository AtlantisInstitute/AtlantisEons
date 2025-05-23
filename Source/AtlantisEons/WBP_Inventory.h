#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "WBP_InventorySlot.h"
#include "WBP_ItemDescription.h"
#include "BP_Item.h"
#include "WBP_Inventory.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotClickedSignature, int32, SlotIndex);

/**
 * Widget class for the inventory system.
 * Handles displaying and managing inventory slots in a grid layout.
 */
UCLASS()
class ATLANTISEONS_API UWBP_Inventory : public UUserWidget
{
    GENERATED_BODY()

public:
    UWBP_Inventory(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnInitialized() override;

    /** Initialize the inventory with a specific number of slots */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void InitializeInventory(int32 Slots);

    /** Get an empty inventory slot */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    class UWBP_InventorySlot* GetEmptySlot();

    /** Get the current storage usage text */
    UFUNCTION(BlueprintPure, Category = "Inventory")
    FText GetStorageNumber() const;

    /** Add an item to the inventory */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(const FStructure_ItemInfo& ItemInfo);

    /** Remove an item from a specific slot */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(int32 SlotIndex, int32 Amount = 1);

    /** Get all inventory slot widgets */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    const TArray<class UWBP_InventorySlot*>& GetInventorySlotWidgets() const;

    /** Get the number of inventory slots */
    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetInventorySlotCount() const { return InventorySlots.Num(); }

    /** Get the inventory grid panel */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UUniformGridPanel* GetInventoryPanel() const { return InventoryGrid; }

protected:
    /** Create all necessary inventory components */
    UFUNCTION()
    void CreateInventoryComponents();

    /** Create the inventory grid */
    UFUNCTION()
    void CreateInventoryGrid();

    /** Create the close button */
    UFUNCTION()
    void CreateCloseButton();

    /** Create the storage text */
    UFUNCTION()
    void CreateStorageText();

    /** Update the storage text display */
    UFUNCTION()
    void UpdateStorageText();

    /** Handle slot click events */
    UFUNCTION()
    void OnSlotClicked(int32 SlotIndex);

    /** Handle close button click */
    UFUNCTION()
    void OnCloseButtonClicked();

    /** Update a specific inventory slot */
    void UpdateSlot(int32 SlotIndex);

    /** Find an empty slot in the inventory */
    int32 FindEmptySlot() const;

    /** Find a slot that can stack with the given item */
    int32 FindStackableSlot(const FStructure_ItemInfo& ItemInfo) const;

protected:
    /** The grid panel containing inventory slots */
    UPROPERTY()
    UUniformGridPanel* InventoryGrid;

    /** Button to close the inventory */
    UPROPERTY()
    UButton* CloseButton;

    /** Text showing storage capacity */
    UPROPERTY()
    UTextBlock* StorageText;

    /** Array of inventory slot widgets */
    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    TArray<class UWBP_InventorySlot*> InventorySlots;

    /** Array of inventory item data */
    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    TArray<FStructure_ItemInfo> InventoryData;

    /** Maximum number of inventory slots */
    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    int32 MaxSlots;

    /** Widget for displaying item descriptions */
    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    UWBP_ItemDescription* ItemDescriptionWidget;

    /** Class to use for inventory slots */
    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    TSubclassOf<class UWBP_InventorySlot> InventorySlotClass;

    /** Delegate for slot click events */
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventorySlotClickedSignature OnInventorySlotClicked;

public:
    /** Constants */
    static constexpr int32 DEFAULT_GRID_COLUMNS = 5;
    static constexpr int32 DEFAULT_GRID_ROWS = 6;
    static constexpr int32 MAX_STACK_SIZE = 99;
};
