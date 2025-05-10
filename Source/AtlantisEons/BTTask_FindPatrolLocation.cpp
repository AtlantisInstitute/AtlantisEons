#include "BTTask_FindPatrolLocation.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "ZombieAIController.h"

UBTTask_FindPatrolLocation::UBTTask_FindPatrolLocation()
{
    NodeName = TEXT("Find Patrol Location");
}

EBTNodeResult::Type UBTTask_FindPatrolLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Get AI controller and its pawn
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return EBTNodeResult::Failed;

    APawn* AIPawn = AIController->GetPawn();
    if (!AIPawn) return EBTNodeResult::Failed;

    // Get navigation system and find random point
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetNavigationSystem(GetWorld());
    if (!NavSys) return EBTNodeResult::Failed;

    // Get patrol radius from blackboard
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    float PatrolRadius = Blackboard->GetValueAsFloat(AZombieAIController::PatrolRadiusKey);

    // Get random point in navigable radius
    FNavLocation RandomLocation;
    bool bFound = NavSys->GetRandomReachablePointInRadius(
        AIPawn->GetActorLocation(),
        PatrolRadius,
        RandomLocation
    );

    if (!bFound) return EBTNodeResult::Failed;

    // Set the target location in the blackboard
    Blackboard->SetValueAsVector(AZombieAIController::TargetLocationKey, RandomLocation.Location);

    return EBTNodeResult::Succeeded;
}
