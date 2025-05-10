#include "ZombieCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"
#include "AtlantisEonsCharacter.h"
#include "DamageNumberSystem.h"
#include "EngineUtils.h" // For TActorIterator

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

    // Configure collision
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block); // Block by default
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // But overlap with other pawns
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Ignore camera

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
                
                // DIRECT APPROACH: Apply damage to the player directly
                UGameplayStatics::ApplyDamage(PlayerCharacter, AttackDamage, GetController(), this, UDamageType::StaticClass());
                
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
        for (auto& Overlap : Overlaps)
        {
            AActor* HitActor = Overlap.GetActor();
            
            // Check if we hit a player character
            if (HitActor && HitActor != this && HitActor->IsA(AAtlantisEonsCharacter::StaticClass()))
            {
                PlayerCharacter = Cast<AAtlantisEonsCharacter>(HitActor);
                if (PlayerCharacter)
                {
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: OVERLAP hit. Dealing %f damage to %s"), 
                           AttackDamage, *PlayerCharacter->GetName());
                    
                    // Apply damage to the player
                    UGameplayStatics::ApplyDamage(PlayerCharacter, AttackDamage, GetController(), this, UDamageType::StaticClass());
                    
                    // Visual feedback
                    DrawDebugSphere(GetWorld(), HitActor->GetActorLocation(), 35.0f, 12, FColor::Green, false, 2.0f);
                    
                    // Attempt to play hit react animation on the player
                    PlayerCharacter->PlayHitReactMontage();
                    break; // Found and hit the player, no need to check other overlaps
                }
            }
        }
    }
}

void AZombieCharacter::PlayHitReactMontage()
{
    if (HitReactMontage && GetMesh())
    {
        GetMesh()->GetAnimInstance()->Montage_Play(HitReactMontage);
    }
}

void AZombieCharacter::ApplyDamage(float DamageAmount)
{
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter::ApplyDamage: Amount=%f, Current Health=%f, Dead=%s"),
        DamageAmount, CurrentHealth, bIsDead ? TEXT("Yes") : TEXT("No"));
    
    // Exit early if already dead
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Already dead, ignoring damage"));
        return;
    }

    // Play hit reaction if we took damage
    if (DamageAmount > 0.0f)
    {
        PlayHitReactMontage();
    }

    if (CurrentHealth > 0.0f)
    {
        // Calculate new health
        float PreviousHealth = CurrentHealth;
        CurrentHealth = FMath::Max(CurrentHealth - DamageAmount, 0.0f);
        
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Damage applied: New Health = %f, Max Health = %f"), 
            CurrentHealth, MaxHealth);
        
        // NOTE: Damage numbers are now handled in TakeDamage_Implementation instead of here
        // This prevents duplicate damage numbers from appearing
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter::ApplyDamage: Amount=%.6f, Current Health=%.6f, Dead=%s"),
               DamageAmount, CurrentHealth, bIsDead ? TEXT("Yes") : TEXT("No"));
        
        // Check if health is zero or less and zombie is not already dead
        if (CurrentHealth <= 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Health reached zero, calling HandleDeath"));
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

    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Setting bIsDead to true and disabling movement"));
    bIsDead = true;

    // Disable movement
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // Play death animation if available
    if (DeathMontage)
    {
        PlayAnimMontage(DeathMontage);
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Playing death animation"));
        
        // Set a timer to destroy the actor after the death animation
        float MontageLength = DeathMontage->GetPlayLength();
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Death animation finished, destroying actor"));
            Destroy();
        }, MontageLength, false);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: No death animation, destroying actor immediately"));
        Destroy();
    }
}

// Keep track of damage event IDs we've already processed to prevent duplicate damage numbers
static TMap<uint32, float> ProcessedDamageEvents;

float AZombieCharacter::TakeDamage_Implementation(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: TakeDamage called with damage amount: %f from %s"), 
           DamageAmount, 
           DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"));
    
    // Early out if damage is not applicable
    if (DamageAmount <= 0.0f || bIsDead)
    {
        return 0.0f;
    }
    
    // Check if damage is coming from the player - we want to spawn damage numbers if it is
    bool bDamageFromPlayer = false;
    if (DamageCauser && DamageCauser->IsA(AAtlantisEonsCharacter::StaticClass()))
    {
        bDamageFromPlayer = true;
        UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter TakeDamage: Damage from player character"));
    }
    
    // Handle damage numbers for player-caused damage only
    if (bDamageFromPlayer)
    {
        // IMPORTANT: We use game frame to prevent duplicates within the same tick/frame
        // This is more reliable than time-based deduplication
        // Using instance variables (declared in header) instead of static ones
        // This prevents false duplicate detection between different zombies
        
        uint64 CurrentFrame = GFrameCounter;
        FVector CurrentDamageLocation = GetActorLocation();
        
        // Try to get precise hit location if available
        if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
        {
            const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
            if (PointDamageEvent)
            {
                CurrentDamageLocation = PointDamageEvent->HitInfo.ImpactPoint;
            }
        }
        
        // Check for duplicate hits within the same or very recent frames
        bool bIsDuplicate = false;
        if (CurrentFrame <= LastDamageFrame + 2 && // Allow for small frame differences
            LastDamageCauser == DamageCauser && 
            FMath::IsNearlyEqual(LastDamageAmount, DamageAmount, 0.1f) &&
            FVector::DistSquared(LastDamageLocation, CurrentDamageLocation) < 10000.0f) // 100 units squared
        {
            bIsDuplicate = true;
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter %s: Detected duplicate damage in frame %llu (last: %llu), SKIPPING damage number"),
                   *GetName(), CurrentFrame, LastDamageFrame);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter %s: New damage event in frame %llu (last: %llu)"),
                   *GetName(), CurrentFrame, LastDamageFrame);
        }
        
        // Update tracking variables for next time
        LastDamageFrame = CurrentFrame;
        LastDamageCauser = DamageCauser;
        LastDamageAmount = DamageAmount;
        LastDamageLocation = CurrentDamageLocation;
        
        // Only spawn damage numbers for non-duplicate hits
        if (!bIsDuplicate)
        {
            // Get damage number system instance
            ADamageNumberSystem* DamageSystem = nullptr;
            DamageSystem = ADamageNumberSystem::GetInstance(GetWorld());
            
            if (!DamageSystem)
            {
                TArray<AActor*> FoundActors;
                UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADamageNumberSystem::StaticClass(), FoundActors);
                
                if (FoundActors.Num() > 0)
                {
                    DamageSystem = Cast<ADamageNumberSystem>(FoundActors[0]);
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter TakeDamage: Found DamageNumberSystem: %s"), 
                           *DamageSystem->GetName());
                }
            }
            
            if (DamageSystem)
            {
                // Draw debug sphere to visualize where the damage number will spawn
                DrawDebugSphere(GetWorld(), CurrentDamageLocation, 10.0f, 8, FColor::Red, false, 2.0f, 0, 1.0f);
                
                // Spawn the damage number at the exact hit location
                DamageSystem->SpawnDamageNumberAtLocation(this, CurrentDamageLocation, DamageAmount, false);
                UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter TakeDamage: Spawned damage number at hit location for %.1f damage"), DamageAmount);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("ZombieCharacter TakeDamage: Could not find DamageNumberSystem!"));
            }
        }
    }
    
    // Apply the damage to health regardless of whether we spawned a number
    ApplyDamage(DamageAmount);
    
    return DamageAmount;
}
