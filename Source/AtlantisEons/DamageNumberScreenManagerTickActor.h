#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageNumberScreenManagerTickActor.generated.h"

UCLASS()
class ATLANTISEONS_API ADamageNumberScreenManagerTickActor : public AActor
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
};
