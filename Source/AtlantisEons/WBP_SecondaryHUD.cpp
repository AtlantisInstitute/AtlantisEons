#include "WBP_SecondaryHUD.h"
#include "AtlantisEonsCharacter.h"
#include "CharacterStatsComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetTextLibrary.h"
#include "Engine/Engine.h"

UWBP_SecondaryHUD::UWBP_SecondaryHUD(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize default values
    PlayerLevel = 1;
    CurrentExp = 0;
    ExpForNextLevel = 100;
    BaseExpRequirement = 100;
    ExpGrowthMultiplier = 1.5f;
    RecentlyEarnedExp = 0;
    MaxLevel = 100;
    
    // Initialize default colors
    HealthyColor = FLinearColor::Red;      // Health bar always red
    DamagedColor = FLinearColor::Red;      // Health bar always red
    CriticalColor = FLinearColor::Red;     // Health bar always red
    ExpBarColor = FLinearColor::Green;     // Experience bar always green
    
    // Initialize animation properties
    ExpEarnedAnimationProgress = 0.0f;
    LevelUpAnimationProgress = 0.0f;
    ExpEarnedAnimationDuration = 0.75f;
    LevelUpAnimationDuration = 1.5f;
    AnimationUpdateRate = 60.0f;
    
    Character = nullptr;
    
    UE_LOG(LogTemp, Log, TEXT("WBP_SecondaryHUD: Constructor called"));
}

void UWBP_SecondaryHUD::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Error, TEXT("🔧 WBP_SecondaryHUD: NativeConstruct called - HUD is being created"));
    
    // Verify all widget bindings
    UE_LOG(LogTemp, Warning, TEXT("🔧 Widget Binding Verification:"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   CanvasPanel: %s"), CanvasPanel ? TEXT("✅ FOUND") : TEXT("❌ NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   HealthBar: %s"), HealthBar ? TEXT("✅ FOUND") : TEXT("❌ NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   ExpBar: %s"), ExpBar ? TEXT("✅ FOUND") : TEXT("❌ NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   PlayerLevelText: %s"), PlayerLevelText ? TEXT("✅ FOUND") : TEXT("❌ NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   ExpEarnedText: %s"), ExpEarnedText ? TEXT("✅ FOUND") : TEXT("❌ NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   LevelUpTxt: %s"), LevelUpTxt ? TEXT("✅ FOUND") : TEXT("❌ NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   WeaponText: %s"), WeaponText ? TEXT("✅ FOUND") : TEXT("❌ NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   DodgeText: %s"), DodgeText ? TEXT("✅ FOUND") : TEXT("❌ NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🔧   ShieldText: %s"), ShieldText ? TEXT("✅ FOUND") : TEXT("❌ NULL"));
    
    // Setup text bindings for all text elements
    SetupTextBindings();
    
    // Try to automatically find and bind to the player character
    if (UWorld* World = GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("🔧 WBP_SecondaryHUD: World found, looking for player controller"));
        
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            UE_LOG(LogTemp, Error, TEXT("🔧 WBP_SecondaryHUD: Player controller found, looking for pawn"));
            
            if (AAtlantisEonsCharacter* PlayerCharacter = Cast<AAtlantisEonsCharacter>(PC->GetPawn()))
            {
                UE_LOG(LogTemp, Error, TEXT("✅ WBP_SecondaryHUD: Player character found - auto-connecting"));
                InitializeHUD(PlayerCharacter);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ WBP_SecondaryHUD: Player controller pawn is not AtlantisEonsCharacter"));
                if (PC->GetPawn())
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ WBP_SecondaryHUD: Pawn type: %s"), *PC->GetPawn()->GetClass()->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ WBP_SecondaryHUD: No pawn found on player controller"));
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ WBP_SecondaryHUD: No player controller found"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ WBP_SecondaryHUD: No world found"));
    }
}

void UWBP_SecondaryHUD::NativeDestruct()
{
    // Clean up animation timers
    if (GetWorld())
    {
        if (GetWorld()->GetTimerManager().IsTimerActive(ExpEarnedAnimationTimer))
        {
            GetWorld()->GetTimerManager().ClearTimer(ExpEarnedAnimationTimer);
        }
        if (GetWorld()->GetTimerManager().IsTimerActive(LevelUpAnimationTimer))
        {
            GetWorld()->GetTimerManager().ClearTimer(LevelUpAnimationTimer);
        }
    }
    
    // Unbind from character events before destruction
    UnbindFromCharacterEvents();
    
    Super::NativeDestruct();
    
    UE_LOG(LogTemp, Log, TEXT("WBP_SecondaryHUD: NativeDestruct called"));
}

void UWBP_SecondaryHUD::InitializeHUD(AAtlantisEonsCharacter* InCharacter)
{
    if (!InCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ WBP_SecondaryHUD::InitializeHUD: Character is null"));
        return;
    }
    
    UE_LOG(LogTemp, Error, TEXT("🔧 WBP_SecondaryHUD: Initializing HUD with character %s"), *InCharacter->GetName());
    
    // Unbind from previous character if any
    UnbindFromCharacterEvents();
    
    // Set new character reference
    Character = InCharacter;
    
    // Check if character has stats component
    if (UCharacterStatsComponent* StatsComp = Character->GetStatsComponent())
    {
        UE_LOG(LogTemp, Error, TEXT("✅ WBP_SecondaryHUD: Character has valid StatsComponent"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ WBP_SecondaryHUD: Character has NO StatsComponent!"));
    }
    
    // Bind to character events
    BindToCharacterEvents();
    
    // Update all UI elements
    UpdateAllElements();
    
    UE_LOG(LogTemp, Error, TEXT("✅ WBP_SecondaryHUD: Successfully initialized with character %s"), *Character->GetName());
}

void UWBP_SecondaryHUD::UpdateAllElements()
{
    UpdateHealthBar();
    UpdateExperienceBar();
    UpdateManaBar();
    
    UE_LOG(LogTemp, Log, TEXT("WBP_SecondaryHUD: Updated all UI elements"));
}

void UWBP_SecondaryHUD::UpdateHealthBar()
{
    UE_LOG(LogTemp, Warning, TEXT("🩺 UpdateHealthBar called"));
    UE_LOG(LogTemp, Warning, TEXT("🩺   Character: %s"), Character ? TEXT("VALID") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("🩺   HealthBar: %s"), HealthBar ? TEXT("VALID") : TEXT("NULL"));
    
    // ENHANCED DEBUG: Add call stack info to track what's triggering this update
    if (Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("🩺 📍 Character Name: %s, Address: %p"), *Character->GetName(), Character);
        UE_LOG(LogTemp, Warning, TEXT("🩺 📍 Character Valid: %s, Is Pending Kill: %s"), 
               IsValid(Character) ? TEXT("YES") : TEXT("NO"),
               Character->IsActorBeingDestroyed() ? TEXT("YES") : TEXT("NO"));
    }
    
    if (!Character || !HealthBar)
    {
        UE_LOG(LogTemp, Error, TEXT("🩺 ❌ Cannot update health bar - missing Character or HealthBar widget"));
        return;
    }
    
    // ENHANCED DEBUG: Get health values with detailed logging
    float CurrentHealth = Character->GetCurrentHealth();
    float MaxHealth = Character->GetMaxHealth();
    
    UE_LOG(LogTemp, Warning, TEXT("🩺 📊 CHARACTER Health values: Current=%.1f / Max=%.1f"), CurrentHealth, MaxHealth);
    
    // Check CharacterStatsComponent values separately
    float StatsCurrentHealth = 0.0f;
    float StatsMaxHealth = 0.0f;
    bool bHasValidStatsComponent = false;
    
    if (Character->GetStatsComponent())
    {
        StatsCurrentHealth = Character->GetStatsComponent()->GetCurrentHealth();
        StatsMaxHealth = Character->GetStatsComponent()->GetMaxHealth();
        bHasValidStatsComponent = true;
        UE_LOG(LogTemp, Warning, TEXT("🩺 📊 STATSCOMPONENT Health values: Current=%.1f / Max=%.1f"), StatsCurrentHealth, StatsMaxHealth);
        
        if (FMath::Abs(CurrentHealth - StatsCurrentHealth) > 0.1f || FMath::Abs(MaxHealth - StatsMaxHealth) > 0.1f)
        {
            UE_LOG(LogTemp, Error, TEXT("🩺 ⚠️ MISMATCH between Character and StatsComponent health values!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🩺 ❌ No CharacterStatsComponent found!"));
    }
    
    // DECISION LOGIC: Choose which health values to use
    float FinalCurrentHealth = CurrentHealth;
    float FinalMaxHealth = MaxHealth;
    
    // If Character health values are invalid or zero, try StatsComponent
    if ((MaxHealth <= 0.0f || CurrentHealth < 0.0f) && bHasValidStatsComponent && StatsMaxHealth > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🩺 🔄 Using StatsComponent values because Character values are invalid"));
        FinalCurrentHealth = StatsCurrentHealth;
        FinalMaxHealth = StatsMaxHealth;
    }
    // If both Character values are 0 but StatsComponent has valid values, prefer StatsComponent
    else if (CurrentHealth == 0.0f && MaxHealth == 0.0f && bHasValidStatsComponent && StatsMaxHealth > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🩺 🔄 Using StatsComponent values because Character has zero health"));
        FinalCurrentHealth = StatsCurrentHealth;
        FinalMaxHealth = StatsMaxHealth;
    }
    // If still no valid values, wait for proper initialization
    else if (FinalMaxHealth <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("🩺 ⚠️ No valid health values available - Character may not be fully initialized yet"));
        UE_LOG(LogTemp, Error, TEXT("🩺 ⚠️ Skipping health bar update until valid values are available"));
        return;  // Don't update with invalid values
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🩺 ✅ FINAL Health values chosen: Current=%.1f / Max=%.1f"), FinalCurrentHealth, FinalMaxHealth);
    
    // Calculate percentage
    float HealthPercentage = (FinalMaxHealth > 0.0f) ? (FinalCurrentHealth / FinalMaxHealth) : 0.0f;
    HealthPercentage = FMath::Clamp(HealthPercentage, 0.0f, 1.0f);
    
    UE_LOG(LogTemp, Warning, TEXT("🩺 Setting health bar percentage to: %.3f (%.1f%%)"), HealthPercentage, HealthPercentage * 100.0f);
    
    // FORCE VISUAL REFRESH: Ensure the health bar is visible and properly styled
    if (HealthBar)
    {
        // ENHANCED DEBUG: Force maximum visibility
        HealthBar->SetVisibility(ESlateVisibility::Visible);
        HealthBar->SetRenderOpacity(1.0f);
        
        // Set the percentage
        HealthBar->SetPercent(HealthPercentage);
        
        // CRITICAL DEBUG: Check if SetPercent actually worked
        float ActualPercentage = HealthBar->GetPercent();
        UE_LOG(LogTemp, Error, TEXT("🔥 CRITICAL: Set percentage to %.3f, but actual percentage is %.3f"), 
               HealthPercentage, ActualPercentage);
        
        if (FMath::Abs(HealthPercentage - ActualPercentage) > 0.01f)
        {
            UE_LOG(LogTemp, Error, TEXT("🔥 ❌ PERCENTAGE MISMATCH! SetPercent() is not working or being overridden!"));
            
            // Try setting it multiple times
            HealthBar->SetPercent(HealthPercentage);
            HealthBar->SetPercent(HealthPercentage);
            HealthBar->SetPercent(HealthPercentage);
            
            float SecondTryPercentage = HealthBar->GetPercent();
            UE_LOG(LogTemp, Error, TEXT("🔥 After multiple attempts, percentage is now: %.3f"), SecondTryPercentage);
        }
        
        // FORCE HEALTH BAR TO RED as requested by user
        FLinearColor HealthColor = FLinearColor::Red;  // Always red for health bar
        
        // FORCE RED VISIBLE COLOR - Override any Blueprint settings
        HealthBar->SetFillColorAndOpacity(HealthColor);
        
        // ADDITIONAL DEBUG: Force widget properties refresh
        HealthBar->SetIsEnabled(true);
        
        // FORCE WIDGET REFRESH - Use proper UE5 methods
        HealthBar->SynchronizeProperties();
        
        UE_LOG(LogTemp, Error, TEXT("🩺 🔥 FORCED VISIBILITY: Health bar percentage=%.3f, Color=R%.2f G%.2f B%.2f, Opacity=%.2f"), 
               HealthPercentage, HealthColor.R, HealthColor.G, HealthColor.B, HealthBar->GetRenderOpacity());
        
        UE_LOG(LogTemp, Warning, TEXT("🩺 ✅ Health bar updated - %.1f/%.1f (%.1f%%) Color: R=%.2f G=%.2f B=%.2f"), 
               FinalCurrentHealth, FinalMaxHealth, HealthPercentage * 100.0f, 
               HealthColor.R, HealthColor.G, HealthColor.B);
    }
}

void UWBP_SecondaryHUD::UpdateExperienceBar()
{
    UE_LOG(LogTemp, Warning, TEXT("📊 UpdateExperienceBar called"));
    UE_LOG(LogTemp, Warning, TEXT("📊   ExpBar: %s"), ExpBar ? TEXT("VALID") : TEXT("NULL"));
    
    if (!ExpBar)
    {
        UE_LOG(LogTemp, Error, TEXT("📊 ❌ Cannot update exp bar - missing ExpBar widget"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("📊 Exp values: %d / %d (Level %d)"), CurrentExp, ExpForNextLevel, PlayerLevel);
    
    // Calculate experience percentage for current level
    float ExpPercentage = GetExpPercentage();
    
    UE_LOG(LogTemp, Warning, TEXT("📊 Setting exp bar percentage to: %.3f"), ExpPercentage);
    
    // Update progress bar with green color
    ExpBar->SetPercent(ExpPercentage);
    ExpBar->SetFillColorAndOpacity(FLinearColor::Green);  // Always green for experience bar
    
    UE_LOG(LogTemp, Warning, TEXT("📊 ✅ Exp bar updated - %d/%d (%.1f%%) Level %d"), 
           CurrentExp, ExpForNextLevel, ExpPercentage * 100.0f, PlayerLevel);
}

void UWBP_SecondaryHUD::AddExperience(int32 ExpAmount)
{
    if (ExpAmount <= 0 || PlayerLevel >= MaxLevel)
    {
        return;
    }
    
    // Store recently earned exp for display
    RecentlyEarnedExp = ExpAmount;
    
    // Add experience
    CurrentExp += ExpAmount;
    
    UE_LOG(LogTemp, Error, TEXT("💰 WBP_SecondaryHUD: Added %d experience (%d total)"), ExpAmount, CurrentExp);
    UE_LOG(LogTemp, Error, TEXT("💰 About to play ExpEarned animation..."));
    
    // Play experience earned animation
    PlayExpEarnedAnimation();
    
    // Check for level ups
    bool bLeveledUp = false;
    while (CurrentExp >= ExpForNextLevel && PlayerLevel < MaxLevel)
    {
        // Level up!
        CurrentExp -= ExpForNextLevel;
        PlayerLevel++;
        bLeveledUp = true;
        
        // Calculate new exp requirement for next level
        ExpForNextLevel = CalculateExpRequiredForLevel(PlayerLevel + 1) - CalculateExpRequiredForLevel(PlayerLevel);
        
        UE_LOG(LogTemp, Warning, TEXT("WBP_SecondaryHUD: LEVEL UP! Player is now level %d"), PlayerLevel);
        
        // Broadcast level up event
        OnPlayerLevelUp.Broadcast(PlayerLevel);
        
        // Handle level up logic
        HandleLevelUp();
    }
    
    // Broadcast experience changed event
    OnExperienceChanged.Broadcast(CurrentExp, ExpForNextLevel);
    
    // Update UI
    UpdateExperienceBar();
    
    if (bLeveledUp)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SecondaryHUD: Player leveled up to %d! Next level requires %d exp"), 
               PlayerLevel, ExpForNextLevel);
    }
}

int32 UWBP_SecondaryHUD::GetExpForCurrentLevel() const
{
    if (PlayerLevel <= 1)
    {
        return 0;
    }
    
    return CalculateExpRequiredForLevel(PlayerLevel);
}

int32 UWBP_SecondaryHUD::CalculateExpRequiredForLevel(int32 Level) const
{
    if (Level <= 1)
    {
        return 0;
    }
    
    // Calculate total experience needed to reach this level using exponential growth
    int32 TotalExp = 0;
    for (int32 i = 2; i <= Level; ++i)
    {
        int32 ExpForThisLevel = FMath::RoundToInt(BaseExpRequirement * FMath::Pow(ExpGrowthMultiplier, i - 2));
        TotalExp += ExpForThisLevel;
    }
    
    return TotalExp;
}

float UWBP_SecondaryHUD::GetExpPercentage() const
{
    if (ExpForNextLevel <= 0)
    {
        return 1.0f; // Max level
    }
    
    return static_cast<float>(CurrentExp) / static_cast<float>(ExpForNextLevel);
}

// ========== TEXT BINDING FUNCTIONS ==========

FText UWBP_SecondaryHUD::GetCurrentHealthText() const
{
    if (!Character)
    {
        return FText::FromString(TEXT("0"));
    }
    
    return FText::FromString(FString::Printf(TEXT("%.0f"), Character->GetCurrentHealth()));
}

FText UWBP_SecondaryHUD::GetMaxHealthText() const
{
    if (!Character)
    {
        return FText::FromString(TEXT("0"));
    }
    
    return FText::FromString(FString::Printf(TEXT("%.0f"), Character->GetMaxHealth()));
}

FText UWBP_SecondaryHUD::GetPlayerLevelText() const
{
    return FText::FromString(FString::Printf(TEXT("Level %d"), PlayerLevel));
}

FText UWBP_SecondaryHUD::GetCurrentExpText() const
{
    return FText::FromString(FString::Printf(TEXT("%d"), CurrentExp));
}

FText UWBP_SecondaryHUD::GetMaxExpText() const
{
    return FText::FromString(FString::Printf(TEXT("%d"), ExpForNextLevel));
}

FText UWBP_SecondaryHUD::GetEarnedExpText() const
{
    if (RecentlyEarnedExp > 0)
    {
        return FText::FromString(FString::Printf(TEXT("Exp +%d"), RecentlyEarnedExp));
    }
    
    return FText::FromString(TEXT(""));
}

// ========== HELPER FUNCTIONS ==========

void UWBP_SecondaryHUD::UpdateHealthBarColor()
{
    if (!HealthBar || !Character)
    {
        return;
    }
    
    FLinearColor BarColor = GetHealthBarColor();
    HealthBar->SetFillColorAndOpacity(BarColor);
}

void UWBP_SecondaryHUD::UpdateManaBar()
{
    if (!ManaBar || !Character)
    {
        return;
    }

    // Get current and max mana from character
    float CurrentMana = Character->GetCurrentMP();
    float MaxMana = Character->GetMaxMP();
    bool bHasValidStatsComponent = (Character->GetStatsComponent() != nullptr);
    float StatsCurrentMana = 0.0f;
    float StatsMaxMana = 0.0f;
    if (bHasValidStatsComponent)
    {
        StatsCurrentMana = static_cast<float>(Character->GetStatsComponent()->GetCurrentMP());
        StatsMaxMana = static_cast<float>(Character->GetStatsComponent()->GetMaxMP());
    }

    float FinalCurrentMana = CurrentMana;
    float FinalMaxMana = MaxMana;

    // Prefer StatsComponent values if character's values are invalid or zero
    if ((MaxMana <= 0.0f || CurrentMana < 0.0f) && bHasValidStatsComponent && StatsMaxMana > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔵 🔄 Using StatsComponent values because Character values are invalid"));
        FinalCurrentMana = StatsCurrentMana;
        FinalMaxMana = StatsMaxMana;
    }
    else if (CurrentMana == 0.0f && MaxMana == 0.0f && bHasValidStatsComponent && StatsMaxMana > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔵 🔄 Using StatsComponent values because Character has zero mana"));
        FinalCurrentMana = StatsCurrentMana;
        FinalMaxMana = StatsMaxMana;
    }
    else if (FinalMaxMana <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("🔵 ⚠️ No valid mana values available - Character may not be fully initialized yet"));
        UE_LOG(LogTemp, Error, TEXT("🔵 ⚠️ Skipping mana bar update until valid values are available"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("🔵 ✅ FINAL Mana values chosen: Current=%.1f / Max=%.1f"), FinalCurrentMana, FinalMaxMana);

    // Calculate percentage
    float ManaPercentage = (FinalMaxMana > 0.0f) ? (FinalCurrentMana / FinalMaxMana) : 0.0f;
    ManaPercentage = FMath::Clamp(ManaPercentage, 0.0f, 1.0f);

    UE_LOG(LogTemp, Warning, TEXT("🔵 Setting mana bar percentage to: %.3f (%.1f%%)"), ManaPercentage, ManaPercentage * 100.0f);

    // Update progress bar
    ManaBar->SetPercent(ManaPercentage);
    ManaBar->SetFillColorAndOpacity(ManaBarColor);

    UE_LOG(LogTemp, Warning, TEXT("🔵 Mana bar updated - %.1f/%.1f (%.1f%%)"), 
           FinalCurrentMana, FinalMaxMana, ManaPercentage * 100.0f);
}

FLinearColor UWBP_SecondaryHUD::GetHealthBarColor() const
{
    // Always return red for health bar
    return FLinearColor::Red;
}

void UWBP_SecondaryHUD::HandleLevelUp()
{
    // This can be extended in Blueprint or overridden for specific level up behavior
    // For now, just log the level up
    UE_LOG(LogTemp, Warning, TEXT("WBP_SecondaryHUD: Player reached level %d!"), PlayerLevel);
    
    // Play level up animation
    PlayLevelUpAnimation();
    
    // You could add level up effects, sounds, or stat increases here
}

void UWBP_SecondaryHUD::BindToCharacterEvents()
{
    if (!Character)
    {
        return;
    }
    
    // Bind to character's stats component events
    if (UCharacterStatsComponent* StatsComp = Character->GetStatsComponent())
    {
        // Bind to health changes
        StatsComp->OnHealthChanged.AddDynamic(this, &UWBP_SecondaryHUD::OnCharacterHealthChanged);
        UE_LOG(LogTemp, Error, TEXT("🔗 WBP_SecondaryHUD: Bound to character health change events"));
        
        // Bind to experience changes
        StatsComp->OnExperienceChanged.AddDynamic(this, &UWBP_SecondaryHUD::OnCharacterExperienceChanged);
        UE_LOG(LogTemp, Error, TEXT("🔗 WBP_SecondaryHUD: Bound to character experience change events"));
        
        // Bind to level up events
        StatsComp->OnPlayerLevelUp.AddDynamic(this, &UWBP_SecondaryHUD::OnCharacterLevelUp);
        UE_LOG(LogTemp, Error, TEXT("🔗 WBP_SecondaryHUD: Bound to character level up events"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ WBP_SecondaryHUD: Failed to get StatsComponent for event binding!"));
    }
}

void UWBP_SecondaryHUD::UnbindFromCharacterEvents()
{
    if (!Character)
    {
        return;
    }
    
    // Unbind from character's stats component events
    if (UCharacterStatsComponent* StatsComp = Character->GetStatsComponent())
    {
        StatsComp->OnHealthChanged.RemoveDynamic(this, &UWBP_SecondaryHUD::OnCharacterHealthChanged);
        StatsComp->OnExperienceChanged.RemoveDynamic(this, &UWBP_SecondaryHUD::OnCharacterExperienceChanged);
        StatsComp->OnPlayerLevelUp.RemoveDynamic(this, &UWBP_SecondaryHUD::OnCharacterLevelUp);
        UE_LOG(LogTemp, Error, TEXT("🔗 WBP_SecondaryHUD: Unbound from all character events"));
    }
}

void UWBP_SecondaryHUD::OnCharacterHealthChanged(float NewHealthPercent)
{
    // Update health bar when character health changes
    UpdateHealthBar();
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("WBP_SecondaryHUD: Character health changed to %.1f%%"), NewHealthPercent * 100.0f);
}

void UWBP_SecondaryHUD::OnCharacterExperienceChanged(int32 NewCurrentExp, int32 NewMaxExp)
{
    UE_LOG(LogTemp, Error, TEXT("💰 WBP_SecondaryHUD: Experience changed! Current: %d, Max: %d"), NewCurrentExp, NewMaxExp);
    
    // Update our local experience values from the CharacterStatsComponent
    if (Character && Character->GetStatsComponent())
    {
        UCharacterStatsComponent* StatsComp = Character->GetStatsComponent();
        
        // Calculate the difference to show recently earned exp
        int32 ExpDifference = NewCurrentExp - CurrentExp;
        if (ExpDifference > 0)
        {
            RecentlyEarnedExp = ExpDifference;
            UE_LOG(LogTemp, Error, TEXT("💰 Earned %d experience points!"), RecentlyEarnedExp);
            
            // Play experience earned animation
            PlayExpEarnedAnimation();
        }
        
        // Update values from stats component - THIS IS THE KEY FIX
        CurrentExp = NewCurrentExp;
        ExpForNextLevel = NewMaxExp;
        PlayerLevel = StatsComp->GetPlayerLevel();
        
        UE_LOG(LogTemp, Error, TEXT("💰 Updated HUD values - Level: %d, Exp: %d/%d"), PlayerLevel, CurrentExp, ExpForNextLevel);
    }
    
    // Update experience bar
    UpdateExperienceBar();
}

void UWBP_SecondaryHUD::OnCharacterLevelUp(int32 NewLevel)
{
    UE_LOG(LogTemp, Error, TEXT("🎉 WBP_SecondaryHUD: LEVEL UP! New level: %d"), NewLevel);
    
    // Update our level
    PlayerLevel = NewLevel;
    
    // Play level up animation
    PlayLevelUpAnimation();
    
    // Update all UI elements
    UpdateAllElements();
}

void UWBP_SecondaryHUD::SetupTextBindings()
{
    // Bind text delegates for dynamic text updates
    if (PlayerLevelText)
    {
        PlayerLevelText->TextDelegate.BindUFunction(this, FName(TEXT("GetPlayerLevelText")));
        PlayerLevelText->SynchronizeProperties();
    }
    
    if (ExpEarnedText)
    {
        ExpEarnedText->TextDelegate.BindUFunction(this, FName(TEXT("GetEarnedExpText")));
        ExpEarnedText->SynchronizeProperties();
    }
    
    // Set static text for labels
    if (WeaponText)
    {
        WeaponText->SetText(FText::FromString(TEXT("Weapon")));
    }
    
    if (DodgeText)
    {
        DodgeText->SetText(FText::FromString(TEXT("Dodge")));
    }
    
    if (ShieldText)
    {
        ShieldText->SetText(FText::FromString(TEXT("Shield")));
    }
    
    UE_LOG(LogTemp, Log, TEXT("WBP_SecondaryHUD: Setup text bindings"));
}

// ========== ANIMATION FUNCTIONS IMPLEMENTATION ==========

void UWBP_SecondaryHUD::PlayExpEarnedAnimation()
{
    UE_LOG(LogTemp, Error, TEXT("🎬 WBP_SecondaryHUD: PlayExpEarnedAnimation() called (PROGRAMMATIC)!"));
    
    if (!ExpEarnedText)
    {
        UE_LOG(LogTemp, Error, TEXT("🎬 ExpEarnedText widget is NULL - cannot play animation!"));
        return;
    }
    
    if (!IsVisible())
    {
        UE_LOG(LogTemp, Error, TEXT("🎬 Widget not visible - cannot play animation!"));
        return;
    }
    
    // Stop any existing animation
    if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(ExpEarnedAnimationTimer))
    {
        GetWorld()->GetTimerManager().ClearTimer(ExpEarnedAnimationTimer);
        UE_LOG(LogTemp, Warning, TEXT("🎬 Stopped existing ExpEarned animation"));
    }
    
    // Reset animation progress
    ExpEarnedAnimationProgress = 0.0f;
    
    // Set initial text content
    FText ExpText = FText::FromString(FString::Printf(TEXT("+%d EXP"), RecentlyEarnedExp));
    ExpEarnedText->SetText(ExpText);
    
    UE_LOG(LogTemp, Error, TEXT("🎬 Starting programmatic ExpEarned animation:"));
    UE_LOG(LogTemp, Error, TEXT("🎬   Duration: %.2f seconds"), ExpEarnedAnimationDuration);
    UE_LOG(LogTemp, Error, TEXT("🎬   Update Rate: %.1f fps"), AnimationUpdateRate);
    UE_LOG(LogTemp, Error, TEXT("🎬   Text: %s"), *ExpText.ToString());
    
    // Start the animation timer
    if (GetWorld())
    {
        float UpdateInterval = 1.0f / AnimationUpdateRate;
        GetWorld()->GetTimerManager().SetTimer(
            ExpEarnedAnimationTimer,
            this,
            &UWBP_SecondaryHUD::UpdateExpEarnedAnimation,
            UpdateInterval,
            true  // Loop
        );
        
        UE_LOG(LogTemp, Error, TEXT("🎬 Animation timer started - updating every %.4f seconds"), UpdateInterval);
    }
}

void UWBP_SecondaryHUD::UpdateExpEarnedAnimation()
{
    if (!ExpEarnedText || !GetWorld())
    {
        return;
    }
    
    // Update progress
    float DeltaTime = 1.0f / AnimationUpdateRate;
    ExpEarnedAnimationProgress += DeltaTime / ExpEarnedAnimationDuration;
    
    if (ExpEarnedAnimationProgress >= 1.0f)
    {
        // Animation complete
        ExpEarnedAnimationProgress = 1.0f;
        ApplyExpEarnedAnimationFrame(ExpEarnedAnimationProgress);
        
        // Stop the timer
        GetWorld()->GetTimerManager().ClearTimer(ExpEarnedAnimationTimer);
        
        // Reset after a brief delay
        FTimerHandle ResetTimer;
        GetWorld()->GetTimerManager().SetTimer(
            ResetTimer,
            this,
            &UWBP_SecondaryHUD::ResetExpEarnedTextState,
            0.1f,
            false
        );
        
        UE_LOG(LogTemp, Error, TEXT("🎬 ExpEarned animation completed"));
    }
    else
    {
        // Apply current frame
        ApplyExpEarnedAnimationFrame(ExpEarnedAnimationProgress);
    }
}

void UWBP_SecondaryHUD::ApplyExpEarnedAnimationFrame(float Progress)
{
    if (!ExpEarnedText)
    {
        return;
    }
    
    // Animation keyframes based on Blueprint design:
    // 0.0s-0.1s: Fade in (Hidden -> Visible, Opacity 0 -> 1)
    // 0.1s-0.6s: Stay visible, move up
    // 0.6s-0.7s: Fade out (Opacity 1 -> 0, Visible -> Hidden)
    
    float NormalizedTime = Progress; // 0.0 to 1.0 over ExpEarnedAnimationDuration
    
    // Convert to keyframe times (based on 0.75s duration)
    float FadeInEnd = 0.1f / ExpEarnedAnimationDuration;     // ~0.133
    float FadeOutStart = 0.6f / ExpEarnedAnimationDuration;  // ~0.8
    
    ESlateVisibility TargetVisibility;
    float TargetOpacity;
    float TargetTranslationY;
    
    if (NormalizedTime <= FadeInEnd)
    {
        // Fade in phase
        float FadeProgress = NormalizedTime / FadeInEnd;
        TargetVisibility = ESlateVisibility::SelfHitTestInvisible;
        TargetOpacity = FMath::Lerp(0.0f, 1.0f, FadeProgress);
        TargetTranslationY = 0.0f;
    }
    else if (NormalizedTime < FadeOutStart)
    {
        // Visible phase with upward movement
        float MoveProgress = (NormalizedTime - FadeInEnd) / (FadeOutStart - FadeInEnd);
        TargetVisibility = ESlateVisibility::SelfHitTestInvisible;
        TargetOpacity = 1.0f;
        TargetTranslationY = FMath::Lerp(0.0f, -50.0f, MoveProgress);
    }
    else
    {
        // Fade out phase
        float FadeProgress = (NormalizedTime - FadeOutStart) / (1.0f - FadeOutStart);
        TargetVisibility = (FadeProgress >= 1.0f) ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible;
        TargetOpacity = FMath::Lerp(1.0f, 0.0f, FadeProgress);
        TargetTranslationY = -50.0f;
    }
    
    // Apply the animation properties
    ExpEarnedText->SetVisibility(TargetVisibility);
    ExpEarnedText->SetRenderOpacity(TargetOpacity);
    
    // Apply translation
    FVector2D CurrentTranslation = ExpEarnedText->GetRenderTransform().Translation;
    CurrentTranslation.Y = TargetTranslationY;
    
    FWidgetTransform NewTransform = ExpEarnedText->GetRenderTransform();
    NewTransform.Translation = CurrentTranslation;
    ExpEarnedText->SetRenderTransform(NewTransform);
}

void UWBP_SecondaryHUD::ResetExpEarnedTextState()
{
    if (!ExpEarnedText)
    {
        return;
    }
    
    // Reset to default state
    ExpEarnedText->SetVisibility(ESlateVisibility::Hidden);
    ExpEarnedText->SetRenderOpacity(0.0f);
    
    FWidgetTransform ResetTransform = ExpEarnedText->GetRenderTransform();
    ResetTransform.Translation = FVector2D(0.0f, 0.0f);
    ExpEarnedText->SetRenderTransform(ResetTransform);
    
    UE_LOG(LogTemp, Error, TEXT("🎬 ExpEarnedText reset to default state"));
}

void UWBP_SecondaryHUD::PlayLevelUpAnimation()
{
    UE_LOG(LogTemp, Error, TEXT("🎉 WBP_SecondaryHUD: PlayLevelUpAnimation() called (PROGRAMMATIC)!"));
    
    if (!LevelUpTxt)
    {
        UE_LOG(LogTemp, Error, TEXT("🎉 LevelUpTxt widget is NULL - cannot play animation!"));
        return;
    }
    
    if (!IsVisible())
    {
        UE_LOG(LogTemp, Error, TEXT("🎉 Widget not visible - cannot play animation!"));
        return;
    }
    
    // Stop any existing animation
    if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(LevelUpAnimationTimer))
    {
        GetWorld()->GetTimerManager().ClearTimer(LevelUpAnimationTimer);
        UE_LOG(LogTemp, Warning, TEXT("🎉 Stopped existing LevelUp animation"));
    }
    
    // Reset animation progress
    LevelUpAnimationProgress = 0.0f;
    
    // Set text content to generic level up message
    FText LevelText = FText::FromString(TEXT("LEVEL UP!"));
    LevelUpTxt->SetText(LevelText);
    
    UE_LOG(LogTemp, Error, TEXT("🎉 Starting programmatic LevelUp animation:"));
    UE_LOG(LogTemp, Error, TEXT("🎉   Duration: %.2f seconds"), LevelUpAnimationDuration);
    UE_LOG(LogTemp, Error, TEXT("🎉   Text: %s"), *LevelText.ToString());
    
    // Start the animation timer
    if (GetWorld())
    {
        float UpdateInterval = 1.0f / AnimationUpdateRate;
        GetWorld()->GetTimerManager().SetTimer(
            LevelUpAnimationTimer,
            this,
            &UWBP_SecondaryHUD::UpdateLevelUpAnimation,
            UpdateInterval,
            true  // Loop
        );
        
        UE_LOG(LogTemp, Error, TEXT("🎉 LevelUp animation timer started"));
    }
}

void UWBP_SecondaryHUD::UpdateLevelUpAnimation()
{
    if (!LevelUpTxt || !GetWorld())
    {
        return;
    }
    
    // Update progress
    float DeltaTime = 1.0f / AnimationUpdateRate;
    LevelUpAnimationProgress += DeltaTime / LevelUpAnimationDuration;
    
    if (LevelUpAnimationProgress >= 1.0f)
    {
        // Animation complete
        LevelUpAnimationProgress = 1.0f;
        ApplyLevelUpAnimationFrame(LevelUpAnimationProgress);
        
        // Stop the timer
        GetWorld()->GetTimerManager().ClearTimer(LevelUpAnimationTimer);
        
        // Reset after a brief delay
        FTimerHandle ResetTimer;
        GetWorld()->GetTimerManager().SetTimer(
            ResetTimer,
            this,
            &UWBP_SecondaryHUD::ResetLevelUpTextState,
            0.1f,
            false
        );
        
        UE_LOG(LogTemp, Error, TEXT("🎉 LevelUp animation completed"));
    }
    else
    {
        // Apply current frame
        ApplyLevelUpAnimationFrame(LevelUpAnimationProgress);
    }
}

void UWBP_SecondaryHUD::ApplyLevelUpAnimationFrame(float Progress)
{
    if (!LevelUpTxt)
    {
        return;
    }
    
    // Level up animation: longer duration with scaling and glow effect
    float NormalizedTime = Progress;
    
    // Keyframes for 1.5s animation:
    // 0.0s-0.2s: Scale up and fade in
    // 0.2s-1.2s: Stay visible with slight pulsing
    // 1.2s-1.5s: Fade out
    
    float FadeInEnd = 0.2f / LevelUpAnimationDuration;     // ~0.133
    float FadeOutStart = 1.2f / LevelUpAnimationDuration;  // ~0.8
    
    ESlateVisibility TargetVisibility;
    float TargetOpacity;
    float TargetScale;
    
    if (NormalizedTime <= FadeInEnd)
    {
        // Scale up and fade in
        float FadeProgress = NormalizedTime / FadeInEnd;
        TargetVisibility = ESlateVisibility::SelfHitTestInvisible;
        TargetOpacity = FMath::Lerp(0.0f, 1.0f, FadeProgress);
        TargetScale = FMath::Lerp(0.5f, 1.2f, FadeProgress);
    }
    else if (NormalizedTime < FadeOutStart)
    {
        // Visible with slight pulsing
        float PulseProgress = (NormalizedTime - FadeInEnd) / (FadeOutStart - FadeInEnd);
        float PulseValue = FMath::Sin(PulseProgress * PI * 4.0f) * 0.1f; // 4 pulses
        
        TargetVisibility = ESlateVisibility::SelfHitTestInvisible;
        TargetOpacity = 1.0f;
        TargetScale = 1.0f + PulseValue;
    }
    else
    {
        // Fade out
        float FadeProgress = (NormalizedTime - FadeOutStart) / (1.0f - FadeOutStart);
        TargetVisibility = (FadeProgress >= 1.0f) ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible;
        TargetOpacity = FMath::Lerp(1.0f, 0.0f, FadeProgress);
        TargetScale = FMath::Lerp(1.0f, 0.8f, FadeProgress);
    }
    
    // Apply the animation properties
    LevelUpTxt->SetVisibility(TargetVisibility);
    LevelUpTxt->SetRenderOpacity(TargetOpacity);
    
    // Apply scale
    FWidgetTransform NewTransform = LevelUpTxt->GetRenderTransform();
    NewTransform.Scale = FVector2D(TargetScale, TargetScale);
    LevelUpTxt->SetRenderTransform(NewTransform);
}

void UWBP_SecondaryHUD::ResetLevelUpTextState()
{
    if (!LevelUpTxt)
    {
        return;
    }
    
    // Reset to default state
    LevelUpTxt->SetVisibility(ESlateVisibility::Hidden);
    LevelUpTxt->SetRenderOpacity(0.0f);
    
    FWidgetTransform ResetTransform = LevelUpTxt->GetRenderTransform();
    ResetTransform.Scale = FVector2D(1.0f, 1.0f);
    ResetTransform.Translation = FVector2D(0.0f, 0.0f);
    LevelUpTxt->SetRenderTransform(ResetTransform);
    
    UE_LOG(LogTemp, Error, TEXT("🎉 LevelUpTxt reset to default state"));
}

// ========== FORCE SECONDARY HUD REFRESH FUNCTION ==========

void UWBP_SecondaryHUD::ForceRefreshSecondaryHUD()
{
    UE_LOG(LogTemp, Warning, TEXT("🔄 ForceRefreshSecondaryHUD called - ensuring HUD stays visible and functional"));
    
    if (!IsVisible())
    {
        UE_LOG(LogTemp, Warning, TEXT("🔄 SecondaryHUD was hidden - making it visible again"));
        SetVisibility(ESlateVisibility::Visible);
    }
    
    // Force update all UI elements
    UpdateHealthBar();
    UpdateExperienceBar();
    UpdateManaBar();
    
    // Force widget visibility and bring to front
    if (GetParent())
    {
        UE_LOG(LogTemp, Warning, TEXT("🔄 Bringing SecondaryHUD to front"));
        // Force the widget to stay on top and visible
        SetRenderOpacity(1.0f);
        SetVisibility(ESlateVisibility::Visible);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🔄 ✅ SecondaryHUD refresh complete"));
}

void UWBP_SecondaryHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // DISABLED AUTO-REFRESH to prevent health bar reset loop
    // The auto-refresh was causing the health bar to constantly reset to 0%
    // because it was triggering refreshes when HealthBar->GetPercent() == 0.0f
    
    /*
    // Periodic refresh to ensure HUD stays functional (every 1.0 seconds)
    static float RefreshTimer = 0.0f;
    RefreshTimer += InDeltaTime;
    
    if (RefreshTimer >= 1.0f)
    {
        RefreshTimer = 0.0f;
        
        // Only check if we have basic components ready
        if (Character && HealthBar && ExpBar)
        {
            // Check if character is properly initialized (has valid stats component)
            if (Character->GetStatsComponent())
            {
                float StatsMaxHealth = Character->GetStatsComponent()->GetMaxHealth();
                
                // Only trigger refresh if:
                // 1. HUD is not visible, OR
                // 2. Health bar shows 0% but character actually has health
                if (!IsVisible() || (HealthBar->GetPercent() == 0.0f && StatsMaxHealth > 0.0f))
                {
                    UE_LOG(LogTemp, Warning, TEXT("🔄 Auto-refresh triggered - HUD needs updating"));
                    UE_LOG(LogTemp, Warning, TEXT("🔄   Visible: %s, HealthBar Percent: %.3f, Character MaxHealth: %.1f"), 
                           IsVisible() ? TEXT("YES") : TEXT("NO"), 
                           HealthBar->GetPercent(), 
                           StatsMaxHealth);
                    ForceRefreshSecondaryHUD();
                }
            }
            else
            {
                // Character not fully initialized yet, wait longer
                UE_LOG(LogTemp, Log, TEXT("🔄 Character not fully initialized yet, waiting..."));
            }
        }
    }
    */
} 