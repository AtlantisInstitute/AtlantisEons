#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "GenericTeamAgentInterface.h"
#include "DamageNumberSystem.h"
#include "ZombieCharacter.generated.h"

// Forward declarations
class ADamageNumberSystem;

UCLASS()
class ATLANTISEONS_API AZombieCharacter : public ACharacter, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    AZombieCharacter();

    // IGenericTeamAgentInterface
    virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
    virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override { TeamId = InTeamId; }

    /** Combat Functions */
    UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
    void PerformAttack();

    /** Animation Event */
    UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
    void OnAttackHit();

    UFUNCTION(BlueprintCallable, Category = "AI")
    float GetAttackRange() const { return AttackRange; }

    UFUNCTION(BlueprintCallable, Category = "AI")
    float GetPatrolRadius() const { return PatrolRadius; }

    UFUNCTION(BlueprintCallable, Category = "AI")
    UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

    UFUNCTION(BlueprintCallable, Category = "AI")
    UBlackboardData* GetBlackboardData() const { return BlackboardData; }

    // Health getters
    UFUNCTION(BlueprintPure, Category = "Health")
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetMaxHealth() const { return MaxHealth; }


    UPROPERTY(EditDefaultsOnly, Category = "AI")
    class UBehaviorTree* BehaviorTree;

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    class UBlackboardData* BlackboardData;

protected:
    virtual void BeginPlay() override;

    /** Combat properties */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
    class UAnimMontage* AttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
    float AttackDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
    float AttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|AI")
    float PatrolRadius;

    // Health properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Health", meta = (AllowPrivateAccess = "true"))
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Health", meta = (AllowPrivateAccess = "true"))
    float MaxHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
    UAnimMontage* DeathMontage;

    /** Animation montage for hit reaction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
    UAnimMontage* HitReactMontage;


    /** Apply damage to the zombie */
    UFUNCTION(BlueprintCallable, Category = "Zombie|Health")
    void ApplyDamage(float DamageAmount);

    /** Handle zombie death */
    UFUNCTION()
    void HandleDeath();

    UFUNCTION(BlueprintNativeEvent, Category = "Zombie|Combat")
    float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

    /** Play hit reaction montage */
    UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
    void PlayHitReactMontage();

private:
    FGenericTeamId TeamId;
    bool bIsAttacking;
    bool bIsDead;
    
    // Damage tracking for preventing duplicate damage numbers
    uint64 LastDamageFrame;
    AActor* LastDamageCauser;
    float LastDamageAmount;
    FVector LastDamageLocation;
};
