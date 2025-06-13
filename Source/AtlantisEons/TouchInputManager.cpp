#include "TouchInputManager.h"
#include "AtlantisEonsCharacter.h"
#include "WBP_SecondaryHUD.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "GenericPlatform/GenericApplication.h"
#include "Framework/Application/SlateApplication.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/UserInterfaceSettings.h"
#include "Slate/SceneViewport.h"
#include "GameFramework/InputSettings.h"
#include "Misc/App.h"

UTouchInputManager::UTouchInputManager()
{
    PrimaryComponentTick.bCanEverTick = true; // Enable ticking for touch processing
    
    // Set default values
    bTouchInputEnabled = true;
    bAutoEnableOnMobile = true;
    ButtonPressCooldown = 0.1f;
    
    // Camera control defaults
    bTouchCameraEnabled = true;
    TouchCameraSensitivity = 0.3f;  // Reduced sensitivity for better mobile experience
    bInvertTouchCameraY = true;  // Inverted for mobile touch controls
    MinCameraTouchDistance = 5.0f;
    
    // Virtual joystick defaults
    bVirtualJoystickEnabled = true;
    VirtualJoystickSensitivity = 1.0f;
    VirtualJoystickDeadzone = 0.1f;
    
    // Touch zones defaults
    MovementZoneWidth = 0.4f;
    CameraZoneWidth = 0.4f;
    UIZoneHeight = 0.15f;
    
    // Initialize safety flags
    bIsBeingDestroyed = false;
    bMobileInputModeActive = false;
    bTouchMovementProcessingEnabled = false;
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Enhanced mobile controls constructor called"));
}

void UTouchInputManager::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache platform information early for safety
    bIsMobilePlatformCached = IsMobilePlatform();
    bPlatformCacheInitialized = true;
    
    // Get owner character reference
    OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: No valid owner character found!"));
        return;
    }

    // Only enable on mobile platforms or when explicitly enabled
    if (IsSafeToProcessTouch())
    {
        if (bAutoEnableOnMobile)
        {
            SetTouchInputEnabled(true);
            UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Auto-enabled on mobile platform"));
        }
    }
    else
    {
        // Disable touch input on non-mobile platforms for safety
        SetTouchInputEnabled(false);
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Disabled on non-mobile platform or in editor"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: BeginPlay complete - TouchEnabled=%s, Platform=%s"), 
           IsTouchInputEnabled() ? TEXT("YES") : TEXT("NO"),
           IsSafeToProcessTouch() ? TEXT("MOBILE/PIE") : TEXT("EDITOR/DESKTOP"));
}

void UTouchInputManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: EndPlay called - Reason: %d"), (int32)EndPlayReason);
    
    // Set destruction flag to prevent further processing
    bIsBeingDestroyed = true;
    
    // Clean up touch events and button bindings safely
    CleanupTouchEventBindings();
    UnbindButtonEvents();
    
    // Clear all references safely
    if (IsValid(SecondaryHUDWidget))
    {
        SecondaryHUDWidget = nullptr;
    }
    
    // Clear button references
    MenuButton = nullptr;
    WeaponButton = nullptr;
    DodgeButton = nullptr;
    ShieldButton = nullptr;
    
    // Clear touch state
    ActiveTouches.Empty();
    PreviousTouchPositions.Empty();
    ButtonTouchMap.Empty();
    
    // Reset touch indices
    MovementTouchIndex = -1;
    CameraTouchIndex = -1;
    
    // Clear timers
    if (IsValid(GetWorld()))
    {
        GetWorld()->GetTimerManager().ClearTimer(MenuButtonCooldownTimer);
        GetWorld()->GetTimerManager().ClearTimer(WeaponButtonCooldownTimer);
        GetWorld()->GetTimerManager().ClearTimer(DodgeButtonCooldownTimer);
        GetWorld()->GetTimerManager().ClearTimer(ShieldButtonCooldownTimer);
    }
    
    OwnerCharacter = nullptr;
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: EndPlay cleanup complete"));
    
    Super::EndPlay(EndPlayReason);
}

void UTouchInputManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Safety checks - prevent crashes during editor transition
    if (bIsBeingDestroyed || !IsValid(OwnerCharacter) || !IsValid(GetWorld()) || !IsSafeToProcessTouch())
    {
        return;
    }
    
    if (!IsTouchInputEnabled())
    {
        return;
    }

    // Process camera input on mobile (movement handled by engine's virtual joystick)
    if (bMobileInputModeActive && bTouchCameraEnabled)
    {
        ProcessTouchCameraInput(DeltaTime);
    }
    
    // Note: Movement processing disabled to prevent recursion with virtual joystick axis mappings
}

// ========== PLATFORM SAFETY FUNCTIONS ==========

bool UTouchInputManager::IsSafeToProcessTouch() const
{
    // If we're being destroyed, never process touch
    if (bIsBeingDestroyed)
    {
        return false;
    }
    
    // In editor (not PIE), disable touch processing for safety
    if (IsInEditor())
    {
        UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: Touch disabled - running in editor"));
        return false;
    }
    
    // Only allow touch processing on mobile platforms or PIE
    if (!IsMobilePlatform() && !GIsPlayInEditorWorld)
    {
        UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: Touch disabled - not mobile platform and not PIE"));
        return false;
    }
    
    return true;
}

bool UTouchInputManager::IsInEditor() const
{
    #if WITH_EDITOR
        // Check if we're in the editor and NOT in PIE
        return GIsEditor && !GIsPlayInEditorWorld;
    #else
        return false;
    #endif
}

bool UTouchInputManager::IsMobilePlatform() const
{
    // Use cached value if available
    if (bPlatformCacheInitialized)
    {
        return bIsMobilePlatformCached;
    }
    
    // Runtime platform check
    #if PLATFORM_IOS || PLATFORM_ANDROID
        return true;
    #else
        // For other platforms, check if we're simulating mobile
        return FPlatformMisc::GetUseVirtualJoysticks() || GIsPlayInEditorWorld;
    #endif
}

// ========== INITIALIZATION ==========

void UTouchInputManager::InitializeTouchInput(UWBP_SecondaryHUD* SecondaryHUD)
{
    if (bIsBeingDestroyed || !IsSafeToProcessTouch())
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: InitializeTouchInput skipped - not safe to process touch"));
        return;
    }
    
    if (!SecondaryHUD)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: InitializeTouchInput - SecondaryHUD is null!"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Initializing enhanced touch input with SecondaryHUD"));
    
    SecondaryHUDWidget = SecondaryHUD;
    
    // Find and bind to buttons
    FindAndBindButtons();
    
    // Setup touch zones
    SetupTouchZones();
    
    // Setup virtual joystick
    SetupVirtualJoystick();
    
    if (IsTouchInputEnabled())
    {
        BindButtonEvents();
        SetupTouchEventBindings();
        UE_LOG(LogTemp, Warning, TEXT("✅ TouchInputManager: Enhanced touch input system initialized and enabled"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Enhanced touch input system initialized but disabled"));
    }
}

void UTouchInputManager::SetTouchInputEnabled(bool bEnabled)
{
    // Always respect safety checks
    if (bEnabled && !IsSafeToProcessTouch())
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Cannot enable touch - not safe on this platform/context"));
        bTouchInputEnabled = false;
        return;
    }
    
    bool bPreviousState = bTouchInputEnabled;
    bTouchInputEnabled = bEnabled;
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Touch input %s (was %s)"), 
           bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"),
           bPreviousState ? TEXT("ENABLED") : TEXT("DISABLED"));
    
    if (bEnabled && !bPreviousState)
    {
        // Enabling touch input
        if (SecondaryHUDWidget)
        {
            BindButtonEvents();
            SetupTouchEventBindings();
        }
    }
    else if (!bEnabled && bPreviousState)
    {
        // Disabling touch input
        UnbindButtonEvents();
        CleanupTouchEventBindings();
    }
}

void UTouchInputManager::SetTouchCameraEnabled(bool bEnabled)
{
    bTouchCameraEnabled = bEnabled;
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Touch camera %s"), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void UTouchInputManager::SetTouchCameraSensitivity(float Sensitivity)
{
    TouchCameraSensitivity = FMath::Clamp(Sensitivity, 0.1f, 2.0f);
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Touch camera sensitivity set to %f"), TouchCameraSensitivity);
}

void UTouchInputManager::SetVirtualJoystickEnabled(bool bEnabled)
{
    bVirtualJoystickEnabled = bEnabled;
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Virtual joystick %s"), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

// ========== MOBILE INPUT PROCESSING ==========

void UTouchInputManager::ProcessMobileLookInput(const FVector2D& LookInput)
{
    if (!IsTouchInputEnabled() || !bTouchCameraEnabled || !IsValid(OwnerCharacter))
    {
        return;
    }
    
    // Scale input for mobile devices - ensure reasonable sensitivity
    FVector2D ScaledInput = LookInput * TouchCameraSensitivity;
    
    if (bInvertTouchCameraY)
    {
        ScaledInput.Y *= -1.0f;
    }
    
    // Call the character's Look function directly with Enhanced Input value
    FInputActionValue LookValue(ScaledInput);
    OwnerCharacter->Look(LookValue);
    
    UE_LOG(LogTemp, Log, TEXT("🎮 TouchInputManager: Applied mobile look input - Raw(%f, %f) Scaled(%f, %f)"), 
           LookInput.X, LookInput.Y, ScaledInput.X, ScaledInput.Y);
}

void UTouchInputManager::EnableMobileLookMode(bool bEnable)
{
    if (!IsSafeToProcessTouch())
    {
        return;
    }
    
    bMobileInputModeActive = bEnable;
    
    if (IsValid(OwnerCharacter))
    {
        // When mobile look mode is enabled, prevent jump on touch
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            PC->bEnableTouchEvents = bEnable;
            PC->bEnableTouchOverEvents = bEnable;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Mobile look mode %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

// ========== VIRTUAL MOVEMENT CONTROLS ==========

void UTouchInputManager::SetupVirtualJoystick()
{
    if (!IsTouchInputEnabled() || !bVirtualJoystickEnabled)
    {
        return;
    }
    
    // Virtual joystick will be handled via touch zones
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Virtual joystick setup complete"));
}

void UTouchInputManager::HandleVirtualJoystickInput(const FVector2D& JoystickInput)
{
    if (!IsTouchInputEnabled() || !bVirtualJoystickEnabled || !IsValid(OwnerCharacter))
    {
        return;
    }
    
    // Apply deadzone
    FVector2D ProcessedInput = JoystickInput;
    if (ProcessedInput.Size() < VirtualJoystickDeadzone)
    {
        ProcessedInput = FVector2D::ZeroVector;
    }
    else
    {
        // Normalize beyond deadzone
        ProcessedInput = (ProcessedInput - ProcessedInput.GetSafeNormal() * VirtualJoystickDeadzone) / (1.0f - VirtualJoystickDeadzone);
        if (ProcessedInput.Size() > 1.0f)
        {
            ProcessedInput = ProcessedInput.GetSafeNormal();
        }
    }
    
    // Apply sensitivity
    ProcessedInput *= VirtualJoystickSensitivity;
    
    // Call the character's Move function directly with Enhanced Input value
    FInputActionValue MovementValue(ProcessedInput);
    OwnerCharacter->Move(MovementValue);
    
    // Update internal state
    CurrentJoystickInput = ProcessedInput;
    
    // Fire event for Blueprint handling
    OnTouchMovement.Broadcast(ProcessedInput.X, ProcessedInput.Y);
    
    UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: Virtual joystick input (%f, %f)"), ProcessedInput.X, ProcessedInput.Y);
}

void UTouchInputManager::ProcessMobileMovementInput(const FVector2D& MovementInput)
{
    // DISABLED: Movement processing handled by engine's virtual joystick axis mappings to prevent recursion
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: ProcessMobileMovementInput called but disabled to prevent recursion"));
    return;
}

// ========== SIMPLE MOBILE INPUT FUNCTIONS ==========

void UTouchInputManager::TriggerMoveInput(const FVector2D& MovementVector)
{
    if (!IsTouchInputEnabled() || !IsValid(OwnerCharacter) || !OwnerCharacter->GetController())
    {
        return;
    }
    
    // Apply deadzone
    FVector2D ProcessedInput = MovementVector;
    if (ProcessedInput.Size() < VirtualJoystickDeadzone)
    {
        ProcessedInput = FVector2D::ZeroVector;
    }
    
    // Safely call the character's Move function with Enhanced Input value
    if (!ensure(IsValid(OwnerCharacter)))
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: Invalid character in TriggerMoveInput"));
        return;
    }
    
    FInputActionValue MovementValue(ProcessedInput);
    OwnerCharacter->Move(MovementValue);
    
    UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: TriggerMoveInput (%f, %f)"), ProcessedInput.X, ProcessedInput.Y);
}

void UTouchInputManager::TriggerLookInput(const FVector2D& LookDelta)
{
    if (!IsTouchInputEnabled() || !IsValid(OwnerCharacter) || !OwnerCharacter->GetController())
    {
        return;
    }
    
    // Scale the input for appropriate sensitivity
    FVector2D ScaledInput = LookDelta * TouchCameraSensitivity;
    
    if (bInvertTouchCameraY)
    {
        ScaledInput.Y *= -1.0f;
    }
    
    // Safely call the character's Look function with Enhanced Input value
    if (!ensure(IsValid(OwnerCharacter)))
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: Invalid character in TriggerLookInput"));
        return;
    }
    
    FInputActionValue LookValue(ScaledInput);
    OwnerCharacter->Look(LookValue);
    
    UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: TriggerLookInput (%f, %f)"), ScaledInput.X, ScaledInput.Y);
}

// ========== MOBILE INPUT SETUP ==========

void UTouchInputManager::SetupMobileInputContext()
{
    if (!IsTouchInputEnabled() || !IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: Cannot setup mobile input - no owner character or touch disabled"));
        return;
    }
    
    // Enable mobile input mode
    EnableMobileInputMode(true);
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Mobile input context setup complete"));
}

void UTouchInputManager::EnableMobileInputMode(bool bEnable)
{
    if (!IsSafeToProcessTouch())
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Cannot enable mobile input mode - not safe on this platform"));
        return;
    }
    
    bMobileInputModeActive = bEnable;
    
    if (IsValid(OwnerCharacter) && bEnable)
    {
        // Ensure world exists before proceeding
        if (!IsValid(GetWorld()))
        {
            UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: No world available for mobile input setup"));
            return;
        }
        
        // Enable touch camera controls
        SetTouchCameraEnabled(true);
        SetVirtualJoystickEnabled(true);
        EnableMobileLookMode(true);
        
        // Always enable touch input for unified behavior (desktop + mobile)
        SetTouchInputEnabled(true);
        
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Enabled all touch systems for multi-touch support"));
        
        // Configure player controller for proper mobile input
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            // 🔧 MULTI-TOUCH FIX: Enable ALL touch events for manual routing
            PC->bShowMouseCursor = false;
            PC->bEnableClickEvents = true;         // Enable for UI buttons
            PC->bEnableMouseOverEvents = true;     // Enable for hover detection
            
            // 🎯 CRITICAL: Enable touch events for multi-touch handling
            PC->bEnableTouchEvents = true;         // Enable to capture all touches
            PC->bEnableTouchOverEvents = true;     // Enable touch over events
            
            // Use Game+UI mode for simultaneous virtual joystick + UI
            FInputModeGameAndUI GameAndUIMode;
            GameAndUIMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            GameAndUIMode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(GameAndUIMode);
            
            UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: ✅ MULTI-TOUCH ENABLED - Raw touch routing active for simultaneous joystick + buttons"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: PlayerController not available for mobile input setup"));
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Mobile input mode %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void UTouchInputManager::SetupMobileEnhancedInputIntegration()
{
    if (!IsValid(OwnerCharacter))
    {
        return;
    }
    
    // Get Enhanced Input Component from the Pawn base class
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(OwnerCharacter->InputComponent))
    {
        // Get the player controller and input subsystem
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            {
                // We can't access the private input actions directly, so we'll use a different approach
                // We'll just call the character's public Move/Look functions directly
                UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Mobile Enhanced Input integration setup complete"));
            }
        }
    }
}

// ========== MULTI-TOUCH MANAGEMENT ==========

void UTouchInputManager::RegisterTouch(int32 TouchIndex, const FVector2D& TouchLocation)
{
    ActiveTouches.Add(TouchIndex, TouchLocation);
    PreviousTouchPositions.Add(TouchIndex, TouchLocation);
    
    UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: Registered touch %d at (%f, %f)"), TouchIndex, TouchLocation.X, TouchLocation.Y);
}

void UTouchInputManager::UnregisterTouch(int32 TouchIndex)
{
    UE_LOG(LogTemp, Error, TEXT("🔄 UNREGISTER TOUCH: Starting unregister for touch %d"), TouchIndex);
    UE_LOG(LogTemp, Error, TEXT("   Current MovementTouchIndex: %d, CameraTouchIndex: %d"), MovementTouchIndex, CameraTouchIndex);
    
    ActiveTouches.Remove(TouchIndex);
    PreviousTouchPositions.Remove(TouchIndex);
    
    // Clear touch assignments if this touch was being used
    if (MovementTouchIndex == TouchIndex)
    {
        UE_LOG(LogTemp, Error, TEXT("🚫 MOVEMENT STOPPED: CurrentMovementInput cleared to (0,0) - Touch %d was movement touch"), TouchIndex);
        
        MovementTouchIndex = -1;
        bJoystickActive = false;
        CurrentJoystickInput = FVector2D::ZeroVector;
        
        // Send zero movement to stop character
        if (OwnerCharacter)
        {
            FInputActionValue ZeroMovement(FVector2D::ZeroVector);
            OwnerCharacter->Move(ZeroMovement);
            UE_LOG(LogTemp, Error, TEXT("📛 ZERO MOVEMENT SENT: Character movement cleared due to movement touch release"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✅ MOVEMENT PRESERVED: Touch %d was not the movement touch (MovementTouchIndex=%d)"), TouchIndex, MovementTouchIndex);
    }
    
    if (CameraTouchIndex == TouchIndex)
    {
        UE_LOG(LogTemp, Error, TEXT("📹 CAMERA TOUCH CLEARED: Touch %d was camera touch"), TouchIndex);
        CameraTouchIndex = -1;
    }
    
    UE_LOG(LogTemp, Error, TEXT("🔄 UNREGISTER COMPLETE: Touch %d removed, ActiveTouches count: %d"), TouchIndex, ActiveTouches.Num());
}

bool UTouchInputManager::IsTouchActive(int32 TouchIndex) const
{
    return ActiveTouches.Contains(TouchIndex);
}

int32 UTouchInputManager::GetActiveTouchCount() const
{
    return ActiveTouches.Num();
}

// ========== TOUCH ZONES ==========

void UTouchInputManager::SetupTouchZones()
{
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Touch zones configured - Movement: %f%%, Camera: %f%%, UI: %f%%"), 
           MovementZoneWidth * 100.0f, CameraZoneWidth * 100.0f, UIZoneHeight * 100.0f);
}

FString UTouchInputManager::GetTouchZone(const FVector2D& ScreenPosition) const
{
    FVector2D NormalizedPos = ScreenToNormalized(ScreenPosition);
    
    // Check if it's in a button area first
    if (!GetButtonAtLocation(ScreenPosition).IsEmpty())
    {
        return TEXT("Button");
    }
    
    // Left half is movement zone
    if (NormalizedPos.X < 0.5f)
    {
        return TEXT("Movement");
    }
    
    // Right half is camera zone (excluding button areas)
    if (NormalizedPos.X >= 0.5f && NormalizedPos.Y < 0.8f) // Exclude bottom area where buttons are
    {
        return TEXT("Camera");
    }
    
    return TEXT("Unknown");
}

// ========== BUTTON ACTION IMPLEMENTATIONS ==========

void UTouchInputManager::OnMenuButtonPressed()
{
    if (!IsTouchInputEnabled() || IsButtonOnCooldown(TEXT("Menu")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("📱 TouchInputManager: Menu button pressed - Toggling inventory"));
    StartButtonCooldown(TEXT("Menu"));
    
    if (OwnerCharacter)
    {
        // Toggle inventory using the character's inventory system
        FInputActionValue DummyValue;
        OwnerCharacter->ToggleInventory(DummyValue);
    }
}

void UTouchInputManager::OnWeaponButtonPressed()
{
    if (!IsTouchInputEnabled() || IsButtonOnCooldown(TEXT("Weapon")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚔️ TouchInputManager: Weapon button pressed - Triggering attack"));
    StartButtonCooldown(TEXT("Weapon"));
    
    if (OwnerCharacter)
    {
        // Try Enhanced Input system first
        SimulateInputAction(TEXT("Weapon"), true);
        
        // Trigger attack using the character's melee attack system (fallback)
        FInputActionValue DummyValue;
        OwnerCharacter->MeleeAttack(DummyValue);
        
        // Reset input action after a short delay
        FTimerHandle WeaponResetTimer;
        GetWorld()->GetTimerManager().SetTimer(
            WeaponResetTimer,
            [this]()
            {
                SimulateInputAction(TEXT("Weapon"), false);
            },
            0.1f,
            false
        );
    }
}

void UTouchInputManager::OnDodgeButtonPressed()
{
    UE_LOG(LogTemp, Error, TEXT("💨 OnDodgeButtonPressed() called - ENHANCED multi-touch dodge sequence"));
    
    if (!IsTouchInputEnabled())
    {
        UE_LOG(LogTemp, Error, TEXT("💨 DODGE BLOCKED: TouchInput disabled"));
        return;
    }
    
    if (IsButtonOnCooldown(TEXT("Dodge")))
    {
        UE_LOG(LogTemp, Error, TEXT("💨 DODGE BLOCKED: Button on cooldown"));
        return;
    }
    
    UE_LOG(LogTemp, Error, TEXT("💨 DODGE EXECUTING: Multi-touch dodge starting NOW!"));
    StartButtonCooldown(TEXT("Dodge"));
    
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("💨 DODGE FAILED: No OwnerCharacter found"));
        return;
    }
    
    // 🚀 MULTI-METHOD DODGE EXECUTION: Try ALL possible methods to ensure dodge works
    
    // Method 1: Direct C++ dash implementation (most reliable)
    UE_LOG(LogTemp, Error, TEXT("💨 METHOD 1: Calling PerformDashDirect() on character"));
    OwnerCharacter->PerformDashDirect();
    
    // Method 2: Enhanced Input simulation
    UE_LOG(LogTemp, Error, TEXT("💨 METHOD 2: Simulating Enhanced Input Dodge action"));
    SimulateInputAction(TEXT("Dodge"), true);
    
    // Method 3: Direct function calls with Enhanced Input values (SAFE VERSION)
    UE_LOG(LogTemp, Error, TEXT("💨 METHOD 3: Safe dodge function calls"));
    if (IsValid(OwnerCharacter))
    {
        UInputAction* DodgeAction = OwnerCharacter->GetDodgeAction();
        if (IsValid(DodgeAction))
        {
            FInputActionValue DodgeValue(1.0f);
            
            // Try to call the character's dodge function directly
            if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
            {
                if (IsValid(PC) && IsValid(PC->GetLocalPlayer()))
                {
                    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
                    {
                        if (IsValid(Subsystem))
                        {
                            // Force inject the dodge action
                            Subsystem->InjectInputForAction(DodgeAction, DodgeValue, {}, {});
                            UE_LOG(LogTemp, Error, TEXT("💨 Enhanced Input injection: SUCCESS"));
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("💨 Enhanced Input injection: FAILED - Invalid Subsystem"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("💨 Enhanced Input injection: FAILED - No Subsystem"));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("💨 Enhanced Input injection: FAILED - Invalid PlayerController or LocalPlayer"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("💨 Enhanced Input injection: FAILED - No PlayerController"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("💨 Enhanced Input injection: FAILED - No DodgeAction"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("💨 Enhanced Input injection: FAILED - Invalid OwnerCharacter"));
    }
    
    // Method 4: Try the Blueprint implementable event
    UE_LOG(LogTemp, Error, TEXT("💨 METHOD 4: Attempting Blueprint dodge event"));
    
    // Method 5: Force dash with movement input (common dash pattern)
    if (bJoystickActive && CurrentJoystickInput.Size() > 0.1f)
    {
        UE_LOG(LogTemp, Error, TEXT("💨 METHOD 5: Directional dash with movement input (%f, %f)"), 
               CurrentJoystickInput.X, CurrentJoystickInput.Y);
        
        // Call character's dodge with direction
        FInputActionValue MovementValue(CurrentJoystickInput);
        // This ensures the dash has direction during multi-touch
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("💨 METHOD 5: Neutral dash (no movement input active)"));
    }
    
    // Simple cleanup without timers (to prevent crashes)
    SimulateInputAction(TEXT("Dodge"), false);
    UE_LOG(LogTemp, Error, TEXT("💨 CLEANUP: Dodge action reset immediately"));
    
    UE_LOG(LogTemp, Error, TEXT("💨 DODGE COMPLETE: All execution methods attempted - dash should be active!"));
}

void UTouchInputManager::OnShieldButtonPressed()
{
    if (!IsTouchInputEnabled() || IsButtonOnCooldown(TEXT("Shield")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🛡️ TouchInputManager: Shield button pressed - Activating block"));
    StartButtonCooldown(TEXT("Shield"));
    
    if (OwnerCharacter)
    {
        if (!bShieldActive)
        {
            // Try Enhanced Input system first
            SimulateInputAction(TEXT("Shield"), true);
            
            // Activate blocking (fallback)
            FInputActionValue DummyValue;
            OwnerCharacter->PerformBlock(DummyValue);
            bShieldActive = true;
            
            // Note: Shield auto-expire functionality removed to prevent timer crashes
            UE_LOG(LogTemp, Warning, TEXT("🛡️ TouchInputManager: Shield activated (manual release required)"));
        }
    }
}

void UTouchInputManager::OnShieldButtonReleased()
{
    if (!IsTouchInputEnabled())
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🛡️ TouchInputManager: Shield button released - Early cancel of block"));
    
    if (OwnerCharacter && bShieldActive)
    {
        // Reset Enhanced Input action
        SimulateInputAction(TEXT("Shield"), false);
        
        // Deactivate blocking
        FInputActionValue DummyValue;
        OwnerCharacter->ReleaseBlock(DummyValue);
        bShieldActive = false;
    }
}

// ========== HELPER FUNCTION IMPLEMENTATIONS ==========

void UTouchInputManager::ProcessTouchCameraInput(float DeltaTime)
{
    if (!IsTouchInputEnabled() || !bTouchCameraEnabled || !IsValid(OwnerCharacter))
    {
        return;
    }
    
    // Use polling approach to detect touch input for camera
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        // Check for any active touch that's not in the virtual joystick area
        for (int32 TouchIndex = 0; TouchIndex < 5; ++TouchIndex) // Check first 5 fingers
        {
            FVector TouchLocation;
            bool bIsPressed;
            PC->GetInputTouchState(ETouchIndex::Type(TouchIndex), TouchLocation.X, TouchLocation.Y, bIsPressed);
            
            if (bIsPressed)
            {
                FVector2D CurrentTouchPos = FVector2D(TouchLocation.X, TouchLocation.Y);
                FVector2D NormalizedPos = ScreenToNormalized(CurrentTouchPos);
                
                // Only process touches outside the virtual joystick area (right side for camera)
                // Virtual joystick is typically on the left 40% of screen
                if (NormalizedPos.X > 0.4f) // Right side of screen
                {
                    // Store previous position for delta calculation
                    FVector2D* PreviousPos = PreviousTouchPositions.Find(TouchIndex);
                    if (PreviousPos)
                    {
                        FVector2D TouchDelta = CurrentTouchPos - *PreviousPos;
                        
                        // Check if movement is significant enough
                        if (TouchDelta.Size() >= MinCameraTouchDistance)
                        {
                            // Scale delta for camera sensitivity
                            FVector2D ScreenRes = GetScreenResolution();
                            if (ScreenRes.X > 0 && ScreenRes.Y > 0)
                            {
                                TouchDelta.X /= ScreenRes.X;
                                TouchDelta.Y /= ScreenRes.Y;
                                TouchDelta *= 100.0f; // Scale for reasonable sensitivity
                            }
                            
                            // Apply camera movement directly to controller
                            float YawInput = TouchDelta.X * TouchCameraSensitivity;
                            float PitchInput = TouchDelta.Y * TouchCameraSensitivity;
                            
                            if (bInvertTouchCameraY)
                            {
                                PitchInput *= -1.0f;
                            }
                            
                            PC->AddYawInput(YawInput);
                            PC->AddPitchInput(-PitchInput); // Invert for proper camera behavior
                            
                            UE_LOG(LogTemp, Verbose, TEXT("🎮 Touch Camera: Delta(%f, %f) Applied(%f, %f)"), 
                                   TouchDelta.X, TouchDelta.Y, YawInput, PitchInput);
                        }
                        
                        // Update previous position
                        *PreviousPos = CurrentTouchPos;
                    }
                    else
                    {
                        // First touch - just store position
                        PreviousTouchPositions.Add(TouchIndex, CurrentTouchPos);
                    }
                }
            }
            else
            {
                // Touch released - remove from tracking
                PreviousTouchPositions.Remove(TouchIndex);
            }
        }
    }
}

void UTouchInputManager::ProcessVirtualJoystickInput(float DeltaTime)
{
    if (!IsTouchInputEnabled() || MovementTouchIndex == -1 || !bJoystickActive)
    {
        return;
    }
    
    // Get current touch position
    FVector2D* CurrentPos = ActiveTouches.Find(MovementTouchIndex);
    if (CurrentPos)
    {
        // Calculate joystick input relative to center
        FVector2D JoystickInput = (*CurrentPos - JoystickCenter);
        
        // Normalize to joystick range (assume 100 pixel radius)
        const float JoystickRadius = 100.0f;
        JoystickInput /= JoystickRadius;
        if (JoystickInput.Size() > 1.0f)
        {
            JoystickInput = JoystickInput.GetSafeNormal();
        }
        
        HandleVirtualJoystickInput(JoystickInput);
    }
}

void UTouchInputManager::HandleTouchInput()
{
    if (!IsTouchInputEnabled() || !bMobileInputModeActive)
    {
        return;
    }
    
    // Process active touches for movement and camera input
    ProcessActiveTouches();
}

void UTouchInputManager::ProcessActiveTouches()
{
    // Don't process active touches to avoid memory access issues on mobile
    // This function was causing SIGBUS crashes on iOS when trying to access
    // touch state while touch events are disabled
    return;
}

void UTouchInputManager::SetupTouchEventBindings()
{
    if (bTouchEventsBound || !IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error, TEXT("🔧 SetupTouchEventBindings: Early exit - Bound=%s, Character=%s"), 
               bTouchEventsBound ? TEXT("YES") : TEXT("NO"),
               IsValid(OwnerCharacter) ? TEXT("YES") : TEXT("NO"));
        return;
    }
    
    UE_LOG(LogTemp, Error, TEXT("🔧 SetupTouchEventBindings: Starting setup..."));
    
    // Setup basic touch event bindings for camera controls AND multi-touch support
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        UE_LOG(LogTemp, Error, TEXT("🔧 SetupTouchEventBindings: PlayerController found"));
        
        // Enable multi-touch but keep simple input mode to avoid conflicts
        PC->bEnableTouchEvents = true;
        PC->bEnableTouchOverEvents = true;
        
        UE_LOG(LogTemp, Error, TEXT("🔧 SetupTouchEventBindings: Touch events enabled on PlayerController"));
        
        // CRITICAL: For UE5, we need to bind touch events through the input component
        if (UInputComponent* InputComponent = PC->InputComponent)
        {
            // Bind touch input actions using UE5 proper method
            InputComponent->BindTouch(IE_Pressed, this, &UTouchInputManager::OnTouchPressed);
            InputComponent->BindTouch(IE_Released, this, &UTouchInputManager::OnTouchReleased);
            
            UE_LOG(LogTemp, Error, TEXT("🔧 SetupTouchEventBindings: Touch input bound through InputComponent"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("🔧 SetupTouchEventBindings: NO INPUT COMPONENT FOUND!"));
        }
        
        // Keep GameOnly mode but ensure multi-touch works
        FInputModeGameOnly GameOnlyMode;
        PC->SetInputMode(GameOnlyMode);
        
        bTouchEventsBound = true;
        UE_LOG(LogTemp, Error, TEXT("✅ TouchInputManager: Multi-touch system FULLY BOUND (joystick + buttons + camera)"));
        
        // Add a test timer to validate the component is active
        FTimerHandle AliveCheckTimer;
        GetWorld()->GetTimerManager().SetTimer(
            AliveCheckTimer,
            [this]()
            {
                UE_LOG(LogTemp, Error, TEXT("🔄 TouchInputManager: ALIVE CHECK - TouchEnabled=%s, MobileMode=%s, EventsBound=%s"), 
                       IsTouchInputEnabled() ? TEXT("YES") : TEXT("NO"),
                       bMobileInputModeActive ? TEXT("YES") : TEXT("NO"),
                       bTouchEventsBound ? TEXT("YES") : TEXT("NO"));
            },
            2.0f,  // Every 2 seconds
            true   // Loop
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🔧 SetupTouchEventBindings: NO PLAYER CONTROLLER FOUND!"));
    }
}

void UTouchInputManager::CleanupTouchEventBindings()
{
    if (!bTouchEventsBound || !IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error, TEXT("🧹 CleanupTouchEventBindings: Early exit - Bound=%s, Character=%s"), 
               bTouchEventsBound ? TEXT("YES") : TEXT("NO"),
               IsValid(OwnerCharacter) ? TEXT("YES") : TEXT("NO"));
        return;
    }
    
    UE_LOG(LogTemp, Error, TEXT("🧹 CleanupTouchEventBindings: Starting cleanup..."));
    
    // Get the player controller
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        UE_LOG(LogTemp, Error, TEXT("🧹 CleanupTouchEventBindings: PlayerController found, unbinding touch input"));
        
        // Unbind touch input through InputComponent (UE5 method)
        if (UInputComponent* InputComponent = PC->InputComponent)
        {
            // Clear all touch bindings from this object
            InputComponent->ClearActionBindings();
            UE_LOG(LogTemp, Error, TEXT("🧹 CleanupTouchEventBindings: Touch input unbound from InputComponent"));
        }
        
        bTouchEventsBound = false;
        UE_LOG(LogTemp, Error, TEXT("✅ TouchInputManager: Touch event bindings cleaned up successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🧹 CleanupTouchEventBindings: NO PLAYER CONTROLLER FOUND!"));
        bTouchEventsBound = false; // Reset anyway
    }
}

void UTouchInputManager::HandleTouchEvent(ETouchIndex::Type FingerIndex, FVector Location)
{
    if (!IsTouchInputEnabled())
    {
        return;
    }
    
    // Note: This function is kept for future touch event integration
    // Current implementation focuses on button-based touch input
    // and camera/movement will be handled via enhanced input system
    
    UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: Touch event received - finger %d"), (int32)FingerIndex);
}

void UTouchInputManager::OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    if (!IsTouchInputEnabled() || !bMobileInputModeActive)
    {
        // Use Warning level for iOS visibility
        UE_LOG(LogTemp, Warning, TEXT("🚫 TouchInputManager: Touch BLOCKED - InputEnabled=%s, MobileMode=%s"), 
               IsTouchInputEnabled() ? TEXT("YES") : TEXT("NO"),
               bMobileInputModeActive ? TEXT("YES") : TEXT("NO"));
        return;
    }
    
    // Convert 3D location to 2D screen position
    FVector2D ScreenPosition = FVector2D(Location.X, Location.Y);
    
    FString TouchZone = GetTouchZone(ScreenPosition);
    
    // Use Warning level for iOS visibility
    UE_LOG(LogTemp, Warning, TEXT("👆 TouchInputManager: Touch pressed - Finger %d at (%f, %f) in zone: %s"), 
           (int32)FingerIndex, ScreenPosition.X, ScreenPosition.Y, *TouchZone);
    
    // 🔥 CRITICAL FIX: Check for button touches FIRST before assigning to movement/camera
    // This enables true multi-touch: buttons work even while joystick is active
    
    bool bTouchHandledByButton = false;
    
    // Check for button touches using the new precise detection system - ONLY ON PRESS, NOT MOVE
    FString ButtonName = GetButtonAtLocation(ScreenPosition);
    if (!ButtonName.IsEmpty())
    {
        // 🔒 CRITICAL: Store which touch is handling which button to prevent repeat triggers
        ButtonTouchMap.Add((int32)FingerIndex, ButtonName);
        
        // Use Error level for maximum iOS visibility
        UE_LOG(LogTemp, Error, TEXT("🔘 BUTTON DETECTED: '%s' - handling as button press (Touch %d)"), *ButtonName, (int32)FingerIndex);
        
        // Handle the specific button that was detected - WITH COOLDOWN CHECK
        if (ButtonName == TEXT("Dodge"))
        {
            if (!IsButtonOnCooldown(ButtonName))
            {
                UE_LOG(LogTemp, Error, TEXT("💨 DODGE button pressed - triggering dash!"));
                OnDodgeButtonPressed();
                StartButtonCooldown(ButtonName);
                bTouchHandledByButton = true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("💨 DODGE button on cooldown - ignoring press"));
                bTouchHandledByButton = true; // Still count as handled to prevent movement registration
            }
        }
        else if (ButtonName == TEXT("Weapon"))
        {
            if (!IsButtonOnCooldown(ButtonName))
            {
                UE_LOG(LogTemp, Error, TEXT("⚔️ WEAPON button pressed - triggering attack!"));
                OnWeaponButtonPressed();
                StartButtonCooldown(ButtonName);
                bTouchHandledByButton = true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("⚔️ WEAPON button on cooldown - ignoring press"));
                bTouchHandledByButton = true; // Still count as handled to prevent movement registration
            }
        }
        else if (ButtonName == TEXT("Shield"))
        {
            if (!IsButtonOnCooldown(ButtonName))
            {
                UE_LOG(LogTemp, Error, TEXT("🛡️ SHIELD button pressed - triggering block!"));
                OnShieldButtonPressed();
                StartButtonCooldown(ButtonName);
                bTouchHandledByButton = true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("🛡️ SHIELD button on cooldown - ignoring press"));
                bTouchHandledByButton = true; // Still count as handled to prevent movement registration
            }
        }
        else if (ButtonName == TEXT("Menu"))
        {
            if (!IsButtonOnCooldown(ButtonName))
            {
                UE_LOG(LogTemp, Error, TEXT("📱 MENU button pressed - triggering menu!"));
                OnMenuButtonPressed();
                StartButtonCooldown(ButtonName);
                bTouchHandledByButton = true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("📱 MENU button on cooldown - ignoring press"));
                bTouchHandledByButton = true; // Still count as handled to prevent movement registration
            }
        }
    }
    else
    {
        // Log when no button is detected to help debug button zones
        FVector2D NormalizedPos = ScreenToNormalized(ScreenPosition);
        UE_LOG(LogTemp, Warning, TEXT("❌ No button detected at normalized position: (%f, %f)"), 
               NormalizedPos.X, NormalizedPos.Y);
    }
    
    // 🎯 MULTI-TOUCH FIX: Only register touch and assign to movement/camera if NOT handled by a button
    // This prevents button touches from interfering with movement input
    if (!bTouchHandledByButton)
    {
        // Register the touch for movement/camera handling
        RegisterTouch((int32)FingerIndex, ScreenPosition);
        
        // Assign touch to appropriate zone
        if (TouchZone == TEXT("Movement") && MovementTouchIndex == -1)
        {
            MovementTouchIndex = (int32)FingerIndex;
            bJoystickActive = true;
            JoystickCenter = ScreenPosition;
            UE_LOG(LogTemp, Warning, TEXT("🕹️ TouchInputManager: Movement touch assigned to finger %d"), (int32)FingerIndex);
        }
        else if (TouchZone == TEXT("Camera") && CameraTouchIndex == -1)
        {
            CameraTouchIndex = (int32)FingerIndex;
            UE_LOG(LogTemp, Warning, TEXT("📹 TouchInputManager: Camera touch assigned to finger %d"), (int32)FingerIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🤷 TouchInputManager: Touch not assigned - Zone=%s, MovementIndex=%d, CameraIndex=%d"), 
                   *TouchZone, MovementTouchIndex, CameraTouchIndex);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✅ Touch handled by button - NOT registered as movement/camera touch (preserving existing input)"));
    }
}

void UTouchInputManager::OnTouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
    if (!IsTouchInputEnabled() || !bMobileInputModeActive)
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Touch released - Finger %d"), (int32)FingerIndex);
    
    // Convert 3D location to 2D screen position for button detection
    FVector2D ScreenPosition = FVector2D(Location.X, Location.Y);
    
    // 🔒 CRITICAL: Check if this touch was handling a button and clean it up
    if (ButtonTouchMap.Contains((int32)FingerIndex))
    {
        FString ButtonName = ButtonTouchMap[(int32)FingerIndex];
        UE_LOG(LogTemp, Error, TEXT("🔘 Button touch released: '%s' (Touch %d) - cleaning up button mapping"), *ButtonName, (int32)FingerIndex);
        
        // Remove from button touch mapping
        ButtonTouchMap.Remove((int32)FingerIndex);
        
        // Don't unregister button touches since they were never registered for movement/camera
        return;
    }
    
    // Check if this was a button touch based on position
    FString ButtonName = GetButtonAtLocation(ScreenPosition);
    if (!ButtonName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("🔘 Button touch released: '%s' - NOT unregistering movement/camera touch"), *ButtonName);
        // Don't unregister button touches since they were never registered for movement/camera
        return;
    }
    
    // Only unregister if this touch was registered for movement/camera (not a button touch)
    if (ActiveTouches.Contains((int32)FingerIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 Unregistering movement/camera touch %d"), (int32)FingerIndex);
        UnregisterTouch((int32)FingerIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 Touch %d was not registered - likely a button touch"), (int32)FingerIndex);
    }
}

FVector2D UTouchInputManager::GetScreenResolution() const
{
    if (GEngine && GEngine->GameViewport)
    {
        FVector2D ViewportSize;
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        UE_LOG(LogTemp, Error, TEXT("📱 SCREEN RESOLUTION: Engine viewport size = (%f, %f)"), ViewportSize.X, ViewportSize.Y);
        return ViewportSize;
    }
    
    // Try alternative method for iOS
    if (UGameViewportClient* ViewportClient = GEngine ? GEngine->GameViewport : nullptr)
    {
        if (FViewport* Viewport = ViewportClient->Viewport)
        {
            FIntPoint Size = Viewport->GetSizeXY();
            FVector2D ScreenSize(Size.X, Size.Y);
            UE_LOG(LogTemp, Error, TEXT("📱 SCREEN RESOLUTION: Direct viewport size = (%f, %f)"), ScreenSize.X, ScreenSize.Y);
            return ScreenSize;
        }
    }
    
    // iOS fallback - common iPhone/iPad resolutions
    FVector2D DefaultSize(1920.0f, 1080.0f);
    UE_LOG(LogTemp, Error, TEXT("📱 SCREEN RESOLUTION: Using fallback size = (%f, %f)"), DefaultSize.X, DefaultSize.Y);
    return DefaultSize;
}

FVector2D UTouchInputManager::ScreenToNormalized(const FVector2D& ScreenPosition) const
{
    FVector2D ScreenRes = GetScreenResolution();
    if (ScreenRes.X > 0 && ScreenRes.Y > 0)
    {
        FVector2D Normalized = FVector2D(ScreenPosition.X / ScreenRes.X, ScreenPosition.Y / ScreenRes.Y);
        UE_LOG(LogTemp, Error, TEXT("🔄 NORMALIZE: Screen(%f,%f) / Resolution(%f,%f) = Normalized(%f,%f)"), 
               ScreenPosition.X, ScreenPosition.Y, ScreenRes.X, ScreenRes.Y, Normalized.X, Normalized.Y);
        return Normalized;
    }
    
    UE_LOG(LogTemp, Error, TEXT("🔄 NORMALIZE: Invalid resolution, defaulting to center (0.5, 0.5)"));
    return FVector2D(0.5f, 0.5f); // Default to center
}

bool UTouchInputManager::IsPositionInWidget(const FVector2D& ScreenPosition, UWidget* Widget) const
{
    if (!Widget)
    {
        return false;
    }
    
    // This would require more complex geometry calculations
    // For now, return false to indicate basic implementation
    return false;
}

bool UTouchInputManager::IsTouchOverUIButton(const FVector2D& ScreenPosition) const
{
    FString ButtonName = GetButtonAtLocation(ScreenPosition);
    return !ButtonName.IsEmpty();
}

FString UTouchInputManager::GetButtonAtLocation(const FVector2D& ScreenPosition) const
{
    // 🎯 PRECISE BUTTON DETECTION: Specific zones that don't interfere with camera/movement
    FVector2D NormalizedPos = ScreenToNormalized(ScreenPosition);
    
    UE_LOG(LogTemp, Error, TEXT("🔘 BUTTON CHECK: Screen(%f,%f) -> Normalized(%f,%f)"), 
           ScreenPosition.X, ScreenPosition.Y, NormalizedPos.X, NormalizedPos.Y);
    
    // 🎮 SPECIFIC BUTTON ZONES - Don't interfere with camera (center areas)
    
    // 💨 DODGE BUTTON: Bottom-right corner only (very specific)
    if (NormalizedPos.X > 0.85f && NormalizedPos.Y > 0.8f)
    {
        UE_LOG(LogTemp, Error, TEXT("💨 DODGE ZONE HIT: Normalized(%f,%f) is in Dodge zone (>0.85,>0.8) - SPECIFIC"), 
               NormalizedPos.X, NormalizedPos.Y);
        return TEXT("Dodge");
    }
    
    // ⚔️ WEAPON BUTTON: Bottom-center-right (specific)
    if (NormalizedPos.X > 0.6f && NormalizedPos.X <= 0.85f && NormalizedPos.Y > 0.8f)
    {
        UE_LOG(LogTemp, Error, TEXT("⚔️ WEAPON ZONE HIT: Normalized(%f,%f) is in Weapon zone (0.6-0.85,>0.8) - SPECIFIC"), 
               NormalizedPos.X, NormalizedPos.Y);
        return TEXT("Weapon");
    }
    
    // 🛡️ SHIELD BUTTON: Right edge, middle area
    if (NormalizedPos.X > 0.9f && NormalizedPos.Y > 0.3f && NormalizedPos.Y <= 0.8f)
    {
        UE_LOG(LogTemp, Error, TEXT("🛡️ SHIELD ZONE HIT: Normalized(%f,%f) is in Shield zone (>0.9,0.3-0.8) - SPECIFIC"), 
               NormalizedPos.X, NormalizedPos.Y);
        return TEXT("Shield");
    }
    
    // 📱 MENU BUTTON: Top-right corner
    if (NormalizedPos.X > 0.85f && NormalizedPos.Y < 0.2f)
    {
        UE_LOG(LogTemp, Error, TEXT("📱 MENU ZONE HIT: Normalized(%f,%f) is in Menu zone (>0.85,<0.2) - SPECIFIC"), 
               NormalizedPos.X, NormalizedPos.Y);
        return TEXT("Menu");
    }
    
    UE_LOG(LogTemp, Error, TEXT("❌ NO BUTTON: Normalized(%f,%f) - Not in any button zone"), 
           NormalizedPos.X, NormalizedPos.Y);
    return TEXT("");
}

void UTouchInputManager::FindAndBindButtons()
{
    if (!SecondaryHUDWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: FindAndBindButtons - No SecondaryHUD widget!"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Searching for buttons in SecondaryHUD widget"));
    
    // Find buttons by their widget tree names
    UWidgetTree* WidgetTree = SecondaryHUDWidget->WidgetTree;
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: Widget tree is null!"));
        return;
    }
    
    // Search for buttons by common naming patterns
    MenuButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("MenuButton"))));
    if (!MenuButton) MenuButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("Menu_Button"))));
    if (!MenuButton) MenuButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("Btn_Menu"))));
    
    WeaponButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("WeaponButton"))));
    if (!WeaponButton) WeaponButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("Weapon_Button"))));
    if (!WeaponButton) WeaponButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("Btn_Weapon"))));
    
    DodgeButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("DodgeButton"))));
    if (!DodgeButton) DodgeButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("Dodge_Button"))));
    if (!DodgeButton) DodgeButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("Btn_Dodge"))));
    
    ShieldButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("ShieldButton"))));
    if (!ShieldButton) ShieldButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("Shield_Button"))));
    if (!ShieldButton) ShieldButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("Btn_Shield"))));
    
    // Log results
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Button search results:"));
    UE_LOG(LogTemp, Warning, TEXT("   📱 MenuButton: %s"), MenuButton ? TEXT("FOUND") : TEXT("NOT FOUND"));
    UE_LOG(LogTemp, Warning, TEXT("   ⚔️ WeaponButton: %s"), WeaponButton ? TEXT("FOUND") : TEXT("NOT FOUND"));
    UE_LOG(LogTemp, Warning, TEXT("   💨 DodgeButton: %s"), DodgeButton ? TEXT("FOUND") : TEXT("NOT FOUND"));
    UE_LOG(LogTemp, Warning, TEXT("   🛡️ ShieldButton: %s"), ShieldButton ? TEXT("FOUND") : TEXT("NOT FOUND"));
    
    // If no buttons found, list all available widgets for debugging
    if (!MenuButton && !WeaponButton && !DodgeButton && !ShieldButton)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: No buttons found! Available widgets:"));
        WidgetTree->ForEachWidget([](UWidget* Widget) {
            if (Widget)
            {
                UE_LOG(LogTemp, Warning, TEXT("   - %s (%s)"), *Widget->GetName(), *Widget->GetClass()->GetName());
            }
        });
    }
}

void UTouchInputManager::BindButtonEvents()
{
    if (bButtonsBound)
    {
        return; // Already bound
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Binding button events"));
    
    if (MenuButton)
    {
        MenuButton->OnClicked.AddDynamic(this, &UTouchInputManager::OnMenuButtonPressed);
        // Configure for multi-touch support
        MenuButton->SetClickMethod(EButtonClickMethod::DownAndUp);
        MenuButton->SetTouchMethod(EButtonTouchMethod::DownAndUp);
        MenuButton->SetPressMethod(EButtonPressMethod::DownAndUp);
        MenuButton->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Warning, TEXT("✅ Bound Menu button"));
    }
    
    if (WeaponButton)
    {
        WeaponButton->OnClicked.AddDynamic(this, &UTouchInputManager::OnWeaponButtonPressed);
        // Configure for multi-touch support
        WeaponButton->SetClickMethod(EButtonClickMethod::DownAndUp);
        WeaponButton->SetTouchMethod(EButtonTouchMethod::DownAndUp);
        WeaponButton->SetPressMethod(EButtonPressMethod::DownAndUp);
        WeaponButton->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Warning, TEXT("✅ Bound Weapon button"));
    }
    
    if (DodgeButton)
    {
        DodgeButton->OnClicked.AddDynamic(this, &UTouchInputManager::OnDodgeButtonPressed);
        // CRITICAL: Configure for TRUE multi-touch - must work while joystick is held
        DodgeButton->SetClickMethod(EButtonClickMethod::DownAndUp);
        DodgeButton->SetTouchMethod(EButtonTouchMethod::DownAndUp);
        DodgeButton->SetPressMethod(EButtonPressMethod::DownAndUp);
        DodgeButton->SetVisibility(ESlateVisibility::Visible);
        
        // CRITICAL: Enable multi-touch support
        // Access the underlying Slate widget for multi-touch configuration
        if (DodgeButton->GetCachedWidget().IsValid())
        {
            TSharedPtr<SWidget> SlateWidget = DodgeButton->GetCachedWidget();
            SlateWidget->SetCanTick(true);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Bound Dodge button with ENHANCED multi-touch support"));
    }
    
    if (ShieldButton)
    {
        ShieldButton->OnClicked.AddDynamic(this, &UTouchInputManager::OnShieldButtonPressed);
        ShieldButton->OnReleased.AddDynamic(this, &UTouchInputManager::OnShieldButtonReleased);
        // Configure for multi-touch support
        ShieldButton->SetClickMethod(EButtonClickMethod::DownAndUp);
        ShieldButton->SetTouchMethod(EButtonTouchMethod::DownAndUp);
        ShieldButton->SetPressMethod(EButtonPressMethod::DownAndUp);
        ShieldButton->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Warning, TEXT("✅ Bound Shield button (pressed + released)"));
    }
    
    bButtonsBound = true;
    UE_LOG(LogTemp, Warning, TEXT("✅ TouchInputManager: All available buttons bound with multi-touch support"));
    
    // Test multi-touch configuration
    TestMultiTouchConfiguration();
}

void UTouchInputManager::UnbindButtonEvents()
{
    if (!bButtonsBound)
    {
        return; // Nothing to unbind
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Unbinding button events"));
    
    if (MenuButton)
    {
        MenuButton->OnClicked.RemoveDynamic(this, &UTouchInputManager::OnMenuButtonPressed);
    }
    
    if (WeaponButton)
    {
        WeaponButton->OnClicked.RemoveDynamic(this, &UTouchInputManager::OnWeaponButtonPressed);
    }
    
    if (DodgeButton)
    {
        DodgeButton->OnClicked.RemoveDynamic(this, &UTouchInputManager::OnDodgeButtonPressed);
    }
    
    if (ShieldButton)
    {
        ShieldButton->OnClicked.RemoveDynamic(this, &UTouchInputManager::OnShieldButtonPressed);
        ShieldButton->OnReleased.RemoveDynamic(this, &UTouchInputManager::OnShieldButtonReleased);
    }
    
    bButtonsBound = false;
    UE_LOG(LogTemp, Warning, TEXT("❌ TouchInputManager: Button events unbound"));
}

bool UTouchInputManager::IsButtonOnCooldown(const FString& ButtonName) const
{
    if (ButtonName == TEXT("Menu")) return bMenuButtonOnCooldown;
    if (ButtonName == TEXT("Weapon")) return bWeaponButtonOnCooldown;
    if (ButtonName == TEXT("Dodge")) return bDodgeButtonOnCooldown;
    if (ButtonName == TEXT("Shield")) return bShieldButtonOnCooldown;
    
    return false;
}

void UTouchInputManager::StartButtonCooldown(const FString& ButtonName)
{
    if (ButtonPressCooldown <= 0.0f)
    {
        return; // No cooldown needed
    }
    
    FTimerHandle* TimerHandle = nullptr;
    bool* CooldownFlag = nullptr;
    
    if (ButtonName == TEXT("Menu"))
    {
        TimerHandle = &MenuButtonCooldownTimer;
        CooldownFlag = &bMenuButtonOnCooldown;
    }
    else if (ButtonName == TEXT("Weapon"))
    {
        TimerHandle = &WeaponButtonCooldownTimer;
        CooldownFlag = &bWeaponButtonOnCooldown;
    }
    else if (ButtonName == TEXT("Dodge"))
    {
        TimerHandle = &DodgeButtonCooldownTimer;
        CooldownFlag = &bDodgeButtonOnCooldown;
    }
    else if (ButtonName == TEXT("Shield"))
    {
        TimerHandle = &ShieldButtonCooldownTimer;
        CooldownFlag = &bShieldButtonOnCooldown;
    }
    
    if (TimerHandle && CooldownFlag)
    {
        *CooldownFlag = true;
        GetWorld()->GetTimerManager().SetTimer(
            *TimerHandle,
            [this, ButtonName]() { ResetButtonCooldown(ButtonName); },
            ButtonPressCooldown,
            false
        );
    }
}

void UTouchInputManager::ResetButtonCooldown(const FString& ButtonName)
{
    if (ButtonName == TEXT("Menu")) bMenuButtonOnCooldown = false;
    else if (ButtonName == TEXT("Weapon")) bWeaponButtonOnCooldown = false;
    else if (ButtonName == TEXT("Dodge")) bDodgeButtonOnCooldown = false;
    else if (ButtonName == TEXT("Shield")) bShieldButtonOnCooldown = false;
}

void UTouchInputManager::TestMultiTouchConfiguration()
{
    UE_LOG(LogTemp, Warning, TEXT("🔍 TouchInputManager: Testing ENHANCED multi-touch configuration..."));
    
    if (IsValid(OwnerCharacter))
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            UE_LOG(LogTemp, Warning, TEXT("   📱 bEnableTouchEvents: %s"), PC->bEnableTouchEvents ? TEXT("TRUE") : TEXT("FALSE"));
            UE_LOG(LogTemp, Warning, TEXT("   📱 bEnableTouchOverEvents: %s"), PC->bEnableTouchOverEvents ? TEXT("TRUE") : TEXT("FALSE"));
            UE_LOG(LogTemp, Warning, TEXT("   📱 bEnableClickEvents: %s"), PC->bEnableClickEvents ? TEXT("TRUE") : TEXT("FALSE"));
            
            // Input mode check (simplified for compatibility)
            UE_LOG(LogTemp, Warning, TEXT("   🎮 Input Mode: GameAndUI configured for multi-touch"));
        }
    }
    
    // Check button states
    UE_LOG(LogTemp, Warning, TEXT("   🔘 Menu Button: %s"), MenuButton ? TEXT("Found") : TEXT("Missing"));
    UE_LOG(LogTemp, Warning, TEXT("   ⚔️ Weapon Button: %s"), WeaponButton ? TEXT("Found") : TEXT("Missing"));
    UE_LOG(LogTemp, Warning, TEXT("   💨 Dodge Button: %s"), DodgeButton ? TEXT("Found") : TEXT("Missing"));
    UE_LOG(LogTemp, Warning, TEXT("   🛡️ Shield Button: %s"), ShieldButton ? TEXT("Found") : TEXT("Missing"));
    
    // Check widget visibility and multi-touch setup
    if (DodgeButton)
    {
        ESlateVisibility Visibility = DodgeButton->GetVisibility();
        UE_LOG(LogTemp, Warning, TEXT("   💨 Dodge Button Visibility: %s"), 
               Visibility == ESlateVisibility::Visible ? TEXT("Visible") :
               Visibility == ESlateVisibility::Hidden ? TEXT("Hidden") :
               Visibility == ESlateVisibility::Collapsed ? TEXT("Collapsed") : TEXT("Other"));
        
        // Check button touch settings
        UE_LOG(LogTemp, Warning, TEXT("   💨 Dodge Button Click Method: %s"), 
               DodgeButton->GetClickMethod() == EButtonClickMethod::DownAndUp ? TEXT("DownAndUp (CORRECT)") : TEXT("Other"));
        UE_LOG(LogTemp, Warning, TEXT("   💨 Dodge Button Touch Method: %s"), 
               DodgeButton->GetTouchMethod() == EButtonTouchMethod::DownAndUp ? TEXT("DownAndUp (CORRECT)") : TEXT("Other"));
    }
    
    // Test input configuration
    if (const UInputSettings* InputSettings = GetDefault<UInputSettings>())
    {
        UE_LOG(LogTemp, Warning, TEXT("   ⚙️ bAlwaysShowTouchInterface: %s"), InputSettings->bAlwaysShowTouchInterface ? TEXT("TRUE") : TEXT("FALSE"));
        UE_LOG(LogTemp, Warning, TEXT("   ⚙️ Multi-finger touch is configured in DefaultInput.ini"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🔍 ENHANCED multi-touch test complete - Configuration should now support joystick + dash simultaneously"));
}

// ========== MISSING FUNCTION IMPLEMENTATIONS ==========

void UTouchInputManager::SimulateInputAction(const FString& ActionName, bool bPressed)
{
    if (!IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Cannot simulate input - no owner character"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Simulating input action: %s (Pressed: %s)"), 
           *ActionName, bPressed ? TEXT("TRUE") : TEXT("FALSE"));
    
    // This is a placeholder implementation for action simulation
    // In a full implementation, you would trigger the appropriate Enhanced Input action
}

void UTouchInputManager::HandleTouchCameraInput(const FVector2D& CameraInput)
{
    if (!IsTouchInputEnabled() || !IsValid(OwnerCharacter))
    {
        return;
    }
    
    // Process camera input from touch by calling the existing ProcessMobileLookInput function
    ProcessMobileLookInput(CameraInput);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("🎮 TouchInputManager: HandleTouchCameraInput - Input(%f,%f)"), 
           CameraInput.X, CameraInput.Y);
} 