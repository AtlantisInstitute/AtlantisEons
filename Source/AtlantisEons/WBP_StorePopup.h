#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/HorizontalBox.h"
#include "ItemTypes.h"
#include "UniversalDataTableReader.h"
#include "WBP_StorePopup.generated.h"

/**
 * Store popup widget for purchase confirmation and quantity selection
 */
UCLASS()
class ATLANTISEONS_API UWBP_StorePopup : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    // Item data
    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 ItemIndex;

    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 StackNumber;

    // New item data structure
    UPROPERTY(BlueprintReadWrite, Category = "Store")
    FStructure_ItemInfo ItemInfo;

    // Selected quantity for purchase
    UPROPERTY(BlueprintReadWrite, Category = "Store")
    int32 SelectedQuantity = 1;

    // UI Elements for new system
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ItemNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ItemDescriptionText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* QuantityText;

    // USER CONFIRMED: The actual quantity widget name in Blueprint
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* Quantity;

    // USER CONFIRMED: The actual item thumbnail widget name in Blueprint
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UImage* ItemThumbnail;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* UnitPriceText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UImage* ItemThumbnailImage;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* IncreaseQuantityButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* DecreaseQuantityButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* ConfirmPurchaseButton;

    // UI Elements with proper BindWidget tags
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* OKButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* ConfirmButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* CancelButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* Button_Up;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* Button_Down;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* TextBlock_StackCounter;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* TextBlock_GoldAmount;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* TotalPriceText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UImage* Image_ItemThumbnail;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UOverlay* Overlay_StackCounter;

    // Horizontal box containers for quantity controls
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    class UHorizontalBox* HorizontalBox_1;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    class UHorizontalBox* HorizontalBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    class UHorizontalBox* QuantityContainer;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    class UHorizontalBox* StackCounterContainer;

    // Alternative UI element names (in case Blueprint uses different naming)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* ButtonOK;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* ButtonCancel;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* UpButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* DownButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* StackCounterText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* GoldAmountText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* PriceText;

    // USER CONFIRMED: The actual price widget name in Blueprint
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* Price;

    // New button event handlers
    UFUNCTION()
    void OnIncreaseQuantityClicked();

    UFUNCTION()
    void OnDecreaseQuantityClicked();

    UFUNCTION()
    void OnConfirmPurchaseClicked();

    // Button event handlers
    UFUNCTION()
    void OnOKButtonClicked();

    UFUNCTION()
    void OnCancelButtonClicked();

    UFUNCTION()
    void OnUpButtonClicked();

    UFUNCTION()
    void OnDownButtonClicked();

    // Blueprint callable functions
    UFUNCTION(BlueprintCallable, Category = "Store")
    void UpdateItemDisplay();

    UFUNCTION(BlueprintCallable, Category = "Store")
    void UpdateItemDetails(int32 InItemIndex);

    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_StackNumber() const;

    UFUNCTION(BlueprintCallable, Category = "Store")
    FText GetText_TotalPrice();

    // New display update methods
    UFUNCTION(BlueprintCallable, Category = "Store")
    void UpdateQuantityDisplay();

    UFUNCTION(BlueprintCallable, Category = "Store")
    void UpdatePriceDisplay();

    UFUNCTION(BlueprintCallable, Category = "Store")
    void UpdateQuantityWidgetVisibility();

private:
    // Helper function to bind buttons safely
    void BindButtonSafely(UButton* Button, FScriptDelegate& Delegate);
    
    // Helper function to update UI text safely
    void UpdateTextSafely(UTextBlock* TextBlock, const FText& NewText);
    
    // Helper function to setup button bindings
    void SetupButtonBindings();
    
    // Helper function to update price displays
    void UpdatePriceDisplays();
    
    // Helper function to get total price
    int32 GetTotalPrice();
    
    // Helper function to adjust image position based on quantity control visibility
    void AdjustImagePositionForLayout(bool bShowQuantityControls);
};
