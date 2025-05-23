#include "WBP_StoreItemElement.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "WBP_StorePopup.h"
#include "AtlantisEonsCharacter.h"
#include "WBP_Main.h"

void UWBP_StoreItemElement::OnBuyButtonClicked()
{
    // Create Store Popup Widget
    UWBP_StorePopup* StorePopup = CreateWidget<UWBP_StorePopup>(GetWorld(), LoadClass<UWBP_StorePopup>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/Store/WBP_StorePopup.WBP_StorePopup_C")));
    
    if (StorePopup)
    {
        // Set alignment and add to viewport
        StorePopup->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
        StorePopup->AddToViewport();
        
        // Set the item index
        StorePopup->ItemIndex = ItemIndex;
        
        // Get player character and set buying state
        if (ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
        {
            if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PlayerCharacter))
            {
                if (UWBP_Main* MainWidget = Character->Main)
                {
                    MainWidget->buying = true;
                }
            }
        }
    }
}
