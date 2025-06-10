#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "UITouchInputManager.generated.h"

// Forward declarations
class AAtlantisEonsCharacter;
class UWBP_Main;
class UWBP_InventorySlot;
class UWBP_Store;
class UWBP_CharacterInfo;
class UWBP_Inventory;
class UWidgetSwitcher;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UUITouchInputManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UUITouchInputManager();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ========== UI TOUCH INPUT SETUP ==========

    /** Initialize the UI touch input system with the main widget */
    UFUNCTION(BlueprintCallable, Category = "UI Touch Input")
    void InitializeUITouchInput(UWBP_Main* MainWidget);

    /** Enable/disable UI touch input system */
    UFUNCTION(BlueprintCallable, Category = "UI Touch Input")
    void SetUITouchInputEnabled(bool bEnabled);

    /** Check if UI touch input is currently enabled */
    UFUNCTION(BlueprintPure, Category = "UI Touch Input")
    bool IsUITouchInputEnabled() const { return bUITouchInputEnabled; }

    // ========== WIDGET SWITCHER TOUCH SUPPORT ==========

    /** Setup touch input for widget switcher navigation buttons */
    UFUNCTION(BlueprintCallable, Category = "UI Touch Input")
    void SetupWidgetSwitcherTouchInput();

    /** Handle Character Info tab button press */
    UFUNCTION()
    void OnCharacterInfoTabPressed();

    /** Handle Inventory tab button press */
    UFUNCTION()
    void OnInventoryTabPressed();

    /** Handle Store tab button press */
    UFUNCTION()
    void OnStoreTabPressed();

    // ========== INVENTORY SLOTS TOUCH SUPPORT ==========

    /** Setup touch input for all inventory slots */
    UFUNCTION(BlueprintCallable, Category = "UI Touch Input")
    void SetupInventorySlotsTouchInput();

    /** Handle inventory slot touch */
    UFUNCTION()
    void OnInventorySlotTouched();

    // ========== EQUIPMENT SLOTS TOUCH SUPPORT ==========

    /** Setup touch input for all equipment slots */
    UFUNCTION(BlueprintCallable, Category = "UI Touch Input")
    void SetupEquipmentSlotsTouchInput();

    /** Handle equipment slot touch */
    UFUNCTION()
    void OnEquipmentSlotTouched();

    // ========== STORE BUTTONS TOUCH SUPPORT ==========

    /** Setup touch input for store interface */
    UFUNCTION(BlueprintCallable, Category = "UI Touch Input")
    void SetupStoreTouchInput();

    /** Handle store item purchase button touch */
    UFUNCTION()
    void OnStorePurchaseButtonTouched();

    /** Handle store popup confirm button touch */
    UFUNCTION()
    void OnStoreConfirmButtonTouched();

    /** Handle store popup cancel button touch */
    UFUNCTION()
    void OnStoreCancelButtonTouched();

protected:
    // ========== UI TOUCH INPUT CONFIGURATION ==========

    /** Whether UI touch input is currently enabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Touch Input Settings")
    bool bUITouchInputEnabled = true;

    /** Whether to automatically enable UI touch input on mobile platforms */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Touch Input Settings")
    bool bAutoEnableOnMobile = true;

    /** Cooldown between UI button presses to prevent spam */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Touch Input Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UIButtonPressCooldown = 0.2f;

    // ========== UI WIDGET REFERENCES ==========

    /** Reference to the main widget */
    UPROPERTY(BlueprintReadOnly, Category = "UI Touch Input")
    UWBP_Main* MainWidgetRef;

    /** Reference to the character that owns this component */
    UPROPERTY(BlueprintReadOnly, Category = "UI Touch Input")
    AAtlantisEonsCharacter* OwnerCharacter;

    /** Timer handles for UI button cooldowns */
    FTimerHandle TabButtonCooldownTimer;
    FTimerHandle InventorySlotCooldownTimer;
    FTimerHandle EquipmentSlotCooldownTimer;
    FTimerHandle StoreButtonCooldownTimer;

    /** UI interaction cooldown states */
    bool bTabButtonOnCooldown = false;
    bool bInventorySlotOnCooldown = false;
    bool bEquipmentSlotOnCooldown = false;
    bool bStoreButtonOnCooldown = false;

    // ========== TOUCH INPUT TRACKING ==========

    /** Track which inventory slot was last touched */
    int32 LastTouchedInventorySlotIndex = -1;

    /** Track which equipment slot was last touched */
    int32 LastTouchedEquipmentSlotIndex = -1;

    /** Track which store item was last touched */
    int32 LastTouchedStoreItemIndex = -1;

    // ========== HELPER FUNCTIONS ==========

    /** Enable touch input for a specific widget */
    void EnableWidgetTouchInput(UWidget* Widget);

    /** Add touch event handlers to a widget */
    void AddTouchHandlersToWidget(UWidget* Widget, const FString& WidgetType);

    /** Check if a specific UI element is on cooldown */
    bool IsUIElementOnCooldown(const FString& ElementType) const;

    /** Start cooldown for a specific UI element */
    void StartUIElementCooldown(const FString& ElementType);

    /** Reset cooldown for a specific UI element */
    void ResetUIElementCooldown(const FString& ElementType);

    /** Check if the current platform supports touch input */
    bool IsTouchPlatform() const;

    /** Find and setup touch input for inventory slots */
    void FindAndSetupInventorySlots();

    /** Find and setup touch input for equipment slots */
    void FindAndSetupEquipmentSlots();

    /** Find and setup touch input for store elements */
    void FindAndSetupStoreElements();

    /** Find and setup touch input for widget switcher tabs */
    void FindAndSetupTabNavigation();

private:
    /** Internal flag to track if UI touch input is initialized */
    bool bUITouchInputInitialized = false;

    /** Get owner character safely */
    AAtlantisEonsCharacter* GetOwnerCharacter();
}; 