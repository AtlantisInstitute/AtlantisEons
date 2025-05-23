#include "AtlantisEonsPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

AAtlantisEonsPlayerController::AAtlantisEonsPlayerController()
{
    // Initialize with game input mode
    bShowMouseCursor = false;
    bEnableClickEvents = false;
    bEnableMouseOverEvents = false;
}

void AAtlantisEonsPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("PlayerController BeginPlay - Setting up proper input mode"));
    
    // Set game input mode directly
    FInputModeGameOnly GameOnlyInputMode;
    SetInputMode(GameOnlyInputMode);
    bShowMouseCursor = false;
    
    // Make sure this controller has input enabled
    EnableInput(this);
    
    // The pawn may not be spawned yet, but if it is, enable its input
    if (GetPawn())
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerController BeginPlay - Enabling input for pawn: %s"), *GetPawn()->GetName());
        GetPawn()->EnableInput(this);
        
        // Make sure the Enhanced Input system is properly set up if available
        if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                UE_LOG(LogTemp, Warning, TEXT("PlayerController BeginPlay - Enhanced Input Subsystem available"));
                
                // The actual mapping context will be added by the character
                // We just make sure the subsystem is ready
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerController BeginPlay - No pawn available yet"));
    }
}
