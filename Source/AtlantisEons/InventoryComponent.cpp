#include "InventoryComponent.h"
#include "AtlantisEonsCharacter.h"
#include "BP_ConcreteItemInfo.h"
#include "WBP_Inventory.h"
#include "StoreSystemFix.h"
#include "AtlantisEonsGameInstance.h"
#include "UniversalItemLoader.h"
#include "BP_Item.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // Initialize inventory size
    InventorySize = 30;
    MaxStackSize = 99;
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Get owner character reference
    OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryComponent: Owner is not an AAtlantisEonsCharacter!"));
        return;
    }
    
    // Initialize the inventory
    InitializeInventory();
    
    UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: Initialized for character %s"), *OwnerCharacter->GetName());
}

void UInventoryComponent::InitializeInventory()
{
    // Initialize inventory array
    InventoryItems.SetNum(InventorySize);
    
    // Clear all slots
    for (int32 i = 0; i < InventoryItems.Num(); ++i)
    {
        InventoryItems[i] = nullptr;
    }
    
    UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Initialized %d inventory slots"), InventorySize);
}

bool UInventoryComponent::AddItem(int32 ItemIndex, int32 StackNumber)
{
    if (ItemIndex < 0 || StackNumber <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: Invalid item parameters - Index: %d, Stack: %d"), ItemIndex, StackNumber);
        return false;
    }

    // Get item data
    FStructure_ItemInfo ItemData;
    if (!GetItemDataForIndex(ItemIndex, ItemData))
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryComponent: Failed to get item data for index %d"), ItemIndex);
        return false;
    }

    // Create item info object
    UBP_ConcreteItemInfo* NewItemInfo = CreateItemFromData(ItemData, StackNumber);
    if (!NewItemInfo)
    {
        return false;
    }

    return AddItemInfo(NewItemInfo);
}

bool UInventoryComponent::AddItemInfo(UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo || !ItemInfo->bIsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: Attempted to add invalid item"));
        return false;
    }

    bool bItemAdded = false;
    int32 RemainingStack = ItemInfo->StackNumber;

    // First try to stack with existing items if stackable
    if (ItemInfo->bIsStackable)
    {
        for (int32 i = 0; i < InventoryItems.Num(); ++i)
        {
            if (InventoryItems[i] && InventoryItems[i]->ItemIndex == ItemInfo->ItemIndex)
            {
                // Calculate how much we can add to this stack
                int32 SpaceInStack = MaxStackSize - InventoryItems[i]->StackNumber;
                if (SpaceInStack > 0)
                {
                    int32 AmountToAdd = FMath::Min(RemainingStack, SpaceInStack);
                    InventoryItems[i]->StackNumber += AmountToAdd;
                    RemainingStack -= AmountToAdd;
                    bItemAdded = true;

                    UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Stacked %d items at slot %d"), AmountToAdd, i);

                    if (RemainingStack <= 0)
                    {
                        break;
                    }
                }
            }
        }
    }

    // If we still have items to add, find empty slots
    while (RemainingStack > 0)
    {
        int32 EmptySlot = GetFirstEmptySlot();
        if (EmptySlot == -1)
        {
            UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: No empty slots available"));
            break;
        }

        // Create a new item info object for this slot
        UBP_ConcreteItemInfo* NewSlotItem = NewObject<UBP_ConcreteItemInfo>(this);
        if (!NewSlotItem)
        {
            break;
        }

        // Copy data from original item
        NewSlotItem->CopyFromItemInfo(ItemInfo);

        // Set stack size for this slot
        if (ItemInfo->bIsStackable)
        {
            NewSlotItem->StackNumber = FMath::Min(RemainingStack, MaxStackSize);
            RemainingStack -= NewSlotItem->StackNumber;
        }
        else
        {
            NewSlotItem->StackNumber = 1;
            RemainingStack--;
        }

        // Add to inventory
        InventoryItems[EmptySlot] = NewSlotItem;
        bItemAdded = true;

        UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Added %d items to slot %d"), NewSlotItem->StackNumber, EmptySlot);
    }

    if (bItemAdded)
    {
        BroadcastInventoryChanged();
        OnItemAdded.Broadcast(ItemInfo);
        UpdateInventoryUI();
    }

    return bItemAdded;
}

bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Amount)
{
    if (SlotIndex < 0 || SlotIndex >= InventoryItems.Num() || !InventoryItems[SlotIndex])
    {
        return false;
    }

    UBP_ItemInfo* ItemAtSlot = InventoryItems[SlotIndex];
    
    if (Amount >= ItemAtSlot->StackNumber)
    {
        // Remove entire stack
        OnItemRemoved.Broadcast(ItemAtSlot);
        InventoryItems[SlotIndex] = nullptr;
    }
    else
    {
        // Reduce stack size
        ItemAtSlot->StackNumber -= Amount;
    }

    BroadcastInventoryChanged();
    UpdateInventoryUI();
    return true;
}

bool UInventoryComponent::RemoveItemByIndex(int32 ItemIndex, int32 Amount)
{
    int32 RemainingToRemove = Amount;

    for (int32 i = 0; i < InventoryItems.Num() && RemainingToRemove > 0; ++i)
    {
        if (InventoryItems[i] && InventoryItems[i]->ItemIndex == ItemIndex)
        {
            int32 AmountToRemove = FMath::Min(RemainingToRemove, InventoryItems[i]->StackNumber);
            RemoveItem(i, AmountToRemove);
            RemainingToRemove -= AmountToRemove;
        }
    }

    return RemainingToRemove == 0;
}

bool UInventoryComponent::HasItem(int32 ItemIndex, int32 Amount) const
{
    return GetItemCount(ItemIndex) >= Amount;
}

int32 UInventoryComponent::GetItemCount(int32 ItemIndex) const
{
    int32 TotalCount = 0;
    
    for (UBP_ItemInfo* Item : InventoryItems)
    {
        if (Item && Item->ItemIndex == ItemIndex)
        {
            TotalCount += Item->StackNumber;
        }
    }
    
    return TotalCount;
}

UBP_ItemInfo* UInventoryComponent::GetItemAtSlot(int32 SlotIndex) const
{
    if (SlotIndex >= 0 && SlotIndex < InventoryItems.Num())
    {
        return InventoryItems[SlotIndex];
    }
    return nullptr;
}

bool UInventoryComponent::IsSlotEmpty(int32 SlotIndex) const
{
    return GetItemAtSlot(SlotIndex) == nullptr;
}

int32 UInventoryComponent::GetFirstEmptySlot() const
{
    for (int32 i = 0; i < InventoryItems.Num(); ++i)
    {
        if (!InventoryItems[i])
        {
            return i;
        }
    }
    return -1;
}

int32 UInventoryComponent::GetAvailableSlots() const
{
    int32 EmptySlots = 0;
    for (UBP_ItemInfo* Item : InventoryItems)
    {
        if (!Item)
        {
            EmptySlots++;
        }
    }
    return EmptySlots;
}

void UInventoryComponent::UseItem(UWBP_InventorySlot* InventorySlot)
{
    if (!InventorySlot || !OwnerCharacter)
    {
        return;
    }

    UBP_ItemInfo* ItemInfo = InventorySlot->GetInventoryItemInfoRef();
    if (!ItemInfo)
    {
        return;
    }

    // Handle item use based on type
    switch (ItemInfo->ItemType)
    {
        case EItemType::Equip:
            EquipItem(ItemInfo);
            break;
        case EItemType::Consume_HP:
        case EItemType::Consume_MP:
            ConsumeItem(ItemInfo, InventorySlot, ItemInfo->RecoveryHP, ItemInfo->RecoveryMP, ItemInfo->ItemType);
            break;
        default:
            UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: Unknown item type for use"));
            break;
    }

    OnItemUsed.Broadcast(ItemInfo);
}

void UInventoryComponent::ThrowItem(UWBP_InventorySlot* InventorySlot)
{
    if (!InventorySlot || !OwnerCharacter)
    {
        return;
    }

    UBP_ItemInfo* ItemInfo = InventorySlot->GetInventoryItemInfoRef();
    if (!ItemInfo)
    {
        return;
    }

    int32 SlotIndex = InventorySlot->SlotIndex;
    
    // Remove item from inventory
    if (RemoveItem(SlotIndex, ItemInfo->StackNumber))
    {
        // Spawn the item in the world
        FVector PlayerLocation = OwnerCharacter->GetActorLocation();
        FVector PlayerForward = OwnerCharacter->GetActorForwardVector();
        FVector SpawnLocation = PlayerLocation + (PlayerForward * 200.0f);
        SpawnLocation.Z = PlayerLocation.Z;
        
        FRotator SpawnRotation = OwnerCharacter->GetActorRotation();
        
        SpawnItemInWorld(ItemInfo->ItemIndex, ItemInfo->StackNumber, SpawnLocation, SpawnRotation);
        
        UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Threw item %s"), *ItemInfo->ItemName);
    }
}

void UInventoryComponent::EquipItem(UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo || !OwnerCharacter)
    {
        return;
    }

    // Delegate to character's equip function
    OwnerCharacter->EquipInventoryItem(ItemInfo);
}

void UInventoryComponent::ConsumeItem(UBP_ItemInfo* ItemInfo, UWBP_InventorySlot* InventorySlot, int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (!ItemInfo || !InventorySlot || !OwnerCharacter)
    {
        return;
    }

    // Apply recovery effects through character's stats component
    if (RecoverHP > 0)
    {
        OwnerCharacter->RecoverHealth(RecoverHP);
    }

    if (RecoverMP > 0)
    {
        OwnerCharacter->RecoverMP(RecoverMP);
    }

    // Remove item from inventory
    int32 SlotIndex = InventorySlot->SlotIndex;
    if (ItemInfo->bIsStackable && ItemInfo->StackNumber > 1)
    {
        // Reduce stack by 1
        RemoveItem(SlotIndex, 1);
    }
    else
    {
        // Remove the item completely
        RemoveItem(SlotIndex, ItemInfo->StackNumber);
    }

    UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Consumed item %s (HP+%d, MP+%d)"), *ItemInfo->ItemName, RecoverHP, RecoverMP);
}

void UInventoryComponent::SwapItems(int32 FromSlotIndex, int32 ToSlotIndex)
{
    if (FromSlotIndex < 0 || FromSlotIndex >= InventoryItems.Num() ||
        ToSlotIndex < 0 || ToSlotIndex >= InventoryItems.Num())
    {
        return;
    }

    // Simple swap
    UBP_ItemInfo* TempItem = InventoryItems[FromSlotIndex];
    InventoryItems[FromSlotIndex] = InventoryItems[ToSlotIndex];
    InventoryItems[ToSlotIndex] = TempItem;

    BroadcastInventoryChanged();
    UpdateInventoryUI();

    // Play sound effect
    if (UGameplayStatics::GetCurrentLevelName(this) != TEXT(""))
    {
        UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")));
    }

    UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Swapped items between slots %d and %d"), FromSlotIndex, ToSlotIndex);
}

void UInventoryComponent::StackItems(int32 FromSlotIndex, int32 ToSlotIndex)
{
    if (FromSlotIndex < 0 || FromSlotIndex >= InventoryItems.Num() ||
        ToSlotIndex < 0 || ToSlotIndex >= InventoryItems.Num() ||
        !InventoryItems[FromSlotIndex] || !InventoryItems[ToSlotIndex])
    {
        return;
    }

    UBP_ItemInfo* FromItem = InventoryItems[FromSlotIndex];
    UBP_ItemInfo* ToItem = InventoryItems[ToSlotIndex];

    // Check if items can be stacked
    if (!CanStackItems(FromItem, ToItem))
    {
        return;
    }

    // Calculate total stack
    int32 TotalStack = FromItem->StackNumber + ToItem->StackNumber;
    
    if (TotalStack <= MaxStackSize)
    {
        // Combine into destination slot
        ToItem->StackNumber = TotalStack;
        InventoryItems[FromSlotIndex] = nullptr;
    }
    else
    {
        // Fill destination to max and keep remainder in source
        ToItem->StackNumber = MaxStackSize;
        FromItem->StackNumber = TotalStack - MaxStackSize;
    }

    BroadcastInventoryChanged();
    UpdateInventoryUI();

    UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Stacked items from slot %d to slot %d"), FromSlotIndex, ToSlotIndex);
}

void UInventoryComponent::HandleDragAndDrop(UBP_ItemInfo* FromItem, UWBP_InventorySlot* FromSlot, UBP_ItemInfo* ToItem, UWBP_InventorySlot* ToSlot)
{
    if (!FromSlot || !ToSlot)
    {
        return;
    }

    int32 FromIndex = FromSlot->SlotIndex;
    int32 ToIndex = ToSlot->SlotIndex;

    // Validate indices
    if (FromIndex < 0 || ToIndex < 0 || FromIndex >= InventoryItems.Num() || ToIndex >= InventoryItems.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: Invalid slot indices for drag and drop: From=%d, To=%d"), FromIndex, ToIndex);
        return;
    }

    // Handle stacking if items are the same type
    if (FromItem && ToItem && CanStackItems(FromItem, ToItem))
    {
        StackItems(FromIndex, ToIndex);
    }
    else
    {
        // Simple swap
        SwapItems(FromIndex, ToIndex);
    }
}

void UInventoryComponent::UpdateInventoryUI()
{
    if (!MainWidget || !OwnerCharacter)
    {
        return;
    }

    // Get the inventory widget from the main widget
    UWBP_Inventory* InventoryWidget = MainWidget->GetInventoryWidget();
    if (!InventoryWidget)
    {
        return;
    }

    // Get all inventory slot widgets
    TArray<UWBP_InventorySlot*> Slots = InventoryWidget->GetInventorySlotWidgets();
    
    // Update each slot with the corresponding inventory item
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (Slots[i])
        {
            // Set the slot index first
            Slots[i]->SetSlotIndex(i);
            
            // Set slot type to Inventory to enable context menu
            Slots[i]->SetInventorySlotType(EInventorySlotType::Inventory);
            
            // Bind context menu events to inventory component handlers
            if (!Slots[i]->ContextMenuClickUse.IsBound())
            {
                Slots[i]->ContextMenuClickUse.AddDynamic(this, &UInventoryComponent::OnContextMenuUse);
            }
            if (!Slots[i]->ContextMenuClickThrow.IsBound())
            {
                Slots[i]->ContextMenuClickThrow.AddDynamic(this, &UInventoryComponent::OnContextMenuThrow);
            }
            
            // Check if we have an item for this slot
            if (i < InventoryItems.Num() && InventoryItems[i])
            {
                Slots[i]->UpdateSlot(InventoryItems[i]);
            }
            else
            {
                Slots[i]->ClearSlot();
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Updated inventory UI"));
}

void UInventoryComponent::SetMainWidget(UWBP_Main* NewMainWidget)
{
    MainWidget = NewMainWidget;
    
    if (MainWidget)
    {
        UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Main widget set successfully"));
        UpdateInventoryUI();
    }
}

bool UInventoryComponent::PurchaseItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice)
{
    // Calculate total cost
    int32 TotalCost = ItemPrice * ItemStackNumber;
    
    // Check if player has enough gold
    if (OwnerCharacter && OwnerCharacter->StatsComponent)
    {
        if (OwnerCharacter->StatsComponent->GetGold() >= TotalCost)
        {
            // Try to add item to inventory first
            if (AddItem(ItemIndex, ItemStackNumber))
            {
                // Deduct gold on successful purchase
                OwnerCharacter->StatsComponent->SpendGold(TotalCost);
                
                UE_LOG(LogTemp, Log, TEXT("✅ Successfully purchased %d x Item %d for %d gold"), 
                       ItemStackNumber, ItemIndex, TotalCost);
                return true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("❌ Failed to add item to inventory during purchase"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ Not enough gold for purchase. Need %d, have %d"), 
                   TotalCost, OwnerCharacter->StatsComponent->GetGold());
        }
    }
    
    return false;
}

bool UInventoryComponent::BuyItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice)
{
    // Delegate to PurchaseItem for consistency
    return PurchaseItem(ItemIndex, ItemStackNumber, ItemPrice);
}

void UInventoryComponent::ClearInventory()
{
    for (int32 i = 0; i < InventoryItems.Num(); ++i)
    {
        if (InventoryItems[i])
        {
            OnItemRemoved.Broadcast(InventoryItems[i]);
        }
        InventoryItems[i] = nullptr;
    }

    BroadcastInventoryChanged();
    UpdateInventoryUI();

    UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Cleared all inventory items"));
}

// Context menu event handlers (called by inventory slot widgets)
UFUNCTION()
void UInventoryComponent::OnContextMenuUse(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot)
{
    UseItem(InventorySlot);
}

UFUNCTION()
void UInventoryComponent::OnContextMenuThrow(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot)
{
    ThrowItem(InventorySlot);
}

FStructure_ItemInfo UInventoryComponent::CreateHardcodedItemData(int32 ItemIndex)
{
    FStructure_ItemInfo ItemInfo;
    
    switch (ItemIndex)
    {
        case 1: // Basic HP Potion
            ItemInfo.ItemIndex = 1;
            ItemInfo.ItemName = TEXT("Basic HP Potion");
            ItemInfo.ItemDescription = TEXT("Restores a small amount of HP.");
            ItemInfo.bIsValid = true;
            ItemInfo.bIsStackable = true;
            ItemInfo.StackNumber = 1;
            ItemInfo.ItemType = EItemType::Consume_HP;
            ItemInfo.ItemEquipSlot = EItemEquipSlot::None;
            ItemInfo.RecoveryHP = 30;
            ItemInfo.RecoveryMP = 0;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicHealingPotion")));
            break;
            
        // Add more cases for other items as needed
            
        default:
            UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: Unknown item index %d, creating empty item"), ItemIndex);
            ItemInfo.bIsValid = false;
            break;
    }
    
    return ItemInfo;
}

// Private helper functions
bool UInventoryComponent::CanStackItems(UBP_ItemInfo* Item1, UBP_ItemInfo* Item2) const
{
    if (!Item1 || !Item2)
    {
        return false;
    }

    return Item1->ItemIndex == Item2->ItemIndex && 
           Item1->bIsStackable && 
           Item2->bIsStackable;
}

int32 UInventoryComponent::FindItemSlot(int32 ItemIndex) const
{
    for (int32 i = 0; i < InventoryItems.Num(); ++i)
    {
        if (InventoryItems[i] && InventoryItems[i]->ItemIndex == ItemIndex)
        {
            return i;
        }
    }
    return -1;
}

void UInventoryComponent::BroadcastInventoryChanged()
{
    OnInventoryChanged.Broadcast();
}

UBP_ConcreteItemInfo* UInventoryComponent::CreateItemFromData(const FStructure_ItemInfo& ItemData, int32 StackNumber)
{
    // Create a new item info object
    UBP_ConcreteItemInfo* NewItemInfo = NewObject<UBP_ConcreteItemInfo>(this);
    if (!NewItemInfo)
    {
        return nullptr;
    }

    // Copy data from structure
    NewItemInfo->CopyFromStructure(ItemData);
    NewItemInfo->StackNumber = StackNumber;

    // Load thumbnail using Universal Item Loader
    if (NewItemInfo->ThumbnailBrush.GetResourceObject() == nullptr)
    {
        UTexture2D* LoadedTexture = UUniversalItemLoader::LoadItemTexture(ItemData);
        if (LoadedTexture)
        {
            NewItemInfo->Thumbnail = LoadedTexture;
            
            FSlateBrush NewBrush;
            NewBrush.SetResourceObject(LoadedTexture);
            NewBrush.ImageSize = FVector2D(64.0f, 64.0f);
            NewBrush.DrawAs = ESlateBrushDrawType::Image;
            NewBrush.Tiling = ESlateBrushTileType::NoTile;
            NewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
            NewItemInfo->ThumbnailBrush = NewBrush;
        }
    }

    return NewItemInfo;
}

bool UInventoryComponent::GetItemDataForIndex(int32 ItemIndex, FStructure_ItemInfo& OutItemData)
{
    // Try to get from store system first (most reliable)
    bool bFoundItemData = UStoreSystemFix::GetItemData(ItemIndex, OutItemData);
    
    if (!bFoundItemData && OwnerCharacter)
    {
        // Try to get from game instance as fallback
        UAtlantisEonsGameInstance* GameInstance = Cast<UAtlantisEonsGameInstance>(OwnerCharacter->GetGameInstance());
        if (GameInstance && GameInstance->GetItemInfo(ItemIndex, OutItemData))
        {
            bFoundItemData = true;
        }
    }
    
    if (!bFoundItemData)
    {
        // Use hardcoded data as last resort
        OutItemData = CreateHardcodedItemData(ItemIndex);
        bFoundItemData = OutItemData.bIsValid;
    }
    
    return bFoundItemData;
}

void UInventoryComponent::SpawnItemInWorld(int32 ItemIndex, int32 StackNumber, FVector SpawnLocation, FRotator SpawnRotation)
{
    if (!OwnerCharacter)
    {
        return;
    }

    UWorld* World = OwnerCharacter->GetWorld();
    if (!World)
    {
        return;
    }

    // Load the BP_Item class
    UClass* ItemClass = LoadClass<ABP_Item>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/BP_Item.BP_Item_C"));
    if (ItemClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        
        ABP_Item* SpawnedItem = World->SpawnActor<ABP_Item>(ItemClass, SpawnLocation, SpawnRotation, SpawnParams);
        if (SpawnedItem)
        {
            // Initialize the spawned item with the correct data
            SpawnedItem->InitializeItem(ItemIndex, StackNumber);
            UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Spawned item %d with stack %d in world"), ItemIndex, StackNumber);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("InventoryComponent: Failed to spawn item actor"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryComponent: Failed to load BP_Item class"));
    }
}

void UInventoryComponent::ContextMenuUse(UWBP_InventorySlot* InventorySlot)
{
    if (!InventorySlot) return;
    
    UBP_ItemInfo* ItemInfo = InventorySlot->GetInventoryItemInfoRef();
    if (!ItemInfo) return;
    
    // Handle item use based on type
    switch (ItemInfo->ItemType)
    {
        case EItemType::Equip:
            ContextMenuUse_EquipItem(ItemInfo);
            break;
        case EItemType::Consume_HP:
        case EItemType::Consume_MP:
            ContextMenuUse_ConsumeItem(ItemInfo, InventorySlot, ItemInfo->RecoveryHP, ItemInfo->RecoveryMP, ItemInfo->ItemType);
            break;
        default:
            break;
    }
}

void UInventoryComponent::ContextMenuThrow(UWBP_InventorySlot* InventorySlot)
{
    if (!InventorySlot) return;
    
    UBP_ItemInfo* ItemInfo = InventorySlot->GetInventoryItemInfoRef();
    if (!ItemInfo) return;
    
    // Get the item index and stack information
    int32 ItemIndex = ItemInfo->ItemIndex;
    int32 StackNumber = ItemInfo->StackNumber;
    int32 SlotIndex = InventorySlot->SlotIndex;
    
    // Remove item from inventory array
    if (SlotIndex >= 0 && SlotIndex < InventoryItems.Num())
    {
        InventoryItems[SlotIndex] = nullptr;
        InventorySlot->ClearSlot();
        
        // Update inventory display
        UpdateInventorySlots();
        
        // Spawn the item in the world in front of the player
        if (OwnerCharacter)
        {
            FVector PlayerLocation = OwnerCharacter->GetActorLocation();
            FVector PlayerForward = OwnerCharacter->GetActorForwardVector();
            FVector SpawnLocation = PlayerLocation + (PlayerForward * 200.0f);
            SpawnLocation.Z = PlayerLocation.Z;
            FRotator SpawnRotation = OwnerCharacter->GetActorRotation();
            
            SpawnItemInWorld(ItemIndex, StackNumber, SpawnLocation, SpawnRotation);
        }
    }
}

void UInventoryComponent::ContextMenuUse_EquipItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!ItemInfoRef || !OwnerCharacter) return;
    
    // Delegate to character's equipment system for now
    // TODO: Move equipment logic to EquipmentComponent
    OwnerCharacter->ContextMenuUse_EquipItem(ItemInfoRef);
}

void UInventoryComponent::ContextMenuUse_ConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (!ItemInfoRef || !InventorySlotRef || !OwnerCharacter) return;
    
    // Apply recovery effects to character
    if (RecoverHP > 0)
    {
        OwnerCharacter->RecoverHealth(RecoverHP);
    }
    
    if (RecoverMP > 0)
    {
        OwnerCharacter->RecoverMP(RecoverMP);
    }
    
    // Remove item from inventory array
    int32 SlotIndex = InventorySlotRef->SlotIndex;
    if (SlotIndex >= 0 && SlotIndex < InventoryItems.Num())
    {
        // Check if stackable and has multiple items
        if (ItemInfoRef->bIsStackable && ItemInfoRef->StackNumber > 1)
        {
            // Reduce stack by 1
            ItemInfoRef->StackNumber -= 1;
            InventorySlotRef->UpdateSlot(ItemInfoRef);
        }
        else
        {
            // Remove the item completely
            InventoryItems[SlotIndex] = nullptr;
            InventorySlotRef->ClearSlot();
        }
        
        // Update inventory display
        UpdateInventorySlots();
        BroadcastInventoryChanged();
    }
}

bool UInventoryComponent::AddItemToInventory(UBP_ItemInfo* ItemInfo)
{
    return AddItemInfo(ItemInfo);
}

void UInventoryComponent::UpdateInventorySlots()
{
    if (!MainWidget || !OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryComponent: Cannot update inventory slots - MainWidget or OwnerCharacter is null"));
        return;
    }

    // Get the inventory widget from the main widget
    UWBP_Inventory* InventoryWidget = MainWidget->GetInventoryWidget();
    if (InventoryWidget)
    {
        // Get all inventory slot widgets
        TArray<UWBP_InventorySlot*> Slots = InventoryWidget->GetInventorySlotWidgets();
        
        // Update each slot with the corresponding inventory item
        for (int32 i = 0; i < Slots.Num(); ++i)
        {
            if (Slots[i])
            {
                // Set the slot index first
                Slots[i]->SetSlotIndex(i);
                
                // Set slot type to Inventory to enable context menu
                Slots[i]->SetInventorySlotType(EInventorySlotType::Inventory);
                
                // Bind context menu events to inventory component handlers
                if (!Slots[i]->ContextMenuClickUse.IsBound())
                {
                    Slots[i]->ContextMenuClickUse.AddDynamic(this, &UInventoryComponent::OnContextMenuUse);
                }
                if (!Slots[i]->ContextMenuClickThrow.IsBound())
                {
                    Slots[i]->ContextMenuClickThrow.AddDynamic(this, &UInventoryComponent::OnContextMenuThrow);
                }
                
                // Check if we have an item for this slot
                if (i < InventoryItems.Num() && InventoryItems[i])
                {
                    Slots[i]->UpdateSlot(InventoryItems[i]);
                }
                else
                {
                    Slots[i]->ClearSlot();
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryComponent: Cannot update inventory slots - Inventory widget not found"));
    }
} 