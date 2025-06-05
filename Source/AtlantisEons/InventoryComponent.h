#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "BP_ItemInfo.h"
#include "WBP_InventorySlot.h"
#include "WBP_Main.h"
#include "InventoryComponent.generated.h"

class UBP_ConcreteItemInfo;
class UWBP_Inventory;
class AAtlantisEonsCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAddedSignature, UBP_ItemInfo*, ItemInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemovedSignature, UBP_ItemInfo*, ItemInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUsedSignature, UBP_ItemInfo*, ItemInfo);

/**
 * Component responsible for managing character inventory operations.
 * This separates inventory management from the main character class for better organization.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UInventoryComponent();

    // Inventory events
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryChangedSignature OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnItemAddedSignature OnItemAdded;

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnItemRemovedSignature OnItemRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnItemUsedSignature OnItemUsed;

    // Inventory settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Settings")
    int32 InventorySize = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Settings")
    int32 MaxStackSize = 99;

    // Core inventory functions
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void InitializeInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(int32 ItemIndex, int32 StackNumber = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItemInfo(UBP_ItemInfo* ItemInfo);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(int32 SlotIndex, int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItemByIndex(int32 ItemIndex, int32 Amount = 1);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool HasItem(int32 ItemIndex, int32 Amount = 1) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(int32 ItemIndex) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    UBP_ItemInfo* GetItemAtSlot(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    TArray<UBP_ItemInfo*> GetAllItems() const { return InventoryItems; }

    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool IsSlotEmpty(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetFirstEmptySlot() const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetAvailableSlots() const;

    // Context menu operations
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UseItem(UWBP_InventorySlot* InventorySlot);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ThrowItem(UWBP_InventorySlot* InventorySlot);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ContextMenuUse(UWBP_InventorySlot* InventorySlot);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ContextMenuThrow(UWBP_InventorySlot* InventorySlot);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ContextMenuUse_EquipItem(UBP_ItemInfo* ItemInfoRef);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ContextMenuUse_ConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, int32 RecoverHP, int32 RecoverMP, EItemType ItemType);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void EquipItem(UBP_ItemInfo* ItemInfo);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ConsumeItem(UBP_ItemInfo* ItemInfo, UWBP_InventorySlot* InventorySlot, int32 RecoverHP, int32 RecoverMP, EItemType ItemType);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItemToInventory(UBP_ItemInfo* ItemInfo);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UpdateInventorySlots();

    // Drag and drop operations
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SwapItems(int32 FromSlotIndex, int32 ToSlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void StackItems(int32 FromSlotIndex, int32 ToSlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void HandleDragAndDrop(UBP_ItemInfo* FromItem, UWBP_InventorySlot* FromSlot, UBP_ItemInfo* ToItem, UWBP_InventorySlot* ToSlot);

    // UI management
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UpdateInventoryUI();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SetMainWidget(UWBP_Main* NewMainWidget);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    UWBP_Main* GetMainWidget() const { return MainWidget; }

    // Store integration
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool PurchaseItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool BuyItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice);

    // Utility functions
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ClearInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    FStructure_ItemInfo CreateHardcodedItemData(int32 ItemIndex);

    // Context menu event handlers (called by inventory slot widgets)
    UFUNCTION()
    void OnContextMenuUse(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot);

    UFUNCTION()
    void OnContextMenuThrow(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot);

protected:
    virtual void BeginPlay() override;

private:
    // Internal inventory storage
    UPROPERTY()
    TArray<UBP_ItemInfo*> InventoryItems;

    // UI reference
    UPROPERTY()
    UWBP_Main* MainWidget;

    // Owner character reference
    UPROPERTY()
    AAtlantisEonsCharacter* OwnerCharacter;

    // Helper functions
    bool CanStackItems(UBP_ItemInfo* Item1, UBP_ItemInfo* Item2) const;
    int32 FindItemSlot(int32 ItemIndex) const;
    void BroadcastInventoryChanged();
    UBP_ConcreteItemInfo* CreateItemFromData(const FStructure_ItemInfo& ItemData, int32 StackNumber);
    bool GetItemDataForIndex(int32 ItemIndex, FStructure_ItemInfo& OutItemData);
    void SpawnItemInWorld(int32 ItemIndex, int32 StackNumber, FVector SpawnLocation, FRotator SpawnRotation);
}; 