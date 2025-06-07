#include "WBP_SwordBloom.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "AtlantisEonsCharacter.h"
#include "CombatEffectsManagerComponent.h"

UWBP_SwordBloom::UWBP_SwordBloom(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize widget properties
    // Note: Widget initialization happens in NativeConstruct
}

void UWBP_SwordBloom::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: NativeConstruct called"));
    
    // Try multiple approaches to find the widgets
    
    // First check if BindWidget worked
    if (!SwordSpark)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: SwordSpark not bound, trying GetWidgetFromName"));
        SwordSpark = Cast<UImage>(GetWidgetFromName(TEXT("SwordSpark")));
    }
    
    if (!SwordBloom)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: SwordBloom not bound, trying GetWidgetFromName"));
        SwordBloom = Cast<UImage>(GetWidgetFromName(TEXT("SwordBloom")));
    }
    
    // Final check and setup
    if (SwordSpark)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: ✅ SwordSpark widget found successfully"));
        SwordSpark->SetColorAndOpacity(FLinearColor::White); // White color
        SwordSpark->SetVisibility(ESlateVisibility::Hidden);
        
        // Ensure SwordSpark has the same positioning as SwordBloom
        FWidgetTransform SparkTransform;
        SparkTransform.Translation = FVector2D(0.0f, 0.0f); // Center position
        SparkTransform.Scale = FVector2D(1.0f, 1.0f); // Same scale as bloom
        SparkTransform.Shear = FVector2D(0.0f, 0.0f);
        SparkTransform.Angle = 0.0f;
        SwordSpark->SetRenderTransform(SparkTransform);
        
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: SwordSpark positioned and aligned"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_SwordBloom: ❌ SwordSpark widget could not be found by any method"));
    }
    
    if (SwordBloom)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: ✅ SwordBloom widget found successfully"));
        SwordBloom->SetColorAndOpacity(FLinearColor::White); // White color
        SwordBloom->SetVisibility(ESlateVisibility::Hidden);
        
        // Ensure SwordBloom has consistent positioning
        FWidgetTransform BloomTransform;
        BloomTransform.Translation = FVector2D(0.0f, 0.0f); // Center position  
        BloomTransform.Scale = FVector2D(1.0f, 1.0f); // Initial scale
        BloomTransform.Shear = FVector2D(0.0f, 0.0f);
        BloomTransform.Angle = 0.0f;
        SwordBloom->SetRenderTransform(BloomTransform);
        
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: SwordBloom positioned and aligned"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_SwordBloom: ❌ SwordBloom widget could not be found by any method"));
    }
    
    // Force widget visibility
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Widget setup complete"));
}

void UWBP_SwordBloom::ShowSwordSpark()
{
    UE_LOG(LogTemp, Warning, TEXT("🎆 ⚡ ShowSwordSpark() called"));
    UE_LOG(LogTemp, Warning, TEXT("🎆 DEBUG - SwordSpark widget pointer: %s"), SwordSpark ? TEXT("VALID") : TEXT("NULL"));
    
    if (SwordSpark)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎆 ✅ SwordSpark widget found - proceeding with display"));
        
        // Make sure the spark appears at the same position as the bloom but at FULL scale
        if (SwordBloom)
        {
            // Copy the position from the bloom circle but use full scale for the spark
            FWidgetTransform BloomTransform = SwordBloom->GetRenderTransform();
            FWidgetTransform SparkTransform = BloomTransform; // Copy transform
            SparkTransform.Scale = FVector2D(1.0f, 1.0f); // Override scale to full size
            SwordSpark->SetRenderTransform(SparkTransform);
            
            UE_LOG(LogTemp, Warning, TEXT("🎆 SwordSpark positioned at bloom location but at full scale"));
        }
        else
        {
            // Fallback: just ensure spark is at full scale and centered
            FWidgetTransform SparkTransform;
            SparkTransform.Translation = FVector2D(0.0f, 0.0f);
            SparkTransform.Scale = FVector2D(1.0f, 1.0f); // Full scale
            SparkTransform.Shear = FVector2D(0.0f, 0.0f);
            SparkTransform.Angle = 0.0f;
            SwordSpark->SetRenderTransform(SparkTransform);
            
            UE_LOG(LogTemp, Warning, TEXT("🎆 SwordSpark set to full scale at center (fallback)"));
        }
        
        // Show the spark with debug info
        ESlateVisibility OldVisibility = SwordSpark->GetVisibility();
        SwordSpark->SetVisibility(ESlateVisibility::Visible);
        ESlateVisibility NewVisibility = SwordSpark->GetVisibility();
        
        UE_LOG(LogTemp, Warning, TEXT("🎆 ✨ SwordSpark visibility changed from %d to %d"), (int32)OldVisibility, (int32)NewVisibility);
        UE_LOG(LogTemp, Warning, TEXT("🎆 ✨ SwordSpark effect should now be VISIBLE on screen"));
        
        // Also make sure parent widget is visible
        if (GetVisibility() != ESlateVisibility::Visible && GetVisibility() != ESlateVisibility::SelfHitTestInvisible)
        {
            UE_LOG(LogTemp, Warning, TEXT("🎆 ⚠️ Parent widget not visible - making it visible"));
            SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🎆 ❌ CRITICAL: Cannot show SwordSpark - widget not found"));
        UE_LOG(LogTemp, Error, TEXT("🎆 ❌ This means the Blueprint widget setup is missing SwordSpark Image component"));
    }
}

void UWBP_SwordBloom::HideSwordSpark()
{
    if (SwordSpark)
    {
        SwordSpark->SetVisibility(ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Log, TEXT("WBP_SwordBloom: SwordSpark effect hidden"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Cannot hide SwordSpark - widget not found"));
    }
}

void UWBP_SwordBloom::ShowSwordBloom()
{
    if (SwordBloom)
    {
        SwordBloom->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Log, TEXT("WBP_SwordBloom: SwordBloom effect shown"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Cannot show SwordBloom - widget not found"));
    }
}

void UWBP_SwordBloom::HideSwordBloom()
{
    if (SwordBloom)
    {
        SwordBloom->SetVisibility(ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Log, TEXT("WBP_SwordBloom: SwordBloom effect hidden"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Cannot hide SwordBloom - widget not found"));
    }
}

void UWBP_SwordBloom::ShowAllEffects()
{
    ShowSwordSpark();
    ShowSwordBloom();
    UE_LOG(LogTemp, Log, TEXT("WBP_SwordBloom: All sword effects shown"));
}

void UWBP_SwordBloom::HideAllEffects()
{
    HideSwordSpark();
    HideSwordBloom();
    UE_LOG(LogTemp, Log, TEXT("WBP_SwordBloom: All sword effects hidden"));
}

void UWBP_SwordBloom::SetSwordBloomScale(float Scale)
{
    if (SwordBloom)
    {
        // Clamp scale to reasonable values
        float ClampedScale = FMath::Clamp(Scale, 0.2f, 2.0f);
        
        // Set the render transform scale for the SwordBloom image
        FWidgetTransform Transform = SwordBloom->GetRenderTransform();
        Transform.Scale = FVector2D(ClampedScale, ClampedScale);
        SwordBloom->SetRenderTransform(Transform);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("WBP_SwordBloom: SwordBloom scale set to %.3f"), ClampedScale);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Cannot set scale - SwordBloom widget not found"));
    }
}

void UWBP_SwordBloom::StartBloomCircle()
{
    if (!IsValid(SwordBloom))
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_SwordBloom: SwordBloom image is null - cannot start bloom circle"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("🕐 BLOOM WINDOW OPENED at %.3f seconds"), GetWorld()->GetTimeSeconds());
    UE_LOG(LogTemp, Warning, TEXT("🕐 ⏰ Window will CLOSE at %.3f seconds (in 0.8 seconds)"), GetWorld()->GetTimeSeconds() + 0.8f);
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Started bloom shrinking animation (1.0 to 0.2 over 0.8 seconds)"));
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Spark available starting at %.0f%% progress (%.2f seconds into animation)"), CriticalTimingThreshold * 100.0f, 0.8f * CriticalTimingThreshold);

    // Reset animation state
    CurrentBloomScale = 1.0f;
    BloomScaleProgress = 0.0f;
    bBloomWindowActive = true;

    // Set initial scale and visibility
    SetSwordBloomScale(CurrentBloomScale);
    ShowSwordBloom();

    // Start scaling animation timer (60 FPS for smooth animation)
    GetWorld()->GetTimerManager().SetTimer(BloomScaleTimer, this, &UWBP_SwordBloom::UpdateBloomScaling, 1.0f/60.0f, true);

    // Set bloom window timeout (0.8 seconds - original fast duration)
    GetWorld()->GetTimerManager().SetTimer(BloomWindowTimer, this, &UWBP_SwordBloom::OnBloomWindowTimeout, 0.8f, false);
}

void UWBP_SwordBloom::StartBloomCircleWithDuration(float Duration)
{
    // Clamp duration to reasonable values (minimum 0.5 seconds, maximum 10 seconds)
    float ClampedDuration = FMath::Clamp(Duration, 0.5f, 10.0f);
    
    // Store the current duration for logging
    CurrentBloomDuration = ClampedDuration;
    
    UE_LOG(LogTemp, Warning, TEXT("🕐 BLOOM WINDOW OPENED at %.3f seconds"), GetWorld()->GetTimeSeconds());
    UE_LOG(LogTemp, Warning, TEXT("🕐 ⏰ Window will CLOSE at %.3f seconds (in %.2f seconds)"), GetWorld()->GetTimeSeconds() + ClampedDuration, ClampedDuration);
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Starting bloom circle with CUSTOM duration: %.2f seconds"), ClampedDuration);
    
    // Show the bloom circle
    ShowSwordBloom();
    
    // Initialize scaling animation
    CurrentBloomScale = 1.0f;
    BloomScaleProgress = 0.0f;
    
    // Set bloom window as active for the custom duration
    bBloomWindowActive = true;
    
    // Clear any existing timers
    GetWorld()->GetTimerManager().ClearTimer(BloomWindowTimer);
    GetWorld()->GetTimerManager().ClearTimer(BloomScaleTimer);
    
    // Calculate update rate based on duration (still aim for 60 FPS, but scale progress accordingly)
    float UpdateRate = 1.0f / 60.0f; // 60 FPS
    float ProgressIncrement = UpdateRate / ClampedDuration; // How much progress per frame
    
    // Start scaling animation with custom duration
    GetWorld()->GetTimerManager().SetTimer(
        BloomScaleTimer, 
        [this, ProgressIncrement]()
        {
            // Update animation progress
            BloomScaleProgress += ProgressIncrement;
            BloomScaleProgress = FMath::Clamp(BloomScaleProgress, 0.0f, 1.0f);
            
            // Calculate scale using linear interpolation from 1.0 to 0.2
            CurrentBloomScale = FMath::Lerp(1.0f, 0.2f, BloomScaleProgress);
            
            // Apply the scale to the bloom widget
            SetSwordBloomScale(CurrentBloomScale);
            
            // Stop animation when complete
            if (BloomScaleProgress >= 1.0f)
            {
                GetWorld()->GetTimerManager().ClearTimer(BloomScaleTimer);
                UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Custom duration scaling animation completed"));
            }
        }, 
        UpdateRate, 
        true
    );
    
    // Set timer to close bloom window after custom duration
    GetWorld()->GetTimerManager().SetTimer(BloomWindowTimer, this, &UWBP_SwordBloom::OnBloomWindowTimeout, ClampedDuration, false);
    
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Started bloom shrinking animation (1.0 to 0.2 over %.2f seconds)"), ClampedDuration);
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Spark available starting at %.0f%% progress (%.2f seconds into animation)"), CriticalTimingThreshold * 100.0f, ClampedDuration * CriticalTimingThreshold);
}

bool UWBP_SwordBloom::TryTriggerSpark()
{
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: TryTriggerSpark called"));
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: DEBUG - bBloomWindowActive: %s"), bBloomWindowActive ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: DEBUG - BloomScaleProgress: %.3f (%.1f%%)"), BloomScaleProgress, BloomScaleProgress * 100.0f);
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: DEBUG - CriticalTimingThreshold: %.3f (%.1f%%)"), CriticalTimingThreshold, CriticalTimingThreshold * 100.0f);
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: DEBUG - CurrentBloomScale: %.3f"), CurrentBloomScale);
    
    if (!bBloomWindowActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Bloom window is not active - no spark triggered"));
        return false;
    }
    
    // Check if spark is already visible to prevent duplicates
    if (SwordSpark && SwordSpark->GetVisibility() == ESlateVisibility::Visible)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Spark already visible - preventing duplicate"));
        return true; // Return true since spark is already active
    }
    
    // Check if we're in the critical timing window (final 40% of animation)
    if (BloomScaleProgress >= CriticalTimingThreshold)
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: PERFECT TIMING! Spark triggered at %.1f percent progress"), BloomScaleProgress * 100.0f);
        
        // Show spark effect
        ShowSwordSpark();
        
        // DON'T hide the bloom circle - let it continue its full animation
        // HideSwordBloom(); // REMOVED - bloom circle continues
        
        // Close the bloom window to prevent further sparks, but keep animation running
        bBloomWindowActive = false;
        // DON'T clear the BloomScaleTimer - let the animation complete naturally
        // GetWorld()->GetTimerManager().ClearTimer(BloomScaleTimer); // REMOVED
        
        // Auto-hide spark after 0.5 seconds
        FTimerHandle SparkHideTimer;
        GetWorld()->GetTimerManager().SetTimer(
            SparkHideTimer,
            this,
            &UWBP_SwordBloom::HideSwordSpark,
            0.5f,
            false
        );
        
        return true;
    }
    else
    {
        float PercentageNeeded = (CriticalTimingThreshold - BloomScaleProgress) * 100.0f;
        if (BloomScaleProgress > 0.2f) // If we're getting close, provide encouraging feedback
        {
            UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Getting closer! Current: %.1f%%, need %.1f%% (%.1f%% more to go)"), 
                   BloomScaleProgress * 100.0f, CriticalTimingThreshold * 100.0f, PercentageNeeded);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Too early! Current progress: %.1f percent, need %.1f percent minimum"), 
                   BloomScaleProgress * 100.0f, CriticalTimingThreshold * 100.0f);
        }
        return false;
    }
}

void UWBP_SwordBloom::UpdateBloomScaling()
{
    if (!bBloomWindowActive)
    {
        GetWorld()->GetTimerManager().ClearTimer(BloomScaleTimer);
        return;
    }

    // Calculate progress based on 0.8 second duration (original fast timing)
    BloomScaleProgress += (1.0f / 60.0f) / 0.8f;

    if (BloomScaleProgress >= 1.0f)
    {
        BloomScaleProgress = 1.0f;
        GetWorld()->GetTimerManager().ClearTimer(BloomScaleTimer);
    }

    // Scale from 1.0 to 0.2 over the animation duration
    CurrentBloomScale = FMath::Lerp(1.0f, 0.2f, BloomScaleProgress);
    SetSwordBloomScale(CurrentBloomScale);
}

void UWBP_SwordBloom::OnBloomWindowTimeout()
{
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Bloom window timed out - closing"));
    UE_LOG(LogTemp, Warning, TEXT("🕐 ❌ BLOOM WINDOW CLOSED at %.3f seconds (timed out after %.2f seconds)"), GetWorld()->GetTimeSeconds(), CurrentBloomDuration);
    
    bBloomWindowActive = false;
    GetWorld()->GetTimerManager().ClearTimer(BloomScaleTimer);
    
    // Hide the bloom effect
    HideSwordBloom();
    
    // CRITICAL: Notify the CombatEffectsManagerComponent that bloom window has closed
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (APawn* PlayerPawn = PC->GetPawn())
        {
            if (AAtlantisEonsCharacter* Player = Cast<AAtlantisEonsCharacter>(PlayerPawn))
            {
                if (UCombatEffectsManagerComponent* CombatComp = Player->CombatEffectsManagerComp)
                {
                    CombatComp->OnBloomWindowClosed();
                    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: ✅ Notified CombatEffectsManager that bloom window closed"));
                }
            }
        }
    }
} 