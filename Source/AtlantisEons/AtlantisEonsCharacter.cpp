// Copyright Epic Games, Inc. All Rights Reserved.

#include "AtlantisEonsCharacter.h"
#include "WBP_CharacterInfo.h"
#include "Engine/LocalPlayer.h"
#include "WBP_Main.h"
#include "ZombieCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/DamageEvents.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
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
#include "CollisionQueryParams.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "DashAfterimage.h"
#include "AtlantisEonsGameInstance.h"
#include "BP_ItemInfo.h"
#include "BP_Item.h"
#include "WBP_Inventory.h"
#include "BP_ConcreteItemInfo.h"
#include "WBP_ContextMenu.h"
#include "WBP_InventorySlot.h"
#include "WBP_Main.h"
#include "Blueprint/UserWidget.h"
#include "WBP_Store.h"
#include "Components/VerticalBox.h"
#include "WBP_StorePopup.h"
#include "Components/StaticMeshComponent.h"
#include "BP_Item.h"
#include "StoreSystemFix.h"
#include "UniversalItemLoader.h"
#include "Engine/OverlapResult.h"
#include "WBP_SwordBloom.h"
#include "AtlantisEonsGameMode.h"
#include "InventoryComponent.h"
#include "CharacterStatsComponent.h"
#include "EquipmentComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AAtlantisEonsCharacter

AAtlantisEonsCharacter::AAtlantisEonsCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Initialize gold amount - increased for testing store purchases
    YourGold = 100000000;  // 100 million gold for testing
    Gold = YourGold; // Ensure both Gold properties start with the same value

    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    
    // Set up the skeletal mesh component transforms (mesh asset should be set in Blueprint)
    if (GetMesh())
    {
        // Set the mesh location and rotation - this is appropriate to do in C++
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
        GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        
        // Note: Skeletal Mesh Asset should be assigned in BP_Character Blueprint
        UE_LOG(LogTemp, Warning, TEXT("Character Constructor: Mesh transforms configured. Mesh asset should be set in Blueprint."));
    }
    
    // ENHANCED: Configure collision to prevent physics impulse issues and BLOCK other pawns STRONGLY
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block); // Block by default
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // COMBAT FIX: Block pawns but prevent bouncing with physics constraints
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Block visibility traces for proper line of sight
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // Block world geometry
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // Block dynamic objects
    
    // ENHANCED: Stronger physics prevention with higher mass
    GetCapsuleComponent()->SetSimulatePhysics(false);
    GetCapsuleComponent()->SetEnableGravity(false); // Let character movement handle gravity
    GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, 500.0f, true); // INCREASED: Much heavier to resist pushing
    GetCapsuleComponent()->SetLinearDamping(20.0f); // INCREASED: Much higher damping
    GetCapsuleComponent()->SetAngularDamping(20.0f); // INCREASED: Much higher rotational damping
    GetCapsuleComponent()->SetUseCCD(false); // Disable continuous collision detection
    GetCapsuleComponent()->SetNotifyRigidBodyCollision(false); // Disable collision events to reduce overhead
    
    // ENHANCED: Disable physics simulation to prevent flying when hit
    GetCapsuleComponent()->SetSimulatePhysics(false);
    GetCapsuleComponent()->SetEnableGravity(false); // Let character movement handle gravity
    
    // ENHANCED: Configure mesh to not simulate physics
    if (GetMesh())
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        
        // NOTE: Keep mesh settings minimal to avoid rendering interference
    }

    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 900.0f; // 1.5x increased distance for a balanced battlefield view
    CameraBoom->bUsePawnControlRotation = true; // ENABLE: Player can control camera
    
    // CAMERA COLLISION FIX: Improved collision settings to prevent camera going inside character
    CameraBoom->bDoCollisionTest = false; // DISABLE: Don't pull camera closer when objects are in the way
    CameraBoom->ProbeSize = 8.0f; // Smaller probe size for better collision detection
    CameraBoom->ProbeChannel = ECC_Camera; // Use proper camera collision channel
    CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 60.0f); // Offset target above character center
    CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 0.0f); // No socket offset
    
    // CAMERA RESPONSIVENESS: Restore original fast and responsive camera settings
    CameraBoom->bEnableCameraLag = true; // ENABLE: Keep lag for smoothness
    CameraBoom->bEnableCameraRotationLag = true; // ENABLE: Keep rotation lag for smoothness
    CameraBoom->CameraLagSpeed = 30.0f; // FAST: Original responsive speed restored
    CameraBoom->CameraRotationLagSpeed = 25.0f; // FAST: Original responsive rotation speed restored
    CameraBoom->CameraLagMaxDistance = 100.0f; // Reasonable max distance
    CameraBoom->bUseCameraLagSubstepping = false; 
    CameraBoom->bDrawDebugLagMarkers = false; // Clean visuals

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false; // Correct: Spring arm handles rotation

    // Create equipment components with proper socket attachments
    Helmet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Helmet"));
    Helmet->SetupAttachment(GetMesh(), FName(TEXT("head")));
    Helmet->SetVisibility(false);

    Weapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));
    Weapon->SetupAttachment(GetMesh(), FName(TEXT("hand_r")));
    Weapon->SetVisibility(false);

    Shield = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
    Shield->SetupAttachment(GetMesh(), FName(TEXT("hand_l")));
    Shield->SetVisibility(false);

    // Initialize bloom window state
    bBloomWindowActive = false;

    // Don't rotate when the controller rotates
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // ENHANCED: Configure character movement to resist external forces MORE STRONGLY
    UCharacterMovementComponent* CharMov = GetCharacterMovement();
    if (CharMov)
    {
        CharMov->bOrientRotationToMovement = true;
        CharMov->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
        CharMov->JumpZVelocity = 700.f;
        CharMov->AirControl = 0.35f;
        CharMov->MaxWalkSpeed = 500.f;
        CharMov->MinAnalogWalkSpeed = 20.f;
        CharMov->BrakingDecelerationWalking = 2000.f;
        
        // ENHANCED: Much stronger physics resistance settings
        CharMov->bIgnoreBaseRotation = true;
        CharMov->Mass = 500.0f; // INCREASED: Much heavier mass to resist physics impulses and maintain position
        CharMov->GroundFriction = 15.0f; // INCREASED: Much higher friction to resist sliding and movement
        CharMov->MaxAcceleration = 3000.0f; // INCREASED: Higher acceleration for responsive movement
        CharMov->BrakingDecelerationWalking = 3000.0f; // INCREASED: Higher braking for stability
        CharMov->bApplyGravityWhileJumping = true; // Keep normal jumping
        CharMov->bCanWalkOffLedges = true; // Allow normal movement
        CharMov->bRequestedMoveUseAcceleration = true; // Use acceleration-based movement
        CharMov->bForceMaxAccel = false; // Controlled acceleration
        
        UE_LOG(LogTemp, Warning, TEXT("Player: ENHANCED STRONG movement physics - Mass: 500kg, High friction: 15.0"));
    }

    // Initialize character stats
    BaseHealth = 100.0f;
    CurrentHealth = BaseHealth;
    BaseMP = 100;
    CurrentMP = BaseMP;

    // Initialize combat stats
    BaseSTR = 10;
    BaseDEX = 10;
    BaseINT = 10;
    BaseDefence = 10;
    BaseDamage = 10;
    
    CurrentSTR = BaseSTR;
    CurrentDEX = BaseDEX;
    CurrentINT = BaseINT;
    CurrentDefence = BaseDefence;
    CurrentDamage = BaseDamage;

    // Initialize inventory
    InventoryItems.SetNum(30);
    EquipmentSlots.SetNum(10);
    InventoryToggleLock = false;
    InventoryToggleLockDuration = 0.5f;

    // Initialize combat flags
    bCanAttack = true;
    bIsAttacking = false;
    bIsInventoryOpen = false;
    AttackCooldown = 0.5f;
    DamageAmount = 20.0f;
    DamageSphereOffset = 100.0f;
    AttackRange = 200.0f;
    DefaultCameraLag = 15.0f;
    DefaultCameraRotationLag = 10.0f;

    // Initialize combo system
    CurrentAttackIndex = 0;
    MaxComboAttacks = 4;
    bIsInCombo = false;
    bHitCriticalWindow = false;
    ComboWindowDuration = 2.0f;

    // Create components
    StatsComponent = CreateDefaultSubobject<UCharacterStatsComponent>(TEXT("CharacterStatsComponent"));
    InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
    EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
    
    // Create SwordBloom widget component (restored to original approach)
    SwordBloomWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SwordBloomWidgetComponent"));
    SwordBloomWidgetComponent->SetupAttachment(RootComponent);
    SwordBloomWidgetComponent->SetRelativeLocation(FVector(100.0f, 0.0f, 0.0f)); // Position in front of character
    SwordBloomWidgetComponent->SetVisibility(true);
    SwordBloomWidgetComponent->SetHiddenInGame(false);

    // Initialize SwordEffect component reference (will be found by name from Blueprint)
    SwordEffectComponent = nullptr;

    // Initialize Dragon component references (will be found by name from Blueprint)
    Dragon1Component = nullptr;
    Dragon2Component = nullptr;
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
    // UE_LOG(LogTemp, Warning, TEXT("SetInventoryToggleLock called with bLock: %s, UnlockDelay: %f"),
    //     bLock ? TEXT("true") : TEXT("false"), UnlockDelay);

    bool bPrevLocked = bInventoryToggleLocked;
    bInventoryToggleLocked = bLock;

    // UE_LOG(LogTemp, Warning, TEXT("Inventory toggle lock changed from %s to %s"),
    //     bPrevLocked ? TEXT("Locked") : TEXT("Unlocked"),
    //     bInventoryToggleLocked ? TEXT("Locked") : TEXT("Unlocked"));

    if (UnlockDelay > 0.0f)
    {
        // UE_LOG(LogTemp, Warning, TEXT("Setting up unlock timer for %f seconds"), UnlockDelay);
        GetWorld()->GetTimerManager().ClearTimer(InventoryToggleLockTimer);
        GetWorld()->GetTimerManager().SetTimer(
            InventoryToggleLockTimer,
            FTimerDelegate::CreateWeakLambda(this, [this]() {
                if (IsValid(this) && !IsActorBeingDestroyed()) {
                    SetInventoryToggleLock(false, 0.0f);
                }
            }),
            UnlockDelay,
            false
        );
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("No unlock delay, state will remain: %s"),
        //     bInventoryToggleLocked ? TEXT("Locked") : TEXT("Unlocked"));
    }

    // Log current inventory state
    // UE_LOG(LogTemp, Warning, TEXT("Current inventory state - Open: %s, ToggleLocked: %s"),
    //     bIsInventoryOpen ? TEXT("true") : TEXT("false"),
    //     bInventoryToggleLocked ? TEXT("true") : TEXT("false"));
}

void AAtlantisEonsCharacter::ForceSetInventoryState(bool bNewIsOpen)
{
    // UE_LOG(LogTemp, Warning, TEXT("ForceSetInventoryState called with bNewIsOpen: %s"), bNewIsOpen ? TEXT("true") : TEXT("false"));
    
    bool bPrevState = bIsInventoryOpen;
    bIsInventoryOpen = bNewIsOpen;
    
    // UE_LOG(LogTemp, Warning, TEXT("Character inventory state changed from %s to %s"), 
    //     bPrevState ? TEXT("Open") : TEXT("Closed"),
    //     bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));

    // Log widget state
    if (InventoryWidget)
    {
        // UE_LOG(LogTemp, Warning, TEXT("InventoryWidget exists, visibility: %s"), 
        //     InventoryWidget->IsVisible() ? TEXT("Visible") : TEXT("Hidden"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryWidget is null!"));
    }
}

void AAtlantisEonsCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // Create and set up SwordBloom widget (restored to original approach)
    CreateSwordBloomWidget();
    
    // Find the SwordEffect Niagara component from Blueprint (backup call)
    FindSwordEffectComponent();
    
    // Find the Dragon skeletal mesh components from Blueprint (backup call)
    FindDragonComponents();
    
    // Initialize UI components
    InitializeUI();
    SetupCircularBars();

    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    // Initialize team ID for AI perception
    TeamId = FGenericTeamId(1); // Player team
    
    // ENHANCED: Ensure physics settings are properly configured at runtime to prevent flying
    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetSimulatePhysics(false);
        GetCapsuleComponent()->SetEnableGravity(false);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        
        // CRITICAL: MUCH STRONGER collision blocking to prevent overlapping
        GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // COMBAT FIX: Block pawns but prevent bouncing with physics constraints
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Block visibility traces
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // Block world geometry
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // Block dynamic objects
        
        // ENHANCED: Much stronger physics prevention settings
        GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, 500.0f, true); // INCREASED: Much heavier to resist pushing
        GetCapsuleComponent()->SetLinearDamping(20.0f); // INCREASED: Much higher damping to prevent movement
        GetCapsuleComponent()->SetAngularDamping(20.0f); // INCREASED: Much higher rotational damping
        GetCapsuleComponent()->SetUseCCD(false); // Disable continuous collision detection
        GetCapsuleComponent()->SetNotifyRigidBodyCollision(false); // Disable collision events to reduce overhead
        

        GetCapsuleComponent()->GetBodyInstance()->bOverrideMass = true; // Ensure mass override is applied
        GetCapsuleComponent()->GetBodyInstance()->SetMassOverride(500.0f); // Reinforce mass setting
        GetCapsuleComponent()->GetBodyInstance()->bLockZTranslation = true; // Lock Z-axis to prevent upward bouncing
        GetCapsuleComponent()->GetBodyInstance()->SetLinearVelocity(FVector::ZeroVector, false); // Clear any velocity
        GetCapsuleComponent()->GetBodyInstance()->SetAngularVelocityInRadians(FVector::ZeroVector, false); // Clear angular velocity
        
        // ENHANCED: Configure body instance for maximum collision blocking
        if (GetCapsuleComponent()->GetBodyInstance())
        {
            GetCapsuleComponent()->GetBodyInstance()->SetResponseToAllChannels(ECR_Block);
            GetCapsuleComponent()->GetBodyInstance()->SetResponseToChannel(ECC_Pawn, ECR_Block); // COMBAT FIX: Block pawns but prevent bouncing with physics constraints
            GetCapsuleComponent()->GetBodyInstance()->bLockXTranslation = false; // Allow controlled movement
            GetCapsuleComponent()->GetBodyInstance()->bLockYTranslation = false; // Allow controlled movement
            GetCapsuleComponent()->GetBodyInstance()->bLockZTranslation = true; // Lock Z to prevent flying
            GetCapsuleComponent()->GetBodyInstance()->bLockXRotation = true; // Lock rotations for stability
            GetCapsuleComponent()->GetBodyInstance()->bLockYRotation = true;
            GetCapsuleComponent()->GetBodyInstance()->bLockZRotation = false; // Allow yaw rotation only
        }
        
        UE_LOG(LogTemp, Warning, TEXT("Player: COMBAT COLLISION FIX - Player overlaps pawns to prevent bouncing with 500kg mass"));
    }
    
    if (GetMesh())
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // Support pawn collision queries
        
        // Ensure mesh visibility
        GetMesh()->SetVisibility(true);
        GetMesh()->SetHiddenInGame(false);
        
        UE_LOG(LogTemp, Warning, TEXT("Player: Mesh physics configured - SimulatePhysics: false, Visible: true"));
    }
    
    // ENHANCED: Much stronger character movement configuration
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->bIgnoreBaseRotation = true;
        GetCharacterMovement()->Mass = 500.0f; // INCREASED: Much heavier mass to resist physics impulses and maintain position
        GetCharacterMovement()->GroundFriction = 15.0f; // INCREASED: Much higher friction to resist sliding and movement
        GetCharacterMovement()->MaxAcceleration = 3000.0f; // INCREASED: Higher acceleration for responsive movement
        GetCharacterMovement()->BrakingDecelerationWalking = 3000.0f; // INCREASED: Higher braking for stability
        GetCharacterMovement()->bApplyGravityWhileJumping = true; // Keep normal jumping
        GetCharacterMovement()->bCanWalkOffLedges = true; // Allow normal movement
        GetCharacterMovement()->bRequestedMoveUseAcceleration = true; // Use acceleration-based movement
        GetCharacterMovement()->bForceMaxAccel = false; // Controlled acceleration
        
        UE_LOG(LogTemp, Warning, TEXT("Player: ENHANCED STRONG movement physics - Mass: 500kg, High friction: 15.0"));
    }
    
    // ENHANCED: Apply improved camera settings at runtime
    if (CameraBoom)
    {
        // CAMERA RESPONSIVENESS: Restore original fast and responsive camera settings
        CameraBoom->bEnableCameraLag = true; // ENABLE: Keep lag for smoothness
        CameraBoom->bEnableCameraRotationLag = true; // ENABLE: Keep rotation lag for smoothness
        CameraBoom->CameraLagSpeed = 30.0f; // FAST: Original responsive speed restored
        CameraBoom->CameraRotationLagSpeed = 25.0f; // FAST: Original responsive rotation speed restored
        CameraBoom->CameraLagMaxDistance = 100.0f; // Reasonable max distance
        CameraBoom->bUseCameraLagSubstepping = false; 
        
        // CAMERA COLLISION FIX: Prevent camera from going inside character
        CameraBoom->bDoCollisionTest = false; // DISABLE: Don't pull camera closer when objects are in the way
        CameraBoom->ProbeChannel = ECC_Camera; // Use camera collision channel
        CameraBoom->ProbeSize = 8.0f; // Smaller probe size for better collision detection
        CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 60.0f); // Offset camera target above character center
        
        // CRITICAL: Set minimum distance to prevent camera from going inside character
        CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 0.0f); // No socket offset

        // CAMERA FIX: Explicitly set up third-person camera
        CameraBoom->bUsePawnControlRotation = true; // Enable player camera control
        CameraBoom->TargetArmLength = 900.0f; // Good distance for third-person
        
        // CRITICAL: Reset any fixed rotation and allow free camera movement
        CameraBoom->SetRelativeRotation(FRotator::ZeroRotator); // Clear any fixed rotation
        CameraBoom->bInheritPitch = true; // Allow pitch control
        CameraBoom->bInheritYaw = true; // Allow yaw control
        CameraBoom->bInheritRoll = false; // Don't inherit roll to prevent camera rolling
        
        UE_LOG(LogTemp, Warning, TEXT("Player: THIRD-PERSON CAMERA SETUP - Free camera movement enabled"));
        UE_LOG(LogTemp, Warning, TEXT("Player: Camera Boom Rotation: %s"), *CameraBoom->GetRelativeRotation().ToString());
        UE_LOG(LogTemp, Warning, TEXT("Player: Camera collision fixed - ProbeSize: 8.0, TargetOffset: (0,0,60)"));

        UE_LOG(LogTemp, Warning, TEXT("Player: Camera responsiveness restored - Fast responsive controls enabled, character rotation during attacks disabled"));
    }
    
    // CAMERA FIX: Ensure controller rotation is properly set up for third-person view
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        // Set initial camera rotation to a reasonable third-person angle
        FRotator InitialCameraRotation = FRotator(-15.0f, 0.0f, 0.0f); // Slight downward angle
        PC->SetControlRotation(InitialCameraRotation);
        
        UE_LOG(LogTemp, Warning, TEXT("Player: Set initial camera rotation: %s"), *InitialCameraRotation.ToString());
    }
    
    // Store original materials for invulnerability effects
    StoreOriginalMaterials();

    // Initialize UI
    InitializeUI();

    // Ensure player starts without invulnerability
    bIsInvulnerable = false;
    bIsDead = false;
    
    // Initialize stats
    UpdateAllStats();
    
    UE_LOG(LogTemp, Warning, TEXT("Player BeginPlay: Invulnerable=%s, Dead=%s, Health=%.1f"), 
           bIsInvulnerable ? TEXT("Yes") : TEXT("No"),
           bIsDead ? TEXT("Yes") : TEXT("No"),
           CurrentHealth);
    
    UE_LOG(LogTemp, Warning, TEXT("Player BeginPlay: Character setup complete"));
}

// BeginPlay is now BlueprintImplementableEvent
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
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Character - MoveAction is null! Movement won't work"));
        }

        // Looking
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAtlantisEonsCharacter::Look);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Look action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Character - LookAction is null! Camera control won't work"));
        }

        // Jumping
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAtlantisEonsCharacter::StopJumping);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Jump action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Character - JumpAction is null! Jumping won't work"));
        }

        // Melee Attack
        if (MeleeAttackAction)
        {
            EnhancedInputComponent->BindAction(MeleeAttackAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::MeleeAttack);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound MeleeAttack action"));
        }

        // Pickup
        if (PickupAction)
        {
            EnhancedInputComponent->BindAction(PickupAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::OnPickupPressed);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Pickup action"));
        }
        else
        {
            // Critical warning about missing pickup action
            UE_LOG(LogTemp, Error, TEXT("Character - PickupAction is null! Pickup functionality won't work"));
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("WARNING: PickupAction not set in BP_Character!"));
        }

        // Inventory toggle
        if (InventoryAction)
        {
            EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::ToggleInventory);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Inventory action"));
        }

        // ESC key/Resume
        if (ResumeAction)
        {
            EnhancedInputComponent->BindAction(ResumeAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::CloseInventoryIfOpen);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Resume/ESC action"));
        }

        // Debug damage (for testing)
        if (DebugDamageAction)
        {
            EnhancedInputComponent->BindAction(DebugDamageAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::DebugDamage);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Debug Damage action"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Character - Failed to get Enhanced Input Component! No input will work"));
    }
}

void AAtlantisEonsCharacter::Move(const FInputActionValue& Value)
{
    // Input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // Find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // Get forward and right vectors
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // Add movement 
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AAtlantisEonsCharacter::Look(const FInputActionValue& Value)
{
    // Input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // Apply the input to the controller rotation
        AddControllerYawInput(LookAxisVector.X * CameraYawSensitivity);
        AddControllerPitchInput(LookAxisVector.Y * CameraPitchSensitivity);
    }
}

void AAtlantisEonsCharacter::Jump()
{
    Super::Jump();
}

void AAtlantisEonsCharacter::StopJumping()
{
    Super::StopJumping();
}

void AAtlantisEonsCharacter::InitializeUI()
{
    // DISABLED: Don't create any UI widgets at startup to ensure gameplay works first
    UE_LOG(LogTemplateCharacter, Warning, TEXT("%s: InitializeUI disabled to focus on gameplay first"), *GetName());
    
    // Connect inventory component to HUD when MainWidget is available
    ConnectInventoryToMainWidget();
    
    // Don't create the WBP_Main widget as it's causing cursor issues and blocking input
    /*
    // First check if an instance of WBP_Main already exists in the viewport
    if (!Main)
    {
        // Look for an existing instance of WBP_Main before creating a new one
        UWorld* World = GetWorld();
        if (World)
        {
            bool bFoundExistingWidget = false;
            for (TObjectIterator<UWBP_Main> Itr; Itr; ++Itr)
            {
                UWBP_Main* FoundWidget = *Itr;
                if (FoundWidget && FoundWidget->IsInViewport())
                {
                    UE_LOG(LogTemplateCharacter, Warning, TEXT("%s: Found existing WBP_Main widget in viewport, using it"), *GetName());
                    Main = FoundWidget;
                    bFoundExistingWidget = true;
                    break;
                }
            }
            
            // Create a new widget only if no existing widget was found
            if (!bFoundExistingWidget)
            {
                Main = CreateWidget<UWBP_Main>(World, UWBP_Main::StaticClass());
                if (Main)
                {
                    Main->AddToViewport();
                    UE_LOG(LogTemplateCharacter, Log, TEXT("%s: Successfully created and added Main widget to viewport"), *GetName());
                }
                else
                {
                    UE_LOG(LogTemplateCharacter, Error, TEXT("%s: Failed to create Main widget"), *GetName());
                    return;
                }
            }
        }
        else
        {
            UE_LOG(LogTemplateCharacter, Error, TEXT("%s: Failed to get World reference for widget creation"), *GetName());
            return;
        }
    }

    // The CharacterInfo widget is now created and managed by WBP_Main
    // Setup circular bars if Main widget exists
    if (Main)
    {
        SetupCircularBars();
    }
    */
}

void AAtlantisEonsCharacter::ConnectInventoryToMainWidget()
{
    // Try to get the HUD and its MainWidget
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(PC->GetHUD()))
        {
            UWBP_Main* MainWidget = HUD->GetMainWidget();
            if (MainWidget && InventoryComp)
            {
                InventoryComp->SetMainWidget(MainWidget);
                UE_LOG(LogTemp, Warning, TEXT("✅ Connected InventoryComponent to MainWidget"));
            }
            else if (!MainWidget)
            {
                UE_LOG(LogTemp, Warning, TEXT("⏳ MainWidget not yet available, will connect later"));
            }
        }
    }
}

void AAtlantisEonsCharacter::SetupCircularBars()
{
    if (Main)
    {
        // Initialize HP and MP bars
        SettingCircularBar_HP();
        SettingCircularBar_MP();
    }
}

void AAtlantisEonsCharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();
}

void AAtlantisEonsCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    
    UE_LOG(LogTemp, Warning, TEXT("PostInitializeComponents: Called"));
    
    // Find the SwordEffect Niagara component from Blueprint
    FindSwordEffectComponent();
    
    // Find the Dragon skeletal mesh components from Blueprint
    FindDragonComponents();
    
    // SwordBloom widget is now handled entirely in Blueprint
}

void AAtlantisEonsCharacter::RecoverHealth(int32 Amount)
{
    CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
    SettingCircularBar_HP();
}

void AAtlantisEonsCharacter::RecoverHP(int32 Amount)
{
    CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
    SettingCircularBar_HP();
}

void AAtlantisEonsCharacter::ApplyDamage(float InDamageAmount)
{
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyDamage: Character is already dead, ignoring damage"));
        return;
    }

    if (CurrentHealth > 0.0f)
    {
        CurrentHealth = FMath::Max(CurrentHealth - InDamageAmount, 0.0f);
        UE_LOG(LogTemp, Warning, TEXT("💔 Player: Health reduced from %.1f to %.1f (damage: %.1f)"), 
               CurrentHealth + InDamageAmount, CurrentHealth, InDamageAmount);

        // IMPROVED: Spawn damage number with better positioning and error handling
        if (IsValid(this) && GetWorld() && InDamageAmount > 0.0f)
        {
            // Get damage number location above player's head with more robust positioning
            FVector CharacterLocation = GetActorLocation();
            
            // Verify location is valid (not at origin)
            if (CharacterLocation.SizeSquared() < 1.0f)
            {
                // Fallback: try to get location from root component
                if (RootComponent)
                {
                    CharacterLocation = RootComponent->GetComponentLocation();
                }
                // Second fallback: try mesh location
                if (CharacterLocation.SizeSquared() < 1.0f && GetMesh())
                {
                    CharacterLocation = GetMesh()->GetComponentLocation();
                }
            }
            
            FVector DamageLocation = CharacterLocation + FVector(0, 0, 40); // FIXED: Reduced from 80 to 40 for closer positioning
            
            UE_LOG(LogTemp, Warning, TEXT("💥 Player: Character at %.1f,%.1f,%.1f, damage number at %.1f,%.1f,%.1f"), 
                   CharacterLocation.X, CharacterLocation.Y, CharacterLocation.Z,
                   DamageLocation.X, DamageLocation.Y, DamageLocation.Z);
            
            // Try to find damage number system
            ADamageNumberSystem* DamageSystem = nullptr;
            
            // Method 1: Try GetInstance
            DamageSystem = ADamageNumberSystem::GetInstance(GetWorld());
            
            // Method 2: Search for it if GetInstance failed
            if (!DamageSystem)
            {
                TArray<AActor*> FoundActors;
                UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADamageNumberSystem::StaticClass(), FoundActors);
                
                if (FoundActors.Num() > 0)
                {
                    DamageSystem = Cast<ADamageNumberSystem>(FoundActors[0]);
                }
            }
            
            if (DamageSystem && IsValid(DamageSystem))
            {
                // Use SpawnDamageNumberAtLocation for better positioning
                DamageSystem->SpawnDamageNumberAtLocation(this, DamageLocation, InDamageAmount, true);
                UE_LOG(LogTemp, Warning, TEXT("💥 Player: Spawned damage number for %.1f damage at %.1f,%.1f,%.1f"), 
                       InDamageAmount, DamageLocation.X, DamageLocation.Y, DamageLocation.Z);
                
                // Draw debug sphere to show where damage number spawned
                DrawDebugSphere(GetWorld(), DamageLocation, 15.0f, 8, FColor::Red, false, 5.0f, 0, 2.0f);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ Player: Could not find DamageNumberSystem for damage numbers!"));
            }
        }
        
        // Update UI with safety checks
        if (IsValid(this))
        {
            SettingCircularBar_HP();
        }
    }
    
    // Check if health is zero or less and character is not already dead
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("💀 Player: Health reached zero, calling HandleDeath. CurrentHealth: %f, bIsDead: %d"), 
            CurrentHealth, bIsDead ? 1 : 0);
        HandleDeath();
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath call completed"));
    }
    else if (InDamageAmount > 0.0f && !bIsDead)
    {
        // FIXED: Only play hit reaction if character survives the damage
        PlayHitReactMontage();
    }
    
    UE_LOG(LogTemp, Log, TEXT("Player ApplyDamage completed: DamageAmount=%f, NewHealth=%f"), InDamageAmount, CurrentHealth);
}

void AAtlantisEonsCharacter::SettingCircularBar_HP()
{
    // Early exit if we don't have a valid controller (prevents crashes during thumbnail generation)
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_HP: No valid PlayerController"));
        return;
    }
    
    // Try multiple approaches to update the HP bar
    bool bUpdated = false;
    
    // Method 1: Use WBP_CharacterInfo if available
    if (WBP_CharacterInfo)
    {
        WBP_CharacterInfo->UpdateHPBar();
        bUpdated = true;
        UE_LOG(LogTemp, Log, TEXT("Updated HP bar via WBP_CharacterInfo"));
    }
    
    // Method 2: Use Main widget if available
    if (Main)
    {
        // Update HP bar progress
        float HPPercentage = MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f;
        if (HPBar)
        {
            HPBar->SetPercent(HPPercentage);
            bUpdated = true;
            UE_LOG(LogTemp, Log, TEXT("Updated HP bar via Main widget: %.2f%%"), HPPercentage * 100.0f);
        }
    }
    
    if (!bUpdated)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_HP: Could not update HP bar - no valid UI references"));
    }
}

void AAtlantisEonsCharacter::SettingCircularBar_MP()
{
    // Early exit if we don't have a valid controller (prevents crashes during thumbnail generation)
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_MP: No valid PlayerController"));
        return;
    }
    
    // Try multiple approaches to update the MP bar
    bool bUpdated = false;
    
    // Method 1: Use WBP_CharacterInfo if available
    if (WBP_CharacterInfo)
    {
        float MPPercentage = MaxMP > 0 ? CurrentMP / (float)MaxMP : 0.0f;
        WBP_CharacterInfo->UpdateMPBar(MPPercentage);
        bUpdated = true;
        UE_LOG(LogTemp, Log, TEXT("Updated MP bar via WBP_CharacterInfo"));
    }
    
    // Method 2: Use Main widget if available
    if (Main)
    {
        // Update MP bar progress
        float MPPercentage = MaxMP > 0 ? CurrentMP / (float)MaxMP : 0.0f;
        if (MPBar)
        {
            MPBar->SetPercent(MPPercentage);
            bUpdated = true;
            UE_LOG(LogTemp, Log, TEXT("Updated MP bar via Main widget: %.2f%%"), MPPercentage * 100.0f);
        }
    }
    
    if (!bUpdated)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_MP: Could not update MP bar - no valid UI references"));
    }
}

void AAtlantisEonsCharacter::ResetInvulnerability()
{
    // Implementation will be in Blueprint
}

FGenericTeamId AAtlantisEonsCharacter::GetGenericTeamId() const
{
    return TeamId;
}

void AAtlantisEonsCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
    TeamId = NewTeamId;
}

void AAtlantisEonsCharacter::UpdateAllStats()
{
    // Start with base stats
    int32 TotalSTR = BaseSTR;
    int32 TotalDEX = BaseDEX;
    int32 TotalINT = BaseINT;
    int32 TotalDefence = BaseDefence;
    int32 TotalDamage = BaseDamage;
    float TotalHealth = BaseHealth;
    int32 TotalMP = BaseMP;
    
    // Add equipment bonuses from all equipped items
    for (int32 i = 0; i < EquipmentSlots.Num(); ++i)
    {
        if (EquipmentSlots[i])
        {
            bool bFound = false;
            FStructure_ItemInfo ItemData;
            EquipmentSlots[i]->GetItemTableRow(bFound, ItemData);
            
            if (bFound)
            {
                TotalSTR += ItemData.STR;
                TotalDEX += ItemData.DEX;
                TotalINT += ItemData.INT;
                TotalDefence += ItemData.Defence;
                TotalDamage += ItemData.Damage;
                TotalHealth += ItemData.HP;
                TotalMP += ItemData.MP;
                
                UE_LOG(LogTemp, Log, TEXT("Equipment bonus from '%s': STR+%d, DEX+%d, INT+%d, DEF+%d, DMG+%d, HP+%d, MP+%d"), 
                       *ItemData.ItemName, ItemData.STR, ItemData.DEX, ItemData.INT, 
                       ItemData.Defence, ItemData.Damage, ItemData.HP, ItemData.MP);
            }
        }
    }
    
    // Update current stats
    CurrentSTR = TotalSTR;
    CurrentDEX = TotalDEX;
    CurrentINT = TotalINT;
    CurrentDefence = TotalDefence;
    CurrentDamage = TotalDamage;
    
    // Update health and MP maximums
    float PreviousMaxHealth = MaxHealth;
    MaxHealth = TotalHealth;
    
    // Adjust current health proportionally if max health changed
    if (PreviousMaxHealth > 0.0f && MaxHealth != PreviousMaxHealth)
    {
        float HealthRatio = CurrentHealth / PreviousMaxHealth;
        CurrentHealth = FMath::Min(MaxHealth * HealthRatio, MaxHealth);
    }
    
    // Update MP maximum
    int32 PreviousMaxMP = MaxMP;
    MaxMP = TotalMP;
    
    // Adjust current MP proportionally if max MP changed
    if (PreviousMaxMP > 0 && MaxMP != PreviousMaxMP)
    {
        float MPRatio = static_cast<float>(CurrentMP) / static_cast<float>(PreviousMaxMP);
        CurrentMP = FMath::Min(FMath::RoundToInt(MaxMP * MPRatio), MaxMP);
    }
    
    // Clamp values to valid ranges
    CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
    CurrentMP = FMath::Clamp(CurrentMP, 0, MaxMP);
    
    // Broadcast stat changes
    OnStatsUpdated.Broadcast();
    
    // Update character info widget if it exists
    if (WBP_CharacterInfo)
    {
        WBP_CharacterInfo->UpdateAllStats();
    }
    
    // Force update circular bars to reflect new max values
    SettingCircularBar_HP();
    SettingCircularBar_MP();
    
    UE_LOG(LogTemp, Log, TEXT("Stats updated - STR: %d, DEX: %d, INT: %d, DEF: %d, DMG: %d, HP: %.1f/%.1f, MP: %d/%d"), 
           CurrentSTR, CurrentDEX, CurrentINT, CurrentDefence, CurrentDamage, CurrentHealth, MaxHealth, CurrentMP, MaxMP);
}

void AAtlantisEonsCharacter::RefreshStatsDisplay()
{
    // Force update the character info widget display
    if (WBP_CharacterInfo)
    {
        WBP_CharacterInfo->UpdateAllStats();
    }
    
    // Update circular bars
    SettingCircularBar_HP();
    SettingCircularBar_MP();
    
    UE_LOG(LogTemp, Log, TEXT("Stats display refreshed"));
}

void AAtlantisEonsCharacter::SettingStore()
{
    // Initialize store UI and data
    // TODO: Load store items and prices
}

void AAtlantisEonsCharacter::SetInventorySlot(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotWidgetRef)
{
    if (!ItemInfoRef || !InventorySlotWidgetRef) return;
    
    // Update the inventory slot widget with the item info
    InventorySlotWidgetRef->UpdateSlot(ItemInfoRef);
}

void AAtlantisEonsCharacter::OnPickupPressed()
{
    UE_LOG(LogTemp, Display, TEXT("%s: OnPickupPressed called"), *GetName());
    
    // Use a more reliable method to find items by using sphere trace
    FVector Start = GetActorLocation();
    FVector End = Start;
    float InteractionRadius = 200.0f; // Increased radius for easier item detection
    
    // Create debug sphere to visualize pickup range
    DrawDebugSphere(GetWorld(), Start, InteractionRadius, 12, FColor::Green, false, 2.0f);
    
    // Find all actors of BP_Item class within interaction range
    TArray<AActor*> FoundItems;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABP_Item::StaticClass(), FoundItems);
    
    ABP_Item* ClosestItem = nullptr;
    float ClosestDistance = InteractionRadius;
    
    for (AActor* Item : FoundItems)
    {
        float Distance = FVector::Distance(GetActorLocation(), Item->GetActorLocation());
        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestItem = Cast<ABP_Item>(Item);
        }
    }
    
    if (ClosestItem)
    {
        // Use the item's actual stack number, or default to 1 if it's 0
        int32 ActualStackNumber = FMath::Max(1, ClosestItem->StackNumber);
        
        UE_LOG(LogTemp, Display, TEXT("%s: Found closest item: %s (Index: %d, Stack: %d, Using: %d)"), 
            *GetName(), *ClosestItem->GetName(), ClosestItem->ItemIndex, ClosestItem->StackNumber, ActualStackNumber);
        
        // Add on-screen debug message to confirm item found
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
            FString::Printf(TEXT("Found item: %s"), *ClosestItem->ItemInfo.ItemName));
        
        // Pick up the item with its actual stack number
        bool bSuccess = PickingItem(ClosestItem->ItemIndex, ActualStackNumber);
        
        if (bSuccess)
        {
            UE_LOG(LogTemp, Display, TEXT("Successfully picked up item"));
            
            // Play pickup sound
            if (PickupSound)
            {
                UGameplayStatics::PlaySound2D(this, PickupSound);
            }
            else
            {
                // Use a generic sound if none is set
                USoundBase* DefaultSound = LoadObject<USoundBase>(nullptr, TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
                if (DefaultSound)
                {
                    UGameplayStatics::PlaySound2D(this, DefaultSound);
                }
            }
            
            // Add on-screen confirmation
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
                FString::Printf(TEXT("Added item to inventory")));
            
            // Destroy the item after pickup
            ClosestItem->Destroy();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to pick up item"));
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Failed to add item - inventory might be full"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: No nearby items found"), *GetName());
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("No items nearby"));
    }
}

bool AAtlantisEonsCharacter::PickingItem(int32 ItemIndex, int32 ItemStackNumber)
{
    if (InventoryComp)
    {
        return InventoryComp->AddItem(ItemIndex, ItemStackNumber);
    }
    UE_LOG(LogTemp, Warning, TEXT("PickingItem: InventoryComponent is null, functionality disabled"));
    return false;
}

void AAtlantisEonsCharacter::ContextMenuUse(UWBP_InventorySlot* InventorySlot)
{
    if (InventoryComp)
    {
        InventoryComp->ContextMenuUse(InventorySlot);
    }
}

void AAtlantisEonsCharacter::ContextMenuThrow(UWBP_InventorySlot* InventorySlot)
{
    if (InventoryComp)
    {
        InventoryComp->ContextMenuThrow(InventorySlot);
    }
}

void AAtlantisEonsCharacter::ContextMenuUse_EquipItem(UBP_ItemInfo* ItemInfoRef)
{
    if (InventoryComp)
    {
        InventoryComp->ContextMenuUse_EquipItem(ItemInfoRef);
    }
}

void AAtlantisEonsCharacter::ContextMenuUse_ConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (InventoryComp)
    {
        InventoryComp->ContextMenuUse_ConsumeItem(ItemInfoRef, InventorySlotRef, RecoverHP, RecoverMP, ItemType);
    }
}

bool AAtlantisEonsCharacter::BuyingItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice)
{
    if (InventoryComp)
    {
        return InventoryComp->BuyItem(ItemIndex, ItemStackNumber, ItemPrice);
    }
    return false;
}

void AAtlantisEonsCharacter::EquipItemInSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, const TSoftObjectPtr<UTexture2D>& Texture2D, int32 ItemIndex, UMaterialInterface* MaterialInterface, UMaterialInterface* MaterialInterface2)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->EquipItemInSlot(ItemEquipSlot, StaticMeshID, Texture2D, ItemIndex, MaterialInterface, MaterialInterface2);
    }
}

void AAtlantisEonsCharacter::ProcessEquipItem(UBP_ItemInfo* ItemInfoRef)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->ProcessEquipItem(ItemInfoRef);
    }
}

void AAtlantisEonsCharacter::ProcessConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (!ItemInfoRef || !InventorySlotRef) return;

    // Apply recovery effects
    if (ItemType == EItemType::Consume_HP)
    {
        RecoverHealth(RecoverHP);
    }
    else if (ItemType == EItemType::Consume_MP)
    {
        this->RecoverMP(RecoverMP);
    }

    // Clear the inventory slot
    InventorySlotRef->ClearSlot();
}

void AAtlantisEonsCharacter::DragAndDropExchangeItem(UBP_ItemInfo* FromInventoryItemRef, UWBP_InventorySlot* FromInventorySlotRef,
    UBP_ItemInfo* ToInventoryItemRef, UWBP_InventorySlot* ToInventorySlotRef)
{
    if (InventoryComp)
    {
        InventoryComp->HandleDragAndDrop(FromInventoryItemRef, FromInventorySlotRef, ToInventoryItemRef, ToInventorySlotRef);
    }
}

void AAtlantisEonsCharacter::RecoverMP(int32 Amount)
{
    CurrentMP = FMath::Clamp(CurrentMP + Amount, 0, MaxMP);
    SettingCircularBar_MP();
}

UBP_ItemInfo* AAtlantisEonsCharacter::GetInventoryItemRef() const
{
    // Create a new concrete item info instance
    UClass* ItemInfoClass = LoadClass<UBP_ConcreteItemInfo>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/BP_ConcreteItemInfo.BP_ConcreteItemInfo_C"));
    if (!ItemInfoClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to load BP_ConcreteItemInfo class"), *GetName());
        return nullptr;
    }
    
    UBP_ConcreteItemInfo* NewItemInfo = NewObject<UBP_ConcreteItemInfo>(const_cast<AAtlantisEonsCharacter*>(this), ItemInfoClass);
    if (!NewItemInfo)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to create BP_ConcreteItemInfo object"), *GetName());
        return nullptr;
    }
    
    return NewItemInfo;
}

AActor* AAtlantisEonsCharacter::FindClosestItem() const
{
    // Get all nearby items (BP_Item type actors)
    TArray<AActor*> FoundItems;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundItems);
    
    // Find the closest one within interaction range
    const float InteractionRange = 200.0f;
    AActor* ClosestItem = nullptr;
    float ClosestDistance = InteractionRange;
    
    for (AActor* Item : FoundItems)
    {
        // Skip non-interactable items (should be more specific in actual implementation)
        if (!Item->GetClass()->ImplementsInterface(UBP_ItemInterface::StaticClass()))
        {
            continue;
        }
        
        float Distance = FVector::Distance(GetActorLocation(), Item->GetActorLocation());
        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestItem = Item;
        }
    }
    
    return ClosestItem;
}

// Implementation of ForceEnableMovement
void AAtlantisEonsCharacter::ForceEnableMovement()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: ForceEnableMovement called"), *GetName());
    
    // Make sure movement component is working properly
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        // Enable movement
        MovementComp->SetMovementMode(MOVE_Walking);
        MovementComp->bOrientRotationToMovement = true;
        MovementComp->MaxWalkSpeed = BaseMovementSpeed;
        MovementComp->SetComponentTickEnabled(true);
        
        UE_LOG(LogTemp, Warning, TEXT("%s: CharacterMovement enabled, MaxWalkSpeed=%f"), 
            *GetName(), MovementComp->MaxWalkSpeed);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to get CharacterMovement component!"), *GetName());
    }
}

void AAtlantisEonsCharacter::ResetCharacterInput()
{
    // Reset character input by applying a tiny movement to ensure input system is responsive
    GetWorld()->GetTimerManager().SetTimer(
        CameraLagTimer, // Reusing an existing timer handle
        [this]() { 
            if (IsValid(this))
            {
                AddMovementInput(FVector(-1, 0, 0), 0.01f);
            }
        },
        0.1f,
        false
    );
}

void AAtlantisEonsCharacter::MeleeAttack(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("🗡️ MeleeAttack called. Can attack: %s, CurrentAttackIndex: %d, BloomWindowActive: %s"), 
           bCanAttack ? TEXT("Yes") : TEXT("No"), CurrentAttackIndex, bBloomWindowActive ? TEXT("Yes") : TEXT("No"));

    // CRITICAL FIX: Safety check for stuck bloom window state
    if (bBloomWindowActive)
    {
        // Check if the widget's bloom window is actually active
        if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget())
        {
            // If character says bloom is active but widget says it's not, reset the character flag
            if (!SwordBloom->IsBloomWindowActive())
            {
                UE_LOG(LogTemp, Warning, TEXT("🔧 SAFETY FIX: Character bloom flag stuck, widget says inactive - resetting"));
                bBloomWindowActive = false;
            }
        }
    }

    // Check if we can trigger the conditional spark effect during critical window
    bool bTriggeredSpark = TryTriggerSparkEffect();
    if (bTriggeredSpark)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ✨ Critical window hit! Setting flag for combo continuation"));
        bHitCriticalWindow = true;
        return; // Don't start a new attack, the critical window hit will handle chaining
    }
    else if (bBloomWindowActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚠️ Attack input during bloom window but outside critical timing"));
        return; // Don't interrupt current attack if bloom is active
    }
    
    if (!bCanAttack)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Cannot attack - on cooldown"));
        return;
    }

    // Face the nearest enemy before attacking
    FaceNearestEnemy();

    // Get the appropriate attack montage based on current attack index
    UAnimMontage* CurrentMontage = GetCurrentAttackMontage();
    
    if (CurrentMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Playing attack montage %d: %s"), 
               CurrentAttackIndex + 1, *CurrentMontage->GetName());
        
        // Reset critical window flag at start of new attack sequence
        bHitCriticalWindow = false;
        
        // Try to play the montage
        float MontageLength = PlayAnimMontage(CurrentMontage);
        
        if (MontageLength > 0.0f)
        {
            // Store the montage length for bloom effect timing
            CurrentMontageLength = MontageLength;
            
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Montage duration: %f"), MontageLength);
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Successfully playing attack montage with duration: %f"), MontageLength);
            
            // Check animation state
            if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
            {
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Current animation state: IsPlaying=%d"), AnimInstance->IsAnyMontagePlaying());
            }
            
            // Start the bloom circle immediately when attack begins
            UE_LOG(LogTemp, Warning, TEXT("🔵 ⚡ BLOOM WINDOW ACTIVATED - Starting immediately with attack"));
            
            // CRITICAL FIX: Clear any existing bloom hide timers to prevent conflicts
            GetWorld()->GetTimerManager().ClearTimer(SwordBloomHideTimer);
            UE_LOG(LogTemp, Warning, TEXT("🔧 Cleared existing bloom hide timers for new attack"));
            
            bBloomWindowActive = true;
            if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) 
            { 
                SwordBloom->StartBloomCircle(); 
                UE_LOG(LogTemp, Warning, TEXT("🔵 ✅ SwordBloom widget found - StartBloomCircle() called at attack start")); 
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("🔵 ❌ SwordBloom widget NOT found - no visual effects will show")); 
            }
            
            // Start the combo window after this attack
            StartComboWindow();
            
            // CRITICAL FIX: Add a backup timer to call OnMeleeAttackNotify since animation notify may not be set up
            // This ensures damage application happens even without animation notify setup
            float NotifyTiming = MontageLength * 0.6f; // Call at 60% through the animation
            FTimerHandle AttackNotifyTimer;
            GetWorld()->GetTimerManager().SetTimer(
                AttackNotifyTimer,
                FTimerDelegate::CreateUObject(this, &AAtlantisEonsCharacter::OnMeleeAttackNotify),
                NotifyTiming,
                false
            );
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Set backup timer to call OnMeleeAttackNotify in %.2f seconds"), NotifyTiming);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("🗡️ Failed to play attack montage. Current anim mode: %d"), GetMesh()->GetAnimationMode());
            ResetAttack();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🗡️ No attack montage available for index %d!"), CurrentAttackIndex);
        ResetAttack();
    }
    
    // Set timer for cooldown reset
    GetWorld()->GetTimerManager().SetTimer(
        AttackCooldownTimer,
        FTimerDelegate::CreateUObject(this, &AAtlantisEonsCharacter::ResetAttack),
        AttackCooldown,
        false
    );
}

void AAtlantisEonsCharacter::ResetAttack()
{
    bCanAttack = true;
    bIsAttacking = false;
    
    // Always reset critical window flag after processing
    if (bHitCriticalWindow)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Resetting critical window flag after combo processing"));
        bHitCriticalWindow = false;
    }
    else
    {
        // If we're not continuing a combo, reset the chain
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack finished without critical window hit - resetting combo"));
        ResetComboChain();
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack cooldown reset. Can attack again."));
}

void AAtlantisEonsCharacter::CloseInventoryImpl()
{
    if (bIsInventoryOpen)
    {
        // UE_LOG(LogTemp, Warning, TEXT("CloseInventory - Restoring default mapping context"));
        
        // Get the HUD
        AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
        if (!HUD)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get HUD in CloseInventory"));
            return;
        }
        
        // POPUP FIX: Clear all item info popups BEFORE hiding the inventory widget
        // This ensures we can still access the slots to clear their tooltips
        ClearAllInventoryPopups();
        
        // Hide the inventory widget
        HUD->HideInventoryWidget();
        bIsInventoryOpen = false;
        
        // Get the player controller
        APlayerController* PC = Cast<APlayerController>(Controller);
        if (!PC)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get PlayerController in CloseInventory"));
            return;
        }
        
        // Clear keyboard focus from any widgets and reset input mode
        if (GEngine && GEngine->GameViewport)
        {
            FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
            FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
        }
        
        // Set input mode back to game only
        FInputModeGameOnly GameOnlyMode;
        PC->SetInputMode(GameOnlyMode);
        PC->SetShowMouseCursor(false);
        PC->FlushPressedKeys();
        
        // Restore all input mappings
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            // Clear all mappings first
            Subsystem->ClearAllMappings();
            
            // Then add back the default mapping context
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
                // UE_LOG(LogTemp, Warning, TEXT("CloseInventory - Restored default mapping context"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("CloseInventory - DefaultMappingContext is null!"));
            }
        }
        
        // Re-enable character movement
        if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
        {
            MovementComp->SetMovementMode(MOVE_Walking);
            MovementComp->bOrientRotationToMovement = true;
            MovementComp->MaxWalkSpeed = BaseMovementSpeed;
        }
        
        // Force enable input for both controller and character
        PC->EnableInput(PC);
        EnableInput(PC);
        
        // Reset character input to ensure everything works
        ResetCharacterInput();
        
        // UE_LOG(LogTemp, Warning, TEXT("====== INVENTORY CLOSE: CloseInventory COMPLETED ======"));
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("CloseInventory called but inventory was not open"));
    }
}

void AAtlantisEonsCharacter::ClearAllInventoryPopups()
{
    // Clear all item info popups from inventory slots to prevent them from staying visible
    // when the inventory is closed while hovering over items
    
    UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: Starting comprehensive popup clearing"));
    
    if (!MainWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: MainWidget is null"));
        return;
    }
    
    // Get the inventory widget
    UWBP_Inventory* InventoryWidget = MainWidget->GetInventoryWidget();
    if (!InventoryWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: InventoryWidget is null"));
        return;
    }
    
    // Get all inventory slot widgets and clear their popups
    const TArray<UWBP_InventorySlot*>& InventorySlots = InventoryWidget->GetInventorySlotWidgets();
    
    int32 ClearedPopups = 0;
    for (UWBP_InventorySlot* Slot : InventorySlots)
    {
        if (Slot)
        {
            // Call the slot's RemoveItemDescription function to clear any active popups
            Slot->RemoveItemDescription();
            ClearedPopups++;
        }
    }
    
    // ENHANCED CLEARING: Also clear any ItemDescription widgets directly from each slot's reference
    // This ensures complete cleanup even if some widgets weren't properly removed
    for (UWBP_InventorySlot* Slot : InventorySlots)
    {
        if (Slot)
        {
            // Also clear the ItemDescription widget directly if it exists
            if (Slot->ItemDescription && Slot->ItemDescription->IsInViewport())
            {
                Slot->ItemDescription->RemoveFromParent();
                UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: Force-removed slot %d ItemDescription widget"), Slot->SlotIndex);
            }
            
            // Clear the WidgetItemDescriptionRef too
            if (Slot->WidgetItemDescriptionRef && Slot->WidgetItemDescriptionRef->IsInViewport())
            {
                Slot->WidgetItemDescriptionRef->RemoveFromParent();
                UE_LOG(LogTemp, Warning, TEXT("ClearAllInventoryPopups: Force-removed slot %d WidgetItemDescriptionRef"), Slot->SlotIndex);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("ClearAllInventoryPopups: Cleared popups from %d inventory slots with enhanced cleanup"), ClearedPopups);
    
    // ADDITIONAL SAFETY: Clear any focus from widgets that might be keeping tooltips alive
    if (GEngine && GEngine->GameViewport)
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
        UE_LOG(LogTemp, Log, TEXT("ClearAllInventoryPopups: Cleared widget focus to ensure tooltip cleanup"));
    }
}

void AAtlantisEonsCharacter::ToggleInventory(const FInputActionValue& Value)
{
    // Check if toggle is locked (but allow closing even if locked for emergency situations)
    if (bInventoryToggleLocked && !bIsInventoryOpen)
    {
        // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called but ignored due to toggle lock (opening blocked)"));
        return;
    }
    
    // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called. Current state: %s"), bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));
    
    // Get the HUD first
    AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
    if (!HUD)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get HUD in ToggleInventory"));
        return;
    }
    
    // Lock toggle to prevent rapid switching (but shorter duration)
    bInventoryToggleLocked = true;
    GetWorldTimerManager().SetTimer(InventoryToggleLockTimer, this, &AAtlantisEonsCharacter::UnlockInventoryToggle, 0.1f, false);
    
    // If inventory is open, close it
    if (bIsInventoryOpen)
    {
        // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: Closing inventory"));
        CloseInventory();
    }
    else
    {
        // Only try to open if it's not already visible
        if (!HUD->IsInventoryWidgetVisible())
        {
            // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: Opening inventory"));
            OpenInventory();
        }
        else
        {
            // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: Widget already visible, syncing state"));
            bIsInventoryOpen = true;
        }
    }
}

void AAtlantisEonsCharacter::UnlockInventoryToggle()
{
    bInventoryToggleLocked = false;
}

void AAtlantisEonsCharacter::OpenInventory()
{
    // Get the HUD
    AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
    if (!HUD)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot open inventory - HUD is null"), *GetName());
        return;
    }

    // Show the inventory widget
    if (!HUD->ShowInventoryWidget())
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to show inventory widget"), *GetName());
        return;
    }

    // Get the main widget reference
    MainWidget = HUD->GetMainWidget();
    if (!MainWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to get Main widget from HUD"), *GetName());
        return;
    }

    // Update the inventory state
    bIsInventoryOpen = true;
    
    // Force update all inventory slots immediately
    UpdateInventorySlots();
    
    // Also set up a delayed update to ensure it runs after any Blueprint logic
    ForceUpdateInventorySlotsAfterDelay();
    
    // UE_LOG(LogTemp, Display, TEXT("%s: Inventory opened successfully"), *GetName());
}

void AAtlantisEonsCharacter::UpdateInventorySlots()
{
    if (InventoryComp)
    {
        InventoryComp->UpdateInventorySlots();
    }
}

void AAtlantisEonsCharacter::DelayedUpdateInventorySlots()
{
    UpdateInventorySlots();
}

void AAtlantisEonsCharacter::ForceUpdateInventorySlotsAfterDelay()
{
    if (!bIsInventoryOpen)
    {
        return;
    }
    
    // Clear any existing timer
    GetWorldTimerManager().ClearTimer(InventoryUpdateTimer);
    
    // Set a small delay to ensure Blueprint logic completes first
    GetWorldTimerManager().SetTimer(
        InventoryUpdateTimer,
        this,
        &AAtlantisEonsCharacter::DelayedUpdateInventorySlots,
        0.1f,
        false
    );
}

void AAtlantisEonsCharacter::CloseInventory()
{
    // Use the existing implementation
    CloseInventoryImpl();
}

FStructure_ItemInfo AAtlantisEonsCharacter::CreateHardcodedItemData(int32 ItemIndex)
{
    FStructure_ItemInfo ItemInfo;
    
    switch (ItemIndex)
    {
        case 1: // Basic HP Potion
            ItemInfo.ItemIndex = 1;
            ItemInfo.ItemName = TEXT("Basic HP Potion");
            ItemInfo.ItemDescription = TEXT("Restores a small amount of HP.");
            ItemInfo.bIsValid = true;
            ItemInfo.bIsStackable = true;
            ItemInfo.StackNumber = 1;
            ItemInfo.ItemType = EItemType::Consume_HP;
            ItemInfo.ItemEquipSlot = EItemEquipSlot::None;
            ItemInfo.RecoveryHP = 30;
            ItemInfo.RecoveryMP = 0;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicHealingPotion")));
            break;
            
        // Add more cases for other items as needed
            
        default:
            UE_LOG(LogTemp, Warning, TEXT("%s: Unknown item index %d, creating empty item"), *GetName(), ItemIndex);
            ItemInfo.bIsValid = false;
            break;
    }
    
    return ItemInfo;
}

bool AAtlantisEonsCharacter::AddItemToInventory(UBP_ItemInfo* ItemInfo)
{
    if (InventoryComp)
    {
        return InventoryComp->AddItemToInventory(ItemInfo);
    }
    UE_LOG(LogTemp, Warning, TEXT("AddItemToInventory: InventoryComponent is null"));
    return false;
}

void AAtlantisEonsCharacter::SetMainWidget(UWBP_Main* NewWidget)
{
    if (NewWidget)
    {
        MainWidget = NewWidget;
        UE_LOG(LogTemp, Display, TEXT("%s: Main widget set successfully"), *GetName());
        
        // Get the character info widget from the main widget
        if (MainWidget->GetCharacterInfoWidget())
        {
            WBP_CharacterInfo = MainWidget->GetCharacterInfoWidget();
            UE_LOG(LogTemp, Display, TEXT("%s: Character info widget set from main widget"), *GetName());
        }
        
        // If inventory is currently open, update the slots immediately and with delay
        if (bIsInventoryOpen)
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Inventory is open, updating slots immediately and with delay"), *GetName());
            UpdateInventorySlots();
            ForceUpdateInventorySlotsAfterDelay();
        }
        
        // Initialize equipment slot references and update UI
        InitializeEquipmentSlotReferences();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Attempted to set null Main widget"), *GetName());
    }
}

void AAtlantisEonsCharacter::UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->UpdateEquipmentSlotUI(EquipSlot, ItemInfo);
    }
}

void AAtlantisEonsCharacter::ClearEquipmentSlotUI(EItemEquipSlot EquipSlot)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->ClearEquipmentSlotUI(EquipSlot);
    }
}

void AAtlantisEonsCharacter::UpdateAllEquipmentSlotsUI()
{
    // Update all equipment slots based on currently equipped items
    for (int32 i = 0; i < EquipmentSlots.Num(); ++i)
    {
        EItemEquipSlot SlotType = EItemEquipSlot::None;
        
        // Map array index to equipment slot type
        switch (i)
        {
            case 0:
                SlotType = EItemEquipSlot::Head;
                break;
            case 1:
                SlotType = EItemEquipSlot::Body;
                break;
            case 2:
                SlotType = EItemEquipSlot::Weapon;
                break;
            case 3:
                SlotType = EItemEquipSlot::Accessory;
                break;
            default:
                continue;
        }
        
        // Update the UI slot with the equipped item (or clear if none)
        UpdateEquipmentSlotUI(SlotType, EquipmentSlots[i]);
    }
    
    UE_LOG(LogTemp, Log, TEXT("🔄 Updated all equipment slot UIs"));
}

void AAtlantisEonsCharacter::InitializeEquipmentSlotReferences()
{
    // Get equipment slot references from the character info widget
    if (WBP_CharacterInfo)
    {
        HeadSlot = WBP_CharacterInfo->HeadSlot;
        WeaponSlot = WBP_CharacterInfo->WeaponSlot;
        SuitSlot = WBP_CharacterInfo->SuitSlot;
        CollectableSlot = WBP_CharacterInfo->CollectableSlot;
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Initialized equipment slot references from CharacterInfo widget"));
        UE_LOG(LogTemp, Warning, TEXT("   HeadSlot: %s"), HeadSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   WeaponSlot: %s"), WeaponSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   SuitSlot: %s"), SuitSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   CollectableSlot: %s"), CollectableSlot ? TEXT("Valid") : TEXT("NULL"));
        
        // Set up slot indices for equipment slots (different from inventory slots)
        if (HeadSlot) 
        {
            HeadSlot->SetSlotIndex(100); // Use high numbers to distinguish from inventory
            // Set slot type to Equipment to disable context menu
            HeadSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            // Bind click event for unequipping
            HeadSlot->OnSlotClicked.AddDynamic(this, &AAtlantisEonsCharacter::OnHeadSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   HeadSlot configured as Equipment slot"));
        }
        if (WeaponSlot) 
        {
            WeaponSlot->SetSlotIndex(101);
            // Set slot type to Equipment to disable context menu
            WeaponSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            WeaponSlot->OnSlotClicked.AddDynamic(this, &AAtlantisEonsCharacter::OnWeaponSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   WeaponSlot configured as Equipment slot"));
        }
        if (SuitSlot) 
        {
            SuitSlot->SetSlotIndex(102);
            // Set slot type to Equipment to disable context menu
            SuitSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            SuitSlot->OnSlotClicked.AddDynamic(this, &AAtlantisEonsCharacter::OnSuitSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   SuitSlot configured as Equipment slot"));
        }
        if (CollectableSlot) 
        {
            CollectableSlot->SetSlotIndex(103);
            // Set slot type to Equipment to disable context menu
            CollectableSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            CollectableSlot->OnSlotClicked.AddDynamic(this, &AAtlantisEonsCharacter::OnCollectableSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   CollectableSlot configured as Equipment slot"));
        }
        
        // Update all equipment slots after initialization
        UpdateAllEquipmentSlotsUI();
        
        // Update stats to ensure they reflect current equipment
        UpdateAllStats();
        
        // Force refresh the stats display
        RefreshStatsDisplay();
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("InitializeEquipmentSlotReferences: WBP_CharacterInfo is null"));
    }
}

void AAtlantisEonsCharacter::OnEquipmentSlotClicked(EItemEquipSlot EquipSlot)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->OnEquipmentSlotClicked(EquipSlot);
    }
}

void AAtlantisEonsCharacter::OnHeadSlotClicked(int32 SlotIndex)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->OnHeadSlotClicked(SlotIndex);
    }
}

void AAtlantisEonsCharacter::OnWeaponSlotClicked(int32 SlotIndex)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->OnWeaponSlotClicked(SlotIndex);
    }
}

void AAtlantisEonsCharacter::OnSuitSlotClicked(int32 SlotIndex)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->OnSuitSlotClicked(SlotIndex);
    }
}

void AAtlantisEonsCharacter::OnCollectableSlotClicked(int32 SlotIndex)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->OnCollectableSlotClicked(SlotIndex);
    }
}

void AAtlantisEonsCharacter::EquipInventoryItem(UBP_ItemInfo* ItemInfoRef)
{
    if (EquipmentComponent)
    {
        // Pass character reference directly to bypass world context issues
        EquipmentComponent->EquipInventoryItemWithCharacter(ItemInfoRef, this);
    }
}

void AAtlantisEonsCharacter::OnMeleeAttackNotify()
{
    UE_LOG(LogTemp, Warning, TEXT("🗡️ OnMeleeAttackNotify called - Player attacking"));
    
    // Note: Bloom circle activation now happens immediately when attack starts
    // This function now only handles damage application
    
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Character is dead, skipping attack"));
        return;
    }
    
    // Prevent multiple calls in the same attack sequence
    if (bAttackNotifyInProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack notify already in progress, skipping duplicate call"));
        return;
    }
    
    bAttackNotifyInProgress = true;
    
    // Reset the flag after a short delay to allow for the next attack
    FTimerHandle ResetNotifyTimer;
    GetWorld()->GetTimerManager().SetTimer(
        ResetNotifyTimer,
        [this]() { 
            bAttackNotifyInProgress = false; // Reset the flag
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack notify flag reset - ready for next attack"));
        },
        0.5f, // Reset after 0.5 seconds
        false
    );
    
    // Calculate damage based on character stats and equipped weapon
    float BaseDamage = static_cast<float>(CurrentDamage); // Use character's current damage stat
    float WeaponDamage = 0.0f;
    
    // Check if we have a weapon equipped and get its damage bonus
    if (EquipmentSlots.Num() > 2 && EquipmentSlots[2]) // Weapon slot is index 2
    {
        UBP_ItemInfo* WeaponItem = EquipmentSlots[2];
        if (WeaponItem)
        {
            // Get weapon data from store system
            FStructure_ItemInfo WeaponData;
            if (UStoreSystemFix::GetItemData(WeaponItem->GetItemIndex(), WeaponData))
            {
                WeaponDamage = static_cast<float>(WeaponData.Damage);
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Equipped weapon '%s' adds %f damage"), 
                       *WeaponData.ItemName, WeaponDamage);
            }
        }
    }
    
    float TotalDamage = BaseDamage + WeaponDamage;
    UE_LOG(LogTemp, Warning, TEXT("🗡️ Total calculated damage: %f (Base: %f + Weapon: %f)"), 
           TotalDamage, BaseDamage, WeaponDamage);
    
    // IMPROVED ATTACK DETECTION: Use multiple detection methods for maximum reliability
    FVector StartLocation = GetActorLocation();
    FVector ForwardVector = GetActorForwardVector();
    float AttackRange = 300.0f; // Reasonable attack range
    float AttackRadius = 200.0f; // Larger radius for better detection
    
    // Track which actors we've already hit to prevent multiple hits
    TSet<AActor*> AlreadyHitActors;
    bool bSuccessfulHit = false;
    
    // METHOD 1: Direct sphere overlap at player location (most reliable)
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(AttackRadius);
    
    bool bHitDetected = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        StartLocation, // Use player location directly instead of calculated center
        FQuat::Identity,
        ECollisionChannel::ECC_Pawn,
        SphereShape
    );
    
    UE_LOG(LogTemp, Warning, TEXT("🗡️ Sphere overlap found %d results"), OverlapResults.Num());
    
    if (bHitDetected)
    {
        for (const FOverlapResult& Result : OverlapResults)
        {
            AActor* HitActor = Result.GetActor();
            if (!HitActor || HitActor == this)
                continue;
                
            // Skip if we already hit this actor
            if (AlreadyHitActors.Contains(HitActor))
            {
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Skipping already hit actor: %s"), *HitActor->GetName());
                continue;
            }
            
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Checking actor: %s (Class: %s)"), 
                   *HitActor->GetName(), *HitActor->GetClass()->GetName());
            
            // Check if it's a zombie by class name (most reliable method)
            bool bIsZombie = HitActor->GetClass()->GetName().Contains(TEXT("ZombieCharacter"));
            
            if (bIsZombie)
            {
                UE_LOG(LogTemp, Warning, TEXT("🗡️ ✓ Found zombie by class name: %s"), 
                       *HitActor->GetClass()->GetName());
                
                // Calculate distance and direction for additional validation
                FVector ToTarget = HitActor->GetActorLocation() - GetActorLocation();
                float DistanceToTarget = ToTarget.Size();
                float DotProduct = FVector::DotProduct(ForwardVector, ToTarget.GetSafeNormal());
                
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Zombie %s - Distance: %.1f, Dot: %.2f"), 
                       *HitActor->GetName(), DistanceToTarget, DotProduct);
                
                // More lenient distance and direction check for better hit detection
                if (DistanceToTarget <= AttackRange && DotProduct > -0.5f) // Allow hits from wider angles
                {
                    // Mark this actor as hit
                    AlreadyHitActors.Add(HitActor);
                    
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚔️ ATTACKING zombie %s with %.1f damage!"), 
                           *HitActor->GetName(), TotalDamage);
                    
                    // Apply damage to the zombie
                    float ActualDamage = HitActor->TakeDamage(TotalDamage, FDamageEvent(), GetController(), this);
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ ✅ TakeDamage returned: %.1f"), ActualDamage);
                    
                    bSuccessfulHit = true;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ Zombie %s out of range or wrong direction (Distance: %.1f, Dot: %.2f)"), 
                           *HitActor->GetName(), DistanceToTarget, DotProduct);
                }
            }
        }
    }
    
    // METHOD 2: If no hits found, try a forward sweep as backup
    if (!bSuccessfulHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ No hits from sphere overlap, trying forward sweep backup"));
        
        TArray<FHitResult> HitResults;
        FVector EndLocation = StartLocation + (ForwardVector * AttackRange);
        
        bool bSweepHit = GetWorld()->SweepMultiByChannel(
            HitResults,
            StartLocation,
            EndLocation,
            FQuat::Identity,
            ECollisionChannel::ECC_Pawn,
            FCollisionShape::MakeSphere(AttackRadius * 0.8f) // Slightly smaller for sweep
        );
        
        if (bSweepHit)
        {
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Forward sweep found %d results"), HitResults.Num());
            
            for (const FHitResult& Hit : HitResults)
            {
                AActor* HitActor = Hit.GetActor();
                if (!HitActor || HitActor == this || AlreadyHitActors.Contains(HitActor))
                    continue;
                
                bool bIsZombie = HitActor->GetClass()->GetName().Contains(TEXT("ZombieCharacter"));
                if (bIsZombie)
                {
                    AlreadyHitActors.Add(HitActor);
                    
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚔️ SWEEP ATTACKING zombie %s with %.1f damage!"), 
                           *HitActor->GetName(), TotalDamage);
                    
                    float ActualDamage = HitActor->TakeDamage(TotalDamage, FDamageEvent(), GetController(), this);
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ ✅ Sweep TakeDamage returned: %.1f"), ActualDamage);
                    
                    bSuccessfulHit = true;
                    break; // Only hit one enemy per attack
                }
            }
        }
    }
    
    if (bSuccessfulHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ✅ Successfully hit at least one enemy!"));
        
        // 🎯 DON'T trigger spark here - spark should only show during critical timing window
        // BloomSparkNotify(); // REMOVED - this was causing spark to show on every hit
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ❌ No valid enemies found in attack range"));
    }
    
    // 🎯 SCHEDULE HIDING BLOOM EFFECTS AFTER A SHORT DELAY
    float BloomDuration = FMath::Max(CurrentMontageLength - 0.2f, 0.5f);
    UE_LOG(LogTemp, Warning, TEXT("🗡️ 🔵 Bloom effects will last for %.2f seconds (montage: %.2f)"), BloomDuration, CurrentMontageLength);
    
    GetWorld()->GetTimerManager().SetTimer(
        SwordBloomHideTimer, // Use the class member timer handle instead of local variable
        [this]() { 
            HideBloomEffectsNotify(); // This will call HideBloomEffectsNotify() and set bBloomWindowActive = false
            UE_LOG(LogTemp, Warning, TEXT("🗡️ 🔒 SwordBloom effects hidden after attack"));
        },
        BloomDuration,
        false
    );
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

float AAtlantisEonsCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    UE_LOG(LogTemplateCharacter, Warning, TEXT("Player TakeDamage called: %.1f damage from %s, Invulnerable: %s, Dead: %s"), 
           DamageAmount, 
           DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"),
           bIsInvulnerable ? TEXT("Yes") : TEXT("No"),
           bIsDead ? TEXT("Yes") : TEXT("No"));

    // Don't take damage if already dead
    if (bIsDead)
    {
        UE_LOG(LogTemplateCharacter, Warning, TEXT("Player is dead, ignoring damage"));
        return 0.0f;
    }

    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    
    UE_LOG(LogTemplateCharacter, Warning, TEXT("Player Super::TakeDamage returned: %.1f"), ActualDamage);

    // Apply damage if not invulnerable (for now, always apply damage to fix combat issues)
    // TODO: Implement proper invulnerability frames later
    ApplyDamage(ActualDamage);
    UE_LOG(LogTemplateCharacter, Warning, TEXT("Player took %.1f damage from %s"), ActualDamage, DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"));

    return ActualDamage;
}

void AAtlantisEonsCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Function entered. bIsDead: %d"), bIsDead ? 1 : 0);
    
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Character is already dead, returning early"));
        return;
    }

    // Safety check to ensure we're still valid
    if (!IsValid(this))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleDeath: Character is no longer valid, aborting"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Setting bIsDead to true and disabling movement"));
    bIsDead = true;

    // Disable movement with safety checks
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->DisableMovement();
        MovementComp->StopMovementImmediately();
    }

    // Disable input with safety checks
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Disabled player input"));
    }

    // Play death animation if available
    if (DeathMontage && GetMesh())
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            // Stop any current montages first
            AnimInstance->StopAllMontages(0.1f);
            
            // Play the death montage
            float MontageLength = AnimInstance->Montage_Play(DeathMontage, 1.0f);
            if (MontageLength > 0.0f)
            {
                UE_LOG(LogTemp, Warning, TEXT("💀 HandleDeath: Successfully started death montage (length: %.2fs)"), MontageLength);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("💀 HandleDeath: Failed to play death montage! MontageLength: %.2f"), MontageLength);
                UE_LOG(LogTemp, Error, TEXT("💀 HandleDeath: Check if DeathMontage is compatible with character skeleton"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("💀 HandleDeath: No AnimInstance found for death animation!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("💀 HandleDeath: No death montage assigned in Blueprint or no mesh! Please assign a death montage in BP_Character."));
    }

    // Log death
    UE_LOG(LogTemplateCharacter, Log, TEXT("Character has died"));

    // Start respawn timer with safety checks
    if (GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Starting respawn timer for %f seconds"), RespawnDelay);
        GetWorld()->GetTimerManager().SetTimer(
            RespawnTimerHandle,
            this,
            &AAtlantisEonsCharacter::ResetCharacter,
            RespawnDelay,
            false
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("HandleDeath: Cannot start respawn timer - invalid world"));
    }
}

void AAtlantisEonsCharacter::ResetCharacter()
{
    UE_LOG(LogTemp, Warning, TEXT("ResetCharacter: Resetting character after death"));
    
    // Reset health
    CurrentHealth = MaxHealth;
    bIsDead = false;

    // Stop any ongoing animations
    if (DeathMontage)
    {
        StopAnimMontage(DeathMontage);
    }

    // Re-enable movement
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    // Re-enable input
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        EnableInput(PC);
    }

    // Respawn at appropriate location
    RespawnCharacter();

    // Update UI
    SettingCircularBar_HP();
    SettingCircularBar_MP();

    // Log respawn
    UE_LOG(LogTemplateCharacter, Log, TEXT("Character has respawned with full health"));
}

void AAtlantisEonsCharacter::RespawnCharacter()
{
    UE_LOG(LogTemp, Warning, TEXT("RespawnCharacter: Finding respawn location"));
    
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
            UE_LOG(LogTemp, Warning, TEXT("RespawnCharacter: Found PlayerStart at %s"), *StartSpot->GetActorLocation().ToString());
        }
    }

    if (StartSpot)
    {
        // Teleport to the start spot
        SetActorLocation(StartSpot->GetActorLocation());
        SetActorRotation(StartSpot->GetActorRotation());
        UE_LOG(LogTemp, Warning, TEXT("RespawnCharacter: Teleported to PlayerStart"));
    }
    else
    {
        // If no player start is found, just reset to origin
        SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
        SetActorRotation(FRotator(0.0f));
        UE_LOG(LogTemplateCharacter, Warning, TEXT("No PlayerStart found - respawning at origin"));
    }
}

void AAtlantisEonsCharacter::FaceNearestEnemy()
{
    // SAFETY CHECKS: Prevent crashes during shutdown or invalid states
    if (!IsValid(this) || IsActorBeingDestroyed())
    {
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: Character is being destroyed, skipping"));
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World || !IsValid(World))
    {
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: World is null or invalid, skipping"));
        return;
    }
    
    if (!Controller || !IsValid(Controller))
    {
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: Controller is null or invalid, skipping"));
        return;
    }
    
    // Find the nearest enemy within a reasonable range
    const float SearchRadius = 500.0f; // 5 meter search radius
    AActor* NearestEnemy = nullptr;
    float NearestDistance = SearchRadius;
    
    // Get all actors in the world
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    
    FGenericTeamId MyTeamId = GetGenericTeamId();
    FVector MyLocation = GetActorLocation();
    
    for (AActor* Actor : AllActors)
    {
        if (!Actor || Actor == this || !IsValid(Actor)) continue;
        
        // Additional safety check for actor state
        if (Actor->IsActorBeingDestroyed()) continue;
        
        // Check if this is an enemy
        bool bIsEnemy = false;
        
        // Check if actor has IGenericTeamAgentInterface
        if (Actor->GetClass() && Actor->GetClass()->ImplementsInterface(UGenericTeamAgentInterface::StaticClass()))
        {
            IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
            if (TeamAgent)
            {
                FGenericTeamId ActorTeamId = TeamAgent->GetGenericTeamId();
                if (ActorTeamId != MyTeamId && ActorTeamId.GetId() != 255) // 255 is usually neutral
                {
                    bIsEnemy = true;
                }
            }
        }
        
        // Also check by class name for ZombieCharacter
        if (!bIsEnemy && Actor->GetClass())
        {
            FString ClassName = Actor->GetClass()->GetName();
            if (ClassName.Contains(TEXT("Zombie")) || ClassName.Contains(TEXT("Enemy")))
            {
                bIsEnemy = true;
            }
        }
        
        if (bIsEnemy)
        {
            float Distance = FVector::Dist(MyLocation, Actor->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestEnemy = Actor;
            }
        }
    }
    
    if (NearestEnemy && IsValid(NearestEnemy))
    {
        // Calculate direction to enemy
        FVector DirectionToEnemy = (NearestEnemy->GetActorLocation() - MyLocation).GetSafeNormal();
        
        // Calculate target rotation (only yaw, keep character upright)
        FRotator TargetRotation = DirectionToEnemy.Rotation();
        TargetRotation.Pitch = 0.0f; // Keep character upright
        TargetRotation.Roll = 0.0f;  // Keep character upright
        
        // Get current rotation for logging
        FRotator CurrentRotation = GetActorRotation();
        
        // DEBUG: Log rotation details
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy DEBUG:"));
        UE_LOG(LogTemp, Warning, TEXT("  Current Rotation: P=%.2f Y=%.2f R=%.2f"), CurrentRotation.Pitch, CurrentRotation.Yaw, CurrentRotation.Roll);
        UE_LOG(LogTemp, Warning, TEXT("  Target Rotation: P=%.2f Y=%.2f R=%.2f"), TargetRotation.Pitch, TargetRotation.Yaw, TargetRotation.Roll);
        UE_LOG(LogTemp, Warning, TEXT("  Direction to Enemy: X=%.2f Y=%.2f Z=%.2f"), DirectionToEnemy.X, DirectionToEnemy.Y, DirectionToEnemy.Z);
        
        // INSTANT ROTATION for testing - no interpolation
        SetActorRotation(TargetRotation);
        
        // Verify the rotation was applied
        FRotator NewRotation = GetActorRotation();
        UE_LOG(LogTemp, Warning, TEXT("  Applied Rotation: P=%.2f Y=%.2f R=%.2f"), NewRotation.Pitch, NewRotation.Yaw, NewRotation.Roll);
        
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: INSTANTLY rotated character to face enemy %s at distance %.1f"), 
               *NearestEnemy->GetName(), NearestDistance);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: No valid enemies found within %.1f units"), SearchRadius);
    }
}

void AAtlantisEonsCharacter::PlayHitReactMontage()
{
    // FIXED: Don't play hit reactions if the character is dead
    if (bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Skipping hit reaction - character is dead"));
        return;
    }
    
    // Enhanced hit reaction system with better error handling and fallbacks
    if (!GetMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("🎭 Player: No mesh component found for hit reaction!"));
        return;
    }
    
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("🎭 Player: No AnimInstance found for hit reaction!"));
        return;
    }
    
    if (!HitReactMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎭 Player: HitReactMontage is not assigned in Blueprint!"));
        UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Please assign a hit reaction montage in the Character Blueprint"));
        return; // FIXED: No fallback system - just exit if no montage
    }
    
    // Stop any currently playing montage first for clean transitions
    if (AnimInstance->IsAnyMontagePlaying())
    {
        AnimInstance->StopAllMontages(0.1f);
        UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Stopped existing montages for hit reaction"));
    }
    
    // Disable movement and input during hit reaction
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->DisableMovement();
        UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Movement disabled during hit reaction"));
    }
    
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
        UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Input disabled during hit reaction"));
    }
    
    // Play the hit reaction montage
    float MontageLength = AnimInstance->Montage_Play(HitReactMontage, 1.0f);
    
    if (MontageLength > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Successfully started hit reaction montage (length: %.2fs)"), MontageLength);
        
        // Re-enable movement and input after montage completes
        FTimerHandle HitReactionTimer;
        GetWorld()->GetTimerManager().SetTimer(
            HitReactionTimer,
            [this]() {
                if (GetCharacterMovement() && !bIsDead)
                {
                    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                    UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Restored movement after hit reaction"));
                }
                
                if (APlayerController* PC = Cast<APlayerController>(GetController()))
                {
                    EnableInput(PC);
                    UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Restored input after hit reaction"));
                }
            },
            MontageLength + 0.1f, // Slight delay to ensure montage finishes
            false
        );
    }
    else
    {
        // If montage fails, restore movement and input immediately
        if (GetCharacterMovement() && !bIsDead)
        {
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Restored movement after failed montage"));
        }
        
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            EnableInput(PC);
            UE_LOG(LogTemp, Warning, TEXT("🎭 Player: Restored input after failed montage"));
        }
        
        UE_LOG(LogTemp, Error, TEXT("🎭 Player: Failed to play hit reaction montage! MontageLength: %.2f"), MontageLength);
        UE_LOG(LogTemp, Error, TEXT("🎭 Player: Check if the montage is compatible with the character's skeleton"));
    }
}

// Stat bonus functions
void AAtlantisEonsCharacter::AddingCharacterStatus(int32 ItemIndex)
{
    // Get item data and apply stat bonuses
    FStructure_ItemInfo ItemData;
    if (UStoreSystemFix::GetItemData(ItemIndex, ItemData))
    {
        BaseSTR += ItemData.STR;
        BaseDEX += ItemData.DEX;
        BaseINT += ItemData.INT;
        BaseDefence += ItemData.Defence;
        BaseDamage += ItemData.Damage;
        BaseHealth += ItemData.HP;
        BaseMP += ItemData.MP;
        
        UE_LOG(LogTemp, Log, TEXT("Added stat bonuses for item %d: STR+%d, DEX+%d, INT+%d, DEF+%d, DMG+%d, HP+%d, MP+%d"), 
               ItemIndex, ItemData.STR, ItemData.DEX, ItemData.INT, 
               ItemData.Defence, ItemData.Damage, ItemData.HP, ItemData.MP);
    }
}

void AAtlantisEonsCharacter::SubtractingCharacterStatus(int32 ItemIndex)
{
    // Get item data and remove stat bonuses
    FStructure_ItemInfo ItemData;
    if (UStoreSystemFix::GetItemData(ItemIndex, ItemData))
    {
        BaseSTR = FMath::Max(0, BaseSTR - ItemData.STR);
        BaseDEX = FMath::Max(0, BaseDEX - ItemData.DEX);
        BaseINT = FMath::Max(0, BaseINT - ItemData.INT);
        BaseDefence = FMath::Max(0, BaseDefence - ItemData.Defence);
        BaseDamage = FMath::Max(0, BaseDamage - ItemData.Damage);
        BaseHealth = FMath::Max(1.0f, BaseHealth - ItemData.HP);
        BaseMP = FMath::Max(0, BaseMP - ItemData.MP);
        
        UE_LOG(LogTemp, Log, TEXT("Removed stat bonuses for item %d: STR-%d, DEX-%d, INT-%d, DEF-%d, DMG-%d, HP-%d, MP-%d"), 
               ItemIndex, ItemData.STR, ItemData.DEX, ItemData.INT, ItemData.Defence, ItemData.Damage, ItemData.HP, ItemData.MP);
    }
}

int32 AAtlantisEonsCharacter::GetYourGold() const
{
    return YourGold;
}

int32 AAtlantisEonsCharacter::GetCurrentMP() const
{
    return CurrentMP;
}

// Context Menu Event Handlers - called by inventory slots via events
void AAtlantisEonsCharacter::OnContextMenuUse(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot)
{
    ContextMenuUse(InventorySlot);
}

void AAtlantisEonsCharacter::OnContextMenuThrow(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot)
{
    ContextMenuThrow(InventorySlot);
}

// SwordBloom Widget Functions - disabled due to type confusion

// Animation Notify Functions - now using Blueprint implementable events
void AAtlantisEonsCharacter::BloomCircleNotify()
{
    bBloomWindowActive = true;
    UE_LOG(LogTemp, Warning, TEXT("🔵 ⚡ BLOOM WINDOW ACTIVATED - Critical timing now available"));
    if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) { SwordBloom->StartBloomCircle(); UE_LOG(LogTemp, Warning, TEXT("🔵 ✅ SwordBloom widget found - StartBloomCircle() called")); } else { UE_LOG(LogTemp, Error, TEXT("🔵 ❌ SwordBloom widget NOT found - no visual effects will show")); }
    UE_LOG(LogTemp, Warning, TEXT("🔵 Bloom circle notify called - Blueprint should handle visual effects"));
}

void AAtlantisEonsCharacter::BloomSparkNotify()
{
    if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) { SwordBloom->ShowSwordSpark(); UE_LOG(LogTemp, Warning, TEXT("✨ ✅ SwordBloom widget found - ShowSwordSpark() called")); } else { UE_LOG(LogTemp, Error, TEXT("✨ ❌ SwordBloom widget NOT found - no spark effects will show")); }
    UE_LOG(LogTemp, Warning, TEXT("✨ Bloom spark notify called - Blueprint should handle visual effects"));
}

void AAtlantisEonsCharacter::HideBloomEffectsNotify()
{
    bBloomWindowActive = false;
    if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) { SwordBloom->HideAllEffects(); }
    UE_LOG(LogTemp, Warning, TEXT("🔒 Hide bloom effects notify called - Blueprint should handle visual effects"));
}

bool AAtlantisEonsCharacter::TryTriggerSparkEffect()
{
    // Check if we're currently in a bloom window (this is the main requirement)
    if (!bBloomWindowActive)
    {
        return false;
    }
    
    // Get the SwordBloom widget and check if we hit the critical window
    if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget())
    {
        bool bSuccessfulSpark = SwordBloom->TryTriggerSpark();
        
        if (bSuccessfulSpark)
        {
            UE_LOG(LogTemp, Warning, TEXT("✨ ✅ CRITICAL WINDOW HIT! Processing spark effect"));
            
            // If we're not already in a combo, start one
            if (!bIsInCombo)
            {
                bIsInCombo = true;
                UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ Starting combo chain from first attack"));
            }
            
            // Advance to the next attack in the combo
            AdvanceComboChain();
            
            // ========== SWORD EFFECT ACTIVATION LOGIC ==========
            // Activate SwordEffect when hitting critical windows on second montage and beyond
            if (CurrentAttackIndex == 2) // Only when transitioning from second to third montage
            {
                UE_LOG(LogTemp, Warning, TEXT("⚡ 🌟 Critical window hit on second montage - ACTIVATING SwordEffect for third montage!"));
                ActivateSwordEffect();
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("⚡ Critical window hit on attack %d - SwordEffect remains inactive"), CurrentAttackIndex);
            }
            
            // ========== DRAGON VISIBILITY LOGIC ==========
            // Show Dragons when transitioning to final attack (fourth montage)
            if (CurrentAttackIndex == 3) // Only when transitioning to final attack
            {
                UE_LOG(LogTemp, Warning, TEXT("🐉 Critical window hit on third montage - SHOWING Dragons for final attack!"));
                ShowDragons();
            }
            
            // Immediately start the next attack montage if available
            if (CurrentAttackIndex < MaxComboAttacks)
            {
                UAnimMontage* NextMontage = GetCurrentAttackMontage();
                if (NextMontage)
                {
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ CHAINING to attack montage %d: %s"), 
                           CurrentAttackIndex + 1, *NextMontage->GetName());
                    
                    // Stop current montage and play next one
                    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
                    {
                        AnimInstance->Montage_Stop(0.1f);
                    }
                    
                    // Small delay to ensure clean transition
                    FTimerHandle ChainTimer;
                    GetWorld()->GetTimerManager().SetTimer(
                        ChainTimer,
                        [this, NextMontage]()
                        {
                            UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ CHAINING TIMER CALLBACK: About to play %s for attack index %d"), 
                                   NextMontage ? *NextMontage->GetName() : TEXT("NULL"), CurrentAttackIndex);
                            
                            // Face the nearest enemy before chained attack (like in main attack function)
                            FaceNearestEnemy();
                            
                            // Reset attack notify flag for chained attacks
                            bAttackNotifyInProgress = false;
                            UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ Attack notify flag reset for chained attack"));
                            
                            float MontageLength = PlayAnimMontage(NextMontage);
                            if (MontageLength > 0.0f)
                            {
                                // Store the new montage length for bloom timing
                                CurrentMontageLength = MontageLength;
                                
                                UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ Successfully chained to next attack (duration: %.2f)"), MontageLength);
                                
                                // IMPORTANT: Start bloom circle for the chained attack immediately
                                UE_LOG(LogTemp, Warning, TEXT("🔵 ⚡ BLOOM WINDOW ACTIVATED - Starting for chained attack index %d (%s)"), 
                                       CurrentAttackIndex, NextMontage ? *NextMontage->GetName() : TEXT("NULL"));
                                
                                // CRITICAL FIX: Clear any existing bloom hide timers to prevent conflicts
                                GetWorld()->GetTimerManager().ClearTimer(SwordBloomHideTimer);
                                UE_LOG(LogTemp, Warning, TEXT("🔧 Cleared existing bloom hide timers to prevent conflicts"));
                                
                                bBloomWindowActive = true;
                                if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) 
                                { 
                                    SwordBloom->StartBloomCircle(); 
                                    UE_LOG(LogTemp, Warning, TEXT("🔵 ✅ SwordBloom widget found - StartBloomCircle() called for chained attack %s"), 
                                           NextMontage ? *NextMontage->GetName() : TEXT("NULL")); 
                                } 
                                else 
                                { 
                                    UE_LOG(LogTemp, Error, TEXT("🔵 ❌ SwordBloom widget NOT found for chained attack %s"), 
                                           NextMontage ? *NextMontage->GetName() : TEXT("NULL")); 
                                }
                                
                                // CRITICAL FIX: Add backup timer for chained attacks to ensure damage application
                                // This was missing and causing damage numbers to stop appearing after first combo hit
                                float NotifyTiming = MontageLength * 0.6f; // Call at 60% through the animation
                                FTimerHandle AttackNotifyTimer;
                                GetWorld()->GetTimerManager().SetTimer(
                                    AttackNotifyTimer,
                                    FTimerDelegate::CreateUObject(this, &AAtlantisEonsCharacter::OnMeleeAttackNotify),
                                    NotifyTiming,
                                    false
                                );
                                UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚡ Set backup attack notify timer for chained attack in %.2f seconds"), NotifyTiming);
                                
                                StartComboWindow(); // Restart the combo timing window
                            }
                            else
                            {
                                UE_LOG(LogTemp, Error, TEXT("🗡️ ❌ Failed to play chained montage %s for attack index %d"), 
                                       NextMontage ? *NextMontage->GetName() : TEXT("NULL"), CurrentAttackIndex);
                            }
                        },
                        0.05f,
                        false
                    );
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("🔚 Ending combo chain - resetting to first attack"));
                ResetComboChain();
            }
            
            return true;
        }
    }
    
    return false;
}

void AAtlantisEonsCharacter::StartBloomScaling()
{
    // This function now handled in Blueprint
    UE_LOG(LogTemp, Warning, TEXT("📈 StartBloomScaling called - implement in Blueprint"));
}

void AAtlantisEonsCharacter::UpdateBloomScaling()
{
    // This function now handled in Blueprint  
    UE_LOG(LogTemp, Warning, TEXT("📈 UpdateBloomScaling called - implement in Blueprint"));
}

void AAtlantisEonsCharacter::TriggerProgrammaticBloomEffect(float MontageLength)
{
    if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) { SwordBloom->StartBloomCircle(); UE_LOG(LogTemp, Warning, TEXT("🔵 ✅ SwordBloom widget found - StartBloomCircle() called")); } else { UE_LOG(LogTemp, Error, TEXT("🔵 ❌ SwordBloom widget NOT found - no visual effects will show")); }
    UE_LOG(LogTemp, Warning, TEXT("🎯 Programmatic bloom effect triggered via Blueprint for %.2f second montage"), MontageLength);
}

void AAtlantisEonsCharacter::CreateSwordBloomWidget()
{
    if (!SwordBloomWidgetComponent) return;
    
    UE_LOG(LogTemp, Warning, TEXT("🌟 CreateSwordBloomWidget: Starting widget setup..."));
    
    // First, try to get an existing widget if one is already set
    if (UUserWidget* ExistingWidget = SwordBloomWidgetComponent->GetUserWidgetObject())
    {
        SwordBloomWidget = Cast<UWBP_SwordBloom>(ExistingWidget);
        if (SwordBloomWidget)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ Found existing SwordBloom widget"));
            return;
        }
    }
    
    // Load the WBP_SwordBloom class with correct path
    UClass* SwordBloomClass = LoadClass<UWBP_SwordBloom>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/WBP_SwordBloom.WBP_SwordBloom_C"));
    
    if (SwordBloomClass)
    {
        SwordBloomWidgetComponent->SetWidgetClass(SwordBloomClass);
        
        // Set up widget component properties for proper display
        SwordBloomWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
        SwordBloomWidgetComponent->SetDrawSize(FVector2D(800.0f, 800.0f));  // Make it larger
        SwordBloomWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));  // Center pivot
        SwordBloomWidgetComponent->SetVisibility(true);
        SwordBloomWidgetComponent->SetHiddenInGame(false);
        SwordBloomWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        
        // Position it in screen center
        SwordBloomWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
        
        UE_LOG(LogTemp, Warning, TEXT("🌟 SwordBloom widget component configured - Size: 800x800, Screen space"));
        
        SwordBloomWidget = Cast<UWBP_SwordBloom>(SwordBloomWidgetComponent->GetUserWidgetObject());
        if (SwordBloomWidget)
        {
            UE_LOG(LogTemp, Warning, TEXT("✅ SwordBloom widget created and cast successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to cast widget to UWBP_SwordBloom"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to load WBP_SwordBloom class"));
    }
}

UWBP_SwordBloom* AAtlantisEonsCharacter::GetSwordBloomWidget()
{
    if (!SwordBloomWidget && SwordBloomWidgetComponent)
    {
        SwordBloomWidget = Cast<UWBP_SwordBloom>(SwordBloomWidgetComponent->GetUserWidgetObject()); if (SwordBloomWidget) { UE_LOG(LogTemp, Warning, TEXT("✅ GetSwordBloomWidget: Found widget via cast")); } else { UE_LOG(LogTemp, Warning, TEXT("❌ GetSwordBloomWidget: Cast failed")); }
    }
    return SwordBloomWidget;
}

// Animation Notify Functions

// Missing function implementations for linker

void AAtlantisEonsCharacter::ResetComboChain()
{
    CurrentAttackIndex = 0;
    bIsInCombo = false;
    bHitCriticalWindow = false;
    
    // CRITICAL FIX: Reset bloom window when combo chain ends
    bBloomWindowActive = false;
    UE_LOG(LogTemp, Warning, TEXT("🔧 Reset bloom window flag when resetting combo chain"));
    
    // ========== SWORD EFFECT DEACTIVATION LOGIC ==========
    // Deactivate SwordEffect when reverting to first attack
    UE_LOG(LogTemp, Warning, TEXT("🔒 🌟 Combo chain reset - DEACTIVATING SwordEffect"));
    DeactivateSwordEffect();
    
    // ========== DRAGON HIDING LOGIC ==========
    // Hide Dragons when combo ends
    UE_LOG(LogTemp, Warning, TEXT("🔒 🐉 Combo chain reset - HIDING Dragons"));
    HideDragons();
    
    // Clear combo reset timer
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimer);
    
    UE_LOG(LogTemp, Warning, TEXT("🔄 Combo chain reset to first attack"));
}

void AAtlantisEonsCharacter::AdvanceComboChain()
{
    if (CurrentAttackIndex < MaxComboAttacks - 1)
    {
        CurrentAttackIndex++;
        UE_LOG(LogTemp, Warning, TEXT("⚡ Advanced combo to attack %d/%d"), CurrentAttackIndex + 1, MaxComboAttacks);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚡ Reached maximum combo attacks, resetting chain"));
        ResetComboChain();
    }
}

bool AAtlantisEonsCharacter::CanContinueCombo() const
{
    return bIsInCombo && bHitCriticalWindow && (CurrentAttackIndex < MaxComboAttacks - 1);
}

void AAtlantisEonsCharacter::StartComboWindow()
{
    bIsInCombo = true;
    
    // Clear any existing combo reset timer
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimer);
    
    // Set timer to reset combo if no critical window is hit
    GetWorld()->GetTimerManager().SetTimer(
        ComboResetTimer,
        FTimerDelegate::CreateUObject(this, &AAtlantisEonsCharacter::EndComboChain),
        ComboWindowDuration,
        false
    );
    
    UE_LOG(LogTemp, Warning, TEXT("⏰ Started combo window (%.1fs to hit critical timing)"), ComboWindowDuration);
}

void AAtlantisEonsCharacter::EndComboChain()
{
    UE_LOG(LogTemp, Warning, TEXT("🔚 Ending combo chain - resetting to first attack"));
    
    // CRITICAL FIX: Reset bloom window when combo chain ends
    bBloomWindowActive = false;
    UE_LOG(LogTemp, Warning, TEXT("🔧 Reset bloom window flag when ending combo chain"));
    
    // ========== SWORD EFFECT DEACTIVATION LOGIC ==========
    // Deactivate SwordEffect when combo times out
    UE_LOG(LogTemp, Warning, TEXT("🔒 🌟 Combo chain timed out - DEACTIVATING SwordEffect"));
    DeactivateSwordEffect();
    
    // ========== DRAGON HIDING LOGIC ==========
    // Hide Dragons when combo times out
    UE_LOG(LogTemp, Warning, TEXT("🔒 🐉 Combo chain timed out - HIDING Dragons"));
    HideDragons();
    
    ResetComboChain();
}

void AAtlantisEonsCharacter::HandleDisarmItem(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, int32 ItemIndex)
{
    if (EquipmentComponent)
    {
        EquipmentComponent->HandleDisarmItem(ItemEquipSlot, StaticMeshID, ItemIndex);
    }
}

void AAtlantisEonsCharacter::OnDamageTakenEvent(float IncomingDamage, bool bIsAlive)
{
    UE_LOG(LogTemp, Log, TEXT("OnDamageTakenEvent: %.1f damage, alive: %s"), IncomingDamage, bIsAlive ? TEXT("Yes") : TEXT("No"));
    // Implementation for damage taken event
}

void AAtlantisEonsCharacter::OnHealthChangedEvent(float NewHealthPercent)
{
    UE_LOG(LogTemp, Log, TEXT("OnHealthChangedEvent: %.1f%%"), NewHealthPercent * 100.0f);
    // Implementation for health changed event
}

void AAtlantisEonsCharacter::OnCharacterDeathEvent()
{
    UE_LOG(LogTemp, Log, TEXT("OnCharacterDeathEvent called"));
    HandleDeath();
}

void AAtlantisEonsCharacter::OnInventoryChangedEvent()
{
    UE_LOG(LogTemp, Log, TEXT("OnInventoryChangedEvent called"));
    UpdateInventorySlots();
}

float AAtlantisEonsCharacter::GetMaxHealth() const
{
    return MaxHealth;
}

int32 AAtlantisEonsCharacter::GetCurrentDEX() const
{
    return CurrentDEX;
}

int32 AAtlantisEonsCharacter::GetCurrentINT() const
{
    return CurrentINT;
}

int32 AAtlantisEonsCharacter::GetCurrentSTR() const
{
    return CurrentSTR;
}

int32 AAtlantisEonsCharacter::GetCurrentDamage() const
{
    return CurrentDamage;
}

float AAtlantisEonsCharacter::GetCurrentHealth() const
{
    return CurrentHealth;
}

int32 AAtlantisEonsCharacter::GetCurrentDefence() const
{
    return CurrentDefence;
}

UAnimMontage* AAtlantisEonsCharacter::GetCurrentAttackMontage() const
{
    UAnimMontage* SelectedMontage = nullptr;
    
    switch (CurrentAttackIndex)
    {
        case 0:
            SelectedMontage = MeleeAttackMontage1;
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack Index 0 -> MeleeAttackMontage1: %s"), 
                   SelectedMontage ? *SelectedMontage->GetName() : TEXT("NULL"));
            break;
        case 1:
            SelectedMontage = MeleeAttackMontage2;
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack Index 1 -> MeleeAttackMontage2: %s"), 
                   SelectedMontage ? *SelectedMontage->GetName() : TEXT("NULL"));
            break;
        case 2:
            SelectedMontage = MeleeAttackMontage4; // Skip MeleeAttackMontage3
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack Index 2 -> MeleeAttackMontage4: %s"), 
                   SelectedMontage ? *SelectedMontage->GetName() : TEXT("NULL"));
            break;
        case 3:
            SelectedMontage = MeleeAttackMontage5;
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack Index 3 -> MeleeAttackMontage5: %s"), 
                   SelectedMontage ? *SelectedMontage->GetName() : TEXT("NULL"));
            break;
        default:
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Invalid attack index %d, returning first montage"), CurrentAttackIndex);
            SelectedMontage = MeleeAttackMontage1;
            break;
    }
    
    return SelectedMontage;
}

int32 AAtlantisEonsCharacter::GetGold() const
{
    return YourGold;
}

int32 AAtlantisEonsCharacter::GetMaxMP() const
{
    return MaxMP;
}

void AAtlantisEonsCharacter::UnequipInventoryItem(UBP_ItemInfo* ItemInfoRef)
{
    if (EquipmentComponent)
    {
        // Pass character reference directly to bypass world context issues
        EquipmentComponent->UnequipInventoryItemWithCharacter(ItemInfoRef, this);
    }
}

// ========== SWORD EFFECT NIAGARA SYSTEM MANAGEMENT ==========

void AAtlantisEonsCharacter::FindSwordEffectComponent()
{
    UE_LOG(LogTemp, Warning, TEXT("🔍 FindSwordEffectComponent: Searching for SwordEffect Niagara component..."));
    
    // Try to find the component by name from all components
    TArray<UActorComponent*> Components;
    GetComponents<UActorComponent>(Components);
    
    for (UActorComponent* Component : Components)
    {
        if (Component && Component->GetName() == TEXT("SwordEffect"))
        {
            SwordEffectComponent = Cast<UNiagaraComponent>(Component);
            if (SwordEffectComponent)
            {
                UE_LOG(LogTemp, Warning, TEXT("✅ Found SwordEffect Niagara component by name!"));
                
                // Ensure it starts deactivated
                SwordEffectComponent->Deactivate();
                UE_LOG(LogTemp, Warning, TEXT("🔧 SwordEffect component deactivated on startup"));
                return;
            }
        }
    }
    
    // Alternative search by class type
    UNiagaraComponent* FoundNiagara = FindComponentByClass<UNiagaraComponent>();
    if (FoundNiagara && FoundNiagara->GetName().Contains(TEXT("SwordEffect")))
    {
        SwordEffectComponent = FoundNiagara;
        UE_LOG(LogTemp, Warning, TEXT("✅ Found SwordEffect Niagara component by class search!"));
        
        // Ensure it starts deactivated
        SwordEffectComponent->Deactivate();
        UE_LOG(LogTemp, Warning, TEXT("🔧 SwordEffect component deactivated on startup"));
        return;
    }
    
    UE_LOG(LogTemp, Error, TEXT("❌ Could not find SwordEffect Niagara component! Make sure it's named 'SwordEffect' in the Blueprint."));
}

void AAtlantisEonsCharacter::ActivateSwordEffect()
{
    if (SwordEffectComponent)
    {
        if (!SwordEffectComponent->IsActive())
        {
            SwordEffectComponent->Activate();
            UE_LOG(LogTemp, Warning, TEXT("⚡ SwordEffect Niagara system ACTIVATED for combo chain!"));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("⚡ SwordEffect already active"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Cannot activate SwordEffect - component not found!"));
        // Try to find it again
        FindSwordEffectComponent();
    }
}

void AAtlantisEonsCharacter::DeactivateSwordEffect()
{
    if (SwordEffectComponent)
    {
        if (SwordEffectComponent->IsActive())
        {
            SwordEffectComponent->Deactivate();
            UE_LOG(LogTemp, Warning, TEXT("🔒 SwordEffect Niagara system DEACTIVATED - combo ended"));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("🔒 SwordEffect already inactive"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Cannot deactivate SwordEffect - component not found!"));
    }
}

bool AAtlantisEonsCharacter::ShouldSwordEffectBeActive() const
{
    // SwordEffect should be active when:
    // 1. We're in a combo (bIsInCombo = true)
    // 2. We're on the third montage or higher (CurrentAttackIndex >= 2)
    bool bShouldBeActive = bIsInCombo && (CurrentAttackIndex >= 2);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("🔍 ShouldSwordEffectBeActive: InCombo=%s, AttackIndex=%d, Result=%s"),
           bIsInCombo ? TEXT("Yes") : TEXT("No"), CurrentAttackIndex, bShouldBeActive ? TEXT("Yes") : TEXT("No"));
    
    return bShouldBeActive;
}

// ========== DRAGON SKELETAL MESH VISIBILITY MANAGEMENT ==========

void AAtlantisEonsCharacter::FindDragonComponents()
{
    UE_LOG(LogTemp, Warning, TEXT("🔍 FindDragonComponents: Searching for Dragon1 and Dragon2 skeletal mesh components..."));
    
    // Try to find the components by name from all components
    TArray<UActorComponent*> Components;
    GetComponents<UActorComponent>(Components);
    
    for (UActorComponent* Component : Components)
    {
        if (Component)
        {
            // Check for Dragon1
            if (Component->GetName() == TEXT("Dragon1"))
            {
                Dragon1Component = Cast<USkeletalMeshComponent>(Component);
                if (Dragon1Component)
                {
                    UE_LOG(LogTemp, Warning, TEXT("✅ Found Dragon1 skeletal mesh component by name!"));
                    
                    // Ensure it starts hidden
                    Dragon1Component->SetVisibility(false);
                    UE_LOG(LogTemp, Warning, TEXT("🔧 Dragon1 component hidden on startup"));
                }
            }
            // Check for Dragon2
            else if (Component->GetName() == TEXT("Dragon2"))
            {
                Dragon2Component = Cast<USkeletalMeshComponent>(Component);
                if (Dragon2Component)
                {
                    UE_LOG(LogTemp, Warning, TEXT("✅ Found Dragon2 skeletal mesh component by name!"));
                    
                    // Ensure it starts hidden
                    Dragon2Component->SetVisibility(false);
                    UE_LOG(LogTemp, Warning, TEXT("🔧 Dragon2 component hidden on startup"));
                }
            }
        }
    }
    
    // Log results
    if (!Dragon1Component)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Could not find Dragon1 skeletal mesh component! Make sure it's named 'Dragon1' in the Blueprint."));
    }
    if (!Dragon2Component)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Could not find Dragon2 skeletal mesh component! Make sure it's named 'Dragon2' in the Blueprint."));
    }
}

void AAtlantisEonsCharacter::ShowDragons()
{
    bool bShowedAny = false;
    
    if (Dragon1Component)
    {
        Dragon1Component->SetVisibility(true);
        UE_LOG(LogTemp, Warning, TEXT("🐉 Dragon1 skeletal mesh SHOWN for final attack!"));
        bShowedAny = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Cannot show Dragon1 - component not found!"));
    }
    
    if (Dragon2Component)
    {
        Dragon2Component->SetVisibility(true);
        UE_LOG(LogTemp, Warning, TEXT("🐉 Dragon2 skeletal mesh SHOWN for final attack!"));
        bShowedAny = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Cannot show Dragon2 - component not found!"));
    }
    
    if (!bShowedAny)
    {
        // Try to find them again
        FindDragonComponents();
    }
}

void AAtlantisEonsCharacter::HideDragons()
{
    if (Dragon1Component)
    {
        Dragon1Component->SetVisibility(false);
        UE_LOG(LogTemp, Warning, TEXT("🔒 Dragon1 skeletal mesh HIDDEN - final attack ended"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Cannot hide Dragon1 - component not found!"));
    }
    
    if (Dragon2Component)
    {
        Dragon2Component->SetVisibility(false);
        UE_LOG(LogTemp, Warning, TEXT("🔒 Dragon2 skeletal mesh HIDDEN - final attack ended"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Cannot hide Dragon2 - component not found!"));
    }
}

bool AAtlantisEonsCharacter::ShouldDragonsBeVisible() const
{
    // Dragons should be visible when:
    // 1. We're in a combo (bIsInCombo = true)
    // 2. We're on the final attack (CurrentAttackIndex == 3)
    bool bShouldBeVisible = bIsInCombo && (CurrentAttackIndex == 3);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("🔍 ShouldDragonsBeVisible: InCombo=%s, AttackIndex=%d, Result=%s"),
           bIsInCombo ? TEXT("Yes") : TEXT("No"), CurrentAttackIndex, bShouldBeVisible ? TEXT("Yes") : TEXT("No"));
    
    return bShouldBeVisible;
}




