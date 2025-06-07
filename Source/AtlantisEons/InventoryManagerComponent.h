#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "InputActionValue.h"
#include "Engine/TimerHandle.h"

class AAtlantisEonsCharacter;
class UWBP_Main;
class UWBP_InventorySlot;
class UBP_ItemInfo;
class AAtlantisEonsHUD;

#include "InventoryManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryStateChangedSignature);

/**
 * InventoryManagerComponent
 * 
 * Manages all high-level inventory operations, UI state management, and input handling.
 * Extracted from AtlantisEonsCharacter to reduce its size and centralize inventory logic.
 * 
 * Features:
 * - Inventory UI state management (open/close, toggle locking)
 * - Input handling for inventory toggle and ESC key
 * - Integration with HUD and main widget systems
 * - Context menu operations delegation
 * - Drag and drop operations coordination
 * - Equipment slot management integration
 * - Store integration for buying items
 * - Inventory update scheduling and batching
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UInventoryManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryManagerComponent();

protected:
    virtual void BeginPlay() override;

public:
    // ========== COMPONENT INITIALIZATION ==========
    
    /** Initialize the component with owner character reference */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void InitializeInventoryManager(AAtlantisEonsCharacter* InOwnerCharacter);

    // ========== INVENTORY UI STATE MANAGEMENT ==========
    
    /** Toggle the inventory open/closed */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ToggleInventory(const FInputActionValue& Value);

    /** Open the inventory */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void OpenInventory();

    /** Close the inventory */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void CloseInventory();

    /** Implementation of inventory closing */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void CloseInventoryImpl();

    /** Force set inventory state without checking current state */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ForceSetInventoryState(bool bNewIsOpen);

    /** Close inventory if open (for ESC key handling) */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void CloseInventoryIfOpen(const FInputActionValue& Value);

    /** Check if inventory is currently open */
    UFUNCTION(BlueprintPure, Category = "Inventory Manager")
    bool IsInventoryOpen() const { return bIsInventoryOpen; }

    // ========== INVENTORY TOGGLE LOCKING ==========
    
    /** Set inventory toggle lock to prevent rapid switching */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void SetInventoryToggleLock(bool bLock, float UnlockDelay = 0.0f);

    /** Unlock the inventory toggle after delay */
    UFUNCTION()
    void UnlockInventoryToggle();

    /** Check if inventory toggle is currently locked */
    UFUNCTION(BlueprintPure, Category = "Inventory Manager")
    bool IsInventoryToggleLocked() const { return bInventoryToggleLocked; }

    // ========== INVENTORY SLOT MANAGEMENT ==========
    
    /** Update all inventory slot widgets */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void UpdateInventorySlots();

    /** Delayed update for inventory slots */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void DelayedUpdateInventorySlots();

    /** Force update inventory slots after delay */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ForceUpdateInventorySlotsAfterDelay();

    /** Set inventory slot data */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void SetInventorySlot(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotWidgetRef);

    // ========== CONTEXT MENU OPERATIONS ==========
    
    /** Handle context menu use operation */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ContextMenuUse(UWBP_InventorySlot* InventorySlot);

    /** Handle context menu throw operation */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ContextMenuThrow(UWBP_InventorySlot* InventorySlot);

    /** Handle equipping item from context menu */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ContextMenuUse_EquipItem(UBP_ItemInfo* ItemInfoRef);

    /** Handle consuming item from context menu */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ContextMenuUse_ConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef,
        int32 RecoverHP, int32 RecoverMP, EItemType ItemType);

    /** Process consume item operation */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ProcessConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef,
        int32 RecoverHP, int32 RecoverMP, EItemType ItemType);

    // ========== DRAG AND DROP OPERATIONS ==========
    
    /** Handle drag and drop exchange between inventory slots */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void DragAndDropExchangeItem(UBP_ItemInfo* FromInventoryItemRef, UWBP_InventorySlot* FromInventorySlotRef,
        UBP_ItemInfo* ToInventoryItemRef, UWBP_InventorySlot* ToInventorySlotRef);

    // ========== ITEM OPERATIONS ==========
    
    /** Pick up item and add to inventory */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    bool PickingItem(int32 ItemIndex, int32 ItemStackNumber);

    /** Purchase item from store */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    bool BuyingItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice);

    /** Add item to inventory */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    bool AddItemToInventory(UBP_ItemInfo* ItemInfo);

    /** Create item info reference */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    UBP_ItemInfo* GetInventoryItemRef() const;

    // ========== EQUIPMENT INTEGRATION ==========
    
    /** Equip item from inventory */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void EquipInventoryItem(UBP_ItemInfo* ItemInfoRef);

    /** Unequip item to inventory */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void UnequipInventoryItem(UBP_ItemInfo* ItemInfoRef);

    // ========== UI MANAGEMENT ==========
    
    /** Set main widget reference */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void SetMainWidget(UWBP_Main* NewWidget);

    /** Get main widget reference */
    UFUNCTION(BlueprintPure, Category = "Inventory Manager")
    UWBP_Main* GetMainWidget() const { return MainWidget; }

    /** Clear all inventory popups */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ClearAllInventoryPopups();

    /** Connect inventory to main widget */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void ConnectInventoryToMainWidget();

    // ========== STORE INTEGRATION ==========
    
    /** Setup store integration */
    UFUNCTION(BlueprintCallable, Category = "Inventory Manager")
    void SettingStore();

    // ========== EVENT HANDLERS ==========
    
    /** Context menu event handlers */
    UFUNCTION()
    void OnContextMenuUse(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot);

    UFUNCTION()
    void OnContextMenuThrow(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot);

    /** Inventory change event handler */
    UFUNCTION()
    void OnInventoryChangedEvent();

    // ========== BROADCASTS ==========
    
    /** Broadcast when inventory state changes */
    UPROPERTY(BlueprintAssignable, Category = "Inventory Manager")
    FOnInventoryStateChangedSignature OnInventoryStateChanged;

protected:
    // ========== CORE REFERENCES ==========
    
    /** Owner character reference */
    UPROPERTY()
    AAtlantisEonsCharacter* OwnerCharacter;

    /** Main widget reference */
    UPROPERTY()
    UWBP_Main* MainWidget;

    // ========== INVENTORY STATE ==========
    
    /** Whether the inventory is currently open */
    UPROPERTY(BlueprintReadOnly, Category = "Inventory Manager")
    bool bIsInventoryOpen = false;

    /** Whether the inventory toggle is locked to prevent conflicts */
    UPROPERTY(BlueprintReadOnly, Category = "Inventory Manager")
    bool bInventoryToggleLocked = false;

    /** Whether inventory needs update */
    UPROPERTY(BlueprintReadOnly, Category = "Inventory Manager")
    bool bInventoryNeedsUpdate = false;

    // ========== TIMER HANDLES ==========
    
    /** Timer for inventory toggle lock */
    FTimerHandle InventoryToggleLockTimer;

    /** Timer for delayed inventory updates */
    FTimerHandle InventoryUpdateTimer;

private:
    // ========== HELPER FUNCTIONS ==========
    
    /** Get HUD reference safely */
    AAtlantisEonsHUD* GetHUD() const;

    /** Setup input mode for inventory */
    void SetupInventoryInputMode();

    /** Restore game input mode */
    void RestoreGameInputMode();

    /** Initialize widget references */
    void InitializeWidgetReferences();

    /** Update inventory display */
    void UpdateInventoryDisplay();

    /** Handle inventory input setup */
    void HandleInventoryInputSetup();
}; 