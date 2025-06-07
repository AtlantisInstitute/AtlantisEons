#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "BP_ItemInfo.h"
#include "EquipmentVisualsComponent.generated.h"

class UStaticMeshComponent;
class UWBP_InventorySlot;

/**
 * EQUIPMENT VISUALS COMPONENT
 * 
 * Handles all equipment visual updates for the character including:
 * - Physical mesh swapping (helmet, weapon, shield)
 * - Material application and updates
 * - Equipment slot UI synchronization
 * - Visual feedback for equipment changes
 * 
 * Extracted from AAtlantisEonsCharacter to maintain separation of concerns
 * while preserving all Blueprint functionality and interfaces.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UEquipmentVisualsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentVisualsComponent();

	/** Initialize component with character's equipment mesh components */
	UFUNCTION(BlueprintCallable, Category = "Equipment Visuals")
	void InitializeEquipmentMeshes(UStaticMeshComponent* InHelmet, UStaticMeshComponent* InWeapon, UStaticMeshComponent* InShield);

	/** Initialize equipment slot UI widget references */
	UFUNCTION(BlueprintCallable, Category = "Equipment Visuals")
	void InitializeEquipmentSlotWidgets(UWBP_InventorySlot* InHeadSlot, UWBP_InventorySlot* InWeaponSlot, 
		UWBP_InventorySlot* InSuitSlot, UWBP_InventorySlot* InCollectableSlot);

	// ========== PRIMARY EQUIPMENT FUNCTIONS ==========
	
	/** Equip item to specific slot with full visual update */
	UFUNCTION(BlueprintCallable, Category = "Equipment Visuals")
	void EquipItemToSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, 
		const TSoftObjectPtr<UTexture2D>& Texture2D, int32 ItemIndex, 
		UMaterialInterface* MaterialInterface = nullptr, UMaterialInterface* MaterialInterface2 = nullptr);

	/** Remove item from specific slot */
	UFUNCTION(BlueprintCallable, Category = "Equipment Visuals")
	void DisarmItemFromSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, int32 ItemIndex);

	/** Process equipment of item with full validation and visual updates */
	UFUNCTION(BlueprintCallable, Category = "Equipment Visuals")
	void ProcessEquipmentChange(UBP_ItemInfo* ItemInfoRef);

	// ========== UI SYNCHRONIZATION ==========
	
	/** Update specific equipment slot UI with item info */
	UFUNCTION(BlueprintCallable, Category = "Equipment Visuals")
	void UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo);

	/** Clear specific equipment slot UI */
	UFUNCTION(BlueprintCallable, Category = "Equipment Visuals")
	void ClearEquipmentSlotUI(EItemEquipSlot EquipSlot);

	/** Update all equipment slot UIs */
	UFUNCTION(BlueprintCallable, Category = "Equipment Visuals")
	void UpdateAllEquipmentSlotsUI();

	// ========== EQUIPMENT SLOT EVENT HANDLING ==========
	
	/** Handle equipment slot click events */
	UFUNCTION(BlueprintCallable, Category = "Equipment Visuals")
	void HandleEquipmentSlotClicked(EItemEquipSlot EquipSlot);

protected:
	virtual void BeginPlay() override;

	// ========== EQUIPMENT MESH COMPONENT REFERENCES ==========
	
	/** Reference to character's helmet mesh component */
	UPROPERTY()
	UStaticMeshComponent* HelmetMesh;

	/** Reference to character's weapon mesh component */
	UPROPERTY()
	UStaticMeshComponent* WeaponMesh;

	/** Reference to character's shield mesh component */
	UPROPERTY()
	UStaticMeshComponent* ShieldMesh;

	// ========== EQUIPMENT SLOT UI WIDGET REFERENCES ==========
	
	/** Reference to head equipment slot widget */
	UPROPERTY()
	UWBP_InventorySlot* HeadSlotWidget;

	/** Reference to weapon equipment slot widget */
	UPROPERTY()
	UWBP_InventorySlot* WeaponSlotWidget;

	/** Reference to suit equipment slot widget */
	UPROPERTY()
	UWBP_InventorySlot* SuitSlotWidget;

	/** Reference to collectable equipment slot widget */
	UPROPERTY()
	UWBP_InventorySlot* CollectableSlotWidget;

	// ========== HELPER FUNCTIONS ==========
	
	/** Get equipment slot widget by slot type */
	UWBP_InventorySlot* GetEquipmentSlotWidget(EItemEquipSlot EquipSlot);

	/** Get equipment mesh component by slot type */
	UStaticMeshComponent* GetEquipmentMeshComponent(EItemEquipSlot EquipSlot);

	/** Apply mesh and materials to equipment component */
	void ApplyMeshAndMaterials(UStaticMeshComponent* MeshComponent, const TSoftObjectPtr<UStaticMesh>& MeshAsset,
		UMaterialInterface* Material1 = nullptr, UMaterialInterface* Material2 = nullptr);

	/** Log equipment change for debugging */
	void LogEquipmentChange(EItemEquipSlot EquipSlot, const FString& Action, int32 ItemIndex = -1);

private:
	/** Owner character reference for accessing equipment arrays */
	UPROPERTY()
	class AAtlantisEonsCharacter* OwnerCharacter;

	/** Cache owner character reference */
	void CacheOwnerCharacter();

	/** Validate component is properly initialized */
	bool IsValidForEquipment() const;
}; 