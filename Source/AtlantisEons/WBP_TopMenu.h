#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "WBP_TopMenu.generated.h"

/**
 * Top menu widget for the game UI
 */
UCLASS(Blueprintable)
class ATLANTISEONS_API UWBP_TopMenu : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    
    UFUNCTION()
    virtual void NativeOnInitialized() override;
    
    UFUNCTION()
    virtual void NativeConstruct() override;

    // The button shown in the screenshot
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    class UButton* Button_0;

    // Text block shown in the screenshot
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    class UTextBlock* TextBlock_0;
};
