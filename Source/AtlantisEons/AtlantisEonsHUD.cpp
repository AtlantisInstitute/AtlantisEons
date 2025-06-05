#include "AtlantisEonsHUD.h"
#include "AtlantisEonsCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"
#include "WBP_Main.h"
#include "WBP_CharacterInfo.h"

AAtlantisEonsHUD::AAtlantisEonsHUD()
    : bIsGameStarted(false)
    , bIsProcessingResume(false)
{
}

void AAtlantisEonsHUD::ShowCharacterInfo()
{
    UE_LOG(LogTemp, Warning, TEXT("AAtlantisEonsHUD::ShowCharacterInfo() called"));
    
    // First check if we have a main widget with a widget switcher that might be handling this
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("ShowCharacterInfo: No valid world"));
        return;
    }
    
    // Check for existing WBP_Main widget in the viewport
    for (TObjectIterator<UWBP_Main> Itr; Itr; ++Itr)
    {
        UWBP_Main* MainWidget = *Itr;
        if (MainWidget && MainWidget->IsInViewport())
        {
            UE_LOG(LogTemp, Warning, TEXT("ShowCharacterInfo: Found WBP_Main in viewport, switching to CharacterInfo tab"));
            // Call the switcher function to show character info
            MainWidget->Switcher1();
            return;
        }
    }
    
    // If we didn't find a main widget with switcher, proceed with the standalone approach
    
    // Check if the widget already exists
    if (CharacterInfoWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowCharacterInfo: Using existing CharacterInfoWidget"));
        CharacterInfoWidget->SetVisibility(ESlateVisibility::Visible);
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            PC->SetInputMode(FInputModeGameAndUI());
            PC->SetShowMouseCursor(true);
        }
        return;
    }

    // Create and show the character info if widget class is set
    if (CharacterInfoWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowCharacterInfo: Creating new CharacterInfoWidget"));
        CharacterInfoWidget = CreateWidget<UUserWidget>(GetWorld(), CharacterInfoWidgetClass);
        if (CharacterInfoWidget)
        {
            CharacterInfoWidget->AddToViewport(100);
            if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
            {
                PC->SetInputMode(FInputModeGameAndUI());
                PC->SetShowMouseCursor(true);
            }
            
            // If this is a WBP_CharacterInfo, initialize it
            UWBP_CharacterInfo* CharacterInfoWBP = Cast<UWBP_CharacterInfo>(CharacterInfoWidget);
            if (CharacterInfoWBP)
            {
                UE_LOG(LogTemp, Warning, TEXT("ShowCharacterInfo: Initializing WBP_CharacterInfo"));
                // Try to set the character reference
                if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
                {
                    if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PC->GetPawn()))
                    {
                        CharacterInfoWBP->Character = Character;
                        CharacterInfoWBP->UpdateAllStats();
                    }
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ShowCharacterInfo: CharacterInfoWidgetClass is not set!"));
    }
}

void AAtlantisEonsHUD::HideCharacterInfo()
{
    // First check if we have a main widget with a widget switcher
    for (TObjectIterator<UWBP_Main> Itr; Itr; ++Itr)
    {
        UWBP_Main* MainWidget = *Itr;
        if (MainWidget && MainWidget->IsInViewport())
        {
            // If we're currently showing character info (index 0), then switch to something else or hide
            if (MainWidget->WidgetSwitcher && MainWidget->WidgetSwitcher->GetActiveWidgetIndex() == 0)
            {
                // Just set to hidden for now - the game will handle input modes
                if (MainWidget->WBP_CharacterInfo)
                {
                    MainWidget->WBP_CharacterInfo->SetVisibility(ESlateVisibility::Hidden);
                }
            }
            return;
        }
    }

    // Standalone approach if no main widget
    if (CharacterInfoWidget)
    {
        CharacterInfoWidget->RemoveFromParent();
        CharacterInfoWidget = nullptr;
    }

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->SetShowMouseCursor(false);
    }
}

bool AAtlantisEonsHUD::IsCharacterInfoVisible() const
{
    // Check if it's visible in the main widget
    for (TObjectIterator<UWBP_Main> Itr; Itr; ++Itr)
    {
        UWBP_Main* MainWidget = *Itr;
        if (MainWidget && MainWidget->IsInViewport())
        {
            if (MainWidget->WidgetSwitcher && MainWidget->WidgetSwitcher->GetActiveWidgetIndex() == 0 &&
                MainWidget->WBP_CharacterInfo && 
                MainWidget->WBP_CharacterInfo->GetVisibility() == ESlateVisibility::Visible)
            {
                return true;
            }
            return false;
        }
    }
    
    // Standalone approach
    return CharacterInfoWidget && CharacterInfoWidget->IsVisible();
}

void AAtlantisEonsHUD::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("HUD - BeginPlay: Starting without showing main menu"));
    
    // Don't show menu at start, focus on getting gameplay working first
    // if (!bIsGameStarted)
    // {
    //    ShowMainMenu();
    // }
    
    // Make sure game is properly started
    bIsGameStarted = true;
    
    // Ensure player controller is set to game input mode
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        FInputModeGameOnly GameOnlyMode;
        PC->SetInputMode(GameOnlyMode);
        PC->SetShowMouseCursor(false);
        
        // Log PC and pawn status for debugging
        if (APawn* Pawn = PC->GetPawn())
        {
            UE_LOG(LogTemp, Warning, TEXT("HUD - BeginPlay: Player has Pawn: %s"), *Pawn->GetName());
            
            // If we have a character, make sure input is set up
            if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(Pawn))
            {
                Character->ResetCharacterInput();
                UE_LOG(LogTemp, Warning, TEXT("HUD - BeginPlay: Reset character input"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("HUD - BeginPlay: Player has no Pawn, requesting spawn"));
            if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
            {
                GameMode->RestartPlayer(PC);
            }
        }
    }
}

void AAtlantisEonsHUD::ShowMainMenu()
{
    // First check if there are existing widgets with the same class in the viewport
    TArray<UUserWidget*> FoundWidgets;
    if (MainMenuWidgetClass)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            for (TObjectIterator<UUserWidget> Itr; Itr; ++Itr)
            {
                UUserWidget* FoundWidget = *Itr;
                if (FoundWidget && FoundWidget->IsA(MainMenuWidgetClass) && FoundWidget->IsInViewport())
                {
                    UE_LOG(LogTemp, Warning, TEXT("HUD - Found existing MainMenu widget in viewport: %s"), *FoundWidget->GetName());
                    // Use the existing widget
                    MainMenuWidget = FoundWidget;
                    MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
                    SetGameInputMode(false);
                    return;
                }
            }
        }
    }
    
    // Check if the widget already exists
    if (MainMenuWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - MainMenuWidget already exists, showing it"));
        MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
        SetGameInputMode(false);
        return;
    }

    // Create and show the main menu if widget class is set
    UE_LOG(LogTemp, Warning, TEXT("HUD - Checking MainMenuWidgetClass"));
    if (MainMenuWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - MainMenuWidgetClass is valid: %s"), *MainMenuWidgetClass->GetName());
        // Check if we're in editor to handle PIE case
        UWorld* World = GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Error, TEXT("HUD - Could not get world"));
            return;
        }
        
        // Create the widget
        MainMenuWidget = CreateWidget<UUserWidget>(World, MainMenuWidgetClass);
        if (MainMenuWidget)
        {
            UE_LOG(LogTemp, Warning, TEXT("HUD - Widget created successfully, setting up visibility"));
            MainMenuWidget->AddToViewport(100);
            SetGameInputMode(false);
            MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
        }
    }
    
    // Initialize inventory widget pointer
    InventoryWidget = nullptr;
}

void AAtlantisEonsHUD::ResumeGame()
{
    HideCharacterInfo();

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
                Character->CloseInventoryImpl();
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
                Character->CloseInventoryImpl();
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
    
    // Use CompleteGameResume directly for more reliable input handling
    UE_LOG(LogTemp, Warning, TEXT("HUD - Universal resume using enhanced CompleteGameResume"));
    CompleteGameResume();
    
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
    Character->CloseInventoryImpl();
    
    // Final state check
    UE_LOG(LogTemp, Warning, TEXT("INVENTORY RESUME: Final inventory state: %s"), 
           Character->IsInventoryOpen() ? TEXT("STILL OPEN - ERROR") : TEXT("CLOSED - SUCCESS"));
           
    UE_LOG(LogTemp, Warning, TEXT("====== INVENTORY RESUME: ForceCloseInventoryAndResume COMPLETED ======"));
}

void AAtlantisEonsHUD::CompleteGameResume()
{
    UE_LOG(LogTemp, Warning, TEXT("HUD - COMPLETE GAME RESUME called"));
    
    // Get the world
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("HUD - CompleteGameResume: No valid World!"));
        return;
    }
    
    // PHASE 1: Get the player controller and character
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("HUD - CompleteGameResume: No valid PlayerController!"));
        return;
    }
    
    // Temporarily disable input to prevent multiple clicks
    PC->DisableInput(PC);
    
    // Find the character
    APawn* PlayerPawn = PC->GetPawn();
    AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PlayerPawn);
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("HUD - CompleteGameResume: No valid Character!"));
        PC->EnableInput(PC); // Re-enable input before returning
        return;
    }
    
    // PHASE 2: Clear all UI elements
    
    // Lock the inventory toggle to prevent any input conflicts
    Character->SetInventoryToggleLock(true, 1.0f);
    UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Locked inventory toggle"));
    
    // Close the inventory if it's open
    if (Character->IsInventoryOpen())
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Closing inventory"));
        Character->CloseInventoryImpl();
    }
    
    // Hide character info if visible
    if (IsCharacterInfoVisible())
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Hiding character info"));
        HideCharacterInfo();
    }
    
    // Remove all UI widgets that might have focus
    if (MainMenuWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Removing main menu"));
        MainMenuWidget->RemoveFromParent();
        MainMenuWidget = nullptr;
    }
    
    if (InventoryWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Removing inventory widget"));
        InventoryWidget->RemoveFromParent();
        InventoryWidget = nullptr;
    }
    
    // PHASE 3: Force clear any UI focus
    if (GEngine && GEngine->GameViewport)
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
        UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Cleared UI focus"));
    }
    
    // PHASE 4: Reset input mode to game only
    FInputModeGameOnly GameOnlyMode;
    PC->SetShowMouseCursor(false);
    PC->SetInputMode(GameOnlyMode);
    PC->FlushPressedKeys();
    UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Reset input mode to game only"));
    
    // PHASE 5: Reset Enhanced Input for the character
    if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PC->Player))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            // Clear all mappings first
            Subsystem->ClearAllMappings();
            
            // Add the default mapping context
            if (Character->GetDefaultMappingContext())
            {
                Subsystem->AddMappingContext(Character->GetDefaultMappingContext(), 0);
                UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Reset enhanced input mapping context"));
            }
        }
    }
    
    // PHASE 6: Reset character input and movement
    
    // Use character's dedicated input reset function
    Character->ResetCharacterInput();
    UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Called Character->ResetCharacterInput()"));
    
    // Force enable movement
    Character->ForceEnableMovement();
    UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Called Character->ForceEnableMovement()"));
    
    // PHASE 7: Re-enable input with a short delay to ensure everything is set up
    FTimerHandle EnableInputTimerHandle;
    World->GetTimerManager().SetTimer(
        EnableInputTimerHandle,
        [PC, Character]() {
            if (PC && Character)
            {
                // Re-enable input
                PC->EnableInput(PC);
                Character->EnableInput(PC);
                
                // Simulate movement to wake up the input system
                Character->AddMovementInput(FVector(1, 0, 0), 0.01f);
                Character->AddMovementInput(FVector(-1, 0, 0), 0.01f);
                
                UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Re-enabled input and simulated movement"));
            }
        },
        0.1f,
        false
    );
    
    // PHASE 8: Set game started flag
    bIsGameStarted = true;
    
    UE_LOG(LogTemp, Warning, TEXT("HUD - CompleteGameResume: Process completed successfully"));
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
    
    // If the widget already exists and is visible, do nothing
    if (InventoryWidget && InventoryWidget->IsVisible())
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - Inventory widget already visible"));
        return true;
    }
    
    // Create the main widget if it doesn't exist
    if (!MainWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - Creating Main widget"));
        
        // First check if we have a MainWidgetClass set directly
        TSubclassOf<UWBP_Main> MainWidgetClassToUse = MainWidgetClass;
        
        // If not set in Blueprint, try to find it dynamically
        if (!MainWidgetClassToUse)
        {
            UE_LOG(LogTemp, Warning, TEXT("HUD - MainWidgetClass not set, trying to find it dynamically"));
            
            // Try multiple possible paths for the WBP_Main class
            TSubclassOf<UWBP_Main> FoundWidgetClass = nullptr;
            
            // Path options to try in order
            TArray<FString> PossiblePaths;
            PossiblePaths.Add(TEXT("/Game/AtlantisEons/UI/WBP_Main.WBP_Main_C"));
            PossiblePaths.Add(TEXT("/Game/AtlantisEons/Blueprints/UI/WBP_Main.WBP_Main_C"));
            PossiblePaths.Add(TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/WBP_Main.WBP_Main_C"));
            PossiblePaths.Add(TEXT("/Game/AtlantisEons/WBP_Main.WBP_Main_C"));
            
            // Try each path until we find one that works
            for (const FString& PathToTry : PossiblePaths)
            {
                UE_LOG(LogTemp, Warning, TEXT("HUD - Trying to load WBP_Main from path: %s"), *PathToTry);
                FoundWidgetClass = LoadClass<UWBP_Main>(nullptr, *PathToTry);
                if (FoundWidgetClass)
                {
                    UE_LOG(LogTemp, Warning, TEXT("HUD - Successfully loaded WBP_Main from path: %s"), *PathToTry);
                    MainWidgetClassToUse = FoundWidgetClass;
                    break;
                }
            }
            
            // If we still couldn't find it, try a more general search
            if (!MainWidgetClassToUse)
            {
                // Try to find it in the game content directory
                for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
                {
                    UClass* FoundClass = *ClassIt;
                    if (FoundClass->IsChildOf(UWBP_Main::StaticClass()) && !FoundClass->HasAnyClassFlags(CLASS_Abstract) && FoundClass->GetName().Contains(TEXT("WBP_Main")))
                    {
                        MainWidgetClassToUse = FoundClass;
                        UE_LOG(LogTemp, Warning, TEXT("HUD - Found WBP_Main class via object iterator: %s"), *FoundClass->GetPathName());
                        break;
                    }
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("HUD - Using MainWidgetClass set in Blueprint: %s"), *MainWidgetClassToUse->GetName());
        }
        
        // If we still don't have a valid class, try the fallback approach
        if (!MainWidgetClassToUse)
        {
            UE_LOG(LogTemp, Error, TEXT("HUD - Failed to find WBP_Main class by any method, trying fallback"));
            
            // Emergency fallback: Create an inventory widget directly if we have that class
            if (InventoryWidgetClass)
            {
                UE_LOG(LogTemp, Warning, TEXT("HUD - Creating fallback inventory widget"));
                InventoryWidget = CreateWidget<UUserWidget>(GetWorld(), InventoryWidgetClass);
                if (InventoryWidget)
                {
                    // Add to viewport and show cursor
                    InventoryWidget->AddToViewport(100);
                    SetGameInputMode(false);
                    
                    // Notify the player character that inventory is open
                    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
                    {
                        if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PC->GetPawn()))
                        {
                            Character->ForceSetInventoryState(true);
                            UE_LOG(LogTemp, Warning, TEXT("HUD - Set character inventory state to open"));
                        }
                    }
                    
                    UE_LOG(LogTemp, Warning, TEXT("HUD - Fallback inventory widget created"));
                    return true;
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("HUD - Failed to create fallback inventory widget"));
                    return false;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("HUD - No fallback inventory widget class available"));
                
                // Add on-screen message to inform the player
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Inventory system is missing required assets. Check logs."));
                }
                
                return false;
            }
        }
        
        // Create the widget with our found class
        MainWidget = CreateWidget<UWBP_Main>(GetWorld(), MainWidgetClassToUse);
        if (!MainWidget)
        {
            UE_LOG(LogTemp, Error, TEXT("HUD - Failed to create Main widget"));
            return false;
        }
        
        // Connect the inventory component to the newly created MainWidget
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PC->GetPawn()))
            {
                Character->ConnectInventoryToMainWidget();
            }
        }
    }
    
    // Add the widget to the viewport if it's not already there
    if (!MainWidget->IsInViewport())
    {
        MainWidget->AddToViewport(100);
    }
    
    // Make the widget visible
    MainWidget->SetVisibility(ESlateVisibility::Visible);
    
    // Switch to the inventory tab (index 1)
    if (MainWidget->WidgetSwitcher)
    {
        MainWidget->WidgetSwitcher->SetActiveWidgetIndex(1);
        UE_LOG(LogTemp, Warning, TEXT("HUD - Switched to inventory tab"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("HUD - Widget switcher is null!"));
    }
    
    // Set input mode to UI mode
    SetGameInputMode(false);
    
    // Notify the player character that inventory is open
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(PC->GetPawn()))
        {
            if (!Character->IsInventoryOpen())
            {
                Character->ForceSetInventoryState(true);
                UE_LOG(LogTemp, Warning, TEXT("HUD - Set character inventory state to open"));
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("HUD - Inventory widget created and shown"));
    return true;
}

void AAtlantisEonsHUD::HideInventoryWidget()
{
    UE_LOG(LogTemp, Warning, TEXT("HUD - HideInventoryWidget called"));
    
    // If the main widget exists, hide it
    if (MainWidget && MainWidget->IsInViewport())
    {
        // Just remove it from viewport completely
        MainWidget->RemoveFromParent();
        UE_LOG(LogTemp, Warning, TEXT("HUD - Main widget removed from viewport"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD - Main widget not in viewport or null"));
    }
    
    // Set input mode back to game
    SetGameInputMode(true);
    
    UE_LOG(LogTemp, Warning, TEXT("HUD - Inventory widget hidden and removed"));
}

bool AAtlantisEonsHUD::IsInventoryWidgetVisible() const
{
    // Check if the main widget exists and is visible
    if (MainWidget && MainWidget->IsInViewport())
    {
        // Check if the widget switcher exists and is showing the inventory tab (index 1)
        if (MainWidget->WidgetSwitcher && MainWidget->WidgetSwitcher->GetActiveWidgetIndex() == 1)
        {
            return true;
        }
    }
    
    return false;
}

UWBP_Main* AAtlantisEonsHUD::GetMainWidget() const
{
    return MainWidget;
}
