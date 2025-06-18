#include "CharacterUIManager.h"
#include "AtlantisEonsCharacter.h"
#include "WBP_Main.h"
#include "WBP_CharacterInfo.h"
#include "WBP_InventorySlot.h"
#include "WBP_Inventory.h"
#include "WBP_Store.h"
#include "BP_ItemInfo.h"
#include "InventoryComponent.h"
#include "CharacterStatsComponent.h"
#include "EquipmentComponent.h"
#include "AtlantisEonsHUD.h"
#include "Components/ProgressBar.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Engine.h"

UCharacterUIManager::UCharacterUIManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    OwnerCharacter = nullptr;
    MainWidget = nullptr;
    CircularHPMaterial = nullptr;
    CircularMPMaterial = nullptr;
}

void UCharacterUIManager::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache reference to owning character
    OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterUIManager: Owner is not an AtlantisEonsCharacter!"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("CharacterUIManager: Initialized for character %s"), *OwnerCharacter->GetName());
}

void UCharacterUIManager::InitializeUI()
{
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterUIManager::InitializeUI: OwnerCharacter is null"));
        return;
    }
    
    // DISABLED: Don't create any UI widgets at startup to ensure gameplay works first
    UE_LOG(LogTemp, Warning, TEXT("CharacterUIManager: InitializeUI disabled to focus on gameplay first"));
    
    // Connect inventory component to HUD when MainWidget is available
    ConnectInventoryToMainWidget();
}

void UCharacterUIManager::ConnectInventoryToMainWidget()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Try to get the HUD and its MainWidget
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        if (AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(PC->GetHUD()))
        {
            UWBP_Main* MainWidget = HUD->GetMainWidget();
            if (MainWidget && OwnerCharacter->GetInventoryComponent())
            {
                OwnerCharacter->GetInventoryComponent()->SetMainWidget(MainWidget);
                UE_LOG(LogTemp, Warning, TEXT("✅ Connected InventoryComponent to MainWidget"));
            }
            else if (!MainWidget)
            {
                UE_LOG(LogTemp, Warning, TEXT("⏳ MainWidget not yet available, will connect later"));
            }
        }
    }
}

void UCharacterUIManager::SetupCircularBars()
{
    if (MainWidget)
    {
        // Initialize HP and MP bars
        SettingCircularBar_HP();
        SettingCircularBar_MP();
    }
}

void UCharacterUIManager::SettingCircularBar_HP()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Early exit if we don't have a valid controller (prevents crashes during thumbnail generation)
    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_HP: No valid PlayerController"));
        return;
    }
    
    // Try multiple approaches to update the HP bar
    bool bUpdated = false;
    
    // Method 1: Use WBP_CharacterInfo if available
    if (OwnerCharacter->WBP_CharacterInfo)
    {
        OwnerCharacter->WBP_CharacterInfo->UpdateHPBar();
        bUpdated = true;
        UE_LOG(LogTemp, Log, TEXT("Updated HP bar via WBP_CharacterInfo"));
    }
    
    // Method 2: Use Main widget if available
    if (OwnerCharacter->Main)
    {
        // Update HP bar progress
        float HPPercentage = OwnerCharacter->MaxHealth > 0 ? OwnerCharacter->CurrentHealth / OwnerCharacter->MaxHealth : 0.0f;
        if (OwnerCharacter->HPBar)
        {
            OwnerCharacter->HPBar->SetPercent(HPPercentage);
            bUpdated = true;
            UE_LOG(LogTemp, Log, TEXT("Updated HP bar via Main widget: %.2f%%"), HPPercentage * 100.0f);
        }
    }
    
    if (!bUpdated)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_HP: Could not update HP bar - no valid UI references"));
    }
}

void UCharacterUIManager::SettingCircularBar_MP()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Early exit if we don't have a valid controller (prevents crashes during thumbnail generation)
    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_MP: No valid PlayerController"));
        return;
    }
    
    // Try multiple approaches to update the MP bar
    bool bUpdated = false;
    
    // Method 1: Use WBP_CharacterInfo if available
    if (OwnerCharacter->WBP_CharacterInfo)
    {
        float MPPercentage = OwnerCharacter->MaxMP > 0 ? OwnerCharacter->CurrentMP / (float)OwnerCharacter->MaxMP : 0.0f;
        OwnerCharacter->WBP_CharacterInfo->UpdateMPBar(MPPercentage);
        bUpdated = true;
        UE_LOG(LogTemp, Log, TEXT("Updated MP bar via WBP_CharacterInfo"));
    }
    
    // Method 2: Use Main widget if available
    if (OwnerCharacter->Main)
    {
        // Update MP bar progress
        float MPPercentage = OwnerCharacter->MaxMP > 0 ? OwnerCharacter->CurrentMP / (float)OwnerCharacter->MaxMP : 0.0f;
        if (OwnerCharacter->MPBar)
        {
            OwnerCharacter->MPBar->SetPercent(MPPercentage);
            bUpdated = true;
            UE_LOG(LogTemp, Log, TEXT("Updated MP bar via Main widget: %.2f%%"), MPPercentage * 100.0f);
        }
    }
    
    // Method 3: Use Secondary HUD if available
    if (OwnerCharacter->SecondaryHUDWidget)
    {
        OwnerCharacter->SecondaryHUDWidget->UpdateManaBar();
        bUpdated = true;
        UE_LOG(LogTemp, Log, TEXT("Updated MP bar via SecondaryHUD"));
    }
    
    if (!bUpdated)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_MP: Could not update MP bar - no valid UI references"));
    }
}

void UCharacterUIManager::UpdateCircularBar_HP()
{
    SettingCircularBar_HP();
}

void UCharacterUIManager::UpdateCircularBar_MP()
{
    SettingCircularBar_MP();
}

void UCharacterUIManager::RefreshStatsDisplay()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Force update the character info widget display
    if (OwnerCharacter->WBP_CharacterInfo)
    {
        OwnerCharacter->WBP_CharacterInfo->UpdateAllStats();
    }
    
    // Update circular bars
    SettingCircularBar_HP();
    SettingCircularBar_MP();
    
    UE_LOG(LogTemp, Log, TEXT("Stats display refreshed"));
}

void UCharacterUIManager::UpdateAllStatsUI()
{
    RefreshStatsDisplay();
}

void UCharacterUIManager::UpdateInventorySlots()
{
    if (!OwnerCharacter || !OwnerCharacter->GetInventoryComponent())
    {
        return;
    }
    
    OwnerCharacter->GetInventoryComponent()->UpdateInventorySlots();
}

void UCharacterUIManager::DelayedUpdateInventorySlots()
{
    UpdateInventorySlots();
}

void UCharacterUIManager::ForceUpdateInventorySlotsAfterDelay()
{
    if (!OwnerCharacter || !OwnerCharacter->bIsInventoryOpen)
    {
        return;
    }
    
    // Clear any existing timer
    GetWorld()->GetTimerManager().ClearTimer(InventoryUpdateTimer);
    
    // Set a small delay to ensure Blueprint logic completes first
    GetWorld()->GetTimerManager().SetTimer(
        InventoryUpdateTimer,
        this,
        &UCharacterUIManager::DelayedUpdateInventorySlots,
        0.1f,
        false
    );
}

void UCharacterUIManager::SetInventorySlot(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotWidgetRef)
{
    if (!ItemInfoRef || !InventorySlotWidgetRef) 
    {
        return;
    }
    
    // Update the inventory slot widget with the item info
    InventorySlotWidgetRef->UpdateSlot(ItemInfoRef);
}

void UCharacterUIManager::ClearAllInventoryPopups()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Clear all item info popups from inventory slots to prevent them from staying visible
    // when the inventory is closed while hovering over items
    
    UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: Starting comprehensive popup clearing"));
    
    if (!OwnerCharacter->MainWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: MainWidget is null"));
        return;
    }
    
    // Get the inventory widget
    UWBP_Inventory* InventoryWidget = OwnerCharacter->MainWidget->GetInventoryWidget();
    if (!InventoryWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: InventoryWidget is null"));
        return;
    }
    
    // Get all inventory slot widgets and clear their popups
    const TArray<UWBP_InventorySlot*>& InventorySlots = InventoryWidget->GetInventorySlotWidgets();
    
    int32 ClearedPopups = 0;
    for (UWBP_InventorySlot* Slot : InventorySlots)
    {
        if (Slot)
        {
            // Call the slot's RemoveItemDescription function to clear any active popups
            Slot->RemoveItemDescription();
            ClearedPopups++;
        }
    }
    
    // ENHANCED CLEARING: Also clear any ItemDescription widgets directly from each slot's reference
    // This ensures complete cleanup even if some widgets weren't properly removed
    for (UWBP_InventorySlot* Slot : InventorySlots)
    {
        if (Slot)
        {
            // Also clear the ItemDescription widget directly if it exists
            if (Slot->ItemDescription && Slot->ItemDescription->IsInViewport())
            {
                Slot->ItemDescription->RemoveFromParent();
                UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: Force-removed slot %d ItemDescription widget"), Slot->SlotIndex);
            }
            
            // Clear the WidgetItemDescriptionRef too
            if (Slot->WidgetItemDescriptionRef && Slot->WidgetItemDescriptionRef->IsInViewport())
            {
                Slot->WidgetItemDescriptionRef->RemoveFromParent();
                UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: Force-removed slot %d WidgetItemDescriptionRef"), Slot->SlotIndex);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClearAllInventoryPopups: Cleared popups from %d inventory slots with enhanced cleanup"), ClearedPopups);
    
    // ADDITIONAL SAFETY: Clear any focus from widgets that might be keeping tooltips alive
    if (GEngine && GEngine->GameViewport)
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
        UE_LOG(LogTemp, Log, TEXT("ClearAllInventoryPopups: Cleared widget focus to ensure tooltip cleanup"));
    }
}

void UCharacterUIManager::UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo)
{
    if (!OwnerCharacter || !OwnerCharacter->EquipmentComponent)
    {
        return;
    }
    
    OwnerCharacter->EquipmentComponent->UpdateEquipmentSlotUI(EquipSlot, ItemInfo);
}

void UCharacterUIManager::ClearEquipmentSlotUI(EItemEquipSlot EquipSlot)
{
    if (!OwnerCharacter || !OwnerCharacter->EquipmentComponent)
    {
        return;
    }
    
    OwnerCharacter->EquipmentComponent->ClearEquipmentSlotUI(EquipSlot);
}

void UCharacterUIManager::UpdateAllEquipmentSlotsUI()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Update all equipment slots based on currently equipped items
    for (int32 i = 0; i < OwnerCharacter->EquipmentSlots.Num(); ++i)
    {
        EItemEquipSlot SlotType = EItemEquipSlot::None;
        
        // Map array index to equipment slot type
        switch (i)
        {
            case 0:
                SlotType = EItemEquipSlot::Head;
                break;
            case 1:
                SlotType = EItemEquipSlot::Body;
                break;
            case 2:
                SlotType = EItemEquipSlot::Weapon;
                break;
            case 3:
                SlotType = EItemEquipSlot::Accessory;
                break;
            default:
                continue;
        }
        
        // Update the UI slot with the equipped item (or clear if none)
        UpdateEquipmentSlotUI(SlotType, OwnerCharacter->EquipmentSlots[i]);
    }
    
    UE_LOG(LogTemp, Log, TEXT("🔄 Updated all equipment slot UIs"));
}

void UCharacterUIManager::InitializeEquipmentSlotReferences()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Get equipment slot references from the character info widget
    if (OwnerCharacter->WBP_CharacterInfo)
    {
        OwnerCharacter->HeadSlot = OwnerCharacter->WBP_CharacterInfo->HeadSlot;
        OwnerCharacter->WeaponSlot = OwnerCharacter->WBP_CharacterInfo->WeaponSlot;
        OwnerCharacter->SuitSlot = OwnerCharacter->WBP_CharacterInfo->SuitSlot;
        OwnerCharacter->CollectableSlot = OwnerCharacter->WBP_CharacterInfo->CollectableSlot;
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Initialized equipment slot references from CharacterInfo widget"));
        UE_LOG(LogTemp, Warning, TEXT("   HeadSlot: %s"), OwnerCharacter->HeadSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   WeaponSlot: %s"), OwnerCharacter->WeaponSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   SuitSlot: %s"), OwnerCharacter->SuitSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   CollectableSlot: %s"), OwnerCharacter->CollectableSlot ? TEXT("Valid") : TEXT("NULL"));
        
        // Set up slot indices for equipment slots (different from inventory slots)
        if (OwnerCharacter->HeadSlot) 
        {
            OwnerCharacter->HeadSlot->SetSlotIndex(100); // Use high numbers to distinguish from inventory
            // Set slot type to Equipment to disable context menu
            OwnerCharacter->HeadSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            // Bind click event for unequipping
            OwnerCharacter->HeadSlot->OnSlotClicked.AddDynamic(OwnerCharacter, &AAtlantisEonsCharacter::OnHeadSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   HeadSlot configured as Equipment slot"));
        }
        if (OwnerCharacter->WeaponSlot) 
        {
            OwnerCharacter->WeaponSlot->SetSlotIndex(101);
            // Set slot type to Equipment to disable context menu
            OwnerCharacter->WeaponSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            OwnerCharacter->WeaponSlot->OnSlotClicked.AddDynamic(OwnerCharacter, &AAtlantisEonsCharacter::OnWeaponSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   WeaponSlot configured as Equipment slot"));
        }
        if (OwnerCharacter->SuitSlot) 
        {
            OwnerCharacter->SuitSlot->SetSlotIndex(102);
            // Set slot type to Equipment to disable context menu
            OwnerCharacter->SuitSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            OwnerCharacter->SuitSlot->OnSlotClicked.AddDynamic(OwnerCharacter, &AAtlantisEonsCharacter::OnSuitSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   SuitSlot configured as Equipment slot"));
        }
        if (OwnerCharacter->CollectableSlot) 
        {
            OwnerCharacter->CollectableSlot->SetSlotIndex(103);
            // Set slot type to Equipment to disable context menu
            OwnerCharacter->CollectableSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            OwnerCharacter->CollectableSlot->OnSlotClicked.AddDynamic(OwnerCharacter, &AAtlantisEonsCharacter::OnCollectableSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   CollectableSlot configured as Equipment slot"));
        }
        
        // Update all equipment slots after initialization
        UpdateAllEquipmentSlotsUI();
        
        // Update stats to ensure they reflect current equipment
        if (OwnerCharacter->StatsComponent)
        {
            OwnerCharacter->StatsComponent->UpdateAllStats();
        }
        
        // Force refresh the stats display
        RefreshStatsDisplay();
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("InitializeEquipmentSlotReferences: WBP_CharacterInfo is null"));
    }
}

void UCharacterUIManager::SettingStore()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Initialize store UI and data
    // TODO: Load store items and prices
    UE_LOG(LogTemp, Log, TEXT("CharacterUIManager: Store UI initialized"));
}

void UCharacterUIManager::SetMainWidget(UWBP_Main* NewWidget)
{
    MainWidget = NewWidget;
    
    if (!OwnerCharacter)
    {
        return;
    }
    
    if (NewWidget)
    {
        OwnerCharacter->MainWidget = NewWidget;
        UE_LOG(LogTemp, Display, TEXT("CharacterUIManager: Main widget set successfully"));
        
        // Get the character info widget from the main widget
        if (MainWidget->GetCharacterInfoWidget())
        {
            OwnerCharacter->WBP_CharacterInfo = MainWidget->GetCharacterInfoWidget();
            UE_LOG(LogTemp, Display, TEXT("CharacterUIManager: Character info widget set from main widget"));
        }
        
        // If inventory is currently open, update the slots immediately and with delay
        if (OwnerCharacter->bIsInventoryOpen)
        {
            UE_LOG(LogTemp, Display, TEXT("CharacterUIManager: Inventory is open, updating slots immediately and with delay"));
            UpdateInventorySlots();
            ForceUpdateInventorySlotsAfterDelay();
        }
        
        // Initialize equipment slot references and update UI
        InitializeEquipmentSlotReferences();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterUIManager: Attempted to set null Main widget"));
    }
}

void UCharacterUIManager::CacheUIReferences()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Cache main widget reference
    MainWidget = OwnerCharacter->Main;
    
    // Cache other UI references as needed
    // This can be expanded as we extract more UI functionality
}

bool UCharacterUIManager::ValidateUIReferences() const
{
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterUIManager: OwnerCharacter is null"));
        return false;
    }
    
    // Add validation for required UI references
    return true;
}

UWBP_InventorySlot* UCharacterUIManager::GetEquipmentSlotWidget(EItemEquipSlot EquipSlot) const
{
    if (!OwnerCharacter)
    {
        return nullptr;
    }
    
    switch (EquipSlot)
    {
        case EItemEquipSlot::Head:
            return OwnerCharacter->HeadSlot;
        case EItemEquipSlot::Weapon:
            return OwnerCharacter->WeaponSlot;
        case EItemEquipSlot::Body:
            return OwnerCharacter->SuitSlot;
        case EItemEquipSlot::Accessory:
            return OwnerCharacter->CollectableSlot;
        default:
            return nullptr;
    }
} 