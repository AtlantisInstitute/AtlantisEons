#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AttackPlayer.generated.h"

// Memory struct for the attack task
struct FBTAttackTaskMemory
{
    float RemainingCooldown;

    FBTAttackTaskMemory()
        : RemainingCooldown(0.0f)
    {
    }
};

/**
 * Task that makes the zombie perform an attack on the player
 */
UCLASS()
class ATLANTISEONS_API UBTTask_AttackPlayer : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_AttackPlayer();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTAttackTaskMemory); }

protected:
    UPROPERTY(EditAnywhere, Category = "AI")
    float AttackCooldown;

    UPROPERTY(EditAnywhere, Category = "AI")
    float AttackRange;
};
