#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "WBP_ItemNamePopUp.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FClickItemNameDispatcher);

UCLASS()
class ATLANTISEONS_API UWBP_ItemNamePopUp : public UUserWidget
{
    GENERATED_BODY()

public:
    // UI Components
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* Button_0;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* TextBlock;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* SideBorder;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* SideBorder2;

    // Animations
    UPROPERTY(Transient, meta = (BindWidgetAnim))
    UWidgetAnimation* ItemNameShow;

    UPROPERTY(Transient, meta = (BindWidgetAnim))
    UWidgetAnimation* ItemNameHide;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Item Name")
    FClickItemNameDispatcher ClickItemNameDispatcher;

    virtual void NativeConstruct() override;

    // Functions
    UFUNCTION(BlueprintCallable, Category = "Item Name")
    void PlayShowAnimation() { PlayAnimation(ItemNameShow); }

    UFUNCTION(BlueprintCallable, Category = "Item Name")
    void PlayHideAnimation() { PlayAnimation(ItemNameHide); }

    UFUNCTION(BlueprintCallable, Category = "Item Name")
    void StopHideAnimation()
    {
        if (IsAnimationPlaying(ItemNameHide))
        {
            StopAnimation(ItemNameHide);
        }
    }

    UFUNCTION()
    void OnButtonClicked();

    UFUNCTION(BlueprintCallable, Category = "Item Name")
    void SetItemName(const FString& NewName);
};
