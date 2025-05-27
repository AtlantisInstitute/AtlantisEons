#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "BP_ItemInfo.h"
#include "CharacterStatsComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "EquipmentComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChangedSignature, EItemEquipSlot, Slot, UBP_ItemInfo*, ItemInfo);

/**
 * Component responsible for managing character equipment
 * Handles equipping/unequipping items, visual updates, and stat modifications
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UEquipmentComponent();

    // Equipment change event
    UPROPERTY(BlueprintAssignable, Category = "Equipment")
    FOnEquipmentChangedSignature OnEquipmentChanged;

    // Equipment slot management
    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    TArray<UBP_ItemInfo*> EquipmentSlots;

    // Equipment mesh components (set by character in BeginPlay)
    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    UStaticMeshComponent* HelmetMesh;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    UStaticMeshComponent* WeaponMesh;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    UStaticMeshComponent* ShieldMesh;

    // Equipment functions
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool EquipItem(UBP_ItemInfo* ItemInfo, int32 InventorySlotIndex = -1);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    UBP_ItemInfo* UnequipItem(EItemEquipSlot Slot);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    UBP_ItemInfo* GetEquippedItem(EItemEquipSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool IsSlotOccupied(EItemEquipSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void UpdateEquipmentVisuals(EItemEquipSlot Slot, UBP_ItemInfo* ItemInfo);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void ClearEquipmentVisuals(EItemEquipSlot Slot);

    // Equipment validation
    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool CanEquipItem(UBP_ItemInfo* ItemInfo) const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool IsEquipmentType(UBP_ItemInfo* ItemInfo) const;

    // Initialize with mesh components from character
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void InitializeEquipmentMeshes(UStaticMeshComponent* Helmet, UStaticMeshComponent* Weapon, UStaticMeshComponent* Shield);

    // Set reference to stats component for stat modifications
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void SetStatsComponent(UCharacterStatsComponent* StatsComp);

    // Get all equipped items
    UFUNCTION(BlueprintPure, Category = "Equipment")
    TArray<UBP_ItemInfo*> GetAllEquippedItems() const;

    // Get equipment slot count
    UFUNCTION(BlueprintPure, Category = "Equipment")
    int32 GetEquipmentSlotCount() const { return static_cast<int32>(EItemEquipSlot::Accessory) + 1; }

protected:
    virtual void BeginPlay() override;

    // Reference to stats component for stat modifications
    UPROPERTY()
    UCharacterStatsComponent* StatsComponent;

    // Apply stat modifiers from equipped item
    void ApplyItemStats(UBP_ItemInfo* ItemInfo);

    // Remove stat modifiers from unequipped item
    void RemoveItemStats(UBP_ItemInfo* ItemInfo);

    // Get mesh component for equipment slot
    UStaticMeshComponent* GetMeshComponentForSlot(EItemEquipSlot Slot) const;

    // Convert equipment slot enum to array index
    int32 SlotEnumToIndex(EItemEquipSlot Slot) const;

    // Convert array index to equipment slot enum
    EItemEquipSlot IndexToSlotEnum(int32 Index) const;

    // Load and apply mesh and materials
    void LoadAndApplyEquipmentAssets(EItemEquipSlot Slot, UBP_ItemInfo* ItemInfo);

private:
    // Async loading handles for equipment assets
    TMap<EItemEquipSlot, TSharedPtr<FStreamableHandle>> LoadingHandles;

    // Callback for when equipment assets are loaded
    void OnEquipmentAssetsLoaded(EItemEquipSlot Slot, UBP_ItemInfo* ItemInfo);
}; 