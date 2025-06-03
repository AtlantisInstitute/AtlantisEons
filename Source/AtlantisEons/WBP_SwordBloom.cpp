#include "WBP_SwordBloom.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"

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
    if (SwordSpark)
    {
        // Make sure the spark appears at the same position as the bloom but at FULL scale
        if (SwordBloom)
        {
            // Copy the position from the bloom circle but use full scale for the spark
            FWidgetTransform BloomTransform = SwordBloom->GetRenderTransform();
            FWidgetTransform SparkTransform = BloomTransform; // Copy transform
            SparkTransform.Scale = FVector2D(1.0f, 1.0f); // Override scale to full size
            SwordSpark->SetRenderTransform(SparkTransform);
            
            UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: SwordSpark positioned at bloom location but at full scale"));
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
            
            UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: SwordSpark set to full scale at center (fallback)"));
        }
        
        SwordSpark->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Log, TEXT("WBP_SwordBloom: SwordSpark effect shown"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Cannot show SwordSpark - widget not found"));
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
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Starting bloom circle with timing window"));
    
    // Show the bloom circle
    ShowSwordBloom();
    
    // Initialize scaling animation
    CurrentBloomScale = 1.0f;
    BloomScaleProgress = 0.0f;
    
    // Set bloom window as active for 1 second
    bBloomWindowActive = true;
    
    // Clear any existing timers
    GetWorld()->GetTimerManager().ClearTimer(BloomWindowTimer);
    GetWorld()->GetTimerManager().ClearTimer(BloomScaleTimer);
    
    // Start scaling animation at 60 FPS
    GetWorld()->GetTimerManager().SetTimer(BloomScaleTimer, this, &UWBP_SwordBloom::UpdateBloomScaling, 1.0f / 60.0f, true);
    
    // Set timer to close bloom window after 1 second
    GetWorld()->GetTimerManager().SetTimer(BloomWindowTimer, this, &UWBP_SwordBloom::OnBloomWindowTimeout, 1.0f, false);
    
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Started bloom shrinking animation (1.0 to 0.2 over 1 second)"));
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Spark only available in FINAL 0.4 seconds (60-100 percent progress)"));
}

void UWBP_SwordBloom::StartBloomCircleWithDuration(float Duration)
{
    // Clamp duration to reasonable values (minimum 0.5 seconds, maximum 10 seconds)
    float ClampedDuration = FMath::Clamp(Duration, 0.5f, 10.0f);
    
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
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Spark only available in FINAL %.1f seconds (60-100 percent progress)"), ClampedDuration * 0.4f);
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
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Too early! Current progress: %.1f percent, need %.1f percent minimum"), 
               BloomScaleProgress * 100.0f, CriticalTimingThreshold * 100.0f);
        return false;
    }
}

void UWBP_SwordBloom::UpdateBloomScaling()
{
    // Update animation progress (0.0 to 1.0 over 1 second)
    BloomScaleProgress += (1.0f / 60.0f); // 60 FPS increment
    BloomScaleProgress = FMath::Clamp(BloomScaleProgress, 0.0f, 1.0f);
    
    // Calculate scale using linear interpolation from 1.0 to 0.2
    CurrentBloomScale = FMath::Lerp(1.0f, 0.2f, BloomScaleProgress);
    
    // Apply the scale to the bloom widget
    SetSwordBloomScale(CurrentBloomScale);
    
    // Stop animation when complete
    if (BloomScaleProgress >= 1.0f)
    {
        GetWorld()->GetTimerManager().ClearTimer(BloomScaleTimer);
        UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Scaling animation completed"));
    }
}

void UWBP_SwordBloom::OnBloomWindowTimeout()
{
    UE_LOG(LogTemp, Warning, TEXT("WBP_SwordBloom: Bloom window timed out - closing"));
    
    bBloomWindowActive = false;
    GetWorld()->GetTimerManager().ClearTimer(BloomScaleTimer);
    
    // Hide the bloom effect
    HideSwordBloom();
} 