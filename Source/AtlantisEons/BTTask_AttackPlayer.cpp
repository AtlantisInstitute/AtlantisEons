#include "BTTask_AttackPlayer.h"
#include "AIController.h"
#include "ZombieCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ZombieAIController.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

UBTTask_AttackPlayer::UBTTask_AttackPlayer()
{
    NodeName = TEXT("Attack Player");
    bNotifyTick = true;
    AttackCooldown = 2.0f;
    
    // Log when the task is created
    UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Task created"));
}

EBTNodeResult::Type UBTTask_AttackPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: ExecuteTask called"));

    // Get the AI controller and blackboard
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    
    if (!AIController || !Blackboard)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Missing AIController or Blackboard"));
        return EBTNodeResult::Failed;
    }

    // Get the zombie character
    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: AIController's Pawn is null"));
        return EBTNodeResult::Failed;
    }

    UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Controlled Pawn Class: %s"), *ControlledPawn->GetClass()->GetName());
    
    // Log the AIController's details
    UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: AIController=%s, AIController Class=%s"),
        *AIController->GetName(),
        *AIController->GetClass()->GetName());

    // Log the controlled pawn's details
    UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: ControlledPawn=%s, Class=%s, Location=%s"),
        *ControlledPawn->GetName(),
        *ControlledPawn->GetClass()->GetName(),
        *ControlledPawn->GetActorLocation().ToString());

    // Try to get the ZombieCharacter interface
    IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(ControlledPawn);
    if (TeamAgent)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Pawn implements IGenericTeamAgentInterface with Team ID: %d"),
            TeamAgent->GetGenericTeamId().GetId());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Pawn does not implement IGenericTeamAgentInterface"));
    }
    
    AZombieCharacter* Zombie = Cast<AZombieCharacter>(ControlledPawn);
    if (!Zombie)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Failed to cast Pawn to ZombieCharacter"));
        return EBTNodeResult::Failed;
    }

    // Get the player from blackboard
    AActor* Player = Cast<AActor>(Blackboard->GetValueAsObject(AZombieAIController::PlayerKey));
    if (!Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: No player found in blackboard"));
        return EBTNodeResult::Failed;
    }

    // Double check the actual distance
    UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Calculating distance to player"));
    float DistanceToPlayer = FVector::Distance(Zombie->GetActorLocation(), Player->GetActorLocation());
    bool bIsInAttackRange = DistanceToPlayer <= Zombie->GetAttackRange();
    bool bBlackboardInRange = Blackboard->GetValueAsBool(AZombieAIController::IsInAttackRangeKey);
    
    UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Distance=%.1f, InRange=%s, BlackboardInRange=%s"), 
           DistanceToPlayer, bIsInAttackRange ? TEXT("true") : TEXT("false"),
           bBlackboardInRange ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Attack range set to %.1f"), Zombie->GetAttackRange());
    
    // Only attack if we're in range according to both our check and the blackboard
    if (!bIsInAttackRange || !bBlackboardInRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Not in attack range, distance: %.1f, attack range: %.1f"), DistanceToPlayer, Zombie->GetAttackRange());
        return EBTNodeResult::Failed;
    }

    UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Starting attack at distance %.1f"), DistanceToPlayer);

    // Face the player before attacking
    FVector Direction = Player->GetActorLocation() - Zombie->GetActorLocation();
    FRotator NewRotation = Direction.Rotation();
    NewRotation.Pitch = 0.0f;
    NewRotation.Roll = 0.0f;
    Zombie->SetActorRotation(NewRotation);

    // Initialize memory
    FBTAttackTaskMemory* MyMemory = (FBTAttackTaskMemory*)NodeMemory;
    MyMemory->RemainingCooldown = AttackCooldown;
    
    // Perform the attack
    Zombie->PerformAttack();

    return EBTNodeResult::InProgress;
}

void UBTTask_AttackPlayer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

    FBTAttackTaskMemory* MyMemory = (FBTAttackTaskMemory*)NodeMemory;
    MyMemory->RemainingCooldown -= DeltaSeconds;

    // Get the AI controller and blackboard
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    
    if (!AIController || !Blackboard)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Get the zombie character
    AZombieCharacter* Zombie = Cast<AZombieCharacter>(AIController->GetPawn());
    if (!Zombie)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Get the player from blackboard
    AActor* Player = Cast<AActor>(Blackboard->GetValueAsObject(AZombieAIController::PlayerKey));
    if (!Player)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Check if we're still in range during cooldown
    float DistanceToPlayer = FVector::Distance(Zombie->GetActorLocation(), Player->GetActorLocation());
    bool bIsInAttackRange = DistanceToPlayer <= Zombie->GetAttackRange();

    // If we're out of range, abort the attack
    if (!bIsInAttackRange || !Blackboard->GetValueAsBool(AZombieAIController::IsInAttackRangeKey))
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Target moved out of range during cooldown (Distance: %.1f)"), 
               DistanceToPlayer);
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Keep facing the player during cooldown
    FVector Direction = Player->GetActorLocation() - Zombie->GetActorLocation();
    FRotator NewRotation = Direction.Rotation();
    NewRotation.Pitch = 0.0f;
    NewRotation.Roll = 0.0f;
    Zombie->SetActorRotation(NewRotation);

    // REDUCED LOGGING: Only log every 2 seconds instead of every tick
    static float LastLogTime = 0.0f;
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastLogTime > 2.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("BTTask_AttackPlayer: Cooldown remaining: %.1fs, distance: %.1f"), MyMemory->RemainingCooldown, DistanceToPlayer);
        LastLogTime = CurrentTime;
    }

    // Check if cooldown is finished
    if (MyMemory->RemainingCooldown <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_AttackPlayer: Attack cooldown finished at distance %.1f"), 
               DistanceToPlayer);
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}
