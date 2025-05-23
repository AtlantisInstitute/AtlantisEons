#include "WBP_CharacterInfo.h"
#include "AtlantisEonsCharacter.h"
#include "AtlantisEonsHUD.h"
#include "AtlantisEonsPlayerController.h"
#include "BP_SceneCapture.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "WBP_InventorySlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/KismetTextLibrary.h"
#include "GameFramework/InputSettings.h"
#include "TimerManager.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Math/Vector2D.h"
#include "Math/Color.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "HAL/PlatformMisc.h"
#include "Engine/GameInstance.h"
#include "UObject/Field.h"
#include "UObject/ObjectPtr.h"
#include "UObject/FieldIterator.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SlateBasics.h"
#include "Engine/Engine.h"

void UWBP_CharacterInfo::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTemp, Warning, TEXT("======== %s: Starting NativeConstruct ========"), *GetName());
    
    // If Canvas Panel is not bound, try to proceed anyway
    if (!IsValid(CanvasPanel))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Root Canvas Panel is invalid, but continuing anyway"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Root Canvas Panel is valid"), *GetName());
    }

    // Log text block status but continue anyway
    UE_LOG(LogTemp, Warning, TEXT("%s: Text block status - HP: %d, MP: %d, Damage: %d"), 
        *GetName(), IsValid(HP), IsValid(MP), IsValid(Damage));
    
    // Log button status with more detail
    UE_LOG(LogTemp, Warning, TEXT("%s: ==== Button Status Check ===="), *GetName());
    if (IsValid(QuitButton))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: QuitButton is valid (Name: %s, Visibility: %d)"), 
            *GetName(), *QuitButton->GetName(), (int32)QuitButton->GetVisibility());
            
        // Log any existing bindings
        int32 BindingCount = QuitButton->OnClicked.IsBound() ? 1 : 0;
        UE_LOG(LogTemp, Warning, TEXT("%s: QuitButton has %d OnClicked bindings before setup"), 
            *GetName(), BindingCount);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: QuitButton is NULL! Check your widget blueprint."), *GetName());
    }
    
    if (IsValid(ResumeButton))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: ResumeButton is valid (Name: %s, Visibility: %d)"), 
            *GetName(), *ResumeButton->GetName(), (int32)ResumeButton->GetVisibility());
            
        // Log any existing bindings
        int32 BindingCount = ResumeButton->OnClicked.IsBound() ? 1 : 0;
        UE_LOG(LogTemp, Warning, TEXT("%s: ResumeButton has %d OnClicked bindings before setup"), 
            *GetName(), BindingCount);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: ResumeButton is NULL! Check your widget blueprint."), *GetName());
    }

    // Setup button bindings with robust error checking
    UE_LOG(LogTemp, Warning, TEXT("%s: ==== Setting up Button Bindings ===="), *GetName());
    if (IsValid(QuitButton))
    {
        // First clear any existing bindings to prevent duplicate delegates
        QuitButton->OnClicked.Clear();
        QuitButton->OnClicked.AddDynamic(this, &UWBP_CharacterInfo::OnQuitButtonClicked);
        
        // Verify binding was successful
        bool bIsBound = QuitButton->OnClicked.IsBound();
        UE_LOG(LogTemp, Warning, TEXT("%s: Quit button bound successfully: %s"), 
            *GetName(), bIsBound ? TEXT("YES") : TEXT("NO - BINDING FAILED"));
            
        // Make sure it's visible and interactive
        if (QuitButton->GetVisibility() != ESlateVisibility::Visible)
        {
            QuitButton->SetVisibility(ESlateVisibility::Visible);
            UE_LOG(LogTemp, Warning, TEXT("%s: Explicitly set QuitButton to Visible"), *GetName());
        }
        
        // Make sure it's not disabled
        QuitButton->SetIsEnabled(true);
        UE_LOG(LogTemp, Warning, TEXT("%s: Ensured QuitButton is enabled"), *GetName());
    }

    if (IsValid(ResumeButton))
    {
        // First clear any existing bindings to prevent duplicate delegates
        ResumeButton->OnClicked.Clear();
        ResumeButton->OnClicked.AddDynamic(this, &UWBP_CharacterInfo::OnResumeButtonClicked);
        
        // Verify binding was successful
        bool bIsBound = ResumeButton->OnClicked.IsBound();
        UE_LOG(LogTemp, Warning, TEXT("%s: Resume button bound successfully: %s"), 
            *GetName(), bIsBound ? TEXT("YES") : TEXT("NO - BINDING FAILED"));
            
        // Make sure it's visible and interactive
        if (ResumeButton->GetVisibility() != ESlateVisibility::Visible)
        {
            ResumeButton->SetVisibility(ESlateVisibility::Visible);
            UE_LOG(LogTemp, Warning, TEXT("%s: Explicitly set ResumeButton to Visible"), *GetName());
        }
        
        // Make sure it's not disabled
        ResumeButton->SetIsEnabled(true);
        UE_LOG(LogTemp, Warning, TEXT("%s: Ensured ResumeButton is enabled"), *GetName());
    }

    // Try multiple methods to get character reference
    // 1. Check if Character is already set by parent widget
    if (IsValid(Character))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Character already set by parent: %s"), *GetName(), *Character->GetName());
    }
    else
    {
        // 2. Try via PlayerController
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
            {
                Character = Cast<AAtlantisEonsCharacter>(PC->GetPawn());
                
                if (IsValid(Character))
                {
                    UE_LOG(LogTemp, Warning, TEXT("%s: Found character via PlayerController: %s"), *GetName(), *Character->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("%s: Failed to get character via PlayerController"), *GetName());
                }
                
                if (IsValid(PC))
                {
                    PC->bShowMouseCursor = true;
                    PC->SetInputMode(FInputModeUIOnly());
                }
            }
        }
        
        // 3. Try finding directly in world if still not found
        if (!IsValid(Character) && GetWorld())
        {
            for (TActorIterator<AAtlantisEonsCharacter> It(GetWorld()); It; ++It)
            {
                Character = *It;
                if (IsValid(Character))
                {
                    UE_LOG(LogTemp, Warning, TEXT("%s: Found character via World Iterator: %s"), *GetName(), *Character->GetName());
                    break;
                }
            }
        }
    }

    // If we still don't have a valid character, we'll set up the rest of the widget
    // but skip the character-dependent parts
    if (!IsValid(Character))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: No valid character found, skipping character-dependent initialization"), *GetName());
        
        // Setup text bindings with null character
        SetupTextBindings();
        
        // Make sure this widget is visible
        SetVisibility(ESlateVisibility::Visible);
        return;
    }

    // Initialize the character preview - now with more resilience
    InitializeCharacterPreview();

    // Setup text bindings
    SetupTextBindings();

    // Update stats
    UpdateAllStats();

    UE_LOG(LogTemp, Warning, TEXT("%s: Widget constructed and stats updated"), *GetName());

    // Make sure this widget is visible
    SetVisibility(ESlateVisibility::Visible);
    
    // Final check of button bindings after construction
    UE_LOG(LogTemp, Warning, TEXT("%s: ==== Final Button Binding Check ===="), *GetName());
    if (IsValid(QuitButton))
    {
        bool bIsBound = QuitButton->OnClicked.IsBound();
        UE_LOG(LogTemp, Warning, TEXT("%s: QuitButton final binding check: %s"), 
            *GetName(), bIsBound ? TEXT("BOUND") : TEXT("NOT BOUND"));
    }
    
    if (IsValid(ResumeButton))
    {
        bool bIsBound = ResumeButton->OnClicked.IsBound();
        UE_LOG(LogTemp, Warning, TEXT("%s: ResumeButton final binding check: %s"), 
            *GetName(), bIsBound ? TEXT("BOUND") : TEXT("NOT BOUND"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("======== %s: NativeConstruct Complete ========"), *GetName());
}

void UWBP_CharacterInfo::InitializeCharacterPreview()
{
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot initialize character preview - Character is null"), *GetName());
        return;
    }

    if (!CharacterPreviewImage)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: CharacterPreviewImage widget is null"), *GetName());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s: Character preview system has been disabled"), *GetName());

    // Create a simple placeholder image instead
    UMaterialInterface* PreviewMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/AtlantisEons/Materials/M_RT_CharacterPreview_FullBody"));
    if (!IsValid(PreviewMaterial))
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to load preview material"), *GetName());
        return;
    }

    // Create dynamic material instance
    UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
    if (!IsValid(DynamicMaterial))
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to create dynamic material instance"), *GetName());
        return;
    }

    // Create and configure new brush
    FSlateBrush NewBrush;
    NewBrush.SetResourceObject(DynamicMaterial);
    NewBrush.DrawAs = ESlateBrushDrawType::Image;
    NewBrush.Tiling = ESlateBrushTileType::NoTile;
    NewBrush.ImageSize = FVector2D(512.0f, 512.0f);
    NewBrush.TintColor = FLinearColor::White;

    // Apply the brush to the image widget
    if (IsValid(CharacterPreviewImage))
    {
        CharacterPreviewImage->SetBrush(NewBrush);
        UE_LOG(LogTemp, Warning, TEXT("%s: Applied placeholder brush to image"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: CharacterPreviewImage became invalid during initialization"), *GetName());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s: Character preview placeholder initialized"), *GetName());
}

void UWBP_CharacterInfo::SetupTextBindings()
{
    // Bind all text blocks to their respective functions, even if Character is null
    // The getter functions will handle null checks
    if (HP) 
    {
        HP->TextDelegate.Clear();
        HP->TextDelegate.BindUFunction(this, FName("GetHPText"));
    }
    else 
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: HP text block is invalid"), *GetName());
    }

    if (MP) 
    {
        MP->TextDelegate.Clear();
        MP->TextDelegate.BindUFunction(this, FName("GetMPText"));
    }
    else 
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: MP text block is invalid"), *GetName());
    }

    if (Damage) 
    {
        Damage->TextDelegate.Clear();
        Damage->TextDelegate.BindUFunction(this, FName("GetDamageText"));
    }
    else 
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Damage text block is invalid"), *GetName());
    }

    if (Defence) 
    {
        Defence->TextDelegate.Clear();
        Defence->TextDelegate.BindUFunction(this, FName("GetDefenceText"));
    }
    else 
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Defence text block is invalid"), *GetName());
    }

    if (Strength) 
    {
        Strength->TextDelegate.Clear();
        Strength->TextDelegate.BindUFunction(this, FName("GetStrengthText"));
    }
    else 
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Strength text block is invalid"), *GetName());
    }

    if (Dex) 
    {
        Dex->TextDelegate.Clear();
        Dex->TextDelegate.BindUFunction(this, FName("GetDexText"));
    }
    else 
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Dex text block is invalid"), *GetName());
    }

    if (Intelligence) 
    {
        Intelligence->TextDelegate.Clear();
        Intelligence->TextDelegate.BindUFunction(this, FName("GetIntelligenceText"));
    }
    else 
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Intelligence text block is invalid"), *GetName());
    }

    UE_LOG(LogTemp, Warning, TEXT("%s: Text bindings setup complete"), *GetName());
}

FText UWBP_CharacterInfo::GetHPText() const
{
    if (!Character) return FText::FromString(TEXT("N/A"));
    return FText::FromString(FString::Printf(TEXT("%d/%d"), FMath::RoundToInt(Character->GetCurrentHealth()), FMath::RoundToInt(Character->GetBaseHealth())));
}

FText UWBP_CharacterInfo::GetMPText() const
{
    if (!Character) return FText::FromString(TEXT("N/A"));
    return FText::FromString(FString::Printf(TEXT("%d/%d"), Character->GetCurrentMP(), Character->GetMaxMP()));
}

FText UWBP_CharacterInfo::GetDamageText() const
{
    if (!Character) return FText::FromString(TEXT("N/A"));
    return FText::FromString(FString::Printf(TEXT("%d"), Character->GetCurrentDamage()));
}

FText UWBP_CharacterInfo::GetDefenceText() const
{
    if (!Character) return FText::FromString(TEXT("N/A"));
    return FText::FromString(FString::Printf(TEXT("%d"), Character->GetCurrentDefence()));
}

FText UWBP_CharacterInfo::GetStrengthText() const
{
    if (!Character) return FText::FromString(TEXT("N/A"));
    return FText::FromString(FString::Printf(TEXT("%d"), Character->GetCurrentSTR()));
}

FText UWBP_CharacterInfo::GetDexText() const
{
    if (!Character) return FText::FromString(TEXT("N/A"));
    return FText::FromString(FString::Printf(TEXT("%d"), Character->GetCurrentDEX()));
}

FText UWBP_CharacterInfo::GetIntelligenceText() const
{
    if (!Character) return FText::FromString(TEXT("N/A"));
    return FText::FromString(FString::Printf(TEXT("%d"), Character->GetCurrentINT()));
}

void UWBP_CharacterInfo::UpdateAllStats()
{
    if (!IsValid(Character))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s::UpdateAllStats() - No character reference"), *GetName());
        return;
    }

    // Update all text blocks with character stats
    // Health stat
    if (IsValid(HP))
    {
        HP->TextDelegate.BindUFunction(this, FName(TEXT("GetHPText")));
        HP->SynchronizeProperties();
    }

    // MP stat
    if (IsValid(MP))
    {
        MP->TextDelegate.BindUFunction(this, FName(TEXT("GetMPText")));
        MP->SynchronizeProperties();
    }

    // Damage stat
    if (IsValid(Damage))
    {
        Damage->TextDelegate.BindUFunction(this, FName(TEXT("GetDamageText")));
        Damage->SynchronizeProperties();
    }

    // Defence stat
    if (IsValid(Defence))
    {
        Defence->TextDelegate.BindUFunction(this, FName(TEXT("GetDefenceText")));
        Defence->SynchronizeProperties();
    }

    // Strength stat
    if (IsValid(Strength))
    {
        Strength->TextDelegate.BindUFunction(this, FName(TEXT("GetStrengthText")));
        Strength->SynchronizeProperties();
    }

    // Dex stat
    if (IsValid(Dex))
    {
        Dex->TextDelegate.BindUFunction(this, FName(TEXT("GetDexText")));
        Dex->SynchronizeProperties();
    }

    // Intelligence stat
    if (IsValid(Intelligence))
    {
        Intelligence->TextDelegate.BindUFunction(this, FName(TEXT("GetIntelligenceText")));
        Intelligence->SynchronizeProperties();
    }

    // Update bars if Character is valid and has MP
    if (Character->GetMaxMP() > 0)
    {
        float MPPercentage = static_cast<float>(Character->GetCurrentMP()) / Character->GetMaxMP();
        UpdateMPBar(MPPercentage);
    }

    // Update HP bar if Character has health
    if (Character->GetBaseHealth() > 0)
    {
        float HPPercentage = static_cast<float>(Character->GetCurrentHealth()) / Character->GetBaseHealth();
        UpdateHPBar();
    }

    UE_LOG(LogTemp, Warning, TEXT("%s::UpdateAllStats() - All stats updated"), *GetName());
}

void UWBP_CharacterInfo::UpdateHPBar()
{
    if (!CircularBarHP) return;

    // Try to load the base material with better error handling
    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/AtlantisEons/Materials/M_CircularBar"));
    if (!BaseMaterial)
    {
        // Try alternative paths or use a default material
        BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
        if (!BaseMaterial)
        {
            // Skip material update if we can't load any material
            return;
        }
    }

    // Create dynamic material instance if we have a valid base material
    UMaterialInstanceDynamic* DynamicHPMaterial = Cast<UMaterialInstanceDynamic>(CircularBarHP->GetBrush().GetResourceObject());
    if (!DynamicHPMaterial && BaseMaterial)
    {
        DynamicHPMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        if (DynamicHPMaterial)
        {
            FSlateBrush NewBrush = CircularBarHP->GetBrush();
            NewBrush.SetResourceObject(DynamicHPMaterial);
            CircularBarHP->SetBrush(NewBrush);
        }
    }

    // Update the material parameters if we have the dynamic material
    if (DynamicHPMaterial && Character)
    {
        float HPPercent = Character->GetCurrentHealth() > 0 ? 
            Character->GetCurrentHealth() / Character->GetBaseHealth() : 0.0f;
        
        DynamicHPMaterial->SetScalarParameterValue(TEXT("Percent"), HPPercent);
        DynamicHPMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Red);
    }
}

void UWBP_CharacterInfo::UpdateMPBar(float Percentage)
{
    if (!IsValid(CircularBarMP))
    {
        return;
    }
    
    // Clamp percentage between 0 and 1
    Percentage = FMath::Clamp(Percentage, 0.0f, 1.0f);
    
    // Implementation for updating MP bar visual
    // Check if we need to load or create a dynamic material
    UMaterialInstanceDynamic* MPMaterial = Cast<UMaterialInstanceDynamic>(CircularBarMP->GetBrush().GetResourceObject());
    
    if (!IsValid(MPMaterial))
    {
        // Try to load the base material with better error handling
        UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/AtlantisEons/Materials/M_CircularBar"));
        if (!BaseMaterial)
        {
            // Try alternative paths or use a default material
            BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
            if (!BaseMaterial)
            {
                // Skip material update if we can't load any material
                return;
            }
        }
        
        MPMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        if (!IsValid(MPMaterial))
        {
            return;
        }
        
        // Set the new material to the image
        FSlateBrush NewBrush = CircularBarMP->GetBrush();
        NewBrush.SetResourceObject(MPMaterial);
        CircularBarMP->SetBrush(NewBrush);
    }
    
    // Update the material parameter
    MPMaterial->SetScalarParameterValue(TEXT("Percent"), Percentage);
    
    // Set color to blue for MP
    FLinearColor MPColor = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
    MPMaterial->SetVectorParameterValue(TEXT("Color"), MPColor);
}

void UWBP_CharacterInfo::OnQuitButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("===== %s: QUIT BUTTON CLICKED ====="), *GetName());
    
    // Try multiple methods to get a valid world or player controller
    APlayerController* PC = nullptr;
    
    // Method 1: Direct GetWorld
    UWorld* World = GetWorld();
    if (World)
    {
        PC = World->GetFirstPlayerController();
        UE_LOG(LogTemp, Warning, TEXT("%s: Got PlayerController via direct GetWorld"), *GetName());
    }
    
    // Method 2: Via GameInstance
    if (!PC)
    {
        UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetOwningPlayer());
        if (GameInstance)
        {
            World = GameInstance->GetWorld();
            if (World)
            {
                PC = World->GetFirstPlayerController();
                UE_LOG(LogTemp, Warning, TEXT("%s: Got PlayerController via GameInstance"), *GetName());
            }
        }
    }
    
    // Method 3: Try to get PC directly from owning player
    if (!PC)
    {
        PC = GetOwningPlayer();
        if (PC)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Got PlayerController via GetOwningPlayer"), *GetName());
        }
    }
    
    // Now quit the game using the most appropriate method available
    if (PC)
    {
        // Method 1: ConsoleCommand - works in PIE and packaged games
        PC->ConsoleCommand("quit");
        UE_LOG(LogTemp, Warning, TEXT("%s: Issued quit console command"), *GetName());
    }
    else
    {
        // Method 2: Try static FPlatformMisc method - works across most platforms
        UE_LOG(LogTemp, Warning, TEXT("%s: Last resort quit attempt"), *GetName());
        
        // Try to get game instance first
        UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
        if (GameInstance)
        {
            // This is safer than requesting immediate shutdown
            GameInstance->ReturnToMainMenu();
            UE_LOG(LogTemp, Warning, TEXT("%s: Called ReturnToMainMenu on GameInstance"), *GetName());
        }
        else
        {
            // Direct platform quit - last resort
            FPlatformMisc::RequestExit(false);
            UE_LOG(LogTemp, Warning, TEXT("%s: Called FPlatformMisc::RequestExit"), *GetName());
        }
    }
}

void UWBP_CharacterInfo::OnResumeButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("===== %s: RESUME BUTTON CLICKED ====="), *GetName());
    
    // STEP 1: Immediately clear any UI focus
    if (GEngine && GEngine->GameViewport)
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
        UE_LOG(LogTemp, Warning, TEXT("%s: Cleared UI focus"), *GetName());
    }
    
    // STEP 2: Get the player controller and character
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    }
    
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Could not find PlayerController!"), *GetName());
        return;
    }
    
    // Find the character
    APawn* Pawn = PC->GetPawn();
    AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(Pawn);
    
    // STEP 3: Find and use the HUD's CompleteGameResume function
    AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(PC->GetHUD());
    if (HUD)
    {
        // Hide this widget first
        SetVisibility(ESlateVisibility::Hidden);
        
        // Call the enhanced resume function which will handle everything
        UE_LOG(LogTemp, Warning, TEXT("%s: Calling HUD->CompleteGameResume()"), *GetName());
        HUD->CompleteGameResume();
    }
    else
    {
        // Fallback if HUD is not available - do our best to restore input directly
        UE_LOG(LogTemp, Warning, TEXT("%s: No HUD found, using fallback input restoration"), *GetName());
        
        // Hide UI
        SetVisibility(ESlateVisibility::Hidden);
        
        // Remove from parent
        RemoveFromParent();
        
        // Set input mode to game only
        FInputModeGameOnly GameOnlyMode;
        PC->SetShowMouseCursor(false);
        PC->SetInputMode(GameOnlyMode);
        
        // Restore character input if possible
        if (Character)
        {
            Character->ResetCharacterInput();
            Character->ForceEnableMovement();
        }
    }
    
    // Broadcast the OnResumeGame event for any listeners
    OnResumeGame.Broadcast();
}

FText UWBP_CharacterInfo::GetText_CharacterHP()
{
    if (!Character)
    {
        return FText::GetEmpty();
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentHealth());
}

FText UWBP_CharacterInfo::GetText_CharacterMP()
{
    if (!Character)
    {
        return FText::GetEmpty();
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentMP());
}

FText UWBP_CharacterInfo::GetText_CharacterDamage()
{
    if (!Character)
    {
        return FText::GetEmpty();
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentDamage());
}

FText UWBP_CharacterInfo::GetText_CharacterDefence()
{
    if (!Character)
    {
        return FText::GetEmpty();
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentDefence());
}

FText UWBP_CharacterInfo::GetText_CharacterSTR()
{
    if (!Character)
    {
        return FText::GetEmpty();
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentSTR());
}

FText UWBP_CharacterInfo::GetText_CharacterDEX()
{
    if (!Character)
    {
        return FText::GetEmpty();
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentDEX());
}

FText UWBP_CharacterInfo::GetText_CharacterINT()
{
    if (!Character)
    {
        return FText::FromString(TEXT("0"));
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentINT());
}

FText UWBP_CharacterInfo::GetText_HPPercentage()
{
    if (!Character)
    {
        return FText::FromString(TEXT("0%"));
    }
    float HPPercentage = (Character->GetCurrentHealth() / Character->GetBaseHealth()) * 100.0f;
    return FText::FromString(FString::Printf(TEXT("%.1f%%"), HPPercentage));
}

FText UWBP_CharacterInfo::GetText_MPPercentage()
{
    if (!Character)
    {
        return FText::FromString(TEXT("0%"));
    }
    float MPPercentage = (float(Character->GetCurrentMP()) / float(Character->GetMaxMP())) * 100.0f;
    return FText::FromString(FString::Printf(TEXT("%.1f%%"), MPPercentage));
}

FText UWBP_CharacterInfo::GetText_DamageValue()
{
    if (!Character)
    {
        return FText::FromString(TEXT("0"));
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentDamage());
}

FText UWBP_CharacterInfo::GetText_DefenceValue()
{
    if (!Character)
    {
        return FText::FromString(TEXT("0"));
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentDefence());
}

FText UWBP_CharacterInfo::GetText_StrengthValue()
{
    if (!Character)
    {
        return FText::FromString(TEXT("0"));
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentSTR());
}

FText UWBP_CharacterInfo::GetText_DexValue()
{
    if (!Character)
    {
        return FText::FromString(TEXT("0"));
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentDEX());
}

FText UWBP_CharacterInfo::GetText_IntelligenceValue()
{
    if (!Character)
    {
        return FText::FromString(TEXT("0"));
    }
    return UKismetTextLibrary::Conv_IntToText(Character->GetCurrentINT());
}

void UWBP_CharacterInfo::HandleResumeClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: HandleResumeClicked called"), *GetName());
    
    // STEP 1: Immediately clear any UI focus
    if (GEngine && GEngine->GameViewport)
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
        UE_LOG(LogTemp, Warning, TEXT("%s: Cleared UI focus"), *GetName());
    }
    
    // STEP 2: Get the player controller and character
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    }
    
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Could not find PlayerController!"), *GetName());
        return;
    }
    
    // Find the character
    APawn* Pawn = PC->GetPawn();
    AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(Pawn);
    
    // STEP 3: Find and use the HUD's CompleteGameResume function
    AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(PC->GetHUD());
    if (HUD)
    {
        // Hide this widget first
        SetVisibility(ESlateVisibility::Hidden);
        
        // Call the enhanced resume function which will handle everything
        UE_LOG(LogTemp, Warning, TEXT("%s: Calling HUD->CompleteGameResume()"), *GetName());
        HUD->CompleteGameResume();
    }
    else
    {
        // Fallback if HUD is not available - do our best to restore input directly
        UE_LOG(LogTemp, Warning, TEXT("%s: No HUD found, using fallback input restoration"), *GetName());
        
        // Hide UI
        SetVisibility(ESlateVisibility::Hidden);
        
        // Remove from parent
        RemoveFromParent();
        
        // Set input mode to game only
        FInputModeGameOnly GameOnlyMode;
        PC->SetShowMouseCursor(false);
        PC->SetInputMode(GameOnlyMode);
        
        // Restore character input if possible
        if (Character)
        {
            Character->ResetCharacterInput();
            Character->ForceEnableMovement();
        }
    }
    
    // Broadcast the OnResumeGame event for any listeners
    OnResumeGame.Broadcast();
}
