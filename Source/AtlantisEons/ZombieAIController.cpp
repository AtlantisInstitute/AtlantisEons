#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Perception/AIPerceptionSystem.h"
#include "Navigation/PathFollowingComponent.h"

// Initialize static blackboard key names
const FName AZombieAIController::PlayerKey("PlayerActor");
const FName AZombieAIController::TargetLocationKey("TargetLocation");
const FName AZombieAIController::IsInAttackRangeKey("IsInAttackRange");
const FName AZombieAIController::PatrolRadiusKey("PatrolRadius");

AZombieAIController::AZombieAIController()
{
    // Enable tick
    PrimaryActorTick.bCanEverTick = true;

    // Create and set up behavior tree component
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    
    // Create and set up blackboard component
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

    // Create and set up AI perception component
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
    
    // Set up sight configuration
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));
    check(SightConfig);
    
    // Configure sight settings
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 2000.0f;
    SightConfig->PeripheralVisionAngleDegrees = 360.0f;
    SightConfig->SetMaxAge(5.0f);
    SightConfig->AutoSuccessRangeFromLastSeenLocation = 0.0f;
    
    // Detection by affiliation
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = false;

    // Set team ID (team 2 for zombies)
    SetGenericTeamId(FGenericTeamId(2));
    
    // Teleport protection variables have been disabled for manual implementation
    LastKnownPlayerLocation = FVector::ZeroVector;
    LastPlayerLocationUpdateTime = 0.0f;
    TeleportDetectionThreshold = 0.0f;  // Disabled
    TeleportRecoveryDelay = 0.0f;       // Disabled
    bIsInTeleportRecovery = false;
    TeleportRecoveryTimer = 0.0f;

    // Add sight config to perception component
    if (AIPerceptionComponent)
    {
        AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
        AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AZombieAIController::OnTargetPerceptionUpdated);
        AIPerceptionComponent->ConfigureSense(*SightConfig);
        AIPerceptionComponent->SetActive(true);
        
        UE_LOG(LogTemp, Warning, TEXT("AI Perception Component configured in constructor"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AIPerceptionComponent is null in constructor"));
    }
}

void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();

    // // Debug log the team ID
    // UE_LOG(LogTemp, Warning, TEXT("ZombieAIController Team ID: %d"), GetGenericTeamId().GetId());

    // Force perception system update
    if (AIPerceptionComponent)
    {
        // Re-register sight config
        if (SightConfig)
        {
            // Update sight config settings
            SightConfig->SightRadius = 1500.0f;
            SightConfig->LoseSightRadius = 2000.0f;
            SightConfig->PeripheralVisionAngleDegrees = 360.0f;
            SightConfig->SetMaxAge(5.0f);
            SightConfig->AutoSuccessRangeFromLastSeenLocation = 0.0f;  // Disable auto success
            SightConfig->DetectionByAffiliation.bDetectEnemies = true;
            SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
            SightConfig->DetectionByAffiliation.bDetectNeutrals = false;

            AIPerceptionComponent->ConfigureSense(*SightConfig);
            // UE_LOG(LogTemp, Warning, TEXT("Re-registered sight config in BeginPlay"));
        }

        AIPerceptionComponent->RequestStimuliListenerUpdate();
        // UE_LOG(LogTemp, Warning, TEXT("Perception system activated and updated in BeginPlay"));

        // // Debug perception state
        // TArray<AActor*> PerceivedActors;
        // AIPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
        // UE_LOG(LogTemp, Warning, TEXT("Initial perceived actors: %d"), PerceivedActors.Num());

        // // Debug sight configuration
        // UE_LOG(LogTemp, Warning, TEXT("Sight Config Status:"));
        // UE_LOG(LogTemp, Warning, TEXT("  - Implementation Class: %s"), *SightConfig->GetSenseImplementation()->GetName());
        // UE_LOG(LogTemp, Warning, TEXT("  - Sight Radius: %.2f"), SightConfig->SightRadius);
        // UE_LOG(LogTemp, Warning, TEXT("  - Lose Sight Radius: %.2f"), SightConfig->LoseSightRadius);
        // UE_LOG(LogTemp, Warning, TEXT("  - Peripheral Vision Angle: %.2f"), SightConfig->PeripheralVisionAngleDegrees);
        // UE_LOG(LogTemp, Warning, TEXT("  - Detect Enemies: %s"), SightConfig->DetectionByAffiliation.bDetectEnemies ? TEXT("true") : TEXT("false"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AIPerceptionComponent is null in BeginPlay"));
    }

    // // Debug: Print all actors in the world
    // TArray<AActor*> AllActors;
    // UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllActors);
    // UE_LOG(LogTemp, Warning, TEXT("Number of characters in world: %d"), AllActors.Num());
    // for (AActor* Actor : AllActors)
    // {
    //     if (Actor)
    //     {
    //         // Print actor name and team ID if it implements IGenericTeamAgentInterface
    //         IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
    //         if (TeamAgent)
    //         {
    //             FGenericTeamId OtherTeamID = TeamAgent->GetGenericTeamId();
    //             ETeamAttitude::Type Attitude = GetTeamAttitudeTowards(*Actor);
    //             UE_LOG(LogTemp, Warning, TEXT("Character in world: %s (Team ID: %d, Attitude: %d)"), 
    //                 *Actor->GetName(), OtherTeamID.GetId(), (int32)Attitude);
    //         }
    //         else
    //         {
    //             UE_LOG(LogTemp, Warning, TEXT("Character in world: %s (No Team ID)"), *Actor->GetName());
    //         }
    //     }
    // }
}

void AZombieAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Teleport/dash recovery code removed for manual implementation
    // This previously made zombies confused when player used dash abilities
    if (bIsInTeleportRecovery)
    {
        // Reset the flag so we don't get stuck in recovery mode
        bIsInTeleportRecovery = false;
    }

    // // Debug perception every few seconds
    // static float TimeSinceLastDebug = 0.0f;
    // TimeSinceLastDebug += DeltaTime;
    // if (TimeSinceLastDebug >= 2.0f)
    // {
    //     TimeSinceLastDebug = 0.0f;

    //     if (AIPerceptionComponent)
    //     {
    //         TArray<AActor*> PerceivedActors;
    //         AIPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
            
    //         UE_LOG(LogTemp, Warning, TEXT("Number of currently perceived actors: %d"), PerceivedActors.Num());
    //         UE_LOG(LogTemp, Warning, TEXT("Perception Component Active: %s"), AIPerceptionComponent->IsActive() ? TEXT("true") : TEXT("false"));
    //         UE_LOG(LogTemp, Warning, TEXT("AI Controller Valid: %s"), IsValid(this) ? TEXT("true") : TEXT("false"));
            
    //         if (APawn* ControlledPawn = GetPawn())
    //         {
    //             UE_LOG(LogTemp, Warning, TEXT("Controlled Pawn Location: X=%.3f Y=%.3f Z=%.3f"), 
    //                 ControlledPawn->GetActorLocation().X,
    //                 ControlledPawn->GetActorLocation().Y,
    //                 ControlledPawn->GetActorLocation().Z);
    //         }
            
    //         UE_LOG(LogTemp, Warning, TEXT("Sight Config - Detect Enemies: %s"), 
    //             SightConfig->DetectionByAffiliation.bDetectEnemies ? TEXT("true") : TEXT("false"));
    //         UE_LOG(LogTemp, Warning, TEXT("Sight Config - Sight Radius: %.2f"), 
    //             SightConfig->SightRadius);

    //         // Draw debug sphere to visualize sight radius
    //         if (GetPawn())
    //         {
    //             DrawDebugSphere(
    //                 GetWorld(),
    //                 GetPawn()->GetActorLocation(),
    //                 SightConfig->SightRadius,
    //                 32,
    //                 bIsInTeleportRecovery ? FColor::Yellow : FColor::Red,  // Yellow during recovery
    //                 false,
    //                 2.1f
    //             );
    //         }
            
    //         // Debug teleport detection system
    //         if (LastKnownPlayerLocation != FVector::ZeroVector)
    //         {
    //             UE_LOG(LogTemp, Warning, TEXT("Teleport Detection - Recovery: %s, Threshold: %.1f, Last Update: %.1f"),
    //                 bIsInTeleportRecovery ? TEXT("Active") : TEXT("Inactive"),
    //                 TeleportDetectionThreshold,
    //                 GetWorld()->GetTimeSeconds() - LastPlayerLocationUpdateTime);
    //         }
    //     }
    // }
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // UE_LOG(LogTemp, Warning, TEXT("OnPossess called for %s"), *InPawn->GetName());
    
    // // Log the class of the possessed pawn
    // UE_LOG(LogTemp, Warning, TEXT("Possessed pawn class: %s"), *InPawn->GetClass()->GetName());

    // Try to cast to ZombieCharacter
    AZombieCharacter* ZombieChar = Cast<AZombieCharacter>(InPawn);
    // if (ZombieChar)
    // {
    //     UE_LOG(LogTemp, Warning, TEXT("Successfully cast possessed pawn to ZombieCharacter"));
    // }
    // else
    // {
    //     UE_LOG(LogTemp, Warning, TEXT("Failed to cast possessed pawn to ZombieCharacter"));
    // }
    
    // Initialize blackboard
    if (!BehaviorTree)
    {
        UE_LOG(LogTemp, Error, TEXT("BehaviorTree is null! Make sure to set it in the Blueprint"));
        return;
    }

    if (!BehaviorTree->BlackboardAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("BlackboardAsset is null in the BehaviorTree!"));
        return;
    }

    if (!BlackboardComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("BlackboardComponent is null!"));
        return;
    }

    if (!BehaviorTreeComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("BehaviorTreeComponent is null!"));
        return;
    }

    // Initialize blackboard
    BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
    // UE_LOG(LogTemp, Warning, TEXT("Blackboard initialized"));

    // Start behavior tree
    BehaviorTreeComponent->StartTree(*BehaviorTree);
    // UE_LOG(LogTemp, Warning, TEXT("Behavior Tree started"));

    // Make sure perception is set up
    if (AIPerceptionComponent)
    {
        // Re-bind perception event
        AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AZombieAIController::OnTargetPerceptionUpdated);
        AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AZombieAIController::OnTargetPerceptionUpdated);
        
        // Force update
        AIPerceptionComponent->RequestStimuliListenerUpdate();
        // UE_LOG(LogTemp, Warning, TEXT("Perception system activated after possession"));

        // // Debug current perception state
        // TArray<AActor*> PerceivedActors;
        // AIPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
        // UE_LOG(LogTemp, Warning, TEXT("Number of perceived actors at possession: %d"), PerceivedActors.Num());
        
        // for (AActor* Actor : PerceivedActors)
        // {
        //     if (Actor)
        //     {
        //         UE_LOG(LogTemp, Warning, TEXT("  - Perceived: %s"), *Actor->GetName());
        //         if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor))
        //         {
        //             UE_LOG(LogTemp, Warning, TEXT("    Team ID: %d"), TeamAgent->GetGenericTeamId().GetId());
        //         }
        //     }
        // }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AIPerceptionComponent is null!"));
    }
}

void AZombieAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor || !BlackboardComponent)
    {
        return;
    }

    // UE_LOG(LogTemp, Warning, TEXT("OnTargetPerceptionUpdated called"));
    // UE_LOG(LogTemp, Warning, TEXT("  - Actor: %s"), *Actor->GetName());
    // UE_LOG(LogTemp, Warning, TEXT("  - Stimulus Type Index: %d"), Stimulus.Type.Index);
    // UE_LOG(LogTemp, Warning, TEXT("  - Stimulus Strength: %.2f"), Stimulus.Strength);
    // UE_LOG(LogTemp, Warning, TEXT("  - Was Successfully Sensed: %s"), Stimulus.WasSuccessfullySensed() ? TEXT("true") : TEXT("false"));

    // // Get the team ID and attitude of the detected actor
    // IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
    // FGenericTeamId ActorTeamId = TeamAgent ? TeamAgent->GetGenericTeamId() : FGenericTeamId::NoTeam;
    
    // UE_LOG(LogTemp, Warning, TEXT("  - Actor Team ID: %d"), ActorTeamId.GetId());
    // UE_LOG(LogTemp, Warning, TEXT("  - Attitude: %d"), GetTeamAttitudeTowards(*Actor));

    // Get the perception component
    UAIPerceptionComponent* PerceptionComp = GetAIPerceptionComponent();
    if (!PerceptionComp)
    {
        // UE_LOG(LogTemp, Error, TEXT("AIPerceptionComponent is null!"));
        return;
    }

    // If we successfully sensed a hostile actor, update the blackboard
    if (Stimulus.WasSuccessfullySensed() && GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile)
    {
        // Check for teleportation if we already have a last known location
        if (LastKnownPlayerLocation != FVector::ZeroVector)
        {
            FVector NewLocation = Actor->GetActorLocation();
            if (IsPlayerTeleport(NewLocation))
            {
                // Handle teleport - don't immediately update target location
                HandlePlayerTeleport(NewLocation);
                // UE_LOG(LogTemp, Warning, TEXT("TELEPORT DETECTED - AI in recovery for %.1f seconds"), TeleportRecoveryDelay);
                return;
            }
        }
        
        // Normal update
        BlackboardComponent->SetValueAsObject(PlayerKey, Actor);
        LastKnownPlayerLocation = Actor->GetActorLocation();
        LastPlayerLocationUpdateTime = GetWorld()->GetTimeSeconds();
        // UE_LOG(LogTemp, Warning, TEXT("Updated blackboard with hostile actor: %s"), *Actor->GetName());
    }
    else if (!Stimulus.WasSuccessfullySensed())
    {
        // If we lost sight of the actor and it was our target, clear it from the blackboard
        if (BlackboardComponent->GetValueAsObject(PlayerKey) == Actor)
        {
            BlackboardComponent->ClearValue(PlayerKey);
            BlackboardComponent->SetValueAsBool(IsInAttackRangeKey, false);
            // UE_LOG(LogTemp, Warning, TEXT("Cleared blackboard target: %s"), *Actor->GetName());
        }
    }

    // // Debug perception state
    // TArray<AActor*> PerceivedActors;
    // PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
    // UE_LOG(LogTemp, Warning, TEXT("Currently perceived actors: %d"), PerceivedActors.Num());

    // Make sure perception stays active
    if (!PerceptionComp->IsActive())
    {
        // UE_LOG(LogTemp, Warning, TEXT("Reactivating perception component"));
        PerceptionComp->Activate();
    }
}

bool AZombieAIController::IsPlayerTeleport(const FVector& NewLocation)
{
    // Don't detect teleports if we're still in recovery
    if (bIsInTeleportRecovery)
    {
        return false;
    }
    
    float TimeSinceLastUpdate = GetWorld()->GetTimeSeconds() - LastPlayerLocationUpdateTime;
    if (TimeSinceLastUpdate <= 0.001f) // Avoid division by zero
    {
        return false;
    }

    // Calculate speed of movement
    float Distance = FVector::Dist(LastKnownPlayerLocation, NewLocation);
    float Speed = Distance / TimeSinceLastUpdate;
    
    // // Log detection information
    // UE_LOG(LogTemp, Warning, TEXT("Movement Check - Distance: %.1f, Time: %.3f, Speed: %.1f"), 
    //        Distance, TimeSinceLastUpdate, Speed);
    
    // Check if movement speed exceeds detection threshold
    // Movement detection removed for manual implementation
    return false; // Disabled teleport detection
}

void AZombieAIController::HandlePlayerTeleport(const FVector& NewLocation)
{
    // Teleport handling has been disabled for manual implementation
    // This function previously made zombies confused when detecting player teleport/dash
    
    // Simply update the location without any special handling
    LastKnownPlayerLocation = NewLocation;
    LastPlayerLocationUpdateTime = GetWorld()->GetTimeSeconds();
    
    // UE_LOG(LogTemp, Warning, TEXT("ZombieAIController HandlePlayerTeleport - Functionality disabled for manual implementation"));
}

void AZombieAIController::UpdateMovementAfterTeleport()
{
    // Function disabled - previously used to handle zombie confusion after player dash
    bIsInTeleportRecovery = false;
    
    // // Log that function is disabled
    // UE_LOG(LogTemp, Warning, TEXT("AZombieAIController::UpdateMovementAfterTeleport - Function disabled for manual implementation"));
}
