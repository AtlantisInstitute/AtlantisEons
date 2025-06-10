#include "UITouchInputManager.h"
#include "AtlantisEonsCharacter.h"
#include "WBP_Main.h"
#include "WBP_InventorySlot.h"
#include "WBP_Store.h"
#include "WBP_CharacterInfo.h"
#include "WBP_Inventory.h"
#include "WBP_TopMenu.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "GenericPlatform/GenericApplication.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"

UUITouchInputManager::UUITouchInputManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // Set default values
    bUITouchInputEnabled = true;
    bAutoEnableOnMobile = true;
    UIButtonPressCooldown = 0.2f;
    
    // Auto-enable on mobile platforms
    if (bAutoEnableOnMobile && IsTouchPlatform())
    {
        bUITouchInputEnabled = true;
    }
}

void UUITouchInputManager::BeginPlay()
{
    Super::BeginPlay();
    
    OwnerCharacter = GetOwnerCharacter();
    
    if (OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Initialized for character %s"), *OwnerCharacter->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 UITouchInputManager: Failed to get owner character"));
    }
}

void UUITouchInputManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear all timers
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TabButtonCooldownTimer);
        GetWorld()->GetTimerManager().ClearTimer(InventorySlotCooldownTimer);
        GetWorld()->GetTimerManager().ClearTimer(EquipmentSlotCooldownTimer);
        GetWorld()->GetTimerManager().ClearTimer(StoreButtonCooldownTimer);
    }
    
    // Clear references
    OwnerCharacter = nullptr;
    MainWidgetRef = nullptr;
    
    Super::EndPlay(EndPlayReason);
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: EndPlay called"));
}

void UUITouchInputManager::InitializeUITouchInput(UWBP_Main* MainWidget)
{
    if (!MainWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 UITouchInputManager: InitializeUITouchInput - MainWidget is null!"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Initializing UI touch input with MainWidget"));
    
    MainWidgetRef = MainWidget;
    
    if (bUITouchInputEnabled)
    {
        // Setup touch input for all UI elements
        SetupWidgetSwitcherTouchInput();
        SetupInventorySlotsTouchInput();
        SetupEquipmentSlotsTouchInput();
        SetupStoreTouchInput();
        
        bUITouchInputInitialized = true;
        UE_LOG(LogTemp, Warning, TEXT("✅ UITouchInputManager: UI touch input system initialized and enabled"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: UI touch input system initialized but disabled"));
    }
}

void UUITouchInputManager::SetUITouchInputEnabled(bool bEnabled)
{
    if (bUITouchInputEnabled == bEnabled)
    {
        return; // No change needed
    }
    
    bUITouchInputEnabled = bEnabled;
    
    if (bEnabled && MainWidgetRef)
    {
        // Re-initialize touch input
        InitializeUITouchInput(MainWidgetRef);
        UE_LOG(LogTemp, Warning, TEXT("✅ UITouchInputManager: UI touch input enabled"));
    }
    else
    {
        bUITouchInputInitialized = false;
        UE_LOG(LogTemp, Warning, TEXT("❌ UITouchInputManager: UI touch input disabled"));
    }
}

// ========== WIDGET SWITCHER TOUCH SUPPORT ==========

void UUITouchInputManager::SetupWidgetSwitcherTouchInput()
{
    if (!MainWidgetRef)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 UITouchInputManager: SetupWidgetSwitcherTouchInput - MainWidget is null"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Setting up widget switcher touch input"));
    
    FindAndSetupTabNavigation();
}

void UUITouchInputManager::OnCharacterInfoTabPressed()
{
    if (!bUITouchInputEnabled || IsUIElementOnCooldown(TEXT("Tab")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("👤 UITouchInputManager: Character Info tab touched"));
    StartUIElementCooldown(TEXT("Tab"));
    
    if (MainWidgetRef)
    {
        MainWidgetRef->Switcher1();
    }
}

void UUITouchInputManager::OnInventoryTabPressed()
{
    if (!bUITouchInputEnabled || IsUIElementOnCooldown(TEXT("Tab")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎒 UITouchInputManager: Inventory tab touched"));
    StartUIElementCooldown(TEXT("Tab"));
    
    if (MainWidgetRef)
    {
        MainWidgetRef->Switcher2();
    }
}

void UUITouchInputManager::OnStoreTabPressed()
{
    if (!bUITouchInputEnabled || IsUIElementOnCooldown(TEXT("Tab")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🏪 UITouchInputManager: Store tab touched"));
    StartUIElementCooldown(TEXT("Tab"));
    
    if (MainWidgetRef)
    {
        MainWidgetRef->Switcher3();
    }
}

// ========== INVENTORY SLOTS TOUCH SUPPORT ==========

void UUITouchInputManager::SetupInventorySlotsTouchInput()
{
    if (!MainWidgetRef)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 UITouchInputManager: SetupInventorySlotsTouchInput - MainWidget is null"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Setting up inventory slots touch input"));
    
    FindAndSetupInventorySlots();
}

void UUITouchInputManager::OnInventorySlotTouched()
{
    if (!bUITouchInputEnabled || IsUIElementOnCooldown(TEXT("InventorySlot")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎒 UITouchInputManager: Inventory slot touched (index: %d)"), LastTouchedInventorySlotIndex);
    StartUIElementCooldown(TEXT("InventorySlot"));
    
    // The actual slot touch handling is done by the slot widgets themselves
    // This just provides touch input detection and cooldown management
}

// ========== EQUIPMENT SLOTS TOUCH SUPPORT ==========

void UUITouchInputManager::SetupEquipmentSlotsTouchInput()
{
    if (!MainWidgetRef)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 UITouchInputManager: SetupEquipmentSlotsTouchInput - MainWidget is null"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Setting up equipment slots touch input"));
    
    FindAndSetupEquipmentSlots();
}

void UUITouchInputManager::OnEquipmentSlotTouched()
{
    if (!bUITouchInputEnabled || IsUIElementOnCooldown(TEXT("EquipmentSlot")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚔️ UITouchInputManager: Equipment slot touched (index: %d)"), LastTouchedEquipmentSlotIndex);
    StartUIElementCooldown(TEXT("EquipmentSlot"));
    
    // The actual slot touch handling is done by the slot widgets themselves
    // This just provides touch input detection and cooldown management
}

// ========== STORE BUTTONS TOUCH SUPPORT ==========

void UUITouchInputManager::SetupStoreTouchInput()
{
    if (!MainWidgetRef)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 UITouchInputManager: SetupStoreTouchInput - MainWidget is null"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Setting up store touch input"));
    
    FindAndSetupStoreElements();
}

void UUITouchInputManager::OnStorePurchaseButtonTouched()
{
    if (!bUITouchInputEnabled || IsUIElementOnCooldown(TEXT("StoreButton")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🏪 UITouchInputManager: Store purchase button touched"));
    StartUIElementCooldown(TEXT("StoreButton"));
    
    // The actual purchase handling is done by the store widget
    // This just provides touch input detection and cooldown management
}

void UUITouchInputManager::OnStoreConfirmButtonTouched()
{
    if (!bUITouchInputEnabled || IsUIElementOnCooldown(TEXT("StoreButton")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("✅ UITouchInputManager: Store confirm button touched"));
    StartUIElementCooldown(TEXT("StoreButton"));
}

void UUITouchInputManager::OnStoreCancelButtonTouched()
{
    if (!bUITouchInputEnabled || IsUIElementOnCooldown(TEXT("StoreButton")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("❌ UITouchInputManager: Store cancel button touched"));
    StartUIElementCooldown(TEXT("StoreButton"));
}

// ========== HELPER FUNCTION IMPLEMENTATIONS ==========

void UUITouchInputManager::EnableWidgetTouchInput(UWidget* Widget)
{
    if (!Widget)
    {
        return;
    }
    
    // Enable interaction for the widget
    Widget->SetIsEnabled(true);
    
    // For buttons, ensure they can receive input
    if (UButton* Button = Cast<UButton>(Widget))
    {
        Button->SetClickMethod(EButtonClickMethod::DownAndUp);
        Button->SetTouchMethod(EButtonTouchMethod::DownAndUp);
    }
}

void UUITouchInputManager::AddTouchHandlersToWidget(UWidget* Widget, const FString& WidgetType)
{
    EnableWidgetTouchInput(Widget);
    UE_LOG(LogTemp, Verbose, TEXT("🎮 UITouchInputManager: Added touch handlers to %s widget"), *WidgetType);
}

bool UUITouchInputManager::IsUIElementOnCooldown(const FString& ElementType) const
{
    if (ElementType == TEXT("Tab"))
    {
        return bTabButtonOnCooldown;
    }
    else if (ElementType == TEXT("InventorySlot"))
    {
        return bInventorySlotOnCooldown;
    }
    else if (ElementType == TEXT("EquipmentSlot"))
    {
        return bEquipmentSlotOnCooldown;
    }
    else if (ElementType == TEXT("StoreButton"))
    {
        return bStoreButtonOnCooldown;
    }
    
    return false;
}

void UUITouchInputManager::StartUIElementCooldown(const FString& ElementType)
{
    if (ElementType == TEXT("Tab"))
    {
        bTabButtonOnCooldown = true;
        GetWorld()->GetTimerManager().SetTimer(
            TabButtonCooldownTimer,
            [this]() { bTabButtonOnCooldown = false; },
            UIButtonPressCooldown,
            false
        );
    }
    else if (ElementType == TEXT("InventorySlot"))
    {
        bInventorySlotOnCooldown = true;
        GetWorld()->GetTimerManager().SetTimer(
            InventorySlotCooldownTimer,
            [this]() { bInventorySlotOnCooldown = false; },
            UIButtonPressCooldown,
            false
        );
    }
    else if (ElementType == TEXT("EquipmentSlot"))
    {
        bEquipmentSlotOnCooldown = true;
        GetWorld()->GetTimerManager().SetTimer(
            EquipmentSlotCooldownTimer,
            [this]() { bEquipmentSlotOnCooldown = false; },
            UIButtonPressCooldown,
            false
        );
    }
    else if (ElementType == TEXT("StoreButton"))
    {
        bStoreButtonOnCooldown = true;
        GetWorld()->GetTimerManager().SetTimer(
            StoreButtonCooldownTimer,
            [this]() { bStoreButtonOnCooldown = false; },
            UIButtonPressCooldown,
            false
        );
    }
}

void UUITouchInputManager::ResetUIElementCooldown(const FString& ElementType)
{
    if (ElementType == TEXT("Tab"))
    {
        bTabButtonOnCooldown = false;
        GetWorld()->GetTimerManager().ClearTimer(TabButtonCooldownTimer);
    }
    else if (ElementType == TEXT("InventorySlot"))
    {
        bInventorySlotOnCooldown = false;
        GetWorld()->GetTimerManager().ClearTimer(InventorySlotCooldownTimer);
    }
    else if (ElementType == TEXT("EquipmentSlot"))
    {
        bEquipmentSlotOnCooldown = false;
        GetWorld()->GetTimerManager().ClearTimer(EquipmentSlotCooldownTimer);
    }
    else if (ElementType == TEXT("StoreButton"))
    {
        bStoreButtonOnCooldown = false;
        GetWorld()->GetTimerManager().ClearTimer(StoreButtonCooldownTimer);
    }
}

bool UUITouchInputManager::IsTouchPlatform() const
{
    // Check if we're running on a touch-capable platform
    #if PLATFORM_ANDROID || PLATFORM_IOS
        return true;
    #else
        // For desktop platforms, return false by default
        // Can be manually enabled via SetUITouchInputEnabled(true)
        return false;
    #endif
}

void UUITouchInputManager::FindAndSetupInventorySlots()
{
    if (!MainWidgetRef || !MainWidgetRef->WBP_Inventory)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: No inventory widget found"));
        return;
    }
    
    // Get all inventory slot widgets
    TArray<UWBP_InventorySlot*> InventorySlots = MainWidgetRef->WBP_Inventory->GetInventorySlotWidgets();
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Found %d inventory slots"), InventorySlots.Num());
    
    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (InventorySlots[i])
        {
            AddTouchHandlersToWidget(InventorySlots[i], TEXT("InventorySlot"));
        }
    }
}

void UUITouchInputManager::FindAndSetupEquipmentSlots()
{
    if (!MainWidgetRef || !MainWidgetRef->WBP_CharacterInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: No character info widget found"));
        return;
    }
    
    // Setup touch input for equipment slots
    if (MainWidgetRef->WBP_CharacterInfo->HeadSlot)
    {
        AddTouchHandlersToWidget(MainWidgetRef->WBP_CharacterInfo->HeadSlot, TEXT("HeadSlot"));
    }
    
    if (MainWidgetRef->WBP_CharacterInfo->WeaponSlot)
    {
        AddTouchHandlersToWidget(MainWidgetRef->WBP_CharacterInfo->WeaponSlot, TEXT("WeaponSlot"));
    }
    
    if (MainWidgetRef->WBP_CharacterInfo->SuitSlot)
    {
        AddTouchHandlersToWidget(MainWidgetRef->WBP_CharacterInfo->SuitSlot, TEXT("SuitSlot"));
    }
    
    if (MainWidgetRef->WBP_CharacterInfo->CollectableSlot)
    {
        AddTouchHandlersToWidget(MainWidgetRef->WBP_CharacterInfo->CollectableSlot, TEXT("CollectableSlot"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Setup touch input for equipment slots"));
}

void UUITouchInputManager::FindAndSetupStoreElements()
{
    UWBP_Store* StoreWidget = MainWidgetRef ? MainWidgetRef->GetStoreWidget() : nullptr;
    if (!MainWidgetRef || !StoreWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: No store widget found"));
        return;
    }
    
    // Setup touch input for store buy button
    if (StoreWidget->BuyButton)
    {
        AddTouchHandlersToWidget(StoreWidget->BuyButton, TEXT("StoreBuyButton"));
    }
    
    // Setup touch input for store popup buttons if they exist
    UWBP_StorePopup* StorePopup = MainWidgetRef->GetStorePopupWidget();
    if (StorePopup)
    {
        if (StorePopup->ConfirmButton)
        {
            AddTouchHandlersToWidget(StorePopup->ConfirmButton, TEXT("StoreConfirmButton"));
        }
        
        if (StorePopup->CancelButton)
        {
            AddTouchHandlersToWidget(StorePopup->CancelButton, TEXT("StoreCancelButton"));
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Setup touch input for store elements"));
}

void UUITouchInputManager::FindAndSetupTabNavigation()
{
    if (!MainWidgetRef)
    {
        return;
    }
    
    // The tab navigation buttons are handled through the existing button binding system
    // in WBP_Main.cpp, so we just ensure they have proper touch input enabled
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 UITouchInputManager: Tab navigation touch input is handled by existing button bindings"));
}

AAtlantisEonsCharacter* UUITouchInputManager::GetOwnerCharacter()
{
    return Cast<AAtlantisEonsCharacter>(GetOwner());
} 