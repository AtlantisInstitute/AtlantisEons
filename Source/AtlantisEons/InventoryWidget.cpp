#include "InventoryWidget.h"
#include "AtlantisEonsCharacter.h"

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Get the owning player character
    if (APlayerController* PC = GetOwningPlayer())
    {
        OwningCharacter = Cast<AAtlantisEonsCharacter>(PC->GetCharacter());
    }
}

void UInventoryWidget::CloseInventory()
{
    if (OwningCharacter)
    {
        OwningCharacter->CloseInventory();
    }
}
