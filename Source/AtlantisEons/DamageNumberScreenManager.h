#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.h"
#include "DamageNumberScreenManager.generated.h"

USTRUCT()
struct FScreenDamageNumberData
{
	GENERATED_BODY()

	UUserWidget* Widget;
	FVector WorldLocation;
	float Lifetime;
	float Elapsed;
};

UCLASS()
class ATLANTISEONS_API UDamageNumberScreenManager : public UObject
{
    GENERATED_BODY()
public:
    static UDamageNumberScreenManager* Get(UWorld* World);

    void ShowDamageNumber(UWorld* World, float DamageAmount, FVector WorldLocation, bool bIsPlayerDamage);
    void Tick(float DeltaTime);

    void SetDamageNumberWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

private:
    TArray<FScreenDamageNumberData> ActiveNumbers;
    TSubclassOf<UUserWidget> DamageNumberWidgetClass;
    bool bInitialized = false;
    FTimerHandle TickTimerHandle;

    void Initialize(UWorld* World);
};
