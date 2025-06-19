#include "WBP_StoreItemElement.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "WBP_StorePopup.h"
#include "AtlantisEonsCharacter.h"
#include "WBP_Main.h"
#include "StoreSystemFix.h"
#include "UniversalItemLoader.h"

void UWBP_StoreItemElement::SetItemInfo(const FStructure_ItemInfo& InItemInfo)
{
    ItemInfo = InItemInfo;

    // Set up buy button binding
    if (BuyButton)
    {
        BuyButton->OnClicked.Clear();
        BuyButton->OnClicked.AddDynamic(this, &UWBP_StoreItemElement::OnBuyButtonClicked);
    }

    // Update UI elements with item data
    UpdateItemDisplay();
}

void UWBP_StoreItemElement::UpdateItemDisplay()
{
    // Update main item image/thumbnail using Universal Item Loader
    if (ItemImage)
    {
        UTexture2D* LoadedTexture = UUniversalItemLoader::LoadItemTexture(ItemInfo);
        if (LoadedTexture)
        {
            ItemImage->SetBrushFromTexture(LoadedTexture);
        }
    }

    // Also update the legacy thumbnail image if it exists using Universal Loader
    if (ItemThumbnailImage)
    {
        UTexture2D* LoadedTexture = UUniversalItemLoader::LoadItemTexture(ItemInfo);
        if (LoadedTexture)
        {
            ItemThumbnailImage->SetBrushFromTexture(LoadedTexture);
        }
    }

    // Update item title/name
    if (Title)
    {
        Title->SetText(FText::FromString(ItemInfo.ItemName));
    }

    // Update item slot type
    if (ItemSlotType)
    {
        FString SlotTypeString;
        switch (ItemInfo.ItemEquipSlot)
        {
            case EItemEquipSlot::Consumable:
                SlotTypeString = TEXT("Consumable");
                break;
            case EItemEquipSlot::Weapon:
                SlotTypeString = TEXT("Weapon");
                break;
            case EItemEquipSlot::Head:
                // Since Helmet maps to Head, display as Helmet for better UX
                SlotTypeString = TEXT("Helmet");
                break;
            case EItemEquipSlot::Body:
                // Since Suit maps to Body, display as Armor/Suit for better UX
                SlotTypeString = TEXT("Armor");
                break;
            case EItemEquipSlot::Accessory:
                // Since Shield maps to Accessory, we need to check item name to display correctly
                if (ItemInfo.ItemName.Contains(TEXT("Shield")))
                {
                    SlotTypeString = TEXT("Shield");
                }
                else
                {
                    SlotTypeString = TEXT("Accessory");
                }
                break;
            default:
                SlotTypeString = TEXT("Equipment");
                break;
        }
        ItemSlotType->SetText(FText::FromString(SlotTypeString));
    }

    // Update Recovery HP (only for consumables)
    if (RecoveryHP)
    {
        bool bIsConsumable = (ItemInfo.ItemType == EItemType::Consume_HP || ItemInfo.ItemType == EItemType::Consume_MP);
        if (bIsConsumable && ItemInfo.RecoveryHP > 0)
        {
            FString HPRecoveryText = FString::Printf(TEXT("HP %d"), ItemInfo.RecoveryHP);
            RecoveryHP->SetText(FText::FromString(HPRecoveryText));
            RecoveryHP->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            RecoveryHP->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Update Recovery MP (only for consumables)
    if (RecoveryMP)
    {
        bool bIsConsumable = (ItemInfo.ItemType == EItemType::Consume_HP || ItemInfo.ItemType == EItemType::Consume_MP);
        if (bIsConsumable && ItemInfo.RecoveryMP > 0)
        {
            FString MPRecoveryText = FString::Printf(TEXT("MP %d"), ItemInfo.RecoveryMP);
            RecoveryMP->SetText(FText::FromString(MPRecoveryText));
            RecoveryMP->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            RecoveryMP->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Update Damage (only for equipment)
    if (DamageText)
    {
        bool bIsEquipment = (ItemInfo.ItemType == EItemType::Equip);
        if (bIsEquipment && ItemInfo.Damage > 0)
        {
            FString DamageString = FString::Printf(TEXT("DAMAGE %d"), ItemInfo.Damage);
            DamageText->SetText(FText::FromString(DamageString));
            DamageText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            DamageText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Update Defence (only for equipment)
    if (DefenceText)
    {
        bool bIsEquipment = (ItemInfo.ItemType == EItemType::Equip);
        if (bIsEquipment && ItemInfo.Defence > 0)
        {
            FString DefenceString = FString::Printf(TEXT("DEFENCE %d"), ItemInfo.Defence);
            DefenceText->SetText(FText::FromString(DefenceString));
            DefenceText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            DefenceText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Update HP stat (only for equipment)
    if (HPText)
    {
        bool bIsEquipment = (ItemInfo.ItemType == EItemType::Equip);
        if (bIsEquipment && ItemInfo.HP > 0)
        {
            FString HPString = FString::Printf(TEXT("HP %d"), ItemInfo.HP);
            HPText->SetText(FText::FromString(HPString));
            HPText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            HPText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Update MP stat (only for equipment)
    if (MPText)
    {
        bool bIsEquipment = (ItemInfo.ItemType == EItemType::Equip);
        if (bIsEquipment && ItemInfo.MP > 0)
        {
            FString MPString = FString::Printf(TEXT("MP %d"), ItemInfo.MP);
            MPText->SetText(FText::FromString(MPString));
            MPText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            MPText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Update STR stat (only for equipment)
    if (STRText)
    {
        bool bIsEquipment = (ItemInfo.ItemType == EItemType::Equip);
        if (bIsEquipment && ItemInfo.STR > 0)
        {
            FString STRString = FString::Printf(TEXT("STR %d"), ItemInfo.STR);
            STRText->SetText(FText::FromString(STRString));
            STRText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            STRText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Update DEX stat (only for equipment)
    if (DEXText)
    {
        bool bIsEquipment = (ItemInfo.ItemType == EItemType::Equip);
        if (bIsEquipment && ItemInfo.DEX > 0)
        {
            FString DEXString = FString::Printf(TEXT("DEX %d"), ItemInfo.DEX);
            DEXText->SetText(FText::FromString(DEXString));
            DEXText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            DEXText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Update INT stat (only for equipment)
    if (INTText)
    {
        bool bIsEquipment = (ItemInfo.ItemType == EItemType::Equip);
        if (bIsEquipment && ItemInfo.INT > 0)
        {
            FString INTString = FString::Printf(TEXT("INT %d"), ItemInfo.INT);
            INTText->SetText(FText::FromString(INTString));
            INTText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            INTText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Update description
    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(ItemInfo.ItemDescription));
    }

    // Update Price display
    if (Price)
    {
        FString PriceString = FString::Printf(TEXT("%d"), ItemInfo.Price > 0 ? ItemInfo.Price : 100);
        Price->SetText(FText::FromString(PriceString));
        UE_LOG(LogTemp, Verbose, TEXT("StoreItem: Set price to %s for %s"), *PriceString, *ItemInfo.ItemName);
    }

    // Update optional price-related widgets if they exist
    if (ItemPriceText)
    {
        FString PriceString = FString::Printf(TEXT("%d Gold"), ItemInfo.Price);
        ItemPriceText->SetText(FText::FromString(PriceString));
    }

    // Also try to update legacy optional widgets
    if (ItemNameText)
    {
        ItemNameText->SetText(FText::FromString(ItemInfo.ItemName));
    }

    if (ItemDescriptionText)
    {
        ItemDescriptionText->SetText(FText::FromString(ItemInfo.ItemDescription));
    }


}

void UWBP_StoreItemElement::OnBuyButtonClicked()
{
    if (!ParentStore)
    {
        UE_LOG(LogTemp, Error, TEXT("StoreItem: ParentStore is null"));
        return;
    }

    // Verify item data is valid
    if (ItemInfo.ItemIndex <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("StoreItem: Invalid item index: %d"), ItemInfo.ItemIndex);
        return;
    }

    // Get fresh item data to ensure we have the latest information
    FStructure_ItemInfo FreshItemInfo;
    if (!UStoreSystemFix::GetItemData(ItemInfo.ItemIndex, FreshItemInfo))
    {
        UE_LOG(LogTemp, Error, TEXT("StoreItem: Failed to get fresh data for %d"), ItemInfo.ItemIndex);
        return;
    }

    // Open purchase popup with fresh data
    ParentStore->OpenPurchasePopup(FreshItemInfo);
}

void UWBP_StoreItemElement::SetParentStore(UWBP_Store* InParentStore)
{
    ParentStore = InParentStore;
}
