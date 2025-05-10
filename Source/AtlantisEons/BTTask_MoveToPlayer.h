#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_MoveToPlayer.generated.h"

struct FBTMoveToTaskMemory : public FBTTaskMemory
{
    FDelegateHandle BBObserverDelegateHandle;
    FVector PreviousGoalLocation;
    
    FBTMoveToTaskMemory()
        : PreviousGoalLocation(FVector::ZeroVector)
    {
    }
};

/**
 * Task that moves the zombie towards the player
 */
UCLASS()
class ATLANTISEONS_API UBTTask_MoveToPlayer : public UBTTask_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTTask_MoveToPlayer();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
    UPROPERTY(EditAnywhere, Category = "Movement")
    float AcceptableRadius;

    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTMoveToTaskMemory); }
};
