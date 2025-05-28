#include "WBP_StorePopup.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Engine/DataTable.h"
#include "AtlantisEonsCharacter.h"
#include "WBP_Main.h"
#include "DataTableDiagnostic.h"
#include "StoreSystemFix.h"
#include "TextureDiagnostic.h"
#include "UniversalItemLoader.h"

void UWBP_StorePopup::NativeConstruct()
{
    Super::NativeConstruct();

    // Initialize basic properties with proper defaults
    if (ItemIndex == 0) ItemIndex = 1; // Default to item 1 if not set
    if (StackNumber == 0) StackNumber = 1; // Default stack
    if (SelectedQuantity == 0) SelectedQuantity = 1; // Default selected quantity

    // Setup button bindings with proper fallbacks
    SetupButtonBindings();
    
    // Update item display
    UpdateItemDisplay();
    
    // Force update quantity and price displays
    UpdateQuantityDisplay();
    UpdatePriceDisplay();
}

void UWBP_StorePopup::SetupButtonBindings()
{
    // Bind OK/Confirm button (try multiple names)
    UButton* OKBtn = OKButton ? OKButton : (ButtonOK ? ButtonOK : ConfirmButton);
    if (OKBtn)
    {
        OKBtn->OnClicked.Clear();
        OKBtn->OnClicked.AddDynamic(this, &UWBP_StorePopup::OnOKButtonClicked);
        ConfirmButton = OKBtn; // Set reference for WBP_Main
    }
    
    // Bind Cancel button
    UButton* CancelBtn = CancelButton ? CancelButton : ButtonCancel;
    if (CancelBtn)
    {
        CancelBtn->OnClicked.Clear();
        CancelBtn->OnClicked.AddDynamic(this, &UWBP_StorePopup::OnCancelButtonClicked);
    }
    
    // Bind Up button
    UButton* UpBtn = Button_Up ? Button_Up : UpButton;
    if (UpBtn)
    {
        UpBtn->OnClicked.Clear();
        UpBtn->OnClicked.AddDynamic(this, &UWBP_StorePopup::OnUpButtonClicked);
    }
    
    // Bind Down button
    UButton* DownBtn = Button_Down ? Button_Down : DownButton;
    if (DownBtn)
    {
        DownBtn->OnClicked.Clear();
        DownBtn->OnClicked.AddDynamic(this, &UWBP_StorePopup::OnDownButtonClicked);
    }
}

void UWBP_StorePopup::UpdateItemDisplay()
{
    // Update item name
    if (ItemNameText)
    {
        ItemNameText->SetText(FText::FromString(ItemInfo.ItemName));
    }

    // Update item description
    if (ItemDescriptionText)
    {
        ItemDescriptionText->SetText(FText::FromString(ItemInfo.ItemDescription));
    }

    // Load and set the item thumbnail using Universal Item Loader
    UTexture2D* LoadedTexture = UUniversalItemLoader::LoadItemTexture(ItemInfo);
    
    if (LoadedTexture)
    {
        // Update the confirmed ItemThumbnail widget
        if (ItemThumbnail)
        {
            ItemThumbnail->SetBrushFromTexture(LoadedTexture);
            ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
            ItemThumbnail->SetRenderOpacity(1.0f);
            UE_LOG(LogTemp, Log, TEXT("StorePopup: Set ItemThumbnail with texture %s"), *LoadedTexture->GetName());
        }

        // Update legacy thumbnail images
        if (ItemThumbnailImage)
        {
            ItemThumbnailImage->SetBrushFromTexture(LoadedTexture);
            ItemThumbnailImage->SetVisibility(ESlateVisibility::Visible);
            ItemThumbnailImage->SetRenderOpacity(1.0f);
        }

        if (Image_ItemThumbnail)
        {
            Image_ItemThumbnail->SetBrushFromTexture(LoadedTexture);
            Image_ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
            Image_ItemThumbnail->SetRenderOpacity(1.0f);
        }
        
        // Try to find any Image widget that might be the thumbnail
        TArray<FString> PossibleImageWidgetNames = {
            TEXT("ItemImage"),
            TEXT("Image_Item"),
            TEXT("Thumbnail"),
            TEXT("Image_Thumbnail"),
            TEXT("ItemIcon"),
            TEXT("Image_Icon"),
            TEXT("ProductImage"),
            TEXT("Image_Product")
        };
        
        for (const FString& WidgetName : PossibleImageWidgetNames)
        {
            if (UImage* ImageWidget = Cast<UImage>(GetWidgetFromName(*WidgetName)))
            {
                ImageWidget->SetBrushFromTexture(LoadedTexture);
                ImageWidget->SetVisibility(ESlateVisibility::Visible);
                ImageWidget->SetRenderOpacity(1.0f);
                UE_LOG(LogTemp, Log, TEXT("StorePopup: Set %s with texture"), *WidgetName);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("StorePopup: Failed to load texture for item %s"), *ItemInfo.ItemName);
        
        // Set a default texture if available
        UTexture2D* DefaultTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Engine/EditorResources/S_Actor")));
        if (DefaultTexture)
        {
            if (ItemThumbnail)
            {
                ItemThumbnail->SetBrushFromTexture(DefaultTexture);
                ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
            }
        }
    }

    // Update price displays
    UpdatePriceDisplay();
    
    // Update quantity widget visibility based on item type
    UpdateQuantityWidgetVisibility();
}

void UWBP_StorePopup::UpdateQuantityDisplay()
{
    // UE_LOG(LogTemp, Log, TEXT("StorePopup: Updating quantity display to %d"), SelectedQuantity);
    
    // Validate quantity
    if (SelectedQuantity <= 0)
    {
        SelectedQuantity = 1;
        // UE_LOG(LogTemp, Warning, TEXT("StorePopup: Fixed invalid SelectedQuantity, set to 1"));
    }
    
    // Convert to string
    FString QuantityString = FString::FromInt(SelectedQuantity);
    
    // Update Quantity widget if it exists
    if (Quantity)
    {
        // UE_LOG(LogTemp, Log, TEXT("StorePopup: Set Quantity widget to %s"), *QuantityString);
    }
    
    // Update QuantityText if it exists
    if (QuantityText)
    {
        // UE_LOG(LogTemp, Log, TEXT("StorePopup: Set QuantityText to %s"), *QuantityString);
    }
    
    // Update TextBlock_StackCounter if it exists
    if (TextBlock_StackCounter)
    {
        // UE_LOG(LogTemp, Log, TEXT("StorePopup: Set TextBlock_StackCounter to %s"), *QuantityString);
    }
    
    // Update StackCounterText if it exists  
    if (StackCounterText)
    {
        // UE_LOG(LogTemp, Log, TEXT("StorePopup: Set StackCounterText to %s"), *QuantityString);
    }
    
    // Try to find and update any text block that might be showing quantity
    // Expanded list of possible widget names based on common Blueprint naming patterns
    TArray<FString> PossibleQuantityWidgetNames = {
        TEXT("Quantity"),                    // USER CONFIRMED: This is the actual widget name
        TEXT("Text_Quantity"),
        TEXT("TextBlock_Quantity"), 
        TEXT("Quantity_Text"),
        TEXT("QuantityDisplay"),
        TEXT("StackDisplay"),
        TEXT("CountText"),
        TEXT("NumberText"),
        TEXT("Text_StackNumber"),
        TEXT("TextBlock_StackNumber"),
        TEXT("StackNumber_Text"),
        TEXT("Text_Count"),
        TEXT("TextBlock_Count"),
        TEXT("Count_Text"),
        TEXT("Text_Amount"),
        TEXT("TextBlock_Amount"),
        TEXT("Amount_Text"),
        TEXT("Text_Num"),
        TEXT("TextBlock_Num"),
        TEXT("Num_Text"),
        TEXT("Text_Stack"),
        TEXT("TextBlock_Stack"),
        TEXT("Stack_Text"),
        TEXT("QuantityLabel"),
        TEXT("StackLabel"),
        TEXT("CountLabel"),
        TEXT("NumberLabel"),
        TEXT("AmountLabel")
    };
    
    int32 WidgetsUpdated = 0;
    for (const FString& WidgetName : PossibleQuantityWidgetNames)
    {
        if (UTextBlock* QuantityWidget = Cast<UTextBlock>(GetWidgetFromName(*WidgetName)))
        {
            QuantityWidget->SetText(FText::FromString(QuantityString));
            WidgetsUpdated++;
            // UE_LOG(LogTemp, Log, TEXT("StorePopup: Set %s to %s"), *WidgetName, *QuantityString);
        }
    }
    
    // UE_LOG(LogTemp, Log, TEXT("StorePopup: Updated %d quantity widgets"), WidgetsUpdated);
    
    // Also update the legacy StackNumber for backwards compatibility
    StackNumber = SelectedQuantity;
    
    // Force a widget refresh
    if (GetRootWidget())
    {
        GetRootWidget()->InvalidateLayoutAndVolatility();
    }
}

void UWBP_StorePopup::UpdatePriceDisplay()
{
    int32 UnitPrice = ItemInfo.Price > 0 ? ItemInfo.Price : 100;
    int32 TotalPrice = UnitPrice * SelectedQuantity;
    
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Updating price - Unit: %d, Qty: %d, Total: %d"), UnitPrice, SelectedQuantity, TotalPrice);
    
    // Update unit price displays
    FText UnitPriceTextValue = FText::FromString(FString::Printf(TEXT("%d"), UnitPrice));
    if (UnitPriceText)
    {
        UnitPriceText->SetText(UnitPriceTextValue);
        UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Set UnitPriceText to %d"), UnitPrice);
    }
    
    // Update total price displays
    FText TotalPriceTextValue = FText::FromString(FString::Printf(TEXT("%d"), TotalPrice));
    
    if (TotalPriceText)
    {
        TotalPriceText->SetText(TotalPriceTextValue);
        UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Set TotalPriceText to %d"), TotalPrice);
    }
    
    if (PriceText)
    {
        PriceText->SetText(TotalPriceTextValue);
        UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Set PriceText to %d"), TotalPrice);
    }
    
    // USER CONFIRMED: Update the actual Price widget
    if (Price)
    {
        Price->SetText(TotalPriceTextValue);
        UE_LOG(LogTemp, Warning, TEXT("StorePopup: ✅ Set Price widget to %d"), TotalPrice);
    }
    
    if (TextBlock_GoldAmount)
    {
        TextBlock_GoldAmount->SetText(TotalPriceTextValue);
        UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Set TextBlock_GoldAmount to %d"), TotalPrice);
    }
    
    if (GoldAmountText)
    {
        GoldAmountText->SetText(TotalPriceTextValue);
        UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Set GoldAmountText to %d"), TotalPrice);
    }
    
    // Try to find and update any text block that might be showing price
    // Comprehensive list of possible price widget names
    TArray<FString> PossiblePriceWidgetNames = {
        TEXT("Price"),                       // USER CONFIRMED: This is the actual widget name
        TEXT("Text_Price"),
        TEXT("TextBlock_Price"),
        TEXT("Price_Text"),
        TEXT("PriceDisplay"),
        TEXT("TotalPrice"),
        TEXT("Text_TotalPrice"),
        TEXT("TextBlock_TotalPrice"),
        TEXT("TotalPrice_Text"),
        TEXT("Cost"),
        TEXT("Text_Cost"),
        TEXT("TextBlock_Cost"),
        TEXT("Cost_Text"),
        TEXT("Gold"),
        TEXT("Text_Gold"),
        TEXT("TextBlock_Gold"),
        TEXT("Gold_Text"),
        TEXT("Amount"),
        TEXT("Text_Amount"),
        TEXT("TextBlock_Amount"),
        TEXT("Amount_Text"),
        TEXT("PriceLabel"),
        TEXT("CostLabel"),
        TEXT("GoldLabel"),
        TEXT("AmountLabel")
    };
    
    int32 PriceWidgetsUpdated = 0;
    for (const FString& WidgetName : PossiblePriceWidgetNames)
    {
        if (UTextBlock* PriceWidget = Cast<UTextBlock>(GetWidgetFromName(*WidgetName)))
        {
            PriceWidget->SetText(TotalPriceTextValue);
            PriceWidgetsUpdated++;
            UE_LOG(LogTemp, Warning, TEXT("StorePopup: ✅ Set %s to %d"), *WidgetName, TotalPrice);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Price display updated - %s costs %d each, %d total (Updated %d price widgets)"), *ItemInfo.ItemName, UnitPrice, TotalPrice, PriceWidgetsUpdated);
}

void UWBP_StorePopup::OnIncreaseQuantityClicked()
{
    int32 MaxQuantity = ItemInfo.bIsStackable ? 99 : 1;
    if (SelectedQuantity < MaxQuantity)
    {
        SelectedQuantity++;
        UpdateQuantityDisplay();
        UpdatePriceDisplay();
    }
}

void UWBP_StorePopup::OnDecreaseQuantityClicked()
{
    if (SelectedQuantity > 1)
    {
        SelectedQuantity--;
        UpdateQuantityDisplay();
        UpdatePriceDisplay();
    }
}

void UWBP_StorePopup::OnConfirmPurchaseClicked()
{
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Confirm purchase - item %d, qty %d"), ItemInfo.ItemIndex, SelectedQuantity);

    // Get player character
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!PlayerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("StorePopup: No player character"));
        return;
    }

    AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PlayerCharacter);
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("StorePopup: Failed to cast to AtlantisEonsCharacter"));
        return;
    }

    // Calculate total cost
    int32 TotalCost = ItemInfo.Price * SelectedQuantity;
    
    // Check if player has enough gold
    if (Character->Gold < TotalCost)
    {
        UE_LOG(LogTemp, Warning, TEXT("StorePopup: Insufficient gold. Need %d, have %d"), TotalCost, Character->Gold);
        // Could show an error message here
        return;
    }

    // Deduct gold
    Character->Gold -= TotalCost;
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Deducted %d gold. Remaining: %d"), TotalCost, Character->Gold);

    // Add items to inventory
    for (int32 i = 0; i < SelectedQuantity; i++)
    {
        Character->PickingItem(ItemInfo.ItemIndex, 1);
    }

    // Update main UI
    if (Character->Main)
    {
        Character->Main->UpdateDisplayedGold();
        Character->Main->buying = false;
    }

    // Close popup
    RemoveFromParent();
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Purchase completed"));
}

void UWBP_StorePopup::OnOKButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Purchase - Item %d, Qty %d"), ItemIndex, StackNumber);
    
    // Validate item index and stack number
    if (ItemIndex < 0 || StackNumber <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("StorePopup: Invalid params - Index: %d, Qty: %d"), ItemIndex, StackNumber);
        RemoveFromParent();
        return;
    }
    
    // Get player character
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("StorePopup: No player controller"));
        RemoveFromParent();
        return;
    }

    AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PC->GetCharacter());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("StorePopup: No player character"));
        RemoveFromParent();
        return;
    }

    UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Player has %d gold"), Character->YourGold);

    // Get item information using the corrected StoreSystemFix instead of UniversalDataTableReader
    FStructure_ItemInfo ItemInfo;
    bool bFoundItemData = UStoreSystemFix::GetItemData(ItemIndex, ItemInfo);
    
    if (!bFoundItemData || !ItemInfo.bIsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("StorePopup: No item data, using fallback pricing"));
        ItemInfo.ItemIndex = ItemIndex;
        ItemInfo.Price = 100 * FMath::Max(1, ItemIndex); // Fallback pricing
        ItemInfo.ItemName = FString::Printf(TEXT("Item %d"), ItemIndex);
        ItemInfo.bIsValid = true;
        ItemInfo.bIsStackable = true;
        ItemInfo.StackNumber = 1;
    }
    
    // Calculate total price using REAL data table price
    int32 TotalPrice = ItemInfo.Price * StackNumber;
    UE_LOG(LogTemp, Log, TEXT("StorePopup: %s - Unit: %d, Qty: %d, Total: %d"), *ItemInfo.ItemName, ItemInfo.Price, StackNumber, TotalPrice);
    
    // Check if character has enough gold
    if (Character->YourGold < TotalPrice)
    {
        UE_LOG(LogTemp, Warning, TEXT("StorePopup: Insufficient gold - Need: %d, Have: %d"), TotalPrice, Character->YourGold);
        RemoveFromParent();
        return;
    }
    
    // Set main widget buying state
    if (UWBP_Main* MainWidget = Character->Main)
    {
        MainWidget->buying = true;
    }
    
    // Attempt to add items to inventory
    int32 SuccessfullyAdded = 0;
    bool bInventoryFull = false;
    
    for (int32 i = 0; i < StackNumber; ++i)
    {
        bool bAdded = Character->PickingItem(ItemIndex, 1);
        if (bAdded)
        {
            SuccessfullyAdded++;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("StorePopup: Inventory full at item %d/%d"), i + 1, StackNumber);
            bInventoryFull = true;
            break;
        }
    }
    
    if (SuccessfullyAdded == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("StorePopup: No items added to inventory"));
        RemoveFromParent();
        return;
    }
    
    // Deduct gold for successfully added items using REAL price
    int32 ActualCost = ItemInfo.Price * SuccessfullyAdded;
    Character->YourGold -= ActualCost;
    Character->Gold = Character->YourGold; // Keep Gold synchronized
    
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Purchase complete! Added %d items, spent %d gold. Remaining: %d"), SuccessfullyAdded, ActualCost, Character->YourGold);
    
    // Update displayed gold in UI
    if (UWBP_Main* MainWidget = Character->Main)
    {
        MainWidget->UpdateDisplayedGold();
        MainWidget->buying = false; // Reset buying state
    }
    
    // Force update inventory display
    Character->UpdateInventorySlots();
    
    // Show feedback for partial purchases
    if (bInventoryFull && SuccessfullyAdded < StackNumber)
    {
        UE_LOG(LogTemp, Warning, TEXT("StorePopup: Partial purchase - %d/%d items added"), SuccessfullyAdded, StackNumber);
    }
    
    // Close popup
    RemoveFromParent();
}

void UWBP_StorePopup::OnCancelButtonClicked()
{
    UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Cancel clicked"));
    RemoveFromParent();
}

void UWBP_StorePopup::OnUpButtonClicked()
{
    UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Up clicked - Current: %d"), SelectedQuantity);
    
    int32 MaxQuantity = ItemInfo.bIsStackable ? 99 : 1;
    
    if (SelectedQuantity < MaxQuantity)
    {
        SelectedQuantity++;
        StackNumber = SelectedQuantity; // Keep legacy variable in sync
        
        UE_LOG(LogTemp, Log, TEXT("StorePopup: Increased quantity to %d"), SelectedQuantity);
        
        // Update displays
        UpdateQuantityDisplay();
        UpdatePriceDisplay();
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Already at max quantity %d"), MaxQuantity);
    }
}

void UWBP_StorePopup::OnDownButtonClicked()
{
    UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Down clicked - Current: %d"), SelectedQuantity);
    
    if (SelectedQuantity > 1)
    {
        SelectedQuantity--;
        StackNumber = SelectedQuantity; // Keep legacy variable in sync
        
        UE_LOG(LogTemp, Log, TEXT("StorePopup: Decreased quantity to %d"), SelectedQuantity);
        
        // Update displays
        UpdateQuantityDisplay();
        UpdatePriceDisplay();
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Already at minimum quantity 1"));
    }
}

FText UWBP_StorePopup::GetText_StackNumber() const
{
    return FText::AsNumber(SelectedQuantity);
}

FText UWBP_StorePopup::GetText_TotalPrice()
{
    int32 TotalPrice = ItemInfo.Price * SelectedQuantity;
    return FText::AsNumber(TotalPrice);
}

void UWBP_StorePopup::UpdateItemDetails(int32 InItemIndex)
{
    ItemIndex = InItemIndex;
    StackNumber = 1; // Reset stack number for new item
    
    UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Updating details for ItemIndex=%d"), ItemIndex);
    
    UpdateItemDisplay();
}

void UWBP_StorePopup::UpdateTextSafely(UTextBlock* TextBlock, const FText& NewText)
{
    if (TextBlock)
    {
        TextBlock->SetText(NewText);
    }
}

int32 UWBP_StorePopup::GetTotalPrice()
{
    return ItemInfo.Price * SelectedQuantity;
}

void UWBP_StorePopup::UpdateQuantityWidgetVisibility()
{
    // Determine if quantity controls should be visible
    // Only show for consumable items (HP/MP potions), hide for equipment and collectibles
    bool bShowQuantityControls = (ItemInfo.ItemType == EItemType::Consume_HP || ItemInfo.ItemType == EItemType::Consume_MP);
    
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Setting quantity widget visibility to %s for item type %d"), 
           bShowQuantityControls ? TEXT("Visible") : TEXT("Hidden"), (int32)ItemInfo.ItemType);
    
    // Hide/show quantity text widgets (but NOT the purchase button)
    if (Quantity)
    {
        Quantity->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    if (QuantityText)
    {
        QuantityText->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    if (TextBlock_StackCounter)
    {
        TextBlock_StackCounter->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    if (StackCounterText)
    {
        StackCounterText->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    // Hide/show quantity control buttons (up/down buttons)
    if (Button_Up)
    {
        Button_Up->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    if (Button_Down)
    {
        Button_Down->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    if (UpButton)
    {
        UpButton->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    if (DownButton)
    {
        DownButton->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    if (IncreaseQuantityButton)
    {
        IncreaseQuantityButton->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    if (DecreaseQuantityButton)
    {
        DecreaseQuantityButton->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    
    // KEEP PURCHASE BUTTONS VISIBLE FOR ALL ITEMS
    // Don't hide OKButton, ConfirmButton, or ConfirmPurchaseButton
    
    // Only hide the specific overlay that contains the stack counter (the small grey box)
    if (Overlay_StackCounter)
    {
        Overlay_StackCounter->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Log, TEXT("StorePopup: Set Overlay_StackCounter visibility to %s"), bShowQuantityControls ? TEXT("Visible") : TEXT("Hidden"));
    }
    
    // Only hide the specific container that holds quantity controls, not main containers
    if (StackCounterContainer)
    {
        StackCounterContainer->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Log, TEXT("StorePopup: Set StackCounterContainer visibility to %s"), bShowQuantityControls ? TEXT("Visible") : TEXT("Hidden"));
    }
    
    // Hide the StackCounterBackground widget (the grey box)
    if (UWidget* StackCounterBackground = GetWidgetFromName(TEXT("StackCounterBackground")))
    {
        StackCounterBackground->SetVisibility(bShowQuantityControls ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Warning, TEXT("StorePopup: Set StackCounterBackground visibility to %s"), bShowQuantityControls ? TEXT("Visible") : TEXT("Hidden"));
    }
    
    // Adjust item image position for equipment and collectible items
    // When quantity controls are hidden, move the image down to fill the empty space
    AdjustImagePositionForLayout(bShowQuantityControls);
    
    // For non-consumable items, force quantity to 1
    if (!bShowQuantityControls)
    {
        SelectedQuantity = 1;
        StackNumber = 1;
        UpdateQuantityDisplay();
        UpdatePriceDisplay();
    }
    
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Quantity widget visibility updated for %s (Purchase buttons remain visible)"), *ItemInfo.ItemName);
}

void UWBP_StorePopup::AdjustImagePositionForLayout(bool bShowQuantityControls)
{
    // Define the offset amount to move the image down when quantity controls are hidden
    float VerticalOffset = bShowQuantityControls ? 0.0f : 30.0f; // Move down by 30 pixels for equipment items
    
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Adjusting image position - Offset: %.1f (Quantity controls: %s)"), 
           VerticalOffset, bShowQuantityControls ? TEXT("Visible") : TEXT("Hidden"));
    
    // Apply the offset to all possible item image widgets
    TArray<UImage*> ImageWidgets = {
        ItemThumbnail,
        ItemThumbnailImage,
        Image_ItemThumbnail
    };
    
    // Also try to find additional image widgets by name
    TArray<FString> PossibleImageWidgetNames = {
        TEXT("ItemImage"),
        TEXT("Image_Item"),
        TEXT("Thumbnail"),
        TEXT("Image_Thumbnail"),
        TEXT("ItemIcon"),
        TEXT("Image_Icon"),
        TEXT("ProductImage"),
        TEXT("Image_Product")
    };
    
    for (const FString& WidgetName : PossibleImageWidgetNames)
    {
        if (UImage* ImageWidget = Cast<UImage>(GetWidgetFromName(*WidgetName)))
        {
            ImageWidgets.Add(ImageWidget);
        }
    }
    
    // Apply the vertical offset to all found image widgets
    int32 AdjustedImages = 0;
    for (UImage* ImageWidget : ImageWidgets)
    {
        if (ImageWidget)
        {
            // Create a render transform with the vertical offset
            FWidgetTransform Transform = ImageWidget->GetRenderTransform();
            Transform.Translation.Y = VerticalOffset;
            ImageWidget->SetRenderTransform(Transform);
            
            AdjustedImages++;
            UE_LOG(LogTemp, Verbose, TEXT("StorePopup: Applied vertical offset %.1f to image widget"), VerticalOffset);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Adjusted position for %d image widgets with offset %.1f"), AdjustedImages, VerticalOffset);
}
