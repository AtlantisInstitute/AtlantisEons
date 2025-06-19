#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/WrapBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "BP_Item.h"
#include "WBP_StoreItemElement.h"
#include "WBP_Store.generated.h"

class UWBP_StoreItemElement;

UCLASS()
class ATLANTISEONS_API UWBP_Store : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store")
    TArray<UWBP_StoreItemElement*> StoreElements;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Store", meta = (DisplayName = "Store Item Widget Class"))
    TSubclassOf<UWBP_StoreItemElement> StoreItemElementClass;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWrapBox* StoreItemContainer;

    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_ALL() const;

    UFUNCTION(BlueprintCallable, Category = "Store")
    FText GetItemNumber(EItemEquipSlot ItemEquipSlotType, FText& TotalNumber);

    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_CONSUMABLE();

    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_WEAPON();

    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_SUIT();

    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_SHIELD();

    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_HELMET();
    
    // Get the currently selected item index
    UFUNCTION(BlueprintCallable, Category = "Store")
    int32 GetSelectedItemIndex() const;
    
    // Purchase the currently selected item
    UFUNCTION(BlueprintCallable, Category = "Store")
    void PurchaseSelectedItem();

    // UI Elements
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* ALLButton;
    
    // BuyButton is actually in the StorePopup, make it optional here
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* BuyButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* WeaponButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* HelmetButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* ShieldButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* SuitButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* NoneButton;

    // Text blocks
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* StoreText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ItemStoreText;

    // Store item elements
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWBP_StoreItemElement* WBP_StoreItemElement_C_0;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWBP_StoreItemElement* WBP_StoreItemElement_C_1;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWBP_StoreItemElement* WBP_StoreItemElement_C_2;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWBP_StoreItemElement* WBP_StoreItemElement_C_3;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UWBP_StoreItemElement* WBP_StoreItemElement_C_4;

    // Bottom borders
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* BottomBorder0;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* BottomBorder1;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* BottomBorder2;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* BottomBorder3;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* BottomBorder4;

    // Open purchase popup
    UFUNCTION(BlueprintCallable, Category = "Store")
    void OpenPurchasePopup(const FStructure_ItemInfo& ItemInfo);

    // Backward compatibility - calls GetText_CONSUMABLE()
    UFUNCTION(BlueprintPure, Category = "Store")
    FText GetText_NONE();

protected:
    virtual void NativeConstruct() override;

    // Button click handlers
    UFUNCTION()
    void OnALLButtonClicked();

    UFUNCTION()
    void OnWeaponButtonClicked();

    UFUNCTION()
    void OnHelmetButtonClicked();

    UFUNCTION()
    void OnShieldButtonClicked();

    UFUNCTION()
    void OnSuitButtonClicked();

    UFUNCTION()
    void OnNoneButtonClicked();

    // Store element management
    UFUNCTION()
    void AddingStoreElement(const FStructure_ItemInfo& ItemInfo);
    
    // Legacy method for backward compatibility
    bool AddingStoreElement(int32 ItemIndex, UWBP_StoreItemElement* WBPStoreItemElementRef);
    
    // Handle item selection
    UFUNCTION()
    void OnItemSelected(int32 ItemIndex);
    
private:
    void InitializeStoreElements();
    void UpdateElementVisibility(uint8 EquipSlotType);
    
    // Helper methods for store initialization
    FStructure_ItemInfo CreateFallbackItemData(int32 ItemIndex);
    void PopulateStoreElementUI(UWBP_StoreItemElement* Element, const FStructure_ItemInfo& ItemInfo);
    
    // Apply equipment slot corrections to match character system
    void ApplyEquipmentSlotCorrections(FStructure_ItemInfo& ItemInfo);
    
    // Currently selected item index
    UPROPERTY()
    int32 SelectedItemIndex;
};
