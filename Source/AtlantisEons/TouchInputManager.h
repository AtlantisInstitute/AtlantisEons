#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/Button.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "InputActionValue.h"
#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "TouchInputManager.generated.h"

// Forward declarations
class AAtlantisEonsCharacter;
class UWBP_SecondaryHUD;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTouchCameraMove, float, DeltaX, float, DeltaY);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTouchMovement, float, X, float, Y);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UTouchInputManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UTouchInputManager();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ========== TOUCH INPUT SETUP ==========

    /** Initialize the touch input system with the SecondaryHUD widget */
    UFUNCTION(BlueprintCallable, Category = "Touch Input")
    void InitializeTouchInput(UWBP_SecondaryHUD* SecondaryHUD);

    /** Enable/disable touch input system */
    UFUNCTION(BlueprintCallable, Category = "Touch Input")
    void SetTouchInputEnabled(bool bEnabled);

    /** Check if touch input is currently enabled */
    UFUNCTION(BlueprintPure, Category = "Touch Input")
    bool IsTouchInputEnabled() const { return bTouchInputEnabled; }

    // ========== MOBILE CAMERA CONTROLS ==========

    /** Enable/disable touch camera movement */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Camera")
    void SetTouchCameraEnabled(bool bEnabled);

    /** Set camera sensitivity for touch input */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Camera")
    void SetTouchCameraSensitivity(float Sensitivity);

    /** Handle touch camera movement */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Camera")
    void HandleTouchCameraInput(const FVector2D& TouchDelta);

    /** Process mouse/touch look input for mobile (called from Blueprint) */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Camera")
    void ProcessMobileLookInput(const FVector2D& LookInput);

    /** Enable mobile look mode (disables jump on touch) */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Camera")
    void EnableMobileLookMode(bool bEnable);

    // ========== VIRTUAL MOVEMENT CONTROLS ==========

    /** Setup virtual joystick for movement */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Movement")
    void SetupVirtualJoystick();

    /** Handle virtual joystick movement */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Movement")
    void HandleVirtualJoystickInput(const FVector2D& JoystickValue);

    /** Enable/disable virtual joystick */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Movement")
    void SetVirtualJoystickEnabled(bool bEnabled);

    /** Process mobile movement input (called from Blueprint) */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Movement")
    void ProcessMobileMovementInput(const FVector2D& MovementInput);

    // ========== MOBILE INPUT SETUP ==========

    /** Setup mobile input mapping context for touch controls */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Setup")
    void SetupMobileInputContext();

    /** Enable mobile input mode (changes input behavior for touch) */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Setup")
    void EnableMobileInputMode(bool bEnable);

    /** Check if mobile input mode is active */
    UFUNCTION(BlueprintPure, Category = "Touch Input|Setup")
    bool IsMobileInputModeActive() const { return bMobileInputModeActive; }

    /** Check if mobile input mode is currently active */
    UFUNCTION(BlueprintPure, Category = "Touch Input")
    bool IsInMobileInputMode() const { return bMobileInputModeActive; }

    // ========== MULTI-TOUCH MANAGEMENT ==========

    /** Register a touch for multi-touch tracking */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|MultiTouch")
    void RegisterTouch(int32 TouchIndex, const FVector2D& TouchLocation);

    /** Unregister a touch from multi-touch tracking */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|MultiTouch")
    void UnregisterTouch(int32 TouchIndex);

    /** Check if a specific touch index is active */
    UFUNCTION(BlueprintPure, Category = "Touch Input|MultiTouch")
    bool IsTouchActive(int32 TouchIndex) const;

    /** Get active touch count */
    UFUNCTION(BlueprintPure, Category = "Touch Input|MultiTouch")
    int32 GetActiveTouchCount() const;

    // ========== TOUCH ZONES ==========

    /** Define touch zones for different input types */
    UFUNCTION(BlueprintCallable, Category = "Touch Input|Zones")
    void SetupTouchZones();

    /** Check which touch zone a screen position belongs to */
    UFUNCTION(BlueprintPure, Category = "Touch Input|Zones")
    FString GetTouchZone(const FVector2D& ScreenPosition) const;

    // ========== BUTTON ACTIONS ==========

    /** Handle Menu button press - opens inventory */
    UFUNCTION()
    void OnMenuButtonPressed();

    /** Handle Weapon button press - triggers attack */
    UFUNCTION()
    void OnWeaponButtonPressed();

    /** Handle Dodge button press - activates dash input */
    UFUNCTION()
    void OnDodgeButtonPressed();

    /** Handle Shield button press - activates block mechanic */
    UFUNCTION()
    void OnShieldButtonPressed();

    /** Handle Shield button release - deactivates block mechanic */
    UFUNCTION()
    void OnShieldButtonReleased();

    // ========== EVENTS ==========

    /** Event fired when touch camera movement occurs */
    UPROPERTY(BlueprintAssignable, Category = "Touch Input Events")
    FOnTouchCameraMove OnTouchCameraMove;

    /** Event fired when virtual joystick movement occurs */
    UPROPERTY(BlueprintAssignable, Category = "Touch Input Events")
    FOnTouchMovement OnTouchMovement;

protected:
    // ========== TOUCH INPUT CONFIGURATION ==========

    /** Whether touch input is currently enabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Input Settings")
    bool bTouchInputEnabled = true;

    /** Whether to automatically enable touch input on mobile platforms */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Input Settings")
    bool bAutoEnableOnMobile = true;

    /** Cooldown between button presses to prevent spam */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Input Settings", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float ButtonPressCooldown = 0.1f;

    // ========== CAMERA CONTROL SETTINGS ==========

    /** Whether touch camera movement is enabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Camera Settings")
    bool bTouchCameraEnabled = true;

    /** Camera sensitivity for touch input */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Camera Settings", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float TouchCameraSensitivity = 1.0f;

    /** Whether to invert Y-axis for camera */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Camera Settings")
    bool bInvertTouchCameraY = false;

    /** Minimum touch distance to register as camera movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Camera Settings", meta = (ClampMin = "1.0", ClampMax = "50.0"))
    float MinCameraTouchDistance = 5.0f;

    // ========== VIRTUAL JOYSTICK SETTINGS ==========

    /** Whether virtual joystick is enabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Joystick Settings")
    bool bVirtualJoystickEnabled = true;

    /** Virtual joystick sensitivity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Joystick Settings", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float VirtualJoystickSensitivity = 1.0f;

    /** Virtual joystick deadzone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Virtual Joystick Settings", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float VirtualJoystickDeadzone = 0.1f;

    // ========== TOUCH ZONES CONFIGURATION ==========

    /** Screen percentage for movement zone (left side) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Zones", meta = (ClampMin = "0.1", ClampMax = "0.8"))
    float MovementZoneWidth = 0.4f;

    /** Screen percentage for camera zone (right side) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Zones", meta = (ClampMin = "0.1", ClampMax = "0.8"))
    float CameraZoneWidth = 0.4f;

    /** Screen percentage for UI zone (top) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Zones", meta = (ClampMin = "0.05", ClampMax = "0.3"))
    float UIZoneHeight = 0.15f;

    // ========== BUTTON REFERENCES ==========

    /** Reference to the Menu button widget */
    UPROPERTY(BlueprintReadOnly, Category = "Touch Input")
    UButton* MenuButton;

    /** Reference to the Weapon button widget */
    UPROPERTY(BlueprintReadOnly, Category = "Touch Input")
    UButton* WeaponButton;

    /** Reference to the Dodge button widget */
    UPROPERTY(BlueprintReadOnly, Category = "Touch Input")
    UButton* DodgeButton;

    /** Reference to the Shield button widget */
    UPROPERTY(BlueprintReadOnly, Category = "Touch Input")
    UButton* ShieldButton;

    // ========== INTERNAL STATE ==========

    /** Reference to the character that owns this component */
    UPROPERTY(BlueprintReadOnly, Category = "Touch Input")
    AAtlantisEonsCharacter* OwnerCharacter;

    /** Reference to the Secondary HUD widget */
    UPROPERTY(BlueprintReadOnly, Category = "Touch Input")
    UWBP_SecondaryHUD* SecondaryHUDWidget;

    /** Timer handles for button cooldowns */
    FTimerHandle MenuButtonCooldownTimer;
    FTimerHandle WeaponButtonCooldownTimer;
    FTimerHandle DodgeButtonCooldownTimer;
    FTimerHandle ShieldButtonCooldownTimer;

    /** Cooldown states for each button */
    bool bMenuButtonOnCooldown = false;
    bool bWeaponButtonOnCooldown = false;
    bool bDodgeButtonOnCooldown = false;
    bool bShieldButtonOnCooldown = false;

    /** Whether shield is currently active (for toggle behavior) */
    bool bShieldActive = false;

    /** Timer handle for 3-second shield auto-expire */
    FTimerHandle ShieldExpireTimer;

    // ========== MULTI-TOUCH STATE ==========

    /** Map of active touches */
    UPROPERTY()
    TMap<int32, FVector2D> ActiveTouches;

    /** Previous touch positions for delta calculation */
    UPROPERTY()
    TMap<int32, FVector2D> PreviousTouchPositions;

    /** Track which touch is used for movement */
    UPROPERTY()
    int32 MovementTouchIndex = -1;

    /** Track which touch is used for camera */
    UPROPERTY()
    int32 CameraTouchIndex = -1;

    // ========== VIRTUAL JOYSTICK STATE ==========

    /** Current virtual joystick input value */
    UPROPERTY()
    FVector2D CurrentJoystickInput = FVector2D::ZeroVector;

    /** Virtual joystick center position */
    UPROPERTY()
    FVector2D JoystickCenter = FVector2D::ZeroVector;

    /** Whether virtual joystick is currently being used */
    UPROPERTY()
    bool bJoystickActive = false;

    // ========== HELPER FUNCTIONS ==========

    /** Find and bind to buttons in the Secondary HUD widget */
    void FindAndBindButtons();

    /** Bind button events */
    void BindButtonEvents();

    /** Unbind button events */
    void UnbindButtonEvents();

    /** Check if a specific button is on cooldown */
    bool IsButtonOnCooldown(const FString& ButtonName) const;

    /** Start cooldown for a specific button */
    void StartButtonCooldown(const FString& ButtonName);

    /** Reset cooldown for a specific button */
    void ResetButtonCooldown(const FString& ButtonName);

    /** Check if the current platform supports touch input */
    bool IsTouchPlatform() const;

    /** Simulate input action for keyboard/gamepad equivalents */
    void SimulateInputAction(const FString& ActionName, bool bPressed = true);

    /** Process touch input for camera movement */
    void ProcessTouchCameraInput(float DeltaTime);

    /** Process virtual joystick input for movement */
    void ProcessVirtualJoystickInput(float DeltaTime);

    /** Get screen resolution for touch calculations */
    FVector2D GetScreenResolution() const;

    /** Convert screen position to normalized coordinates */
    FVector2D ScreenToNormalized(const FVector2D& ScreenPosition) const;

    /** Check if a screen position is within a widget's bounds */
    bool IsPositionInWidget(const FVector2D& ScreenPosition, UWidget* Widget) const;

    /** Handle raw touch events */
    void HandleTouchInput();

    /** Process all active touches for movement and camera */
    void ProcessActiveTouches();

    /** Setup touch event bindings */
    void SetupTouchEventBindings();

    /** Cleanup touch event bindings */
    void CleanupTouchEventBindings();

    /** Handle touch event from player controller */
    UFUNCTION()
    void HandleTouchEvent(ETouchIndex::Type FingerIndex, FVector Location);

    /** Handle touch pressed event */
    UFUNCTION()
    void OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);

    /** Handle touch released event */
    UFUNCTION()
    void OnTouchReleased(ETouchIndex::Type FingerIndex, FVector Location);

    /** Setup mobile Enhanced Input integration */
    void SetupMobileEnhancedInputIntegration();

    /** Trigger Enhanced Input Action manually for mobile devices */
    void TriggerEnhancedInputAction(class UInputAction* Action, const FInputActionValue& Value);

private:
    /** Internal flag to track if buttons are currently bound */
    bool bButtonsBound = false;

    /** Internal flag to track if touch events are bound */
    bool bTouchEventsBound = false;

    /** Get owner character safely */
    AAtlantisEonsCharacter* GetOwnerCharacter();

    /** Internal flag to track mobile input mode */
    bool bMobileInputModeActive = false;
}; 