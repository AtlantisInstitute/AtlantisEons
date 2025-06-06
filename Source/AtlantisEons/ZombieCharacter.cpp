#include "ZombieCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"
#include "AtlantisEonsCharacter.h"
#include "DamageNumberSystem.h"
#include "EngineUtils.h" // For TActorIterator
#include "Engine/OverlapResult.h"  // Corrected include for FOverlapResult definition
#include "WBP_ZombieHealthBar.h"

AZombieCharacter::AZombieCharacter()
{
    // Set this character to call Tick() every frame
    PrimaryActorTick.bCanEverTick = true;

    // Set default team ID
    TeamId = FGenericTeamId(2);

    // Create health bar widget component
    HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
    if (HealthBarWidget)
    {
        HealthBarWidget->SetupAttachment(RootComponent);
        HealthBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f)); // Position above zombie head
        HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen); // Always face camera
        HealthBarWidget->SetDrawSize(FVector2D(200.0f, 50.0f)); // Set appropriate size
        
        // Set scale and opacity
        HealthBarWidget->SetRelativeScale3D(FVector(0.75f, 0.25f, 1.0f)); // Scale X to 0.75, Y to 0.25
        
        // Set the widget class to our custom zombie health bar
        HealthBarWidget->SetWidgetClass(UWBP_ZombieHealthBar::StaticClass());
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Created health bar widget component with UWBP_ZombieHealthBar class, X scale 0.75, Y scale 0.25"));
    }

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;  // Character should face movement direction
    bUseControllerRotationYaw = false;  // Don't use controller rotation
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 0.0f; // DISABLED: Set to 0 for combat testing - zombie won't move
    GetCharacterMovement()->bUseRVOAvoidance = true; // Enable avoidance to prevent zombies getting stuck
    GetCharacterMovement()->bRequestedMoveUseAcceleration = true;
    GetCharacterMovement()->MaxAcceleration = 1024.0f; // FIXED: Reduced for more zombie-like movement
    GetCharacterMovement()->BrakingDecelerationWalking = 1024.0f; // FIXED: Reduced for smoother stopping
    GetCharacterMovement()->NavAgentProps.bCanCrouch = false;
    GetCharacterMovement()->NavAgentProps.bCanJump = false;
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    GetCharacterMovement()->bUseControllerDesiredRotation = true;
    GetCharacterMovement()->GravityScale = 1.0f;  // Make sure gravity is enabled

    // ENHANCED: Configure collision to properly block player but prevent physics issues
    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetSimulatePhysics(false);
        GetCapsuleComponent()->SetEnableGravity(false);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        
        // OPTIMIZED: Strong collision blocking with proper channel setup
        GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // Block player movement
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Allow camera
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Block visibility traces
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        
        // BALANCED: Heavy but not excessive mass to prevent penetration without physics chaos
        GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, 2000.0f, true); // Heavy but reasonable
        GetCapsuleComponent()->SetLinearDamping(50.0f); // High damping to resist movement
        GetCapsuleComponent()->SetAngularDamping(50.0f); // High rotational damping
        GetCapsuleComponent()->SetUseCCD(true); // Enable CCD to prevent high-speed penetration
        GetCapsuleComponent()->SetNotifyRigidBodyCollision(false); // No collision events for performance
        
        // ENHANCED: Proper body instance setup for solid collision
        if (GetCapsuleComponent()->GetBodyInstance())
        {
            GetCapsuleComponent()->GetBodyInstance()->SetResponseToAllChannels(ECR_Block);
            GetCapsuleComponent()->GetBodyInstance()->SetResponseToChannel(ECC_Pawn, ECR_Block); // Block player
            GetCapsuleComponent()->GetBodyInstance()->bLockXTranslation = false; // Allow AI movement
            GetCapsuleComponent()->GetBodyInstance()->bLockYTranslation = false;
            GetCapsuleComponent()->GetBodyInstance()->bLockZTranslation = true; // Lock Z to prevent flying
            GetCapsuleComponent()->GetBodyInstance()->bLockXRotation = true; // Lock pitch/roll
            GetCapsuleComponent()->GetBodyInstance()->bLockYRotation = true;
            GetCapsuleComponent()->GetBodyInstance()->bLockZRotation = false; // Allow yaw for AI
            
            // CRITICAL: Set collision shape to prevent penetration
            GetCapsuleComponent()->GetBodyInstance()->bUseCCD = true; // Continuous collision detection
            GetCapsuleComponent()->GetBodyInstance()->SetMassOverride(2000.0f, true); // Reinforce mass
        }
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: ENHANCED collision blocking - Mass: 2000kg, CCD enabled"));
    }

    // FIXED: Disable physics simulation to prevent flying when hit
    GetCapsuleComponent()->SetSimulatePhysics(false);
    GetCapsuleComponent()->SetEnableGravity(false); // Let character movement handle gravity
    
    // ENHANCED: Configure mesh to prevent clipping with player during attacks
    if (GetMesh())
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // CRITICAL: Don't collide with player mesh
        GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Support visibility/line traces
        GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Mesh collision configured to IGNORE player mesh - prevents clipping"));
    }

    // Set default values - adjusted for better gameplay
    AttackRange = 250.0f;  // Increased attack range slightly
    PatrolRadius = 1500.0f; // Increased patrol radius
    AttackDamage = 25.0f;   // Slightly increased damage
    bIsAttacking = false;
    bIsDead = false;
    
    // Initialize health
    MaxHealth = 10000.0f;  // Set a higher max health for zombies
    CurrentHealth = MaxHealth;
    
    // Initialize damage tracking variables
    LastDamageFrame = 0;
    LastDamageCauser = nullptr;
    LastDamageAmount = 0.0f;
    LastDamageLocation = FVector::ZeroVector;

    // Add tag for identification
    Tags.Add(FName("AdvancedZombieEnemy"));
    
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter initialized with team ID %d"), TeamId.GetId());
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter initialized with %.6f health"), CurrentHealth);
}

void AZombieCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Initialize zombie stats - INCREASED for better combat testing
    MaxHealth = 10000.0f;  // Increased to 10,000 for combat system testing
    CurrentHealth = MaxHealth;
    BaseDamage = 25.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter initialized with %f health"), CurrentHealth);

    // ENHANCED: Prevent physics launching with optimized collision setup
    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetSimulatePhysics(false);
        GetCapsuleComponent()->SetEnableGravity(false);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        
        // OPTIMIZED: Strong collision blocking to prevent player penetration
        GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // Block player movement
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        
        // BALANCED: Heavy but reasonable mass to prevent penetration
        GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, 2000.0f, true); // Heavy but not excessive
        GetCapsuleComponent()->SetLinearDamping(50.0f); // High damping to resist movement
        GetCapsuleComponent()->SetAngularDamping(50.0f); // High rotational damping
        GetCapsuleComponent()->SetUseCCD(true); // Enable CCD to prevent high-speed penetration
        GetCapsuleComponent()->SetNotifyRigidBodyCollision(false); // No collision events for performance
        
        // ENHANCED: Proper body instance setup for solid collision
        if (GetCapsuleComponent()->GetBodyInstance())
        {
            GetCapsuleComponent()->GetBodyInstance()->SetResponseToAllChannels(ECR_Block);
            GetCapsuleComponent()->GetBodyInstance()->SetResponseToChannel(ECC_Pawn, ECR_Block); // Block player
            GetCapsuleComponent()->GetBodyInstance()->bLockXTranslation = false; // Allow AI movement
            GetCapsuleComponent()->GetBodyInstance()->bLockYTranslation = false;
            GetCapsuleComponent()->GetBodyInstance()->bLockZTranslation = true; // Lock Z to prevent flying
            GetCapsuleComponent()->GetBodyInstance()->bLockXRotation = true; // Lock pitch/roll
            GetCapsuleComponent()->GetBodyInstance()->bLockYRotation = true;
            GetCapsuleComponent()->GetBodyInstance()->bLockZRotation = false; // Allow yaw for AI
            
            // CRITICAL: Enhanced collision detection
            GetCapsuleComponent()->GetBodyInstance()->bUseCCD = true; // Continuous collision detection
            GetCapsuleComponent()->GetBodyInstance()->SetMassOverride(2000.0f, true); // Reinforce mass
        }
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: ENHANCED collision blocking applied - Mass: 2000kg, CCD enabled"));
    }
    
    if (GetMesh())
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        
        // ENHANCED: Configure mesh to prevent clipping with player
        GetMesh()->SetMassOverrideInKg(NAME_None, 0.0f, false); // Zero mass for mesh
        GetMesh()->SetEnableGravity(false);
        GetMesh()->SetUseCCD(false);
        GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // CRITICAL: Don't collide with player mesh
        GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Support visibility traces
        GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera
        GetMesh()->SetNotifyRigidBodyCollision(false); // No collision events
        GetMesh()->SetLinearDamping(100.0f); // Maximum damping
        GetMesh()->SetAngularDamping(100.0f); // Maximum rotational damping
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Mesh configured to IGNORE player mesh - prevents attack clipping"));
    }
    
    // ENHANCED: Balanced character movement with proper collision resistance
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->bIgnoreBaseRotation = true;
        GetCharacterMovement()->bIgnoreClientMovementErrorChecksAndCorrection = true;
        GetCharacterMovement()->Mass = 2000.0f; // Heavy but reasonable mass
        GetCharacterMovement()->GroundFriction = 25.0f; // High friction to resist sliding
        GetCharacterMovement()->MaxAcceleration = 1024.0f; // Controlled acceleration
        GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f; // Fast but reasonable braking
        GetCharacterMovement()->bApplyGravityWhileJumping = false;
        GetCharacterMovement()->bNotifyApex = false;
        GetCharacterMovement()->bCanWalkOffLedges = false; // Prevent falling off edges
        
        // BALANCED: Prevent external velocity changes without being excessive
        GetCharacterMovement()->bRequestedMoveUseAcceleration = true;
        GetCharacterMovement()->bForceMaxAccel = false;
        GetCharacterMovement()->bRunPhysicsWithNoController = false;
        GetCharacterMovement()->bImpartBaseVelocityZ = false; // Don't inherit velocity
        GetCharacterMovement()->bImpartBaseVelocityX = false; // Don't inherit velocity
        GetCharacterMovement()->bImpartBaseVelocityY = false; // Don't inherit velocity
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: ENHANCED movement physics resistance - Mass: 2000kg, Friction: 25.0"));
        
        // TESTING: Re-enable movement for AI behavior with proper collision
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Movement enabled with enhanced collision resistance"));
        
        // LOG: Movement settings for verification
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Movement Settings - MaxWalkSpeed: %.1f, Mass: %.1f"), 
               GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->Mass);
    }

    // Debug log the team ID
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter Team ID: %d"), GetGenericTeamId().GetId());
    
    // Verify that the behavior tree and blackboard are set
    if (BehaviorTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter has valid BehaviorTree: %s"), *BehaviorTree->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ZombieCharacter is missing BehaviorTree!!"));
    }
    
    if (BlackboardData)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter has valid BlackboardData: %s"), *BlackboardData->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ZombieCharacter is missing BlackboardData!!"));
    }
    
    // Ensure we have our tag set
    if (!ActorHasTag("AdvancedZombieEnemy"))
    {
        Tags.Add(FName("AdvancedZombieEnemy"));
        UE_LOG(LogTemp, Warning, TEXT("Added missing AdvancedZombieEnemy tag to zombie"));
    }
    
    // ADDED: Debug animation setup
    UE_LOG(LogTemp, Warning, TEXT("🎭 ZombieCharacter Animation Debug:"));
    UE_LOG(LogTemp, Warning, TEXT("  - Mesh: %s"), GetMesh() ? TEXT("Valid") : TEXT("NULL"));
    if (GetMesh())
    {
        UE_LOG(LogTemp, Warning, TEXT("  - AnimInstance: %s"), GetMesh()->GetAnimInstance() ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("  - SkeletalMesh: %s"), GetMesh()->GetSkeletalMeshAsset() ? TEXT("Valid") : TEXT("NULL"));
    }
    UE_LOG(LogTemp, Warning, TEXT("  - HitReactMontage: %s"), HitReactMontage ? TEXT("Valid") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("  - AttackMontage: %s"), AttackMontage ? TEXT("Valid") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("  - DeathMontage: %s"), DeathMontage ? TEXT("Valid") : TEXT("NULL"));

    // Set health bar widget opacity after initialization
    if (HealthBarWidget)
    {
        // Set the widget opacity using a timer to ensure the widget is created
        FTimerHandle OpacityTimer;
        GetWorld()->GetTimerManager().SetTimer(OpacityTimer, [this]()
        {
            if (HealthBarWidget && HealthBarWidget->GetWidget())
            {
                HealthBarWidget->GetWidget()->SetRenderOpacity(1.0f);
                UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Set health bar widget opacity to 1.0"));
            }
        }, 0.1f, false);
    }
}

void AZombieCharacter::PerformAttack()
{
    // COMBAT TESTING: Temporarily disable zombie attacks
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: PerformAttack disabled for combat testing"));
    return;
    
    if (bIsAttacking || bIsDead)
    {
        return; // Don't start another attack while one is in progress or if dead
    }

    if (AttackMontage)
    {
        bIsAttacking = true;
        PlayAnimMontage(AttackMontage);
        
        // Set a timer to reset the attacking flag when the montage ends
        float MontageLength = AttackMontage->GetPlayLength();
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            bIsAttacking = false;
        }, MontageLength, false);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AttackMontage is not set in ZombieCharacter!"));
    }
}

void AZombieCharacter::OnAttackHit()
{
    // COMBAT TESTING: Temporarily disable zombie attack damage
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: OnAttackHit disabled for combat testing"));
    return;
    
    if (bIsDead) return; // Don't attack if dead
    
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: OnAttackHit called"));
    
    // Increase hit detection radius to make hitting more reliable
    const float SphereRadius = 250.0f; // Increased from 100.0f
    
    // Get character's current location
    FVector StartLocation = GetActorLocation();
    StartLocation.Z += 50.0f; // Adjust for better attack height
    
    // Get forward vector for attack direction
    FVector ForwardVector = GetActorForwardVector();
    
    // Calculate end position for tracing
    FVector EndLocation = StartLocation + ForwardVector * 250.0f; // Increased attack range
    
    // Set up collision parameters
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    
    // Draw debug visual to see the attack range
    // DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f, 0, 3.0f);
    // DrawDebugSphere(GetWorld(), StartLocation, SphereRadius, 12, FColor::Yellow, false, 2.0f);
    
    // Look for the player in the world first as a direct approach
    AAtlantisEonsCharacter* PlayerCharacter = nullptr;
    for (TActorIterator<AAtlantisEonsCharacter> It(GetWorld()); It; ++It)
    {
        PlayerCharacter = *It;
        if (PlayerCharacter)
        {
            // Check if player is within attack range
            float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
            
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Found player at distance %.1f"), DistanceToPlayer);
            
            if (DistanceToPlayer <= SphereRadius * 1.5f) // Give a bit extra range for reliable hits
            {
                UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Player in DIRECT attack range. Dealing %f damage to %s"), 
                       AttackDamage, *PlayerCharacter->GetName());
                
                // DETAILED LOGGING: Log player state before attack
                UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Player state before attack - Health: %.1f"), 
                       PlayerCharacter->GetCurrentHealth());
                
                // DETAILED LOGGING: Log the exact parameters being passed to ApplyDamage
                UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: About to call UGameplayStatics::ApplyDamage with:"));
                UE_LOG(LogTemp, Warning, TEXT("  - Target: %s"), *PlayerCharacter->GetName());
                UE_LOG(LogTemp, Warning, TEXT("  - Damage: %.1f"), AttackDamage);
                UE_LOG(LogTemp, Warning, TEXT("  - Instigator: %s"), GetController() ? *GetController()->GetName() : TEXT("NULL"));
                UE_LOG(LogTemp, Warning, TEXT("  - DamageCauser: %s"), *GetName());
                UE_LOG(LogTemp, Warning, TEXT("  - DamageType: %s"), *UDamageType::StaticClass()->GetName());
                
                // DIRECT APPROACH: Apply damage to the player directly
                float DamageResult = UGameplayStatics::ApplyDamage(PlayerCharacter, AttackDamage, GetController(), this, UDamageType::StaticClass());
                
                UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: ApplyDamage returned %.1f"), DamageResult);
                
                // ADDITIONAL CHECK: Try calling the player's TakeDamage function directly as a fallback
                if (DamageResult <= 0.0f)
                {
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: ApplyDamage returned 0, trying direct TakeDamage call"));
                    FDamageEvent DamageEvent;
                    float DirectDamageResult = PlayerCharacter->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Direct TakeDamage returned %.1f"), DirectDamageResult);
                }
                
                // Check player state after attack
                UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Player state after attack - Health: %.1f"), 
                       PlayerCharacter->GetCurrentHealth());
                
                // Visual feedback
                // DrawDebugSphere(GetWorld(), PlayerCharacter->GetActorLocation(), 50.0f, 12, FColor::Red, false, 2.0f);
                
                // Attempt to play hit react animation
                PlayerCharacter->PlayHitReactMontage();
                return; // Successfully attacked player, no need to continue
            }
        }
    }
    
    // Fall back to sphere overlap check
    TArray<FOverlapResult> Overlaps;
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(SphereRadius);
    
    // Perform the sphere sweep using the Pawn channel
    bool bHit = GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        StartLocation,
        FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn),
        SphereShape,
        QueryParams
    );
    
    // Process hits
    if (bHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Overlap detected %d results"), Overlaps.Num());
        
        for (auto& Overlap : Overlaps)
        {
            AActor* HitActor = Overlap.GetActor();
            
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Checking overlap with actor: %s"), 
                   HitActor ? *HitActor->GetName() : TEXT("NULL"));
            
            // Check if we hit a player character
            if (HitActor && HitActor != this && HitActor->IsA(AAtlantisEonsCharacter::StaticClass()))
            {
                PlayerCharacter = Cast<AAtlantisEonsCharacter>(HitActor);
                if (PlayerCharacter)
                {
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: OVERLAP hit. Dealing %f damage to %s"), 
                           AttackDamage, *PlayerCharacter->GetName());
                    
                    // Log player state before attack
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Player state before overlap attack - Health: %.1f"), 
                           PlayerCharacter->GetCurrentHealth());
                    
                    // DETAILED LOGGING: Log the exact parameters being passed to ApplyDamage
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: About to call UGameplayStatics::ApplyDamage (OVERLAP) with:"));
                    UE_LOG(LogTemp, Warning, TEXT("  - Target: %s"), *PlayerCharacter->GetName());
                    UE_LOG(LogTemp, Warning, TEXT("  - Damage: %.1f"), AttackDamage);
                    UE_LOG(LogTemp, Warning, TEXT("  - Instigator: %s"), GetController() ? *GetController()->GetName() : TEXT("NULL"));
                    UE_LOG(LogTemp, Warning, TEXT("  - DamageCauser: %s"), *GetName());
                    UE_LOG(LogTemp, Warning, TEXT("  - DamageType: %s"), *UDamageType::StaticClass()->GetName());
                    
                    // Apply damage to the player
                    float DamageResult = UGameplayStatics::ApplyDamage(PlayerCharacter, AttackDamage, GetController(), this, UDamageType::StaticClass());
                    
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Overlap ApplyDamage returned %.1f"), DamageResult);
                    
                    // ADDITIONAL CHECK: Try calling the player's TakeDamage function directly as a fallback
                    if (DamageResult <= 0.0f)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Overlap ApplyDamage returned 0, trying direct TakeDamage call"));
                        FDamageEvent DamageEvent;
                        float DirectDamageResult = PlayerCharacter->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
                        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Overlap Direct TakeDamage returned %.1f"), DirectDamageResult);
                    }
                    
                    // Check player state after attack
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Player state after overlap attack - Health: %.1f"), 
                           PlayerCharacter->GetCurrentHealth());
                    
                    // Visual feedback
                    // DrawDebugSphere(GetWorld(), HitActor->GetActorLocation(), 35.0f, 12, FColor::Green, false, 2.0f);
                    
                    // Attempt to play hit react animation on the player
                    PlayerCharacter->PlayHitReactMontage();
                    break; // Found and hit the player, no need to check other overlaps
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: No overlaps detected"));
    }
}

void AZombieCharacter::AttackPlayer()
{
    // COMBAT TESTING: Temporarily disable zombie attacks
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: AttackPlayer disabled for combat testing"));
    return;
    
    if (!bIsDead && !bIsAttacking)
    {
        PerformAttack();
    }
}

void AZombieCharacter::PlayHitReactMontage()
{
    UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE HIT REACTION CALLED! Starting comprehensive hit reaction system..."));
    
    // Enhanced hit reaction animation system with comprehensive error handling
    if (!GetMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("🎭 ZombieCharacter: No mesh component found for hit reaction!"));
        return;
    }
    
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("🎭 ZombieCharacter: No AnimInstance found for hit reaction!"));
        return;
    }
    
    // FORCE PLAY the hit reaction montage with maximum priority and correct slot
    UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: About to force play montage - HitReactMontage: %s"), 
           HitReactMontage ? *HitReactMontage->GetName() : TEXT("NULL"));
    
    // DEBUG: Check animation blueprint state
    if (AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE ANIM DEBUG: AnimInstance class: %s"), *AnimInstance->GetClass()->GetName());
        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE ANIM DEBUG: Is any montage playing: %s"), AnimInstance->IsAnyMontagePlaying() ? TEXT("YES") : TEXT("NO"));
        
        if (AnimInstance->IsAnyMontagePlaying())
        {
            UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage();
            UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE ANIM DEBUG: Current active montage: %s"), 
                   CurrentMontage ? *CurrentMontage->GetName() : TEXT("NULL"));
        }
    }
    
    // CRITICAL FIX: Play montage with UpperBody slot to avoid locomotion interference
    float MontageLength = AnimInstance->Montage_Play(HitReactMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
    
    // If that doesn't work, try with the default slot but force stop all other montages first
    if (MontageLength <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: First play attempt failed, trying with force stop"));
        AnimInstance->StopAllMontages(0.1f); // Stop all montages with short blend out
        
        // Wait a frame and try again
        FTimerHandle RetryTimer;
        GetWorld()->GetTimerManager().SetTimer(
            RetryTimer,
            [this]()
            {
                if (IsValid(this) && GetMesh() && GetMesh()->GetAnimInstance() && HitReactMontage)
                {
                    UAnimInstance* AI = GetMesh()->GetAnimInstance();
                    float RetryLength = AI->Montage_Play(HitReactMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
                    UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: RETRY montage play returned: %.2f"), RetryLength);
                    
                    if (RetryLength > 0.0f)
                    {
                        // Set blend in/out to prevent state machine interference
                        FOnMontageBlendingOutStarted BlendOutDelegate = FOnMontageBlendingOutStarted::CreateUObject(this, &AZombieCharacter::OnHitReactMontageBlendOut);
                        AI->Montage_SetBlendingOutDelegate(BlendOutDelegate);
                        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: ✅ RETRY SUCCESS - Hit reaction montage now playing!"));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: ❌ RETRY FAILED - Animation Blueprint may be blocking montages"));
                    }
                }
            },
            0.1f,
            false
        );
        return;
    }
    
    // IMMEDIATE DEBUG: Check if montage actually started
    UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: Montage_Play returned length: %.2f"), MontageLength);
    
    if (MontageLength > 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: ✅ FORCEFULLY started hit reaction montage (length: %.2fs)"), MontageLength);
        
        // Set blend out delegate to restore AI behavior
        FOnMontageBlendingOutStarted BlendOutDelegate = FOnMontageBlendingOutStarted::CreateUObject(this, &AZombieCharacter::OnHitReactMontageBlendOut);
        AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate);
        
        // Temporarily disable attacking during hit reaction
        bool bWasAttacking = bIsAttacking;
        bIsAttacking = true; // Prevent new attacks during hit reaction
        
        // DEBUG: Verify montage is actually playing after force start
        FTimerHandle DebugTimer;
        GetWorld()->GetTimerManager().SetTimer(
            DebugTimer,
            [this, bWasAttacking]()
            {
                if (IsValid(this) && GetMesh() && GetMesh()->GetAnimInstance())
                {
                    UAnimInstance* AI = GetMesh()->GetAnimInstance();
                    bool bIsPlaying = AI->Montage_IsPlaying(HitReactMontage);
                    float CurrentPos = AI->Montage_GetPosition(HitReactMontage);
                    float PlayRate = AI->Montage_GetPlayRate(HitReactMontage);
                    bool bIsActive = AI->Montage_IsActive(HitReactMontage);
                    
                    UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE DEBUG CHECK (0.2s later): IsPlaying=%s, IsActive=%s, Position=%.2f, PlayRate=%.2f"), 
                           bIsPlaying ? TEXT("YES") : TEXT("NO"),
                           bIsActive ? TEXT("YES") : TEXT("NO"),
                           CurrentPos,
                           PlayRate);
                    
                    if (!bIsPlaying && !bIsActive)
                    {
                        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: ❌ CRITICAL - Montage was stopped! Animation Blueprint overriding!"));
                        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: FORCING FALLBACK - Manual animation control"));
                        
                        // ULTIMATE FALLBACK: Manually control the mesh animation
                        if (GetMesh())
                        {
                            // Stop all movement
                            GetCharacterMovement()->StopMovementImmediately();
                            GetCharacterMovement()->DisableMovement();
                            
                            // Pause AI
                            if (AAIController* AIController = Cast<AAIController>(GetController()))
                            {
                                AIController->GetBrainComponent()->PauseLogic(TEXT("Force Hit Reaction"));
                            }
                            
                            // Re-enable after a delay
                            FTimerHandle RestoreTimer;
                            GetWorld()->GetTimerManager().SetTimer(
                                RestoreTimer,
                                [this, bWasAttacking]()
                                {
                                    if (IsValid(this))
                                    {
                                        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                                        bIsAttacking = bWasAttacking;
                                        
                                        if (AAIController* AIController = Cast<AAIController>(GetController()))
                                        {
                                            AIController->GetBrainComponent()->ResumeLogic(TEXT("Force Hit Reaction Complete"));
                                        }
                                        
                                        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: Manual hit reaction complete - systems restored"));
                                    }
                                },
                                1.0f,
                                false
                            );
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: ✅ SUCCESS - Hit reaction montage is playing correctly!"));
                    }
                }
            },
            0.2f, // Check 0.2 seconds later
            false
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: ❌ Failed to play hit reaction montage - using simple fallback"));
        
        // SIMPLIFIED fallback - NO physics effects that could cause launching
        // Just briefly pause movement and AI
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
        
        // Pause the AI behavior tree briefly
        if (AAIController* AIController = Cast<AAIController>(GetController()))
        {
            AIController->GetBrainComponent()->PauseLogic(TEXT("Hit Reaction Fallback"));
            if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
            {
                BlackboardComp->SetValueAsBool(FName("IsHitReacting"), true);
            }
        }
        
        // REMOVED: All knockback and mesh shake effects that could cause flying
        // Just a simple pause and restore
        
        // Restore movement and AI after brief pause
        FTimerHandle MovementRestoreTimer;
        GetWorld()->GetTimerManager().SetTimer(
            MovementRestoreTimer,
            [this]()
            {
                if (IsValid(this))
                {
                    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                    UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: Simple fallback hit reaction complete"));
                    
                    // Re-enable AI
                    if (AAIController* AIController = Cast<AAIController>(GetController()))
                    {
                        AIController->GetBrainComponent()->ResumeLogic(TEXT("Hit Reaction Fallback Complete"));
                        if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
                        {
                            BlackboardComp->SetValueAsBool(FName("IsHitReacting"), false);
                        }
                    }
                }
            },
            0.5f, // Much shorter pause
            false
        );
    }
}

void AZombieCharacter::ApplyDamage(float DamageAmount)
{
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter::ApplyDamage"));
    
    // Exit early if already dead
    if (bIsDead)
    {
        return;
    }
    
    // Apply damage
    if (DamageAmount > 0.0f)
    {
        float PreviousHealth = CurrentHealth;
        CurrentHealth = FMath::Max(CurrentHealth - DamageAmount, 0.0f);
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Damage applied"));
        
        // NOTE: Damage numbers are now handled in TakeDamage_Implementation instead of here
        
        // Check if zombie has died
        if (CurrentHealth <= 0.0f)
        {
            bIsDead = true;
            HandleDeath();
        }
    }
}

void AZombieCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: HandleDeath function entered. bIsDead: %d"), bIsDead ? 1 : 0);
    
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Already dead, returning early"));
        return;
    }
    
    bIsDead = true;

    // Disable movement
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // Disable collision so the zombie doesn't block movement
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Play death animation if available
    if (DeathMontage)
    {
        PlayAnimMontage(DeathMontage);
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Playing death animation"));

        // Set up timer to destroy the actor after animation
        float MontageLength = DeathMontage->GetPlayLength();
        FTimerHandle DeathTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, [this]()
        {
            OnDeath();
        }, MontageLength + 1.0f, false); // Add 1 second delay after animation
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: No death montage, destroying immediately"));
        // If no death montage, call OnDeath after a short delay
        FTimerHandle DeathTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, [this]()
        {
            OnDeath();
        }, 2.0f, false);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Death sequence initiated"));
}

void AZombieCharacter::OnDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: OnDeath called - destroying zombie"));
    
    // Final cleanup and destroy the actor
    if (IsValid(this))
    {
        Destroy();
    }
}

// Keep track of damage event IDs we've already processed to prevent duplicate damage numbers
static TMap<uint32, float> ProcessedDamageEvents;

float AZombieCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // Early out if damage is not applicable
    if (DamageAmount <= 0.0f || bIsDead)
    {
        return 0.0f;
    }
    
    // CRITICAL: Completely lock down physics to prevent flying
    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetSimulatePhysics(false);
        if (UPrimitiveComponent* PrimComp = GetCapsuleComponent())
        {
            if (FBodyInstance* BodyInst = PrimComp->GetBodyInstance())
            {
                BodyInst->SetLinearVelocity(FVector::ZeroVector, false);
                BodyInst->SetAngularVelocityInRadians(FVector::ZeroVector, false);
            }
        }
    }
    
    if (GetMesh())
    {
        GetMesh()->SetSimulatePhysics(false);
        if (FBodyInstance* MeshBodyInst = GetMesh()->GetBodyInstance())
        {
            MeshBodyInst->SetLinearVelocity(FVector::ZeroVector, false);
            MeshBodyInst->SetAngularVelocityInRadians(FVector::ZeroVector, false);
        }
    }
    
    // Apply damage to health
    float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Max(CurrentHealth - DamageAmount, 0.0f);
    float ActualDamage = PreviousHealth - CurrentHealth;
    
    // Update health bar
    UpdateHealthBar();
    
    // Get damage number location
    FVector DamageLocation = GetActorLocation() + FVector(0, 0, 120);
    
    // Try to find damage number system with multiple fallbacks
    ADamageNumberSystem* DamageSystem = ADamageNumberSystem::GetInstance(GetWorld());
    
    if (!DamageSystem)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADamageNumberSystem::StaticClass(), FoundActors);
        if (FoundActors.Num() > 0)
        {
            DamageSystem = Cast<ADamageNumberSystem>(FoundActors[0]);
        }
    }
    
    if (!DamageSystem)
    {
        DamageSystem = GetWorld()->SpawnActor<ADamageNumberSystem>();
    }
    
    // Spawn damage number
    if (DamageSystem && IsValid(DamageSystem))
    {
        DamageSystem->SpawnDamageNumberAtLocation(this, DamageLocation, ActualDamage, false);
    }
    
    // Check for death
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        HandleDeath();
    }
    
    return ActualDamage;
}

void AZombieCharacter::ShowDamageNumber(float DamageAmount, FVector Location, bool bIsCritical)
{
    if (ADamageNumberSystem* System = Cast<ADamageNumberSystem>(UGameplayStatics::GetActorOfClass(GetWorld(), ADamageNumberSystem::StaticClass())))
    {
        System->ShowDamage(DamageAmount, Location, bIsCritical);
    }
}

void AZombieCharacter::UpdateHealthBar()
{
    if (HealthBarWidget && HealthBarWidget->GetWidget())
    {
        // Cast to our custom zombie health bar widget
        UWBP_ZombieHealthBar* ZombieHealthBarWidget = Cast<UWBP_ZombieHealthBar>(HealthBarWidget->GetWidget());
        if (ZombieHealthBarWidget)
        {
            // Calculate health percentage
            float HealthPercentage = MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f;
            HealthPercentage = FMath::Clamp(HealthPercentage, 0.0f, 1.0f);
            
            // Update the widget with current health values
            ZombieHealthBarWidget->UpdateHealth(CurrentHealth, MaxHealth, HealthPercentage);
            
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Updated zombie health bar widget - %.1f/%.1f (%.1f%%)"), 
                   CurrentHealth, MaxHealth, HealthPercentage * 100.0f);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Could not cast to UWBP_ZombieHealthBar - using fallback method"));
            
            // Fallback to generic widget approach
            UUserWidget* HealthWidget = HealthBarWidget->GetWidget();
            if (HealthWidget)
            {
                // Calculate health percentage
                float HealthPercentage = MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f;
                HealthPercentage = FMath::Clamp(HealthPercentage, 0.0f, 1.0f);
                
                // Try to call UpdateHealth function if it exists in the widget
                if (UFunction* UpdateHealthFunction = HealthWidget->FindFunction(FName("UpdateHealth")))
                {
                    struct
                    {
                        float CurrentHealth;
                        float MaxHealth;
                        float Percentage;
                    } Params;
                    
                    Params.CurrentHealth = CurrentHealth;
                    Params.MaxHealth = MaxHealth;
                    Params.Percentage = HealthPercentage;
                    
                    HealthWidget->ProcessEvent(UpdateHealthFunction, &Params);
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Updated health bar via fallback - %.1f/%.1f (%.1f%%)"), 
                           CurrentHealth, MaxHealth, HealthPercentage * 100.0f);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: HealthBarWidget or its widget is NULL"));
    }
}

void AZombieCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AZombieCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AZombieCharacter::OnHitReactMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: Hit reaction montage finished blending out. Interrupted: %s"), 
           bInterrupted ? TEXT("YES") : TEXT("NO"));
    
    // Restore AI behavior and movement
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        AIController->GetBrainComponent()->ResumeLogic(TEXT("Hit Reaction Complete"));
        if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
        {
            BlackboardComp->SetValueAsBool(FName("IsHitReacting"), false);
        }
    }
    
    // Restore movement if it was disabled
    if (GetCharacterMovement()->MovementMode == MOVE_None)
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
    
    UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: All systems restored after hit reaction montage"));
}

void AZombieCharacter::DebugTakeDamage(float DamageAmount)
{
    UE_LOG(LogTemp, Warning, TEXT("🔧 DEBUG: DebugTakeDamage called with %.1f damage"), DamageAmount);
    
    // Create a simple damage event
    FDamageEvent DamageEvent;
    
    // Call TakeDamage directly to test the damage system
    float ActualDamage = TakeDamage(DamageAmount, DamageEvent, nullptr, nullptr);
    
    UE_LOG(LogTemp, Warning, TEXT("🔧 DEBUG: DebugTakeDamage completed, actual damage: %.1f"), ActualDamage);
}
