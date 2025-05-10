// Copyright (c) 2025
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AtlantisCardinalDirector.generated.h"

UCLASS(Blueprintable)
class ATLANTISEONS_API AAtlantisCardinalDirector : public AActor
{
    GENERATED_BODY()

public:
    AAtlantisCardinalDirector();

protected:
    virtual void BeginPlay() override;

    // Damage Number System class to spawn
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Game Systems")
    TSubclassOf<AActor> DamageNumberSystemClass;

    // Screen Manager Tick Actor class to spawn
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Game Systems")
    TSubclassOf<AActor> ScreenManagerTickActorClass;

    // Spawned system references
    UPROPERTY(BlueprintReadOnly, Category="Game Systems")
    AActor* DamageNumberSystemInstance;

    UPROPERTY(BlueprintReadOnly, Category="Game Systems")
    AActor* ScreenManagerTickActorInstance;
};
