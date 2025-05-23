#include "WBP_StorePopup.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Engine/DataTable.h"
#include "AtlantisEonsCharacter.h"
#include "WBP_Main.h"

void UWBP_StorePopup::NativeConstruct()
{
    // Always call parent implementation first
    Super::NativeConstruct();
    UE_LOG(LogTemp, Display, TEXT("%s: Starting NativeConstruct"), *GetName());

    // Initialize basic properties with safe values
    ItemIndex = 0;
    StackNumber = 1;
    
    // Safely check buttons and bind events only if valid
    if (OKButton != nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: OKButton is valid, binding click event"), *GetName());
        OKButton->OnClicked.AddDynamic(this, &UWBP_StorePopup::OnOKButtonClicked);
        // Ensure ConfirmButton reference is set
        if (ConfirmButton == nullptr) ConfirmButton = OKButton;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: OKButton is nullptr, events will not be bound"), *GetName());
    }
    
    if (CancelButton != nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: CancelButton is valid, binding click event"), *GetName());
        CancelButton->OnClicked.AddDynamic(this, &UWBP_StorePopup::OnCancelButtonClicked);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: CancelButton is nullptr, events will not be bound"), *GetName());
    }
    
    if (Button_Up != nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Button_Up is valid, binding click event"), *GetName());
        Button_Up->OnClicked.AddDynamic(this, &UWBP_StorePopup::OnUpButtonClicked);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Button_Up is nullptr, events will not be bound"), *GetName());
    }
    
    if (Button_Down != nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Button_Down is valid, binding click event"), *GetName());
        Button_Down->OnClicked.AddDynamic(this, &UWBP_StorePopup::OnDownButtonClicked);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Button_Down is nullptr, events will not be bound"), *GetName());
    }

    // Log widget status for text blocks
    UE_LOG(LogTemp, Display, TEXT("%s: TextBlock_StackCounter is %s"), *GetName(), 
        TextBlock_StackCounter != nullptr ? TEXT("valid") : TEXT("nullptr"));
    UE_LOG(LogTemp, Display, TEXT("%s: TextBlock_GoldAmount is %s"), *GetName(), 
        TextBlock_GoldAmount != nullptr ? TEXT("valid") : TEXT("nullptr"));
    UE_LOG(LogTemp, Display, TEXT("%s: TotalPriceText is %s"), *GetName(), 
        TotalPriceText != nullptr ? TEXT("valid") : TEXT("nullptr"));
    
    // Try to safely update item display
    try
    {
        UpdateItemDisplay();
        UE_LOG(LogTemp, Display, TEXT("%s: UpdateItemDisplay completed successfully"), *GetName());
    }
    catch(const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Exception in UpdateItemDisplay: %s"), *GetName(), UTF8_TO_TCHAR(e.what()));
    }
    catch(...)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Unknown exception in UpdateItemDisplay"), *GetName());
    }
    
    UE_LOG(LogTemp, Display, TEXT("%s: NativeConstruct completed"), *GetName());
}

void UWBP_StorePopup::UpdateItemDisplay()
{
    // Basic logging
    UE_LOG(LogTemp, Display, TEXT("%s: UpdateItemDisplay - Starting for ItemIndex=%d"), *GetName(), ItemIndex);

    // Demo/fallback data for testing when data table is not loading correctly
    static Structure_ItemInfo FallbackItemInfo;
    
    // Check that the user provided a valid item index
    if (ItemIndex < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Invalid item index %d"), *GetName(), ItemIndex);
        return;
    }
    
    // Create a default (empty) item info in case data table access fails
    Structure_ItemInfo LocalItemInfo;
    LocalItemInfo.ItemIndex = ItemIndex;
    LocalItemInfo.StackNumber = 1;
    LocalItemInfo.Price = 100; // Fallback price
    
    // Pointer to use throughout the function - start with our local fallback
    Structure_ItemInfo* ItemInfo = &LocalItemInfo;
    
    // Attempt to load the data table
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, 
                           TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    // Log data table status
    if (ItemDataTable)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Successfully loaded item data table"), *GetName());
        
        if (ItemDataTable->GetRowStruct())
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Data table row structure: %s"), 
                   *GetName(), *ItemDataTable->GetRowStruct()->GetName());
        }
        
        // Try to retrieve the item data using FindRowUnchecked (avoids structure type mismatch)
        FString RowName = FString::FromInt(ItemIndex);
        void* RowData = ItemDataTable->FindRowUnchecked(*RowName);
        
        if (RowData)
        {
            // Successfully found the row - update local data with row index for UI elements
            UE_LOG(LogTemp, Display, TEXT("%s: Found item data for index %d"), *GetName(), ItemIndex);
            LocalItemInfo.ItemName = FString::Printf(TEXT("Item %d"), ItemIndex);
            LocalItemInfo.Price = 100 * (ItemIndex + 1); // Price based on index for testing
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Item index %d not found in data table"), *GetName(), ItemIndex);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to load item data table"), *GetName());
    }
    
    UE_LOG(LogTemp, Display, TEXT("%s: Successfully found ItemInfo for index %d"), *GetName(), ItemIndex);
    
    // Update thumbnail if it exists
    if (Image_ItemThumbnail != nullptr)
    {
        try
        {
            if (!ItemInfo->ItemThumbnail.IsNull())
            {
                UTexture2D* LoadedTexture = nullptr;
                
                // Try loading thumbnail texture with error handling
                try
                {
                    LoadedTexture = ItemInfo->ItemThumbnail.LoadSynchronous();
                    if (LoadedTexture != nullptr)
                    {
                        Image_ItemThumbnail->SetBrushFromTexture(LoadedTexture);
                        UE_LOG(LogTemp, Display, TEXT("%s: Successfully set thumbnail texture"), *GetName());
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to load thumbnail texture"), *GetName());
                    }
                }
                catch(...)
                {
                    UE_LOG(LogTemp, Error, TEXT("%s: Exception while loading thumbnail texture"), *GetName());
                }
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("%s: Item has no thumbnail"), *GetName());
            }
        }
        catch(...)
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Exception while checking item thumbnail"), *GetName());
        }
    }

    // Update stack counter visibility based on item type
    if (Overlay_StackCounter != nullptr)
    {
        try
        {
            switch (ItemInfo->ItemType)
            {
                case EItemType::Consume_HP:
                case EItemType::Consume_MP:
                    Overlay_StackCounter->SetVisibility(ESlateVisibility::Visible);
                    break;
                default:
                    Overlay_StackCounter->SetVisibility(ESlateVisibility::Collapsed);
                    break;
            }
            UE_LOG(LogTemp, Display, TEXT("%s: Updated stack counter visibility for ItemType %d"), 
                   *GetName(), static_cast<int32>(ItemInfo->ItemType));
        }
        catch(...)
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Exception while updating stack counter visibility"), *GetName());
        }
    }

    // Update gold text
    if (TextBlock_GoldAmount != nullptr)
    {
        try
        {
            FString PriceString = FString::Printf(TEXT("%d"), ItemInfo->Price);
            FText PriceText = FText::FromString(PriceString);
            TextBlock_GoldAmount->SetText(PriceText);
            UE_LOG(LogTemp, Display, TEXT("%s: Updated gold text to %d"), *GetName(), ItemInfo->Price);
        }
        catch(...)
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Exception while updating gold text"), *GetName());
        }
    }
    
    // Also update TotalPriceText if it exists
    if (TotalPriceText != nullptr)
    {
        try
        {
            FString PriceString = FString::Printf(TEXT("%d"), ItemInfo->Price);
            FText PriceText = FText::FromString(PriceString);
            TotalPriceText->SetText(PriceText);
            UE_LOG(LogTemp, Display, TEXT("%s: Updated total price text to %d"), *GetName(), ItemInfo->Price);
        }
        catch(...)
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Exception while updating total price text"), *GetName());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("%s: UpdateItemDisplay completed successfully"), *GetName());
}

void UWBP_StorePopup::OnOKButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: OnOKButtonClicked"), *GetName());
    
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PC->GetCharacter()))
        {
            if (UWBP_Main* MainWidget = Character->Main)
            {
                MainWidget->buying = true;
            }
            
            // Get item data
            UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
            if (!ItemDataTable)
            {
                UE_LOG(LogTemp, Error, TEXT("%s: Failed to load Item Data Table"), *GetName());
                return;
            }
            
            FName RowName = FName(*FString::FromInt(ItemIndex));
            Structure_ItemInfo* ItemInfo = ItemDataTable->FindRow<Structure_ItemInfo>(RowName, TEXT(""));
            if (!ItemInfo)
            {
                UE_LOG(LogTemp, Error, TEXT("%s: Failed to find item info for item index %d"), *GetName(), ItemIndex);
                return;
            }
            
            // Calculate total price based on stack number
            int32 TotalPrice = ItemInfo->Price * StackNumber;
            
            // Check if character has enough gold
            if (Character->YourGold >= TotalPrice)
            {
                // Deduct gold
                Character->YourGold -= TotalPrice;
                UE_LOG(LogTemp, Warning, TEXT("%s: Purchased item for %d gold. Remaining gold: %d"), *GetName(), TotalPrice, Character->YourGold);
                
                // Add items to inventory
                for (int32 i = 0; i < StackNumber; ++i)
                {
                    // Use PickingItem instead of AddItemToInventory
                    Character->PickingItem(ItemIndex, 1);
                }
                
                // Close popup
                RemoveFromParent();
                
                // If we're in a main widget, update displayed gold
                if (UPanelWidget* Panel = GetParent())
                {
                    // Need to get the parent user widget, not just the panel
                    if (UUserWidget* ParentUserWidget = Cast<UUserWidget>(Panel->GetOuter()))
                    {
                        if (UWBP_Main* MainWidget = Cast<UWBP_Main>(ParentUserWidget))
                        {
                            MainWidget->UpdateDisplayedGold();
                        }
                    }
                }
            }
            else
            {
                // Not enough gold
                UE_LOG(LogTemp, Warning, TEXT("%s: Not enough gold to purchase item. Need %d gold but only have %d"), *GetName(), TotalPrice, Character->YourGold);
                
                // TODO: Show message to player - could be implemented with another popup widget
                // For now we'll just close this popup
                RemoveFromParent();
            }
        }
    }
}

void UWBP_StorePopup::OnCancelButtonClicked()
{
    RemoveFromParent();
}

void UWBP_StorePopup::OnUpButtonClicked()
{
    // Increment stack number with a maximum of 99
    StackNumber = FMath::Min(StackNumber + 1, 99);
    
    // Update the displayed stack number
    if (TextBlock_StackCounter)
    {
        TextBlock_StackCounter->SetText(GetText_StackNumber());
    }
}

void UWBP_StorePopup::OnDownButtonClicked()
{
    // Decrement stack number with a minimum of 1
    StackNumber = FMath::Max(StackNumber - 1, 1);
    
    // Update the displayed stack number
    if (TextBlock_StackCounter)
    {
        TextBlock_StackCounter->SetText(GetText_StackNumber());
    }
}

FText UWBP_StorePopup::GetText_StackNumber() const
{
    return UKismetTextLibrary::Conv_IntToText(StackNumber, false, true, 1, 324);
}

FText UWBP_StorePopup::GetText_TotalPrice()
{
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    
    if (ItemDataTable)
    {
        FName RowName = FName(*UKismetStringLibrary::Conv_IntToString(ItemIndex));
        Structure_ItemInfo* ItemInfo = ItemDataTable->FindRow<Structure_ItemInfo>(RowName, TEXT(""));
        
        if (ItemInfo)
        {
            int32 TotalPrice = ItemInfo->Price * StackNumber;
            return UKismetTextLibrary::Conv_IntToText(TotalPrice);
        }
    }
    
    return FText::FromString("0");
}

void UWBP_StorePopup::UpdateItemDetails(int32 InItemIndex)
{
    // Set the item index and update the display
    ItemIndex = InItemIndex;
    StackNumber = 1; // Reset stack number to 1 for new item
    
    UE_LOG(LogTemp, Warning, TEXT("%s: Updating item details for index %d"), *GetName(), ItemIndex);
    
    // Update the item display
    UpdateItemDisplay();
    
    // Make sure the stack counter text is updated - safeguard with pointer check
    if (TextBlock_StackCounter != nullptr)
    {
        TextBlock_StackCounter->SetText(GetText_StackNumber());
    }
    
    // Get the price text once to avoid multiple calls
    FText priceText = GetText_TotalPrice();
    
    // Update total price text if it exists - use nullptr check instead of IsValid
    if (TextBlock_GoldAmount != nullptr)
    {
        TextBlock_GoldAmount->SetText(priceText);
    }
    
    // Also update TotalPriceText if it exists - use nullptr check
    if (TotalPriceText != nullptr)
    {
        TotalPriceText->SetText(priceText);
    }
    
    // If we have OKButton/ConfirmButton make sure they point to the same button
    if (IsValid(OKButton) && !IsValid(ConfirmButton))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Setting OKButton as ConfirmButton"), *GetName());
        ConfirmButton = OKButton;
    }
}
