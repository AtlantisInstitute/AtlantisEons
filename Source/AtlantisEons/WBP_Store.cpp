#include "WBP_Store.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "AtlantisEonsCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "UniversalDataTableReader.h"
#include "DataTableDiagnostic.h"
#include "WBP_StoreItemElement.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "StoreSystemFix.h"
#include "StoreDataTableInspector.h"
#include "TextureDiagnostic.h"
#include "UniversalItemLoader.h"

void UWBP_Store::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Log, TEXT("Store: Initializing"));

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
    if (!StoreItemContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("Store: StoreItemContainer is null"));
        return;
    }

    // Clear existing elements
    StoreItemContainer->ClearChildren();
    StoreElements.Empty();

    // Get all store items using the universal loader (which uses the established JSON system)
    TArray<FStructure_ItemInfo> StoreItems = UUniversalItemLoader::GetAllItems();

    // Apply equipment slot corrections to match character system
    for (FStructure_ItemInfo& ItemInfo : StoreItems)
    {
        ApplyEquipmentSlotCorrections(ItemInfo);
    }

    // Create store elements for each item
    for (const FStructure_ItemInfo& ItemInfo : StoreItems)
    {
        if (ItemInfo.bIsValid && ItemInfo.ItemIndex > 0)
        {
            AddingStoreElement(ItemInfo);
        }
    }
}

void UWBP_Store::ApplyEquipmentSlotCorrections(FStructure_ItemInfo& ItemInfo)
{
    // Apply equipment slot corrections to match character system
    if (ItemInfo.ItemName.Contains(TEXT("Sword")) || ItemInfo.ItemName.Contains(TEXT("Axe")) || 
        ItemInfo.ItemName.Contains(TEXT("Pistol")) || ItemInfo.ItemName.Contains(TEXT("Rifle")) || 
        ItemInfo.ItemName.Contains(TEXT("Spike")) || ItemInfo.ItemName.Contains(TEXT("Laser")))
    {
        if (ItemInfo.ItemEquipSlot != EItemEquipSlot::Weapon)
        {
            UE_LOG(LogTemp, Warning, TEXT("Store: Corrected '%s' (Index: %d) from slot %d to slot %d"), 
                   *ItemInfo.ItemName, ItemInfo.ItemIndex, (int32)ItemInfo.ItemEquipSlot, (int32)EItemEquipSlot::Weapon);
            ItemInfo.ItemEquipSlot = EItemEquipSlot::Weapon;
        }
    }
    else if (ItemInfo.ItemName.Contains(TEXT("Shield")))
    {
        if (ItemInfo.ItemEquipSlot != EItemEquipSlot::Accessory)
        {
            UE_LOG(LogTemp, Warning, TEXT("Store: Corrected '%s' (Index: %d) from slot %d to slot %d"), 
                   *ItemInfo.ItemName, ItemInfo.ItemIndex, (int32)ItemInfo.ItemEquipSlot, (int32)EItemEquipSlot::Accessory);
            ItemInfo.ItemEquipSlot = EItemEquipSlot::Accessory;
        }
    }
    else if (ItemInfo.ItemName.Contains(TEXT("Helmet")) || ItemInfo.ItemName.Contains(TEXT("Hat")) || 
             ItemInfo.ItemName.Contains(TEXT("Mask")) || ItemInfo.ItemName.Contains(TEXT("SWAT")))
    {
        if (ItemInfo.ItemEquipSlot != EItemEquipSlot::Head)
        {
            UE_LOG(LogTemp, Warning, TEXT("Store: Corrected '%s' (Index: %d) from slot %d to slot %d"), 
                   *ItemInfo.ItemName, ItemInfo.ItemIndex, (int32)ItemInfo.ItemEquipSlot, (int32)EItemEquipSlot::Head);
            ItemInfo.ItemEquipSlot = EItemEquipSlot::Head;
        }
    }
    else if (ItemInfo.ItemName.Contains(TEXT("Suit")))
    {
        if (ItemInfo.ItemEquipSlot != EItemEquipSlot::Body)
        {
            UE_LOG(LogTemp, Warning, TEXT("Store: Corrected '%s' (Index: %d) from slot %d to slot %d"), 
                   *ItemInfo.ItemName, ItemInfo.ItemIndex, (int32)ItemInfo.ItemEquipSlot, (int32)EItemEquipSlot::Body);
            ItemInfo.ItemEquipSlot = EItemEquipSlot::Body;
        }
    }
    
    // Fix consumable item data - move HP/MP values to RecoveryHP/RecoveryMP for potions
    if (ItemInfo.ItemType == EItemType::Consume_HP || ItemInfo.ItemType == EItemType::Consume_MP)
    {
        // If RecoveryHP/RecoveryMP are 0 but HP/MP have values, move them
        if (ItemInfo.RecoveryHP == 0 && ItemInfo.HP > 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Store: Fixed consumable '%s' - moved HP %d to RecoveryHP"), 
                   *ItemInfo.ItemName, ItemInfo.HP);
            ItemInfo.RecoveryHP = ItemInfo.HP;
            ItemInfo.HP = 0; // Clear the base HP since it's not used for consumables
        }
        
        if (ItemInfo.RecoveryMP == 0 && ItemInfo.MP > 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Store: Fixed consumable '%s' - moved MP %d to RecoveryMP"), 
                   *ItemInfo.ItemName, ItemInfo.MP);
            ItemInfo.RecoveryMP = ItemInfo.MP;
            ItemInfo.MP = 0; // Clear the base MP since it's not used for consumables
        }
    }
}

void UWBP_Store::AddingStoreElement(const FStructure_ItemInfo& ItemInfo)
{
    if (!StoreItemContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("Store: StoreItemContainer is null"));
        return;
    }

    // Create the store item element widget
    UWBP_StoreItemElement* StoreElement = CreateWidget<UWBP_StoreItemElement>(this, StoreItemElementClass);
    if (!StoreElement)
    {
        UE_LOG(LogTemp, Error, TEXT("Store: Failed to create store element widget"));
        return;
    }

    // Set up the store element with item data
    StoreElement->SetItemInfo(ItemInfo);
    StoreElement->SetParentStore(this);

    // Add to container
    StoreItemContainer->AddChild(StoreElement);
    StoreElements.Add(StoreElement);
}

bool UWBP_Store::AddingStoreElement(int32 ItemIndex, UWBP_StoreItemElement* WBPStoreItemElementRef)
{
    // This method is deprecated - use the new AddingStoreElement(const FStructure_ItemInfo&) instead
    UE_LOG(LogTemp, Warning, TEXT("WBP_Store: Using deprecated AddingStoreElement method"));
    return false;
}

FStructure_ItemInfo UWBP_Store::CreateFallbackItemData(int32 ItemIndex)
{
    FStructure_ItemInfo ItemInfo;
    
    // Basic properties
    ItemInfo.ItemIndex = ItemIndex;
    ItemInfo.ItemName = FString::Printf(TEXT("Item %d"), ItemIndex);
    ItemInfo.ItemDescription = FString::Printf(TEXT("Description for item %d"), ItemIndex);
    ItemInfo.bIsValid = true;
    ItemInfo.Price = 100 * FMath::Max(1, ItemIndex);
    
    // Set item type and equipment slot based on index ranges (example logic)
    if (ItemIndex >= 1 && ItemIndex <= 10)
    {
        // Consumables
        ItemInfo.ItemType = (ItemIndex % 2 == 1) ? EItemType::Consume_HP : EItemType::Consume_MP;
        ItemInfo.ItemEquipSlot = EItemEquipSlot::None;
        ItemInfo.bIsStackable = true;
        ItemInfo.StackNumber = 99;
        ItemInfo.RecoveryHP = (ItemInfo.ItemType == EItemType::Consume_HP) ? 50 : 0;
        ItemInfo.RecoveryMP = (ItemInfo.ItemType == EItemType::Consume_MP) ? 30 : 0;
    }
    else if (ItemIndex >= 11 && ItemIndex <= 20)
    {
        // Weapons
        ItemInfo.ItemType = EItemType::Equip;
        ItemInfo.ItemEquipSlot = EItemEquipSlot::Weapon;
        ItemInfo.bIsStackable = false;
        ItemInfo.StackNumber = 1;
        ItemInfo.Damage = 10 + (ItemIndex - 10) * 5;
        ItemInfo.STR = 2 + (ItemIndex - 10);
    }
    else if (ItemIndex >= 21 && ItemIndex <= 30)
    {
        // Helmets
        ItemInfo.ItemType = EItemType::Equip;
        ItemInfo.ItemEquipSlot = EItemEquipSlot::Head;
        ItemInfo.bIsStackable = false;
        ItemInfo.StackNumber = 1;
        ItemInfo.Defence = 5 + (ItemIndex - 20) * 3;
        ItemInfo.HP = 10 + (ItemIndex - 20) * 5;
    }
    else if (ItemIndex >= 31 && ItemIndex <= 40)
    {
        // Body armor
        ItemInfo.ItemType = EItemType::Equip;
        ItemInfo.ItemEquipSlot = EItemEquipSlot::Body;
        ItemInfo.bIsStackable = false;
        ItemInfo.StackNumber = 1;
        ItemInfo.Defence = 8 + (ItemIndex - 30) * 4;
        ItemInfo.HP = 15 + (ItemIndex - 30) * 7;
    }
    else
    {
        // Accessories/Other
        ItemInfo.ItemType = EItemType::Equip;
        ItemInfo.ItemEquipSlot = EItemEquipSlot::Accessory;
        ItemInfo.bIsStackable = false;
        ItemInfo.StackNumber = 1;
        ItemInfo.INT = 3 + ItemIndex;
        ItemInfo.MP = 20 + ItemIndex * 2;
    }

    // Try to load thumbnail
    TArray<FString> PossibleThumbnailPaths = {
        FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_Item_%d"), ItemIndex),
        FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_%d"), ItemIndex),
        TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicHealingPotion"),
        TEXT("/Engine/EditorResources/S_Actor")
    };

    for (const FString& Path : PossibleThumbnailPaths)
    {
        UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *Path));
        if (Texture)
        {
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(Texture);
            UE_LOG(LogTemp, Log, TEXT("WBP_Store: Loaded thumbnail for item %d from %s"), ItemIndex, *Path);
            break;
        }
    }

    return ItemInfo;
}

void UWBP_Store::PopulateStoreElementUI(UWBP_StoreItemElement* Element, const FStructure_ItemInfo& ItemInfo)
{
    if (!Element)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Store::PopulateStoreElementUI: Element is null"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("WBP_Store::PopulateStoreElementUI: Populating UI for item %s"), *ItemInfo.ItemName);

    // Set item thumbnail
    if (UImage* ItemImage = Element->GetItemImage())
    {
        if (!ItemInfo.ItemThumbnail.IsNull())
        {
            UTexture2D* Texture = ItemInfo.ItemThumbnail.LoadSynchronous();
            if (Texture)
            {
                ItemImage->SetBrushFromTexture(Texture);
                UE_LOG(LogTemp, Log, TEXT("WBP_Store: Set thumbnail for item %s"), *ItemInfo.ItemName);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("WBP_Store: Failed to load thumbnail for item %s"), *ItemInfo.ItemName);
            }
        }
    }

    // Set title
    if (UTextBlock* TitleText = Element->GetTitleText())
    {
        TitleText->SetText(FText::FromString(ItemInfo.ItemName));
        UE_LOG(LogTemp, Log, TEXT("WBP_Store: Set title for item %s"), *ItemInfo.ItemName);
    }

    // Set description
    if (UTextBlock* DescText = Element->GetDescriptionText())
    {
        DescText->SetText(FText::FromString(ItemInfo.ItemDescription));
        UE_LOG(LogTemp, Log, TEXT("WBP_Store: Set description for item %s"), *ItemInfo.ItemName);
    }

    // Determine item category for UI visibility
    bool bIsConsumable = ItemInfo.ItemType == EItemType::Consume_HP || ItemInfo.ItemType == EItemType::Consume_MP;
    bool bIsEquipment = ItemInfo.ItemType == EItemType::Equip;

    UE_LOG(LogTemp, Log, TEXT("WBP_Store: Item %s - IsConsumable: %s, IsEquipment: %s"), 
           *ItemInfo.ItemName, bIsConsumable ? TEXT("true") : TEXT("false"), bIsEquipment ? TEXT("true") : TEXT("false"));

    // Recovery stats (for consumables)
    if (UTextBlock* RecoveryHPText = Element->GetRecoveryHPText())
    {
        RecoveryHPText->SetVisibility(bIsConsumable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsConsumable && ItemInfo.RecoveryHP > 0)
        {
            RecoveryHPText->SetText(FText::FromString(FString::Printf(TEXT("+%d HP"), ItemInfo.RecoveryHP)));
        }
    }

    if (UTextBlock* RecoveryMPText = Element->GetRecoveryMPText())
    {
        RecoveryMPText->SetVisibility(bIsConsumable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsConsumable && ItemInfo.RecoveryMP > 0)
        {
            RecoveryMPText->SetText(FText::FromString(FString::Printf(TEXT("+%d MP"), ItemInfo.RecoveryMP)));
        }
    }

    // Equipment stats
    if (UTextBlock* DamageText = Element->GetDamageText())
    {
        DamageText->SetVisibility(bIsEquipment && ItemInfo.Damage > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment && ItemInfo.Damage > 0)
        {
            DamageText->SetText(FText::FromString(FString::Printf(TEXT("+%d ATK"), ItemInfo.Damage)));
        }
    }

    if (UTextBlock* DefenceText = Element->GetDefenceText())
    {
        DefenceText->SetVisibility(bIsEquipment && ItemInfo.Defence > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment && ItemInfo.Defence > 0)
        {
            DefenceText->SetText(FText::FromString(FString::Printf(TEXT("+%d DEF"), ItemInfo.Defence)));
        }
    }

    // Base stats for equipment
    if (UTextBlock* HPText = Element->GetHPText())
    {
        HPText->SetVisibility(bIsEquipment && ItemInfo.HP > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment && ItemInfo.HP > 0)
        {
            HPText->SetText(FText::FromString(FString::Printf(TEXT("+%d HP"), ItemInfo.HP)));
        }
    }

    if (UTextBlock* MPText = Element->GetMPText())
    {
        MPText->SetVisibility(bIsEquipment && ItemInfo.MP > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment && ItemInfo.MP > 0)
        {
            MPText->SetText(FText::FromString(FString::Printf(TEXT("+%d MP"), ItemInfo.MP)));
        }
    }

    if (UTextBlock* STRText = Element->GetSTRText())
    {
        STRText->SetVisibility(bIsEquipment && ItemInfo.STR > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment && ItemInfo.STR > 0)
        {
            STRText->SetText(FText::FromString(FString::Printf(TEXT("+%d STR"), ItemInfo.STR)));
        }
    }

    if (UTextBlock* DEXText = Element->GetDEXText())
    {
        DEXText->SetVisibility(bIsEquipment && ItemInfo.DEX > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment && ItemInfo.DEX > 0)
        {
            DEXText->SetText(FText::FromString(FString::Printf(TEXT("+%d DEX"), ItemInfo.DEX)));
        }
    }

    if (UTextBlock* INTText = Element->GetINTText())
    {
        INTText->SetVisibility(bIsEquipment && ItemInfo.INT > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bIsEquipment && ItemInfo.INT > 0)
        {
            INTText->SetText(FText::FromString(FString::Printf(TEXT("+%d INT"), ItemInfo.INT)));
        }
    }

    // Set item slot type text
    if (UTextBlock* SlotTypeText = Element->GetItemSlotTypeText())
    {
        FString SlotTypeString;
        switch (ItemInfo.ItemEquipSlot)
        {
            case EItemEquipSlot::Weapon:
                SlotTypeString = TEXT("Weapon");
                break;
            case EItemEquipSlot::Head:
                SlotTypeString = TEXT("Helmet");
                break;
            case EItemEquipSlot::Body:
                SlotTypeString = TEXT("Body Armor");
                break;
            case EItemEquipSlot::Accessory:
                SlotTypeString = TEXT("Accessory");
                break;
            case EItemEquipSlot::None:
            default:
                SlotTypeString = bIsConsumable ? TEXT("Consumable") : TEXT("Misc");
                break;
        }
        SlotTypeText->SetText(FText::FromString(SlotTypeString));
        UE_LOG(LogTemp, Log, TEXT("WBP_Store: Set slot type %s for item %s"), *SlotTypeString, *ItemInfo.ItemName);
    }

    UE_LOG(LogTemp, Log, TEXT("WBP_Store::PopulateStoreElementUI: Completed UI population for item %s"), *ItemInfo.ItemName);
}

void UWBP_Store::OnALLButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("Store: Showing all items"));
    
    // Show all store elements
    int32 VisibleCount = 0;
    for (UWBP_StoreItemElement* Element : StoreElements)
    {
        if (Element)
        {
            Element->SetVisibility(ESlateVisibility::Visible);
            VisibleCount++;
        }
    }
    
    UE_LOG(LogTemp, Verbose, TEXT("Store: Showing %d items in ALL"), VisibleCount);

    // Show bottom border for ALL button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Visible);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_Store::OnWeaponButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("Store: Filtering by weapons"));
    UpdateElementVisibility(static_cast<uint8>(EItemEquipSlot::Weapon));
    
    // Show bottom border for Weapon button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Visible);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_Store::OnHelmetButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("Store: Filtering by helmets"));
    // Helmet items are mapped to Head in our enum system
    UpdateElementVisibility(static_cast<uint8>(EItemEquipSlot::Head));
    
    // Show bottom border for Helmet button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Visible);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_Store::OnShieldButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("Store: Filtering by shields"));
    // Shield items are mapped to Accessory in our enum system
    UpdateElementVisibility(static_cast<uint8>(EItemEquipSlot::Accessory));
    
    // Show bottom border for Shield button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Visible);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_Store::OnSuitButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("Store: Filtering by suits"));
    // Suit items are mapped to Body in our enum system
    UpdateElementVisibility(static_cast<uint8>(EItemEquipSlot::Body));
    
    // Show bottom border for Suit button
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Visible);
}

void UWBP_Store::OnNoneButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("Store: Filtering by consumables"));
    UpdateElementVisibility(static_cast<uint8>(EItemEquipSlot::None));
    
    // Show border for None button (assuming this should show consumables)
    if (BottomBorder0) BottomBorder0->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder1) BottomBorder1->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder2) BottomBorder2->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder3) BottomBorder3->SetVisibility(ESlateVisibility::Collapsed);
    if (BottomBorder4) BottomBorder4->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_Store::UpdateElementVisibility(uint8 EquipSlotType)
{
    EItemEquipSlot FilterSlot = static_cast<EItemEquipSlot>(EquipSlotType);
    int32 VisibleCount = 0;
    int32 TotalCount = 0;
    
    UE_LOG(LogTemp, Verbose, TEXT("Store: Filtering by slot %d"), EquipSlotType);
    
    // Update visibility for each store element based on equip slot type
    for (UWBP_StoreItemElement* Element : StoreElements)
    {
        if (Element)
        {
            TotalCount++;
            FStructure_ItemInfo ElementItemInfo = Element->GetItemInfo();
            EItemEquipSlot ElementSlot = ElementItemInfo.ItemEquipSlot;
            bool bShouldShow = (ElementSlot == FilterSlot);
            
            Element->SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            
            if (bShouldShow)
            {
                VisibleCount++;
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Store: Filtered %d of %d items for slot %d"), VisibleCount, TotalCount, EquipSlotType);
}

FText UWBP_Store::GetText_ALL() const
{
    int32 TotalItems = StoreElements.Num();
    return FText::AsNumber(TotalItems);
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

    TotalNumber = FText::AsNumber(Count);
    return FText::AsNumber(Count);
}

FText UWBP_Store::GetText_NONE()
{
    int32 Count = 0;
    for (const auto* Element : StoreElements)
    {
        if (Element && Element->GetItemInfo().ItemEquipSlot == EItemEquipSlot::None)
        {
            Count++;
        }
    }
    return FText::AsNumber(Count);
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
    FText TotalNumber;
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

void UWBP_Store::OpenPurchasePopup(const FStructure_ItemInfo& ItemInfo)
{
    UE_LOG(LogTemp, Log, TEXT("Store: Opening popup for %d: %s"), ItemInfo.ItemIndex, *ItemInfo.ItemName);

    // Try to load the store popup class
    UClass* StorePopupClass = LoadClass<UWBP_StorePopup>(nullptr, 
        TEXT("/Game/AtlantisEons/Blueprints/Store/WBP_StorePopup.WBP_StorePopup_C"));
    
    if (!StorePopupClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Store: Failed to load WBP_StorePopup class"));
        return;
    }

    // Create the popup widget
    UWBP_StorePopup* StorePopup = CreateWidget<UWBP_StorePopup>(GetWorld(), StorePopupClass);
    if (!StorePopup)
    {
        UE_LOG(LogTemp, Error, TEXT("Store: Failed to create store popup widget"));
        return;
    }

    // Set up the popup with item data - ALL necessary fields
    StorePopup->ItemInfo = ItemInfo;
    StorePopup->ItemIndex = ItemInfo.ItemIndex;  // Ensure this is set correctly
    StorePopup->StackNumber = 1;                 // Start with 1 item
    StorePopup->SelectedQuantity = 1;            // Also set the new quantity field

    // Update the popup display to ensure all UI elements are properly populated
    StorePopup->UpdateItemDisplay();

    // Add to viewport
    StorePopup->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
    StorePopup->AddToViewport(1000);

    // Set buying state
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (PlayerCharacter)
    {
        AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PlayerCharacter);
        if (Character && Character->Main)
        {
            Character->Main->buying = true;
            Character->Main->UpdateDisplayedGold();
        }
    }

    UE_LOG(LogTemp, Verbose, TEXT("Store: Popup opened for item %d"), ItemInfo.ItemIndex);
}
