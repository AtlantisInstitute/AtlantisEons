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
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Created health bar widget component"));
    }

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;  // Character should face movement direction
    bUseControllerRotationYaw = false;  // Don't use controller rotation
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 100.0f; // REVERTED: Back to 100.0f as requested
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

    // FIXED: Configure collision to prevent physics impulse issues
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block); // Block by default
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // But overlap with other pawns
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera
    
    // FIXED: Disable physics simulation to prevent flying when hit
    GetCapsuleComponent()->SetSimulatePhysics(false);
    GetCapsuleComponent()->SetEnableGravity(false); // Let character movement handle gravity
    
    // FIXED: Configure mesh to not simulate physics
    if (GetMesh())
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    }

    // Set default values - adjusted for better gameplay
    AttackRange = 250.0f;  // Increased attack range slightly
    PatrolRadius = 1500.0f; // Increased patrol radius
    AttackDamage = 25.0f;   // Slightly increased damage
    bIsAttacking = false;
    bIsDead = false;
    
    // Initialize health
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
    
    // Initialize damage tracking variables
    LastDamageFrame = 0;
    LastDamageCauser = nullptr;
    LastDamageAmount = 0.0f;
    LastDamageLocation = FVector::ZeroVector;

    // Add tag for identification
    Tags.Add(FName("AdvancedZombieEnemy"));
    
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter initialized with team ID %d"), TeamId.GetId());
}

void AZombieCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Initialize zombie stats - INCREASED for better combat testing
    MaxHealth = 500.0f;  // Increased from 100 to 500 for testing
    CurrentHealth = MaxHealth;
    BaseDamage = 25.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter initialized with %f health"), CurrentHealth);

    // CRITICAL: Completely prevent physics launching - Enhanced setup
    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetSimulatePhysics(false);
        GetCapsuleComponent()->SetEnableGravity(false);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        
        // CRITICAL: Prevent any physics responses that could launch the zombie
        GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // Overlap with other pawns
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        
        // ENHANCED: Lock down all physics properties completely
        GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, 10000.0f, true); // Extremely heavy
        GetCapsuleComponent()->SetLinearDamping(50.0f); // Maximum damping
        GetCapsuleComponent()->SetAngularDamping(50.0f); // Maximum rotational damping
        GetCapsuleComponent()->SetUseCCD(false); // No continuous collision
        GetCapsuleComponent()->SetNotifyRigidBodyCollision(false); // No collision events
        
        // CRITICAL: Disable any physics body responses
        if (GetCapsuleComponent()->GetBodyInstance())
        {
            GetCapsuleComponent()->GetBodyInstance()->SetResponseToAllChannels(ECR_Block);
            GetCapsuleComponent()->GetBodyInstance()->SetResponseToChannel(ECC_Pawn, ECR_Overlap);
            GetCapsuleComponent()->GetBodyInstance()->bLockXTranslation = false; // Allow movement but controlled
            GetCapsuleComponent()->GetBodyInstance()->bLockYTranslation = false;
            GetCapsuleComponent()->GetBodyInstance()->bLockZTranslation = true; // Lock Z to prevent flying
            GetCapsuleComponent()->GetBodyInstance()->bLockXRotation = true; // Lock rotations
            GetCapsuleComponent()->GetBodyInstance()->bLockYRotation = true;
            GetCapsuleComponent()->GetBodyInstance()->bLockZRotation = false; // Allow yaw rotation only
        }
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Enhanced capsule physics - SimulatePhysics: false, EnableGravity: false, Mass: 10000kg"));
    }
    
    if (GetMesh())
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        
        // ENHANCED: Ensure mesh has no physics interactions whatsoever
        GetMesh()->SetMassOverrideInKg(NAME_None, 0.0f, false); // Zero mass
        GetMesh()->SetEnableGravity(false);
        GetMesh()->SetUseCCD(false);
        GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        GetMesh()->SetNotifyRigidBodyCollision(false); // No collision events
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Enhanced mesh physics - SimulatePhysics: false, Mass: 0kg"));
    }
    
    // ENHANCED: Configure character movement to resist external forces completely
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->bIgnoreBaseRotation = true;
        GetCharacterMovement()->bIgnoreClientMovementErrorChecksAndCorrection = true;
        GetCharacterMovement()->Mass = 10000.0f; // Extremely heavy to resist any physics impulses
        GetCharacterMovement()->GroundFriction = 50.0f; // Maximum friction
        GetCharacterMovement()->MaxAcceleration = 2048.0f;
        GetCharacterMovement()->BrakingDecelerationWalking = 4096.0f; // High braking
        GetCharacterMovement()->bApplyGravityWhileJumping = false;
        GetCharacterMovement()->bNotifyApex = false;
        GetCharacterMovement()->bCanWalkOffLedges = false; // Prevent falling off edges
        GetCharacterMovement()->bIgnoreBaseRotation = true;
        
        // CRITICAL: Prevent any external velocity changes
        GetCharacterMovement()->bRequestedMoveUseAcceleration = true;
        GetCharacterMovement()->bForceMaxAccel = false;
        GetCharacterMovement()->bRunPhysicsWithNoController = false;
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Enhanced movement physics - Mass: 10000kg, High friction"));
        
        // FIXED: Log movement settings to verify animation sync
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Movement Speed Settings - MaxWalkSpeed: %.1f, MaxAcceleration: %.1f"), 
               GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxAcceleration);
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
}

void AZombieCharacter::PerformAttack()
{
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
    DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f, 0, 3.0f);
    DrawDebugSphere(GetWorld(), StartLocation, SphereRadius, 12, FColor::Yellow, false, 2.0f);
    
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
                DrawDebugSphere(GetWorld(), PlayerCharacter->GetActorLocation(), 50.0f, 12, FColor::Red, false, 2.0f);
                
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
                    DrawDebugSphere(GetWorld(), HitActor->GetActorLocation(), 35.0f, 12, FColor::Green, false, 2.0f);
                    
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
    UE_LOG(LogTemp, Warning, TEXT("🎯 ZombieCharacter: TakeDamage called with damage amount: %f from %s"), 
           DamageAmount, 
           DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"));
    
    // Early out if damage is not applicable
    if (DamageAmount <= 0.0f || bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ ZombieCharacter: Ignoring damage - Amount: %f, bIsDead: %s"), 
               DamageAmount, bIsDead ? TEXT("true") : TEXT("false"));
        return 0.0f;
    }
    
    // CRITICAL: Completely lock down physics to prevent flying
    if (GetCapsuleComponent())
    {
        // Store current location to reset if needed
        FVector CurrentLocation = GetActorLocation();
        FRotator CurrentRotation = GetActorRotation();
        
        // Ensure physics are completely disabled
        GetCapsuleComponent()->SetSimulatePhysics(false);
        GetCapsuleComponent()->SetEnableGravity(false);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        
        // CRITICAL: Lock all movement axes to prevent any displacement
        if (GetCapsuleComponent()->GetBodyInstance())
        {
            GetCapsuleComponent()->GetBodyInstance()->bLockXTranslation = true;
            GetCapsuleComponent()->GetBodyInstance()->bLockYTranslation = true;
            GetCapsuleComponent()->GetBodyInstance()->bLockZTranslation = true; // Lock ALL translation
            GetCapsuleComponent()->GetBodyInstance()->bLockXRotation = true;
            GetCapsuleComponent()->GetBodyInstance()->bLockYRotation = true;
            GetCapsuleComponent()->GetBodyInstance()->bLockZRotation = true; // Lock ALL rotation during damage
        }
        
        // FORCE: Reset velocity and stop any movement
        if (GetCharacterMovement())
        {
            GetCharacterMovement()->StopMovementImmediately();
            GetCharacterMovement()->Velocity = FVector::ZeroVector;
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            
            // CRITICAL: Clear any pending impulses or forces
            GetCharacterMovement()->ClearAccumulatedForces();
        }
        
        // SAFETY: Force reset to exact location if anything changed
        SetActorLocationAndRotation(CurrentLocation, CurrentRotation, false, nullptr, ETeleportType::ResetPhysics);
        
        UE_LOG(LogTemp, Warning, TEXT("🔒 ZombieCharacter: Physics completely locked down at location: %s"), 
               *CurrentLocation.ToString());
    }
    
    if (GetMesh())
    {
        // Ensure mesh physics are also disabled
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetEnableGravity(false);
    }
    
    // SIMPLIFIED: Process damage immediately without calling Super::TakeDamage
    float ActualDamage = DamageAmount;
    
    UE_LOG(LogTemp, Warning, TEXT("✅ ZombieCharacter: Processing damage: %f"), ActualDamage);
    
    // Get hit location for damage numbers
    FVector DamageLocation = GetActorLocation() + FVector(0, 0, 40); // FIXED: Match player offset for consistency
    
    if (DamageEvent.GetTypeID() == FPointDamageEvent::ClassID)
    {
        const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
        if (!PointDamageEvent->HitInfo.ImpactPoint.IsZero())
        {
            DamageLocation = PointDamageEvent->HitInfo.ImpactPoint;
            DamageLocation.Z = FMath::Max(DamageLocation.Z, GetActorLocation().Z + 40.0f); // FIXED: Consistent offset
        }
    }
    
    // Play hit reaction animation - ALWAYS attempt this for any damage > 0
    if (ActualDamage > 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE DAMAGE: Attempting hit reaction for %.1f damage"), ActualDamage);
        
        // Check components
        bool bHasMesh = GetMesh() != nullptr;
        bool bHasAnimInstance = bHasMesh ? (GetMesh()->GetAnimInstance() != nullptr) : false;
        bool bHasMontage = HitReactMontage != nullptr;
        
        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE COMPONENTS: Mesh=%s, AnimInstance=%s, HitReactMontage=%s"), 
               bHasMesh ? TEXT("✅") : TEXT("❌"),
               bHasAnimInstance ? TEXT("✅") : TEXT("❌"),
               bHasMontage ? TEXT("✅") : TEXT("❌"));
        
        // ALWAYS call PlayHitReactMontage - it has its own fallback systems
        PlayHitReactMontage();
        UE_LOG(LogTemp, Error, TEXT("🎭🧟 ZOMBIE: Hit reaction function called!"));
        
        // ALSO spawn damage numbers
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
        
        if (DamageSystem && IsValid(DamageSystem))
        {
            DamageSystem->SpawnDamageNumberAtLocation(this, DamageLocation, ActualDamage, false);
            UE_LOG(LogTemp, Warning, TEXT("💥 ZombieCharacter: Spawned damage number for %.1f damage"), ActualDamage);
            
            // Visual debug
            DrawDebugSphere(GetWorld(), DamageLocation, 15.0f, 8, FColor::Orange, false, 3.0f, 0, 2.0f);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ ZombieCharacter: Could not find DamageNumberSystem!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🎭🧟 ZOMBIE: No hit reaction - damage was %.1f"), ActualDamage);
    }
    
    // Apply damage to health - SIMPLIFIED
    float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Max(CurrentHealth - ActualDamage, 0.0f);
    
    UE_LOG(LogTemp, Warning, TEXT("💔 ZombieCharacter: Health reduced from %.1f to %.1f (damage: %.1f)"), 
           PreviousHealth, CurrentHealth, ActualDamage);
    
    // Update health bar display
    UpdateHealthBar();
    
    // VISUAL FEEDBACK: Highlight mesh yellow briefly when taking damage
    if (GetMesh() && ActualDamage > 0.0f)
    {
        // Simple and reliable visibility flash - this WILL work regardless of material
        UE_LOG(LogTemp, Warning, TEXT("💛 ZombieCharacter: Starting visibility flash damage feedback"));
        
        // Quick flash sequence: hide -> show -> hide -> show
        GetMesh()->SetVisibility(false);
        
        // First flash restore
        FTimerHandle Flash1Timer;
        GetWorld()->GetTimerManager().SetTimer(Flash1Timer, [this]()
        {
            if (GetMesh())
            {
                GetMesh()->SetVisibility(true);
                UE_LOG(LogTemp, Warning, TEXT("💛 Flash 1: Visible"));
            }
        }, 0.1f, false);
        
        // Second flash hide
        FTimerHandle Flash2Timer;
        GetWorld()->GetTimerManager().SetTimer(Flash2Timer, [this]()
        {
            if (GetMesh())
            {
                GetMesh()->SetVisibility(false);
                UE_LOG(LogTemp, Warning, TEXT("💛 Flash 2: Hidden"));
            }
        }, 0.2f, false);
        
        // Third flash restore
        FTimerHandle Flash3Timer;
        GetWorld()->GetTimerManager().SetTimer(Flash3Timer, [this]()
        {
            if (GetMesh())
            {
                GetMesh()->SetVisibility(true);
                UE_LOG(LogTemp, Warning, TEXT("💛 Flash 3: Visible"));
            }
        }, 0.3f, false);
        
        // Final hide
        FTimerHandle Flash4Timer;
        GetWorld()->GetTimerManager().SetTimer(Flash4Timer, [this]()
        {
            if (GetMesh())
            {
                GetMesh()->SetVisibility(false);
                UE_LOG(LogTemp, Warning, TEXT("💛 Flash 4: Hidden"));
            }
        }, 0.4f, false);
        
        // Restore permanently
        FTimerHandle RestoreTimer;
        GetWorld()->GetTimerManager().SetTimer(RestoreTimer, [this]()
        {
            if (GetMesh())
            {
                GetMesh()->SetVisibility(true);
                UE_LOG(LogTemp, Warning, TEXT("🔄 ZombieCharacter: Damage flash complete - restored visibility"));
            }
        }, 0.5f, false);
    }
    
    // FINAL SAFETY: Ensure the zombie is still in the right place after processing damage
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->Velocity = FVector::ZeroVector;
    }
    
    // CRITICAL: Unlock movement axes after damage processing for normal movement
    if (GetCapsuleComponent() && GetCapsuleComponent()->GetBodyInstance())
    {
        GetCapsuleComponent()->GetBodyInstance()->bLockXTranslation = false; // Allow normal movement
        GetCapsuleComponent()->GetBodyInstance()->bLockYTranslation = false;
        GetCapsuleComponent()->GetBodyInstance()->bLockZTranslation = true;  // Keep Z locked to prevent flying
        GetCapsuleComponent()->GetBodyInstance()->bLockXRotation = true;     // Keep pitch/roll locked
        GetCapsuleComponent()->GetBodyInstance()->bLockYRotation = true;
        GetCapsuleComponent()->GetBodyInstance()->bLockZRotation = false;    // Allow yaw rotation
        
        UE_LOG(LogTemp, Warning, TEXT("🔓 ZombieCharacter: Movement unlocked for normal gameplay"));
    }
    
    // Check if zombie has died
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("💀 ZombieCharacter: Health reached zero, calling HandleDeath"));
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
        // Try to find and update health bar percentage
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
                UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Updated health bar - %.1f/%.1f (%.1f%%)"), 
                       CurrentHealth, MaxHealth, HealthPercentage * 100.0f);
            }
        }
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
