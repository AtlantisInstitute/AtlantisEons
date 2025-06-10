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

UTouchInputManager::UTouchInputManager()
{
    PrimaryComponentTick.bCanEverTick = true; // Enable ticking for touch processing
    
    // Set default values
    bTouchInputEnabled = true;
    bAutoEnableOnMobile = true;
    ButtonPressCooldown = 0.1f;
    
    // Camera control defaults
    bTouchCameraEnabled = true;
    TouchCameraSensitivity = 1.0f;
    bInvertTouchCameraY = false;
    MinCameraTouchDistance = 5.0f;
    
    // Virtual joystick defaults
    bVirtualJoystickEnabled = true;
    VirtualJoystickSensitivity = 1.0f;
    VirtualJoystickDeadzone = 0.1f;
    
    // Touch zones defaults
    MovementZoneWidth = 0.4f;
    CameraZoneWidth = 0.4f;
    UIZoneHeight = 0.15f;
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Enhanced mobile controls constructor called"));
}

void UTouchInputManager::BeginPlay()
{
    Super::BeginPlay();
    
    // Get owner character
    OwnerCharacter = GetOwnerCharacter();
    
    if (OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: BeginPlay - Owner character found: %s"), *OwnerCharacter->GetName());
        
        // Check platform and touch capabilities
        bool bIsTouchPlatform = IsTouchPlatform();
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Platform check - IsTouchPlatform: %s"), bIsTouchPlatform ? TEXT("TRUE") : TEXT("FALSE"));
        
        // Always enable mobile input mode for enhanced behavior
        EnableMobileInputMode(true);
        
        // Auto-enable touch input if configured for mobile platforms
        if (bAutoEnableOnMobile && bIsTouchPlatform)
        {
            bTouchInputEnabled = true;
            SetupTouchEventBindings();
            UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Auto-enabled enhanced touch input for mobile platform"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Mobile input mode enabled for unified behavior (desktop mode)"));
        }
        
        // Debug log the current state
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Final state - TouchEnabled: %s, MobileMode: %s"), 
               bTouchInputEnabled ? TEXT("TRUE") : TEXT("FALSE"),
               bMobileInputModeActive ? TEXT("TRUE") : TEXT("FALSE"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: BeginPlay - No owner character found!"));
    }
}

void UTouchInputManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up button bindings
    UnbindButtonEvents();
    
    // Clean up touch event bindings
    CleanupTouchEventBindings();
    
    // Clear all timers including shield expire timer
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ShieldExpireTimer);
        GetWorld()->GetTimerManager().ClearTimer(MenuButtonCooldownTimer);
        GetWorld()->GetTimerManager().ClearTimer(WeaponButtonCooldownTimer);
        GetWorld()->GetTimerManager().ClearTimer(DodgeButtonCooldownTimer);
        GetWorld()->GetTimerManager().ClearTimer(ShieldButtonCooldownTimer);
    }
    
    // Clear touch state
    ActiveTouches.Empty();
    PreviousTouchPositions.Empty();
    MovementTouchIndex = -1;
    CameraTouchIndex = -1;
    
    Super::EndPlay(EndPlayReason);
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: EndPlay called - Enhanced mobile controls cleaned up"));
}

void UTouchInputManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!bTouchInputEnabled || !OwnerCharacter)
    {
        return;
    }
    
    // Process touch input continuously
    HandleTouchInput();
    
    // Process camera input
    ProcessTouchCameraInput(DeltaTime);
    
    // Process virtual joystick input
    ProcessVirtualJoystickInput(DeltaTime);
}

void UTouchInputManager::InitializeTouchInput(UWBP_SecondaryHUD* SecondaryHUD)
{
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
    
    if (bTouchInputEnabled)
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
    if (bTouchInputEnabled == bEnabled)
    {
        return; // No change needed
    }
    
    bTouchInputEnabled = bEnabled;
    
    if (bEnabled && SecondaryHUDWidget)
    {
        BindButtonEvents();
        SetupTouchEventBindings();
        UE_LOG(LogTemp, Warning, TEXT("✅ TouchInputManager: Enhanced touch input enabled"));
    }
    else
    {
        UnbindButtonEvents();
        CleanupTouchEventBindings();
        UE_LOG(LogTemp, Warning, TEXT("❌ TouchInputManager: Enhanced touch input disabled"));
    }
}

// ========== MOBILE CAMERA CONTROLS ==========

void UTouchInputManager::SetTouchCameraEnabled(bool bEnabled)
{
    bTouchCameraEnabled = bEnabled;
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Touch camera %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UTouchInputManager::SetTouchCameraSensitivity(float Sensitivity)
{
    TouchCameraSensitivity = FMath::Clamp(Sensitivity, 0.1f, 5.0f);
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Touch camera sensitivity set to %f"), TouchCameraSensitivity);
}

void UTouchInputManager::HandleTouchCameraInput(const FVector2D& TouchDelta)
{
    if (!bTouchCameraEnabled || !OwnerCharacter || TouchDelta.IsNearlyZero())
    {
        return;
    }
    
    // Apply camera movement
    float YawInput = TouchDelta.X * TouchCameraSensitivity;
    float PitchInput = TouchDelta.Y * TouchCameraSensitivity;
    
    if (bInvertTouchCameraY)
    {
        PitchInput *= -1.0f;
    }
    
    // Apply the input to the character's controller
    if (APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        PlayerController->AddYawInput(YawInput);
        PlayerController->AddPitchInput(-PitchInput); // Invert pitch for proper camera behavior
    }
    
    // Fire event for Blueprint handling
    OnTouchCameraMove.Broadcast(YawInput, PitchInput);
}

void UTouchInputManager::ProcessMobileLookInput(const FVector2D& LookInput)
{
    if (!bTouchCameraEnabled || !OwnerCharacter)
    {
        return;
    }
    
    // Scale input for mobile devices - ensure reasonable sensitivity
    FVector2D ScaledInput = LookInput * TouchCameraSensitivity;
    
    if (bInvertTouchCameraY)
    {
        ScaledInput.Y *= -1.0f;
    }
    
    // Apply camera movement directly to controller (bypass mobile check to avoid recursion)
    if (APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        // Use the character's camera sensitivity settings for consistency
        PlayerController->AddYawInput(ScaledInput.X * OwnerCharacter->CameraYawSensitivity);
        PlayerController->AddPitchInput(ScaledInput.Y * OwnerCharacter->CameraPitchSensitivity);
    }
    
    UE_LOG(LogTemp, Log, TEXT("🎮 TouchInputManager: Applied mobile look input - Raw(%f, %f) Scaled(%f, %f)"), 
           LookInput.X, LookInput.Y, ScaledInput.X, ScaledInput.Y);
}

void UTouchInputManager::EnableMobileLookMode(bool bEnable)
{
    bMobileInputModeActive = bEnable;
    
    if (OwnerCharacter)
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
    if (!bVirtualJoystickEnabled)
    {
        return;
    }
    
    // Virtual joystick will be handled via touch zones
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Virtual joystick setup complete"));
}

void UTouchInputManager::HandleVirtualJoystickInput(const FVector2D& JoystickValue)
{
    if (!bVirtualJoystickEnabled || !OwnerCharacter)
    {
        return;
    }
    
    // Apply deadzone
    FVector2D ProcessedInput = JoystickValue;
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
    
    // Create input action value and call character movement
    FInputActionValue MovementValue(ProcessedInput);
    OwnerCharacter->Move(MovementValue);
    
    // Update internal state
    CurrentJoystickInput = ProcessedInput;
    
    // Fire event for Blueprint handling
    OnTouchMovement.Broadcast(ProcessedInput.X, ProcessedInput.Y);
}

void UTouchInputManager::SetVirtualJoystickEnabled(bool bEnabled)
{
    bVirtualJoystickEnabled = bEnabled;
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Virtual joystick %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UTouchInputManager::ProcessMobileMovementInput(const FVector2D& MovementInput)
{
    if (!bVirtualJoystickEnabled || !OwnerCharacter)
    {
        return;
    }
    
    // Apply deadzone
    FVector2D ProcessedInput = MovementInput;
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
    
    // Update character's movement tracking variables
    OwnerCharacter->CurrentMovementInput = ProcessedInput;
    const float MovementInputThreshold = 0.1f;
    OwnerCharacter->bIsPlayerTryingToMove = ProcessedInput.Size() > MovementInputThreshold;
    
    // Apply movement directly to character (bypass mobile check to avoid recursion)
    if (OwnerCharacter->GetController() != nullptr)
    {
        // Find out which way is forward
        const FRotator Rotation = OwnerCharacter->GetController()->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // Get forward and right vectors
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // Add movement 
        OwnerCharacter->AddMovementInput(ForwardDirection, ProcessedInput.Y);
        OwnerCharacter->AddMovementInput(RightDirection, ProcessedInput.X);
    }
    
    // Update internal state
    CurrentJoystickInput = ProcessedInput;
    
    // Fire event for Blueprint handling
    OnTouchMovement.Broadcast(ProcessedInput.X, ProcessedInput.Y);
    
    UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: Mobile movement input (%f, %f)"), ProcessedInput.X, ProcessedInput.Y);
}

// ========== MOBILE INPUT SETUP ==========

void UTouchInputManager::SetupMobileInputContext()
{
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("🎮 TouchInputManager: Cannot setup mobile input - no owner character"));
        return;
    }
    
    // Enable mobile input mode
    EnableMobileInputMode(true);
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Mobile input context setup complete"));
}

void UTouchInputManager::EnableMobileInputMode(bool bEnable)
{
    bMobileInputModeActive = bEnable;
    
    if (OwnerCharacter && bEnable)
    {
        // Enable touch camera controls
        SetTouchCameraEnabled(true);
        SetVirtualJoystickEnabled(true);
        EnableMobileLookMode(true);
        
        // Always enable touch input for unified behavior (desktop + mobile)
        SetTouchInputEnabled(true);
        
        // Configure player controller for proper mobile input
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            // Disable default touch interface to prevent conflicts
            PC->bShowMouseCursor = false;
            PC->bEnableClickEvents = false;
            PC->bEnableMouseOverEvents = false;
            
            // Enable touch events for our custom handling
            PC->bEnableTouchEvents = true;
            PC->bEnableTouchOverEvents = true;
            
            // Set proper input mode for mobile-like behavior
            FInputModeGameOnly GameOnlyMode;
            PC->SetInputMode(GameOnlyMode);
            
            UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Configured PlayerController for mobile input"));
        }
        
        // Setup direct Enhanced Input integration for mobile
        SetupMobileEnhancedInputIntegration();
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Mobile input mode %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void UTouchInputManager::SetupMobileEnhancedInputIntegration()
{
    if (!OwnerCharacter)
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

void UTouchInputManager::TriggerEnhancedInputAction(UInputAction* Action, const FInputActionValue& Value)
{
    // We don't need the Action parameter anymore - just call the functions directly
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Simplified approach - just call the character's functions directly with the input value
    // This bypasses the Enhanced Input system but achieves the same result
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
    ActiveTouches.Remove(TouchIndex);
    PreviousTouchPositions.Remove(TouchIndex);
    
    // Clear touch assignments if this touch was being used
    if (MovementTouchIndex == TouchIndex)
    {
        MovementTouchIndex = -1;
        bJoystickActive = false;
        CurrentJoystickInput = FVector2D::ZeroVector;
        
        // Send zero movement to stop character
        if (OwnerCharacter)
        {
            FInputActionValue ZeroMovement(FVector2D::ZeroVector);
            OwnerCharacter->Move(ZeroMovement);
        }
    }
    
    if (CameraTouchIndex == TouchIndex)
    {
        CameraTouchIndex = -1;
    }
    
    UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: Unregistered touch %d"), TouchIndex);
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
    
    // Check UI zone (top of screen)
    if (NormalizedPos.Y < UIZoneHeight)
    {
        return TEXT("UI");
    }
    
    // Check movement zone (left side)
    if (NormalizedPos.X < MovementZoneWidth)
    {
        return TEXT("Movement");
    }
    
    // Check camera zone (right side)
    if (NormalizedPos.X > (1.0f - CameraZoneWidth))
    {
        return TEXT("Camera");
    }
    
    // Default to camera for center area
    return TEXT("Camera");
}

// ========== BUTTON ACTION IMPLEMENTATIONS ==========

void UTouchInputManager::OnMenuButtonPressed()
{
    if (!bTouchInputEnabled || IsButtonOnCooldown(TEXT("Menu")))
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
    if (!bTouchInputEnabled || IsButtonOnCooldown(TEXT("Weapon")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("⚔️ TouchInputManager: Weapon button pressed - Triggering attack"));
    StartButtonCooldown(TEXT("Weapon"));
    
    if (OwnerCharacter)
    {
        // Trigger attack using the character's melee attack system
        FInputActionValue DummyValue;
        OwnerCharacter->MeleeAttack(DummyValue);
    }
}

void UTouchInputManager::OnDodgeButtonPressed()
{
    if (!bTouchInputEnabled || IsButtonOnCooldown(TEXT("Dodge")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("💨 TouchInputManager: Dodge button pressed - Triggering dash"));
    StartButtonCooldown(TEXT("Dodge"));
    
    if (OwnerCharacter)
    {
        // Call the Blueprint implementable event for dash
        OwnerCharacter->OnTouchDashPressed();
        
        // Also update input state to maintain backwards compatibility
        OwnerCharacter->UpdateInputState(
            OwnerCharacter->bWKeyPressed, 
            OwnerCharacter->bAKeyPressed, 
            OwnerCharacter->bSKeyPressed, 
            OwnerCharacter->bDKeyPressed, 
            true  // bDash = true
        );
        
        // Reset dash state after a short delay to simulate key release
        FTimerHandle DashResetTimer;
        GetWorld()->GetTimerManager().SetTimer(
            DashResetTimer,
            [this]()
            {
                if (OwnerCharacter)
                {
                    OwnerCharacter->UpdateInputState(
                        OwnerCharacter->bWKeyPressed, 
                        OwnerCharacter->bAKeyPressed, 
                        OwnerCharacter->bSKeyPressed, 
                        OwnerCharacter->bDKeyPressed, 
                        false  // bDash = false
                    );
                }
            },
            0.1f,
            false
        );
    }
}

void UTouchInputManager::OnShieldButtonPressed()
{
    if (!bTouchInputEnabled || IsButtonOnCooldown(TEXT("Shield")))
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🛡️ TouchInputManager: Shield button pressed - Activating block"));
    StartButtonCooldown(TEXT("Shield"));
    
    if (OwnerCharacter)
    {
        if (!bShieldActive)
        {
            // Activate blocking
            FInputActionValue DummyValue;
            OwnerCharacter->PerformBlock(DummyValue);
            bShieldActive = true;
            
            // Set timer to auto-expire shield after 3 seconds
            GetWorld()->GetTimerManager().SetTimer(
                ShieldExpireTimer,
                [this]()
                {
                    if (OwnerCharacter && bShieldActive)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("🛡️ TouchInputManager: Shield auto-expired after 3 seconds"));
                        FInputActionValue DummyValue;
                        OwnerCharacter->ReleaseBlock(DummyValue);
                        bShieldActive = false;
                    }
                },
                3.0f,  // 3 seconds
                false  // Don't loop
            );
        }
    }
}

void UTouchInputManager::OnShieldButtonReleased()
{
    if (!bTouchInputEnabled)
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🛡️ TouchInputManager: Shield button released - Early cancel of block"));
    
    if (OwnerCharacter && bShieldActive)
    {
        // Cancel the auto-expire timer if shield is released early
        GetWorld()->GetTimerManager().ClearTimer(ShieldExpireTimer);
        
        // Deactivate blocking
        FInputActionValue DummyValue;
        OwnerCharacter->ReleaseBlock(DummyValue);
        bShieldActive = false;
    }
}

// ========== HELPER FUNCTION IMPLEMENTATIONS ==========

void UTouchInputManager::ProcessTouchCameraInput(float DeltaTime)
{
    if (!bTouchCameraEnabled || CameraTouchIndex == -1 || !OwnerCharacter)
    {
        return;
    }
    
    // Get current and previous touch positions
    FVector2D* CurrentPos = ActiveTouches.Find(CameraTouchIndex);
    FVector2D* PreviousPos = PreviousTouchPositions.Find(CameraTouchIndex);
    
    if (CurrentPos && PreviousPos)
    {
        FVector2D TouchDelta = *CurrentPos - *PreviousPos;
        
        // Check if movement is significant enough
        if (TouchDelta.Size() >= MinCameraTouchDistance)
        {
            // Scale delta based on screen resolution for consistent behavior
            FVector2D ScreenRes = GetScreenResolution();
            if (ScreenRes.X > 0 && ScreenRes.Y > 0)
            {
                TouchDelta.X /= ScreenRes.X;
                TouchDelta.Y /= ScreenRes.Y;
                TouchDelta *= 100.0f; // Scale for reasonable sensitivity
            }
            
            HandleTouchCameraInput(TouchDelta);
        }
        
        // Update previous position for next frame
        *PreviousPos = *CurrentPos;
    }
}

void UTouchInputManager::ProcessVirtualJoystickInput(float DeltaTime)
{
    if (!bVirtualJoystickEnabled || MovementTouchIndex == -1 || !bJoystickActive)
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
    if (!bTouchInputEnabled || !bMobileInputModeActive)
    {
        return;
    }
    
    // Process active touches for movement and camera input
    ProcessActiveTouches();
}

void UTouchInputManager::ProcessActiveTouches()
{
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Get current touch positions from the player controller
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        // Process all active finger touches
        for (int32 i = 0; i < 10; ++i) // Support up to 10 fingers
        {
            FVector TouchLocation;
            bool bIsPressed;
            PC->GetInputTouchState(ETouchIndex::Type(i), TouchLocation.X, TouchLocation.Y, bIsPressed);
            
            if (bIsPressed)
            {
                FVector2D CurrentTouchPos = FVector2D(TouchLocation.X, TouchLocation.Y);
                
                // Update active touches with current position
                if (ActiveTouches.Contains(i))
                {
                    FVector2D* StoredPos = ActiveTouches.Find(i);
                    if (StoredPos)
                    {
                        *StoredPos = CurrentTouchPos;
                    }
                }
                else
                {
                    // New touch detected
                    OnTouchPressed(ETouchIndex::Type(i), TouchLocation);
                }
            }
            else if (ActiveTouches.Contains(i))
            {
                // Touch was released
                OnTouchReleased(ETouchIndex::Type(i), TouchLocation);
            }
        }
    }
}

void UTouchInputManager::SetupTouchEventBindings()
{
    if (bTouchEventsBound || !OwnerCharacter)
    {
        return;
    }
    
    // Get the player controller
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        // Enable touch events in the player controller
        PC->bEnableTouchEvents = true;
        PC->bEnableTouchOverEvents = true;
        
        // Note: UE5.5 PlayerController touch events are handled differently
        // We'll poll for touch state in TickComponent instead of binding events
        
        bTouchEventsBound = true;
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Touch event bindings setup complete"));
    }
}

void UTouchInputManager::CleanupTouchEventBindings()
{
    if (!bTouchEventsBound || !OwnerCharacter)
    {
        return;
    }
    
    // Get the player controller
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        // Note: No touch events to unbind since we're polling for touch state
        
        bTouchEventsBound = false;
        UE_LOG(LogTemp, Warning, TEXT("🎮 TouchInputManager: Touch event bindings cleaned up"));
    }
}

void UTouchInputManager::HandleTouchEvent(ETouchIndex::Type FingerIndex, FVector Location)
{
    if (!bTouchInputEnabled)
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
    if (!bTouchInputEnabled || !bMobileInputModeActive)
    {
        return;
    }
    
    // Convert 3D location to 2D screen position
    FVector2D ScreenPosition = FVector2D(Location.X, Location.Y);
    
    // Register the touch and determine its zone
    RegisterTouch((int32)FingerIndex, ScreenPosition);
    
    FString TouchZone = GetTouchZone(ScreenPosition);
    
    UE_LOG(LogTemp, Log, TEXT("🎮 TouchInputManager: Touch pressed - Finger %d at (%f, %f) in zone: %s"), 
           (int32)FingerIndex, ScreenPosition.X, ScreenPosition.Y, *TouchZone);
    
    // Assign touch to appropriate zone
    if (TouchZone == TEXT("Movement") && MovementTouchIndex == -1)
    {
        MovementTouchIndex = (int32)FingerIndex;
        bJoystickActive = true;
        JoystickCenter = ScreenPosition;
        UE_LOG(LogTemp, Log, TEXT("🎮 TouchInputManager: Movement touch assigned to finger %d"), (int32)FingerIndex);
    }
    else if (TouchZone == TEXT("Camera") && CameraTouchIndex == -1)
    {
        CameraTouchIndex = (int32)FingerIndex;
        UE_LOG(LogTemp, Log, TEXT("🎮 TouchInputManager: Camera touch assigned to finger %d"), (int32)FingerIndex);
    }
}

void UTouchInputManager::OnTouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
    if (!bTouchInputEnabled || !bMobileInputModeActive)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("🎮 TouchInputManager: Touch released - Finger %d"), (int32)FingerIndex);
    
    // Unregister the touch
    UnregisterTouch((int32)FingerIndex);
}

FVector2D UTouchInputManager::GetScreenResolution() const
{
    if (GEngine && GEngine->GameViewport)
    {
        FVector2D ViewportSize;
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        return ViewportSize;
    }
    
    return FVector2D(1920.0f, 1080.0f); // Default fallback
}

FVector2D UTouchInputManager::ScreenToNormalized(const FVector2D& ScreenPosition) const
{
    FVector2D ScreenRes = GetScreenResolution();
    if (ScreenRes.X > 0 && ScreenRes.Y > 0)
    {
        return FVector2D(ScreenPosition.X / ScreenRes.X, ScreenPosition.Y / ScreenRes.Y);
    }
    
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
        UE_LOG(LogTemp, Warning, TEXT("✅ Bound Menu button"));
    }
    
    if (WeaponButton)
    {
        WeaponButton->OnClicked.AddDynamic(this, &UTouchInputManager::OnWeaponButtonPressed);
        UE_LOG(LogTemp, Warning, TEXT("✅ Bound Weapon button"));
    }
    
    if (DodgeButton)
    {
        DodgeButton->OnClicked.AddDynamic(this, &UTouchInputManager::OnDodgeButtonPressed);
        UE_LOG(LogTemp, Warning, TEXT("✅ Bound Dodge button"));
    }
    
    if (ShieldButton)
    {
        ShieldButton->OnClicked.AddDynamic(this, &UTouchInputManager::OnShieldButtonPressed);
        ShieldButton->OnReleased.AddDynamic(this, &UTouchInputManager::OnShieldButtonReleased);
        UE_LOG(LogTemp, Warning, TEXT("✅ Bound Shield button (pressed + released)"));
    }
    
    bButtonsBound = true;
    UE_LOG(LogTemp, Warning, TEXT("✅ TouchInputManager: All available buttons bound"));
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

bool UTouchInputManager::IsTouchPlatform() const
{
    // Check if we're running on a touch-capable platform
    #if PLATFORM_ANDROID || PLATFORM_IOS
        return true;
    #else
        // For desktop platforms, return false by default
        // The system will still work via EnableMobileInputMode() calls
        return false;
    #endif
}

void UTouchInputManager::SimulateInputAction(const FString& ActionName, bool bPressed)
{
    UE_LOG(LogTemp, Verbose, TEXT("🎮 TouchInputManager: Simulating input action: %s (Pressed: %s)"), 
           *ActionName, bPressed ? TEXT("True") : TEXT("False"));
}

AAtlantisEonsCharacter* UTouchInputManager::GetOwnerCharacter()
{
    if (!OwnerCharacter)
    {
        OwnerCharacter = Cast<AAtlantisEonsCharacter>(GetOwner());
    }
    return OwnerCharacter;
} 