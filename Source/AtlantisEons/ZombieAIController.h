#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AIPerceptionTypes.h"
#include "GenericTeamAgentInterface.h"
#include "ZombieAIController.generated.h"

UCLASS()
class ATLANTISEONS_API AZombieAIController : public AAIController
{
    GENERATED_BODY()

public:
    AZombieAIController();

    // Blackboard key names
    static const FName PlayerKey;
    static const FName TargetLocationKey;
    static const FName IsInAttackRangeKey;
    static const FName PatrolRadiusKey;

    FORCEINLINE UBlackboardComponent* GetBlackboard() const { return BlackboardComponent; }

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBehaviorTree* BehaviorTree;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
    class UBlackboardData* BlackboardData;

    UPROPERTY()
    UBlackboardComponent* BlackboardComponent;

    UPROPERTY()
    UBehaviorTreeComponent* BehaviorTreeComponent;

    // AI Perception
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    class UAIPerceptionComponent* AIPerceptionComponent;

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UAISenseConfig_Sight* SightConfig;

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override
    {
        if (const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(&Other))
        {
            FGenericTeamId OtherTeamID = OtherTeamAgent->GetGenericTeamId();
            FGenericTeamId MyTeamID = GetGenericTeamId();

            // If the other actor is on a different team, consider them hostile
            if (OtherTeamID != MyTeamID)
            {
                return ETeamAttitude::Hostile;
            }
        }
        return ETeamAttitude::Neutral;
    }

private:
    void UpdateDebugVisualization();
};
