#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "WBP_TopMenu.h"
#include "WBP_CharacterInfo.h"
#include "WBP_Store.h"
#include "WBP_StorePopup.h"
#include "WBP_Inventory.h"
#include "WBP_Main.generated.h"

/**
 * Main widget that handles the UI layout and tab switching
 */
UCLASS()
class ATLANTISEONS_API UWBP_Main : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI")
    FText GetText_Gold() const;

    // Widget switcher to switch between different menus
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UWidgetSwitcher* WidgetSwitcher;

    // Character info widget - updated to work with either WBP_CharacterInfo or WBP_CharacterInfo2
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UWBP_CharacterInfo* WBP_CharacterInfo;

    // Inventory widget - exposed for easier access
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UWBP_Inventory* WBP_Inventory;

    // Custom events for switching tabs
    UFUNCTION()
    void Switcher1();

    UFUNCTION()
    void Switcher2();

    UFUNCTION()
    void Switcher3();

    // State variable for buying mode
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    bool buying;
    
    // Update character info - moved to public for access from other classes
    UFUNCTION(BlueprintCallable, Category = "Character")
    void UpdateCharacterInfo();
    
    // Update the displayed gold amount - moved to public for access from WBP_StorePopup
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateDisplayedGold();

    // Get the inventory widget
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    class UWBP_Inventory* GetInventoryWidget() const { return WBP_Inventory; }

    // Get the character info widget
    UFUNCTION(BlueprintCallable, Category = "Character")
    class UWBP_CharacterInfo* GetCharacterInfoWidget() const { return WBP_CharacterInfo; }

protected:
    UFUNCTION()
    virtual void NativePreConstruct() override;
    
    UFUNCTION()
    virtual void NativeConstruct() override;
    
    UFUNCTION()
    virtual void NativeOnInitialized() override;

    // Store widget
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UWBP_Store* StoreWidget;
    
    // Store popup widget - optional binding
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    class UWBP_StorePopup* WBP_StorePopup;

    // Top menu widgets for each tab
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UWBP_TopMenu* WBP_TopMenu_1;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UWBP_TopMenu* WBP_TopMenu_2;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UWBP_TopMenu* WBP_TopMenu_3;

    // Gold text display
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UTextBlock* GoldText;

    // Setup the menu buttons
    UFUNCTION()
    void SetupMenuButtons();

    // Setup popup navigation based on the Blueprint implementation
    UFUNCTION()
    void SetupPopupNavigation();
    
    // Handle navigation between main panel and popup
    UFUNCTION()
    void HandlePopupNavigation(bool bShowPopup);
    
    // Store purchase handling functions
    UFUNCTION()
    void HandleStoreItemPurchase();
    
    UFUNCTION()
    void HandleStoreConfirmPurchase();
    
    UFUNCTION()
    void HandleStoreCancelPurchase();
};
