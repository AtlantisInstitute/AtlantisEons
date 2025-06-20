#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "BP_ItemInfo.h"
#include "WBP_InventorySlot.h"
#include "WBP_Main.h"
#include "EquipmentComponent.generated.h"

class AAtlantisEonsCharacter;
class UStaticMeshComponent;
class UWBP_CharacterInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquippedSignature, UBP_ItemInfo*, ItemInfo, EItemEquipSlot, EquipSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUnequippedSignature, UBP_ItemInfo*, ItemInfo, EItemEquipSlot, EquipSlot);

/**
 * Component responsible for managing character equipment operations.
 * This separates equipment management from the main character class for better organization.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UEquipmentComponent();

    // Equipment events
    UPROPERTY(BlueprintAssignable, Category = "Equipment")
    FOnItemEquippedSignature OnItemEquipped;

    UPROPERTY(BlueprintAssignable, Category = "Equipment")
    FOnItemUnequippedSignature OnItemUnequipped;

    // Equipment settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Settings")
    int32 MaxEquipmentSlots = 10;

    // Core equipment functions
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void InitializeEquipment();

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool EquipItem(UBP_ItemInfo* ItemInfo);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool UnequipItem(EItemEquipSlot EquipSlot);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool UnequipItemByInfo(UBP_ItemInfo* ItemInfo);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    UBP_ItemInfo* GetEquippedItem(EItemEquipSlot EquipSlot) const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    TArray<UBP_ItemInfo*> GetAllEquippedItems() const { return EquipmentSlots; }

    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool IsSlotEmpty(EItemEquipSlot EquipSlot) const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool HasItemEquipped(int32 ItemIndex) const;

    // Visual equipment functions
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void EquipItemInSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, 
                        const TSoftObjectPtr<UTexture2D>& Texture2D, int32 ItemIndex, 
                        UMaterialInterface* MaterialInterface = nullptr, UMaterialInterface* MaterialInterface2 = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void HandleDisarmItem(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, int32 ItemIndex);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void ProcessEquipItem(UBP_ItemInfo* ItemInfoRef);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void ProcessConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, 
                           int32 RecoverHP, int32 RecoverMP, EItemType ItemType);

    // UI management functions
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void ClearEquipmentSlotUI(EItemEquipSlot EquipSlot);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void UpdateAllEquipmentSlotsUI();

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void InitializeEquipmentSlotReferences();

    // Equipment slot click handlers
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void OnEquipmentSlotClicked(EItemEquipSlot EquipSlot);

    UFUNCTION()
    void OnHeadSlotClicked(int32 SlotIndex);

    UFUNCTION()
    void OnWeaponSlotClicked(int32 SlotIndex);

    UFUNCTION()
    void OnSuitSlotClicked(int32 SlotIndex);

    UFUNCTION()
    void OnCollectableSlotClicked(int32 SlotIndex);

    // High-level equipment operations
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void EquipInventoryItem(UBP_ItemInfo* ItemInfoRef);

    // Fixed version that bypasses world context issues
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void EquipInventoryItemWithCharacter(UBP_ItemInfo* ItemInfoRef, AAtlantisEonsCharacter* CharacterRef);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void UnequipInventoryItem(UBP_ItemInfo* ItemInfoRef);

    // Fixed version that bypasses world context issues
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void UnequipInventoryItemWithCharacter(UBP_ItemInfo* ItemInfoRef, AAtlantisEonsCharacter* CharacterRef);

    // UI setup
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void SetMainWidget(UWBP_Main* NewMainWidget);

    UFUNCTION(BlueprintPure, Category = "Equipment")
    UWBP_Main* GetMainWidget() const { return MainWidget; }

    // Utility functions
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void ClearAllEquipment();

    UFUNCTION(BlueprintPure, Category = "Equipment")
    int32 GetEquipSlotIndex(EItemEquipSlot EquipSlot) const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    EItemEquipSlot GetEquipSlotFromIndex(int32 SlotIndex) const;

    // Equipment validation
    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool CanEquipItem(UBP_ItemInfo* ItemInfo) const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    EItemEquipSlot DetermineEquipSlot(UBP_ItemInfo* ItemInfo) const;

    // Async mesh loading callback
    UFUNCTION()
    void OnMeshLoaded(UStaticMeshComponent* TargetComponent, int32 ItemIndex, FString SlotName);
    
    UFUNCTION()
    void OnMeshLoadedAsync(UStaticMeshComponent* TargetComponent, int32 ItemIndex, FString SlotName, TSoftObjectPtr<UStaticMesh> StaticMeshID);

protected:
    virtual void BeginPlay() override;

private:
    // Internal equipment storage
    UPROPERTY()
    TArray<UBP_ItemInfo*> EquipmentSlots;

    // UI references
    UPROPERTY()
    UWBP_Main* MainWidget;

    UPROPERTY()
    UWBP_CharacterInfo* WBP_CharacterInfo;

    // Equipment slot widget references
    UPROPERTY()
    UWBP_InventorySlot* HeadSlot;

    UPROPERTY()
    UWBP_InventorySlot* WeaponSlot;

    UPROPERTY()
    UWBP_InventorySlot* SuitSlot;

    UPROPERTY()
    UWBP_InventorySlot* CollectableSlot;

    // Owner character reference
    UPROPERTY()
    AAtlantisEonsCharacter* OwnerCharacter;

    // Equipment mesh component references
    UPROPERTY()
    UStaticMeshComponent* Helmet;

    UPROPERTY()
    UStaticMeshComponent* Weapon;

    UPROPERTY()
    UStaticMeshComponent* Shield;

    // Helper functions
    void BroadcastEquipmentChanged();
    UStaticMeshComponent* GetMeshComponentForSlot(EItemEquipSlot EquipSlot) const;
    FString GetSlotName(EItemEquipSlot EquipSlot) const;
    void ApplyEquipmentHotfixes(UBP_ItemInfo* ItemInfo, FStructure_ItemInfo& ItemData) const;
    
    /** Apply weapon loading system (scale, materials, and other configurations) for all weapons */
    void ApplyWeaponLoadingSystem(UStaticMeshComponent* MeshComponent, int32 ItemIndex, EItemEquipSlot EquipSlot);
    
    /** Load and apply standard weapon materials to weapon mesh components */
    void LoadWeaponMaterials(UStaticMeshComponent* MeshComponent, int32 ItemIndex);
}; 