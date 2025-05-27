#include "EquipmentComponent.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // Initialize equipment slots array
    const int32 SlotCount = GetEquipmentSlotCount();
    EquipmentSlots.SetNum(SlotCount);
    
    // Initialize all slots to nullptr
    for (int32 i = 0; i < SlotCount; i++)
    {
        EquipmentSlots[i] = nullptr;
    }
}

void UEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("Equipment Component initialized with %d equipment slots"), EquipmentSlots.Num());
}

void UEquipmentComponent::InitializeEquipmentMeshes(UStaticMeshComponent* Helmet, UStaticMeshComponent* Weapon, UStaticMeshComponent* Shield)
{
    HelmetMesh = Helmet;
    WeaponMesh = Weapon;
    ShieldMesh = Shield;
    
    UE_LOG(LogTemp, Log, TEXT("Equipment meshes initialized - Helmet: %s, Weapon: %s, Shield: %s"),
           HelmetMesh ? TEXT("Valid") : TEXT("Null"),
           WeaponMesh ? TEXT("Valid") : TEXT("Null"),
           ShieldMesh ? TEXT("Valid") : TEXT("Null"));
}

void UEquipmentComponent::SetStatsComponent(UCharacterStatsComponent* StatsComp)
{
    StatsComponent = StatsComp;
    UE_LOG(LogTemp, Log, TEXT("Stats component reference set: %s"), StatsComponent ? TEXT("Valid") : TEXT("Null"));
}

bool UEquipmentComponent::EquipItem(UBP_ItemInfo* ItemInfo, int32 InventorySlotIndex)
{
    if (!ItemInfo || !IsValid(ItemInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot equip null or invalid item"));
        return false;
    }

    if (!CanEquipItem(ItemInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot equip item: validation failed"));
        return false;
    }

    // Get item information from the ItemInfo
    FStructure_ItemInfo ItemData;
    bool bFound = false;
    ItemInfo->GetItemTableRow(bFound, ItemData);
    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get item data for equipping"));
        return false;
    }

    EItemEquipSlot TargetSlot = ItemData.ItemEquipSlot;
    int32 SlotIndex = SlotEnumToIndex(TargetSlot);

    if (SlotIndex < 0 || SlotIndex >= EquipmentSlots.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid equipment slot index: %d"), SlotIndex);
        return false;
    }

    // Unequip existing item if slot is occupied
    if (EquipmentSlots[SlotIndex] && IsValid(EquipmentSlots[SlotIndex]))
    {
        UnequipItem(TargetSlot);
    }

    // Equip the new item
    EquipmentSlots[SlotIndex] = ItemInfo;

    // Apply stat bonuses
    ApplyItemStats(ItemInfo);

    // Update visual appearance
    UpdateEquipmentVisuals(TargetSlot, ItemInfo);

    // Broadcast equipment change
    OnEquipmentChanged.Broadcast(TargetSlot, ItemInfo);

    UE_LOG(LogTemp, Log, TEXT("Successfully equipped item '%s' in slot %d"), 
           *ItemData.ItemName, static_cast<int32>(TargetSlot));

    return true;
}

UBP_ItemInfo* UEquipmentComponent::UnequipItem(EItemEquipSlot Slot)
{
    int32 SlotIndex = SlotEnumToIndex(Slot);

    if (SlotIndex < 0 || SlotIndex >= EquipmentSlots.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid equipment slot for unequipping: %d"), static_cast<int32>(Slot));
        return nullptr;
    }

    UBP_ItemInfo* UnequippedItem = EquipmentSlots[SlotIndex];
    if (!UnequippedItem || !IsValid(UnequippedItem))
    {
        UE_LOG(LogTemp, Warning, TEXT("No item equipped in slot %d to unequip"), static_cast<int32>(Slot));
        return nullptr;
    }

    // Remove stat bonuses
    RemoveItemStats(UnequippedItem);

    // Clear visual appearance
    ClearEquipmentVisuals(Slot);

    // Clear the slot
    EquipmentSlots[SlotIndex] = nullptr;

    // Broadcast equipment change
    OnEquipmentChanged.Broadcast(Slot, nullptr);

    FStructure_ItemInfo ItemData;
    bool bFound = false;
    UnequippedItem->GetItemTableRow(bFound, ItemData);
    if (bFound)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully unequipped item '%s' from slot %d"), 
               *ItemData.ItemName, static_cast<int32>(Slot));
    }

    return UnequippedItem;
}

UBP_ItemInfo* UEquipmentComponent::GetEquippedItem(EItemEquipSlot Slot) const
{
    int32 SlotIndex = SlotEnumToIndex(Slot);

    if (SlotIndex < 0 || SlotIndex >= EquipmentSlots.Num())
    {
        return nullptr;
    }

    return EquipmentSlots[SlotIndex];
}

bool UEquipmentComponent::IsSlotOccupied(EItemEquipSlot Slot) const
{
    UBP_ItemInfo* EquippedItem = GetEquippedItem(Slot);
    return EquippedItem && IsValid(EquippedItem);
}

void UEquipmentComponent::UpdateEquipmentVisuals(EItemEquipSlot Slot, UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo || !IsValid(ItemInfo))
    {
        ClearEquipmentVisuals(Slot);
        return;
    }

    LoadAndApplyEquipmentAssets(Slot, ItemInfo);
}

void UEquipmentComponent::ClearEquipmentVisuals(EItemEquipSlot Slot)
{
    UStaticMeshComponent* MeshComponent = GetMeshComponentForSlot(Slot);
    if (MeshComponent && IsValid(MeshComponent))
    {
        MeshComponent->SetStaticMesh(nullptr);
        MeshComponent->SetVisibility(false);
        
        UE_LOG(LogTemp, Log, TEXT("Cleared visuals for equipment slot %d"), static_cast<int32>(Slot));
    }
}

bool UEquipmentComponent::CanEquipItem(UBP_ItemInfo* ItemInfo) const
{
    if (!ItemInfo || !IsValid(ItemInfo))
    {
        return false;
    }

    return IsEquipmentType(ItemInfo);
}

bool UEquipmentComponent::IsEquipmentType(UBP_ItemInfo* ItemInfo) const
{
    if (!ItemInfo || !IsValid(ItemInfo))
    {
        return false;
    }

    FStructure_ItemInfo ItemData;
    bool bFound = false;
    ItemInfo->GetItemTableRow(bFound, ItemData);
    if (!bFound)
    {
        return false;
    }

    return ItemData.ItemType == EItemType::Equip && ItemData.ItemEquipSlot != EItemEquipSlot::None;
}

TArray<UBP_ItemInfo*> UEquipmentComponent::GetAllEquippedItems() const
{
    TArray<UBP_ItemInfo*> EquippedItems;
    
    for (UBP_ItemInfo* Item : EquipmentSlots)
    {
        if (Item && IsValid(Item))
        {
            EquippedItems.Add(Item);
        }
    }
    
    return EquippedItems;
}

void UEquipmentComponent::ApplyItemStats(UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo || !IsValid(ItemInfo) || !StatsComponent || !IsValid(StatsComponent))
    {
        return;
    }

    FStructure_ItemInfo ItemData;
    bool bFound = false;
    ItemInfo->GetItemTableRow(bFound, ItemData);
    if (!bFound)
    {
        return;
    }

    StatsComponent->AddStatModifier(
        ItemData.STR,
        ItemData.DEX,
        ItemData.INT,
        ItemData.Defence,
        ItemData.Damage,
        ItemData.HP,
        ItemData.MP
    );

    UE_LOG(LogTemp, Log, TEXT("Applied stats from item '%s': STR+%d, DEX+%d, INT+%d, DEF+%d, DMG+%d, HP+%d, MP+%d"),
           *ItemData.ItemName, ItemData.STR, ItemData.DEX, ItemData.INT, 
           ItemData.Defence, ItemData.Damage, ItemData.HP, ItemData.MP);
}

void UEquipmentComponent::RemoveItemStats(UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo || !IsValid(ItemInfo) || !StatsComponent || !IsValid(StatsComponent))
    {
        return;
    }

    FStructure_ItemInfo ItemData;
    bool bFound = false;
    ItemInfo->GetItemTableRow(bFound, ItemData);
    if (!bFound)
    {
        return;
    }

    StatsComponent->RemoveStatModifier(
        ItemData.STR,
        ItemData.DEX,
        ItemData.INT,
        ItemData.Defence,
        ItemData.Damage,
        ItemData.HP,
        ItemData.MP
    );

    UE_LOG(LogTemp, Log, TEXT("Removed stats from item '%s': STR-%d, DEX-%d, INT-%d, DEF-%d, DMG-%d, HP-%d, MP-%d"),
           *ItemData.ItemName, ItemData.STR, ItemData.DEX, ItemData.INT, 
           ItemData.Defence, ItemData.Damage, ItemData.HP, ItemData.MP);
}

UStaticMeshComponent* UEquipmentComponent::GetMeshComponentForSlot(EItemEquipSlot Slot) const
{
    switch (Slot)
    {
        case EItemEquipSlot::Head:
            return HelmetMesh;
        case EItemEquipSlot::Weapon:
            return WeaponMesh;
        case EItemEquipSlot::Accessory: // Using shield for accessory for now
            return ShieldMesh;
        default:
            return nullptr;
    }
}

int32 UEquipmentComponent::SlotEnumToIndex(EItemEquipSlot Slot) const
{
    return static_cast<int32>(Slot) - 1; // Subtract 1 because None = 0
}

EItemEquipSlot UEquipmentComponent::IndexToSlotEnum(int32 Index) const
{
    return static_cast<EItemEquipSlot>(Index + 1); // Add 1 because None = 0
}

void UEquipmentComponent::LoadAndApplyEquipmentAssets(EItemEquipSlot Slot, UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo || !IsValid(ItemInfo))
    {
        return;
    }

    UStaticMeshComponent* MeshComponent = GetMeshComponentForSlot(Slot);
    if (!MeshComponent || !IsValid(MeshComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("No mesh component available for equipment slot %d"), static_cast<int32>(Slot));
        return;
    }

    FStructure_ItemInfo ItemData;
    bool bFound = false;
    ItemInfo->GetItemTableRow(bFound, ItemData);
    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get item data for visual update"));
        return;
    }

    // Cancel any existing loading for this slot
    if (LoadingHandles.Contains(Slot))
    {
        LoadingHandles[Slot]->CancelHandle();
        LoadingHandles.Remove(Slot);
    }

    // If we have a valid static mesh, load it asynchronously
    if (!ItemData.StaticMeshID.IsNull())
    {
        FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
        TArray<FSoftObjectPath> AssetsToLoad;
        AssetsToLoad.Add(ItemData.StaticMeshID.ToSoftObjectPath());

        TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(
            AssetsToLoad,
            FStreamableDelegate::CreateUObject(this, &UEquipmentComponent::OnEquipmentAssetsLoaded, Slot, ItemInfo)
        );

        LoadingHandles.Add(Slot, Handle);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Item '%s' has no static mesh specified"), *ItemData.ItemName);
        ClearEquipmentVisuals(Slot);
    }
}

void UEquipmentComponent::OnEquipmentAssetsLoaded(EItemEquipSlot Slot, UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo || !IsValid(ItemInfo))
    {
        return;
    }

    UStaticMeshComponent* MeshComponent = GetMeshComponentForSlot(Slot);
    if (!MeshComponent || !IsValid(MeshComponent))
    {
        return;
    }

    FStructure_ItemInfo ItemData;
    bool bFound = false;
    ItemInfo->GetItemTableRow(bFound, ItemData);
    if (!bFound)
    {
        return;
    }

    // Load the static mesh
    UStaticMesh* LoadedMesh = ItemData.StaticMeshID.LoadSynchronous();
    if (LoadedMesh && IsValid(LoadedMesh))
    {
        MeshComponent->SetStaticMesh(LoadedMesh);
        MeshComponent->SetVisibility(true);
        
        UE_LOG(LogTemp, Log, TEXT("Successfully applied equipment visual for slot %d: %s"), 
               static_cast<int32>(Slot), *ItemData.ItemName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load equipment mesh for item '%s'"), *ItemData.ItemName);
        ClearEquipmentVisuals(Slot);
    }

    // Clean up the loading handle
    LoadingHandles.Remove(Slot);
} 