#include "InventoryManagerComponent.h"
#include "AtlantisEonsCharacter.h"
#include "AtlantisEonsHUD.h"
#include "WBP_Main.h"
#include "WBP_InventorySlot.h"
#include "BP_ItemInfo.h"
#include "BP_ConcreteItemInfo.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "CharacterUIManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SlateWrapperTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"

UInventoryManagerComponent::UInventoryManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bIsInventoryOpen = false;
    bInventoryToggleLocked = false;
    bInventoryNeedsUpdate = false;
    OwnerCharacter = nullptr;
    MainWidget = nullptr;
}

void UInventoryManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize with owner character
    OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
    if (OwnerCharacter)
    {
        UE_LOG(LogTemp, Log, TEXT("InventoryManagerComponent: Initialized with owner character"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Failed to get owner character"));
    }
}

void UInventoryManagerComponent::InitializeInventoryManager(AAtlantisEonsCharacter* InOwnerCharacter)
{
    OwnerCharacter = InOwnerCharacter;
    
    if (OwnerCharacter)
    {
        UE_LOG(LogTemp, Log, TEXT("InventoryManagerComponent: Successfully initialized with character reference"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Failed to initialize - character reference is null"));
    }
}

// ========== INVENTORY UI STATE MANAGEMENT ==========

void UInventoryManagerComponent::ToggleInventory(const FInputActionValue& Value)
{
    // Check if toggle is locked (but allow closing even if locked for emergency situations)
    if (bInventoryToggleLocked && !bIsInventoryOpen)
    {
        return;
    }
    
    // Get the HUD first
    AAtlantisEonsHUD* HUD = GetHUD();
    if (!HUD)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Failed to get HUD in ToggleInventory"));
        return;
    }
    
    // Lock toggle to prevent rapid switching (but shorter duration)
    bInventoryToggleLocked = true;
    GetWorld()->GetTimerManager().SetTimer(InventoryToggleLockTimer, this, &UInventoryManagerComponent::UnlockInventoryToggle, 0.1f, false);
    
    // If inventory is open, close it
    if (bIsInventoryOpen)
    {
        CloseInventory();
    }
    else
    {
        // Only try to open if it's not already visible
        if (!HUD->IsInventoryWidgetVisible())
        {
            OpenInventory();
        }
        else
        {
            bIsInventoryOpen = true;
        }
    }
}

void UInventoryManagerComponent::OpenInventory()
{
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Cannot open inventory - OwnerCharacter is null"));
        return;
    }

    // Get the HUD
    AAtlantisEonsHUD* HUD = GetHUD();
    if (!HUD)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Cannot open inventory - HUD is null"));
        return;
    }

    // Show the inventory widget
    if (!HUD->ShowInventoryWidget())
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Failed to show inventory widget"));
        return;
    }

    // Get the main widget reference
    MainWidget = HUD->GetMainWidget();
    if (!MainWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Failed to get Main widget from HUD"));
        return;
    }

    // Update the inventory state
    bIsInventoryOpen = true;
    
    // Force update all inventory slots immediately
    UpdateInventorySlots();
    
    // Also set up a delayed update to ensure it runs after any Blueprint logic
    ForceUpdateInventorySlotsAfterDelay();
    
    // Broadcast state change
    OnInventoryStateChanged.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("InventoryManagerComponent: Inventory opened successfully"));
}

void UInventoryManagerComponent::CloseInventory()
{
    CloseInventoryImpl();
}

void UInventoryManagerComponent::CloseInventoryImpl()
{
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Cannot close inventory - OwnerCharacter is null"));
        return;
    }

    if (bIsInventoryOpen)
    {
        // Get the HUD
        AAtlantisEonsHUD* HUD = GetHUD();
        if (!HUD)
        {
            UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Failed to get HUD in CloseInventory"));
            return;
        }
        
        // Clear all item info popups BEFORE hiding the inventory widget
        ClearAllInventoryPopups();
        
        // Hide the inventory widget
        HUD->HideInventoryWidget();
        bIsInventoryOpen = false;
        
        // Get the player controller
        APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
        if (!PC)
        {
            UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Failed to get PlayerController in CloseInventory"));
            return;
        }
        
        // Restore game input mode
        RestoreGameInputMode();
        
        // Broadcast state change
        OnInventoryStateChanged.Broadcast();
        
        UE_LOG(LogTemp, Log, TEXT("InventoryManagerComponent: Inventory closed successfully"));
    }
}

void UInventoryManagerComponent::ForceSetInventoryState(bool bNewIsOpen)
{
    bool bPrevState = bIsInventoryOpen;
    bIsInventoryOpen = bNewIsOpen;
    
    if (bPrevState != bIsInventoryOpen)
    {
        OnInventoryStateChanged.Broadcast();
    }
    
    UE_LOG(LogTemp, Log, TEXT("InventoryManagerComponent: Forced inventory state to %s"), 
           bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));
}

void UInventoryManagerComponent::CloseInventoryIfOpen(const FInputActionValue& Value)
{
    if (bIsInventoryOpen)
    {
        CloseInventory();
    }
}

// ========== INVENTORY TOGGLE LOCKING ==========

void UInventoryManagerComponent::SetInventoryToggleLock(bool bLock, float UnlockDelay)
{
    bool bPrevLocked = bInventoryToggleLocked;
    bInventoryToggleLocked = bLock;

    if (UnlockDelay > 0.0f)
    {
        GetWorld()->GetTimerManager().ClearTimer(InventoryToggleLockTimer);
        GetWorld()->GetTimerManager().SetTimer(
            InventoryToggleLockTimer,
            FTimerDelegate::CreateWeakLambda(this, [this]() {
                if (IsValid(this) && OwnerCharacter && !OwnerCharacter->IsActorBeingDestroyed()) {
                    SetInventoryToggleLock(false, 0.0f);
                }
            }),
            UnlockDelay,
            false
        );
    }
}

void UInventoryManagerComponent::UnlockInventoryToggle()
{
    bInventoryToggleLocked = false;
}

// ========== INVENTORY SLOT MANAGEMENT ==========

void UInventoryManagerComponent::UpdateInventorySlots()
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->UIManager)
    {
        OwnerCharacter->UIManager->UpdateInventorySlots();
    }
}

void UInventoryManagerComponent::DelayedUpdateInventorySlots()
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->UIManager)
    {
        OwnerCharacter->UIManager->DelayedUpdateInventorySlots();
    }
}

void UInventoryManagerComponent::ForceUpdateInventorySlotsAfterDelay()
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->UIManager)
    {
        OwnerCharacter->UIManager->ForceUpdateInventorySlotsAfterDelay();
    }
}

void UInventoryManagerComponent::SetInventorySlot(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotWidgetRef)
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->UIManager)
    {
        OwnerCharacter->UIManager->SetInventorySlot(ItemInfoRef, InventorySlotWidgetRef);
    }
}

// ========== CONTEXT MENU OPERATIONS ==========

void UInventoryManagerComponent::ContextMenuUse(UWBP_InventorySlot* InventorySlot)
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->InventoryComp)
    {
        OwnerCharacter->InventoryComp->ContextMenuUse(InventorySlot);
    }
}

void UInventoryManagerComponent::ContextMenuThrow(UWBP_InventorySlot* InventorySlot)
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->InventoryComp)
    {
        OwnerCharacter->InventoryComp->ContextMenuThrow(InventorySlot);
    }
}

void UInventoryManagerComponent::ContextMenuUse_EquipItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->InventoryComp)
    {
        OwnerCharacter->InventoryComp->ContextMenuUse_EquipItem(ItemInfoRef);
    }
}

void UInventoryManagerComponent::ContextMenuUse_ConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, 
    int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->InventoryComp)
    {
        OwnerCharacter->InventoryComp->ContextMenuUse_ConsumeItem(ItemInfoRef, InventorySlotRef, RecoverHP, RecoverMP, ItemType);
    }
}

void UInventoryManagerComponent::ProcessConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, 
    int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (!ItemInfoRef || !InventorySlotRef || !OwnerCharacter) return;

    // Apply recovery effects
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

// ========== DRAG AND DROP OPERATIONS ==========

void UInventoryManagerComponent::DragAndDropExchangeItem(UBP_ItemInfo* FromInventoryItemRef, UWBP_InventorySlot* FromInventorySlotRef,
    UBP_ItemInfo* ToInventoryItemRef, UWBP_InventorySlot* ToInventorySlotRef)
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->InventoryComp)
    {
        OwnerCharacter->InventoryComp->HandleDragAndDrop(FromInventoryItemRef, FromInventorySlotRef, ToInventoryItemRef, ToInventorySlotRef);
    }
}

// ========== ITEM OPERATIONS ==========

bool UInventoryManagerComponent::PickingItem(int32 ItemIndex, int32 ItemStackNumber)
{
    if (!OwnerCharacter)
    {
        return false;
    }

    if (OwnerCharacter->InventoryComp)
    {
        return OwnerCharacter->InventoryComp->AddItem(ItemIndex, ItemStackNumber);
    }
    UE_LOG(LogTemp, Warning, TEXT("InventoryManagerComponent: PickingItem - InventoryComponent is null"));
    return false;
}

bool UInventoryManagerComponent::BuyingItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice)
{
    if (!OwnerCharacter)
    {
        return false;
    }

    if (OwnerCharacter->InventoryComp)
    {
        return OwnerCharacter->InventoryComp->BuyItem(ItemIndex, ItemStackNumber, ItemPrice);
    }
    return false;
}

bool UInventoryManagerComponent::AddItemToInventory(UBP_ItemInfo* ItemInfo)
{
    if (!OwnerCharacter)
    {
        return false;
    }

    if (OwnerCharacter->InventoryComp)
    {
        return OwnerCharacter->InventoryComp->AddItemToInventory(ItemInfo);
    }
    UE_LOG(LogTemp, Warning, TEXT("InventoryManagerComponent: AddItemToInventory - InventoryComponent is null"));
    return false;
}

UBP_ItemInfo* UInventoryManagerComponent::GetInventoryItemRef() const
{
    if (!OwnerCharacter)
    {
        return nullptr;
    }

    // Create a new concrete item info instance
    UClass* ItemInfoClass = LoadClass<UBP_ConcreteItemInfo>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/BP_ConcreteItemInfo.BP_ConcreteItemInfo_C"));
    if (!ItemInfoClass)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Failed to load BP_ConcreteItemInfo class"));
        return nullptr;
    }
    
    UBP_ConcreteItemInfo* NewItemInfo = NewObject<UBP_ConcreteItemInfo>(const_cast<UInventoryManagerComponent*>(this), ItemInfoClass);
    if (!NewItemInfo)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryManagerComponent: Failed to create BP_ConcreteItemInfo object"));
        return nullptr;
    }
    
    return NewItemInfo;
}

// ========== EQUIPMENT INTEGRATION ==========

void UInventoryManagerComponent::EquipInventoryItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->EquipmentComponent)
    {
        OwnerCharacter->EquipmentComponent->EquipInventoryItemWithCharacter(ItemInfoRef, OwnerCharacter);
    }
}

void UInventoryManagerComponent::UnequipInventoryItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->EquipmentComponent)
    {
        OwnerCharacter->EquipmentComponent->UnequipInventoryItemWithCharacter(ItemInfoRef, OwnerCharacter);
    }
}

// ========== UI MANAGEMENT ==========

void UInventoryManagerComponent::SetMainWidget(UWBP_Main* NewWidget)
{
    MainWidget = NewWidget;
    
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->UIManager)
    {
        OwnerCharacter->UIManager->SetMainWidget(NewWidget);
    }
    
    // Also set the legacy reference for backward compatibility
    if (NewWidget && OwnerCharacter)
    {
        OwnerCharacter->MainWidget = NewWidget;
        OwnerCharacter->Main = NewWidget;
        
        // Get the character info widget from the main widget
        if (MainWidget->GetCharacterInfoWidget())
        {
            OwnerCharacter->WBP_CharacterInfo = MainWidget->GetCharacterInfoWidget();
        }
    }
}

void UInventoryManagerComponent::ClearAllInventoryPopups()
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->UIManager)
    {
        OwnerCharacter->UIManager->ClearAllInventoryPopups();
    }
}

void UInventoryManagerComponent::ConnectInventoryToMainWidget()
{
    if (!OwnerCharacter)
    {
        return;
    }

    // Set the main widget reference in the inventory component
    if (OwnerCharacter->InventoryComp && MainWidget)
    {
        OwnerCharacter->InventoryComp->SetMainWidget(MainWidget);
    }
}

// ========== STORE INTEGRATION ==========

void UInventoryManagerComponent::SettingStore()
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (OwnerCharacter->UIManager)
    {
        OwnerCharacter->UIManager->SettingStore();
    }
}

// ========== EVENT HANDLERS ==========

void UInventoryManagerComponent::OnContextMenuUse(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot)
{
    ContextMenuUse(InventorySlot);
}

void UInventoryManagerComponent::OnContextMenuThrow(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot)
{
    ContextMenuThrow(InventorySlot);
}

void UInventoryManagerComponent::OnInventoryChangedEvent()
{
    UE_LOG(LogTemp, Log, TEXT("InventoryManagerComponent: OnInventoryChangedEvent called"));
    UpdateInventorySlots();
}

// ========== PRIVATE HELPER FUNCTIONS ==========

AAtlantisEonsHUD* UInventoryManagerComponent::GetHUD() const
{
    if (!OwnerCharacter)
    {
        return nullptr;
    }

    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC)
    {
        return nullptr;
    }

    return Cast<AAtlantisEonsHUD>(PC->GetHUD());
}

void UInventoryManagerComponent::SetupInventoryInputMode()
{
    if (!OwnerCharacter)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC)
    {
        return;
    }

    // Set input mode to UI for inventory interaction
    FInputModeGameAndUI GameAndUIMode;
    GameAndUIMode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(GameAndUIMode);
    PC->SetShowMouseCursor(true);
}

void UInventoryManagerComponent::RestoreGameInputMode()
{
    if (!OwnerCharacter)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC)
    {
        return;
    }

    // Clear keyboard focus from any widgets and reset input mode
    if (GEngine && GEngine->GameViewport)
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
    }
    
    // Set input mode back to game only
    FInputModeGameOnly GameOnlyMode;
    PC->SetInputMode(GameOnlyMode);
    PC->SetShowMouseCursor(false);
    PC->FlushPressedKeys();
    
    // Restore all input mappings
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
    {
        // Clear all mappings first
        Subsystem->ClearAllMappings();
        
        // Then add back the default mapping context
        if (OwnerCharacter->GetDefaultMappingContext())
        {
            Subsystem->AddMappingContext(OwnerCharacter->GetDefaultMappingContext(), 0);
        }
    }
    
    // Re-enable character movement
    if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
    {
        MovementComp->SetMovementMode(MOVE_Walking);
        MovementComp->bOrientRotationToMovement = true;
        MovementComp->MaxWalkSpeed = OwnerCharacter->BaseMovementSpeed;
    }
    
    // Force enable input for both controller and character
    PC->EnableInput(PC);
    OwnerCharacter->EnableInput(PC);
    
    // Reset character input to ensure everything works
    OwnerCharacter->ResetCharacterInput();
} 