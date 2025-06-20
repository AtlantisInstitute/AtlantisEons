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
    
    // Configure the widget component for proper 3D world rendering
    if (WidgetComponent)
    {
        // Set the widget space - Screen means it faces the camera, World means it's in 3D space
        WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
        
        // Set the draw size (how big the widget appears in the world)
        WidgetComponent->SetDrawSize(FVector2D(200.0f, 50.0f));
        
        // Set the pivot point to center
        WidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
        
        // Make sure it's visible
        WidgetComponent->SetVisibility(true);
        
        // Set collision to none since this is just a UI element
        WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    // Set default colors
    PlayerDamageColor = FLinearColor(1.0f, 0.0f, 0.0f);  // Red for damage taken by player
    EnemyDamageColor = FLinearColor(1.0f, 1.0f, 1.0f);  // White for damage dealt to enemies
    
    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Constructor completed"));
}

// Called when the game starts or when spawned
void ADamageNumberSystem::BeginPlay()
{
    Super::BeginPlay();
    
    // Try multiple methods to load the widget class if it's not already set
    if (!DamageNumberWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Widget class not set, attempting to load..."));
        
        // Method 1: Try loading via StaticLoadClass with the correct Blueprint path
        const FString WidgetBlueprintPath = TEXT("/Game/AtlantisEons/Blueprints/WBP_DamageNumber.WBP_DamageNumber_C");
        DamageNumberWidgetClass = StaticLoadClass(UDamageNumberWidget::StaticClass(), nullptr, *WidgetBlueprintPath);
        
        if (DamageNumberWidgetClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: ✅ Successfully loaded widget class via StaticLoadClass: %s"), 
                   *DamageNumberWidgetClass->GetName());
        }
        else
        {
            // Method 2: Try loading with TSoftClassPtr approach
            FSoftObjectPath WidgetPath(WidgetBlueprintPath);
            TSoftClassPtr<UDamageNumberWidget> WidgetClassPtr(WidgetPath);
            DamageNumberWidgetClass = WidgetClassPtr.LoadSynchronous();
            
            if (DamageNumberWidgetClass)
            {
                UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: ✅ Successfully loaded widget class via TSoftClassPtr: %s"), 
                       *DamageNumberWidgetClass->GetName());
            }
            else
            {
                // Method 3: Try loading the blueprint asset directly
                const FString BlueprintAssetPath = TEXT("/Game/AtlantisEons/Blueprints/WBP_DamageNumber");
                UBlueprint* WidgetBlueprint = LoadObject<UBlueprint>(nullptr, *BlueprintAssetPath);
                
                if (WidgetBlueprint && WidgetBlueprint->GeneratedClass)
                {
                    DamageNumberWidgetClass = WidgetBlueprint->GeneratedClass;
                    UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: ✅ Successfully loaded widget class via blueprint asset: %s"), 
                           *DamageNumberWidgetClass->GetName());
                }
                else
                {
                    // Method 4: Try finding the class by name in global space
                    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
                    {
                        UClass* Class = *ClassIterator;
                        if (Class && Class->GetName() == TEXT("WBP_DamageNumber_C") && Class->IsChildOf(UUserWidget::StaticClass()))
                        {
                            DamageNumberWidgetClass = Class;
                            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: ✅ Successfully found widget class by iteration: %s"), 
                                   *DamageNumberWidgetClass->GetName());
                            break;
                        }
                    }
                    
                    if (!DamageNumberWidgetClass)
                    {
                        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: ❌ FAILED to load widget class from any method! Blueprint path: %s"), 
                               *BlueprintAssetPath);
                        
                        // As a last resort, try to use the base UUserWidget class
                        DamageNumberWidgetClass = UUserWidget::StaticClass();
                        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Using fallback UUserWidget class"));
                    }
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Widget class already set: %s"), 
               *DamageNumberWidgetClass->GetName());
    }
    
    // Set up the singleton instance properly
    if (GetLifeSpan() <= 0.0f)
    {
        // This is the main damage number system actor, not a dynamically spawned one
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: This is the main system actor, setting as singleton"));
        
        // Only set as singleton if no valid instance exists
        if (!Instance || !IsValid(Instance))
        {
            Instance = this;
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Main system actor set as singleton instance"));
        }
        
        // Make sure the widget component is configured but don't initialize a widget on the main actor
        if (WidgetComponent && DamageNumberWidgetClass)
        {
            WidgetComponent->SetWidgetClass(DamageNumberWidgetClass);
            WidgetComponent->SetDrawSize(FVector2D(500.0f, 200.0f));
            WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
            WidgetComponent->SetTwoSided(true);
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Main actor widget component configured"));
        }
    }
    else
    {
        // This is a dynamically spawned damage number, initialize the widget
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: This is a temporary damage number actor, initializing widget"));
        InitWidget();
    }
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
    // CRASH FIX: Add safety check for world validity
    if (!GetWorld() || !IsValid(GetWorld()))
    {
        return;
    }
    
    // Remove any null or pending kill actors from the array
    for (int32 i = ActiveDamageNumberActors.Num() - 1; i >= 0; --i)
    {
        // CRASH FIX: Enhanced safety checks (UE5 compatible)
        AActor* Actor = ActiveDamageNumberActors[i];
        if (!Actor || !IsValid(Actor) || Actor->IsActorBeingDestroyed())
        {
            ActiveDamageNumberActors.RemoveAt(i);
        }
    }
    
    // CRASH FIX: Limit array size to prevent memory issues
    if (ActiveDamageNumberActors.Num() > 100)
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Too many active actors (%d), force cleaning oldest"), 
               ActiveDamageNumberActors.Num());
        
        // Remove oldest actors
        int32 ToRemove = ActiveDamageNumberActors.Num() - 50;
        for (int32 i = 0; i < ToRemove; ++i)
        {
            if (ActiveDamageNumberActors.IsValidIndex(i) && IsValid(ActiveDamageNumberActors[i]))
            {
                ActiveDamageNumberActors[i]->Destroy();
            }
        }
        ActiveDamageNumberActors.RemoveAt(0, ToRemove);
    }
}

void ADamageNumberSystem::DestroyDamageNumber()
{
    // CRASH FIX: Add comprehensive safety checks (UE5 compatible)
    if (!IsValid(this) || IsActorBeingDestroyed())
    {
        return;
    }
    
    // Log that we're destroying this damage number
    UE_LOG(LogTemp, Log, TEXT("DamageNumberSystem: Destroying damage number actor at location: X=%.1f, Y=%.1f, Z=%.1f"), 
           GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
    
    // Make sure we clean up timers SAFELY
    if (GetWorld() && IsValid(GetWorld()))
    {
        GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    }
    
    // Remove from the tracking array if this is in it
    if (ADamageNumberSystem::Instance && IsValid(ADamageNumberSystem::Instance))
    {
        Instance->ActiveDamageNumberActors.RemoveSingle(this);
    }
    
    // CRASH FIX: Clean up widget component safely
    if (WidgetComponent && IsValid(WidgetComponent))
    {
        UUserWidget* Widget = WidgetComponent->GetUserWidgetObject();
        if (Widget && IsValid(Widget))
        {
            Widget->RemoveFromParent();
        }
        WidgetComponent->SetWidget(nullptr);
    }
    
    // Destroy the actor
    Destroy();
}

void ADamageNumberSystem::SpawnDamageNumber(AActor* DamagedActor, float DamageAmount, bool bIsPlayerDamage)
{
    // CRASH FIX: Add safety checks to prevent system overload
    if (ActiveDamageNumberActors.Num() > 50) // Limit active damage numbers
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Too many active damage numbers (%d), skipping spawn"), 
               ActiveDamageNumberActors.Num());
        return;
    }
    
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
    
    // CRASH FIX: Check if we're being destroyed
    if (IsActorBeingDestroyed() || !IsValid(this))
    {
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: System is being destroyed, skipping spawn"));
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
        // For zombies, use a simpler approach with consistent offset to match other characters
        SpawnLocation = DamagedActor->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f); // FIXED: Consistent offset
        
        // Add a slight random offset for better visibility when multiple numbers appear
        float RandomX = FMath::RandRange(-20.0f, 20.0f);
        float RandomY = FMath::RandRange(-20.0f, 20.0f);
        SpawnLocation += FVector(RandomX, RandomY, 0.0f);
        
        UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Using consistent zombie location: X=%.1f, Y=%.1f, Z=%.1f"), 
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
        
        // CRASH FIX: Use a safer timer approach with proper cleanup
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(
                DestroyTimerHandle,
                FTimerDelegate::CreateWeakLambda(DamageNumberActor, [DamageNumberActor]() {
                    if (IsValid(DamageNumberActor) && !DamageNumberActor->IsActorBeingDestroyed()) {
                        DamageNumberActor->DestroyDamageNumber();
                    }
                }),
                DestroyDelay,
                false
            );
            
            UE_LOG(LogTemp, Warning, TEXT("DamageNumberSystem: Set destroy timer for %.1f seconds"), DestroyDelay);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DamageNumberSystem: No widget class set, cannot initialize damage number"));
        // CRASH FIX: Clean up properly
        if (IsValid(DamageNumberActor))
        {
            DamageNumberActor->Destroy();
        }
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
        // FIXED: Don't apply fixed offsets that cause all damage numbers to appear at same location
        // Instead, add small random offsets for better visibility when multiple numbers appear
        float RandomOffsetX = FMath::RandRange(-15.0f, 15.0f);
        float RandomOffsetY = FMath::RandRange(-10.0f, 10.0f);
        ScreenLocation.X += RandomOffsetX;
        ScreenLocation.Y += RandomOffsetY;
        
        // FIXED: Use proper screen coordinates (true parameter)
        DamageWidget->SetPositionInViewport(ScreenLocation, true);
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
    ActorLocation.Z += 10.0f; // FIXED: Reduced for consistency with other offsets
    
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
