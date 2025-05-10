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
    
    // Input mapping context is handled by the character
    // We don't need to set up any input here
}
