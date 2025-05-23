#include "WBP_ItemDescription.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

UWBP_ItemDescription::UWBP_ItemDescription(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UWBP_ItemDescription::NativeConstruct()
{
    Super::NativeConstruct();
}

void UWBP_ItemDescription::UpdateDescription(const FStructure_ItemInfo& ItemInfo)
{
    if (ItemThumbnail)
    {
        if (!ItemInfo.ItemThumbnail.IsNull())
        {
            if (UTexture2D* LoadedTexture = ItemInfo.ItemThumbnail.LoadSynchronous())
            {
                ItemThumbnail->SetBrushFromTexture(LoadedTexture);
            }
        }
    }

    if (Title)
    {
        Title->SetText(FText::FromString(ItemInfo.ItemName));
    }

    if (ItemSlotType)
    {
        ItemSlotType->SetText(FText::FromString(GetItemTypeString(ItemInfo.ItemType)));
    }

    if (RecoveryHP)
    {
        RecoveryHP->SetText(ItemInfo.RecoveryHP > 0 
            ? FText::FromString(FString::Printf(TEXT("+HP %d"), ItemInfo.RecoveryHP))
            : FText::GetEmpty());
    }

    if (RecoveryMP)
    {
        RecoveryMP->SetText(ItemInfo.RecoveryMP > 0
            ? FText::FromString(FString::Printf(TEXT("+MP %d"), ItemInfo.RecoveryMP))
            : FText::GetEmpty());
    }

    if (DamageText)
    {
        DamageText->SetText(ItemInfo.Damage > 0
            ? FText::FromString(FString::Printf(TEXT("DAMAGE +%d"), ItemInfo.Damage))
            : FText::GetEmpty());
    }

    if (DefenceText)
    {
        DefenceText->SetText(ItemInfo.Defence > 0
            ? FText::FromString(FString::Printf(TEXT("DEFENCE +%d"), ItemInfo.Defence))
            : FText::GetEmpty());
    }

    if (HPText)
    {
        HPText->SetText(ItemInfo.HP > 0
            ? FText::FromString(FString::Printf(TEXT("HP +%d"), ItemInfo.HP))
            : FText::GetEmpty());
    }

    if (MPText)
    {
        MPText->SetText(ItemInfo.MP > 0
            ? FText::FromString(FString::Printf(TEXT("MP +%d"), ItemInfo.MP))
            : FText::GetEmpty());
    }

    if (STRText)
    {
        STRText->SetText(ItemInfo.STR > 0
            ? FText::FromString(FString::Printf(TEXT("STR +%d"), ItemInfo.STR))
            : FText::GetEmpty());
    }

    if (DEXText)
    {
        DEXText->SetText(ItemInfo.DEX > 0
            ? FText::FromString(FString::Printf(TEXT("DEX +%d"), ItemInfo.DEX))
            : FText::GetEmpty());
    }

    if (INTText)
    {
        INTText->SetText(ItemInfo.INT > 0
            ? FText::FromString(FString::Printf(TEXT("INT +%d"), ItemInfo.INT))
            : FText::GetEmpty());
    }

    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(ItemInfo.ItemDescription));
    }
}

FString UWBP_ItemDescription::GetItemTypeString(EItemType ItemType) const
{
    switch (ItemType)
    {
        case EItemType::Equip:
            return TEXT("Equipment");
        case EItemType::Consume_HP:
            return TEXT("HP Consumable");
        case EItemType::Consume_MP:
            return TEXT("MP Consumable");
        case EItemType::Collection:
            return TEXT("Collection Item");
        default:
            return TEXT("Unknown");
    }
}


