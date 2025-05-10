#include "DamageNumberScreenManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

UDamageNumberScreenManager* UDamageNumberScreenManager::Get(UWorld* World)
{
	static UDamageNumberScreenManager* Manager = nullptr;
	if (!Manager)
	{
		Manager = NewObject<UDamageNumberScreenManager>();
		Manager->AddToRoot();
	}
	return Manager;
}

void UDamageNumberScreenManager::Initialize(UWorld* World)
{
    if (bInitialized) return;

    // Load the widget class if not already set
    if (!DamageNumberWidgetClass)
    {
        // Try to load the widget blueprint class
        FString WidgetPath = TEXT("/Game/UI/Widgets/WBP_DamageNumber");
        UClass* LoadedClass = LoadClass<UUserWidget>(nullptr, *WidgetPath);
        if (LoadedClass)
        {
            DamageNumberWidgetClass = LoadedClass;
            UE_LOG(LogTemp, Warning, TEXT("Initialize: Successfully loaded widget class from %s"), *WidgetPath);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Initialize: Failed to load widget class from %s"), *WidgetPath);
        }
    }

    bInitialized = true;
}

void UDamageNumberScreenManager::SetDamageNumberWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
    DamageNumberWidgetClass = InWidgetClass;
}

void UDamageNumberScreenManager::ShowDamageNumber(UWorld* World, float DamageAmount, FVector WorldLocation, bool bIsPlayerDamage)
{
    Initialize(World);
    UE_LOG(LogTemp, Warning, TEXT("ShowDamageNumber: DamageNumberWidgetClass is %s"), *GetNameSafe(DamageNumberWidgetClass));
    if (!DamageNumberWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("ShowDamageNumber: Widget class is null!"));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("ShowDamageNumber: PlayerController is null!"));
        return;
    }

    UUserWidget* Widget = CreateWidget<UUserWidget>(PC, DamageNumberWidgetClass);
    if (!Widget)
    {
        UE_LOG(LogTemp, Error, TEXT("ShowDamageNumber: Failed to create widget!"));
        return;
    }

    // Convert world location to screen space
    FVector2D ScreenPosition;
    if (UGameplayStatics::ProjectWorldToScreen(PC, WorldLocation, ScreenPosition))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowDamageNumber: Projected to screen position: X=%.2f, Y=%.2f"), ScreenPosition.X, ScreenPosition.Y);
        Widget->AddToViewport(100); // High Z-order to ensure visibility

        if (UDamageNumberWidget* DamageWidget = Cast<UDamageNumberWidget>(Widget))
        {
            DamageWidget->InitializeDamageNumber(DamageAmount, bIsPlayerDamage);
            DamageWidget->SetPositionInViewport(ScreenPosition, true);
        }

        FScreenDamageNumberData Data;
        Data.Widget = Widget;
        Data.WorldLocation = WorldLocation;
        Data.Lifetime = 2.0f; // Increased lifetime
        Data.Elapsed = 0.0f;
        ActiveNumbers.Add(Data);

        // Register for tick if not already registered
        if (!World->GetTimerManager().IsTimerActive(TickTimerHandle))
        {
            FTimerDelegate TickDelegate;
            TickDelegate.BindUFunction(this, FName("Tick"));
            World->GetTimerManager().SetTimer(TickTimerHandle, TickDelegate, 0.016f, true);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ShowDamageNumber: Failed to project world location to screen!"));
        Widget->RemoveFromParent();
    }
}

void UDamageNumberScreenManager::Tick(float DeltaTime)
{
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(this);
    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    
    // Update and remove expired numbers
    for (int32 i = ActiveNumbers.Num() - 1; i >= 0; --i)
    {
        FScreenDamageNumberData& Data = ActiveNumbers[i];
        Data.Elapsed += DeltaTime;
        
        if (Data.Elapsed >= Data.Lifetime)
        {
            if (Data.Widget)
            {
                Data.Widget->RemoveFromParent();
            }
            ActiveNumbers.RemoveAt(i);
            continue;
        }
        
        if (PC && Data.Widget)
        {
            FVector2D ScreenPosition;
            if (UGameplayStatics::ProjectWorldToScreen(PC, Data.WorldLocation, ScreenPosition))
            {
                // Add some vertical movement for better visibility
                ScreenPosition.Y -= Data.Elapsed * 50.0f; // Move up by 50 units per second
                
                // Add slight random horizontal movement
                float HorizontalOffset = FMath::Sin(Data.Elapsed * 5.0f) * 10.0f;
                ScreenPosition.X += HorizontalOffset;
                
                Data.Widget->SetPositionInViewport(ScreenPosition, true);
                
                // Fade out near the end of lifetime
                if (UDamageNumberWidget* DamageWidget = Cast<UDamageNumberWidget>(Data.Widget))
                {
                    float Alpha = 1.0f - (Data.Elapsed / Data.Lifetime);
                    Alpha = FMath::Clamp(Alpha * 2.0f, 0.0f, 1.0f); // Fade out in the last half of lifetime
                    DamageWidget->SetRenderOpacity(Alpha);
                }
            }
        }
    }
    
    // Stop ticking if no active numbers
    if (ActiveNumbers.Num() == 0)
    {
        World->GetTimerManager().ClearTimer(TickTimerHandle);
    }
}
