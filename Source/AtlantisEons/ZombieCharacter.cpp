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

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;  // Character should face movement direction
    bUseControllerRotationYaw = false;  // Don't use controller rotation
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 400.0f; // Reduced slightly to make zombie easier to escape
    GetCharacterMovement()->bUseRVOAvoidance = true; // Enable avoidance to prevent zombies getting stuck
    GetCharacterMovement()->bRequestedMoveUseAcceleration = true;
    GetCharacterMovement()->MaxAcceleration = 2048.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
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

    // FIXED: Ensure physics settings are properly configured at runtime
    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetSimulatePhysics(false);
        GetCapsuleComponent()->SetEnableGravity(false);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        
        // ENHANCED: More robust physics prevention
        GetCapsuleComponent()->SetMassOverrideInKg(NAME_None, 1000.0f, true); // Make very heavy
        GetCapsuleComponent()->SetLinearDamping(10.0f); // High damping to resist movement
        GetCapsuleComponent()->SetAngularDamping(10.0f); // High rotational damping
        GetCapsuleComponent()->SetUseCCD(false); // Disable continuous collision detection
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Enhanced capsule physics - SimulatePhysics: false, EnableGravity: false, Mass: 1000kg"));
    }
    
    if (GetMesh())
    {
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        
        // ENHANCED: Ensure mesh has no physics interactions
        GetMesh()->SetMassOverrideInKg(NAME_None, 0.0f, false); // Zero mass
        GetMesh()->SetEnableGravity(false);
        GetMesh()->SetUseCCD(false);
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Enhanced mesh physics - SimulatePhysics: false, Mass: 0kg"));
    }
    
    // ENHANCED: Configure character movement to resist external forces
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->bIgnoreBaseRotation = true;
        GetCharacterMovement()->bIgnoreClientMovementErrorChecksAndCorrection = true;
        GetCharacterMovement()->Mass = 1000.0f; // Very heavy to resist physics impulses
        GetCharacterMovement()->GroundFriction = 20.0f; // High friction to resist sliding
        GetCharacterMovement()->MaxAcceleration = 2048.0f;
        GetCharacterMovement()->BrakingDecelerationWalking = 4096.0f; // High braking
        GetCharacterMovement()->bApplyGravityWhileJumping = false;
        GetCharacterMovement()->bNotifyApex = false;
        GetCharacterMovement()->bCanWalkOffLedges = false; // Prevent falling off edges
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Enhanced movement physics - Mass: 1000kg, High friction"));
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
    // FIXED: Enhanced hit reaction animation system
    if (!HitReactMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎭 ZombieCharacter: No HitReactMontage assigned!"));
        return;
    }
    
    if (!GetMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("🎭 ZombieCharacter: No mesh component found!"));
        return;
    }
    
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("🎭 ZombieCharacter: No AnimInstance found!"));
        return;
    }
    
    // Stop any currently playing montage first
    if (AnimInstance->IsAnyMontagePlaying())
    {
        AnimInstance->StopAllMontages(0.1f);
        UE_LOG(LogTemp, Warning, TEXT("🎭 ZombieCharacter: Stopped existing montages"));
    }
    
    // Play the hit reaction montage
    float MontageLength = AnimInstance->Montage_Play(HitReactMontage, 1.0f);
    
    if (MontageLength > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎭 ZombieCharacter: Successfully started hit reaction montage (length: %.2fs)"), MontageLength);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🎭 ZombieCharacter: Failed to play hit reaction montage!"));
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
        
        // Ensure physics are completely disabled
        GetCapsuleComponent()->SetSimulatePhysics(false);
        GetCapsuleComponent()->SetEnableGravity(false);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        
        // FORCE: Reset velocity and stop any movement
        if (GetCharacterMovement())
        {
            GetCharacterMovement()->StopMovementImmediately();
            GetCharacterMovement()->Velocity = FVector::ZeroVector;
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
        
        // SAFETY: Reset location if it changed unexpectedly
        SetActorLocation(CurrentLocation, false, nullptr, ETeleportType::ResetPhysics);
        
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
    FVector DamageLocation = GetActorLocation() + FVector(0, 0, 120); // Above zombie's head
    
    if (DamageEvent.GetTypeID() == FPointDamageEvent::ClassID)
    {
        const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
        if (!PointDamageEvent->HitInfo.ImpactPoint.IsZero())
        {
            DamageLocation = PointDamageEvent->HitInfo.ImpactPoint;
            DamageLocation.Z = FMath::Max(DamageLocation.Z, GetActorLocation().Z + 80.0f);
        }
    }
    
    // ALWAYS spawn damage numbers for any damage > 0
    if (ActualDamage > 0.0f)
    {
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
    
    // Apply damage to health - SIMPLIFIED
    float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Max(CurrentHealth - ActualDamage, 0.0f);
    
    UE_LOG(LogTemp, Warning, TEXT("💔 ZombieCharacter: Health reduced from %.1f to %.1f (damage: %.1f)"), 
           PreviousHealth, CurrentHealth, ActualDamage);
    
    // Play hit reaction animation
    if (HitReactMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        PlayHitReactMontage();
        UE_LOG(LogTemp, Warning, TEXT("🎭 ZombieCharacter: Playing hit reaction montage"));
    }
    
    // Check if zombie has died
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("💀 ZombieCharacter: Health reached zero, calling HandleDeath"));
        HandleDeath();
    }
    
    // FINAL SAFETY: Ensure the zombie is still in the right place after processing damage
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->Velocity = FVector::ZeroVector;
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

void AZombieCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AZombieCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}
