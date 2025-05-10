#include "AtlantisEonsHUD.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"

AAtlantisEonsHUD::AAtlantisEonsHUD()
    : bIsGameStarted(false)
    , bIsProcessingResume(false)
{
}

void AAtlantisEonsHUD::BeginPlay()
{
    Super::BeginPlay();
    
    // Only show menu at start if game hasn't started
    if (!bIsGameStarted)
    {
        ShowMainMenu();
    }
}

void AAtlantisEonsHUD::ShowMainMenu()
{
    // Create and show the main menu if widget class is set
    UE_LOG(LogTemp, Warning, TEXT("HUD - Checking MainMenuWidgetClass"));
    if (MainMenuWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - MainMenuWidgetClass is valid: %s"), *MainMenuWidgetClass->GetName());
        MainMenuWidget = CreateWidget<UUserWidget>(GetWorld(), MainMenuWidgetClass);
        if (MainMenuWidget)
        {
            UE_LOG(LogTemp, Warning, TEXT("HUD - Widget created successfully, setting up visibility"));
            MainMenuWidget->AddToViewport(100);
            SetGameInputMode(false);
            MainMenuWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    
    // Initialize inventory widget pointer
    InventoryWidget = nullptr;
}

void AAtlantisEonsHUD::ResumeGame()
{
    // Prevent multiple resume calls in the same frame
    if (bIsProcessingResume)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - ResumeGame already processing, skipping redundant call"));
        return;
    }
    
    bIsProcessingResume = true;
    UE_LOG(LogTemp, Warning, TEXT("HUD - ResumeGame called"));
    
    // First, remove the menu
    if (MainMenuWidget)
    {
        MainMenuWidget->RemoveFromParent();
        UE_LOG(LogTemp, Warning, TEXT("HUD - Removed main menu widget"));
    }
    
    // Also close the inventory if it's open
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->GetPawn())
    {
        if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PC->GetPawn()))
        {
            // Check if inventory is open and close it
            if (Character->IsInventoryOpen())
            {
                UE_LOG(LogTemp, Warning, TEXT("HUD - Found open inventory, closing it"));
                Character->CloseInventory();
            }
        }
    }

    // Get the GameMode and request player spawn if needed
    if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
    {
        FString GameModeName = GameMode->GetName();
        UE_LOG(LogTemp, Warning, TEXT("HUD - Got GameMode: %s"), *GameModeName);
        
        // Check if we have a player
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            if (!PC->GetPawn())
            {
                UE_LOG(LogTemp, Warning, TEXT("HUD - No pawn found, requesting restart"));
                GameMode->RestartPlayer(PC);
            }
            else
            {
                APawn* ExistingPawn = PC->GetPawn();
                if (ExistingPawn)
                {
                    UE_LOG(LogTemp, Warning, TEXT("HUD - Using existing pawn: %s"), *ExistingPawn->GetName());
                    // Make sure the pawn is properly possessed
                    if (ExistingPawn->GetController() != PC)
                    {
                        PC->Possess(ExistingPawn);
                    }
                    
                    // Ensure input mapping context is set up
                    if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(ExistingPawn))
                    {
                        if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PC->Player))
                        {
                            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                            {
                                UE_LOG(LogTemp, Warning, TEXT("HUD - Setting up input mapping context"));
                                
                                // First clear any existing mappings
                                Subsystem->ClearAllMappings();
                                
                                if (UInputMappingContext* MappingContext = Character->GetDefaultMappingContext())
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("HUD - Adding default mapping context"));
                                    Subsystem->AddMappingContext(MappingContext, 0);
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Error, TEXT("HUD - Failed to get default mapping context from character!"));
                                }
                            }
                            else
                            {
                                UE_LOG(LogTemp, Error, TEXT("HUD - Failed to get Enhanced Input Subsystem!"));
                            }
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("HUD - Failed to get Local Player!"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("HUD - Failed to cast pawn to AtlantisEonsCharacter!"));
                    }
                    
                    // Make sure the pawn is enabled for input
                    ExistingPawn->EnableInput(PC);
                }
            }
            
            // Set game input mode AFTER handling the pawn
            SetGameInputMode(true);
            
            // Mark game as started
            bIsGameStarted = true;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("HUD - Failed to get GameMode"));
    }
    
    // Reset processing flag with a slight delay to prevent multiple rapid calls
    GetWorld()->GetTimerManager().SetTimer(
        ResumeCooldownTimer, 
        [this](){ bIsProcessingResume = false; }, 
        0.2f, // 200ms cooldown should be enough to prevent double-clicks
        false
    );
}

void AAtlantisEonsHUD::ToggleMainMenu()
{
    if (IsMainMenuVisible())
    {
        ResumeGame();
    }
    else
    {
        ShowMainMenu();
    }
}

void AAtlantisEonsHUD::ResumeGameButtonHandler()
{
    UE_LOG(LogTemp, Warning, TEXT("HUD - Direct resume button handler called"));
    
    // CRITICAL: First lock the inventory toggle to prevent any input conflicts
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->GetPawn())
    {
        if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PC->GetPawn()))
        {
            // Lock inventory toggle for 1 second to prevent reopening via input
            Character->SetInventoryToggleLock(true, 1.0f);
            UE_LOG(LogTemp, Warning, TEXT("HUD - INVENTORY TOGGLE LOCKED to prevent input conflicts"));
            
            // Then, close the inventory if it's open
            if (Character->IsInventoryOpen())
            {
                UE_LOG(LogTemp, Warning, TEXT("HUD - Directly closing inventory"));
                Character->CloseInventory();
            }
        }
    }
    
    // If we had a main menu open, remove it
    if (MainMenuWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - Directly removing main menu widget"));
        MainMenuWidget->RemoveFromParent();
        MainMenuWidget = nullptr; // Clear the reference
    }
    
    // Set the input mode to game only, directly
    SetGameInputMode(true);
    
    // Mark the game as started
    bIsGameStarted = true;
    
    UE_LOG(LogTemp, Warning, TEXT("HUD - Direct resume button handler completed"));
}

void AAtlantisEonsHUD::UniversalResumeGame(TSubclassOf<UUserWidget> SourceWidgetClass)
{
    static bool bIsProcessingResume = false;
    
    // Only allow one click to be processed at a time
    if (bIsProcessingResume)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - Universal resume already processing, ignoring click"));
        return;
    }
    
    bIsProcessingResume = true;
    
    // Determine if we're resuming from the main menu or the inventory
    if (MainMenuWidgetClass && SourceWidgetClass == MainMenuWidgetClass)
    {
        // Coming from main menu - just call the main menu resume handler
        UE_LOG(LogTemp, Warning, TEXT("HUD - Universal resume from MAIN MENU"));
        ResumeGameButtonHandler();
    }
    else
    {
        // Coming from inventory or other widget - call the inventory handler
        UE_LOG(LogTemp, Warning, TEXT("HUD - Universal resume from INVENTORY"));
        ForceCloseInventoryAndResume();
    }
    
    bIsProcessingResume = false;
}

void AAtlantisEonsHUD::ForceCloseInventoryAndResume()
{
    UE_LOG(LogTemp, Warning, TEXT("====== INVENTORY RESUME: ForceCloseInventoryAndResume STARTED ======"));
    
    // Ensure we have a valid world
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("INVENTORY RESUME: No valid World!"));
        return;
    }
    
    // Get references
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("INVENTORY RESUME: No valid PlayerController!"));
        return;
    }
    
    if (!PC->GetPawn())
    {
        UE_LOG(LogTemp, Error, TEXT("INVENTORY RESUME: PlayerController has no Pawn!"));
        return;
    }
    
    // Find the character
    AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PC->GetPawn());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("INVENTORY RESUME: Pawn is not an AtlantisEonsCharacter!"));
        return;
    }
    
    // Log the current state
    UE_LOG(LogTemp, Warning, TEXT("INVENTORY RESUME: Current inventory state: %s"), 
           Character->IsInventoryOpen() ? TEXT("OPEN") : TEXT("CLOSED"));
    
    // STEP 1: Lock inventory toggle to prevent spam
    Character->SetInventoryToggleLock(true, 1.0f);
    UE_LOG(LogTemp, Warning, TEXT("INVENTORY RESUME: Inventory toggle locked for 1 second"));
    
    // STEP 2: Force close inventory
    UE_LOG(LogTemp, Warning, TEXT("INVENTORY RESUME: Beginning force close sequence"));
    
    // First approach: Force the state directly
    Character->ForceSetInventoryState(false);
    UE_LOG(LogTemp, Warning, TEXT("INVENTORY RESUME: Forced inventory state to CLOSED"));
    
    // Second approach: Call close function which will handle input mode and mapping context
    UE_LOG(LogTemp, Warning, TEXT("INVENTORY RESUME: Directly closing inventory"));
    Character->CloseInventory();
    
    // Final state check
    UE_LOG(LogTemp, Warning, TEXT("INVENTORY RESUME: Final inventory state: %s"), 
           Character->IsInventoryOpen() ? TEXT("STILL OPEN - ERROR") : TEXT("CLOSED - SUCCESS"));
           
    UE_LOG(LogTemp, Warning, TEXT("====== INVENTORY RESUME: ForceCloseInventoryAndResume COMPLETED ======"));
}

void AAtlantisEonsHUD::CompleteGameResume()
{
    UE_LOG(LogTemp, Warning, TEXT("HUD - COMPLETE GAME RESUME called"));
    
    // PHASE 1: Temporarily disable input to prevent multiple clicks
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->DisableInput(PC);
    }
    
    // PHASE 2: Lock the inventory toggle immediately to prevent any input conflicts
    if (PC)
    {
        APawn* PlayerPawn = PC->GetPawn();
        AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PlayerPawn);
        if (Character)
        {
            // Lock inventory toggle for 1 second to prevent any conflicts
            Character->SetInventoryToggleLock(true, 1.0f);
            UE_LOG(LogTemp, Warning, TEXT("HUD - COMPLETE RESUME: locked inventory toggle"));
        }
    }
    
    // PHASE 3: Check if inventory is open and close it first
    if (PC)
    {
        APawn* PlayerPawn = PC->GetPawn();
        AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PlayerPawn);
        if (Character)
        {
            if (Character->IsInventoryOpen())
            {
                UE_LOG(LogTemp, Warning, TEXT("HUD - COMPLETE RESUME: closing inventory"));
                Character->CloseInventory();
            }
        }
    }
    
    // PHASE 3: Remove the main menu immediately
    if (MainMenuWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - COMPLETE RESUME: removing main menu"));
        MainMenuWidget->RemoveFromParent();
        MainMenuWidget = nullptr; // Clear it completely
    }
    
    // PHASE 4: Set game input mode and re-enable input
    if (PC)
    {
        // Set game-only input mode
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(false);
        
        // Re-enable input
        PC->EnableInput(PC);
        
        UE_LOG(LogTemp, Warning, TEXT("HUD - COMPLETE RESUME: input mode reset to game only"));
    }
    
    // PHASE 5: Set game started flag
    bIsGameStarted = true;
    
    UE_LOG(LogTemp, Warning, TEXT("HUD - COMPLETE GAME RESUME completed"));
}

bool AAtlantisEonsHUD::IsMainMenuVisible() const
{
    return MainMenuWidget && MainMenuWidget->IsVisible();
}

void AAtlantisEonsHUD::SetGameInputMode(bool bGameMode)
{
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (bGameMode)
        {
            // Game mode - hide cursor and lock input to game
            FInputModeGameOnly InputMode;
            PC->SetShowMouseCursor(false); // Hide cursor first
            PC->SetInputMode(InputMode);
            
            // Force game view target
            if (APawn* Pawn = PC->GetPawn())
            {
                PC->SetViewTarget(Pawn);
            }
            
            UE_LOG(LogTemp, Warning, TEXT("HUD - Set input mode to Game"));
        }
        else
        {
            // UI mode - show cursor and allow UI interaction
            FInputModeGameAndUI InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockInFullscreen);
            InputMode.SetHideCursorDuringCapture(false);
            PC->SetShowMouseCursor(true); // Show cursor first
            PC->SetInputMode(InputMode);
            
            UE_LOG(LogTemp, Warning, TEXT("HUD - Set input mode to UI and enabled cursor"));
        }
        
        // Force a cursor update
        PC->UpdateCameraManager(0.0f);
    }
}

bool AAtlantisEonsHUD::ShowInventoryWidget()
{
    UE_LOG(LogTemp, Warning, TEXT("HUD - ShowInventoryWidget called"));
    
    // If widget is already visible, just return success
    if (InventoryWidget && InventoryWidget->IsValidLowLevel() && 
        InventoryWidget->IsVisible() && InventoryWidget->IsInViewport())
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - Inventory widget already visible"));
        return true;
    }
    
    // If we don't have a widget class, we can't create the widget
    if (!InventoryWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("HUD - No inventory widget class set!"));
        return false;
    }
    
    // Clean up any existing widget first
    if (InventoryWidget)
    {
        InventoryWidget->RemoveFromParent();
        InventoryWidget = nullptr;
    }
    
    // Create new widget first
    InventoryWidget = CreateWidget<UUserWidget>(GetWorld(), InventoryWidgetClass);
    if (!InventoryWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("HUD - Failed to create inventory widget!"));
        return false;
    }
    
    // Set input mode before adding to viewport
    SetGameInputMode(false);
    
    // Add to viewport and show
    InventoryWidget->AddToViewport(100);
    InventoryWidget->SetVisibility(ESlateVisibility::Visible);
    
    UE_LOG(LogTemp, Warning, TEXT("HUD - Inventory widget created and shown"));
    return true;
}

void AAtlantisEonsHUD::HideInventoryWidget()
{
    UE_LOG(LogTemp, Warning, TEXT("HUD - HideInventoryWidget called"));
    
    // Set input mode first to ensure game controls are restored
    SetGameInputMode(true);
    
    if (InventoryWidget && InventoryWidget->IsValidLowLevel())
    {
        // Hide first
        InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
        
        // Then remove from viewport
        InventoryWidget->RemoveFromParent();
        
        // Clear reference
        InventoryWidget = nullptr;
        
        UE_LOG(LogTemp, Warning, TEXT("HUD - Inventory widget hidden and removed"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - No valid inventory widget to hide"));
    }
}

bool AAtlantisEonsHUD::IsInventoryWidgetVisible() const
{
    return InventoryWidget && InventoryWidget->IsVisible();
}
