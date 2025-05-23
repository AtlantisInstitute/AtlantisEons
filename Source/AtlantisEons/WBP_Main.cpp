#include "WBP_Main.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "WBP_TopMenu.h"
#include "WBP_CharacterInfo.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetTextLibrary.h"
#include "AtlantisEonsCharacter.h"

void UWBP_Main::NativePreConstruct()
{
    Super::NativePreConstruct();
    UE_LOG(LogTemp, Warning, TEXT("WBP_Main::NativePreConstruct() called"));
}

void UWBP_Main::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UE_LOG(LogTemp, Warning, TEXT("WBP_Main::NativeOnInitialized() called"));
}

void UWBP_Main::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTemp, Warning, TEXT("%s::NativeConstruct() called"), *GetName());

    // Setup menu buttons first
    SetupMenuButtons();

    // Ensure widget switcher is valid
    if (!IsValid(WidgetSwitcher))
    {
        UE_LOG(LogTemp, Error, TEXT("%s: WidgetSwitcher is invalid! Make sure 'Is Variable' is checked in the widget blueprint"), *GetName());
        return;
    }

    // Log widget switcher details
    int32 NumWidgets = WidgetSwitcher->GetNumWidgets();
    UE_LOG(LogTemp, Warning, TEXT("%s: WidgetSwitcher has %d widgets"), *GetName(), NumWidgets);
    
    // Log each widget in the switcher
    for (int32 i = 0; i < NumWidgets; ++i)
    {
        if (UWidget* Widget = WidgetSwitcher->GetWidgetAtIndex(i))
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Widget at index %d: %s"), *GetName(), i, *Widget->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Invalid widget at index %d"), *GetName(), i);
        }
    }

    // Initialize popup visibility - only if the popup widget exists
    // The WBP_StorePopup is optional, so we need to check if it exists
    if (WBP_StorePopup)
    {
        WBP_StorePopup->SetVisibility(ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Warning, TEXT("%s: StorePopup found and initialized"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: StorePopup not found - this is ok if you're not using it"), *GetName());
    }

    // Setup navigation events for popups as seen in the Blueprint
    SetupPopupNavigation();

    // Set up character info widget with more robust character finding
    AAtlantisEonsCharacter* Character = nullptr;
    
    // Try multiple methods to find the character
    // Method 1: Direct GetPlayerCharacter
    if (!Character)
    {
        if (ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0))
        {
            Character = Cast<AAtlantisEonsCharacter>(PlayerCharacter);
            if (Character)
            {
                UE_LOG(LogTemp, Warning, TEXT("%s: Found character via GetPlayerCharacter"), *GetName());
            }
        }
    }
    
    // Method 2: Player Controller's Character
    if (!Character)
    {
        if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
        {
            if (ACharacter* PlayerCharacter = PC->GetCharacter())
            {
                Character = Cast<AAtlantisEonsCharacter>(PlayerCharacter);
                if (Character)
                {
                    UE_LOG(LogTemp, Warning, TEXT("%s: Found character via PlayerController"), *GetName());
                }
            }
        }
    }
    
    // Method 3: Find in World
    if (!Character && GetWorld())
    {
        for (TActorIterator<AAtlantisEonsCharacter> It(GetWorld()); It; ++It)
        {
            Character = *It;
            if (Character)
            {
                UE_LOG(LogTemp, Warning, TEXT("%s: Found character via World Iterator"), *GetName());
                break;
            }
        }
    }
    
    // Set the character on the widget if found
    if (Character)
    {
        if (WBP_CharacterInfo)
        {
            // Set character with detailed logging
            UE_LOG(LogTemp, Warning, TEXT("%s: Setting Character = %s on WBP_CharacterInfo = %s"), 
                *GetName(), *Character->GetName(), *WBP_CharacterInfo->GetName());
            
            WBP_CharacterInfo->Character = Character;
            
            // Double-check if the reference was properly set
            if (WBP_CharacterInfo->Character == Character)
            {
                UE_LOG(LogTemp, Warning, TEXT("%s: Character reference set successfully"), *GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("%s: Character reference NOT set correctly - assignment failed"), *GetName());
            }
            
            // Verify the widget's type and functionality
            UE_LOG(LogTemp, Warning, TEXT("%s: CharacterInfo widget class is: %s"), *GetName(), *WBP_CharacterInfo->GetClass()->GetName());
            
            // Make sure we call these functions in this order
            WBP_CharacterInfo->InitializeCharacterPreview();
            WBP_CharacterInfo->UpdateAllStats();
            UE_LOG(LogTemp, Warning, TEXT("%s: Initial widget setup complete"), *GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: CharacterInfo widget is null!"), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to find any player character in the world!"), *GetName());
    }

    // Set initial widget to character info (index 0)
    WidgetSwitcher->SetActiveWidgetIndex(0);
    UpdateCharacterInfo();
    UE_LOG(LogTemp, Warning, TEXT("WBP_Main::NativeConstruct() - Initial widget setup complete"));
}

void UWBP_Main::SetupMenuButtons()
{
    UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() called"));

    if (WBP_TopMenu_1)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() - WBP_TopMenu_1 is valid"));
        if (WBP_TopMenu_1->TextBlock_0)
        {
            UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() - Setting up Profile button text"));
            WBP_TopMenu_1->TextBlock_0->SetText(FText::FromString(TEXT("Profile")));
            if (WBP_TopMenu_1->Button_0)
            {
                UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() - Binding Profile button click"));
                WBP_TopMenu_1->Button_0->OnClicked.AddDynamic(this, &UWBP_Main::Switcher1);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("WBP_Main::SetupMenuButtons() - Profile button is null!"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("WBP_Main::SetupMenuButtons() - Profile text block is null!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Main::SetupMenuButtons() - WBP_TopMenu_1 is null!"));
    }

    if (WBP_TopMenu_2)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() - WBP_TopMenu_2 is valid"));
        if (WBP_TopMenu_2->TextBlock_0)
        {
            UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() - Setting up Inventory button text"));
            WBP_TopMenu_2->TextBlock_0->SetText(FText::FromString(TEXT("Inventory")));
            if (WBP_TopMenu_2->Button_0)
            {
                UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() - Binding Inventory button click"));
                WBP_TopMenu_2->Button_0->OnClicked.AddDynamic(this, &UWBP_Main::Switcher2);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("WBP_Main::SetupMenuButtons() - Inventory button is null!"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("WBP_Main::SetupMenuButtons() - Inventory text block is null!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Main::SetupMenuButtons() - WBP_TopMenu_2 is null!"));
    }

    if (WBP_TopMenu_3)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() - WBP_TopMenu_3 is valid"));
        if (WBP_TopMenu_3->TextBlock_0)
        {
            UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() - Setting up Store button text"));
            WBP_TopMenu_3->TextBlock_0->SetText(FText::FromString(TEXT("Store")));
            if (WBP_TopMenu_3->Button_0)
            {
                UE_LOG(LogTemp, Warning, TEXT("WBP_Main::SetupMenuButtons() - Binding Store button click"));
                WBP_TopMenu_3->Button_0->OnClicked.AddDynamic(this, &UWBP_Main::Switcher3);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("WBP_Main::SetupMenuButtons() - Store button is null!"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("WBP_Main::SetupMenuButtons() - Store text block is null!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Main::SetupMenuButtons() - WBP_TopMenu_3 is null!"));
    }
}

void UWBP_Main::Switcher1()
{
    UE_LOG(LogTemp, Warning, TEXT("WBP_Main::Switcher1() - Switching to Character Info"));
    if (!WidgetSwitcher)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Main::Switcher1() - WidgetSwitcher is null!"));
        return;
    }

    // Check and log character info widget details
    if (WBP_CharacterInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_Main::Switcher1() - Character info widget exists, class: %s"), *WBP_CharacterInfo->GetClass()->GetName());
        UE_LOG(LogTemp, Warning, TEXT("WBP_Main::Switcher1() - Character info widget visibility: %d"), (int32)WBP_CharacterInfo->GetVisibility());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Main::Switcher1() - Character info widget is null!"));
    }

    // Switch to character info (index 0)
    WidgetSwitcher->SetActiveWidgetIndex(0);
    
    // Update character info stats if we have the widget
    if (WBP_CharacterInfo)
    {
        // Log info about the widget state
        UE_LOG(LogTemp, Warning, TEXT("WBP_Main::Switcher1() - Widget at index 0: %s"), *WidgetSwitcher->GetWidgetAtIndex(0)->GetName());
        
        // Set the widget to visible explicitly
        WBP_CharacterInfo->SetVisibility(ESlateVisibility::Visible);
        
        // Update the stats
        WBP_CharacterInfo->UpdateAllStats();
        UE_LOG(LogTemp, Warning, TEXT("WBP_Main::Switcher1() - Updated character info stats"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Main::Switcher1() - Character info widget is null!"));
    }
}

void UWBP_Main::Switcher2()
{
    UE_LOG(LogTemp, Warning, TEXT("WBP_Main::Switcher2() - Switching to Inventory"));
    if (!WidgetSwitcher)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Main::Switcher2() - WidgetSwitcher is null!"));
        return;
    }

    // Switch to inventory (index 1)
    WidgetSwitcher->SetActiveWidgetIndex(1);
}

void UWBP_Main::Switcher3()
{
    UE_LOG(LogTemp, Warning, TEXT("WBP_Main::Switcher3() - Switching to Store"));
    if (!WidgetSwitcher)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Main::Switcher3() - WidgetSwitcher is null!"));
        return;
    }

    // Switch to store (index 2)
    WidgetSwitcher->SetActiveWidgetIndex(2);
}

FText UWBP_Main::GetText_Gold() const
{
    // Get the player character
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    
    // Cast to AtlantisEonsCharacter
    if (AAtlantisEonsCharacter* BPCharacter = Cast<AAtlantisEonsCharacter>(PlayerCharacter))
    {
        // Get YourGold value
        int32 YourGold = BPCharacter->GetYourGold();
        
        // Convert to text with specific parameters
        return UKismetTextLibrary::Conv_IntToText(YourGold, false, true, 1, 324);
    }
    
    // Return empty text if cast fails
    return FText::GetEmpty();
}

void UWBP_Main::UpdateCharacterInfo()
{
    if (WBP_CharacterInfo)
    {
        WBP_CharacterInfo->UpdateAllStats();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Character Info widget is invalid in UpdateCharacterInfo"), *GetName());
    }
}

void UWBP_Main::SetupPopupNavigation()
{
    // Even if we don't have a popup, we can still set up the store button
    if (WBP_Store && WBP_Store->BuyButton)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Setting up Store BuyButton"), *GetName());
        WBP_Store->BuyButton->OnClicked.AddDynamic(this, &UWBP_Main::HandleStoreItemPurchase);
    }
    
    // Only set up popup navigation if we have the popup widget
    if (WBP_StorePopup)
    {
        // Based on the Blueprint, we need to setup navigation between main widget and popup
        UE_LOG(LogTemp, Warning, TEXT("%s: Setting up popup navigation"), *GetName());
        
        // Connect the confirm and cancel buttons
        if (WBP_StorePopup->ConfirmButton)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Binding ConfirmButton"), *GetName());
            WBP_StorePopup->ConfirmButton->OnClicked.AddDynamic(this, &UWBP_Main::HandleStoreConfirmPurchase);
        }
        
        if (WBP_StorePopup->CancelButton)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Binding CancelButton"), *GetName());
            WBP_StorePopup->CancelButton->OnClicked.AddDynamic(this, &UWBP_Main::HandleStoreCancelPurchase);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: No StorePopup widget found, skipping popup navigation setup"), *GetName());
    }
}

void UWBP_Main::HandlePopupNavigation(bool bShowPopup)
{
    // If we don't have a popup widget, just log a message and return
    if (!WBP_StorePopup)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: StorePopup widget not found, can't %s popup"), 
               *GetName(), bShowPopup ? TEXT("show") : TEXT("hide"));
        return;
    }
    
    // Implement the navigation logic from the Blueprint
    if (bShowPopup)
    {
        // Show the popup
        WBP_StorePopup->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Warning, TEXT("%s: Showing store popup"), *GetName());
        
        // Update popup content if needed
        if (WBP_Store)
        {
            // Get selected item and update popup
            int32 SelectedItem = WBP_Store->GetSelectedItemIndex();
            WBP_StorePopup->UpdateItemDetails(SelectedItem);
        }
    }
    else
    {
        // Hide the popup
        WBP_StorePopup->SetVisibility(ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Warning, TEXT("%s: Hiding store popup"), *GetName());
    }

}

// Handle store purchase button clicked
void UWBP_Main::HandleStoreItemPurchase()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: Store Buy button clicked"), *GetName());
    HandlePopupNavigation(true);
}

// Handle store confirmation button clicked
void UWBP_Main::HandleStoreConfirmPurchase()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: Store Confirm button clicked"), *GetName());
    
    // Process the purchase
    if (WBP_Store)
    {
        WBP_Store->PurchaseSelectedItem();
    }
    
    // Hide the popup
    HandlePopupNavigation(false);
}

// Handle store cancel button clicked
void UWBP_Main::HandleStoreCancelPurchase()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: HandleStoreCancelPurchase called"), *GetName());
    
    // Hide the popup and show the main store widget
    HandlePopupNavigation(false);
}

void UWBP_Main::UpdateDisplayedGold()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: UpdateDisplayedGold called"), *GetName());
    
    // Update gold text if it exists
    if (IsValid(GoldText))
    {
        GoldText->SetText(GetText_Gold());
        UE_LOG(LogTemp, Warning, TEXT("%s: Gold display updated"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Gold text widget is invalid"), *GetName());
    }
}
