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
    
    // Look for the player in the world
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;
        
        // Try to cast to player character
        AAtlantisEonsCharacter* PlayerCharacter = Cast<AAtlantisEonsCharacter>(Actor);
        if (PlayerCharacter)
        {
            // Check if player is within attack range
            float DistanceToPlayer = FVector::Distance(GetActorLocation(), Actor->GetActorLocation());
            
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Found player"));
            
            if (DistanceToPlayer <= SphereRadius * 1.5f) // Give a bit extra range for reliable hits
            {
                UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Player in DIRECT attack range"));
                
                // Apply damage to the player
                UGameplayStatics::ApplyDamage(Actor, AttackDamage, GetController(), this, UDamageType::StaticClass());
                
                // Visual feedback
                DrawDebugSphere(GetWorld(), Actor->GetActorLocation(), 50.0f, 12, FColor::Red, false, 2.0f);
                
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
                    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: OVERLAP hit"));
                    
                    // Apply damage to the player
                    UGameplayStatics::ApplyDamage(static_cast<AActor*>(PlayerCharacter), AttackDamage, GetController(), this, UDamageType::StaticClass());
                    
                    // Visual feedback
                    DrawDebugSphere(GetWorld(), HitActor->GetActorLocation(), 50.0f, 12, FColor::Red, false, 2.0f);
                    
                    // Attempt to play hit react animation
                    PlayerCharacter->PlayHitReactMontage();
                    break; // Found and hit the player, no need to check other overlaps
                }
            }
        }
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
    if (HitReactMontage && GetMesh())
    {
        GetMesh()->GetAnimInstance()->Montage_Play(HitReactMontage);
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
    bIsDead = true;

    // Disable movement
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // Play death animation if available
    if (DeathMontage)
    {
        PlayAnimMontage(DeathMontage);

        // Set up timer to destroy the actor after animation
        float MontageLength = DeathMontage->GetPlayLength();
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            OnDeath();
        }, MontageLength, false);
    }
    else
    {
        // If no death montage, call OnDeath immediately
        OnDeath();
    }
}

void AZombieCharacter::OnDeath()
{
    HandleDeath();
}

// Keep track of damage event IDs we've already processed to prevent duplicate damage numbers
static TMap<uint32, float> ProcessedDamageEvents;

float AZombieCharacter::TakeDamage_Implementation(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // Implement damage logic here, e.g., reduce health and check for death
    if (bIsDead) return 0.0f;
    CurrentHealth -= DamageAmount;
    if (CurrentHealth <= 0.0f)
    {
        CurrentHealth = 0.0f;
        bIsDead = true;
        OnDeath();
    }
    return DamageAmount;
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
