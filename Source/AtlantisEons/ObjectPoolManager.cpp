#include "ObjectPoolManager.h"
#include "DamageNumberActor.h"
#include "DamageNumberWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

// FDamageNumberPool Implementation
FDamageNumberPool::FDamageNumberPool(TSubclassOf<ADamageNumberActor> ActorClass, int32 InitialSize, int32 MaxSize)
    : TObjectPool<ADamageNumberActor>(InitialSize, MaxSize)
    , DamageNumberActorClass(ActorClass)
{
    UE_LOG(LogTemp, Log, TEXT("Damage Number Pool created - Initial: %d, Max: %d"), InitialSize, MaxSize);
}

ADamageNumberActor* FDamageNumberPool::CreateNewObject(UWorld* World)
{
    if (!World || !DamageNumberActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot create damage number actor - invalid world or class"));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    ADamageNumberActor* NewActor = World->SpawnActor<ADamageNumberActor>(DamageNumberActorClass, SpawnParams);
    if (NewActor)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Created new damage number actor from pool"));
    }
    
    return NewActor;
}

void FDamageNumberPool::DestroyObject(ADamageNumberActor* Object)
{
    if (Object && IsValid(Object))
    {
        Object->Destroy();
        UE_LOG(LogTemp, VeryVerbose, TEXT("Destroyed damage number actor"));
    }
}

void FDamageNumberPool::OnObjectRetrieved(ADamageNumberActor* Object)
{
    if (Object && IsValid(Object))
    {
        // Reset the actor to a clean state
        Object->SetActorHiddenInGame(false);
        Object->SetActorEnableCollision(false);
        Object->SetActorTickEnabled(true);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("Retrieved damage number actor from pool"));
    }
}

void FDamageNumberPool::OnObjectReturned(ADamageNumberActor* Object)
{
    if (Object && IsValid(Object))
    {
        // Clean up the actor for reuse
        Object->SetActorHiddenInGame(true);
        Object->SetActorLocation(FVector::ZeroVector);
        Object->SetActorRotation(FRotator::ZeroRotator);
        Object->SetActorTickEnabled(false);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("Returned damage number actor to pool"));
    }
}

// FDamageNumberWidgetPool Implementation
FDamageNumberWidgetPool::FDamageNumberWidgetPool(TSubclassOf<UDamageNumberWidget> WidgetClass, int32 InitialSize, int32 MaxSize)
    : TObjectPool<UDamageNumberWidget>(InitialSize, MaxSize)
    , DamageNumberWidgetClass(WidgetClass)
{
    UE_LOG(LogTemp, Log, TEXT("Damage Number Widget Pool created - Initial: %d, Max: %d"), InitialSize, MaxSize);
}

UDamageNumberWidget* FDamageNumberWidgetPool::CreateNewObject(UWorld* World)
{
    if (!World || !DamageNumberWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot create damage number widget - invalid world or class"));
        return nullptr;
    }

    UDamageNumberWidget* NewWidget = CreateWidget<UDamageNumberWidget>(World, DamageNumberWidgetClass);
    if (NewWidget)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Created new damage number widget from pool"));
    }
    
    return NewWidget;
}

void FDamageNumberWidgetPool::DestroyObject(UDamageNumberWidget* Object)
{
    if (Object && IsValid(Object))
    {
        Object->RemoveFromParent();
        Object->MarkAsGarbage();
        UE_LOG(LogTemp, VeryVerbose, TEXT("Destroyed damage number widget"));
    }
}

void FDamageNumberWidgetPool::OnObjectRetrieved(UDamageNumberWidget* Object)
{
    if (Object && IsValid(Object))
    {
        // Reset widget to clean state
        Object->SetVisibility(ESlateVisibility::Visible);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("Retrieved damage number widget from pool"));
    }
}

void FDamageNumberWidgetPool::OnObjectReturned(UDamageNumberWidget* Object)
{
    if (Object && IsValid(Object))
    {
        // Clean up widget for reuse
        Object->RemoveFromParent();
        Object->SetVisibility(ESlateVisibility::Hidden);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("Returned damage number widget to pool"));
    }
}

// UObjectPoolManager Implementation
void UObjectPoolManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogTemp, Log, TEXT("Object Pool Manager initialized"));
    
    // Set up periodic cleanup timer (every 30 seconds)
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            CleanupTimerHandle,
            this,
            &UObjectPoolManager::PerformPeriodicCleanup,
            30.0f,
            true
        );
    }
}

void UObjectPoolManager::Deinitialize()
{
    ClearAllPools();
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CleanupTimerHandle);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Object Pool Manager deinitialized"));
    
    Super::Deinitialize();
}

UObjectPoolManager* UObjectPoolManager::GetInstance(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }
    
    return World->GetSubsystem<UObjectPoolManager>();
}

void UObjectPoolManager::InitializeDamageNumberPool(TSubclassOf<ADamageNumberActor> ActorClass, int32 InitialSize, int32 MaxSize)
{
    if (!ActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot initialize damage number pool - invalid actor class"));
        return;
    }
    
    DamageNumberPool = MakeUnique<FDamageNumberPool>(ActorClass, InitialSize, MaxSize);
    UE_LOG(LogTemp, Log, TEXT("Damage number pool initialized"));
}

ADamageNumberActor* UObjectPoolManager::GetDamageNumberActor()
{
    if (!bEnablePooling)
    {
        return nullptr;
    }
    
    EnsureDamageNumberPoolExists();
    
    if (DamageNumberPool)
    {
        return DamageNumberPool->GetPooledObject(GetWorld());
    }
    
    return nullptr;
}

void UObjectPoolManager::ReturnDamageNumberActor(ADamageNumberActor* Actor)
{
    if (!bEnablePooling || !Actor)
    {
        return;
    }
    
    if (DamageNumberPool)
    {
        DamageNumberPool->ReturnToPool(Actor);
    }
}

void UObjectPoolManager::InitializeDamageNumberWidgetPool(TSubclassOf<UDamageNumberWidget> WidgetClass, int32 InitialSize, int32 MaxSize)
{
    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot initialize damage number widget pool - invalid widget class"));
        return;
    }
    
    DamageNumberWidgetPool = MakeUnique<FDamageNumberWidgetPool>(WidgetClass, InitialSize, MaxSize);
    UE_LOG(LogTemp, Log, TEXT("Damage number widget pool initialized"));
}

UDamageNumberWidget* UObjectPoolManager::GetDamageNumberWidget()
{
    if (!bEnablePooling)
    {
        return nullptr;
    }
    
    EnsureDamageNumberWidgetPoolExists();
    
    if (DamageNumberWidgetPool)
    {
        return DamageNumberWidgetPool->GetPooledObject(GetWorld());
    }
    
    return nullptr;
}

void UObjectPoolManager::ReturnDamageNumberWidget(UDamageNumberWidget* Widget)
{
    if (!bEnablePooling || !Widget)
    {
        return;
    }
    
    if (DamageNumberWidgetPool)
    {
        DamageNumberWidgetPool->ReturnToPool(Widget);
    }
}

FString UObjectPoolManager::GetPoolStatistics() const
{
    FString Stats = TEXT("Object Pool Statistics:\n");
    
    if (DamageNumberPool)
    {
        Stats += FString::Printf(TEXT("Damage Number Pool - Available: %d, Active: %d, Total: %d, Max: %d\n"),
            DamageNumberPool->GetAvailableCount(),
            DamageNumberPool->GetActiveCount(),
            DamageNumberPool->GetTotalCount(),
            DamageNumberPool->GetMaxPoolSize());
    }
    
    if (DamageNumberWidgetPool)
    {
        Stats += FString::Printf(TEXT("Damage Widget Pool - Available: %d, Active: %d, Total: %d, Max: %d\n"),
            DamageNumberWidgetPool->GetAvailableCount(),
            DamageNumberWidgetPool->GetActiveCount(),
            DamageNumberWidgetPool->GetTotalCount(),
            DamageNumberWidgetPool->GetMaxPoolSize());
    }
    
    return Stats;
}

void UObjectPoolManager::LogPoolStatistics() const
{
    if (bLogPoolStatistics)
    {
        UE_LOG(LogTemp, Log, TEXT("%s"), *GetPoolStatistics());
    }
}

void UObjectPoolManager::ClearAllPools()
{
    if (DamageNumberPool)
    {
        DamageNumberPool->ClearPool();
        DamageNumberPool.Reset();
    }
    
    if (DamageNumberWidgetPool)
    {
        DamageNumberWidgetPool->ClearPool();
        DamageNumberWidgetPool.Reset();
    }
    
    UE_LOG(LogTemp, Log, TEXT("All object pools cleared"));
}

void UObjectPoolManager::PerformPeriodicCleanup()
{
    // Log statistics if enabled
    LogPoolStatistics();
    
    // Could add logic here to trim pools that are too large
    // or perform other maintenance tasks
}

void UObjectPoolManager::EnsureDamageNumberPoolExists()
{
    if (!DamageNumberPool)
    {
        UE_LOG(LogTemp, Warning, TEXT("Damage number pool not initialized - creating default pool"));
        // You might want to have default classes configured somewhere
        // For now, we'll just log a warning
    }
}

void UObjectPoolManager::EnsureDamageNumberWidgetPoolExists()
{
    if (!DamageNumberWidgetPool)
    {
        UE_LOG(LogTemp, Warning, TEXT("Damage number widget pool not initialized - creating default pool"));
        // You might want to have default classes configured somewhere
        // For now, we'll just log a warning
    }
} 