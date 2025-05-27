#include "CharacterStatsComponent.h"
#include "Engine/Engine.h"

UCharacterStatsComponent::UCharacterStatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // Initialize stats to base values
    CurrentHealth = BaseHealth;
    MaxHealth = BaseHealth;
    CurrentSTR = BaseSTR;
    CurrentDEX = BaseDEX;
    CurrentINT = BaseINT;
    CurrentDefence = BaseDefence;
    CurrentDamage = BaseDamage;
}

void UCharacterStatsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeStats();
}

void UCharacterStatsComponent::InitializeStats()
{
    // Set current stats to base values
    CurrentHealth = BaseHealth;
    MaxHealth = BaseHealth;
    CurrentSTR = BaseSTR;
    CurrentDEX = BaseDEX;
    CurrentINT = BaseINT;
    CurrentDefence = BaseDefence;
    CurrentDamage = BaseDamage;
    
    ClampHealthMana();
    BroadcastStatChanges();
    
    UE_LOG(LogTemp, Log, TEXT("Character Stats Initialized - Health: %f/%f, STR: %d, DEX: %d, INT: %d"), 
           CurrentHealth, MaxHealth, CurrentSTR, CurrentDEX, CurrentINT);
}

void UCharacterStatsComponent::UpdateAllStats()
{
    // Recalculate max health based on current stats
    float PreviousMaxHealth = MaxHealth;
    MaxHealth = BaseHealth + (CurrentSTR * 2.0f); // Example: 2 HP per STR point
    
    // Adjust current health proportionally if max health changed
    if (PreviousMaxHealth > 0.0f && MaxHealth != PreviousMaxHealth)
    {
        float HealthRatio = CurrentHealth / PreviousMaxHealth;
        CurrentHealth = MaxHealth * HealthRatio;
    }
    
    // Update max mana based on INT
    int32 PreviousMaxMP = MaxMP;
    MaxMP = 100 + (CurrentINT * 5); // Example: 5 MP per INT point
    
    // Adjust current mana proportionally
    if (PreviousMaxMP > 0 && MaxMP != PreviousMaxMP)
    {
        float ManaRatio = static_cast<float>(CurrentMP) / static_cast<float>(PreviousMaxMP);
        CurrentMP = FMath::RoundToInt(MaxMP * ManaRatio);
    }
    
    ClampHealthMana();
    BroadcastStatChanges();
}

void UCharacterStatsComponent::RecoverHealth(int32 Amount)
{
    if (Amount <= 0) return;
    
    float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
    
    if (CurrentHealth != PreviousHealth)
    {
        OnHealthChanged.Broadcast(GetHealthPercent());
        UE_LOG(LogTemp, Log, TEXT("Health recovered: +%d (%.1f/%.1f)"), Amount, CurrentHealth, MaxHealth);
    }
}

void UCharacterStatsComponent::TakeDamage(float DamageAmount)
{
    if (DamageAmount <= 0.0f) return;
    
    float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Max(CurrentHealth - DamageAmount, 0.0f);
    
    if (CurrentHealth != PreviousHealth)
    {
        OnHealthChanged.Broadcast(GetHealthPercent());
        UE_LOG(LogTemp, Log, TEXT("Damage taken: %.1f (%.1f/%.1f remaining)"), DamageAmount, CurrentHealth, MaxHealth);
    }
}

float UCharacterStatsComponent::GetHealthPercent() const
{
    return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

void UCharacterStatsComponent::RecoverMana(int32 Amount)
{
    if (Amount <= 0) return;
    
    int32 PreviousMP = CurrentMP;
    CurrentMP = FMath::Min(CurrentMP + Amount, MaxMP);
    
    if (CurrentMP != PreviousMP)
    {
        OnManaChanged.Broadcast(GetManaPercent());
        UE_LOG(LogTemp, Log, TEXT("Mana recovered: +%d (%d/%d)"), Amount, CurrentMP, MaxMP);
    }
}

void UCharacterStatsComponent::ConsumeMana(int32 Amount)
{
    if (Amount <= 0) return;
    
    int32 PreviousMP = CurrentMP;
    CurrentMP = FMath::Max(CurrentMP - Amount, 0);
    
    if (CurrentMP != PreviousMP)
    {
        OnManaChanged.Broadcast(GetManaPercent());
        UE_LOG(LogTemp, Log, TEXT("Mana consumed: -%d (%d/%d remaining)"), Amount, CurrentMP, MaxMP);
    }
}

float UCharacterStatsComponent::GetManaPercent() const
{
    return MaxMP > 0 ? static_cast<float>(CurrentMP) / static_cast<float>(MaxMP) : 0.0f;
}

void UCharacterStatsComponent::AddStatModifier(int32 STRBonus, int32 DEXBonus, int32 INTBonus, int32 DefenceBonus, int32 DamageBonus, int32 HPBonus, int32 MPBonus)
{
    CurrentSTR += STRBonus;
    CurrentDEX += DEXBonus;
    CurrentINT += INTBonus;
    CurrentDefence += DefenceBonus;
    CurrentDamage += DamageBonus;
    
    // Handle health and mana bonuses
    if (HPBonus != 0)
    {
        MaxHealth += HPBonus;
        CurrentHealth += HPBonus; // Add to current health as well
    }
    
    if (MPBonus != 0)
    {
        MaxMP += MPBonus;
        CurrentMP += MPBonus; // Add to current mana as well
    }
    
    ClampHealthMana();
    BroadcastStatChanges();
    
    UE_LOG(LogTemp, Log, TEXT("Stat modifiers added - STR: +%d, DEX: +%d, INT: +%d, DEF: +%d, DMG: +%d, HP: +%d, MP: +%d"), 
           STRBonus, DEXBonus, INTBonus, DefenceBonus, DamageBonus, HPBonus, MPBonus);
}

void UCharacterStatsComponent::RemoveStatModifier(int32 STRBonus, int32 DEXBonus, int32 INTBonus, int32 DefenceBonus, int32 DamageBonus, int32 HPBonus, int32 MPBonus)
{
    CurrentSTR = FMath::Max(CurrentSTR - STRBonus, BaseSTR);
    CurrentDEX = FMath::Max(CurrentDEX - DEXBonus, BaseDEX);
    CurrentINT = FMath::Max(CurrentINT - INTBonus, BaseINT);
    CurrentDefence = FMath::Max(CurrentDefence - DefenceBonus, BaseDefence);
    CurrentDamage = FMath::Max(CurrentDamage - DamageBonus, BaseDamage);
    
    // Handle health and mana bonus removal
    if (HPBonus != 0)
    {
        MaxHealth = FMath::Max(MaxHealth - HPBonus, BaseHealth);
        // Don't remove from current health if it would exceed new max
        CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
    }
    
    if (MPBonus != 0)
    {
        MaxMP = FMath::Max(MaxMP - MPBonus, 100); // Minimum 100 MP
        CurrentMP = FMath::Min(CurrentMP, MaxMP);
    }
    
    ClampHealthMana();
    BroadcastStatChanges();
    
    UE_LOG(LogTemp, Log, TEXT("Stat modifiers removed - STR: -%d, DEX: -%d, INT: -%d, DEF: -%d, DMG: -%d, HP: -%d, MP: -%d"), 
           STRBonus, DEXBonus, INTBonus, DefenceBonus, DamageBonus, HPBonus, MPBonus);
}

bool UCharacterStatsComponent::SpendGold(int32 Amount)
{
    if (Amount <= 0 || Gold < Amount)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot spend %d gold - insufficient funds (%d available)"), Amount, Gold);
        return false;
    }
    
    Gold -= Amount;
    UE_LOG(LogTemp, Log, TEXT("Spent %d gold (%d remaining)"), Amount, Gold);
    return true;
}

void UCharacterStatsComponent::AddGold(int32 Amount)
{
    if (Amount <= 0) return;
    
    Gold += Amount;
    UE_LOG(LogTemp, Log, TEXT("Gained %d gold (%d total)"), Amount, Gold);
}

void UCharacterStatsComponent::ClampHealthMana()
{
    CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
    CurrentMP = FMath::Clamp(CurrentMP, 0, MaxMP);
}

void UCharacterStatsComponent::BroadcastStatChanges()
{
    OnStatsChanged.Broadcast();
    OnHealthChanged.Broadcast(GetHealthPercent());
    OnManaChanged.Broadcast(GetManaPercent());
} 