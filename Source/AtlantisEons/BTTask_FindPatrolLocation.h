#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindPatrolLocation.generated.h"

UCLASS()
class ATLANTISEONS_API UBTTask_FindPatrolLocation : public UBTTask_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTTask_FindPatrolLocation();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
