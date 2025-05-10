// Copyright (c) 2025
#include "AtlantisCardinalDirector.h"
#include "Engine/World.h"

AAtlantisCardinalDirector::AAtlantisCardinalDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    DamageNumberSystemClass = nullptr;
    ScreenManagerTickActorClass = nullptr;
    DamageNumberSystemInstance = nullptr;
    ScreenManagerTickActorInstance = nullptr;
}

void AAtlantisCardinalDirector::BeginPlay()
{
    Super::BeginPlay();
    UWorld* World = GetWorld();
    if (!World) return;

    // Spawn Damage Number System
    if (DamageNumberSystemClass)
    {
        DamageNumberSystemInstance = World->SpawnActor<AActor>(DamageNumberSystemClass, GetActorLocation(), FRotator::ZeroRotator);
    }

    // Spawn Screen Manager Tick Actor
    if (ScreenManagerTickActorClass)
    {
        ScreenManagerTickActorInstance = World->SpawnActor<AActor>(ScreenManagerTickActorClass, GetActorLocation(), FRotator::ZeroRotator);
    }
}
