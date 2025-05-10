// Copyright Epic Games, Inc. All Rights Reserved.

#include "AtlantisEonsGameMode.h"
#include "AtlantisEonsCharacter.h"
#include "AtlantisEonsHUD.h"
#include "AtlantisEonsPlayerController.h"
#include "UObject/ConstructorHelpers.h"

AAtlantisEonsGameMode::AAtlantisEonsGameMode()
{
    // Set default classes
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/AtlantisEons/Blueprints/BP_Character"));
    if (PlayerPawnBPClass.Class != NULL)
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Found BP_Character class"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: Failed to find BP_Character class!"));
    }

    // Set HUD and PlayerController classes
    static ConstructorHelpers::FClassFinder<AHUD> HUDBPClass(TEXT("/Game/AtlantisEons/Blueprints/BP_AtlantisEonsHUD"));
    if (HUDBPClass.Class != NULL)
    {
        HUDClass = HUDBPClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Found BP_AtlantisEonsHUD class"));
    }
    else
    {
        HUDClass = AAtlantisEonsHUD::StaticClass();
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Using default HUD class"));
    }

    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/AtlantisEons/Blueprints/BP_AtlantisEonsPlayerController"));
    if (PlayerControllerBPClass.Class != NULL)
    {
        PlayerControllerClass = PlayerControllerBPClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Found BP_AtlantisEonsPlayerController class"));
    }
    else
    {
        PlayerControllerClass = AAtlantisEonsPlayerController::StaticClass();
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Using default PlayerController class"));
    }

    // Make sure we spawn a character when game starts
    bStartPlayersAsSpectators = false;
}

void AAtlantisEonsGameMode::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay"));
    
    // We don't need to manually spawn the player here
    // Unreal Engine handles the initial spawn automatically
}

void AAtlantisEonsGameMode::RestartPlayer(AController* NewPlayer)
{
    if (NewPlayer == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: RestartPlayer called with null controller"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("GameMode: Restarting player"));
    
    // Find a PlayerStart
    AActor* StartSpot = FindPlayerStart(NewPlayer);
    if (StartSpot == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: No PlayerStart found in level!"));
        return;
    }
    
    // Log default pawn class
    if (DefaultPawnClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Using DefaultPawnClass: %s"), *DefaultPawnClass->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: No DefaultPawnClass set!"));
        return;
    }

    // Spawn the pawn
    FRotator StartRotation = StartSpot->GetActorRotation();
    APawn* NewPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, StartSpot->GetActorLocation(), StartRotation);
    
    if (NewPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Successfully spawned pawn"));
        NewPlayer->Possess(NewPawn);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: Failed to spawn pawn!"));
    }
}

void AAtlantisEonsGameMode::RestartCurrentPlayer()
{
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        RestartPlayer(PC);
    }
}
