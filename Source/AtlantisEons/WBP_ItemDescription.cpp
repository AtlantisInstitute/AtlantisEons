#include "WBP_ItemDescription.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "UniversalItemLoader.h"
#include "AtlantisEonsCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UWBP_ItemDescription::UWBP_ItemDescription(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), bIsMonitoring(false)
{
}

void UWBP_ItemDescription::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Start monitoring inventory state when the tooltip is created
    if (!bIsMonitoring)
    {
        bIsMonitoring = true;
        
        // Check inventory state every 0.1 seconds
        GetWorld()->GetTimerManager().SetTimer(
            InventoryStateCheckTimer,
            this,
            &UWBP_ItemDescription::CheckInventoryState,
            0.1f,
            true // Loop
        );
        
        UE_LOG(LogTemp, Log, TEXT("ItemDescription: Started inventory state monitoring"));
    }
}

void UWBP_ItemDescription::NativeDestruct()
{
    // Clean up timer when widget is destroyed
    if (bIsMonitoring && GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(InventoryStateCheckTimer);
        bIsMonitoring = false;
        UE_LOG(LogTemp, Log, TEXT("ItemDescription: Stopped inventory state monitoring"));
    }
    
    Super::NativeDestruct();
}

void UWBP_ItemDescription::CheckInventoryState()
{
    // Get the player character
    if (AAtlantisEonsCharacter* PlayerCharacter = Cast<AAtlantisEonsCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
    {
        // Check if the inventory is closed
        if (!PlayerCharacter->IsInventoryOpen())
        {
            // Inventory is closed, remove this tooltip widget
            UE_LOG(LogTemp, Warning, TEXT("ItemDescription: Inventory closed, auto-removing tooltip"));
            
            // Stop monitoring
            bIsMonitoring = false;
            GetWorld()->GetTimerManager().ClearTimer(InventoryStateCheckTimer);
            
            // Remove from viewport
            if (IsInViewport())
            {
                RemoveFromParent();
            }
            return;
        }
    }
    else
    {
        // If we can't get the player character, something is wrong, remove the tooltip
        UE_LOG(LogTemp, Warning, TEXT("ItemDescription: Could not get player character, removing tooltip"));
        
        // Stop monitoring
        bIsMonitoring = false;
        GetWorld()->GetTimerManager().ClearTimer(InventoryStateCheckTimer);
        
        // Remove from viewport
        if (IsInViewport())
        {
            RemoveFromParent();
        }
    }
}

void UWBP_ItemDescription::UpdateDescription(const FStructure_ItemInfo& ItemInfo)
{
    UE_LOG(LogTemp, Warning, TEXT("ItemDescription: UpdateDescription called for %s"), *ItemInfo.ItemName);
    
    // Ensure the widget itself is visible and properly styled
    SetVisibility(ESlateVisibility::Visible);
    SetRenderOpacity(1.0f);
    SetRenderScale(FVector2D(1.0f, 1.0f));
    
    // Force the widget to be on top and visible
    if (UPanelWidget* RootPanel = Cast<UPanelWidget>(GetRootWidget()))
    {
        RootPanel->SetVisibility(ESlateVisibility::Visible);
        RootPanel->SetRenderOpacity(1.0f);
        RootPanel->SetRenderScale(FVector2D(1.0f, 1.0f));
        UE_LOG(LogTemp, Warning, TEXT("ItemDescription: Set root panel visibility"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ItemDescription: No root panel found!"));
    }
    
    // Set a dark semi-transparent background for the tooltip
    FLinearColor BackgroundColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.9f); // Dark gray background
    SetColorAndOpacity(BackgroundColor);
    UE_LOG(LogTemp, Log, TEXT("ItemDescription: Set dark background"));
    
    // Load and set the item thumbnail using Universal Item Loader (same system as inventory and store)
    UTexture2D* LoadedTexture = UUniversalItemLoader::LoadItemTexture(ItemInfo);
    
    if (LoadedTexture)
    {
        // Try to set the texture on multiple possible image widget names
        bool bImageSet = false;
        
        if (ItemThumbnail)
        {
            ItemThumbnail->SetBrushFromTexture(LoadedTexture);
            ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
            ItemThumbnail->SetRenderOpacity(1.0f);
            bImageSet = true;
            UE_LOG(LogTemp, Warning, TEXT("ItemDescription: Set ItemThumbnail for %s"), *ItemInfo.ItemName);
        }
        
        if (Image)
        {
            Image->SetBrushFromTexture(LoadedTexture);
            Image->SetVisibility(ESlateVisibility::Visible);
            Image->SetRenderOpacity(1.0f);
            bImageSet = true;
            UE_LOG(LogTemp, Warning, TEXT("ItemDescription: Set Image for %s"), *ItemInfo.ItemName);
        }
        
        if (ItemImage)
        {
            ItemImage->SetBrushFromTexture(LoadedTexture);
            ItemImage->SetVisibility(ESlateVisibility::Visible);
            ItemImage->SetRenderOpacity(1.0f);
            bImageSet = true;
            UE_LOG(LogTemp, Warning, TEXT("ItemDescription: Set ItemImage for %s"), *ItemInfo.ItemName);
        }
        
        if (ThumbnailImage)
        {
            ThumbnailImage->SetBrushFromTexture(LoadedTexture);
            ThumbnailImage->SetVisibility(ESlateVisibility::Visible);
            ThumbnailImage->SetRenderOpacity(1.0f);
            bImageSet = true;
            UE_LOG(LogTemp, Warning, TEXT("ItemDescription: Set ThumbnailImage for %s"), *ItemInfo.ItemName);
        }
        
        if (Image_Item)
        {
            Image_Item->SetBrushFromTexture(LoadedTexture);
            Image_Item->SetVisibility(ESlateVisibility::Visible);
            Image_Item->SetRenderOpacity(1.0f);
            bImageSet = true;
            UE_LOG(LogTemp, Warning, TEXT("ItemDescription: Set Image_Item for %s"), *ItemInfo.ItemName);
        }
        
        if (bImageSet)
        {
            UE_LOG(LogTemp, Log, TEXT("ItemDescription: Successfully set thumbnail for %s using texture %s"), *ItemInfo.ItemName, *LoadedTexture->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ItemDescription: No image widgets found to bind for %s"), *ItemInfo.ItemName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ItemDescription: Failed to load thumbnail for %s"), *ItemInfo.ItemName);
    }

    if (Title)
    {
        Title->SetText(FText::FromString(ItemInfo.ItemName));
        Title->SetVisibility(ESlateVisibility::Visible);
        Title->SetRenderOpacity(1.0f);
        UE_LOG(LogTemp, Warning, TEXT("ItemDescription: Set title to %s"), *ItemInfo.ItemName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ItemDescription: Title widget is NULL!"));
    }

    if (ItemSlotType)
    {
        ItemSlotType->SetText(FText::FromString(GetItemTypeString(ItemInfo.ItemType)));
        ItemSlotType->SetVisibility(ESlateVisibility::Visible);
        ItemSlotType->SetRenderOpacity(1.0f);
    }

    if (RecoveryHP)
    {
        FText HPText = ItemInfo.RecoveryHP > 0 
            ? FText::FromString(FString::Printf(TEXT("+HP %d"), ItemInfo.RecoveryHP))
            : FText::GetEmpty();
        RecoveryHP->SetText(HPText);
        RecoveryHP->SetVisibility(!HPText.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        RecoveryHP->SetRenderOpacity(1.0f);
    }

    if (RecoveryMP)
    {
        FText MPText = ItemInfo.RecoveryMP > 0
            ? FText::FromString(FString::Printf(TEXT("+MP %d"), ItemInfo.RecoveryMP))
            : FText::GetEmpty();
        RecoveryMP->SetText(MPText);
        RecoveryMP->SetVisibility(!MPText.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        RecoveryMP->SetRenderOpacity(1.0f);
    }

    if (DamageText)
    {
        FText DmgText = ItemInfo.Damage > 0
            ? FText::FromString(FString::Printf(TEXT("DAMAGE +%d"), ItemInfo.Damage))
            : FText::GetEmpty();
        DamageText->SetText(DmgText);
        DamageText->SetVisibility(!DmgText.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        DamageText->SetRenderOpacity(1.0f);
    }

    if (DefenceText)
    {
        FText DefText = ItemInfo.Defence > 0
            ? FText::FromString(FString::Printf(TEXT("DEFENCE +%d"), ItemInfo.Defence))
            : FText::GetEmpty();
        DefenceText->SetText(DefText);
        DefenceText->SetVisibility(!DefText.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        DefenceText->SetRenderOpacity(1.0f);
    }

    if (HPText)
    {
        FText HPStatText = ItemInfo.HP > 0
            ? FText::FromString(FString::Printf(TEXT("HP +%d"), ItemInfo.HP))
            : FText::GetEmpty();
        HPText->SetText(HPStatText);
        HPText->SetVisibility(!HPStatText.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        HPText->SetRenderOpacity(1.0f);
    }

    if (MPText)
    {
        FText MPStatText = ItemInfo.MP > 0
            ? FText::FromString(FString::Printf(TEXT("MP +%d"), ItemInfo.MP))
            : FText::GetEmpty();
        MPText->SetText(MPStatText);
        MPText->SetVisibility(!MPStatText.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        MPText->SetRenderOpacity(1.0f);
    }

    if (STRText)
    {
        FText StrText = ItemInfo.STR > 0
            ? FText::FromString(FString::Printf(TEXT("STR +%d"), ItemInfo.STR))
            : FText::GetEmpty();
        STRText->SetText(StrText);
        STRText->SetVisibility(!StrText.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        STRText->SetRenderOpacity(1.0f);
    }

    if (DEXText)
    {
        FText DexText = ItemInfo.DEX > 0
            ? FText::FromString(FString::Printf(TEXT("DEX +%d"), ItemInfo.DEX))
            : FText::GetEmpty();
        DEXText->SetText(DexText);
        DEXText->SetVisibility(!DexText.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        DEXText->SetRenderOpacity(1.0f);
    }

    if (INTText)
    {
        FText IntText = ItemInfo.INT > 0
            ? FText::FromString(FString::Printf(TEXT("INT +%d"), ItemInfo.INT))
            : FText::GetEmpty();
        INTText->SetText(IntText);
        INTText->SetVisibility(!IntText.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        INTText->SetRenderOpacity(1.0f);
    }

    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(ItemInfo.ItemDescription));
        DescriptionText->SetVisibility(ESlateVisibility::Visible);
        DescriptionText->SetRenderOpacity(1.0f);
    }
    
    // Force layout update
    ForceLayoutPrepass();
    InvalidateLayoutAndVolatility();
    
    UE_LOG(LogTemp, Log, TEXT("ItemDescription: Updated description for %s with visibility checks"), *ItemInfo.ItemName);
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


