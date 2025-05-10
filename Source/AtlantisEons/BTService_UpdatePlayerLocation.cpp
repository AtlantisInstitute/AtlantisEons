#include "BTService_UpdatePlayerLocation.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "ZombieAIController.h"
#include "Kismet/GameplayStatics.h"

UBTService_UpdatePlayerLocation::UBTService_UpdatePlayerLocation()
{
    NodeName = TEXT("Update Player Location");
    Interval = 0.5f; // Update every half second
}

void UBTService_UpdatePlayerLocation::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    // Get AI controller and blackboard
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return;

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard) return;

    // Get player character
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn) return;

    // Update blackboard with player reference and location
    Blackboard->SetValueAsObject(AZombieAIController::PlayerKey, PlayerPawn);
    Blackboard->SetValueAsVector(AZombieAIController::TargetLocationKey, PlayerPawn->GetActorLocation());
}
