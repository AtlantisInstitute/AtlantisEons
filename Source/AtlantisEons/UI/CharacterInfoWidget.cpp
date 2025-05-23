#include "CharacterInfoWidget.h"
#include "../AtlantisEonsHUD.h"
#include "Kismet/GameplayStatics.h"

void UCharacterInfoWidget::HandleResumeClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("CharacterInfoWidget - HandleResumeClicked called"));
    
    // STEP 1: Immediately clear any UI focus
    if (GEngine && GEngine->GameViewport)
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
        UE_LOG(LogTemp, Warning, TEXT("CharacterInfoWidget - Cleared UI focus"));
    }
    
    // STEP 2: Get the HUD and call CompleteGameResume for reliable input handling
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PC)
    {
        // Hide this widget first
        SetVisibility(ESlateVisibility::Hidden);
        
        if (AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(PC->GetHUD()))
        {
            UE_LOG(LogTemp, Warning, TEXT("CharacterInfoWidget - Calling HUD->CompleteGameResume()"));
            HUD->CompleteGameResume();
        }
        else
        {
            // Fallback if HUD isn't available
            UE_LOG(LogTemp, Warning, TEXT("CharacterInfoWidget - No HUD found, using fallback approach"));
            
            // Set game input mode directly
            FInputModeGameOnly GameOnlyMode;
            PC->SetInputMode(GameOnlyMode);
            PC->SetShowMouseCursor(false);
            PC->FlushPressedKeys();
            
            // Reset character input if possible
            if (APawn* Pawn = PC->GetPawn())
            {
                if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(Pawn))
                {
                    Character->ResetCharacterInput();
                    Character->ForceEnableMovement();
                }
            }
        }
    }
    
    // Remove this widget from the parent
    RemoveFromParent();
    
    // Broadcast the event for any parent widgets that might be listening
    OnResumeGame.Broadcast();
} 