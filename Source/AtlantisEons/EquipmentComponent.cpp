#include "EquipmentComponent.h"
#include "AtlantisEonsCharacter.h"
#include "WBP_CharacterInfo.h"
#include "StoreSystemFix.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"

UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // Initialize equipment settings
    MaxEquipmentSlots = 10;
}

void UEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Get owner character reference
    OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("EquipmentComponent: Owner is not an AAtlantisEonsCharacter!"));
        return;
    }
    
    // Get equipment mesh component references from character
    Helmet = OwnerCharacter->Helmet;
    Weapon = OwnerCharacter->Weapon;
    Shield = OwnerCharacter->Shield;
    
    // Initialize the equipment system
    InitializeEquipment();
    
    UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: Initialized for character %s"), *OwnerCharacter->GetName());
}

void UEquipmentComponent::InitializeEquipment()
{
    // Initialize equipment array
    EquipmentSlots.SetNum(MaxEquipmentSlots);
    
    // Clear all slots
    for (int32 i = 0; i < EquipmentSlots.Num(); ++i)
    {
        EquipmentSlots[i] = nullptr;
    }
    
    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: Initialized %d equipment slots"), MaxEquipmentSlots);
}

bool UEquipmentComponent::EquipItem(UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo || !OwnerCharacter)
    {
        return false;
    }
    
    // Delegate to the main equip function
    EquipInventoryItem(ItemInfo);
    return true;
}

bool UEquipmentComponent::UnequipItem(EItemEquipSlot EquipSlot)
{
    int32 SlotIndex = GetEquipSlotIndex(EquipSlot);
    if (SlotIndex >= 0 && SlotIndex < EquipmentSlots.Num() && EquipmentSlots[SlotIndex])
    {
        UBP_ItemInfo* ItemToUnequip = EquipmentSlots[SlotIndex];
        UnequipInventoryItem(ItemToUnequip);
        return true;
    }
    return false;
}

bool UEquipmentComponent::UnequipItemByInfo(UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo)
    {
        return false;
    }
    
    UnequipInventoryItem(ItemInfo);
    return true;
}

UBP_ItemInfo* UEquipmentComponent::GetEquippedItem(EItemEquipSlot EquipSlot) const
{
    int32 SlotIndex = GetEquipSlotIndex(EquipSlot);
    if (SlotIndex >= 0 && SlotIndex < EquipmentSlots.Num())
    {
        return EquipmentSlots[SlotIndex];
    }
    return nullptr;
}

bool UEquipmentComponent::IsSlotEmpty(EItemEquipSlot EquipSlot) const
{
    return GetEquippedItem(EquipSlot) == nullptr;
}

bool UEquipmentComponent::HasItemEquipped(int32 ItemIndex) const
{
    for (UBP_ItemInfo* EquippedItem : EquipmentSlots)
    {
        if (EquippedItem && EquippedItem->ItemIndex == ItemIndex)
        {
            return true;
        }
    }
    return false;
}

void UEquipmentComponent::EquipItemInSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, const TSoftObjectPtr<UTexture2D>& Texture2D, int32 ItemIndex, UMaterialInterface* MaterialInterface, UMaterialInterface* MaterialInterface2)
{
    UE_LOG(LogTemp, Warning, TEXT("🔧 EquipItemInSlot called - Item %d, Slot: %d"), ItemIndex, static_cast<int32>(ItemEquipSlot));
    
    // Find the appropriate mesh component based on slot
    UStaticMeshComponent* TargetComponent = GetMeshComponentForSlot(ItemEquipSlot);
    FString SlotName = GetSlotName(ItemEquipSlot);
    
    UE_LOG(LogTemp, Warning, TEXT("🔧 Slot: %s, TargetComponent: %s"), *SlotName, TargetComponent ? TEXT("Valid") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧 StaticMeshID IsNull: %s"), StaticMeshID.IsNull() ? TEXT("YES") : TEXT("NO"));
    
    if (TargetComponent)
    {
        // Clear current mesh first
        TargetComponent->SetStaticMesh(nullptr);
        TargetComponent->SetVisibility(false);
        
        // Load the new mesh asynchronously
        if (!StaticMeshID.IsNull())
        {
            UE_LOG(LogTemp, Warning, TEXT("🔧 Attempting to load mesh for %s slot..."), *SlotName);
            
            // Try to load the mesh synchronously first (for immediate effect)
            UStaticMesh* LoadedMesh = StaticMeshID.LoadSynchronous();
            if (LoadedMesh)
            {
                TargetComponent->SetStaticMesh(LoadedMesh);
                TargetComponent->SetVisibility(true);
                
                // Apply materials if provided
                if (MaterialInterface)
                {
                    TargetComponent->SetMaterial(0, MaterialInterface);
                }
                if (MaterialInterface2)
                {
                    TargetComponent->SetMaterial(1, MaterialInterface2);
                }
                
                // Apply weapon loading system (scale, materials, and configurations)
                ApplyWeaponLoadingSystem(TargetComponent, ItemIndex, ItemEquipSlot);
                
                UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: ✅ Successfully equipped mesh in %s slot (Item %d)"), *SlotName, ItemIndex);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: ❌ Failed to load mesh for %s slot (Item %d)"), *SlotName, ItemIndex);
                
                // Try async loading as fallback
                FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
                StreamableManager.RequestAsyncLoad(StaticMeshID.ToSoftObjectPath(), 
                    FStreamableDelegate::CreateUObject(this, &UEquipmentComponent::OnMeshLoadedAsync, TargetComponent, ItemIndex, SlotName, StaticMeshID));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: No mesh ID provided for %s slot"), *SlotName);
        }
        
        // Note: SwordBloom attachment is handled separately in character
    }
    else if (ItemEquipSlot == EItemEquipSlot::Body)
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: Body equipment visual skipped - no body mesh component available"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: No mesh component found for %s slot"), *SlotName);
    }
}

void UEquipmentComponent::HandleDisarmItem(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, int32 ItemIndex)
{
    // Find the appropriate mesh component based on slot
    UStaticMeshComponent* TargetComponent = GetMeshComponentForSlot(ItemEquipSlot);
    FString SlotName = GetSlotName(ItemEquipSlot);
    
    if (TargetComponent)
    {
        TargetComponent->SetStaticMesh(nullptr);
        TargetComponent->SetVisibility(false);
        
        UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: Disarmed item from %s slot"), *SlotName);
    }
    else if (ItemEquipSlot == EItemEquipSlot::Body)
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: Body equipment unequip skipped - no body mesh component"));
    }
}

void UEquipmentComponent::ProcessEquipItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!ItemInfoRef || !OwnerCharacter) return;

    // Get item information from the data table
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfoRef->GetItemTableRow(bFound, ItemData);
    
    if (!bFound)
    {
        UE_LOG(LogTemp, Error, TEXT("EquipmentComponent::ProcessEquipItem: Failed to get item data for item index %d"), ItemInfoRef->ItemIndex);
        return;
    }

    // Verify this is an equipable item
    if (ItemData.ItemType != EItemType::Equip)
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent::ProcessEquipItem: Item '%s' is not equipable"), *ItemData.ItemName);
        return;
    }

    // Get the item's equipment slot and mesh from the data table
    EItemEquipSlot EquipSlot = ItemData.ItemEquipSlot;
    TSoftObjectPtr<UStaticMesh> MeshID = ItemData.StaticMeshID;
    TSoftObjectPtr<UTexture2D> Thumbnail = ItemData.ItemThumbnail;
    int32 ItemIndex = ItemInfoRef->ItemIndex;

    // Equip the item visually
    EquipItemInSlot(EquipSlot, MeshID, Thumbnail, ItemIndex, nullptr, nullptr);

    // Update character stats through character
    if (OwnerCharacter)
    {
        OwnerCharacter->AddingCharacterStatus(ItemIndex);
        OwnerCharacter->UpdateAllStats();
        OwnerCharacter->RefreshStatsDisplay();
    }
    
    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent::ProcessEquipItem: Successfully processed equipment for '%s'"), *ItemData.ItemName);
}

void UEquipmentComponent::ProcessConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (!ItemInfoRef || !InventorySlotRef || !OwnerCharacter) return;

    // Apply recovery effects through character
    if (ItemType == EItemType::Consume_HP)
    {
        OwnerCharacter->RecoverHealth(RecoverHP);
    }
    else if (ItemType == EItemType::Consume_MP)
    {
        OwnerCharacter->RecoverMP(RecoverMP);
    }

    // Clear the inventory slot
    InventorySlotRef->ClearSlot();
}

void UEquipmentComponent::UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo)
{
    // ALWAYS try to get fresh widget references first
    if (!WBP_CharacterInfo)
    {
        // Method 1: Try MainWidget
        if (MainWidget && MainWidget->GetCharacterInfoWidget())
        {
            WBP_CharacterInfo = MainWidget->GetCharacterInfoWidget();
            UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent::UpdateEquipmentSlotUI: ✅ Got WBP_CharacterInfo from MainWidget"));
        }
        // Method 2: Try character reference
        else if (OwnerCharacter && OwnerCharacter->WBP_CharacterInfo)
        {
            WBP_CharacterInfo = OwnerCharacter->WBP_CharacterInfo;
            UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent::UpdateEquipmentSlotUI: ✅ Got WBP_CharacterInfo from character"));
        }
    }
    
    if (WBP_CharacterInfo)
    {
        // Always refresh widget references to prevent null issues
        HeadSlot = WBP_CharacterInfo->HeadSlot;
        WeaponSlot = WBP_CharacterInfo->WeaponSlot;
        SuitSlot = WBP_CharacterInfo->SuitSlot;
        CollectableSlot = WBP_CharacterInfo->CollectableSlot;
        
        UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent::UpdateEquipmentSlotUI: ✅ Updated widget references - HeadSlot: %s, WeaponSlot: %s, SuitSlot: %s, CollectableSlot: %s"),
               HeadSlot ? TEXT("Valid") : TEXT("NULL"),
               WeaponSlot ? TEXT("Valid") : TEXT("NULL"),
               SuitSlot ? TEXT("Valid") : TEXT("NULL"),
               CollectableSlot ? TEXT("Valid") : TEXT("NULL"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("EquipmentComponent::UpdateEquipmentSlotUI: ❌ WBP_CharacterInfo is still null - cannot update equipment slot UI"));
        return;
    }

    // Now update the specific equipment slot
    UWBP_InventorySlot* TargetSlot = nullptr;
    FString SlotName;

    switch (EquipSlot)
    {
        case EItemEquipSlot::Head:
            TargetSlot = HeadSlot;
            SlotName = TEXT("Head");
            break;
        case EItemEquipSlot::Body:
            TargetSlot = SuitSlot;
            SlotName = TEXT("Body");
            break;
        case EItemEquipSlot::Weapon:
            TargetSlot = WeaponSlot;
            SlotName = TEXT("Weapon");
            break;
        case EItemEquipSlot::Accessory:
            TargetSlot = CollectableSlot;
            SlotName = TEXT("Shield/Accessory");
            break;
    }

    if (TargetSlot)
    {
        if (ItemInfo)
        {
            TargetSlot->UpdateSlot(ItemInfo);
            UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent::UpdateEquipmentSlotUI: ✅ Updated %s slot with item %d (%s)"), 
                   *SlotName, ItemInfo->ItemIndex, *ItemInfo->ItemName);
        }
        else
        {
            TargetSlot->ClearSlot();
            UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent::UpdateEquipmentSlotUI: ✅ Cleared %s slot"), *SlotName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("EquipmentComponent::UpdateEquipmentSlotUI: ❌ %s slot widget is NULL"), *SlotName);
    }
}

void UEquipmentComponent::ClearEquipmentSlotUI(EItemEquipSlot EquipSlot)
{
    UpdateEquipmentSlotUI(EquipSlot, nullptr);
}

void UEquipmentComponent::UpdateAllEquipmentSlotsUI()
{
    // Update all equipment slots based on currently equipped items
    for (int32 i = 0; i < EquipmentSlots.Num(); ++i)
    {
        EItemEquipSlot SlotType = GetEquipSlotFromIndex(i);
        if (SlotType != EItemEquipSlot::Consumable)
        {
            // Update the UI slot with the equipped item (or clear if none)
            UpdateEquipmentSlotUI(SlotType, EquipmentSlots[i]);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: 🔄 Updated all equipment slot UIs"));
}

void UEquipmentComponent::InitializeEquipmentSlotReferences()
{
    // Get equipment slot references from the character info widget
    if (WBP_CharacterInfo)
    {
        HeadSlot = WBP_CharacterInfo->HeadSlot;
        WeaponSlot = WBP_CharacterInfo->WeaponSlot;
        SuitSlot = WBP_CharacterInfo->SuitSlot;
        CollectableSlot = WBP_CharacterInfo->CollectableSlot;
        
        UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: ✅ Initialized equipment slot references from CharacterInfo widget"));
        UE_LOG(LogTemp, Warning, TEXT("   HeadSlot: %s"), HeadSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   WeaponSlot: %s"), WeaponSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   SuitSlot: %s"), SuitSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   CollectableSlot: %s"), CollectableSlot ? TEXT("Valid") : TEXT("NULL"));
        
        // Set up slot indices for equipment slots (different from inventory slots)
        if (HeadSlot) 
        {
            HeadSlot->SetSlotIndex(100); // Use high numbers to distinguish from inventory
            HeadSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            HeadSlot->OnSlotClicked.AddDynamic(this, &UEquipmentComponent::OnHeadSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   HeadSlot configured as Equipment slot"));
        }
        if (WeaponSlot) 
        {
            WeaponSlot->SetSlotIndex(101);
            WeaponSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            WeaponSlot->OnSlotClicked.AddDynamic(this, &UEquipmentComponent::OnWeaponSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   WeaponSlot configured as Equipment slot"));
        }
        if (SuitSlot) 
        {
            SuitSlot->SetSlotIndex(102);
            SuitSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            SuitSlot->OnSlotClicked.AddDynamic(this, &UEquipmentComponent::OnSuitSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   SuitSlot configured as Equipment slot"));
        }
        if (CollectableSlot) 
        {
            CollectableSlot->SetSlotIndex(103);
            CollectableSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            CollectableSlot->OnSlotClicked.AddDynamic(this, &UEquipmentComponent::OnCollectableSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   CollectableSlot configured as Equipment slot"));
        }
        
        // Update all equipment slots after initialization
        UpdateAllEquipmentSlotsUI();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent::InitializeEquipmentSlotReferences: WBP_CharacterInfo is null"));
    }
}

void UEquipmentComponent::OnEquipmentSlotClicked(EItemEquipSlot EquipSlot)
{
    // Get the equipment slot index
    int32 EquipSlotIndex = GetEquipSlotIndex(EquipSlot);
    
    // Check if there's an item equipped in this slot
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num() && EquipmentSlots[EquipSlotIndex])
    {
        UBP_ItemInfo* EquippedItem = EquipmentSlots[EquipSlotIndex];
        
        // Get item data for logging and visual updates
        bool bFound = false;
        FStructure_ItemInfo ItemData;
        EquippedItem->GetItemTableRow(bFound, ItemData);
        
        // Find the item in the inventory and mark it as unequipped
        if (OwnerCharacter && OwnerCharacter->InventoryComp)
        {
            bool bFoundInInventory = false;
            for (int32 i = 0; i < OwnerCharacter->InventoryItems.Num(); ++i)
            {
                if (OwnerCharacter->InventoryItems[i] && 
                    OwnerCharacter->InventoryItems[i]->ItemIndex == EquippedItem->ItemIndex && 
                    OwnerCharacter->InventoryItems[i]->Equipped)
                {
                    // Mark as unequipped (keeping it in inventory)
                    OwnerCharacter->InventoryItems[i]->Equipped = false;
                    bFoundInInventory = true;
                    
                    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: ✅ Marked item in inventory slot %d as unequipped"), i);
                    break;
                }
            }
            
            if (!bFoundInInventory)
            {
                UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: ❌ Could not find equipped item in inventory"));
                EquippedItem->Equipped = false;
            }
        }
        
        // Clear the equipment slot
        EquipmentSlots[EquipSlotIndex] = nullptr;
        
        // Handle visual unequipping
        if (bFound)
        {
            HandleDisarmItem(EquipSlot, ItemData.StaticMeshID, EquippedItem->ItemIndex);
            
            // Remove stat bonuses from the unequipped item
            if (OwnerCharacter)
            {
                OwnerCharacter->SubtractingCharacterStatus(EquippedItem->ItemIndex);
            }
            
            UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: ✅ Successfully unequipped '%s' from %s slot (keeping in inventory)"), 
                   *ItemData.ItemName, *GetSlotName(EquipSlot));
        }
        
        // Clear equipment slot UI
        ClearEquipmentSlotUI(EquipSlot);
        
        // Update inventory display to remove "EQUIPPED" text
        if (OwnerCharacter && OwnerCharacter->InventoryComp)
        {
            OwnerCharacter->InventoryComp->UpdateInventorySlots();
        }
        
        // Update character stats
        if (OwnerCharacter)
        {
            OwnerCharacter->UpdateAllStats();
            OwnerCharacter->RefreshStatsDisplay();
        }
        
        // Play unequip sound
        if (UGameplayStatics::GetCurrentLevelName(this) != TEXT(""))
        {
            UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")));
        }
        
        // Show success message
        if (GEngine && bFound)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
                FString::Printf(TEXT("Unequipped %s"), *ItemData.ItemName));
        }
        
        // Broadcast unequipped event
        OnItemUnequipped.Broadcast(EquippedItem, EquipSlot);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("EquipmentComponent::OnEquipmentSlotClicked: No item equipped in this slot"));
    }
}

void UEquipmentComponent::OnHeadSlotClicked(int32 SlotIndex)
{
    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: Head slot clicked (SlotIndex: %d)"), SlotIndex);
    OnEquipmentSlotClicked(EItemEquipSlot::Head);
}

void UEquipmentComponent::OnWeaponSlotClicked(int32 SlotIndex)
{
    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: Weapon slot clicked (SlotIndex: %d)"), SlotIndex);
    OnEquipmentSlotClicked(EItemEquipSlot::Weapon);
}

void UEquipmentComponent::OnSuitSlotClicked(int32 SlotIndex)
{
    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: Suit slot clicked (SlotIndex: %d)"), SlotIndex);
    OnEquipmentSlotClicked(EItemEquipSlot::Body);
}

void UEquipmentComponent::OnCollectableSlotClicked(int32 SlotIndex)
{
    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: Collectable slot clicked (SlotIndex: %d)"), SlotIndex);
    OnEquipmentSlotClicked(EItemEquipSlot::Accessory);
}

void UEquipmentComponent::EquipInventoryItem(UBP_ItemInfo* ItemInfoRef)
{
    UE_LOG(LogTemp, Warning, TEXT("🔧 EquipInventoryItem called for ItemIndex: %d"), ItemInfoRef ? ItemInfoRef->ItemIndex : -1);
    
    // Multiple attempts to get OwnerCharacter reference
    if (!OwnerCharacter)
    {
        // Attempt 1: Try GetOwner() cast
        OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
        UE_LOG(LogTemp, Warning, TEXT("🔧 Attempt 1 - GetOwner() cast: %s"), OwnerCharacter ? TEXT("SUCCESS") : TEXT("FAILED"));
        
        // Attempt 2: Try to get from world through owner
        if (!OwnerCharacter && GetOwner())
        {
            if (UWorld* OwnerWorld = GetOwner()->GetWorld())
            {
                if (APlayerController* PC = OwnerWorld->GetFirstPlayerController())
                {
                    OwnerCharacter = Cast<AAtlantisEonsCharacter>(PC->GetPawn());
                    UE_LOG(LogTemp, Warning, TEXT("🔧 Attempt 2 - Owner world first player pawn: %s"), OwnerCharacter ? TEXT("SUCCESS") : TEXT("FAILED"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("🔧 Attempt 2 - Owner world: NULL"));
            }
        }
        
        // Attempt 3: Try to get from any available world
        if (!OwnerCharacter)
        {
            if (UWorld* World = GEngine->GetWorldContexts().Num() > 0 ? GEngine->GetWorldContexts()[0].World() : nullptr)
            {
                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                    OwnerCharacter = Cast<AAtlantisEonsCharacter>(PC->GetPawn());
                    UE_LOG(LogTemp, Warning, TEXT("🔧 Attempt 3 - Engine world first player pawn: %s"), OwnerCharacter ? TEXT("SUCCESS") : TEXT("FAILED"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("🔧 Attempt 3 - Engine world: NULL"));
            }
        }
        
        // Attempt 4: Try to get from outer object chain
        if (!OwnerCharacter)
        {
            UObject* Outer = GetOuter();
            while (Outer && !OwnerCharacter)
            {
                OwnerCharacter = Cast<AAtlantisEonsCharacter>(Outer);
                if (OwnerCharacter)
                {
                    UE_LOG(LogTemp, Warning, TEXT("🔧 Attempt 4 - Found in outer chain: SUCCESS"));
                    break;
                }
                
                // Try to get the character from the outer if it's a component
                if (UActorComponent* Component = Cast<UActorComponent>(Outer))
                {
                    OwnerCharacter = Cast<AAtlantisEonsCharacter>(Component->GetOwner());
                    if (OwnerCharacter)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("🔧 Attempt 4 - Found via outer component: SUCCESS"));
                        break;
                    }
                }
                
                Outer = Outer->GetOuter();
            }
            if (!OwnerCharacter)
            {
                UE_LOG(LogTemp, Warning, TEXT("🔧 Attempt 4 - Outer chain search: FAILED"));
            }
        }
        
        // If we successfully got OwnerCharacter, set up mesh component references
        if (OwnerCharacter)
        {
            Helmet = OwnerCharacter->Helmet;
            Weapon = OwnerCharacter->Weapon;
            Shield = OwnerCharacter->Shield;
            UE_LOG(LogTemp, Warning, TEXT("🔧 Set up mesh component references - Helmet: %s, Weapon: %s, Shield: %s"), 
                   Helmet ? TEXT("Valid") : TEXT("NULL"),
                   Weapon ? TEXT("Valid") : TEXT("NULL"),
                   Shield ? TEXT("Valid") : TEXT("NULL"));
        }
    }
    
    if (!ItemInfoRef || !OwnerCharacter) 
    {
        UE_LOG(LogTemp, Error, TEXT("🔧 ❌ EquipInventoryItem failed - ItemInfoRef: %s, OwnerCharacter: %s"), 
               ItemInfoRef ? TEXT("Valid") : TEXT("NULL"),
               OwnerCharacter ? TEXT("Valid") : TEXT("NULL"));
        
        // Additional debug info
        UE_LOG(LogTemp, Error, TEXT("🔧 Component debug - GetOwner(): %s, GetOwner()->GetWorld(): %s"), 
               GetOwner() ? TEXT("Valid") : TEXT("NULL"),
               (GetOwner() && GetOwner()->GetWorld()) ? TEXT("Valid") : TEXT("NULL"));
               
        return;
    }
    
    // Get item information from the data table
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfoRef->GetItemTableRow(bFound, ItemData);
    
    UE_LOG(LogTemp, Warning, TEXT("🔧 Item data lookup - Found: %s, ItemName: %s"), 
           bFound ? TEXT("Yes") : TEXT("No"), 
           bFound ? *ItemData.ItemName : TEXT("Unknown"));

    if (!bFound)
    {
        UE_LOG(LogTemp, Error, TEXT("EquipmentComponent::EquipInventoryItem: Failed to get item data for item index %d"), ItemInfoRef->ItemIndex);
        return;
    }

    // Verify this is an equipable item
    if (ItemData.ItemType != EItemType::Equip)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔧 Item %s is not equipable (Type: %d)"), *ItemData.ItemName, static_cast<int32>(ItemData.ItemType));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("🔧 Item is equipable - Slot: %d"), static_cast<int32>(ItemData.ItemEquipSlot));

    // Apply equipment hotfixes
    ApplyEquipmentHotfixes(ItemInfoRef, ItemData);

    // Check if the item has a valid equipment slot (Consumables are not equippable)
    if (ItemData.ItemEquipSlot == EItemEquipSlot::Consumable)
    {
        UE_LOG(LogTemp, Error, TEXT("🔧 ❌ Item %s is a consumable and cannot be equipped"), *ItemData.ItemName);
        return;
    }

    // Calculate equipment slot index
    int32 EquipSlotIndex = GetEquipSlotIndex(ItemData.ItemEquipSlot);
    UE_LOG(LogTemp, Warning, TEXT("🔧 Equipment slot index: %d for slot type %d"), EquipSlotIndex, static_cast<int32>(ItemData.ItemEquipSlot));

    // Check if there's already an item equipped in this slot
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num() && EquipmentSlots[EquipSlotIndex])
    {
        // Unequip the current item first - but keep it in inventory and mark as unequipped
        UBP_ItemInfo* CurrentEquippedItem = EquipmentSlots[EquipSlotIndex];
        if (CurrentEquippedItem)
        {
            // Mark the current item as unequipped
            CurrentEquippedItem->Equipped = false;
            
            // Handle visual unequipping
            HandleDisarmItem(ItemData.ItemEquipSlot, CurrentEquippedItem->MeshID, CurrentEquippedItem->ItemIndex);
            
            // Clear equipment slot UI
            ClearEquipmentSlotUI(ItemData.ItemEquipSlot);
            
            // Remove stat bonuses from the unequipped item
            if (OwnerCharacter)
            {
                OwnerCharacter->SubtractingCharacterStatus(CurrentEquippedItem->ItemIndex);
            }
        }
    }
    
    // Mark the item as equipped (DON'T remove from inventory)
    ItemInfoRef->Equipped = true;
    
    // Equip the new item
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num())
    {
        EquipmentSlots[EquipSlotIndex] = ItemInfoRef;
        
        // Handle visual equipping
        UE_LOG(LogTemp, Warning, TEXT("🔧 About to call EquipItemInSlot - ItemIndex: %d, Slot: %d"), ItemInfoRef->ItemIndex, static_cast<int32>(ItemData.ItemEquipSlot));
        EquipItemInSlot(ItemData.ItemEquipSlot, ItemData.StaticMeshID, ItemData.ItemThumbnail, 
                       ItemInfoRef->ItemIndex, nullptr, nullptr);
        UE_LOG(LogTemp, Warning, TEXT("🔧 EquipItemInSlot call completed"));
        
        // Apply stat bonuses from the equipped item
        if (OwnerCharacter)
        {
            OwnerCharacter->AddingCharacterStatus(ItemInfoRef->ItemIndex);
        }
        
        // FORCE inventory display update to show correct "Equipped" status
        if (OwnerCharacter && OwnerCharacter->InventoryComp)
        {
            // Force a complete refresh of all inventory slots
            OwnerCharacter->InventoryComp->UpdateInventorySlots();
            UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ FORCED complete inventory UI refresh"));
        }
        
        // Update equipment slot UI
        UpdateEquipmentSlotUI(ItemData.ItemEquipSlot, ItemInfoRef);
        
        // Update character stats
        if (OwnerCharacter)
        {
            OwnerCharacter->UpdateAllStats();
            OwnerCharacter->RefreshStatsDisplay();
        }
        
        // Play equip sound
        if (UGameplayStatics::GetCurrentLevelName(this) != TEXT(""))
        {
            UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")));
        }
        
        // Show success message
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
                FString::Printf(TEXT("Equipped %s"), *ItemData.ItemName));
        }
        
        // Broadcast equipped event
        OnItemEquipped.Broadcast(ItemInfoRef, ItemData.ItemEquipSlot);
        
        UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: ✅ Successfully equipped '%s' in %s slot (keeping in inventory)"), 
               *ItemData.ItemName, *GetSlotName(ItemData.ItemEquipSlot));
    }
}

void UEquipmentComponent::EquipInventoryItemWithCharacter(UBP_ItemInfo* ItemInfoRef, AAtlantisEonsCharacter* CharacterRef)
{
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 EquipInventoryItemWithCharacter called for ItemIndex: %d"), ItemInfoRef ? ItemInfoRef->ItemIndex : -1);
    
    // Use the provided character reference directly - no world context issues
    if (!CharacterRef)
    {
        UE_LOG(LogTemp, Error, TEXT("🔧🔄 ❌ EquipInventoryItemWithCharacter failed - CharacterRef is NULL"));
        return;
    }
    
    // Set the OwnerCharacter reference directly
    OwnerCharacter = CharacterRef;
    
    // Set up mesh component references
    Helmet = OwnerCharacter->Helmet;
    Weapon = OwnerCharacter->Weapon;
    Shield = OwnerCharacter->Shield;
    
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 Using provided character reference - Helmet: %s, Weapon: %s, Shield: %s"), 
           Helmet ? TEXT("Valid") : TEXT("NULL"),
           Weapon ? TEXT("Valid") : TEXT("NULL"),
           Shield ? TEXT("Valid") : TEXT("NULL"));
    
    // ENSURE equipment slots array is properly sized
    if (EquipmentSlots.Num() < 4)
    {
        EquipmentSlots.SetNum(4);
        UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ Resized EquipmentSlots array to 4 elements"));
    }
    
    // Ensure we have proper widget references - try multiple methods to get them
    if (!WBP_CharacterInfo)
    {
        // Method 1: Try MainWidget
        if (MainWidget && MainWidget->GetCharacterInfoWidget())
        {
            WBP_CharacterInfo = MainWidget->GetCharacterInfoWidget();
            UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ Got WBP_CharacterInfo from MainWidget"));
        }
        // Method 2: Try character reference
        else if (OwnerCharacter && OwnerCharacter->WBP_CharacterInfo)
        {
            WBP_CharacterInfo = OwnerCharacter->WBP_CharacterInfo;
            UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ Got WBP_CharacterInfo from character"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ⚠️ WBP_CharacterInfo still null - equipment slots won't update visually"));
        }
    }
    
    if (WBP_CharacterInfo)
    {
        // Force reinitialize widget references every time to prevent null issues
        HeadSlot = WBP_CharacterInfo->HeadSlot;
        WeaponSlot = WBP_CharacterInfo->WeaponSlot;
        SuitSlot = WBP_CharacterInfo->SuitSlot;
        CollectableSlot = WBP_CharacterInfo->CollectableSlot;
        
        UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ Widget references updated - HeadSlot: %s, WeaponSlot: %s, SuitSlot: %s, CollectableSlot: %s"),
               HeadSlot ? TEXT("Valid") : TEXT("NULL"),
               WeaponSlot ? TEXT("Valid") : TEXT("NULL"),
               SuitSlot ? TEXT("Valid") : TEXT("NULL"),
               CollectableSlot ? TEXT("Valid") : TEXT("NULL"));
    }
    
    if (!ItemInfoRef || !OwnerCharacter) 
    {
        UE_LOG(LogTemp, Error, TEXT("🔧🔄 ❌ EquipInventoryItemWithCharacter failed - ItemInfoRef: %s, OwnerCharacter: %s"), 
               ItemInfoRef ? TEXT("Valid") : TEXT("NULL"),
               OwnerCharacter ? TEXT("Valid") : TEXT("NULL"));
        return;
    }
    
    // Get item information from the data table
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfoRef->GetItemTableRow(bFound, ItemData);
    
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 Item data lookup - Found: %s, ItemName: %s"), 
           bFound ? TEXT("Yes") : TEXT("No"), 
           bFound ? *ItemData.ItemName : TEXT("Unknown"));

    if (!bFound)
    {
        UE_LOG(LogTemp, Error, TEXT("EquipmentComponent::EquipInventoryItemWithCharacter: Failed to get item data for item index %d"), ItemInfoRef->ItemIndex);
        return;
    }

    // Verify this is an equipable item
    if (ItemData.ItemType != EItemType::Equip)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔧🔄 Item %s is not equipable (Type: %d)"), *ItemData.ItemName, static_cast<int32>(ItemData.ItemType));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 Item is equipable - Slot: %d"), static_cast<int32>(ItemData.ItemEquipSlot));

    // Apply equipment hotfixes
    ApplyEquipmentHotfixes(ItemInfoRef, ItemData);

    // Check if the item has a valid equipment slot (Consumables are not equippable)
    if (ItemData.ItemEquipSlot == EItemEquipSlot::Consumable)
    {
        UE_LOG(LogTemp, Error, TEXT("🔧🔄 ❌ Item %s is a consumable and cannot be equipped"), *ItemData.ItemName);
        return;
    }

    // Calculate equipment slot index
    int32 EquipSlotIndex = GetEquipSlotIndex(ItemData.ItemEquipSlot);
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 Equipment slot index: %d for slot type %d (Array size: %d)"), 
           EquipSlotIndex, static_cast<int32>(ItemData.ItemEquipSlot), EquipmentSlots.Num());

    // Validate slot index before accessing array
    if (EquipSlotIndex < 0 || EquipSlotIndex >= EquipmentSlots.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("🔧🔄 ❌ Invalid equipment slot index %d (Array size: %d)"), EquipSlotIndex, EquipmentSlots.Num());
        return;
    }

    // Check if there's already an item equipped in this slot - REPLACEMENT LOGIC
    UBP_ItemInfo* CurrentEquippedItem = EquipmentSlots[EquipSlotIndex];
    if (CurrentEquippedItem)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔧🔄 🔄 REPLACING currently equipped item %d (%s) with new item %d (%s)"), 
               CurrentEquippedItem->ItemIndex, 
               CurrentEquippedItem->ItemName.IsEmpty() ? TEXT("Unknown") : *CurrentEquippedItem->ItemName,
               ItemInfoRef->ItemIndex, 
               *ItemData.ItemName);
        
        // Mark the current item as unequipped
        CurrentEquippedItem->Equipped = false;
        
        // Handle visual unequipping
        HandleDisarmItem(ItemData.ItemEquipSlot, CurrentEquippedItem->MeshID, CurrentEquippedItem->ItemIndex);
        
        // Clear equipment slot UI
        ClearEquipmentSlotUI(ItemData.ItemEquipSlot);
        
        // Remove stat bonuses from the unequipped item
        if (OwnerCharacter)
        {
            OwnerCharacter->SubtractingCharacterStatus(CurrentEquippedItem->ItemIndex);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ Successfully unequipped previous item %d"), CurrentEquippedItem->ItemIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🔧🔄 No existing item in slot %d - proceeding with fresh equip"), EquipSlotIndex);
    }
    
    // Mark the item as equipped (DON'T remove from inventory)
    ItemInfoRef->Equipped = true;
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ Marked item %d as equipped"), ItemInfoRef->ItemIndex);
    
    // Equip the new item
    EquipmentSlots[EquipSlotIndex] = ItemInfoRef;
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ Set equipment slot %d to item %d"), EquipSlotIndex, ItemInfoRef->ItemIndex);
    
    // Handle visual equipping
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 About to call EquipItemInSlot - ItemIndex: %d, Slot: %d"), ItemInfoRef->ItemIndex, static_cast<int32>(ItemData.ItemEquipSlot));
    EquipItemInSlot(ItemData.ItemEquipSlot, ItemData.StaticMeshID, ItemData.ItemThumbnail, 
                   ItemInfoRef->ItemIndex, nullptr, nullptr);
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 EquipItemInSlot call completed"));
    
    // Apply stat bonuses from the equipped item
    if (OwnerCharacter)
    {
        OwnerCharacter->AddingCharacterStatus(ItemInfoRef->ItemIndex);
    }
    
    // FORCE inventory display update to show correct "Equipped" status
    if (OwnerCharacter && OwnerCharacter->InventoryComp)
    {
        // Force a complete refresh of all inventory slots
        OwnerCharacter->InventoryComp->UpdateInventorySlots();
        UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ FORCED complete inventory UI refresh"));
    }
    
    // Update equipment slot UI with proper widget references
    UpdateEquipmentSlotUI(ItemData.ItemEquipSlot, ItemInfoRef);
    
    // Update character stats
    if (OwnerCharacter)
    {
        OwnerCharacter->UpdateAllStats();
        OwnerCharacter->RefreshStatsDisplay();
    }
    
    // Play equip sound
    if (UGameplayStatics::GetCurrentLevelName(this) != TEXT(""))
    {
        UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")));
    }
    
    // Show success message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
            FString::Printf(TEXT("Equipped %s"), *ItemData.ItemName));
    }
    
    // Broadcast equipped event
    OnItemEquipped.Broadcast(ItemInfoRef, ItemData.ItemEquipSlot);
    
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 ✅ Successfully equipped '%s' in %s slot"), 
           *ItemData.ItemName, *GetSlotName(ItemData.ItemEquipSlot));
}

void UEquipmentComponent::UnequipInventoryItem(UBP_ItemInfo* ItemInfoRef)
{
    // Multiple attempts to get OwnerCharacter reference (same as EquipInventoryItem)
    if (!OwnerCharacter)
    {
        // Attempt 1: Try GetOwner() cast
        OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
        
        // Attempt 2: Try to get from world through owner
        if (!OwnerCharacter && GetOwner())
        {
            if (UWorld* OwnerWorld = GetOwner()->GetWorld())
            {
                if (APlayerController* PC = OwnerWorld->GetFirstPlayerController())
                {
                    OwnerCharacter = Cast<AAtlantisEonsCharacter>(PC->GetPawn());
                }
            }
        }
        
        // Attempt 3: Try to get from any available world
        if (!OwnerCharacter)
        {
            if (UWorld* World = GEngine->GetWorldContexts().Num() > 0 ? GEngine->GetWorldContexts()[0].World() : nullptr)
            {
                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                    OwnerCharacter = Cast<AAtlantisEonsCharacter>(PC->GetPawn());
                }
            }
        }
        
        // Attempt 4: Try to get from outer object chain
        if (!OwnerCharacter)
        {
            UObject* Outer = GetOuter();
            while (Outer && !OwnerCharacter)
            {
                OwnerCharacter = Cast<AAtlantisEonsCharacter>(Outer);
                if (OwnerCharacter) break;
                
                // Try to get the character from the outer if it's a component
                if (UActorComponent* Component = Cast<UActorComponent>(Outer))
                {
                    OwnerCharacter = Cast<AAtlantisEonsCharacter>(Component->GetOwner());
                    if (OwnerCharacter) break;
                }
                
                Outer = Outer->GetOuter();
            }
        }
        
        // If we successfully got OwnerCharacter, set up mesh component references
        if (OwnerCharacter)
        {
            Helmet = OwnerCharacter->Helmet;
            Weapon = OwnerCharacter->Weapon;
            Shield = OwnerCharacter->Shield;
        }
    }
    
    if (!ItemInfoRef || !OwnerCharacter) return;
    
    // Get item information from the data table
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfoRef->GetItemTableRow(bFound, ItemData);
    
    if (!bFound)
    {
        UE_LOG(LogTemp, Error, TEXT("EquipmentComponent::UnequipInventoryItem: Failed to get item data for item index %d"), ItemInfoRef->ItemIndex);
        return;
    }
    
    // Verify this is an equipable item and is currently equipped
    if (ItemData.ItemType != EItemType::Equip || !ItemInfoRef->Equipped)
    {
        return;
    }
    
    // Apply equipment hotfixes
    ApplyEquipmentHotfixes(ItemInfoRef, ItemData);
    
    // Calculate equipment slot index
    int32 EquipSlotIndex = GetEquipSlotIndex(ItemData.ItemEquipSlot);
    
    // Mark the item as unequipped (but keep in inventory)
    ItemInfoRef->Equipped = false;
    
    // Clear the equipment slot
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num())
    {
        EquipmentSlots[EquipSlotIndex] = nullptr;
        
        // Handle visual unequipping
        HandleDisarmItem(ItemData.ItemEquipSlot, ItemData.StaticMeshID, ItemInfoRef->ItemIndex);
        
        // Remove stat bonuses from the unequipped item
        if (OwnerCharacter)
        {
            OwnerCharacter->SubtractingCharacterStatus(ItemInfoRef->ItemIndex);
        }
        
        // Clear equipment slot UI
        ClearEquipmentSlotUI(ItemData.ItemEquipSlot);
        
        // Update inventory display to remove "Equipped" text
        if (OwnerCharacter && OwnerCharacter->InventoryComp)
        {
            OwnerCharacter->InventoryComp->UpdateInventorySlots();
        }
        
        // Update character stats
        if (OwnerCharacter)
        {
            OwnerCharacter->UpdateAllStats();
            OwnerCharacter->RefreshStatsDisplay();
        }
        
        // Play unequip sound
        if (UGameplayStatics::GetCurrentLevelName(this) != TEXT(""))
        {
            UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")));
        }
        
        // Show success message
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
                FString::Printf(TEXT("Unequipped %s"), *ItemData.ItemName));
        }
        
        // Broadcast unequipped event
        OnItemUnequipped.Broadcast(ItemInfoRef, ItemData.ItemEquipSlot);
        
        UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: ✅ Successfully unequipped '%s' from %s slot (keeping in inventory)"), 
               *ItemData.ItemName, *GetSlotName(ItemData.ItemEquipSlot));
    }
}

void UEquipmentComponent::UnequipInventoryItemWithCharacter(UBP_ItemInfo* ItemInfoRef, AAtlantisEonsCharacter* CharacterRef)
{
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 UnequipInventoryItemWithCharacter called for ItemIndex: %d"), ItemInfoRef ? ItemInfoRef->ItemIndex : -1);
    
    // Use the provided character reference directly - no world context issues
    if (!CharacterRef)
    {
        UE_LOG(LogTemp, Error, TEXT("🔧🔄 ❌ UnequipInventoryItemWithCharacter failed - CharacterRef is NULL"));
        return;
    }
    
    // Set the OwnerCharacter reference directly
    OwnerCharacter = CharacterRef;
    
    // Set up mesh component references
    Helmet = OwnerCharacter->Helmet;
    Weapon = OwnerCharacter->Weapon;
    Shield = OwnerCharacter->Shield;
    
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 Using provided character reference for unequip - Helmet: %s, Weapon: %s, Shield: %s"), 
           Helmet ? TEXT("Valid") : TEXT("NULL"),
           Weapon ? TEXT("Valid") : TEXT("NULL"),
           Shield ? TEXT("Valid") : TEXT("NULL"));
    
    if (!ItemInfoRef || !OwnerCharacter) 
    {
        UE_LOG(LogTemp, Error, TEXT("🔧🔄 ❌ UnequipInventoryItemWithCharacter failed - ItemInfoRef: %s, OwnerCharacter: %s"), 
               ItemInfoRef ? TEXT("Valid") : TEXT("NULL"),
               OwnerCharacter ? TEXT("Valid") : TEXT("NULL"));
        return;
    }
    
    // Get item information from the data table
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfoRef->GetItemTableRow(bFound, ItemData);
    
    if (!bFound)
    {
        UE_LOG(LogTemp, Error, TEXT("EquipmentComponent::UnequipInventoryItemWithCharacter: Failed to get item data for item index %d"), ItemInfoRef->ItemIndex);
        return;
    }
    
    // Verify this is an equipable item and is currently equipped
    if (ItemData.ItemType != EItemType::Equip || !ItemInfoRef->Equipped)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔧🔄 Item %s is not equipped or not equipable - skipping unequip"), *ItemData.ItemName);
        return;
    }
    
    // Apply equipment hotfixes
    ApplyEquipmentHotfixes(ItemInfoRef, ItemData);
    
    // Calculate equipment slot index
    int32 EquipSlotIndex = GetEquipSlotIndex(ItemData.ItemEquipSlot);
    UE_LOG(LogTemp, Warning, TEXT("🔧🔄 Unequip slot index: %d for slot type %d"), EquipSlotIndex, static_cast<int32>(ItemData.ItemEquipSlot));
    
    // Mark the item as unequipped (but keep in inventory)
    ItemInfoRef->Equipped = false;
    
    // Clear the equipment slot
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num())
    {
        EquipmentSlots[EquipSlotIndex] = nullptr;
        
        // Handle visual unequipping
        HandleDisarmItem(ItemData.ItemEquipSlot, ItemData.StaticMeshID, ItemInfoRef->ItemIndex);
        
        // Remove stat bonuses from the unequipped item
        if (OwnerCharacter)
        {
            OwnerCharacter->SubtractingCharacterStatus(ItemInfoRef->ItemIndex);
        }
        
        // Clear equipment slot UI
        ClearEquipmentSlotUI(ItemData.ItemEquipSlot);
        
        // Update inventory display to remove "Equipped" text
        if (OwnerCharacter && OwnerCharacter->InventoryComp)
        {
            OwnerCharacter->InventoryComp->UpdateInventorySlots();
        }
        
        // Update character stats
        if (OwnerCharacter)
        {
            OwnerCharacter->UpdateAllStats();
            OwnerCharacter->RefreshStatsDisplay();
        }
        
        // Play unequip sound
        if (UGameplayStatics::GetCurrentLevelName(this) != TEXT(""))
        {
            UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")));
        }
        
        // Show success message
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
                FString::Printf(TEXT("Unequipped %s"), *ItemData.ItemName));
        }
        
        // Broadcast unequipped event
        OnItemUnequipped.Broadcast(ItemInfoRef, ItemData.ItemEquipSlot);
        
        UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: ✅ Successfully unequipped '%s' from %s slot (keeping in inventory)"), 
               *ItemData.ItemName, *GetSlotName(ItemData.ItemEquipSlot));
    }
}

void UEquipmentComponent::SetMainWidget(UWBP_Main* NewMainWidget)
{
    MainWidget = NewMainWidget;
    
    if (MainWidget)
    {
        UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: Main widget set successfully"));
        
        // Get the character info widget from the main widget
        if (MainWidget->GetCharacterInfoWidget())
        {
            WBP_CharacterInfo = MainWidget->GetCharacterInfoWidget();
            UE_LOG(LogTemp, Display, TEXT("EquipmentComponent: Character info widget set from main widget"));
            
            // Initialize equipment slot references
            InitializeEquipmentSlotReferences();
        }
    }
}

void UEquipmentComponent::ClearAllEquipment()
{
    for (int32 i = 0; i < EquipmentSlots.Num(); ++i)
    {
        if (EquipmentSlots[i])
        {
            // Unequip the item
            UnequipInventoryItem(EquipmentSlots[i]);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: Cleared all equipment"));
}

int32 UEquipmentComponent::GetEquipSlotIndex(EItemEquipSlot EquipSlot) const
{
    switch (EquipSlot)
    {
        case EItemEquipSlot::Head:
            return 0;
        case EItemEquipSlot::Body:
            return 1;
        case EItemEquipSlot::Weapon:
            return 2;
        case EItemEquipSlot::Accessory:
            return 3;
        default:
            return -1;
    }
}

EItemEquipSlot UEquipmentComponent::GetEquipSlotFromIndex(int32 SlotIndex) const
{
    switch (SlotIndex)
    {
        case 0:
            return EItemEquipSlot::Head;
        case 1:
            return EItemEquipSlot::Body;
        case 2:
            return EItemEquipSlot::Weapon;
        case 3:
            return EItemEquipSlot::Accessory;
        default:
            return EItemEquipSlot::Consumable;
    }
}

bool UEquipmentComponent::CanEquipItem(UBP_ItemInfo* ItemInfo) const
{
    if (!ItemInfo)
    {
        return false;
    }
    
    // Get item data
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfo->GetItemTableRow(bFound, ItemData);
    
    if (!bFound)
    {
        return false;
    }
    
    // Check if it's an equipment type
    return ItemData.ItemType == EItemType::Equip;
}

EItemEquipSlot UEquipmentComponent::DetermineEquipSlot(UBP_ItemInfo* ItemInfo) const
{
    if (!ItemInfo)
    {
        return EItemEquipSlot::Consumable;
    }
    
    // Get item data
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfo->GetItemTableRow(bFound, ItemData);
    
    if (!bFound)
    {
        return EItemEquipSlot::Consumable;
    }
    
    // Apply hotfixes and return the determined slot
    FStructure_ItemInfo TempData = ItemData;
    ApplyEquipmentHotfixes(ItemInfo, TempData);
    
    return TempData.ItemEquipSlot;
}

// Private helper functions
void UEquipmentComponent::BroadcastEquipmentChanged()
{
    // Update all equipment slots UI
    UpdateAllEquipmentSlotsUI();
    
    UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: Equipment changed - updated UI"));
}

UStaticMeshComponent* UEquipmentComponent::GetMeshComponentForSlot(EItemEquipSlot EquipSlot) const
{
    switch (EquipSlot)
    {
        case EItemEquipSlot::Head:
            return Helmet;
        case EItemEquipSlot::Body:
            return nullptr; // BodyMesh component not available - skip body equipment 
        case EItemEquipSlot::Weapon:
            return Weapon;
        case EItemEquipSlot::Accessory:
            return Shield;
        default:
            return nullptr;
    }
}

FString UEquipmentComponent::GetSlotName(EItemEquipSlot EquipSlot) const
{
    switch (EquipSlot)
    {
        case EItemEquipSlot::Consumable:
            return TEXT("Consumable");
        case EItemEquipSlot::Head:
            return TEXT("Head");
        case EItemEquipSlot::Body:
            return TEXT("Body");
        case EItemEquipSlot::Weapon:
            return TEXT("Weapon");
        case EItemEquipSlot::Accessory:
            return TEXT("Shield/Accessory");
        default:
            return TEXT("Unknown");
    }
}

void UEquipmentComponent::ApplyEquipmentHotfixes(UBP_ItemInfo* ItemInfo, FStructure_ItemInfo& ItemData) const
{
    if (!ItemInfo)
    {
        return;
    }
    
    // Apply the same hotfixes for equipment slots as in the original function
    bool bIsWeapon = false;
    bool bIsShield = false;
    bool bIsHelmet = false;
    bool bIsBodyArmor = false;
    
    // Check by item index (known weapon items)
    if (ItemInfo->ItemIndex == 7 || ItemInfo->ItemIndex == 11 || ItemInfo->ItemIndex == 12 || 
        ItemInfo->ItemIndex == 13 || ItemInfo->ItemIndex == 14 || ItemInfo->ItemIndex == 15 || 
        ItemInfo->ItemIndex == 16)
    {
        bIsWeapon = true;
    }
    // Check by item name patterns for weapons
    else if (ItemData.ItemName.Contains(TEXT("Sword")) || ItemData.ItemName.Contains(TEXT("Axe")) || 
             ItemData.ItemName.Contains(TEXT("Pistol")) || ItemData.ItemName.Contains(TEXT("Rifle")) || 
             ItemData.ItemName.Contains(TEXT("Spike")) || ItemData.ItemName.Contains(TEXT("Laser")) ||
             ItemData.ItemName.Contains(TEXT("Blade")) || ItemData.ItemName.Contains(TEXT("Gun")) ||
             ItemData.ItemName.Contains(TEXT("Weapon")) || ItemData.ItemName.Contains(TEXT("Bow")) ||
             ItemData.ItemName.Contains(TEXT("Staff")) || ItemData.ItemName.Contains(TEXT("Wand")) ||
             ItemData.ItemName.Contains(TEXT("Hammer")) || ItemData.ItemName.Contains(TEXT("Mace")) ||
             ItemData.ItemName.Contains(TEXT("Spear")) || ItemData.ItemName.Contains(TEXT("Dagger")) ||
             ItemData.ItemName.Contains(TEXT("Katana")) || ItemData.ItemName.Contains(TEXT("Scythe")))
    {
        bIsWeapon = true;
    }
    // Check for shields/accessories
    else if (ItemInfo->ItemIndex == 17 || ItemInfo->ItemIndex == 18 ||
             ItemData.ItemName.Contains(TEXT("Shield")) || ItemData.ItemName.Contains(TEXT("Buckler")))
    {
        bIsShield = true;
    }
    // Check for helmets/head gear
    else if (ItemInfo->ItemIndex == 19 || ItemInfo->ItemIndex == 20 || ItemInfo->ItemIndex == 21 || ItemInfo->ItemIndex == 22 ||
             ItemData.ItemName.Contains(TEXT("Helmet")) || ItemData.ItemName.Contains(TEXT("Hat")) || 
             ItemData.ItemName.Contains(TEXT("Cap")) || ItemData.ItemName.Contains(TEXT("Crown")) ||
             ItemData.ItemName.Contains(TEXT("Mask")) || ItemData.ItemName.Contains(TEXT("Hood")))
    {
        bIsHelmet = true;
    }
    // Check for body armor
    else if (ItemInfo->ItemIndex == 23 || ItemInfo->ItemIndex == 24 || ItemInfo->ItemIndex == 25 || ItemInfo->ItemIndex == 26 ||
             ItemData.ItemName.Contains(TEXT("Suit")) || ItemData.ItemName.Contains(TEXT("Armor")) || 
             ItemData.ItemName.Contains(TEXT("Chestplate")) || ItemData.ItemName.Contains(TEXT("Vest")) ||
             ItemData.ItemName.Contains(TEXT("Robe")) || ItemData.ItemName.Contains(TEXT("Tunic")))
    {
        bIsBodyArmor = true;
    }
    
    // Apply the corrections
    if (bIsWeapon)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Weapon;
    }
    else if (bIsShield)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Accessory;
    }
    else if (bIsHelmet)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Head;
    }
    else if (bIsBodyArmor)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Body;
    }
}

void UEquipmentComponent::OnMeshLoaded(UStaticMeshComponent* TargetComponent, int32 ItemIndex, FString SlotName)
{
    if (TargetComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: ✅ Async mesh loaded for %s slot (Item %d)"), *SlotName, ItemIndex);
        TargetComponent->SetVisibility(true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: ❌ Target component null for async mesh load (%s slot, Item %d)"), *SlotName, ItemIndex);
    }
}

void UEquipmentComponent::OnMeshLoadedAsync(UStaticMeshComponent* TargetComponent, int32 ItemIndex, FString SlotName, TSoftObjectPtr<UStaticMesh> StaticMeshID)
{
    if (TargetComponent)
    {
        UStaticMesh* LoadedMesh = StaticMeshID.Get();
        if (LoadedMesh)
        {
            TargetComponent->SetStaticMesh(LoadedMesh);
            TargetComponent->SetVisibility(true);
            
            // Apply weapon loading system for async-loaded meshes
            // Note: We need to determine the EquipSlot from the component
            EItemEquipSlot EquipSlot = EItemEquipSlot::Consumable;
            if (OwnerCharacter)
            {
                if (TargetComponent == OwnerCharacter->Weapon)
                    EquipSlot = EItemEquipSlot::Weapon;
                else if (TargetComponent == OwnerCharacter->Helmet)
                    EquipSlot = EItemEquipSlot::Head;
                else if (TargetComponent == OwnerCharacter->Shield)
                    EquipSlot = EItemEquipSlot::Accessory;
            }
            ApplyWeaponLoadingSystem(TargetComponent, ItemIndex, EquipSlot);
            
            UE_LOG(LogTemp, Log, TEXT("EquipmentComponent: ✅ Async mesh loaded and set for %s slot (Item %d)"), *SlotName, ItemIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: ❌ Async loaded mesh is null for %s slot (Item %d)"), *SlotName, ItemIndex);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipmentComponent: ❌ Target component null for async mesh load (%s slot, Item %d)"), *SlotName, ItemIndex);
    }
}

void UEquipmentComponent::ApplyWeaponLoadingSystem(UStaticMeshComponent* MeshComponent, int32 ItemIndex, EItemEquipSlot EquipSlot)
{
    if (!MeshComponent)
    {
        return;
    }
    
    // Log what item we're processing in the weapon loading system
    UE_LOG(LogTemp, Warning, TEXT("⚙️ Weapon Loading System: Processing Item Index %d, Slot: %d"), ItemIndex, (int32)EquipSlot);
    
    // Apply weapon loading system based on equipment slot
    switch (EquipSlot)
    {
        case EItemEquipSlot::Weapon:
        {
            FVector NewScale;
            
            // Apply smaller scaling for items in the range 11-46
            if (ItemIndex >= 11 && ItemIndex <= 46)
            {
                // Smaller scaling for items in range 11-46 (12% instead of 25%)
                NewScale = FVector(0.12f, 0.12f, 0.12f);
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Weapon Loading System: Applied 12%% scaling to Item Index %d (Range 11-46): Scale set to %f"), ItemIndex, NewScale.X);
            }
            else
            {
                // Standard 25% scaling for other weapons
                NewScale = FVector(0.25f, 0.25f, 0.25f);
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Weapon Loading System: Applied 25%% scaling to Item Index %d: Scale set to %f"), ItemIndex, NewScale.X);
            }
            
            MeshComponent->SetRelativeScale3D(NewScale);
            
            // Load and apply standard weapon materials to all weapons
            LoadWeaponMaterials(MeshComponent, ItemIndex);
            
            break;
        }
        
        case EItemEquipSlot::Head:
        case EItemEquipSlot::Body:
        case EItemEquipSlot::Accessory:
        case EItemEquipSlot::Consumable:
        default:
        {
            // For non-weapon items, use default scale (no weapon loading system applied)
            MeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
            UE_LOG(LogTemp, Log, TEXT("🔧 Weapon Loading System: Applied default scaling to Item Index %d (Slot: %d): Scale set to 1.0"), ItemIndex, (int32)EquipSlot);
            break;
        }
    }
}

void UEquipmentComponent::LoadWeaponMaterials(UStaticMeshComponent* MeshComponent, int32 ItemIndex)
{
    if (!MeshComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ Weapon Loading System: MeshComponent is null for Item %d"), ItemIndex);
        return;
    }

    // Define the standard weapon material paths for the loading system
    TArray<FString> MaterialPaths = {
        TEXT("/Game/AtlantisEons/Sources/Material/Swords/Materials/M_gold"),             // Element 0
        TEXT("/Game/AtlantisEons/Sources/Material/Swords/Materials/M_gold_silver"),      // Element 1
        TEXT("/Game/AtlantisEons/Sources/Material/Swords/Materials/M_gold_blue"),        // Element 2
        TEXT("/Game/AtlantisEons/Sources/Material/Swords/Materials/M_chrome_black"),     // Element 3
        TEXT("/Game/AtlantisEons/Sources/Material/Swords/Materials/M_chrome_white")      // Element 4
    };

    // Apply each material to its corresponding material slot
    for (int32 MaterialIndex = 0; MaterialIndex < MaterialPaths.Num(); MaterialIndex++)
    {
        // Load the material
        UMaterialInterface* LoadedMaterial = Cast<UMaterialInterface>(
            StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *MaterialPaths[MaterialIndex])
        );

        if (LoadedMaterial)
        {
            // Apply the material to the mesh component
            MeshComponent->SetMaterial(MaterialIndex, LoadedMaterial);
            UE_LOG(LogTemp, Log, TEXT("✅ Weapon Loading System: Loaded material %d (%s) for weapon Item %d"), 
                   MaterialIndex, *MaterialPaths[MaterialIndex], ItemIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ Weapon Loading System: Failed to load material %d (%s) for weapon Item %d"), 
                   MaterialIndex, *MaterialPaths[MaterialIndex], ItemIndex);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("🎨 Weapon Loading System: Successfully loaded all materials for Item Index %d"), ItemIndex);
} 