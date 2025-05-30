#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GenericTeamAgentInterface.h"
#include "DamageNumberSystem.h"
#include "Components/WidgetComponent.h"
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

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Reference to the player character
    UPROPERTY()
    class AAtlantisEonsCharacter* PlayerCharacter;

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

    // Health bar widget component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
    class UWidgetComponent* HealthBarWidget;

protected:
    virtual void BeginPlay() override;

    /** Combat properties */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
    class UAnimMontage* AttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
    float AttackDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
    float BaseDamage;

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

    /** Attack the player when in range */
    UFUNCTION()
    void AttackPlayer();

    /** Called when the zombie dies */
    UFUNCTION()
    void OnDeath();

    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    /** Play hit reaction montage */
    UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
    void PlayHitReactMontage();

    /** Called when hit reaction montage finishes blending out */
    UFUNCTION()
    void OnHitReactMontageBlendOut(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION(BlueprintCallable, Category = "Damage")
    void ShowDamageNumber(float DamageAmount, FVector Location, bool bIsCritical = false);

    /** Update the health bar widget display */
    UFUNCTION(BlueprintCallable, Category = "Zombie|Health")
    void UpdateHealthBar();

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
