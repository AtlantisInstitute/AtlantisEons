// Copyright Epic Games, Inc. All Rights Reserved.

#include "AtlantisEonsCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Engine/DamageEvents.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/Vector.h"
#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "AtlantisEonsHUD.h"
#include "DamageNumberSystem.h"
#include "DamageNumberWidget.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AAtlantisEonsCharacter

AAtlantisEonsCharacter::AAtlantisEonsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character    
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->bDoCollisionTest = true; // Enable camera collision
	CameraBoom->ProbeSize = 12.0f; // Adjust probe size for smoother collision
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraLagSpeed = 15.0f;
	CameraBoom->CameraRotationLagSpeed = 10.0f;
	CameraBoom->CameraLagMaxDistance = 100.0f;
	DefaultCameraLag = CameraBoom->CameraLagSpeed;
	DefaultCameraRotationLag = CameraBoom->CameraRotationLagSpeed;

    // Create the follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    if (UCharacterMovementComponent* CharMoveComp = GetCharacterMovement())
    {
        CharMoveComp->bOrientRotationToMovement = true;
        CharMoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
        CharMoveComp->JumpZVelocity = 700.f;
        CharMoveComp->AirControl = 0.35f;
        CharMoveComp->MaxWalkSpeed = 500.f;
        CharMoveComp->MinAnalogWalkSpeed = 20.f;
        CharMoveComp->BrakingDecelerationWalking = 2000.f;
        CharMoveComp->BrakingDecelerationFalling = 1500.0f;
    }



    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

    // Note: Widget components will be set up in Blueprints

    // Note: Widget classes will be set in Blueprints

    // Create AI perception stimuli source component
    AIPerceptionStimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("AIPerceptionStimuliSourceComponent"));
    if (AIPerceptionStimuliSourceComponent)
    {
        AIPerceptionStimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
        AIPerceptionStimuliSourceComponent->RegisterWithPerceptionSystem();
    }

    // Set team ID
    TeamId = FGenericTeamId(1);

    // Set default values
    bIsDodging = false;
    bIsInvulnerable = false;
    bCanDodge = true;
    bCanAttack = true;
    bIsAttacking = false;
    bIsInventoryOpen = false;
    CurrentHealth = MaxHealth = 100.0f;
    DodgeCooldown = 1.0f;
    DodgeInvulnerabilityDuration = 0.5f;
    AttackCooldown = 0.5f;
    DamageAmount = 20.0f;
    DodgeForce = 2000.0f;
    DodgeDistance = 500.0f;
    DodgeStopThreshold = 100.0f;
    DamageSphereRadius = 50.0f;
    DamageSphereOffset = 100.0f;
    AttackRange = 200.0f;
    DefaultCameraLag = 15.0f;
    DefaultCameraRotationLag = 10.0f;
    DodgeCameraLag = 5.0f;
    DodgeCameraRotationLag = 2.0f;
}



void AAtlantisEonsCharacter::PostInitProperties()
{
    Super::PostInitProperties();

    // Initialize attack variables
    AttackCooldown = 1.0f;
	bCanAttack = true;
	AttackCooldown = 1.0f;
}

void AAtlantisEonsCharacter::SetInventoryToggleLock(bool bLock, float UnlockDelay)
{
    bInventoryToggleLocked = bLock;
    
    if (bLock && UnlockDelay > 0.0f)
    {
        // Clear any existing timers first
        GetWorld()->GetTimerManager().ClearTimer(InventoryToggleLockTimer);
        
        // Set a timer to unlock after the specified delay
        GetWorld()->GetTimerManager().SetTimer(
            InventoryToggleLockTimer,
            FTimerDelegate::CreateUObject(this, &AAtlantisEonsCharacter::SetInventoryToggleLock, false, 0.0f),
            UnlockDelay,
            false
        );
        
        UE_LOG(LogTemp, Warning, TEXT("Inventory toggle locked for %f seconds"), UnlockDelay);
    }
    else if (!bLock)
    {
        UE_LOG(LogTemp, Warning, TEXT("Inventory toggle unlocked"));
    }
}

void AAtlantisEonsCharacter::ForceSetInventoryState(bool bNewIsOpen)
{
    bIsInventoryOpen = bNewIsOpen;
    UE_LOG(LogTemp, Warning, TEXT("Character inventory state FORCE SET to: %s"), bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));
}

void AAtlantisEonsCharacter::BeginPlay()
{
    // Call the base class  
    Super::BeginPlay();
    
    // Initialize inventory system
    bIsInventoryOpen = false;
    bInventoryToggleLocked = false;

    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PlayerController->Player))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                UE_LOG(LogTemp, Warning, TEXT("Character - Setting up input mapping context in BeginPlay"));
                Subsystem->ClearAllMappings();
                if (DefaultMappingContext)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Character - Adding default mapping context"));
                    Subsystem->AddMappingContext(DefaultMappingContext, 0);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Character - DefaultMappingContext is null!"));
                }
            }
        }
    }

    // Debug log team ID
    UE_LOG(LogTemp, Warning, TEXT("Player Character Team ID: %d"), GetGenericTeamId().GetId());

    // Debug stimuli source
    if (AIPerceptionStimuliSourceComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player StimuliSource Status:"));
        UE_LOG(LogTemp, Warning, TEXT("  - Is Registered: %s"), AIPerceptionStimuliSourceComponent->IsRegistered() ? TEXT("true") : TEXT("false"));
        UE_LOG(LogTemp, Warning, TEXT("  - Auto Register: %s"), AIPerceptionStimuliSourceComponent->bAutoRegister ? TEXT("true") : TEXT("false"));
    }
}

void AAtlantisEonsCharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();

    // Set up input mapping context when controller changes
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            UE_LOG(LogTemp, Warning, TEXT("Character - Setting up input mapping context after controller change"));
            Subsystem->ClearAllMappings();
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void AAtlantisEonsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UE_LOG(LogTemp, Warning, TEXT("Character - Setting up input bindings"));

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("Character - Successfully got Enhanced Input Component"));

        // Moving
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAtlantisEonsCharacter::Move);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Move action"));
        }

        // Looking
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAtlantisEonsCharacter::Look);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Look action"));
        }

        // Jumping
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAtlantisEonsCharacter::StopJumping);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Jump action"));
        }

        // Dodging
        if (DodgeAction)
        {
            EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::Dodge);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Dodge action"));
        }

        // Debug damage
        if (DebugDamageAction)
        {
            EnhancedInputComponent->BindAction(DebugDamageAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::DebugDamage);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Debug Damage action"));
        }

        // Melee attack
        if (MeleeAttackAction)
        {
            EnhancedInputComponent->BindAction(MeleeAttackAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::MeleeAttack);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Melee Attack action"));
        }

        // Inventory
        // Only bind the Inventory action once - to ToggleInventory
        if (InventoryAction)
        {
            // This binds the 'I' key to toggle inventory (will handle both opening AND closing)
            EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Triggered, this, &AAtlantisEonsCharacter::ToggleInventory);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Inventory action to toggle (open/close)"));
        }

        // Resume/ESC action
        if (ResumeAction)
        {
            EnhancedInputComponent->BindAction(ResumeAction, ETriggerEvent::Triggered, this, &AAtlantisEonsCharacter::CloseInventoryIfOpen);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Resume action to close inventory"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to cast to Enhanced Input Component"), *GetName());
    }
}

void AAtlantisEonsCharacter::Move(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // get forward vector
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    
        // get right vector 
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // add movement 
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AAtlantisEonsCharacter::Look(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // Apply sensitivity multipliers to make camera less slippery
        const float AdjustedYawInput = LookAxisVector.X * CameraYawSensitivity;
        const float AdjustedPitchInput = LookAxisVector.Y * CameraPitchSensitivity;
        
        // Debug logging for camera input adjustment
        UE_LOG(LogTemp, VeryVerbose, TEXT("Camera input adjusted: Original (X: %f, Y: %f), Adjusted (X: %f, Y: %f)"), 
            LookAxisVector.X, LookAxisVector.Y, AdjustedYawInput, AdjustedPitchInput);
        
        // add yaw and pitch input to controller with sensitivity adjustment
        AddControllerYawInput(AdjustedYawInput);
        AddControllerPitchInput(AdjustedPitchInput);
    }
}

void AAtlantisEonsCharacter::Dodge(const FInputActionValue& Value)
{
    if (!bCanDodge || !GetCharacterMovement()->IsMovingOnGround())
    {
        return;
    }

    // Get the last movement input direction
    FVector LastInputVector = GetCharacterMovement()->GetLastInputVector();
    if (!LastInputVector.IsNearlyZero())
    {
        // Normalize and scale by dodge distance
        LastInputVector = LastInputVector.GetSafeNormal() * DodgeDistance;

        // Adjust camera lag for dodge
        if (CameraBoom)
        {
            CameraBoom->CameraLagSpeed = DodgeCameraLag;
            CameraBoom->CameraRotationLagSpeed = DodgeCameraRotationLag;
        }

        // Spawn dodge trail effect
        if (DodgeTrailEffect)
        {
            UGameplayStatics::SpawnEmitterAttached(
                DodgeTrailEffect,
                GetMesh(),
                NAME_None,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                FVector(1.0f),
                EAttachLocation::SnapToTarget,
                true,
                EPSCPoolMethod::AutoRelease
            );
        }

        // Launch character
        LaunchCharacter(LastInputVector, true, false);

        // Set dodge cooldown
        bCanDodge = false;
        GetWorld()->GetTimerManager().SetTimer(DodgeCooldownTimer, this, &AAtlantisEonsCharacter::ResetDodge, DodgeCooldown, false);

        // Set invulnerability
        bIsInvulnerable = true;
        GetWorld()->GetTimerManager().SetTimer(InvulnerabilityTimer, this, &AAtlantisEonsCharacter::ResetInvulnerability, DodgeInvulnerabilityDuration, false);

        // Apply invulnerability effects
        ApplyInvulnerabilityEffects();

        // Play dodge animation montage
        PlayAnimMontage(TwinSword_Dodge_F_Montage);

        // Reset camera lag after dodge animation
        FTimerHandle ResetCameraHandle;
        GetWorld()->GetTimerManager().SetTimer(ResetCameraHandle, [this]()
        {
            if (CameraBoom)
            {
                CameraBoom->CameraLagSpeed = DefaultCameraLag;
                CameraBoom->CameraRotationLagSpeed = DefaultCameraRotationLag;
            }
        }, DodgeInvulnerabilityDuration, false);
    }
}

void AAtlantisEonsCharacter::ResetDodge()
{
    bCanDodge = true;
}

void AAtlantisEonsCharacter::ResetInvulnerability()
{
    bIsInvulnerable = false;
    RemoveInvulnerabilityEffects();
}

void AAtlantisEonsCharacter::StoreOriginalMaterials()
{
    if (OriginalMaterials.Num() == 0 && GetMesh())
    {
        OriginalMaterials.Empty();
        TArray<UMaterialInterface*> Materials = GetMesh()->GetMaterials();
        OriginalMaterials.Append(Materials);
    }
}

void AAtlantisEonsCharacter::ApplyInvulnerabilityEffects()
{
    // Store original materials if not already stored
    StoreOriginalMaterials();

    // Apply invulnerability material if available
    if (InvulnerabilityMaterial && GetMesh())
    {
        for (int32 i = 0; i < GetMesh()->GetNumMaterials(); i++)
        {
            GetMesh()->SetMaterial(i, InvulnerabilityMaterial);
        }
    }

    // Spawn invulnerability particle effect
    if (InvulnerabilityEffect && !InvulnerabilityPSC)
    {
        InvulnerabilityPSC = UGameplayStatics::SpawnEmitterAttached(
            InvulnerabilityEffect,
            GetMesh(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            FVector(1.0f),
            EAttachLocation::SnapToTarget,
            true
        );
    }
}

void AAtlantisEonsCharacter::RemoveInvulnerabilityEffects()
{
    // Restore original materials
    if (GetMesh() && OriginalMaterials.Num() > 0)
    {
        for (int32 i = 0; i < OriginalMaterials.Num(); i++)
        {
            if (OriginalMaterials[i])
            {
                GetMesh()->SetMaterial(i, OriginalMaterials[i]);
            }
        }
    }

    // Remove invulnerability particle effect
    if (InvulnerabilityPSC)
    {
        InvulnerabilityPSC->DestroyComponent();
        InvulnerabilityPSC = nullptr;
    }
}

void AAtlantisEonsCharacter::ApplyDamage(float InDamageAmount)
{
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyDamage: Character is already dead, ignoring damage"));
        return;
    }

    if (InDamageAmount > 0.0f && !bIsDead)
    {
        PlayHitReactMontage();
    }

    if (CurrentHealth > 0.0f)
    {
        CurrentHealth = FMath::Max(CurrentHealth - InDamageAmount, 0.0f);
        UE_LOG(LogTemp, Warning, TEXT("Damage applied: New Health = %f, Max Health = %f"), CurrentHealth, MaxHealth);

        UE_LOG(LogTemp, Warning, TEXT("Health changed delegate broadcasted."));
        
        // Spawn damage number
        ADamageNumberSystem* DamageSystem = ADamageNumberSystem::GetInstance(GetWorld());
        if (DamageSystem)
        {
            DamageSystem->SpawnDamageNumber(this, InDamageAmount, true); // true for player damage
        }
    }
    
    // Check if health is zero or less and character is not already dead
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character health reached zero, calling HandleDeath. CurrentHealth: %f, bIsDead: %d"), 
            CurrentHealth, bIsDead ? 1 : 0);
        HandleDeath();
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath call completed"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("ApplyDamage called: DamageAmount=%f, NewHealth=%f"), InDamageAmount, CurrentHealth);
}

void AAtlantisEonsCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Function entered. bIsDead: %d"), bIsDead ? 1 : 0);
    
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Character is already dead, returning early"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Setting bIsDead to true and disabling movement"));
    bIsDead = true;

    // Disable movement
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // Disable input
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Disabled player input"));
    }

    // Play death animation if available
    if (DeathMontage)
    {
        PlayAnimMontage(DeathMontage);
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Playing death animation"));
    }

    // Log death
    UE_LOG(LogTemplateCharacter, Log, TEXT("Character has died"));

    // Start respawn timer
    UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Starting respawn timer for %f seconds"), RespawnDelay);
    GetWorld()->GetTimerManager().SetTimer(
        RespawnTimerHandle,
        this,
        &AAtlantisEonsCharacter::ResetCharacter,
        RespawnDelay,
        false
    );
}

void AAtlantisEonsCharacter::ResetCharacter()
{
    // Reset health
    CurrentHealth = MaxHealth;
    bIsDead = false;

    // Stop any ongoing animations
    StopAnimMontage(DeathMontage);

    // Re-enable movement
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    // Re-enable input
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        EnableInput(PC);
    }

    // Respawn at appropriate location
    RespawnCharacter();

    // Log respawn
    UE_LOG(LogTemplateCharacter, Log, TEXT("Character has respawned with full health"));


}

void AAtlantisEonsCharacter::RespawnCharacter()
{
    // Find a player start to respawn at
    AActor* StartSpot = nullptr;
    APlayerController* PC = Cast<APlayerController>(GetController());

    if (PC)
    {
        // Try to find a Player Start actor
        TArray<AActor*> PlayerStarts;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

        if (PlayerStarts.Num() > 0)
        {
            // Use the first player start found
            StartSpot = PlayerStarts[0];
        }
    }

    if (StartSpot)
    {
        // Teleport to the start spot
        SetActorLocation(StartSpot->GetActorLocation());
        SetActorRotation(StartSpot->GetActorRotation());
    }
    else
    {
        // If no player start is found, just reset to origin
        SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
        SetActorRotation(FRotator(0.0f));
        UE_LOG(LogTemplateCharacter, Warning, TEXT("No PlayerStart found - respawning at origin"));
    }
}

void AAtlantisEonsCharacter::MeleeAttack(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("MeleeAttack called. Can attack: %s"), bCanAttack ? TEXT("Yes") : TEXT("No"));
    
    if (!bCanAttack)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot attack - on cooldown"));
        return;
    }
    
    if (bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot attack - character is dead"));
        return;
    }
    
    // Check if we have a valid mesh and animation instance
    if (!GetMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("Character mesh is null!"));
        return;
    }
    
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("Animation instance is null!"));
        return;
    }
    
    bCanAttack = false;
    bIsAttacking = true;
    
    FaceNearestEnemy(); // Snap to face nearest enemy
    
    // Debug montage information
    if (MeleeAttackMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("MeleeAttackMontage asset found: %s"), *MeleeAttackMontage->GetName());
        UE_LOG(LogTemp, Warning, TEXT("MeleeAttackMontage duration: %f"), MeleeAttackMontage->GetPlayLength());
        
        // Try to play the montage
        float MontageLength = PlayAnimMontage(MeleeAttackMontage);
        
        if (MontageLength > 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Successfully playing attack montage with duration: %f"), MontageLength);
            UE_LOG(LogTemp, Warning, TEXT("Current animation state: IsPlaying=%d"), AnimInstance->IsAnyMontagePlaying());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to play attack montage. Current anim mode: %d"), GetMesh()->GetAnimationMode());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MeleeAttackMontage is null! Please assign in Blueprint."));
    }
    
    // Set timer for cooldown reset
    GetWorld()->GetTimerManager().SetTimer(AttackCooldownTimer, this, &AAtlantisEonsCharacter::ResetAttack, 0.5f, false);
    UE_LOG(LogTemp, Warning, TEXT("Attack cooldown timer set for 0.5 seconds"));
}

void AAtlantisEonsCharacter::ResetAttack()
{
    bCanAttack = true;
    bIsAttacking = false;
    UE_LOG(LogTemp, Warning, TEXT("Attack cooldown reset. Can attack again."));
}

void AAtlantisEonsCharacter::FaceNearestEnemy()
{
    // Method 1: Try to find enemies with tag
    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("AdvancedZombieEnemy"), Enemies);
    UE_LOG(LogTemp, Warning, TEXT("Searching for enemies with tag 'AdvancedZombieEnemy'. Found %d actors"), Enemies.Num());
    
    // Method 2: If no enemies found with tag, try to find all zombies by class name
    if (Enemies.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No enemies found with tag, looking for ZombieCharacter by class"));
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
        
        for (AActor* Actor : AllActors)
        {
            if (Actor && IsValid(Actor) && Actor->GetClass()->GetName().Contains("Zombie"))
            {
                Enemies.Add(Actor);
                UE_LOG(LogTemp, Warning, TEXT("Found zombie by class name: %s"), *Actor->GetName());
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Found %d zombies by class name"), Enemies.Num());
    }
    
    // Method 3: If still no enemies found, look for any actor with different team ID
    if (Enemies.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No enemies found by class, looking for actors with different team ID"));
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
        
        FGenericTeamId MyTeamId = GetGenericTeamId();
        
        for (AActor* Actor : AllActors)
        {
            if (Actor && IsValid(Actor) && Actor != this)
            {
                IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
                if (TeamAgent && TeamAgent->GetGenericTeamId() != MyTeamId)
                {
                    Enemies.Add(Actor);
                    UE_LOG(LogTemp, Warning, TEXT("Found enemy by team ID: %s"), *Actor->GetName());
                }
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Found %d enemies by team ID"), Enemies.Num());
    }
    
    // Process found enemies
    if (Enemies.Num() > 0)
    {
        AActor* NearestEnemy = nullptr;
        float MinDistance = TNumericLimits<float>::Max();
        FVector MyLocation = GetActorLocation();
        
        for (auto Enemy : Enemies)
        {
            if (!Enemy || !IsValid(Enemy)) continue;
            
            float Distance = FVector::Dist(MyLocation, Enemy->GetActorLocation());
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                NearestEnemy = Enemy;
            }
        }
        
        if (NearestEnemy)
        {
            FVector Direction = (NearestEnemy->GetActorLocation() - MyLocation).GetSafeNormal();
            FRotator NewRotation = Direction.Rotation();
            SetActorRotation(NewRotation); // Rotate the actor directly
            UE_LOG(LogTemp, Warning, TEXT("Set actor rotation to face nearest enemy at distance %f"), MinDistance);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No enemies found by any method"));
    }
}

float AAtlantisEonsCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (!bIsInvulnerable)  // Only apply damage if not invulnerable
    {
        ApplyDamage(ActualDamage);
        UE_LOG(LogTemplateCharacter, Warning, TEXT("Character took %.1f damage from %s"), ActualDamage, *DamageCauser->GetName());
    }
    else
    {
        UE_LOG(LogTemplateCharacter, Warning, TEXT("Character is invulnerable, ignored %.1f damage from %s"), ActualDamage, *DamageCauser->GetName());
    }

    return ActualDamage;
}

void AAtlantisEonsCharacter::OnMeleeAttackNotify()
{
    UE_LOG(LogTemp, Warning, TEXT("OnMeleeAttackNotify called"));
    
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("Character is dead, skipping attack"));
        return;
    }
    
    // Get character location and forward vector
    FVector StartLocation = GetActorLocation();
    FVector ForwardVector = GetActorForwardVector();
    
    // Adjust start position to be slightly in front of character and at appropriate height
    StartLocation += ForwardVector * 50.0f; // Move forward slightly
    StartLocation.Z += 50.0f; // Adjust height to be more appropriate for attacks
    
    // Calculate end position with increased range
    FVector EndLocation = StartLocation + ForwardVector * 200.0f; // Increased attack range to 200 cm for better detection
    
    // Set up collision query with improved parameters
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.bTraceComplex = false; // Simple collision for better performance and reliability
    QueryParams.bReturnPhysicalMaterial = false;
    
    // Draw debug visuals to help with debugging
    DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Blue, false, 3.0f, 0, 3.0f);
    DrawDebugSphere(GetWorld(), StartLocation, 100.0f, 8, FColor::Blue, false, 3.0f, 0, 0.5f);
    
    // Multi-method enemy detection approach
    TArray<AActor*> PotentialEnemies;
    
    // Method 1: Try collision sweep using several different channels for maximum coverage
    TArray<FHitResult> HitResults;
    GetWorld()->SweepMultiByChannel(HitResults, StartLocation, EndLocation, FQuat::Identity, 
                                  ECC_Pawn, FCollisionShape::MakeSphere(100.0f), QueryParams);
    
    // Process hit results from the sweep
    for (auto& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor != this && !PotentialEnemies.Contains(HitActor))
        {
            PotentialEnemies.Add(HitActor);
        }
    }
    
    // Method 2: Try using overlap test as an alternative detection method
    if (PotentialEnemies.Num() == 0)
    {
        TArray<FOverlapResult> OverlapResults;
        FCollisionShape SphereShape = FCollisionShape::MakeSphere(150.0f);
        FCollisionQueryParams OverlapParams;
        OverlapParams.AddIgnoredActor(this);
        
        GetWorld()->OverlapMultiByChannel(
            OverlapResults,
            StartLocation,
            FQuat::Identity,
            ECC_Pawn,
            SphereShape,
            OverlapParams
        );
        
        for (auto& Overlap : OverlapResults)
        {
            AActor* OverlapActor = Overlap.GetActor();
            if (OverlapActor && OverlapActor != this && !PotentialEnemies.Contains(OverlapActor))
            {
                PotentialEnemies.Add(OverlapActor);
            }
        }
    }
    
    // Method 3: Last resort - find all nearby actors if previous methods failed
    if (PotentialEnemies.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No enemies found by collision tests, looking for nearby actors"));
        TArray<AActor*> NearbyActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), NearbyActors);
        
        for (AActor* Actor : NearbyActors)
        {
            if (Actor && Actor != this)
            {
                float Distance = FVector::Dist(StartLocation, Actor->GetActorLocation());
                if (Distance < 300.0f) // Generous distance for detection
                {
                    PotentialEnemies.AddUnique(Actor);
                }
            }
        }
    }
    
    // Process all potential enemies
    bool bHitEnemy = false;
    FGenericTeamId MyTeamId = GetGenericTeamId();
    
    UE_LOG(LogTemp, Warning, TEXT("Found %d potential targets to process"), PotentialEnemies.Num());
    
    for (AActor* Actor : PotentialEnemies)
    {
        if (!Actor || !IsValid(Actor)) continue;
        
        // Multiple ways to detect if this is a valid enemy
        bool bIsEnemy = false;
        
        // Check 1: Class name contains "Zombie"
        if (Actor->GetClass()->GetName().Contains("Zombie"))
        {
            bIsEnemy = true;
            UE_LOG(LogTemp, Warning, TEXT("Enemy detected by class name: %s"), *Actor->GetClass()->GetName());
        }
        
        // Check 2: Actor has the enemy tag
        else if (Actor->ActorHasTag(TEXT("AdvancedZombieEnemy")) || Actor->ActorHasTag(TEXT("Enemy")))
        {
            bIsEnemy = true;
            UE_LOG(LogTemp, Warning, TEXT("Enemy detected by tag"));
        }
        
        // Check 3: Team ID is different
        else 
        {
            IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
            if (TeamAgent && TeamAgent->GetGenericTeamId() != MyTeamId)
            {
                bIsEnemy = true;
                UE_LOG(LogTemp, Warning, TEXT("Enemy detected by team ID"));
            }
        }
        
        // Apply damage if we found an enemy
        if (bIsEnemy)
        {
            float DamageAmount = 50.0f;
            
            // Calculate the actual hit location on the zombie
            FVector HitLocation;
            
            // Use the line trace hit result for a more precise location if available
            // or calculate a reasonable approximation if not
            FVector ActorCenter = Actor->GetActorLocation();
            FVector DirectionToActor = (ActorCenter - GetActorLocation()).GetSafeNormal();
            
            // Get the dimensions of the enemy to calculate a more precise hit point
            FVector ActorExtent = Actor->GetComponentsBoundingBox().GetExtent();
            
            // Calculate front hit point - move from center toward player by the extent of the enemy in that direction
            FVector FrontHitPoint = ActorCenter - (DirectionToActor * ActorExtent.Size() * 0.5f);
            
            // Add slight random offset for better visual effect (hits don't always land in the exact same spot)
            FVector RandomOffset(FMath::RandRange(-10.0f, 10.0f), FMath::RandRange(-10.0f, 10.0f), FMath::RandRange(-10.0f, 20.0f));
            HitLocation = FrontHitPoint + RandomOffset;
            
            // Draw debug visualization of hit point
            DrawDebugSphere(GetWorld(), HitLocation, 10.0f, 8, FColor::Red, false, 3.0f, 0, 2.0f);
            
            // Store the hit location in a custom damage event to pass it to the zombie
            FPointDamageEvent PointDamage;
            PointDamage.Damage = DamageAmount;
            PointDamage.HitInfo.ImpactPoint = HitLocation;
            
            // Add a custom tag to the point damage event to indicate this is from the player's melee attack
            // This will allow the zombie to know this is a player melee attack and handle damage numbers accordingly
            FPointDamageEvent CustomPointDamage;
            CustomPointDamage.Damage = DamageAmount;
            CustomPointDamage.HitInfo.ImpactPoint = HitLocation;
            
            // The zombie's TakeDamage_Implementation will handle spawning damage numbers, so we just need to apply damage
            Actor->TakeDamage(DamageAmount, CustomPointDamage, GetController(), this);
            
            UE_LOG(LogTemp, Warning, TEXT("Hit enemy %s with melee attack at X=%.1f Y=%.1f Z=%.1f, applied %f damage"), 
                  *Actor->GetName(), HitLocation.X, HitLocation.Y, HitLocation.Z, DamageAmount);
            
            bHitEnemy = true;
            DrawDebugSphere(GetWorld(), Actor->GetActorLocation(), 30.0f, 8, FColor::Green, false, 3.0f, 0, 1.0f);
        }
    }
    
    // Visual feedback for hit/miss
    if (!bHitEnemy)
    {
        UE_LOG(LogTemp, Warning, TEXT("Melee attack missed - found no valid targets"));
        DrawDebugSphere(GetWorld(), EndLocation, 75.0f, 12, FColor::Red, false, 3.0f, 0, 1.0f); // Red sphere for miss
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Melee attack hit at least one enemy!"));
        DrawDebugSphere(GetWorld(), StartLocation, 75.0f, 12, FColor::Green, false, 3.0f, 0, 1.0f); // Green sphere for hit
    }
}

void AAtlantisEonsCharacter::ApplyDebugDamage()
{
    ApplyDamage(10.0f); // Apply 10 damage for testing
}

void AAtlantisEonsCharacter::PlayHitReactMontage()
{
    if (HitReactMontage)
    {
        PlayAnimMontage(HitReactMontage);
    }
}

void AAtlantisEonsCharacter::ToggleInventory(const FInputActionValue& Value)
{
    // Check if toggle is locked
    if (bInventoryToggleLocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called but ignored due to toggle lock"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called. Current state: %s"), bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));
    
    // Get the HUD first
    AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
    if (!HUD)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get HUD in ToggleInventory"));
        return;
    }
    
    // Lock toggle to prevent rapid switching
    bInventoryToggleLocked = true;
    GetWorldTimerManager().SetTimer(InventoryToggleLockTimer, this, &AAtlantisEonsCharacter::UnlockInventoryToggle, 0.2f, false);
    
    // If inventory is open, close it
    if (bIsInventoryOpen)
    {
        if (HUD->IsInventoryWidgetVisible())
        {
            CloseInventory();
        }
        else
        {
            // Widget is not visible but state says it's open - fix the state
            bIsInventoryOpen = false;
        }
    }
    else
    {
        // Only try to open if it's not already visible
        if (!HUD->IsInventoryWidgetVisible())
        {
            OpenInventory();
        }
    }
}

void AAtlantisEonsCharacter::UnlockInventoryToggle()
{
    bInventoryToggleLocked = false;
}

void AAtlantisEonsCharacter::OpenInventory()
{
    if (!bIsInventoryOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("OpenInventory called. Current state: %s"), bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));
        
        // Get the HUD
        AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
        if (!HUD)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get HUD in OpenInventory"));
            return;
        }
        
        // Show the inventory widget
        if (HUD->ShowInventoryWidget())
        {
            bIsInventoryOpen = true;
            
            // Clear any existing input mappings
            if (APlayerController* PC = Cast<APlayerController>(Controller))
            {
                if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
                {
                    Subsystem->ClearAllMappings();
                    // Only keep the resume action
                    if (DefaultMappingContext)
                    {
                        Subsystem->AddMappingContext(DefaultMappingContext, 0);
                    }
                }
            }
            
            UE_LOG(LogTemp, Warning, TEXT("After toggle, inventory state: Open"));
        }
    }
}

void AAtlantisEonsCharacter::CloseInventory()
{
    if (bIsInventoryOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("CloseInventory - Restoring default mapping context"));
        
        // Get the HUD
        AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
        if (!HUD)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get HUD in CloseInventory"));
            return;
        }
        
        // Hide the inventory widget
        HUD->HideInventoryWidget();
        bIsInventoryOpen = false;
        
        // Restore all input mappings
        if (APlayerController* PC = Cast<APlayerController>(Controller))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            {
                Subsystem->ClearAllMappings();
                if (DefaultMappingContext)
                {
                    Subsystem->AddMappingContext(DefaultMappingContext, 0);
                    // Clear all mappings first
                    Subsystem->ClearAllMappings();
                    
                    // Then add back the default mapping context
                    if (DefaultMappingContext)
                    {
                        Subsystem->AddMappingContext(DefaultMappingContext, 0);
                    }
                }
            }
        }
        
        // Re-enable character movement
        if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
        {
            MovementComp->SetMovementMode(MOVE_Walking);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("====== INVENTORY CLOSE: CloseInventory COMPLETED ======"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get HUD"));
    }
}

void AAtlantisEonsCharacter::DebugDamage(const FInputActionValue& Value)
{
    ApplyDebugDamage();
}

void AAtlantisEonsCharacter::CloseInventoryIfOpen(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("CloseInventoryIfOpen called (ESC/Resume Key)"));
    
    // Only close if currently open
    if (bIsInventoryOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("CloseInventoryIfOpen - Inventory is open, closing it directly"));
        
        // Try direct HUD method first which is more robust
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (!PC && GetWorld())
        {
            PC = GetWorld()->GetFirstPlayerController();
        }
        
        if (PC)
        {
            AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(PC->GetHUD());
            if (HUD)
            {
                UE_LOG(LogTemp, Warning, TEXT("CloseInventoryIfOpen - Calling ForceCloseInventoryAndResume on HUD"));
                HUD->ForceCloseInventoryAndResume();
                return;
            }
        }
        
        // Fallback to direct close method
        UE_LOG(LogTemp, Warning, TEXT("CloseInventoryIfOpen - No HUD found, using direct close method"));
        CloseInventory();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CloseInventoryIfOpen - Inventory already closed, doing nothing"));
    }
}
