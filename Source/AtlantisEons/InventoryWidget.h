#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class AAtlantisEonsCharacter;

UCLASS()
class ATLANTISEONS_API UInventoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void CloseInventory();

    UPROPERTY()
    AAtlantisEonsCharacter* OwningCharacter;
};
