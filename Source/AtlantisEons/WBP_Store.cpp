#include "WBP_Store.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "AtlantisEonsCharacter.h"
#include "Kismet/GameplayStatics.h"

void UWBP_Store::NativeConstruct()
{
    Super::NativeConstruct();

    // Clear any existing bindings before adding new ones to prevent duplicate delegate errors
    if (ALLButton)
    {
        ALLButton->OnClicked.Clear();
        ALLButton->OnClicked.AddDynamic(this, &UWBP_Store::OnALLButtonClicked);
    }

    if (WeaponButton)
    {
        WeaponButton->OnClicked.Clear();
        WeaponButton->OnClicked.AddDynamic(this, &UWBP_Store::OnWeaponButtonClicked);
    }

    if (HelmetButton)
    {
        HelmetButton->OnClicked.Clear();
        HelmetButton->OnClicked.AddDynamic(this, &UWBP_Store::OnHelmetButtonClicked);
    }

    if (ShieldButton)
    {
        ShieldButton->OnClicked.Clear();
        ShieldButton->OnClicked.AddDynamic(this, &UWBP_Store::OnShieldButtonClicked);
    }

    if (SuitButton)
    {
        SuitButton->OnClicked.Clear();
        SuitButton->OnClicked.AddDynamic(this, &UWBP_Store::OnSuitButtonClicked);
    }

    if (NoneButton)
    {
        NoneButton->OnClicked.Clear();
        NoneButton->OnClicked.AddDynamic(this, &UWBP_Store::OnNoneButtonClicked);
    }

    // Initialize store elements
    InitializeStoreElements();
}

void UWBP_Store::InitializeStoreElements()
{
    // Clear existing elements
    StoreElements.Empty();
    
    // Load the data table
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Item Data Table"));
        return;
    }

    // Get all row names
    TArray<FName> RowNames = ItemDataTable->GetRowNames();

    // Create store elements dynamically based on data table entries
    for (int32 i = 0; i < RowNames.Num(); ++i)
    {
        // Create widget instance
        UWBP_StoreItemElement* NewElement = CreateWidget<UWBP_StoreItemElement>(this, StoreItemElementClass);
        if (NewElement)
        {
            AddingStoreElement(i, NewElement);
            if (StoreItemContainer)
            {
                StoreItemContainer->AddChild(NewElement);
            }
        }
    }
    if (WBP_StoreItemElement_C_3) AddingStoreElement(3, WBP_StoreItemElement_C_3);
    if (WBP_StoreItemElement_C_4) AddingStoreElement(4, WBP_StoreItemElement_C_4);

    // Set initial visibility for all elements
    OnALLButtonClicked();
}

void UWBP_Store::AddingStoreElement(int32 Index, UWBP_StoreItemElement* WBPStoreItemElementRef)
{
    if (!WBPStoreItemElementRef)
    {
        return;
    }

    // Get data from item table with better error handling
    FString RowName = FString::Printf(TEXT("Item_%d"), Index);
    FStructure_ItemInfo ItemInfo;

    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList.Table_ItemList")));
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Item Data Table"));
        return;
    }

    // Use FindRowUnchecked to avoid structure type mismatch errors
    void* RowData = ItemDataTable->FindRowUnchecked(*RowName);
    if (!RowData)
    {
        // Try with just the index number as fallback
        RowName = FString::FromInt(Index);
        RowData = ItemDataTable->FindRowUnchecked(*RowName);
        
        if (!RowData)
        {
            // Use hardcoded fallback data for common items
            ItemInfo = FStructure_ItemInfo();
            ItemInfo.ItemIndex = Index;
            ItemInfo.ItemName = FString::Printf(TEXT("Store Item %d"), Index);
            ItemInfo.ItemDescription = FString::Printf(TEXT("Description for item %d"), Index);
            ItemInfo.Price = 100 * (Index + 1);
            ItemInfo.bIsValid = true;
            ItemInfo.bIsStackable = true;
            ItemInfo.StackNumber = 1;
        }
        else
        {
            // We found data but can't safely cast it, create basic fallback
            ItemInfo = FStructure_ItemInfo();
            ItemInfo.ItemIndex = Index;
            ItemInfo.ItemName = FString::Printf(TEXT("Item %d"), Index);
            ItemInfo.bIsValid = true;
        }
    }
    else
    {
        // We found data but can't safely cast it, create basic fallback
        ItemInfo = FStructure_ItemInfo();
        ItemInfo.ItemIndex = Index;
        ItemInfo.ItemName = FString::Printf(TEXT("Item %d"), Index);
        ItemInfo.bIsValid = true;
    }

    WBPStoreItemElementRef->ItemInfo = ItemInfo;
    WBPStoreItemElementRef->ItemIndex = Index;

    // Add to store elements array
    StoreElements.Add(WBPStoreItemElementRef);

    // Set item thumbnail
    if (UImage* ItemImage = WBPStoreItemElementRef->GetItemImage())
    {
        UTexture2D* Texture = nullptr;
        if (!ItemInfo.ItemThumbnail.IsNull())
        {
            Texture = ItemInfo.ItemThumbnail.LoadSynchronous();
        }
        if (Texture)
        {
            ItemImage->SetBrushFromTexture(Texture);
        }
    }

    // Set title
    if (UTextBlock* TitleText = WBPStoreItemElementRef->GetTitleText())
    {
        TitleText->SetText(FText::FromString(ItemInfo.ItemName));
    }

    // Set description
    if (UTextBlock* DescText = WBPStoreItemElementRef->GetDescriptionText())
    {
        DescText->SetText(FText::FromString(ItemInfo.ItemDescription));
    }

    // Set stats based on item type
    bool bIsConsumable = ItemInfo.ItemType == EItemType::Consume_HP || ItemInfo.ItemType == EItemType::Consume_MP;
    bool bIsEquipment = ItemInfo.ItemType == EItemType::Equip;

    // Recovery stats (for consumables)
    if (UTextBlock* RecoveryHPText = WBPStoreItemElementRef->GetRecoveryHPText())
    {
        RecoveryHPText->SetVisibility(bIsConsumable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsConsumable)
        {
            RecoveryHPText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), ItemInfo.RecoveryHP)));
        }
    }

    if (UTextBlock* RecoveryMPText = WBPStoreItemElementRef->GetRecoveryMPText())
    {
        RecoveryMPText->SetVisibility(bIsConsumable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsConsumable)
        {
            RecoveryMPText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), ItemInfo.RecoveryMP)));
        }
    }

    // Equipment stats
    if (UTextBlock* DamageText = WBPStoreItemElementRef->GetDamageText())
    {
        DamageText->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment)
        {
            DamageText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), ItemInfo.Damage)));
        }
    }

    if (UTextBlock* DefenceText = WBPStoreItemElementRef->GetDefenceText())
    {
        DefenceText->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment)
        {
            DefenceText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), ItemInfo.Defence)));
        }
    }

    // Base stats
    if (UTextBlock* HPText = WBPStoreItemElementRef->GetHPText())
    {
        HPText->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment)
        {
            HPText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), ItemInfo.HP)));
        }
    }

    if (UTextBlock* MPText = WBPStoreItemElementRef->GetMPText())
    {
        MPText->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment)
        {
            MPText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), ItemInfo.MP)));
        }
    }

    if (UTextBlock* STRText = WBPStoreItemElementRef->GetSTRText())
    {
        STRText->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment)
        {
            STRText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), ItemInfo.STR)));
        }
    }

    if (UTextBlock* DEXText = WBPStoreItemElementRef->GetDEXText())
    {
        DEXText->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment)
        {
            DEXText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), ItemInfo.DEX)));
        }
    }

    if (UTextBlock* INTText = WBPStoreItemElementRef->GetINTText())
    {
        INTText->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment)
        {
            INTText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), ItemInfo.INT)));
        }
    }

    // Set item slot type
    if (UTextBlock* SlotTypeText = WBPStoreItemElementRef->GetItemSlotTypeText())
    {
        FString SlotTypeString;
        switch (ItemInfo.ItemEquipSlot)
        {
            case EItemEquipSlot::Weapon:
                SlotTypeString = TEXT("Weapon");
                break;
            case EItemEquipSlot::Head:
                SlotTypeString = TEXT("Head");
                break;
            case EItemEquipSlot::Body:
                SlotTypeString = TEXT("Body");
                break;
            case EItemEquipSlot::Accessory:
                SlotTypeString = TEXT("Accessory");
                break;
            default:
                SlotTypeString = TEXT("Consumable");
                break;
        }
        SlotTypeText->SetText(FText::FromString(SlotTypeString + TEXT(" Type")));
    }
}

void UWBP_Store::OnALLButtonClicked()
{
    // Show all store elements
    for (UWBP_StoreItemElement* Element : StoreElements)
    {
        if (Element)
        {
            Element->SetVisibility(ESlateVisibility::Visible);
        }
    }

    // Show bottom border for ALL button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Visible);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_Store::OnWeaponButtonClicked()
{
    UpdateElementVisibility(1); // Assuming 1 is the enum value for Weapon
    
    // Show bottom border for Weapon button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Visible);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_Store::OnHelmetButtonClicked()
{
    UpdateElementVisibility(9); // Assuming 9 is the enum value for Helmet
    
    // Show bottom border for Helmet button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Visible);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_Store::OnShieldButtonClicked()
{
    UpdateElementVisibility(2); // Assuming 2 is the enum value for Shield
    
    // Show bottom border for Shield button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Visible);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_Store::OnSuitButtonClicked()
{
    UpdateElementVisibility(0); // Assuming 0 is the enum value for Suit
    
    // Show bottom border for Suit button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Visible);
}

void UWBP_Store::OnNoneButtonClicked()
{
    UpdateElementVisibility(static_cast<uint8>(EItemEquipSlot::None));

    // Hide all elements
    for (UWBP_StoreItemElement* Element : StoreElements)
    {
        if (Element)
        {
            Element->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UWBP_Store::UpdateElementVisibility(uint8 EquipSlotType)
{
    // Update visibility for each store element based on equip slot type
    for (UWBP_StoreItemElement* Element : StoreElements)
    {
        if (Element)
        {
            Element->SetVisibility(static_cast<uint8>(Element->GetItemInfo().ItemEquipSlot) == EquipSlotType ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        }
    }
}

FText UWBP_Store::GetText_ALL() const
{
    FString Text = TEXT("ALL");
    int32 TotalItems = StoreElements.Num();
    return FText::FromString(FString::Printf(TEXT("%s (%d)"), *Text, TotalItems));
}

FText UWBP_Store::GetItemNumber(EItemEquipSlot ItemEquipSlotType, FText& TotalNumber)
{
    int32 Count = 0;
    for (const auto* Element : StoreElements)
    {
        if (Element && Element->GetItemInfo().ItemEquipSlot == ItemEquipSlotType)
        {
            Count++;
        }
    }

    FString TypeName;
    switch (ItemEquipSlotType)
    {
        case EItemEquipSlot::Weapon:
            TypeName = TEXT("WEAPON");
            break;
        case EItemEquipSlot::Head:
            TypeName = TEXT("HELMET");
            break;
        case EItemEquipSlot::Body:
            TypeName = TEXT("SUIT");
            break;
        case EItemEquipSlot::Accessory:
            TypeName = TEXT("SHIELD");
            break;
        default:
            TypeName = TEXT("NONE");
            break;
    }

    TotalNumber = FText::AsNumber(Count);
    return FText::FromString(FString::Printf(TEXT("%s (%d)"), *TypeName, Count));
}

FText UWBP_Store::GetText_NONE()
{
    return FText::FromString(TEXT("NONE (0)"));
}

FText UWBP_Store::GetText_WEAPON()
{
    FText TotalNumber;
    return GetItemNumber(EItemEquipSlot::Weapon, TotalNumber);
}

FText UWBP_Store::GetText_SUIT()
{
    FText TotalNumber;
    return GetItemNumber(EItemEquipSlot::Body, TotalNumber);
}

FText UWBP_Store::GetText_SHIELD()
{
    FText TotalNumber;
    return GetItemNumber(EItemEquipSlot::Accessory, TotalNumber);
}

FText UWBP_Store::GetText_HELMET()
{
    // Create a local variable to pass as reference parameter
    FText TotalNumber = FText::FromString("Helmet");
    return GetItemNumber(EItemEquipSlot::Head, TotalNumber);
}

int32 UWBP_Store::GetSelectedItemIndex() const
{
    return SelectedItemIndex;
}

void UWBP_Store::PurchaseSelectedItem()
{
    // Get the player character
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC) return;
    
    AAtlantisEonsCharacter* Character = PC ? Cast<AAtlantisEonsCharacter>(PC->GetPawn()) : nullptr;
    if (!Character) return;
    
    // Use the character's buying method to purchase the selected item
    if (SelectedItemIndex >= 0)
    {
        // Default stack number of 1 and get price from item data
        int32 ItemStackNumber = 1;
        int32 ItemPrice = 100; // Default price if we can't get it from data table
        
        // TODO: Get actual price from data table if available
        
        bool bSuccess = Character->BuyingItem(SelectedItemIndex, ItemStackNumber, ItemPrice);
        if (bSuccess)
        {
            UE_LOG(LogTemp, Warning, TEXT("Purchase successful: Item %d"), SelectedItemIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Purchase failed: Item %d"), SelectedItemIndex);
        }
    }
}

void UWBP_Store::OnItemSelected(int32 ItemIndex)
{
    SelectedItemIndex = ItemIndex;
    UE_LOG(LogTemp, Warning, TEXT("Selected store item: %d"), SelectedItemIndex);
    
    // If BuyButton exists, enable it when an item is selected
    if (BuyButton)
    {
        BuyButton->SetIsEnabled(true);
    }
}
