#include "WBP_ItemNamePopUp.h"

void UWBP_ItemNamePopUp::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button_0)
    {
        Button_0->OnClicked.AddDynamic(this, &UWBP_ItemNamePopUp::OnButtonClicked);
    }
}

void UWBP_ItemNamePopUp::OnButtonClicked()
{
    ClickItemNameDispatcher.Broadcast();
}

// Implementation of SetItemName that was previously BlueprintImplementableEvent
void UWBP_ItemNamePopUp::SetItemName(const FString& NewName)
{
    // Set the text directly in C++ since it's now implemented here instead of in Blueprint
    if (TextBlock)
    {
        // Apply white text color
        FSlateColor TextColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)); // White color
        TextBlock->SetColorAndOpacity(TextColor);
        
        // Set original font size (14)
        FSlateFontInfo FontInfo = TextBlock->GetFont();
        FontInfo.Size = 14; // Standard font size
        TextBlock->SetFont(FontInfo);
        
        // Set the text content
        TextBlock->SetText(FText::FromString(NewName));
        
        UE_LOG(LogTemp, Display, TEXT("SetItemName: Setting text to '%s' with standard styling"), *NewName);
        
        // Force layout recalculation
        if (SideBorder && SideBorder2)
        {
            // Make border elements visible and styled for better framing
            SideBorder->SetVisibility(ESlateVisibility::Visible);
            SideBorder2->SetVisibility(ESlateVisibility::Visible);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SetItemName: TextBlock is null! Cannot set name to '%s'"), *NewName);
        
        // Add to screen debug to help identify the issue
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
                FString::Printf(TEXT("TextBlock is null! Cannot set item name: %s"), *NewName));
        }
    }
    
    // Play show animation if available
    if (ItemNameShow)
    {
        PlayAnimation(ItemNameShow);
    }
}
