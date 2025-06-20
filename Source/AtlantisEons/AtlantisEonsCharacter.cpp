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
#include "Components/PrimitiveComponent.h"
#include "BP_Item.h"
#include "StoreSystemFix.h"
#include "UniversalItemLoader.h"
#include "Engine/OverlapResult.h"
#include "WBP_SwordBloom.h"
#include "AtlantisEonsGameMode.h"
#include "InventoryComponent.h"
#include "CharacterStatsComponent.h"
#include "EquipmentComponent.h"
#include "CharacterUIManager.h"
#include "CameraStabilizationComponent.h"
#include "CombatEffectsManagerComponent.h"


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
    
    // ENHANCED: Configure collision for proper pawn blocking and attack stability
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block); // Block by default
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // Block other pawns
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Block visibility traces
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // Block world geometry
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // Block dynamic objects
    
    // BALANCED: Physics prevention with reasonable mass
    GetCapsuleComponent()->SetSimulatePhysics(false);
    GetCapsuleComponent()->SetEnableGravity(false); // Let character movement handle gravity
    GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, 1000.0f, true); // Heavy enough to resist pushing
    GetCapsuleComponent()->SetLinearDamping(30.0f); // High damping for stability
    GetCapsuleComponent()->SetAngularDamping(30.0f); // High rotational damping
    GetCapsuleComponent()->SetUseCCD(true); // Enable CCD to prevent penetration during attacks
    GetCapsuleComponent()->SetNotifyRigidBodyCollision(false); // Disable collision events for performance
    
    // ENHANCED: Disable physics simulation to prevent flying when hit
    GetCapsuleComponent()->SetSimulatePhysics(false);
    GetCapsuleComponent()->SetEnableGravity(false); // Let character movement handle gravity
    
    // ENHANCED: Configure mesh to prevent clipping during attacks
    if (GetMesh())
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // CRITICAL: Don't collide with enemy meshes
        GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Allow visibility traces
        GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera
        
        UE_LOG(LogTemp, Warning, TEXT("Player: Mesh collision configured to IGNORE pawns - prevents clipping during attacks"));
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
        
        // BALANCED: Physics resistance settings that work with enemy collision
        CharMov->bIgnoreBaseRotation = true;
        CharMov->Mass = 1000.0f; // Heavy enough to resist impulses but not excessive
        CharMov->GroundFriction = 8.0f; // Good friction without being too sticky
        CharMov->MaxAcceleration = 2500.0f; // Responsive but controlled acceleration
        CharMov->BrakingDecelerationWalking = 2500.0f; // Good braking for stability
        CharMov->bApplyGravityWhileJumping = true; // Keep normal jumping
        CharMov->bCanWalkOffLedges = true; // Allow normal movement
        CharMov->bRequestedMoveUseAcceleration = true; // Use acceleration-based movement
        CharMov->bForceMaxAccel = false; // Controlled acceleration
        
        UE_LOG(LogTemp, Warning, TEXT("Player: BALANCED movement physics - Mass: 1000kg, Friction: 8.0"));
    }

    // Initialize character stats
    BaseHealth = 100.0f;
    CurrentHealth = BaseHealth;
    BaseMP = 100;
    CurrentMP = BaseMP;
    MaxMP = BaseMP;
    


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
    UIManager = CreateDefaultSubobject<UCharacterUIManager>(TEXT("UIManager"));
    CameraStabilizationComp = CreateDefaultSubobject<UCameraStabilizationComponent>(TEXT("CameraStabilizationComp"));
    EquipmentVisualsComp = CreateDefaultSubobject<UEquipmentVisualsComponent>(TEXT("EquipmentVisualsComp"));
    ItemDataCreationComp = CreateDefaultSubobject<UItemDataCreationComponent>(TEXT("ItemDataCreationComp"));
    InventoryManagerComp = CreateDefaultSubobject<UInventoryManagerComponent>(TEXT("InventoryManagerComp"));
    CombatEffectsManagerComp = CreateDefaultSubobject<UCombatEffectsManagerComponent>(TEXT("CombatEffectsManagerComp"));

    
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
    if (InventoryManagerComp)
    {
        InventoryManagerComp->SetInventoryToggleLock(bLock, UnlockDelay);
    }
    
    // Keep legacy support for direct access
    bInventoryToggleLocked = bLock;
}

void AAtlantisEonsCharacter::ForceSetInventoryState(bool bNewIsOpen)
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->ForceSetInventoryState(bNewIsOpen);
    }
    
    // Keep legacy support for direct access
    bIsInventoryOpen = bNewIsOpen;
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
        
        // ENHANCED: Strong collision blocking to prevent overlapping with enemies
        GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // Block pawns (enemies)
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Block visibility traces
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // Block world geometry
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // Block dynamic objects
        
        // BALANCED: Physics prevention settings that work with enemy collision
        GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, 1000.0f, true); // Heavy enough to resist pushing
        GetCapsuleComponent()->SetLinearDamping(30.0f); // High damping for stability
        GetCapsuleComponent()->SetAngularDamping(30.0f); // High rotational damping
        GetCapsuleComponent()->SetUseCCD(true); // Enable CCD to prevent penetration during attacks
        GetCapsuleComponent()->SetNotifyRigidBodyCollision(false); // Disable collision events for performance
        

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
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // CRITICAL: Don't collide with enemy meshes
        GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Support visibility traces
        GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera
        
        // Ensure mesh visibility
        GetMesh()->SetVisibility(true);
        GetMesh()->SetHiddenInGame(false);
        
        UE_LOG(LogTemp, Warning, TEXT("Player: Mesh collision configured to IGNORE enemy meshes - prevents attack clipping"));
    }
    
    // BALANCED: Character movement configuration for stable combat
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->bIgnoreBaseRotation = true;
        GetCharacterMovement()->Mass = 1000.0f; // Heavy enough to resist impulses without being excessive
        GetCharacterMovement()->GroundFriction = 8.0f; // Good friction for stability without stickiness
        GetCharacterMovement()->MaxAcceleration = 2500.0f; // Responsive but controlled acceleration
        GetCharacterMovement()->BrakingDecelerationWalking = 2500.0f; // Good braking for stability
        GetCharacterMovement()->bApplyGravityWhileJumping = true; // Keep normal jumping
        GetCharacterMovement()->bCanWalkOffLedges = true; // Allow normal movement
        GetCharacterMovement()->bRequestedMoveUseAcceleration = true; // Use acceleration-based movement
        GetCharacterMovement()->bForceMaxAccel = false; // Controlled acceleration
        
        UE_LOG(LogTemp, Warning, TEXT("Player: BALANCED movement physics - Mass: 1000kg, Friction: 8.0"));
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

void AAtlantisEonsCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up SecondaryHUD widget
    DestroySecondaryHUD();
    
    Super::EndPlay(EndPlayReason);
    
    UE_LOG(LogTemp, Warning, TEXT("Player EndPlay: Character cleanup complete"));
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
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AAtlantisEonsCharacter::StopMoving);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Move action (Triggered + Completed)"));
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
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Character - MeleeAttackAction is null! Combat won't work"));
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("WARNING: MeleeAttackAction not set in BP_Character!"));
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



        // Block
        if (BlockAction)
        {
            EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::PerformBlock);
            EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Completed, this, &AAtlantisEonsCharacter::ReleaseBlock);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Block action (Started + Completed)"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Character - BlockAction is null! Block functionality won't work"));
        }

        // Zoom
        if (ZoomAction)
        {
            EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AAtlantisEonsCharacter::Zoom);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Zoom action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Character - ZoomAction is null! Zoom functionality won't work"));
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
    
    // BREAKABLE STABILIZATION: Track movement input for camera stabilization override
    CurrentMovementInput = MovementVector;
    bool bWasTryingToMove = bIsPlayerTryingToMove;
    const float MovementInputThreshold = 0.1f; // Minimum input magnitude to trigger movement
    bIsPlayerTryingToMove = MovementVector.Size() > MovementInputThreshold;
    
    // Movement input tracking is now handled by CameraStabilizationComponent

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

void AAtlantisEonsCharacter::StopMoving(const FInputActionValue& Value)
{
    // Clear the movement input when all movement keys are released
    CurrentMovementInput = FVector2D::ZeroVector;
    bIsPlayerTryingToMove = false;
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 MOVEMENT STOPPED: CurrentMovementInput cleared to (0,0)"));
}

bool AAtlantisEonsCharacter::IsAnyMovementKeyPressed() const
{
    return bWKeyPressed || bAKeyPressed || bSKeyPressed || bDKeyPressed || bDashKeyPressed;
}

void AAtlantisEonsCharacter::UpdateInputState(bool bW, bool bA, bool bS, bool bD, bool bDash)
{
    bWKeyPressed = bW;
    bAKeyPressed = bA;
    bSKeyPressed = bS;
    bDKeyPressed = bD;
    bDashKeyPressed = bDash;
    
    // Update the legacy tracking variable for backwards compatibility
    bIsPlayerTryingToMove = IsAnyMovementKeyPressed();
    
    UE_LOG(LogTemp, Verbose, TEXT("🎮 Input State Updated: W=%d A=%d S=%d D=%d Dash=%d"), 
           bW ? 1 : 0, bA ? 1 : 0, bS ? 1 : 0, bD ? 1 : 0, bDash ? 1 : 0);
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

void AAtlantisEonsCharacter::Zoom(const FInputActionValue& Value)
{
    // Input is a scalar (float) for mouse wheel
    float ZoomValue = Value.Get<float>();
    
    // Always log zoom input to help debug
    UE_LOG(LogTemp, Warning, TEXT("🔍 ZOOM INPUT RECEIVED: Value=%.2f"), ZoomValue);
    
    if (CameraBoom)
    {
        // Calculate the new target arm length
        float CurrentDistance = CameraBoom->TargetArmLength;
        float NewDistance = CurrentDistance - (ZoomValue * ZoomSpeed);
        
        // Clamp the distance to our min/max values
        NewDistance = FMath::Clamp(NewDistance, MinZoomDistance, MaxZoomDistance);
        
        // Apply the new distance
        CameraBoom->TargetArmLength = NewDistance;
        
        UE_LOG(LogTemp, Warning, TEXT("🔍 Camera Zoom Applied: Input=%.2f, Distance: %.1f -> %.1f, Speed=%.1f, Range=[%.1f-%.1f]"), 
               ZoomValue, CurrentDistance, NewDistance, ZoomSpeed, MinZoomDistance, MaxZoomDistance);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🔍 ZOOM FAILED: CameraBoom is NULL!"));
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
    if (UIManager)
    {
        UIManager->InitializeUI();
    }
    
    // Create Secondary HUD if class is assigned
    CreateSecondaryHUD();
}

void AAtlantisEonsCharacter::ConnectInventoryToMainWidget()
{
    if (UIManager)
    {
        UIManager->ConnectInventoryToMainWidget();
    }
}

void AAtlantisEonsCharacter::SetupCircularBars()
{
    if (UIManager)
    {
        UIManager->SetupCircularBars();
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
    
    // Initialize Equipment Visuals Component with mesh references
    if (EquipmentVisualsComp)
    {
        EquipmentVisualsComp->InitializeEquipmentMeshes(Helmet, Weapon, Shield);
        UE_LOG(LogTemp, Warning, TEXT("🔧 EquipmentVisualsComponent: Initialized with equipment meshes"));
    }
    
    // CRITICAL: Initialize CombatEffectsManagerComponent with character reference
    if (CombatEffectsManagerComp)
    {
        CombatEffectsManagerComp->InitializeWithCharacter(this);
        
        // Bind to damage dealt events for damage numbers
        CombatEffectsManagerComp->OnDamageDealt.AddDynamic(this, &AAtlantisEonsCharacter::OnDamageDealtFromCombat);
        
        UE_LOG(LogTemp, Warning, TEXT("🗡️ CombatEffectsManagerComponent: Initialized with character reference and damage delegate"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ CombatEffectsManagerComponent is null! Combat will not work!"));
    }
    
    // SwordBloom widget is now handled entirely in Blueprint
}

void AAtlantisEonsCharacter::RecoverHealth(int32 Amount)
{
    CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
    SettingCircularBar_HP();
}

void AAtlantisEonsCharacter::RecoverMP(int32 Amount)
{
    CurrentMP = FMath::Clamp(CurrentMP + Amount, 0, MaxMP);
    SettingCircularBar_MP();
}

bool AAtlantisEonsCharacter::HasEnoughMana(int32 ManaCost) const
{
    return CurrentMP >= ManaCost;
}

void AAtlantisEonsCharacter::ConsumeMana(int32 ManaCost)
{
    if (ManaCost <= 0) return;
    
    CurrentMP = FMath::Max(CurrentMP - ManaCost, 0);
    SettingCircularBar_MP();
    
    UE_LOG(LogTemp, Log, TEXT("Mana consumed: %d (%d/%d remaining)"), ManaCost, CurrentMP, MaxMP);
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
    if (UIManager)
    {
        UIManager->SettingCircularBar_HP();
    }
}

void AAtlantisEonsCharacter::SettingCircularBar_MP()
{
    if (UIManager)
    {
        UIManager->SettingCircularBar_MP();
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
    if (UIManager)
    {
        UIManager->RefreshStatsDisplay();
    }
}

void AAtlantisEonsCharacter::SettingStore()
{
    if (UIManager)
    {
        UIManager->SettingStore();
    }
}

void AAtlantisEonsCharacter::SetInventorySlot(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotWidgetRef)
{
    if (UIManager)
    {
        UIManager->SetInventorySlot(ItemInfoRef, InventorySlotWidgetRef);
    }
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
    if (InventoryManagerComp)
    {
        return InventoryManagerComp->PickingItem(ItemIndex, ItemStackNumber);
    }
    return false;
}

void AAtlantisEonsCharacter::ContextMenuUse(UWBP_InventorySlot* InventorySlot)
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->ContextMenuUse(InventorySlot);
    }
}

void AAtlantisEonsCharacter::ContextMenuThrow(UWBP_InventorySlot* InventorySlot)
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->ContextMenuThrow(InventorySlot);
    }
}

void AAtlantisEonsCharacter::ContextMenuUse_EquipItem(UBP_ItemInfo* ItemInfoRef)
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->ContextMenuUse_EquipItem(ItemInfoRef);
    }
}

void AAtlantisEonsCharacter::ContextMenuUse_ConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->ContextMenuUse_ConsumeItem(ItemInfoRef, InventorySlotRef, RecoverHP, RecoverMP, ItemType);
    }
}

bool AAtlantisEonsCharacter::BuyingItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice)
{
    if (InventoryManagerComp)
    {
        return InventoryManagerComp->BuyingItem(ItemIndex, ItemStackNumber, ItemPrice);
    }
    return false;
}

void AAtlantisEonsCharacter::EquipItemInSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, const TSoftObjectPtr<UTexture2D>& Texture2D, int32 ItemIndex, UMaterialInterface* MaterialInterface, UMaterialInterface* MaterialInterface2)
{
    // Delegate to Equipment Component for logic and data management
    if (EquipmentComponent)
    {
        EquipmentComponent->EquipItemInSlot(ItemEquipSlot, StaticMeshID, Texture2D, ItemIndex, MaterialInterface, MaterialInterface2);
    }
    
    // Delegate to Equipment Visuals Component for visual updates
    if (EquipmentVisualsComp)
    {
        EquipmentVisualsComp->EquipItemToSlot(ItemEquipSlot, StaticMeshID, Texture2D, ItemIndex, MaterialInterface, MaterialInterface2);
    }
}

void AAtlantisEonsCharacter::ProcessEquipItem(UBP_ItemInfo* ItemInfoRef)
{
    // Delegate to Equipment Component for logic and data management
    if (EquipmentComponent)
    {
        EquipmentComponent->ProcessEquipItem(ItemInfoRef);
    }
    
    // Delegate to Equipment Visuals Component for visual updates
    if (EquipmentVisualsComp)
    {
        EquipmentVisualsComp->ProcessEquipmentChange(ItemInfoRef);
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
    UE_LOG(LogTemp, Warning, TEXT("🗡️ CHARACTER: MeleeAttack input received at %.3f seconds!"), GetWorld()->GetTimeSeconds());
    UE_LOG(LogTemp, Warning, TEXT("🗡️ CHARACTER: Input value magnitude: %.3f"), Value.GetMagnitude());
    
    if (CombatEffectsManagerComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ CHARACTER: CombatEffectsManagerComp found - checking bloom state"));
        bool bComponentBloomActive = CombatEffectsManagerComp->IsBloomWindowActive();
        UE_LOG(LogTemp, Warning, TEXT("🗡️ CHARACTER: Component bloom window active = %s"), bComponentBloomActive ? TEXT("TRUE") : TEXT("FALSE"));
        
        UE_LOG(LogTemp, Warning, TEXT("🗡️ CHARACTER: Delegating to CombatEffectsManagerComp->MeleeAttack()"));
        CombatEffectsManagerComp->MeleeAttack(Value);
        
        // Sync legacy state variables for Blueprint compatibility
        bCanAttack = CombatEffectsManagerComp->CanAttack();
        bIsAttacking = CombatEffectsManagerComp->IsAttacking();
        CurrentAttackIndex = CombatEffectsManagerComp->GetCurrentAttackIndex();
        bIsInCombo = CombatEffectsManagerComp->IsInCombo();
        bBloomWindowActive = CombatEffectsManagerComp->IsBloomWindowActive();
        
        UE_LOG(LogTemp, Warning, TEXT("🗡️ CHARACTER: State synced - CanAttack=%s, IsAttacking=%s, AttackIndex=%d"), 
               bCanAttack ? TEXT("Yes") : TEXT("No"),
               bIsAttacking ? TEXT("Yes") : TEXT("No"),
               CurrentAttackIndex);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🗡️ CHARACTER: CombatEffectsManagerComp is null! Combat won't work"));
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ERROR: CombatEffectsManagerComp is null!"));
    }
}

void AAtlantisEonsCharacter::ResetAttack()
{
    if (CombatEffectsManagerComp)
    {
        CombatEffectsManagerComp->ResetAttack();
        
        // Sync legacy state variables for Blueprint compatibility
        bCanAttack = CombatEffectsManagerComp->CanAttack();
        bIsAttacking = CombatEffectsManagerComp->IsAttacking();
        CurrentAttackIndex = CombatEffectsManagerComp->GetCurrentAttackIndex();
        bIsInCombo = CombatEffectsManagerComp->IsInCombo();
        bBloomWindowActive = CombatEffectsManagerComp->IsBloomWindowActive();
    }
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
    if (UIManager)
    {
        UIManager->ClearAllInventoryPopups();
    }
}

void AAtlantisEonsCharacter::ToggleInventory(const FInputActionValue& Value)
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->ToggleInventory(Value);
        
        // Sync legacy state
        bIsInventoryOpen = InventoryManagerComp->IsInventoryOpen();
        bInventoryToggleLocked = InventoryManagerComp->IsInventoryToggleLocked();
    }
}

void AAtlantisEonsCharacter::UnlockInventoryToggle()
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->UnlockInventoryToggle();
    }
    bInventoryToggleLocked = false;
}

void AAtlantisEonsCharacter::OpenInventory()
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->OpenInventory();
        
        // Sync legacy state
        bIsInventoryOpen = InventoryManagerComp->IsInventoryOpen();
        if (InventoryManagerComp->GetMainWidget())
        {
            MainWidget = InventoryManagerComp->GetMainWidget();
        }
    }
}

void AAtlantisEonsCharacter::UpdateInventorySlots()
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->UpdateInventorySlots();
    }
}

void AAtlantisEonsCharacter::DelayedUpdateInventorySlots()
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->DelayedUpdateInventorySlots();
    }
}

void AAtlantisEonsCharacter::ForceUpdateInventorySlotsAfterDelay()
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->ForceUpdateInventorySlotsAfterDelay();
    }
}

void AAtlantisEonsCharacter::CloseInventory()
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->CloseInventory();
        
        // Sync legacy state
        bIsInventoryOpen = InventoryManagerComp->IsInventoryOpen();
    }
}

FStructure_ItemInfo AAtlantisEonsCharacter::CreateHardcodedItemData(int32 ItemIndex)
{
    // Delegate to Item Data Creation Component for comprehensive item database
    if (ItemDataCreationComp)
    {
        return ItemDataCreationComp->CreateItemData(ItemIndex);
    }
    
    // Fallback if component is not available
    UE_LOG(LogTemp, Warning, TEXT("ItemDataCreationComp is null, creating basic fallback item"));
    FStructure_ItemInfo ItemInfo;
    ItemInfo.ItemIndex = ItemIndex;
    ItemInfo.ItemName = FString::Printf(TEXT("Fallback Item %d"), ItemIndex);
    ItemInfo.ItemDescription = TEXT("Basic fallback item data.");
    ItemInfo.bIsValid = false;
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
    if (UIManager)
    {
        UIManager->SetMainWidget(NewWidget);
    }
    
    // Also set the legacy reference for backward compatibility
    if (NewWidget)
    {
        MainWidget = NewWidget;
        Main = NewWidget;
        
        // Get the character info widget from the main widget
        if (MainWidget->GetCharacterInfoWidget())
        {
            WBP_CharacterInfo = MainWidget->GetCharacterInfoWidget();
        }
    }
}

void AAtlantisEonsCharacter::UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo)
{
    // Delegate to Equipment Component for logic
    // Only delegate to Equipment Visuals Component for UI updates to avoid duplication
    if (EquipmentVisualsComp)
    {
        EquipmentVisualsComp->UpdateEquipmentSlotUI(EquipSlot, ItemInfo);
    }
}

void AAtlantisEonsCharacter::ClearEquipmentSlotUI(EItemEquipSlot EquipSlot)
{
    // Only delegate to Equipment Visuals Component for UI updates to avoid duplication
    if (EquipmentVisualsComp)
    {
        EquipmentVisualsComp->ClearEquipmentSlotUI(EquipSlot);
    }
}

void AAtlantisEonsCharacter::UpdateAllEquipmentSlotsUI()
{
    // Delegate to UI Manager for main UI updates
    if (UIManager)
    {
        UIManager->UpdateAllEquipmentSlotsUI();
    }
    
    // Delegate to Equipment Visuals Component for visual updates
    if (EquipmentVisualsComp)
    {
        EquipmentVisualsComp->UpdateAllEquipmentSlotsUI();
    }
}

void AAtlantisEonsCharacter::InitializeEquipmentSlotReferences()
{
    if (UIManager)
    {
        UIManager->InitializeEquipmentSlotReferences();
    }
}

void AAtlantisEonsCharacter::OnEquipmentSlotClicked(EItemEquipSlot EquipSlot)
{
    // Delegate to Equipment Component for logic
    if (EquipmentComponent)
    {
        EquipmentComponent->OnEquipmentSlotClicked(EquipSlot);
    }
    
    // Delegate to Equipment Visuals Component for visual feedback
    if (EquipmentVisualsComp)
    {
        EquipmentVisualsComp->HandleEquipmentSlotClicked(EquipSlot);
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
    if (CombatEffectsManagerComp)
    {
        CombatEffectsManagerComp->OnMeleeAttackNotify();
        
        // Sync legacy state variables for Blueprint compatibility
        bAttackNotifyInProgress = CombatEffectsManagerComp->IsAttackNotifyInProgress();
        CurrentMontageLength = CombatEffectsManagerComp->GetCurrentMontageLength();
    }
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
    if (CombatEffectsManagerComp)
    {
        CombatEffectsManagerComp->FaceNearestEnemy();
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
    if (InventoryManagerComp)
    {
        InventoryManagerComp->OnContextMenuUse(ItemInfoRef, InventorySlot);
    }
}

void AAtlantisEonsCharacter::OnContextMenuThrow(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot)
{
    if (InventoryManagerComp)
    {
        InventoryManagerComp->OnContextMenuThrow(ItemInfoRef, InventorySlot);
    }
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
    if (UWBP_SwordBloom* SwordBloom = GetSwordBloomWidget()) { SwordBloom->HideAllEffects(); }
    UE_LOG(LogTemp, Warning, TEXT("🔒 Hide bloom effects notify called - Blueprint should handle visual effects"));
}

bool AAtlantisEonsCharacter::TryTriggerSparkEffect()
{
    // DELEGATE to CombatEffectsManagerComponent for bloom window management
    if (CombatEffectsManagerComp)
    {
        return CombatEffectsManagerComp->TryTriggerSparkEffect();
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
    if (CombatEffectsManagerComp)
    {
        CombatEffectsManagerComp->ResetComboChain();
        
        // Sync legacy state variables for Blueprint compatibility
        CurrentAttackIndex = CombatEffectsManagerComp->GetCurrentAttackIndex();
        bIsInCombo = CombatEffectsManagerComp->IsInCombo();
        bHitCriticalWindow = CombatEffectsManagerComp->HasHitCriticalWindow();
        bBloomWindowActive = CombatEffectsManagerComp->IsBloomWindowActive();
    }
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
    if (CombatEffectsManagerComp)
    {
        CombatEffectsManagerComp->StartComboWindow();
        
        // Sync legacy state variables for Blueprint compatibility
        bIsInCombo = CombatEffectsManagerComp->IsInCombo();
    }
}

void AAtlantisEonsCharacter::EndComboChain()
{
    UE_LOG(LogTemp, Warning, TEXT("🔚 Ending combo chain - resetting to first attack"));
    
    // CAMERA STABILIZATION: Disable when combo chain ends
    DisableAttackCameraStabilization();
    
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
    // Delegate to Equipment Component for logic and data management
    if (EquipmentComponent)
    {
        EquipmentComponent->HandleDisarmItem(ItemEquipSlot, StaticMeshID, ItemIndex);
    }
    
    // Delegate to Equipment Visuals Component for visual updates
    if (EquipmentVisualsComp)
    {
        EquipmentVisualsComp->DisarmItemFromSlot(ItemEquipSlot, StaticMeshID, ItemIndex);
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

void AAtlantisEonsCharacter::OnDamageDealtFromCombat(float DealtDamage, AActor* Target)
{
    UE_LOG(LogTemp, Warning, TEXT("🗡️ OnDamageDealtFromCombat: %.1f damage to %s - Event received from CombatEffectsManagerComponent"), 
           DealtDamage, Target ? *Target->GetName() : TEXT("NULL"));
    
    // NOTE: This event is for game logic purposes (achievements, sound effects, etc.)
    // The CombatEffectsManagerComponent already handles damage number spawning
    // to prevent duplicate damage numbers.
    
    // TODO: Add any additional logic here for when damage is dealt:
    // - Achievement tracking
    // - Sound effects that aren't handled by the component
    // - Screen shake effects
    // - Other game responses to damage dealt that need character-level handling
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

// ========== EXPERIENCE SYSTEM FUNCTIONS ==========

void AAtlantisEonsCharacter::AddExperience(int32 ExpAmount)
{
    if (StatsComponent)
    {
        StatsComponent->AddExperience(ExpAmount);
        
        // Also award experience for killing enemies - this could be called when enemies die
        UE_LOG(LogTemp, Log, TEXT("AtlantisEonsCharacter: Added %d experience"), ExpAmount);
    }
}

int32 AAtlantisEonsCharacter::GetPlayerLevel() const
{
    if (StatsComponent)
    {
        return StatsComponent->GetPlayerLevel();
    }
    return 1;
}

int32 AAtlantisEonsCharacter::GetCurrentExp() const
{
    if (StatsComponent)
    {
        return StatsComponent->GetCurrentExp();
    }
    return 0;
}

int32 AAtlantisEonsCharacter::GetExpForNextLevel() const
{
    if (StatsComponent)
    {
        return StatsComponent->GetExpForNextLevel();
    }
    return 100;
}

float AAtlantisEonsCharacter::GetExpPercentage() const
{
    if (StatsComponent)
    {
        return StatsComponent->GetExpPercentage();
    }
    return 0.0f;
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

// ========== CAMERA STABILIZATION - ROOT MOTION CONTROL ==========

void AAtlantisEonsCharacter::EnableAttackCameraStabilization()
{
    if (CameraStabilizationComp)
    {
        CameraStabilizationComp->EnableAttackCameraStabilization();
    }
}

void AAtlantisEonsCharacter::DisableAttackCameraStabilization()
{
    if (CameraStabilizationComp)
    {
        CameraStabilizationComp->DisableAttackCameraStabilization();
    }
}

bool AAtlantisEonsCharacter::IsNearEnemies() const
{
    if (CameraStabilizationComp)
    {
        return CameraStabilizationComp->IsNearEnemies();
    }
    return false;
}

void AAtlantisEonsCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    
    // Apply camera stabilization component (single-layer, stable approach)
    if (CameraStabilizationComp)
    {
        CameraStabilizationComp->UpdateCameraStabilization(DeltaSeconds);
        CameraStabilizationComp->SetMovementInput(CurrentMovementInput);
    }
}

// Removed dangerous direct stabilization functions that were causing crashes



void AAtlantisEonsCharacter::PerformBlock(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("🛡️ Block action triggered"));
    StartBlocking();
}

void AAtlantisEonsCharacter::ReleaseBlock(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("🛡️ Block released"));
    StopBlocking();
}

// ========== BLOCKING SYSTEM IMPLEMENTATION ==========

void AAtlantisEonsCharacter::StartBlocking()
{
    if (!CanPerformBlock())
    {
        UE_LOG(LogTemp, Warning, TEXT("🛡️ Cannot start blocking - conditions not met"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("🛡️ Starting block"));
    
    bIsBlocking = true;
    bCanBlock = false; // Prevent multiple activations

    // IMPORTANT: Clear any velocity to prevent movement issues when shield spawns
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->Velocity = FVector::ZeroVector;
        GetCharacterMovement()->StopMovementImmediately();
        UE_LOG(LogTemp, Warning, TEXT("🛡️ Cleared character velocity before spawning shield"));
    }

    // Spawn the shield
    SpawnShield();

    // Play block start animation if available
    if (BlockStartMontage)
    {
        USkeletalMeshComponent* Mesh = GetMesh();
        if (Mesh && Mesh->GetAnimInstance())
        {
            float MontageLength = Mesh->GetAnimInstance()->Montage_Play(BlockStartMontage);
            UE_LOG(LogTemp, Warning, TEXT("🛡️ Playing block start montage (length: %.2f)"), MontageLength);
        }
    }
    // If no start montage, play main block montage
    else if (BlockMontage)
    {
        USkeletalMeshComponent* Mesh = GetMesh();
        if (Mesh && Mesh->GetAnimInstance())
        {
            float MontageLength = Mesh->GetAnimInstance()->Montage_Play(BlockMontage);
            UE_LOG(LogTemp, Warning, TEXT("🛡️ Playing block montage (length: %.2f)"), MontageLength);
        }
    }
}

void AAtlantisEonsCharacter::StopBlocking()
{
    if (!bIsBlocking)
    {
        return; // Not currently blocking
    }

    UE_LOG(LogTemp, Warning, TEXT("🛡️ Stopping block"));
    
    bIsBlocking = false;

    // Destroy the shield
    DestroyShield();

    // Play block end animation if available
    if (BlockEndMontage)
    {
        USkeletalMeshComponent* Mesh = GetMesh();
        if (Mesh && Mesh->GetAnimInstance())
        {
            float MontageLength = Mesh->GetAnimInstance()->Montage_Play(BlockEndMontage);
            UE_LOG(LogTemp, Warning, TEXT("🛡️ Playing block end montage (length: %.2f)"), MontageLength);
        }
    }

    // Start cooldown if duration > 0
    if (BlockCooldownDuration > 0.0f)
    {
        bBlockOnCooldown = true;
        GetWorld()->GetTimerManager().SetTimer(
            BlockCooldownTimer,
            this,
            &AAtlantisEonsCharacter::OnBlockCooldownComplete,
            BlockCooldownDuration,
            false
        );
        UE_LOG(LogTemp, Warning, TEXT("🛡️ Block cooldown started (%.2f seconds)"), BlockCooldownDuration);
    }
    else
    {
        bCanBlock = true; // No cooldown - immediately available
    }
}

bool AAtlantisEonsCharacter::CanPerformBlock() const
{
    // Cannot block if already blocking
    if (bIsBlocking)
    {
        return false;
    }

    // Cannot block if on cooldown
    if (bBlockOnCooldown)
    {
        return false;
    }

    // Cannot block if dead
    if (GetCurrentHealth() <= 0.0f)
    {
        return false;
    }

    // Cannot block if not available (internal state)
    if (!bCanBlock)
    {
        return false;
    }

    return true;
}

void AAtlantisEonsCharacter::OnBlockCooldownComplete()
{
    bBlockOnCooldown = false;
    bCanBlock = true;
    UE_LOG(LogTemp, Warning, TEXT("🛡️ Block cooldown complete, blocking available"));
}

void AAtlantisEonsCharacter::SpawnShield()
{
    if (!ShieldBlueprintClass)
    {
        UE_LOG(LogTemp, Error, TEXT("🛡️ ShieldBlueprintClass is not set! Set BP_Master_Shield_01 in Blueprint"));
        return;
    }

    if (CurrentShieldActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("🛡️ Shield already exists, destroying old one"));
        DestroyShield();
    }

    // Spawn the shield actor at a safe location relative to the character
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Spawn shield centered around the character to surround them completely
    FVector SpawnLocation = GetActorLocation();
    FRotator SpawnRotation = GetActorRotation();
    
    // Position shield at proper height around character with custom height offset
    if (GetCapsuleComponent())
    {
        float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        // Start from character's feet position and add custom height offset
        FVector CharacterFeetLocation = GetActorLocation() - FVector(0, 0, CapsuleHalfHeight);
        SpawnLocation.Z = CharacterFeetLocation.Z + ShieldHeightOffset;
    }
    
    // Apply custom offset for fine-tuning shield position
    SpawnLocation += ShieldPositionOffset;
    
    UE_LOG(LogTemp, Warning, TEXT("🛡️ Spawning shield around character: %s (position offset: %s, height offset: %.1f)"), 
           *SpawnLocation.ToString(), *ShieldPositionOffset.ToString(), ShieldHeightOffset);

    CurrentShieldActor = GetWorld()->SpawnActor<AActor>(ShieldBlueprintClass, SpawnLocation, SpawnRotation, SpawnParams);
    
    if (CurrentShieldActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("🛡️ Shield spawned successfully: %s"), *CurrentShieldActor->GetName());

        // IMPORTANT: Disable physics on the shield to prevent movement interference
        CurrentShieldActor->SetActorEnableCollision(true); // Keep collision for blocking
        
        // Disable physics simulation on all components
        TArray<UActorComponent*> Components = CurrentShieldActor->GetComponents().Array();
        for (UActorComponent* Component : Components)
        {
            if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
            {
                PrimComp->SetSimulatePhysics(false);
                PrimComp->SetEnableGravity(false);
                PrimComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // Don't interfere with character movement
                PrimComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore); // Don't collide with world
                PrimComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // Block attacks/projectiles
                UE_LOG(LogTemp, Warning, TEXT("🛡️ Disabled physics on shield component: %s"), *Component->GetName());
            }
        }

        // Apply scale to the shield to properly surround the character
        if (ShieldScale != 1.0f)
        {
            CurrentShieldActor->SetActorScale3D(FVector(ShieldScale));
            UE_LOG(LogTemp, Warning, TEXT("🛡️ Shield scaled to: %.2f"), ShieldScale);
        }

        // Attach shield to character's root component to surround the entire character
        USceneComponent* RootComp = GetRootComponent();
        if (RootComp)
        {
            // Use KeepWorldTransform to maintain the centered position around character
            CurrentShieldActor->AttachToComponent(RootComp, FAttachmentTransformRules::KeepWorldTransform);
            UE_LOG(LogTemp, Warning, TEXT("🛡️ Shield attached to character root component (surrounding character)"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("🛡️ Failed to get character root component for shield attachment"));
        }

        // Final safety check: ensure shield doesn't affect character movement
        CurrentShieldActor->SetActorTickEnabled(false); // Disable tick to prevent any movement updates
        
        // Ensure no root motion or animation blueprint issues
        if (USkeletalMeshComponent* ShieldMesh = CurrentShieldActor->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (UAnimInstance* ShieldAnimInstance = ShieldMesh->GetAnimInstance())
            {
                // Disable root motion from shield animations
                ShieldMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
                UE_LOG(LogTemp, Warning, TEXT("🛡️ Shield animation configured to prevent root motion"));
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("🛡️ Shield physics disabled and safely attached"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🛡️ Failed to spawn shield actor"));
    }
}

void AAtlantisEonsCharacter::DestroyShield()
{
    if (CurrentShieldActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("🛡️ Destroying shield: %s"), *CurrentShieldActor->GetName());
        CurrentShieldActor->Destroy();
        CurrentShieldActor = nullptr;
    }
}

void AAtlantisEonsCharacter::OnSuccessfulBlock(float BlockedDamage)
{
    UE_LOG(LogTemp, Warning, TEXT("🛡️ Successfully blocked %.1f damage!"), BlockedDamage);
    
    // Here you could add visual/audio feedback for successful blocks
    // For example:
    // - Play block sound effect
    // - Spawn block particle effect
    // - Apply camera shake
    // - Show blocked damage number
    
    // Example: Show damage number for blocked damage
    if (BlockedDamage > 0.0f)
    {
        FVector BlockEffectLocation = GetActorLocation() + FVector(0, 0, 100); // Above character
        ShowDamageNumber(BlockedDamage, BlockEffectLocation, false);
    }
}

// ========== DASH DIRECTION HELPER FUNCTIONS IMPLEMENTATION ==========

bool AAtlantisEonsCharacter::ShouldDashBackward() const
{
    // Dash backward if not pressing W (forward) or standing still
    // Forward movement is positive Y in CurrentMovementInput
    return CurrentMovementInput.Y <= 0.1f;
}

bool AAtlantisEonsCharacter::ShouldDashForward() const
{
    // Dash forward if pressing W (forward movement)
    // Forward movement is positive Y in CurrentMovementInput
    return CurrentMovementInput.Y > 0.1f;
}

// ========== SECONDARY HUD SYSTEM IMPLEMENTATION ==========

void AAtlantisEonsCharacter::CreateSecondaryHUD()
{
    UE_LOG(LogTemp, Warning, TEXT("🔧 CreateSecondaryHUD called"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   SecondaryHUDClass: %s"), SecondaryHUDClass ? *SecondaryHUDClass->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   SecondaryHUDWidget: %s"), SecondaryHUDWidget ? TEXT("EXISTS") : TEXT("NULL"));
    
    // Only create if we have a valid class assigned and no existing widget
    if (!SecondaryHUDClass || SecondaryHUDWidget)
    {
        if (!SecondaryHUDClass)
        {
            UE_LOG(LogTemp, Error, TEXT("🔧 ❌ SecondaryHUD: No SecondaryHUDClass assigned! You need to set this in your BP_Character Blueprint!"));
            UE_LOG(LogTemp, Error, TEXT("🔧 ❌ Open BP_Character Blueprint → Details Panel → UI | SecondaryHUD → Set Secondary HUD Class to WBP_SecondaryHUD"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("🔧 SecondaryHUD: Widget already exists, checking if it's still in viewport"));
            
            // FIXED: Check if existing widget is still in viewport, re-add if needed
            if (SecondaryHUDWidget && !SecondaryHUDWidget->IsInViewport())
            {
                UE_LOG(LogTemp, Warning, TEXT("🔧 SecondaryHUD: Widget exists but not in viewport, re-adding"));
                SecondaryHUDWidget->AddToViewport(1000); // High Z-order to stay on top
                SecondaryHUDWidget->InitializeHUD(this);
                UE_LOG(LogTemp, Warning, TEXT("✅ SecondaryHUD: Widget re-added to viewport"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("🔧 SecondaryHUD: Widget already exists and is in viewport, skipping creation"));
            }
        }
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("🔧 SecondaryHUD: Creating widget with class: %s"), *SecondaryHUDClass->GetName());
    
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            UE_LOG(LogTemp, Warning, TEXT("🔧 SecondaryHUD: Creating widget..."));
            
            SecondaryHUDWidget = CreateWidget<UWBP_SecondaryHUD>(PC, SecondaryHUDClass);
            
            if (SecondaryHUDWidget)
            {
                UE_LOG(LogTemp, Warning, TEXT("🔧 SecondaryHUD: Widget created successfully"));
                
                // Add to viewport with very high Z-order to ensure it stays on top
                SecondaryHUDWidget->AddToViewport(10000); // INCREASED Z-order to ensure it stays above other UI
                
                UE_LOG(LogTemp, Warning, TEXT("✅ SecondaryHUD: Widget created and added to viewport successfully!"));
                UE_LOG(LogTemp, Warning, TEXT("🔧 SecondaryHUD: Widget should now be visible with health and experience bars"));
                
                // Force immediate initialization
                SecondaryHUDWidget->InitializeHUD(this);
                UE_LOG(LogTemp, Warning, TEXT("🔧 SecondaryHUD: Manual initialization completed"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ SecondaryHUD: Failed to create widget from class: %s"), *SecondaryHUDClass->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ SecondaryHUD: No player controller found"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ SecondaryHUD: No world found"));
    }
}

void AAtlantisEonsCharacter::DestroySecondaryHUD()
{
    if (SecondaryHUDWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗑️ SecondaryHUD: Destroying widget"));
        SecondaryHUDWidget->RemoveFromParent();
        SecondaryHUDWidget = nullptr;
    }
}

void AAtlantisEonsCharacter::Attack()
{
    if (!bCanAttack || bIsAttacking || bIsDead)
    {
        return;
    }

    // Check if we have enough mana for second or subsequent attacks
    if (AttackSequence > 0)
    {
        int32 ManaCost = (AttackSequence == 1) ? SecondAttackManaCost : SubsequentAttackManaCost;
        if (!HasEnoughMana(ManaCost))
        {
            UE_LOG(LogTemp, Warning, TEXT("Not enough mana for attack sequence %d (required: %d, current: %d)"), 
                   AttackSequence, ManaCost, CurrentMP);
            return;
        }
    }

    bIsAttacking = true;
    bCanAttack = false;

    // Play the appropriate attack montage based on sequence
    UAnimMontage* AttackMontage = nullptr;
    switch (AttackSequence)
    {
        case 0:
            AttackMontage = AttackMontage1;
            break;
        case 1:
            AttackMontage = AttackMontage2;
            ConsumeMana(SecondAttackManaCost);
            break;
        default:
            AttackMontage = AttackMontage3;
            ConsumeMana(SubsequentAttackManaCost);
            break;
    }

    if (AttackMontage)
    {
        PlayAnimMontage(AttackMontage);
        AttackSequence = (AttackSequence + 1) % 3;
    }

    // Set up recovery timer
    GetWorldTimerManager().SetTimer(AttackRecoveryTimer, this, &AAtlantisEonsCharacter::ResetAttackState, AttackRecoveryTime);
}

void AAtlantisEonsCharacter::ResetAttackState()
{
    bIsAttacking = false;
    bCanAttack = true;
}

void AAtlantisEonsCharacter::RecoverHP(int32 RecoverHP)
{
    RecoverHealth(RecoverHP);
}