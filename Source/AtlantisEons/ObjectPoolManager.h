#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectPoolManager.generated.h"

/**
 * Template class for managing pools of specific object types
 */
template<typename T>
class ATLANTISEONS_API TObjectPool
{
public:
    TObjectPool(int32 InitialSize = 10, int32 MaxSize = 100)
        : MaxPoolSize(MaxSize)
    {
        AvailableObjects.Reserve(InitialSize);
        ActiveObjects.Reserve(MaxSize);
    }

    virtual ~TObjectPool() = default;

    // Get an object from the pool (create if none available)
    T* GetPooledObject(UWorld* World)
    {
        T* Object = nullptr;
        
        if (AvailableObjects.Num() > 0)
        {
            Object = AvailableObjects.Pop();
        }
        else if (CanCreateNewObject())
        {
            Object = CreateNewObject(World);
        }

        if (Object && IsValid(Object))
        {
            ActiveObjects.Add(Object);
            OnObjectRetrieved(Object);
        }

        return Object;
    }

    // Return an object to the pool
    void ReturnToPool(T* Object)
    {
        if (!Object || !IsValid(Object))
        {
            return;
        }

        int32 RemovedCount = ActiveObjects.RemoveSingle(Object);
        if (RemovedCount > 0)
        {
            OnObjectReturned(Object);
            
            if (AvailableObjects.Num() < MaxPoolSize)
            {
                AvailableObjects.Add(Object);
            }
            else
            {
                // Pool is full, destroy the object
                DestroyObject(Object);
            }
        }
    }

    // Get pool statistics
    int32 GetAvailableCount() const { return AvailableObjects.Num(); }
    int32 GetActiveCount() const { return ActiveObjects.Num(); }
    int32 GetTotalCount() const { return AvailableObjects.Num() + ActiveObjects.Num(); }
    int32 GetMaxPoolSize() const { return MaxPoolSize; }

    // Clean up all objects in the pool
    void ClearPool()
    {
        for (T* Object : AvailableObjects)
        {
            if (Object && IsValid(Object))
            {
                DestroyObject(Object);
            }
        }
        AvailableObjects.Empty();

        for (T* Object : ActiveObjects)
        {
            if (Object && IsValid(Object))
            {
                DestroyObject(Object);
            }
        }
        ActiveObjects.Empty();
    }

protected:
    // Override these methods in derived classes
    virtual T* CreateNewObject(UWorld* World) = 0;
    virtual void DestroyObject(T* Object) = 0;
    virtual void OnObjectRetrieved(T* Object) {}
    virtual void OnObjectReturned(T* Object) {}

    bool CanCreateNewObject() const
    {
        return GetTotalCount() < MaxPoolSize;
    }

private:
    TArray<T*> AvailableObjects;
    TArray<T*> ActiveObjects;
    int32 MaxPoolSize;
};

// Forward declarations
class ADamageNumberActor;
class UDamageNumberWidget;

/**
 * Specialized pool for damage number actors
 */
class ATLANTISEONS_API FDamageNumberPool : public TObjectPool<ADamageNumberActor>
{
public:
    FDamageNumberPool(TSubclassOf<ADamageNumberActor> ActorClass, int32 InitialSize = 20, int32 MaxSize = 50);

protected:
    virtual ADamageNumberActor* CreateNewObject(UWorld* World) override;
    virtual void DestroyObject(ADamageNumberActor* Object) override;
    virtual void OnObjectRetrieved(ADamageNumberActor* Object) override;
    virtual void OnObjectReturned(ADamageNumberActor* Object) override;

private:
    TSubclassOf<ADamageNumberActor> DamageNumberActorClass;
};

/**
 * Specialized pool for damage number widgets
 */
class ATLANTISEONS_API FDamageNumberWidgetPool : public TObjectPool<UDamageNumberWidget>
{
public:
    FDamageNumberWidgetPool(TSubclassOf<UDamageNumberWidget> WidgetClass, int32 InitialSize = 20, int32 MaxSize = 50);

protected:
    virtual UDamageNumberWidget* CreateNewObject(UWorld* World) override;
    virtual void DestroyObject(UDamageNumberWidget* Object) override;
    virtual void OnObjectRetrieved(UDamageNumberWidget* Object) override;
    virtual void OnObjectReturned(UDamageNumberWidget* Object) override;

private:
    TSubclassOf<UDamageNumberWidget> DamageNumberWidgetClass;
};

/**
 * World subsystem for managing object pools
 * This provides a centralized way to manage object pools throughout the game
 */
UCLASS()
class ATLANTISEONS_API UObjectPoolManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Get the singleton instance
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    static UObjectPoolManager* GetInstance(UWorld* World);

    // Damage number pool management
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void InitializeDamageNumberPool(TSubclassOf<ADamageNumberActor> ActorClass, int32 InitialSize = 20, int32 MaxSize = 50);

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    ADamageNumberActor* GetDamageNumberActor();

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ReturnDamageNumberActor(ADamageNumberActor* Actor);

    // Widget pool management
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void InitializeDamageNumberWidgetPool(TSubclassOf<UDamageNumberWidget> WidgetClass, int32 InitialSize = 20, int32 MaxSize = 50);

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    UDamageNumberWidget* GetDamageNumberWidget();

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ReturnDamageNumberWidget(UDamageNumberWidget* Widget);

    // Pool statistics
    UFUNCTION(BlueprintPure, Category = "Object Pool")
    FString GetPoolStatistics() const;

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void LogPoolStatistics() const;

    // Pool management
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ClearAllPools();

    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Settings")
    bool bEnablePooling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Settings")
    bool bLogPoolStatistics = false;

protected:
    // Pool instances
    TUniquePtr<FDamageNumberPool> DamageNumberPool;
    TUniquePtr<FDamageNumberWidgetPool> DamageNumberWidgetPool;

    // Timer for periodic cleanup
    FTimerHandle CleanupTimerHandle;

    // Periodic cleanup function
    UFUNCTION()
    void PerformPeriodicCleanup();

private:
    // Internal helper to ensure pools are created
    void EnsureDamageNumberPoolExists();
    void EnsureDamageNumberWidgetPoolExists();
}; 