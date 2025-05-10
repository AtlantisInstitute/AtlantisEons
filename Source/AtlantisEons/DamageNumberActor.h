#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/WidgetComponent.h"
#include "DamageNumberWidget.h"
#include "DamageNumberActor.generated.h"

UCLASS()
class ATLANTISEONS_API ADamageNumberActor : public AActor
{
    GENERATED_BODY()
public:
    ADamageNumberActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UWidgetComponent* WidgetComponent;

    void Init(float DamageAmount, TSubclassOf<UUserWidget> DamageWidgetClass, bool bIsPlayerDamage);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    float Lifetime;
    float Elapsed;
};
