#include "BTTask_MoveToPlayer.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

UBTTask_MoveToPlayer::UBTTask_MoveToPlayer()
{
    NodeName = TEXT("Move To Player");
    bNotifyTick = true;
    AcceptableRadius = 120.0f; // Match exactly with attack range to ensure consistent behavior
}

EBTNodeResult::Type UBTTask_MoveToPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    
    if (!AIController || !Blackboard)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToPlayer: Missing AIController or Blackboard"));
        return EBTNodeResult::Failed;
    }

    // Get the player reference from blackboard
    AActor* Player = Cast<AActor>(Blackboard->GetValueAsObject(AZombieAIController::PlayerKey));
    if (!Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToPlayer: No player found in blackboard"));
        return EBTNodeResult::Failed;
    }

    // Get zombie character
    AZombieCharacter* Zombie = Cast<AZombieCharacter>(AIController->GetPawn());
    if (!Zombie)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToPlayer: Failed to get Zombie character"));
        return EBTNodeResult::Failed;
    }

    FVector PlayerLocation = Player->GetActorLocation();
    FVector ZombieLocation = Zombie->GetActorLocation();
    float DistanceToPlayer = FVector::Dist2D(ZombieLocation, PlayerLocation);
    
    UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToPlayer: ExecuteTask called"));
    UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToPlayer: Distance=%.1f, AttackRange=%.1f"), 
           DistanceToPlayer, Zombie->GetAttackRange());

    if (DistanceToPlayer <= Zombie->GetAttackRange())
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToPlayer: Already in attack range"));
        Blackboard->SetValueAsBool(AZombieAIController::IsInAttackRangeKey, true);
        return EBTNodeResult::Succeeded;
    }
    else
    {
        Blackboard->SetValueAsBool(AZombieAIController::IsInAttackRangeKey, false);
    }
    
    // Store the goal location for future reference
    FBTMoveToTaskMemory* MyMemory = (FBTMoveToTaskMemory*)NodeMemory;
    MyMemory->PreviousGoalLocation = PlayerLocation;
    
    // Direct movement - no pathfinding
    FVector Direction = (PlayerLocation - ZombieLocation).GetSafeNormal2D();
    Zombie->AddMovementInput(Direction, 1.0f);
    
    UE_LOG(LogTemp, Warning, TEXT("MoveToPlayer: Started direct movement to player at location: X=%.1f, Y=%.1f, Z=%.1f"), 
        PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z);

    return EBTNodeResult::InProgress;
}

void UBTTask_MoveToPlayer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    
    if (!AIController || !Blackboard)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Get the player reference from blackboard
    AActor* Player = Cast<AActor>(Blackboard->GetValueAsObject(AZombieAIController::PlayerKey));
    if (!Player)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Get zombie character
    AZombieCharacter* Zombie = Cast<AZombieCharacter>(AIController->GetPawn());
    if (!Zombie)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    FVector PlayerLocation = Player->GetActorLocation();
    FVector ZombieLocation = Zombie->GetActorLocation();
    float DistanceToPlayer = FVector::DistXY(ZombieLocation, PlayerLocation);
    
    UE_LOG(LogTemp, Warning, TEXT("MoveToPlayer Tick: Distance2D=%.1f, AcceptableRadius=%.1f, AttackRange=%.1f"), 
           DistanceToPlayer, AcceptableRadius, Zombie->GetAttackRange());

    if (DistanceToPlayer <= Zombie->GetAttackRange())
    {
        Blackboard->SetValueAsBool(AZombieAIController::IsInAttackRangeKey, true);
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // Always update movement direction to follow player
    FVector Direction = (PlayerLocation - ZombieLocation).GetSafeNormal2D();
    Zombie->AddMovementInput(Direction, 1.0f);
    
    // Make the zombie face the player
    FRotator LookAtRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
    LookAtRotation.Pitch = 0.0f;
    LookAtRotation.Roll = 0.0f;
    Zombie->SetActorRotation(LookAtRotation);
    
    // Update stored location
    FBTMoveToTaskMemory* MyMemory = (FBTMoveToTaskMemory*)NodeMemory;
    MyMemory->PreviousGoalLocation = PlayerLocation;
}

void UBTTask_MoveToPlayer::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

    UE_LOG(LogTemp, Warning, TEXT("MoveToPlayer: Task finished with result: %s"), 
           TaskResult == EBTNodeResult::Succeeded ? TEXT("Succeeded") : 
           TaskResult == EBTNodeResult::Failed ? TEXT("Failed") : 
           TaskResult == EBTNodeResult::Aborted ? TEXT("Aborted") : TEXT("InProgress"));

    // Stop movement when task is finished
    if (AAIController* AIController = OwnerComp.GetAIOwner())
    {
        AIController->StopMovement();
    }
}
