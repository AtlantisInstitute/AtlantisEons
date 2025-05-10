#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CheckAttackRange.generated.h"

UCLASS()
class ATLANTISEONS_API UBTService_CheckAttackRange : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_CheckAttackRange();

    virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
    UPROPERTY(EditAnywhere, Category = "AI")
    float AttackRange;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bShowDebugSphere;

private:
    void CheckRangeAndUpdateBlackboard(UBehaviorTreeComponent& OwnerComp);
};
