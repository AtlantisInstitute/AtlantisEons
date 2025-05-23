#include "WBP_TopMenu.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetMathLibrary.h"
#include "WBP_Main.h"

void UWBP_TopMenu::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UE_LOG(LogTemp, Warning, TEXT("WBP_TopMenu::NativeOnInitialized() called"));

    if (Button_0)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_TopMenu::NativeOnInitialized() - Found Button_0"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_TopMenu::NativeOnInitialized() - Button_0 is null!"));
    }
}

void UWBP_TopMenu::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTemp, Warning, TEXT("WBP_TopMenu::NativeConstruct() called"));
}

void UWBP_TopMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (Button_0)
    {
        // Check if button is focused or hovered
        bool bIsFocused = Button_0->HasKeyboardFocus();
        bool bIsHovered = Button_0->IsHovered();
        bool bShouldHighlight = UKismetMathLibrary::BooleanOR(bIsFocused, bIsHovered);

        // Set button color based on state
        if (bShouldHighlight)
        {
            // Highlighted color (light gray)
            Button_0->SetColorAndOpacity(FLinearColor(0.984375f, 0.984375f, 0.984375f, 1.0f));
        }
        else
        {
            // Default color (dark blue-gray)
            Button_0->SetColorAndOpacity(FLinearColor(0.260261f, 0.341393f, 0.359375f, 1.0f));
        }
    }
}
