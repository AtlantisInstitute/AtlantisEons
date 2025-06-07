#include "EquipmentVisualsComponent.h"
#include "AtlantisEonsCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "WBP_InventorySlot.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "UObject/SoftObjectPtr.h"

UEquipmentVisualsComponent::UEquipmentVisualsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Initialize component references to nullptr
	HelmetMesh = nullptr;
	WeaponMesh = nullptr;
	ShieldMesh = nullptr;
	
	HeadSlotWidget = nullptr;
	WeaponSlotWidget = nullptr;
	SuitSlotWidget = nullptr;
	CollectableSlotWidget = nullptr;
	
	OwnerCharacter = nullptr;
	
	UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: Constructor called"));
}

void UEquipmentVisualsComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Cache owner character reference
	CacheOwnerCharacter();
	
	UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: BeginPlay called, Owner: %s"), 
		OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("None"));
}

void UEquipmentVisualsComponent::CacheOwnerCharacter()
{
	OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ EquipmentVisualsComponent: Failed to get owner character!"));
	}
}

bool UEquipmentVisualsComponent::IsValidForEquipment() const
{
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ EquipmentVisualsComponent: No owner character!"));
		return false;
	}
	
	return true;
}

void UEquipmentVisualsComponent::InitializeEquipmentMeshes(UStaticMeshComponent* InHelmet, UStaticMeshComponent* InWeapon, UStaticMeshComponent* InShield)
{
	HelmetMesh = InHelmet;
	WeaponMesh = InWeapon;
	ShieldMesh = InShield;
	
	UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: Initialized equipment meshes - Helmet: %s, Weapon: %s, Shield: %s"),
		HelmetMesh ? TEXT("Valid") : TEXT("Null"),
		WeaponMesh ? TEXT("Valid") : TEXT("Null"),
		ShieldMesh ? TEXT("Valid") : TEXT("Null"));
}

void UEquipmentVisualsComponent::InitializeEquipmentSlotWidgets(UWBP_InventorySlot* InHeadSlot, UWBP_InventorySlot* InWeaponSlot, 
	UWBP_InventorySlot* InSuitSlot, UWBP_InventorySlot* InCollectableSlot)
{
	HeadSlotWidget = InHeadSlot;
	WeaponSlotWidget = InWeaponSlot;
	SuitSlotWidget = InSuitSlot;
	CollectableSlotWidget = InCollectableSlot;
	
	UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: Initialized slot widgets - Head: %s, Weapon: %s, Suit: %s, Collectable: %s"),
		HeadSlotWidget ? TEXT("Valid") : TEXT("Null"),
		WeaponSlotWidget ? TEXT("Valid") : TEXT("Null"),
		SuitSlotWidget ? TEXT("Valid") : TEXT("Null"),
		CollectableSlotWidget ? TEXT("Valid") : TEXT("Null"));
}

void UEquipmentVisualsComponent::EquipItemToSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, 
	const TSoftObjectPtr<UTexture2D>& Texture2D, int32 ItemIndex, 
	UMaterialInterface* MaterialInterface, UMaterialInterface* MaterialInterface2)
{
	if (!IsValidForEquipment())
	{
		return;
	}
	
	LogEquipmentChange(ItemEquipSlot, TEXT("Equipping"), ItemIndex);
	
	// Get the appropriate mesh component for this slot
	UStaticMeshComponent* TargetMeshComponent = GetEquipmentMeshComponent(ItemEquipSlot);
	if (!TargetMeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ EquipmentVisualsComponent: No mesh component for slot %d"), (int32)ItemEquipSlot);
		return;
	}
	
	// Apply mesh and materials to the component
	ApplyMeshAndMaterials(TargetMeshComponent, StaticMeshID, MaterialInterface, MaterialInterface2);
	
	// Call the Blueprint implementable event on the character to maintain compatibility
	if (OwnerCharacter)
	{
		OwnerCharacter->EquipItem(ItemEquipSlot, StaticMeshID, Texture2D.Get(), ItemIndex, MaterialInterface, MaterialInterface2);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("✅ EquipmentVisualsComponent: Successfully equipped item %d to slot %d"), ItemIndex, (int32)ItemEquipSlot);
}

void UEquipmentVisualsComponent::DisarmItemFromSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, int32 ItemIndex)
{
	if (!IsValidForEquipment())
	{
		return;
	}
	
	LogEquipmentChange(ItemEquipSlot, TEXT("Disarming"), ItemIndex);
	
	// Get the appropriate mesh component for this slot
	UStaticMeshComponent* TargetMeshComponent = GetEquipmentMeshComponent(ItemEquipSlot);
	if (!TargetMeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ EquipmentVisualsComponent: No mesh component for slot %d"), (int32)ItemEquipSlot);
		return;
	}
	
	// Clear the mesh component
	TargetMeshComponent->SetStaticMesh(nullptr);
	
	// Call the Blueprint implementable event on the character to maintain compatibility
	if (OwnerCharacter)
	{
		OwnerCharacter->DisarmItem(ItemEquipSlot, StaticMeshID, ItemIndex);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("✅ EquipmentVisualsComponent: Successfully disarmed item %d from slot %d"), ItemIndex, (int32)ItemEquipSlot);
}

void UEquipmentVisualsComponent::ProcessEquipmentChange(UBP_ItemInfo* ItemInfoRef)
{
	if (!IsValidForEquipment() || !ItemInfoRef)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ EquipmentVisualsComponent: Invalid parameters for ProcessEquipmentChange"));
		return;
	}
	
	// This component only handles visual updates - equipment logic is handled by EquipmentComponent
	// Visual updates are triggered via the EquipItemToSlot function called by the character
	UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: ProcessEquipmentChange called for item %d - visuals handled by separate EquipItemToSlot calls"), ItemInfoRef->ItemIndex);
}

UWBP_InventorySlot* UEquipmentVisualsComponent::GetEquipmentSlotWidget(EItemEquipSlot EquipSlot)
{
	switch (EquipSlot)
	{
		case EItemEquipSlot::Head:
			return HeadSlotWidget;
		case EItemEquipSlot::Weapon:
			return WeaponSlotWidget;
		case EItemEquipSlot::Body:
			return SuitSlotWidget;
		case EItemEquipSlot::Accessory:
			return CollectableSlotWidget;
		default:
			UE_LOG(LogTemp, Warning, TEXT("❌ EquipmentVisualsComponent: Unknown equipment slot: %d"), (int32)EquipSlot);
			return nullptr;
	}
}

UStaticMeshComponent* UEquipmentVisualsComponent::GetEquipmentMeshComponent(EItemEquipSlot EquipSlot)
{
	switch (EquipSlot)
	{
		case EItemEquipSlot::Head:
			return HelmetMesh;
		case EItemEquipSlot::Weapon:
			return WeaponMesh;
		case EItemEquipSlot::Body:
			// Suit doesn't have a physical mesh component in this implementation
			return nullptr;
		case EItemEquipSlot::Accessory:
			return ShieldMesh; // Assuming collectable slot uses shield mesh
		default:
			UE_LOG(LogTemp, Warning, TEXT("❌ EquipmentVisualsComponent: Unknown equipment slot: %d"), (int32)EquipSlot);
			return nullptr;
	}
}

void UEquipmentVisualsComponent::ApplyMeshAndMaterials(UStaticMeshComponent* MeshComponent, const TSoftObjectPtr<UStaticMesh>& MeshAsset,
	UMaterialInterface* Material1, UMaterialInterface* Material2)
{
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ EquipmentVisualsComponent: Null mesh component!"));
		return;
	}
	
	// Load and set the mesh
	if (UStaticMesh* LoadedMesh = MeshAsset.LoadSynchronous())
	{
		MeshComponent->SetStaticMesh(LoadedMesh);
		UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: Applied mesh: %s"), *LoadedMesh->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ EquipmentVisualsComponent: Failed to load mesh asset"));
		return;
	}
	
	// Apply materials if provided
	if (Material1)
	{
		MeshComponent->SetMaterial(0, Material1);
		UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: Applied material 1: %s"), *Material1->GetName());
	}
	
	if (Material2)
	{
		MeshComponent->SetMaterial(1, Material2);
		UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: Applied material 2: %s"), *Material2->GetName());
	}
}

void UEquipmentVisualsComponent::UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo)
{
	if (!IsValidForEquipment())
	{
		return;
	}
	
	UWBP_InventorySlot* SlotWidget = GetEquipmentSlotWidget(EquipSlot);
	if (!SlotWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ EquipmentVisualsComponent: No slot widget for equipment slot %d"), (int32)EquipSlot);
		return;
	}
	
	if (ItemInfo)
	{
		SlotWidget->UpdateSlot(ItemInfo);
		LogEquipmentChange(EquipSlot, TEXT("UI Updated"), ItemInfo->ItemIndex);
	}
	else
	{
		SlotWidget->ClearSlot();
		LogEquipmentChange(EquipSlot, TEXT("UI Cleared"), -1);
	}
}

void UEquipmentVisualsComponent::ClearEquipmentSlotUI(EItemEquipSlot EquipSlot)
{
	if (!IsValidForEquipment())
	{
		return;
	}
	
	// Simply call UpdateEquipmentSlotUI with nullptr to clear the slot
	UpdateEquipmentSlotUI(EquipSlot, nullptr);
}

void UEquipmentVisualsComponent::UpdateAllEquipmentSlotsUI()
{
	if (!IsValidForEquipment())
	{
		return;
	}
	
	// Update all equipment slot UI widgets based on current equipment state
	if (OwnerCharacter && OwnerCharacter->EquipmentSlots.Num() > 0)
	{
		// Update Head slot
		if (OwnerCharacter->EquipmentSlots.Num() > 0)
		{
			UpdateEquipmentSlotUI(EItemEquipSlot::Head, OwnerCharacter->EquipmentSlots[0]);
		}
		
		// Update Weapon slot  
		if (OwnerCharacter->EquipmentSlots.Num() > 1)
		{
			UpdateEquipmentSlotUI(EItemEquipSlot::Weapon, OwnerCharacter->EquipmentSlots[1]);
		}
		
		// Update Body slot
		if (OwnerCharacter->EquipmentSlots.Num() > 2)
		{
			UpdateEquipmentSlotUI(EItemEquipSlot::Body, OwnerCharacter->EquipmentSlots[2]);
		}
		
		// Update Accessory slot
		if (OwnerCharacter->EquipmentSlots.Num() > 3)
		{
			UpdateEquipmentSlotUI(EItemEquipSlot::Accessory, OwnerCharacter->EquipmentSlots[3]);
		}
		
		UE_LOG(LogTemp, Warning, TEXT("✅ EquipmentVisualsComponent: Updated all equipment slot UIs"));
	}
}

void UEquipmentVisualsComponent::HandleEquipmentSlotClicked(EItemEquipSlot EquipSlot)
{
	if (!IsValidForEquipment())
	{
		return;
	}
	
	// This component only handles visuals - equipment slot click logic is handled by EquipmentComponent
	// Just log that a visual component received the click
	LogEquipmentChange(EquipSlot, TEXT("Slot Clicked (visual component)"), -1);
}

void UEquipmentVisualsComponent::LogEquipmentChange(EItemEquipSlot EquipSlot, const FString& Action, int32 ItemIndex)
{
	FString SlotName;
	switch (EquipSlot)
	{
		case EItemEquipSlot::Head:
			SlotName = TEXT("Head");
			break;
		case EItemEquipSlot::Weapon:
			SlotName = TEXT("Weapon");
			break;
		case EItemEquipSlot::Body:
			SlotName = TEXT("Suit");
			break;
		case EItemEquipSlot::Accessory:
			SlotName = TEXT("Collectable");
			break;
		default:
			SlotName = TEXT("Unknown");
			break;
	}
	
	if (ItemIndex >= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: %s item %d in slot %s"), *Action, ItemIndex, *SlotName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: %s in slot %s"), *Action, *SlotName);
	}
} 