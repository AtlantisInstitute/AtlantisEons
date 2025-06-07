#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "CharacterUIManager.generated.h"

class AAtlantisEonsCharacter;
class UWBP_Main;
class UWBP_InventorySlot;
class UBP_ItemInfo;
class UProgressBar;
class UMaterialInstanceDynamic;

/**
 * Manages all UI-related functionality for the AtlantisEons Character
 * Handles circular bars, inventory UI updates, equipment slots, and UI state management
 * This component reduces the main character class size while preserving Blueprint compatibility
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UCharacterUIManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UCharacterUIManager();

    // ========== INITIALIZATION ==========
    
    /** Initialize UI components and setup references */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void InitializeUI();

    /** Setup circular health and mana bars */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetupCircularBars();

    // ========== CIRCULAR BAR MANAGEMENT ==========
    
    /** Update the circular health bar display */
    UFUNCTION(BlueprintCallable, Category = "UI|Circular Bar")
    void UpdateCircularBar_HP();

    /** Update the circular mana bar display */
    UFUNCTION(BlueprintCallable, Category = "UI|Circular Bar")
    void UpdateCircularBar_MP();

    /** Setup circular health bar with current values */
    UFUNCTION(BlueprintCallable, Category = "UI|Circular Bar")
    void SettingCircularBar_HP();

    /** Setup circular mana bar with current values */
    UFUNCTION(BlueprintCallable, Category = "UI|Circular Bar")
    void SettingCircularBar_MP();

    // ========== EQUIPMENT SLOT UI MANAGEMENT ==========
    
    /** Update equipment slot UI with item information */
    UFUNCTION(BlueprintCallable, Category = "UI|Equipment")
    void UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo);

    /** Clear equipment slot UI (remove item display) */
    UFUNCTION(BlueprintCallable, Category = "UI|Equipment")
    void ClearEquipmentSlotUI(EItemEquipSlot EquipSlot);

    /** Update all equipment slot UIs */
    UFUNCTION(BlueprintCallable, Category = "UI|Equipment")
    void UpdateAllEquipmentSlotsUI();

    /** Initialize equipment slot widget references */
    UFUNCTION(BlueprintCallable, Category = "UI|Equipment")
    void InitializeEquipmentSlotReferences();

    // ========== INVENTORY UI MANAGEMENT ==========
    
    /** Force update inventory slots after a delay */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void ForceUpdateInventorySlotsAfterDelay();

    /** Update inventory slot widgets */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void UpdateInventorySlots();

    /** Delayed inventory slot update */
    UFUNCTION()
    void DelayedUpdateInventorySlots();

    /** Set inventory slot with item information */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void SetInventorySlot(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotWidgetRef);

    /** Clear all active item info popups */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void ClearAllInventoryPopups();

    // ========== STORE UI MANAGEMENT ==========
    
    /** Setup store UI references and connections */
    UFUNCTION(BlueprintCallable, Category = "UI|Store")
    void SettingStore();

    // ========== UI STATE MANAGEMENT ==========
    
    /** Refresh all stats displays */
    UFUNCTION(BlueprintCallable, Category = "UI|Stats")
    void RefreshStatsDisplay();

    /** Update all UI elements with current character stats */
    UFUNCTION(BlueprintCallable, Category = "UI|Stats")
    void UpdateAllStatsUI();

    /** Set main widget reference */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetMainWidget(UWBP_Main* NewWidget);

    /** Get main widget reference */
    UFUNCTION(BlueprintPure, Category = "UI")
    UWBP_Main* GetMainWidget() const { return MainWidget; }

    /** Connect inventory component to main widget */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ConnectInventoryToMainWidget();

protected:
    virtual void BeginPlay() override;

private:
    // ========== CACHED REFERENCES ==========
    
    /** Reference to the owning character */
    UPROPERTY()
    AAtlantisEonsCharacter* OwnerCharacter;

    /** Main widget reference */
    UPROPERTY()
    UWBP_Main* MainWidget;

    /** Cached material instances for circular bars */
    UPROPERTY()
    UMaterialInstanceDynamic* CircularHPMaterial;

    UPROPERTY()
    UMaterialInstanceDynamic* CircularMPMaterial;

    // ========== TIMER HANDLES ==========
    
    /** Timer for delayed inventory updates */
    FTimerHandle InventoryUpdateTimer;

    // ========== HELPER FUNCTIONS ==========
    
    /** Cache all necessary widget and component references */
    void CacheUIReferences();

    /** Validate that all required references are set */
    bool ValidateUIReferences() const;

    /** Get equipment slot widget by slot type */
    UWBP_InventorySlot* GetEquipmentSlotWidget(EItemEquipSlot EquipSlot) const;
}; 