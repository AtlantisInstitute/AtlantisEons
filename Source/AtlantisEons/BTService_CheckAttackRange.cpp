#include "BTService_CheckAttackRange.h"
#include "AtlantisEonsCharacter.h"
#include "AIController.h"
#include "ZombieCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ZombieAIController.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

UBTService_CheckAttackRange::UBTService_CheckAttackRange()
{
    NodeName = TEXT("Check Attack Range");
    Interval = 0.5f;
    RandomDeviation = 0.1f;
    bShowDebugSphere = true;
}

void UBTService_CheckAttackRange::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);
    
    // Force an immediate range check when the service becomes relevant
    CheckRangeAndUpdateBlackboard(OwnerComp);
}

void UBTService_CheckAttackRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    CheckRangeAndUpdateBlackboard(OwnerComp);
}

void UBTService_CheckAttackRange::CheckRangeAndUpdateBlackboard(UBehaviorTreeComponent& OwnerComp)
{
    // Get the AI controller and blackboard
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    
    if (!AIController || !Blackboard)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTService_CheckAttackRange: Missing AIController or Blackboard"));
        return;
    }

    // Get the zombie character
    AZombieCharacter* Zombie = Cast<AZombieCharacter>(AIController->GetPawn());
    if (!Zombie)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTService_CheckAttackRange: Failed to get Zombie character"));
        return;
    }

    // Get the player from blackboard
    AActor* Player = Cast<AActor>(Blackboard->GetValueAsObject(AZombieAIController::PlayerKey));
    if (!Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTService_CheckAttackRange: No player found in blackboard"));
        Blackboard->SetValueAsBool(AZombieAIController::IsInAttackRangeKey, false);
        return;
    }

    // Calculate distance and check range with hysteresis
    float DistanceToPlayer = FVector::Distance(Zombie->GetActorLocation(), Player->GetActorLocation());
    bool bWasInRange = Blackboard->GetValueAsBool(AZombieAIController::IsInAttackRangeKey);
    
    // Use the zombie's actual attack range
    float EffectiveRange = Zombie->GetAttackRange();
    bool bIsInRange = DistanceToPlayer <= EffectiveRange;

    // Only update blackboard if the state has changed
    if (bIsInRange != bWasInRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTService_CheckAttackRange: Range state changed - Distance: %.1f, Range: %.1f, IsInRange: %s"), 
            DistanceToPlayer, EffectiveRange, *LexToString(bIsInRange));
        
        Blackboard->SetValueAsBool(AZombieAIController::IsInAttackRangeKey, bIsInRange);
    }

    // Debug visualization
    if (bShowDebugSphere)
    {
        FColor SphereColor = bIsInRange ? FColor::Green : FColor::Red;
        DrawDebugSphere(
            GetWorld(),
            Zombie->GetActorLocation(),
            EffectiveRange,
            12,
            SphereColor,
            false,
            0.5f
        );

        // Show line to player
        DrawDebugLine(
            GetWorld(),
            Zombie->GetActorLocation(),
            Player->GetActorLocation(),
            bIsInRange ? FColor::Green : FColor::Red,
            false,
            0.5f
        );

        // Show distance on screen
        FString DebugMessage = FString::Printf(TEXT("Distance: %.1f / %.1f"), DistanceToPlayer, EffectiveRange);
        GEngine->AddOnScreenDebugMessage(-1, 0.5f, bIsInRange ? FColor::Green : FColor::Red, DebugMessage);
    }
}
