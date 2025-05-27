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

    // Update the confirmed ItemThumbnail widget using Universal Item Loader
    if (ItemThumbnail)
    {
        UTexture2D* LoadedTexture = UUniversalItemLoader::LoadItemTexture(ItemInfo);
        if (LoadedTexture)
        {
            ItemThumbnail->SetBrushFromTexture(LoadedTexture);
        }
    }

    // Update legacy thumbnail images using Universal Item Loader
    if (ItemThumbnailImage)
    {
        UTexture2D* LoadedTexture = UUniversalItemLoader::LoadItemTexture(ItemInfo);
        if (LoadedTexture)
        {
            ItemThumbnailImage->SetBrushFromTexture(LoadedTexture);
        }
    }

    if (Image_ItemThumbnail)
    {
        UTexture2D* LoadedTexture = UUniversalItemLoader::LoadItemTexture(ItemInfo);
        if (LoadedTexture)
        {
            Image_ItemThumbnail->SetBrushFromTexture(LoadedTexture);
        }
    }

    // Update price displays
    UpdatePriceDisplay();
}

void UWBP_StorePopup::UpdateQuantityDisplay()
{
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Updating quantity display to %d"), SelectedQuantity);
    
    // Ensure SelectedQuantity is valid
    if (SelectedQuantity <= 0)
    {
        SelectedQuantity = 1;
        UE_LOG(LogTemp, Warning, TEXT("StorePopup: Fixed invalid SelectedQuantity, set to 1"));
    }
    
    FString QuantityString = FString::Printf(TEXT("%d"), SelectedQuantity);
    
    // Update the confirmed quantity widget first
    if (Quantity)
    {
        Quantity->SetText(FText::FromString(QuantityString));
        UE_LOG(LogTemp, Log, TEXT("StorePopup: Set Quantity widget to %s"), *QuantityString);
    }
    
    // Update the new system quantity text
    if (QuantityText)
    {
        QuantityText->SetText(FText::FromString(QuantityString));
        UE_LOG(LogTemp, Log, TEXT("StorePopup: Set QuantityText to %s"), *QuantityString);
    }
    
    // Update legacy stack counter text
    if (TextBlock_StackCounter)
    {
        TextBlock_StackCounter->SetText(FText::FromString(QuantityString));
        UE_LOG(LogTemp, Log, TEXT("StorePopup: Set TextBlock_StackCounter to %s"), *QuantityString);
    }
    
    // Update alternative stack counter text
    if (StackCounterText)
    {
        StackCounterText->SetText(FText::FromString(QuantityString));
        UE_LOG(LogTemp, Log, TEXT("StorePopup: Set StackCounterText to %s"), *QuantityString);
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
            UE_LOG(LogTemp, Log, TEXT("StorePopup: Set %s to %s"), *WidgetName, *QuantityString);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("StorePopup: Updated %d quantity widgets"), WidgetsUpdated);
    
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
