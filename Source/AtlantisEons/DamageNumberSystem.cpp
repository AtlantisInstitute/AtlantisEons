#include "DamageNumberSystem.h"
#include "DamageNumberWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameViewportClient.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Components/TextBlock.h"
#include "SlateCore.h"
#include "DamageNumberWidgetComponent.h"
#include "GenericTeamAgentInterface.h"
#include "DamageNumberActor.h"
#include "DamageNumberScreenManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

// Initialize static instance to nullptr
ADamageNumberSystem* ADamageNumberSystem::Instance = nullptr;

// Sets default values
ADamageNumberSystem::ADamageNumberSystem()
{
    DamageNumberWidgetClass = nullptr;
    
    // Set this actor to call Tick() every frame
    PrimaryActorTick.bCanEverTick = true;
    
    // Create the widget component
    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageNumberWidgetComponent"));
    RootComponent = WidgetComponent;
    
    // Try to find the widget class at construction time
    static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(TEXT("/Game/AtlantisEons/Blueprints/WBP_DamageNumber"));
    if (WidgetClassFinder.Succeeded())
    {
        DamageNumberWidgetClass = WidgetClassFinder.Class;
        if (DamageNumberWidgetClass && WidgetComponent)
        {
            WidgetComponent->SetWidgetClass(DamageNumberWidgetClass);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: DamageNumberWidgetClass or WidgetComponent is null! Widget will not be set."));
        }
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Found widget class in constructor"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Could not find widget class in constructor"));
    }
    
    // CRITICAL: Configure the widget component for proper 3D world rendering
    if (WidgetComponent)
    {
        WidgetComponent->SetWidgetSpace(EWidgetSpace::World);  // Use World space instead of Screen
        WidgetComponent->SetDrawSize(FVector2D(400.0f, 200.0f)); // Much bigger for better visibility while debugging
        WidgetComponent->SetVisibility(true);
        WidgetComponent->SetTwoSided(true);
        WidgetComponent->SetTranslucentSortPriority(1000); // Very high sort priority
        WidgetComponent->SetRelativeRotation(FRotator(0, 180, 0)); // Face the player
        WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // No collision
        WidgetComponent->SetWorldScale3D(FVector(2.0f, 2.0f, 2.0f)); // Double size for visibility
        // Don't set a fixed relative location in the constructor
        // This allows us to retain the world location set during actor spawn
        // Just set a small Z offset to prevent clipping
        WidgetComponent->SetRelativeLocation(FVector(0, 0, 20.0f));
        WidgetComponent->SetWindowFocusable(false);
        // DON'T set window visibility here - it's causing crashes
        // WidgetComponent->SetWindowVisibility(EWindowVisibility::Visible);
        WidgetComponent->SetCullDistance(0.0f); // Disable culling
    }
    
    // Enhanced visibility settings
    if (WidgetComponent)
    {
        WidgetComponent->SetTwoSided(true);
        WidgetComponent->SetWindowFocusable(false);
        // DON'T set window visibility here either - it's causing crashes
        // WidgetComponent->SetWindowVisibility(EWindowVisibility::Visible);
        WidgetComponent->SetCullDistance(0.0f); // Disable culling
    }
    
    // Set default values
    DamageAmount = 0.0f;
    DamageNumberOffset = FVector(0.0f, 0.0f, 100.0f); // Higher offset to be more visible
    RandomOffsetRange = 20.0f;  // Small random offset for multiple numbers
    DamageNumberLifetime = 2.0f;
    
    PlayerDamageColor = FLinearColor(1.0f, 0.0f, 0.0f);  // Red for damage the player receives
    EnemyDamageColor = FLinearColor(1.0f, 1.0f, 1.0f);  // White for damage dealt to enemies
}

// Called when the game starts or when spawned
void ADamageNumberSystem::BeginPlay()
{
    Super::BeginPlay();
    
    // Make sure the widget component is set up
    if (WidgetComponent && DamageNumberWidgetClass)
    {
        // Set the widget class
        WidgetComponent->SetWidgetClass(DamageNumberWidgetClass);
        
        // Set the draw size
        WidgetComponent->SetDrawSize(FVector2D(500.0f, 200.0f));
        
        // Make sure it's visible
        WidgetComponent->SetVisibility(true);
        
        // Set the widget space to screen
        WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
        
        // Make sure it's two-sided
        WidgetComponent->SetTwoSided(true);
        
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Widget component initialized in BeginPlay"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: DamageNumberWidgetClass or WidgetComponent is null! Widget will not be set."));
    }
    
    // IMPORTANT: Only initialize the widget if this is a dynamically spawned damage number
    // This prevents showing "0" before the actual damage amount
    if (GetLifeSpan() <= 0.0f)
    {
        // This is the main damage number system actor, not a dynamically spawned one
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: This is the main system actor, not initializing widget in BeginPlay"));
        
        // CRITICAL: Only the main system actor should set itself as the singleton
        if (!Instance || !IsValid(Instance))
        {
            Instance = this;
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Main system actor set as singleton instance"));
        }
        
        // Don't set a lifespan for the main system actor
    }
    else
    {
        // This is a dynamically spawned damage number, initialize the widget
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: This is a temporary damage number actor, initializing widget"));
        InitWidget();
    }
    
    // Try to load the widget class if it wasn't found in the constructor
    if (!DamageNumberWidgetClass)
    {
        // Try various paths to find the widget blueprint
        TArray<FString> PossiblePaths = {
            TEXT("/Game/AtlantisEons/Blueprints/WBP_DamageNumber.WBP_DamageNumber_C"),
            TEXT("/Game/UI/WBP_DamageNumber.WBP_DamageNumber_C"),
            TEXT("/Game/Blueprints/WBP_DamageNumber.WBP_DamageNumber_C"),
            TEXT("/Game/Widgets/WBP_DamageNumber.WBP_DamageNumber_C"),
            TEXT("/Game/UI/Widgets/WBP_DamageNumber.WBP_DamageNumber_C")
        };
        
        for (const FString& Path : PossiblePaths)
        {
            UClass* FoundClass = LoadClass<UDamageNumberWidget>(nullptr, *Path);
            if (FoundClass)
            {
                DamageNumberWidgetClass = FoundClass;
                UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Found widget class at runtime: %s"), *Path);
                break;
            }
        }
    }
    
    if (DamageNumberWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Initialized"));
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Has widget class %s"), *DamageNumberWidgetClass->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: No widget class set! Damage numbers will not appear."));
        
        // As a last resort, create a hardcoded reference to the widget class
        // This is a hack, but it should work for now
        FSoftClassPath WidgetClassPath(TEXT("/Game/AtlantisEons/Blueprints/WBP_DamageNumber.WBP_DamageNumber_C"));
        UClass* WidgetClass = WidgetClassPath.TryLoadClass<UDamageNumberWidget>();
        if (WidgetClass)
        {
            DamageNumberWidgetClass = WidgetClass;
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Found widget class using FSoftClassPath"));
        }
    }
    
    UDamageNumberScreenManager::Get(GetWorld())->SetDamageNumberWidgetClass(DamageNumberWidgetClass);
}

// Called every frame
void ADamageNumberSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Clean up expired damage number actors
    CleanupExpiredWidgets();
    
    if (WidgetComponent)
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC && PC->PlayerCameraManager)
        {
            FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
            FVector ToCam = CamLoc - WidgetComponent->GetComponentLocation();
            FRotator FaceCamRot = ToCam.Rotation();
            WidgetComponent->SetWorldRotation(FaceCamRot);
        }
    }
}

void ADamageNumberSystem::CleanupExpiredWidgets()
{
    // Remove any null or pending kill actors from the array
    for (int32 i = ActiveDamageNumberActors.Num() - 1; i >= 0; --i)
    {
        if (!ActiveDamageNumberActors[i] || !IsValid(ActiveDamageNumberActors[i]))
        {
            ActiveDamageNumberActors.RemoveAt(i);
        }
    }
}

void ADamageNumberSystem::DestroyDamageNumber()
{
    // Log that we're destroying this damage number
    UE_LOG(LogTemp, Log, TEXT("DamageNumberSystem: Destroying damage number actor at location: X=%.1f, Y=%.1f, Z=%.1f"), 
           GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
    
    // Make sure we clean up timers
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    }
    
    // Remove from the tracking array if this is in it
    if (ADamageNumberSystem::Instance)
    {
        Instance->ActiveDamageNumberActors.Remove(this);
    }
    
    // Destroy the actor
    Destroy();
}

void ADamageNumberSystem::SpawnDamageNumber(AActor* DamagedActor, float DamageAmount, bool bIsPlayerDamage)
{
    // Log damage information for debugging
    UE_LOG(LogTemp, Log, TEXT("Damage Number System: Spawning damage number of %.1f for %s (Player Damage: %s)"), 
           DamageAmount, 
           DamagedActor ? *DamagedActor->GetName() : TEXT("Unknown Actor"),
           bIsPlayerDamage ? TEXT("True") : TEXT("False"));
    
    // Skip if no actor or world is available
    if (!DamagedActor || !IsValid(DamagedActor) || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: Cannot spawn damage number - Invalid Actor or World"));
        return;
    }
    
    // Skip if damage amount is zero or negative
    if (DamageAmount <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Skipping damage number for zero or negative damage"));
        return;
    }
    
    // Special handling for zombies to ensure visibility
    bool bIsZombie = DamagedActor->GetClass()->GetName().Contains("Zombie");
    if (bIsZombie)
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Detected zombie actor: %s - Special handling enabled"), 
               *DamagedActor->GetName());
    }
    
    // Calculate location for the damage number with special handling for zombies
    FVector SpawnLocation;
    if (bIsZombie)
    {
        // For zombies, use a simpler approach with a fixed higher offset to ensure visibility
        SpawnLocation = DamagedActor->GetActorLocation() + FVector(0.0f, 0.0f, 150.0f);
        
        // Add a slight random offset for better visibility when multiple numbers appear
        float RandomX = FMath::RandRange(-20.0f, 20.0f);
        float RandomY = FMath::RandRange(-20.0f, 20.0f);
        SpawnLocation += FVector(RandomX, RandomY, 0.0f);
        
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Using special zombie location: X=%.1f, Y=%.1f, Z=%.1f"), 
               SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
    }
    else
    {
        // For other actors use the normal location calculation
        SpawnLocation = CalculateDamageNumberLocation(DamagedActor);
    }
    
    // Spawn the actor for the damage number
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.Owner = this;
    
    // Create a new damage number actor
    ADamageNumberSystem* DamageNumberActor = GetWorld()->SpawnActor<ADamageNumberSystem>(GetClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
    
    // Check if spawn was successful
    if (!DamageNumberActor || !IsValid(DamageNumberActor))
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: Failed to spawn damage number actor"));
        return;
    }
    
    // Add the actor to our tracking array
    ActiveDamageNumberActors.Add(DamageNumberActor);
    
    // Log successful creation
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Created damage number actor at location: X=%.1f, Y=%.1f, Z=%.1f"), 
           SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
    
    // Configure the spawned actor
    DamageNumberActor->DamageAmount = DamageAmount;
    DamageNumberActor->bIsPlayerDamage = bIsPlayerDamage;
    
    // Set the widget class on the new actor (use our class if it has one or use our default)
    if (DamageNumberWidgetClass)
    {
        DamageNumberActor->DamageNumberWidgetClass = DamageNumberWidgetClass;
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Set widget class: %s"), *DamageNumberWidgetClass->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: No DamageNumberWidgetClass set!"));
    }
    
    // Initialize the widget on the actor
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Calling InitWidget() on damage number actor"));
    DamageNumberActor->InitWidget();
    
    // Extra setup for zombie damage numbers to ensure visibility
    if (bIsZombie)
    {
        // Draw debug sphere to help visualize where the damage number should appear
        DrawDebugSphere(GetWorld(), SpawnLocation, 10.0f, 8, FColor::Yellow, false, 2.0f);
        
        // Increase the scale of zombie damage numbers for better visibility
        DamageNumberActor->SetActorScale3D(FVector(3.0f, 3.0f, 3.0f));
        
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Applied special scaling to zombie damage number"));
    }
    
    // Set timer to automatically destroy the damage number after animation
    if (DamageNumberWidgetClass)
    {
        const float DestroyDelay = 2.0f; // Slightly longer than animation duration to ensure it completes
        FTimerHandle DestroyTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(
            DestroyTimerHandle, 
            FTimerDelegate::CreateUObject(DamageNumberActor, &ADamageNumberSystem::DestroyDamageNumber),
            DestroyDelay, 
            false
        );
        
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Set destroy timer for %.1f seconds"), DestroyDelay);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: No widget class set, cannot initialize damage number"));
        DamageNumberActor->Destroy();
        ActiveDamageNumberActors.Remove(DamageNumberActor);
    }
}

void ADamageNumberSystem::SpawnDamageNumberAtLocation(AActor* DamagedActor, FVector Location, float DamageAmount, bool bIsCritical)
{
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: SpawnDamageNumberAtLocation called - Actor: %s, Damage: %.1f"), 
           DamagedActor ? *DamagedActor->GetName() : TEXT("NULL"), DamageAmount);
    
    // Enhanced validation
    if (!DamagedActor || !IsValid(DamagedActor))
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: Invalid DamagedActor"));
        return;
    }
    
    UWorld* World = DamagedActor->GetWorld();
    if (!World || !IsValid(World))
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: Invalid World from DamagedActor"));
        return;
    }
    
    // Get the game instance world as backup
    if (!World)
    {
        if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this))
        {
            World = GameInstance->GetWorld();
        }
    }
    
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: Cannot get valid world context"));
        return;
    }
    
    // Check if we're in PIE (Play In Editor) mode
    if (!World->IsGameWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Not in game world, skipping damage number spawn"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Valid world found, proceeding with spawn"));
    
    // Try to get player controller for UI context
    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: No valid PlayerController found"));
        return;
    }
    
    // Enhanced widget class validation
    if (!DamageNumberWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: DamageNumberWidgetClass is NULL!"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Creating widget with class: %s"), 
           *DamageNumberWidgetClass->GetName());
    
    // Create the damage number widget with proper world context
    UUserWidget* DamageWidget = CreateWidget<UUserWidget>(PC, DamageNumberWidgetClass);
    if (!DamageWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: Failed to create damage number widget"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Widget created successfully"));
    
    // Set damage amount and critical status via BlueprintImplementableEvent
    if (UFunction* InitFunction = DamageWidget->FindFunction(FName("InitializeDamageNumber")))
    {
        struct
        {
            float Amount;
            bool bCritical;
        } Params;
        
        Params.Amount = DamageAmount;
        Params.bCritical = bIsCritical;
        
        DamageWidget->ProcessEvent(InitFunction, &Params);
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Called InitializeDamageNumber with amount: %.1f"), DamageAmount);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: InitializeDamageNumber function not found, using fallback"));
        
        // Fallback: Try to set the damage amount directly via reflection
        if (FFloatProperty* DamageProperty = FindFProperty<FFloatProperty>(DamageWidget->GetClass(), FName("DamageAmount")))
        {
            DamageProperty->SetPropertyValue_InContainer(DamageWidget, DamageAmount);
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Set DamageAmount property directly"));
        }
    }
    
    // Add to viewport with proper Z-order
    DamageWidget->AddToViewport(1000); // High Z-order to appear on top
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Added widget to viewport"));
    
    // Convert world location to screen space for positioning
    FVector2D ScreenLocation;
    if (UGameplayStatics::ProjectWorldToScreen(PC, Location, ScreenLocation))
    {
        // Offset screen position slightly for better visibility
        ScreenLocation.X -= 25.0f; // Center horizontally
        ScreenLocation.Y -= 10.0f; // Slightly above
        
        DamageWidget->SetPositionInViewport(ScreenLocation, false);
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Positioned widget at screen location: %.1f, %.1f"), 
               ScreenLocation.X, ScreenLocation.Y);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Could not project to screen, using default position"));
    }
    
    // Set up auto-removal timer
    FTimerHandle RemovalTimer;
    World->GetTimerManager().SetTimer(
        RemovalTimer,
        [DamageWidget]()
        {
            if (IsValid(DamageWidget))
            {
                DamageWidget->RemoveFromParent();
                UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Auto-removed damage widget"));
            }
        },
        3.0f, // Remove after 3 seconds
        false
    );
    
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: ✅ Successfully spawned damage number: %.1f at location %.1f,%.1f,%.1f"), 
           DamageAmount, Location.X, Location.Y, Location.Z);
}

FVector ADamageNumberSystem::CalculateDamageNumberLocation(AActor* DamagedActor)
{
    if (!DamagedActor || !IsValid(DamagedActor))
    {
        return FVector::ZeroVector;
    }
    
    FVector ActorLocation = DamagedActor->GetActorLocation();
    
    // Try to find the head bone for characters
    ACharacter* Character = Cast<ACharacter>(DamagedActor);
    if (Character)
    {
        USkeletalMeshComponent* Mesh = Character->GetMesh();
        if (Mesh)
        {
            // Try to find the head bone
            FName HeadBoneName = "head";
            if (Mesh->GetBoneIndex(HeadBoneName) != INDEX_NONE)
            {
                FVector HeadLocation = Mesh->GetBoneLocation(HeadBoneName);
                UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Found head bone %s for actor %s"), 
                       *HeadBoneName.ToString(), *DamagedActor->GetName());
                
                // Use the head bone location
                ActorLocation = HeadLocation;
                UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Using head bone location for actor %s"), 
                       *DamagedActor->GetName());
            }
            else
            {
                // If no head bone, use the capsule height
                UCapsuleComponent* CapsuleComp = Character->GetCapsuleComponent();
                if (CapsuleComp)
                {
                    float CapsuleHalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
                    ActorLocation.Z += CapsuleHalfHeight;
                }
            }
        }
    }
    
    // Add a small fixed Z offset to ensure it's above the head
    ActorLocation.Z += 30.0f;
    
    // Log the final location
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Final location for actor %s: X=%f, Y=%f, Z=%f"), 
           *DamagedActor->GetName(), ActorLocation.X, ActorLocation.Y, ActorLocation.Z);
    
    return ActorLocation;
}

void ADamageNumberSystem::InitWidget()
{
    if (!WidgetComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: No widget component available"));
        return;
    }
    
    // Set the widget class if it's not already set
    if (DamageNumberWidgetClass)
    {
        if (WidgetComponent)
        {
            WidgetComponent->SetWidgetClass(DamageNumberWidgetClass);
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Set widget class to %s"), *DamageNumberWidgetClass->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: WidgetComponent is null! Widget will not be set."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: No widget class set"));
        return;
    }
    
    // Force create the widget
    UUserWidget* UserWidget = WidgetComponent->GetUserWidgetObject();
    if (!UserWidget)
    {
        // Create the widget
        UserWidget = CreateWidget<UUserWidget>(GetWorld(), DamageNumberWidgetClass);
        if (UserWidget)
        {
            // Set the widget on the component
            WidgetComponent->SetWidget(UserWidget);
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Created widget manually"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: Failed to create widget manually"));
            return;
        }
    }
    
    // Get the widget again after ensuring it's created
    UserWidget = WidgetComponent->GetUserWidgetObject();
    if (UserWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: UserWidget obtained successfully"));
        
        // Cast to our damage number widget class
        UDamageNumberWidget* DamageWidget = Cast<UDamageNumberWidget>(UserWidget);
        if (DamageWidget)
        {
            // FIXED: Use the correct method to initialize the damage number widget
            DamageWidget->InitializeDamageNumber(DamageAmount, bIsPlayerDamage);
            
            // CRITICAL: Ensure the widget is visible
            UserWidget->SetVisibility(ESlateVisibility::Visible);
            
            // CRITICAL: Make sure the widget component is visible
            WidgetComponent->SetVisibility(true);
            
            // CRITICAL: Make sure the actor is visible
            SetActorHiddenInGame(false);
            
            // Face the player camera
            if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
            {
                if (PC && PC->GetPawn())
                {
                    // Calculate rotation to face the player
                    FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
                    FVector MyLocation = GetActorLocation();
                    FVector Direction = PlayerLocation - MyLocation;
                    Direction.Z = 0.0f; // Keep it level
                    FRotator FacingRotation = Direction.Rotation();
                    WidgetComponent->SetWorldRotation(FacingRotation);
                    
                    // FIXED: Don't override the actor's spawn location - we're already positioned correctly
                    // Just ensure the widget component is properly positioned relative to the actor
                    WidgetComponent->SetRelativeLocation(FVector::ZeroVector);
                    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Positioned widget component at actor location without offset"));
                }
            }
            
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Successfully initialized damage widget with damage amount: %.2f"), 
                   DamageAmount);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: Failed to cast UserWidget to DamageNumberWidget"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: Failed to get or create UserWidget"));
    }
    
    // Log the final position, scale, and visibility after initialization
    FVector FinalLocation = GetActorLocation();
    FVector FinalScale = GetActorScale3D();
    bool bIsVisible = !IsHidden();
    bool bIsWidgetVisible = WidgetComponent->IsVisible();
    UE_LOG(LogTemp, Log, TEXT("DamageNumberSystem: After initialization - Position: X=%f Y=%f Z=%f, Scale: X=%f Y=%f Z=%f, Actor Visibility: %d, Widget Visibility: %d"),
           FinalLocation.X, FinalLocation.Y, FinalLocation.Z,
           FinalScale.X, FinalScale.Y, FinalScale.Z,
           bIsVisible ? 1 : 0, bIsWidgetVisible ? 1 : 0);
}

ADamageNumberSystem* ADamageNumberSystem::GetInstance(UWorld* World)
{
    // If we already have a valid instance, return it
    if (Instance && IsValid(Instance))
    {
        return Instance;
    }
    
    // Otherwise, try to find an existing damage number system in the world
    if (World)
    {
        for (TActorIterator<ADamageNumberSystem> It(World); It; ++It)
        {
            ADamageNumberSystem* FoundSystem = *It;
            // Only use actors that are not temporarily spawned damage numbers
            // (those have a limited lifespan)
            if (IsValid(FoundSystem) && FoundSystem->GetLifeSpan() <= 0.0f)
            {
                Instance = FoundSystem;
                UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Found main system instance: %s"), *Instance->GetName());
                return Instance;
            }
        }
        
        // If no main system found, spawn one
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        SpawnParams.Name = FName("MainDamageNumberSystem");
        
        Instance = World->SpawnActor<ADamageNumberSystem>(ADamageNumberSystem::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (Instance)
        {
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Created new main system instance"));
        }
    }
    
    return Instance;
}

void ADamageNumberSystem::SetDamageNumberWidgetClass(TSubclassOf<UDamageNumberWidget> WidgetClass)
{
    if (WidgetClass)
    {
        DamageNumberWidgetClass = WidgetClass;
    }
}

void ADamageNumberSystem::ShowDamage(float DamageAmount, FVector Location, bool bIsCritical)
{
    // Stub implementation: log the damage for now, to be replaced with actual UI code
    UE_LOG(LogTemp, Warning, TEXT("Damage: %f at %s, critical: %d"), DamageAmount, *Location.ToString(), bIsCritical ? 1 : 0);
}
