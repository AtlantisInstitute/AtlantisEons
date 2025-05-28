#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageNumberWidget.h"
#include "DamageNumberActor.h"
#include "DamageNumberScreenManager.h"
#include "DamageNumberSystem.generated.h"

class UWidgetComponent;

/**
 * Singleton system for spawning and managing damage number widgets
 */
UCLASS()
class ATLANTISEONS_API ADamageNumberSystem : public AActor
{
    GENERATED_BODY()
    
public:    
    // Sets default values for this actor's properties
    ADamageNumberSystem();

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    
    // Called every frame
    virtual void Tick(float DeltaTime) override;
    
    // Function to spawn a damage number for a given actor
    void SpawnDamageNumber(AActor* DamagedActor, float DamageAmount, bool bIsPlayerDamage);
    
    // Function to spawn a damage number at a specific world location
    void SpawnDamageNumberAtLocation(AActor* DamagedActor, FVector Location, float DamageAmount, bool bIsCritical);
    
    // Calculates the best location to spawn a damage number for the given actor
    FVector CalculateDamageNumberLocation(AActor* DamagedActor);
    
    // Get the singleton instance of the damage number system
    UFUNCTION(BlueprintCallable, Category = "Damage")
    static ADamageNumberSystem* GetInstance(UWorld* World);
    
    // Set the widget class to use for damage numbers
    UFUNCTION(BlueprintCallable, Category = "Damage")
    void SetDamageNumberWidgetClass(TSubclassOf<UDamageNumberWidget> WidgetClass);
    
    // Get the widget class
    UFUNCTION(BlueprintCallable, Category = "Damage")
    TSubclassOf<UDamageNumberWidget> GetDamageNumberWidgetClass() const { return DamageNumberWidgetClass; }
    
    // Initialize the widget with damage amount and color
    void InitWidget();
    
    // Show damage at a specific location
    UFUNCTION(BlueprintCallable, Category = "Damage", meta = (DisplayName = "Show Damage"))
    void ShowDamage(float DamageAmount, FVector Location, bool bIsCritical = false);
    
    // Static instance of the damage number system
    static ADamageNumberSystem* Instance;
    
protected:
    // The widget class to spawn for damage numbers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Numbers")
    TSubclassOf<class UDamageNumberWidget> DamageNumberWidgetClass;
    
    // The color to use for player damage numbers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    FLinearColor PlayerDamageColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f); // Green by default
    
    // The color to use for enemy damage numbers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    FLinearColor EnemyDamageColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f); // Red by default
    
    // How long damage numbers should stay on screen
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float DamageNumberLifetime = 2.0f;
    
    // Offset for spawning damage numbers
    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    FVector DamageNumberOffset = FVector(0.0f, 0.0f, 100.0f);
    
    // Random horizontal offset range for damage numbers
    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float RandomOffsetRange = 50.0f;
    
    // Widget vertical movement speed
    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float WidgetRiseSpeed = 100.0f;
    
    // The amount of damage to display
    float DamageAmount;
    
    // The actor that was damaged
    UPROPERTY()
    AActor* DamagedActor;
    
    // Whether the damage is being applied to the player (true) or an enemy (false)
    bool bIsPlayerDamage;
    
    // The widget component to display the damage number
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UWidgetComponent* WidgetComponent;
    
    // Clean up expired damage number actors
    void CleanupExpiredWidgets();
    
    // Destroys this damage number after animation completes
    UFUNCTION()
    void DestroyDamageNumber();
    
    // Array of active damage number actors
    UPROPERTY()
    TArray<AActor*> ActiveDamageNumberActors;
};
