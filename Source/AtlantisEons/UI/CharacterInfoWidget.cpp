#include "CharacterInfoWidget.h"
#include "AtlantisEons/AtlantisEonsHUD.h"
#include "Kismet/GameplayStatics.h"

void UCharacterInfoWidget::HandleResumeClicked()
{
    // Try to get the HUD and call ResumeGame
    if (AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
    {
        HUD->ResumeGame();
    }
    
    // Broadcast the event for any parent widgets that might be listening
    OnResumeGame.Broadcast();
}
