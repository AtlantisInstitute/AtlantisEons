#include "WBP_ContextMenu.h"
#include "WBP_InventorySlot.h"

void UWBP_ContextMenu::NativePreConstruct()
{
    Super::NativePreConstruct();

    // Set up text for buttons
    if (Use_Text)
    {
        Use_Text->SetText(FText::FromString(TEXT("Use")));
    }

    if (Throw_Text)
    {
        Throw_Text->SetText(FText::FromString(TEXT("Throw")));
    }

    if (Close_Text)
    {
        Close_Text->SetText(FText::FromString(TEXT("Close")));
    }
}

void UWBP_ContextMenu::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind button click events
    if (Use)
    {
        Use->OnClicked.AddDynamic(this, &UWBP_ContextMenu::OnUseClicked);
    }

    if (Throw)
    {
        Throw->OnClicked.AddDynamic(this, &UWBP_ContextMenu::OnThrowClicked);
    }

    if (Close)
    {
        Close->OnClicked.AddDynamic(this, &UWBP_ContextMenu::OnCloseClicked);
    }
}

void UWBP_ContextMenu::OnUseClicked()
{
    if (InventorySlotRef)
    {
        InventorySlotRef->ContextMenuUse();
        // Close the context menu after the action
        CloseContextMenu();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Context Menu: InventorySlotRef is null!"));
        // Still close the menu even if ref is null
        CloseContextMenu();
    }
}

void UWBP_ContextMenu::OnThrowClicked()
{
    if (InventorySlotRef)
    {
        InventorySlotRef->OnContextMenuThrow();
        // Close the context menu after the action
        CloseContextMenu();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Context Menu: InventorySlotRef is null!"));
        // Still close the menu even if ref is null
        CloseContextMenu();
    }
}

void UWBP_ContextMenu::OnCloseClicked()
{
    CloseContextMenu();
}

void UWBP_ContextMenu::CloseContextMenu()
{
    // If we have a valid MenuAnchorRef, close it (traditional approach)
    if (MenuAnchorRef && IsValid(MenuAnchorRef))
    {
        MenuAnchorRef->Close();
    }
    
    // If we're added directly to viewport (our current approach), remove from viewport
    if (IsInViewport())
    {
        RemoveFromParent();
    }
    
    // Don't clear references immediately - let them be cleared naturally
    // This prevents the null reference errors we were seeing
}
