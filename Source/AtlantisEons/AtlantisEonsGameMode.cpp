// Copyright Epic Games, Inc. All Rights Reserved.

#include "AtlantisEonsGameMode.h"
#include "AtlantisEonsCharacter.h"
#include "AtlantisEonsHUD.h"
#include "AtlantisEonsPlayerController.h"
#include "UObject/ConstructorHelpers.h"

AAtlantisEonsGameMode::AAtlantisEonsGameMode()
{
    // Load the BP_Character blueprint class instead of using C++ class directly
    // This ensures skeletal mesh, input actions, and other Blueprint settings are properly configured
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/AtlantisEons/Blueprints/BP_Character"));
    if (PlayerPawnBPClass.Class != nullptr)
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("GameMode Constructor: Set DefaultPawnClass to Blueprint BP_Character"));
    }
    else
    {
        // Fallback to C++ class if Blueprint not found
        DefaultPawnClass = AAtlantisEonsCharacter::StaticClass();
        UE_LOG(LogTemp, Warning, TEXT("GameMode Constructor: Blueprint not found, falling back to C++ AAtlantisEonsCharacter"));
    }
    
    // Load the BP_AtlantisEonsHUD blueprint class
    static ConstructorHelpers::FClassFinder<AHUD> HUDBPClass(TEXT("/Game/AtlantisEons/Blueprints/BP_AtlantisEonsHUD"));
    if (HUDBPClass.Class != nullptr)
    {
        HUDClass = HUDBPClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("GameMode Constructor: Set HUDClass to Blueprint BP_AtlantisEonsHUD"));
    }
    else
    {
        // Fallback to C++ class if Blueprint not found
        HUDClass = AAtlantisEonsHUD::StaticClass();
        UE_LOG(LogTemp, Warning, TEXT("GameMode Constructor: HUD Blueprint not found, falling back to C++ AAtlantisEonsHUD"));
    }
    
    // Load the BP_AtlantisEonsPlayerController blueprint class
    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/AtlantisEons/Blueprints/BP_AtlantisEonsPlayerController"));
    if (PlayerControllerBPClass.Class != nullptr)
    {
        PlayerControllerClass = PlayerControllerBPClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("GameMode Constructor: Set PlayerControllerClass to Blueprint BP_AtlantisEonsPlayerController"));
    }
    else
    {
        // Fallback to C++ class if Blueprint not found
        PlayerControllerClass = AAtlantisEonsPlayerController::StaticClass();
        UE_LOG(LogTemp, Warning, TEXT("GameMode Constructor: PlayerController Blueprint not found, falling back to C++ AAtlantisEonsPlayerController"));
    }

    // Make sure we spawn a character when game starts
    bStartPlayersAsSpectators = false;

    bUseSeamlessTravel = true;
}

void AAtlantisEonsGameMode::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay - Checking player spawn"));
    
    // Verify our classes are set
    UE_LOG(LogTemp, Warning, TEXT("GameMode: DefaultPawnClass = %s"), 
           DefaultPawnClass ? *DefaultPawnClass->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("GameMode: HUDClass = %s"), 
           HUDClass ? *HUDClass->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("GameMode: PlayerControllerClass = %s"), 
           PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("NULL"));
    
    // Check if we have a player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode BeginPlay: NO PLAYER CONTROLLER FOUND!"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay: Found PlayerController: %s"), *PC->GetName());
    
    // Check if player has a pawn
    if (!PC->GetPawn())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay: No pawn found, spawning one now"));
        RestartPlayer(PC);
        
        // Verify spawn worked
        if (PC->GetPawn())
        {
            UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay: Successfully spawned pawn: %s"), *PC->GetPawn()->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("GameMode BeginPlay: FAILED to spawn pawn!"));
        }
    }
    else
    {
        APawn* ExistingPawn = PC->GetPawn();
        UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay: Player already has pawn: %s"), *ExistingPawn->GetName());
        
        // Make sure the pawn is properly possessed
        if (ExistingPawn->GetController() != PC)
        {
            UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay: Re-possessing pawn"));
            PC->Possess(ExistingPawn);
        }
        
        // Enable input
        PC->EnableInput(PC);
        ExistingPawn->EnableInput(PC);
        
        // Set input mode
        FInputModeGameOnly GameOnlyMode;
        PC->SetInputMode(GameOnlyMode);
        PC->SetShowMouseCursor(false);
        
        UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay: Player setup complete"));
    }
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
