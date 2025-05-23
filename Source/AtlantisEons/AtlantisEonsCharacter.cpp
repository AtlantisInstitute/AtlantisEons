// Copyright Epic Games, Inc. All Rights Reserved.

#include "AtlantisEonsCharacter.h"
#include "WBP_CharacterInfo.h"
#include "Engine/LocalPlayer.h"
#include "WBP_Main.h"
#include "ZombieCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BP_SceneCapture.h"
#include "Engine/DamageEvents.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/Vector.h"
#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "AtlantisEonsHUD.h"
#include "DamageNumberSystem.h"
#include "DamageNumberWidget.h"
#include "CollisionQueryParams.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "DashAfterimage.h"
#include "AtlantisEonsGameInstance.h"
#include "BP_ItemInfo.h"
#include "BP_Item.h"
#include "WBP_Inventory.h"
#include "BP_ConcreteItemInfo.h"
#include "WBP_ContextMenu.h"
#include "WBP_InventorySlot.h"
#include "WBP_Main.h"
#include "Blueprint/UserWidget.h"
#include "WBP_Store.h"
#include "Components/VerticalBox.h"
#include "WBP_StorePopup.h"
#include "Components/StaticMeshComponent.h"
#include "BP_Item.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AAtlantisEonsCharacter

AAtlantisEonsCharacter::AAtlantisEonsCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Initialize gold amount
    YourGold = 1000;
    Gold = YourGold; // Ensure both Gold properties start with the same value

    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 900.0f; // 1.5x increased distance for a balanced battlefield view
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->ProbeSize = 12.0f;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraLagSpeed = 15.0f;
    CameraBoom->CameraRotationLagSpeed = 10.0f;
    CameraBoom->CameraLagMaxDistance = 100.0f;
    CameraBoom->SetRelativeRotation(FRotator(-75.0f, 0.0f, 0.0f));

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Create equipment components
    Helmet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Helmet"));
    Helmet->SetupAttachment(GetMesh(), FName(TEXT("head")));
    Helmet->SetVisibility(false);

    Weapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));
    Weapon->SetupAttachment(GetMesh(), FName(TEXT("hand_r")));
    Weapon->SetVisibility(false);

    Shield = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
    Shield->SetupAttachment(GetMesh(), FName(TEXT("hand_l")));
    Shield->SetVisibility(false);

    // Don't rotate when the controller rotates
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    UCharacterMovementComponent* CharMov = GetCharacterMovement();
    if (CharMov)
    {
        CharMov->bOrientRotationToMovement = true;
        CharMov->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
        CharMov->JumpZVelocity = 700.f;
        CharMov->AirControl = 0.35f;
        CharMov->MaxWalkSpeed = 500.f;
        CharMov->MinAnalogWalkSpeed = 20.f;
        CharMov->BrakingDecelerationWalking = 2000.f;
    }

    // Initialize character stats
    BaseHealth = 100.0f;
    CurrentHealth = BaseHealth;
    BaseMP = 100.0f;
    CurrentMP = BaseMP;

    // Initialize combat stats
    BaseSTR = 10;
    BaseDEX = 10;
    BaseINT = 10;
    BaseDefence = 10;
    BaseDamage = 10;
    
    CurrentSTR = BaseSTR;
    CurrentDEX = BaseDEX;
    CurrentINT = BaseINT;
    CurrentDefence = BaseDefence;
    CurrentDamage = BaseDamage;

    // Initialize inventory
    InventoryItems.SetNum(30);
    EquipmentSlots.SetNum(10);
    InventoryToggleLock = false;
    InventoryToggleLockDuration = 0.5f;

    // Initialize combat flags
    bCanAttack = true;
    bIsAttacking = false;
    bIsInventoryOpen = false;
    AttackCooldown = 0.5f;
    DamageAmount = 20.0f;
    DamageSphereOffset = 100.0f;
    AttackRange = 200.0f;
    DefaultCameraLag = 15.0f;
    DefaultCameraRotationLag = 10.0f;
}

void AAtlantisEonsCharacter::PostInitProperties()
{
    Super::PostInitProperties();

    // Initialize attack variables
    AttackCooldown = 1.0f;
	bCanAttack = true;
	AttackCooldown = 1.0f;
}

void AAtlantisEonsCharacter::SetInventoryToggleLock(bool bLock, float UnlockDelay)
{
    UE_LOG(LogTemp, Warning, TEXT("SetInventoryToggleLock called with bLock: %s, UnlockDelay: %f"),
        bLock ? TEXT("true") : TEXT("false"), UnlockDelay);

    bool bPrevLocked = bInventoryToggleLocked;
    bInventoryToggleLocked = bLock;

    UE_LOG(LogTemp, Warning, TEXT("Inventory toggle lock changed from %s to %s"),
        bPrevLocked ? TEXT("Locked") : TEXT("Unlocked"),
        bInventoryToggleLocked ? TEXT("Locked") : TEXT("Unlocked"));

    if (UnlockDelay > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Setting up unlock timer for %f seconds"), UnlockDelay);
        GetWorld()->GetTimerManager().ClearTimer(InventoryToggleLockTimer);
        GetWorld()->GetTimerManager().SetTimer(
            InventoryToggleLockTimer,
            FTimerDelegate::CreateUObject(this, &AAtlantisEonsCharacter::SetInventoryToggleLock, false, 0.0f),
            UnlockDelay,
            false
        );
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No unlock delay, state will remain: %s"),
            bInventoryToggleLocked ? TEXT("Locked") : TEXT("Unlocked"));
    }

    // Log current inventory state
    UE_LOG(LogTemp, Warning, TEXT("Current inventory state - Open: %s, ToggleLocked: %s"),
        bIsInventoryOpen ? TEXT("true") : TEXT("false"),
        bInventoryToggleLocked ? TEXT("true") : TEXT("false"));
}

void AAtlantisEonsCharacter::ForceSetInventoryState(bool bNewIsOpen)
{
    UE_LOG(LogTemp, Warning, TEXT("ForceSetInventoryState called with bNewIsOpen: %s"), bNewIsOpen ? TEXT("true") : TEXT("false"));
    
    bool bPrevState = bIsInventoryOpen;
    bIsInventoryOpen = bNewIsOpen;
    
    UE_LOG(LogTemp, Warning, TEXT("Character inventory state changed from %s to %s"), 
        bPrevState ? TEXT("Open") : TEXT("Closed"),
        bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));

    // Log widget state
    if (InventoryWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryWidget exists, visibility: %s"), 
            InventoryWidget->IsVisible() ? TEXT("Visible") : TEXT("Hidden"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryWidget is null!"));
    }
}

void AAtlantisEonsCharacter::BeginPlay()
{
    // Call the base class  
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("%s: BeginPlay - Starting input initialization"), *GetName());

    // Initialize AI perception stimulus source
    if (AIPerceptionStimuliSourceComponent)
    {
        AIPerceptionStimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
        AIPerceptionStimuliSourceComponent->RegisterWithPerceptionSystem();
    }

    // Initialize inventory flag
    bInventoryNeedsUpdate = false;

    // CRITICAL FIX: Explicitly load the IMC_Default.uasset input mapping context if not already set
    if (!DefaultMappingContext)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: BeginPlay - DefaultMappingContext is null, attempting to load from Content"), *GetName());
        DefaultMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/AtlantisEons/Input/IMC_Default"));
        
        if (DefaultMappingContext)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: BeginPlay - Successfully loaded IMC_Default"), *GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: BeginPlay - CRITICAL: Failed to load IMC_Default, input will not work!"), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: BeginPlay - Using existing DefaultMappingContext"), *GetName());
    }

    // Set up input mapping on the player controller
    APlayerController* PlayerController = Cast<APlayerController>(Controller);
    if (PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: BeginPlay - Got valid PlayerController, setting up input"), *GetName());
        
        // Clear keyboard focus from any widgets
        if (GEngine && GEngine->GameViewport)
        {
            FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
            FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
            UE_LOG(LogTemp, Warning, TEXT("%s: BeginPlay - Cleared slate keyboard focus"), *GetName());
        }
        
        // Make sure we're in game mode with cursor hidden
        FInputModeGameOnly GameOnlyMode;
        PlayerController->SetInputMode(GameOnlyMode);
        PlayerController->SetShowMouseCursor(false);
        PlayerController->FlushPressedKeys();
        
        // Force-enable input for both controller and character
        PlayerController->EnableInput(PlayerController);
        EnableInput(PlayerController);
        
        UE_LOG(LogTemp, Warning, TEXT("%s: BeginPlay - Set input mode to game only, cursor hidden, input enabled"), *GetName());
        
        // Set up Enhanced Input Subsystem
        if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PlayerController->Player))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                UE_LOG(LogTemp, Warning, TEXT("%s: BeginPlay - Got Enhanced Input Subsystem, setting up mapping context"), *GetName());
                
                // First clear any existing mappings
                Subsystem->ClearAllMappings();
                
                // Add our default mapping context
                if (DefaultMappingContext)
                {
                    Subsystem->AddMappingContext(DefaultMappingContext, 0);
                    UE_LOG(LogTemp, Warning, TEXT("%s: BeginPlay - Successfully added mapping context"), *GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("%s: BeginPlay - CRITICAL: DefaultMappingContext is null, input will not work!"), *GetName());
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("%s: BeginPlay - Failed to get Enhanced Input Subsystem"), *GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: BeginPlay - Failed to get Local Player"), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: BeginPlay - Failed to get Player Controller"), *GetName());
    }

    // Initialize UI components - disabled for now to fix input issues
    // InitializeUI();
    
    // Spawn scene capture actor
    SpawnSceneCapture();
    
    // Play background music
    UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")), 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
    
    // Add a small delay to "wake up" the input system with a simulated input
    FTimerHandle WakeupTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        WakeupTimerHandle, 
        [this]() { 
            UE_LOG(LogTemp, Warning, TEXT("%s: Input system wakeup triggered"), *GetName());
            AddMovementInput(FVector(1, 0, 0), 0.01f);
            FTimerHandle SecondWakeupTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(
                SecondWakeupTimerHandle, 
                [this]() { AddMovementInput(FVector(-1, 0, 0), 0.01f); }, 
                0.1f, 
                false
            );
        }, 
        0.5f, 
        false
    );
}

// BeginPlay is now BlueprintImplementableEvent
void AAtlantisEonsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UE_LOG(LogTemp, Warning, TEXT("Character - Setting up input bindings"));

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("Character - Successfully got Enhanced Input Component"));

        // Moving
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAtlantisEonsCharacter::Move);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Move action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Character - MoveAction is null! Movement won't work"));
        }

        // Looking
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAtlantisEonsCharacter::Look);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Look action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Character - LookAction is null! Camera control won't work"));
        }

        // Jumping
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAtlantisEonsCharacter::StopJumping);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Jump action"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Character - JumpAction is null! Jumping won't work"));
        }

        // Melee Attack
        if (MeleeAttackAction)
        {
            EnhancedInputComponent->BindAction(MeleeAttackAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::MeleeAttack);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound MeleeAttack action"));
        }

        // Pickup
        if (PickupAction)
        {
            EnhancedInputComponent->BindAction(PickupAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::OnPickupPressed);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Pickup action"));
        }
        else
        {
            // Critical warning about missing pickup action
            UE_LOG(LogTemp, Error, TEXT("Character - PickupAction is null! Pickup functionality won't work"));
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("WARNING: PickupAction not set in BP_Character!"));
        }

        // Inventory toggle
        if (InventoryAction)
        {
            EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::ToggleInventory);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Inventory action"));
        }

        // ESC key/Resume
        if (ResumeAction)
        {
            EnhancedInputComponent->BindAction(ResumeAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::CloseInventoryIfOpen);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Resume/ESC action"));
        }

        // Debug damage (for testing)
        if (DebugDamageAction)
        {
            EnhancedInputComponent->BindAction(DebugDamageAction, ETriggerEvent::Started, this, &AAtlantisEonsCharacter::DebugDamage);
            UE_LOG(LogTemp, Warning, TEXT("Character - Bound Debug Damage action"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Character - Failed to get Enhanced Input Component! No input will work"));
    }
}

void AAtlantisEonsCharacter::Move(const FInputActionValue& Value)
{
    // Input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // Find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // Get forward and right vectors
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // Add movement 
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AAtlantisEonsCharacter::Look(const FInputActionValue& Value)
{
    // Input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // Apply the input to the controller rotation
        AddControllerYawInput(LookAxisVector.X * CameraYawSensitivity);
        AddControllerPitchInput(LookAxisVector.Y * CameraPitchSensitivity);
    }
}

void AAtlantisEonsCharacter::Jump()
{
    Super::Jump();
}

void AAtlantisEonsCharacter::StopJumping()
{
    Super::StopJumping();
}

void AAtlantisEonsCharacter::InitializeUI()
{
    // DISABLED: Don't create any UI widgets at startup to ensure gameplay works first
    UE_LOG(LogTemplateCharacter, Warning, TEXT("%s: InitializeUI disabled to focus on gameplay first"), *GetName());
    
    // Don't create the WBP_Main widget as it's causing cursor issues and blocking input
    /*
    // First check if an instance of WBP_Main already exists in the viewport
    if (!Main)
    {
        // Look for an existing instance of WBP_Main before creating a new one
        UWorld* World = GetWorld();
        if (World)
        {
            bool bFoundExistingWidget = false;
            for (TObjectIterator<UWBP_Main> Itr; Itr; ++Itr)
            {
                UWBP_Main* FoundWidget = *Itr;
                if (FoundWidget && FoundWidget->IsInViewport())
                {
                    UE_LOG(LogTemplateCharacter, Warning, TEXT("%s: Found existing WBP_Main widget in viewport, using it"), *GetName());
                    Main = FoundWidget;
                    bFoundExistingWidget = true;
                    break;
                }
            }
            
            // Create a new widget only if no existing widget was found
            if (!bFoundExistingWidget)
            {
                Main = CreateWidget<UWBP_Main>(World, UWBP_Main::StaticClass());
                if (Main)
                {
                    Main->AddToViewport();
                    UE_LOG(LogTemplateCharacter, Log, TEXT("%s: Successfully created and added Main widget to viewport"), *GetName());
                }
                else
                {
                    UE_LOG(LogTemplateCharacter, Error, TEXT("%s: Failed to create Main widget"), *GetName());
                    return;
                }
            }
        }
        else
        {
            UE_LOG(LogTemplateCharacter, Error, TEXT("%s: Failed to get World reference for widget creation"), *GetName());
            return;
        }
    }

    // The CharacterInfo widget is now created and managed by WBP_Main
    // Setup circular bars if Main widget exists
    if (Main)
    {
        SetupCircularBars();
    }
    */
}

void AAtlantisEonsCharacter::SetupCircularBars()
{
    if (Main)
    {
        // Initialize HP and MP bars
        SettingCircularBar_HP();
        SettingCircularBar_MP();
    }
}

void AAtlantisEonsCharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();
}

void AAtlantisEonsCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
}

void AAtlantisEonsCharacter::SpawnSceneCapture()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to get World in SpawnSceneCapture"), *GetName());
        return;
    }

    // Check if we already have a scene capture
    if (SceneCapture)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: SceneCapture already exists, destroying old one"), *GetName());
        SceneCapture->Destroy();
        SceneCapture = nullptr;
    }

    // Spawn the scene capture actor - no longer used but kept for compatibility
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    SceneCapture = World->SpawnActor<ABP_SceneCapture>(ABP_SceneCapture::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    
    if (!SceneCapture)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to spawn SceneCapture"), *GetName());
        return;
    }

    // Dummy implementation - the actual functionality has been removed
    UE_LOG(LogTemp, Warning, TEXT("%s: Scene capture system has been disabled"), *GetName());
}

void AAtlantisEonsCharacter::RecoverHealth(int32 Amount)
{
    CurrentHealth = FMath::Min(CurrentHealth + Amount, BaseHealth);
    SettingCircularBar_HP();
}

void AAtlantisEonsCharacter::RecoverHP(int32 Amount)
{
    CurrentHealth = FMath::Min(CurrentHealth + Amount, BaseHealth);
    SettingCircularBar_HP();
}

void AAtlantisEonsCharacter::SettingCircularBar_MP()
{
    if (Main)
    {
        // Update MP bar progress
        float MPPercentage = CurrentMP / (float)BaseMP;
        if (MPBar)
        {
            MPBar->SetPercent(MPPercentage);
        }
    }
}

void AAtlantisEonsCharacter::ResetInvulnerability()
{
    // Implementation will be in Blueprint
}

FGenericTeamId AAtlantisEonsCharacter::GetGenericTeamId() const
{
    return TeamId;
}

void AAtlantisEonsCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
    TeamId = NewTeamId;
}

void AAtlantisEonsCharacter::UpdateAllStats()
{
    // Update all character stats based on equipment and status effects
    int32 TotalSTR = BaseSTR;
    int32 TotalDEX = BaseDEX;
    int32 TotalINT = BaseINT;
    int32 TotalDefence = BaseDefence;
    int32 TotalDamage = BaseDamage;
    
    // TODO: Add equipment bonuses
    
    CurrentSTR = TotalSTR;
    CurrentDEX = TotalDEX;
    CurrentINT = TotalINT;
    CurrentDefence = TotalDefence;
    CurrentDamage = TotalDamage;
}

void AAtlantisEonsCharacter::SettingStore()
{
    // Initialize store UI and data
    // TODO: Load store items and prices
}

void AAtlantisEonsCharacter::SetInventorySlot(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotWidgetRef)
{
    if (!ItemInfoRef || !InventorySlotWidgetRef) return;
    
    // Update the inventory slot widget with the item info
    InventorySlotWidgetRef->UpdateSlot(ItemInfoRef);
}

void AAtlantisEonsCharacter::OnPickupPressed()
{
    UE_LOG(LogTemp, Display, TEXT("%s: OnPickupPressed called"), *GetName());
    
    // Use a more reliable method to find items by using sphere trace
    FVector Start = GetActorLocation();
    FVector End = Start;
    float InteractionRadius = 200.0f; // Increased radius for easier item detection
    
    // Create debug sphere to visualize pickup range
    DrawDebugSphere(GetWorld(), Start, InteractionRadius, 12, FColor::Green, false, 2.0f);
    
    // Find all actors of BP_Item class within interaction range
    TArray<AActor*> FoundItems;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABP_Item::StaticClass(), FoundItems);
    
    ABP_Item* ClosestItem = nullptr;
    float ClosestDistance = InteractionRadius;
    
    for (AActor* Item : FoundItems)
    {
        float Distance = FVector::Distance(GetActorLocation(), Item->GetActorLocation());
        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestItem = Cast<ABP_Item>(Item);
        }
    }
    
    if (ClosestItem)
    {
        // Use the item's actual stack number, or default to 1 if it's 0
        int32 ActualStackNumber = FMath::Max(1, ClosestItem->StackNumber);
        
        UE_LOG(LogTemp, Display, TEXT("%s: Found closest item: %s (Index: %d, Stack: %d, Using: %d)"), 
            *GetName(), *ClosestItem->GetName(), ClosestItem->ItemIndex, ClosestItem->StackNumber, ActualStackNumber);
        
        // Add on-screen debug message to confirm item found
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
            FString::Printf(TEXT("Found item: %s"), *ClosestItem->ItemInfo.ItemName));
        
        // Pick up the item with its actual stack number
        bool bSuccess = PickingItem(ClosestItem->ItemIndex, ActualStackNumber);
        
        if (bSuccess)
        {
            UE_LOG(LogTemp, Display, TEXT("Successfully picked up item"));
            
            // Play pickup sound
            if (PickupSound)
            {
                UGameplayStatics::PlaySound2D(this, PickupSound);
            }
            else
            {
                // Use a generic sound if none is set
                USoundBase* DefaultSound = LoadObject<USoundBase>(nullptr, TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
                if (DefaultSound)
                {
                    UGameplayStatics::PlaySound2D(this, DefaultSound);
                }
            }
            
            // Add on-screen confirmation
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
                FString::Printf(TEXT("Added item to inventory")));
            
            // Destroy the item after pickup
            ClosestItem->Destroy();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to pick up item"));
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Failed to add item - inventory might be full"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: No nearby items found"), *GetName());
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("No items nearby"));
    }
}

bool AAtlantisEonsCharacter::PickingItem(int32 ItemIndex, int32 ItemStackNumber)
{
    // Ensure we have at least 1 item to add
    int32 ActualStackNumber = FMath::Max(1, ItemStackNumber);
    
    UE_LOG(LogTemp, Display, TEXT("%s: PickingItem called - ItemIndex: %d, Requested: %d, Using: %d"), 
        *GetName(), ItemIndex, ItemStackNumber, ActualStackNumber);
    
    // Get item info from data table or hardcoded data
    FStructure_ItemInfo ItemInfo;
    
    // Try to get from game instance first
    UAtlantisEonsGameInstance* GameInstance = Cast<UAtlantisEonsGameInstance>(GetGameInstance());
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Game instance not found, using hardcoded item data"), *GetName());
        ItemInfo = CreateHardcodedItemData(ItemIndex);
    }
    else
    {
        // Get from game instance
        if (!GameInstance->GetItemInfo(ItemIndex, ItemInfo))
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Item not found in game instance, using hardcoded data"), *GetName());
            ItemInfo = CreateHardcodedItemData(ItemIndex);
        }
    }
    
    // Create a new item info object directly from C++ class
    UBP_ConcreteItemInfo* NewItemInfo = NewObject<UBP_ConcreteItemInfo>(this);
    if (!NewItemInfo)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to create new item info object"), *GetName());
        return false;
    }
    
    // Copy data to the new object using CopyFromStructure since ItemInfo is a FStructure_ItemInfo
    NewItemInfo->CopyFromStructure(ItemInfo);
    
    // IMPORTANT: Set the stack number AFTER copying to ensure it doesn't get overridden
    NewItemInfo->StackNumber = ActualStackNumber;
    
    UE_LOG(LogTemp, Display, TEXT("%s: Created item info - Name: %s, Stack: %d, Valid: %s"), 
        *GetName(), *NewItemInfo->ItemName, NewItemInfo->StackNumber, NewItemInfo->bIsValid ? TEXT("true") : TEXT("false"));
    
    // Try to load the thumbnail if not already set
    if (NewItemInfo->ThumbnailBrush.GetResourceObject() == nullptr)
    {
        TArray<FString> ThumbnailPaths = {
            FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_Item_%d"), ItemIndex),
            FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_%s"), *ItemInfo.ItemName.Replace(TEXT(" "), TEXT(""))),
            TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicHealingPotion"),
            TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicManaPotion"),
            TEXT("/Engine/EditorResources/S_Actor")
        };
        
        for (const FString& Path : ThumbnailPaths)
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Trying to load thumbnail from: %s"), *GetName(), *Path);
            UTexture2D* LoadedTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *Path));
            if (LoadedTexture)
            {
                NewItemInfo->Thumbnail = LoadedTexture;
                
                FSlateBrush NewBrush;
                NewBrush.SetResourceObject(LoadedTexture);
                NewBrush.ImageSize = FVector2D(64.0f, 64.0f);
                NewBrush.DrawAs = ESlateBrushDrawType::Image;
                NewBrush.Tiling = ESlateBrushTileType::NoTile;
                NewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
                NewItemInfo->ThumbnailBrush = NewBrush;
                
                UE_LOG(LogTemp, Display, TEXT("%s: Successfully loaded thumbnail from: %s"), *GetName(), *Path);
                break;
            }
        }
    }
    
    // Add to inventory and force update the UI
    bool bAdded = AddItemToInventory(NewItemInfo);
    if (bAdded)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Successfully added item to inventory"), *GetName());
        // Force update inventory slots
        UpdateInventorySlots();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to add item to inventory"), *GetName());
    }
    
    return bAdded;
}

void AAtlantisEonsCharacter::ContextMenuUse(UWBP_InventorySlot* InventorySlot)
{
    if (!InventorySlot) return;
    
    UBP_ItemInfo* ItemInfo = InventorySlot->GetInventoryItemInfoRef();
    if (!ItemInfo) return;
    
    // Handle item use based on type
    switch (ItemInfo->ItemType)
    {
        case EItemType::Equip:
            ContextMenuUse_EquipItem(ItemInfo);
            break;
        case EItemType::Consume_HP:
        case EItemType::Consume_MP:
            ContextMenuUse_ConsumeItem(ItemInfo, InventorySlot, ItemInfo->RecoveryHP, ItemInfo->RecoveryMP, ItemInfo->ItemType);
            break;
        default:
            break;
    }
}

void AAtlantisEonsCharacter::ContextMenuThrow(UWBP_InventorySlot* InventorySlot)
{
    if (!InventorySlot) return;
    
    UBP_ItemInfo* ItemInfo = InventorySlot->GetInventoryItemInfoRef();
    if (!ItemInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("ContextMenuThrow: No item info found"));
        return;
    }
    
    // Get the item index and stack information
    int32 ItemIndex = ItemInfo->ItemIndex;
    int32 StackNumber = ItemInfo->StackNumber;
    int32 SlotIndex = InventorySlot->SlotIndex;
    
    // Remove item from inventory array
    if (SlotIndex >= 0 && SlotIndex < InventoryItems.Num())
    {
        InventoryItems[SlotIndex] = nullptr;
        InventorySlot->ClearSlot();
        
        // Update inventory display
        UpdateInventorySlots();
    }
    
    // Spawn the item in the world in front of the player
    FVector PlayerLocation = GetActorLocation();
    FVector PlayerForward = GetActorForwardVector();
    FVector SpawnLocation = PlayerLocation + (PlayerForward * 200.0f); // 2 meters in front
    SpawnLocation.Z = PlayerLocation.Z; // Keep same height
    
    FRotator SpawnRotation = GetActorRotation();
    
    // Try to spawn the item actor
    UWorld* World = GetWorld();
    if (World)
    {
        // Load the BP_Item class using the correct path
        UClass* ItemClass = LoadClass<ABP_Item>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/BP_Item.BP_Item_C"));
        if (ItemClass)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            
            ABP_Item* SpawnedItem = World->SpawnActor<ABP_Item>(ItemClass, SpawnLocation, SpawnRotation, SpawnParams);
            if (SpawnedItem)
            {
                // Initialize the spawned item with the correct data
                SpawnedItem->InitializeItem(ItemIndex, StackNumber);
                UE_LOG(LogTemp, Display, TEXT("ContextMenuThrow: Successfully spawned item %s"), *ItemInfo->ItemName);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("ContextMenuThrow: Failed to spawn item actor"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ContextMenuThrow: Failed to load BP_Item class"));
        }
    }
}

// Context Menu Event Handlers - called by inventory slots via events
void AAtlantisEonsCharacter::OnContextMenuUse(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot)
{
    // Delegate to the existing context menu use function
    ContextMenuUse(InventorySlot);
}

void AAtlantisEonsCharacter::OnContextMenuThrow(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlot)
{
    // Delegate to the existing context menu throw function
    ContextMenuThrow(InventorySlot);
}

void AAtlantisEonsCharacter::ContextMenuUse_EquipItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!ItemInfoRef) return;
    
    // Handle equipment based on slot type
    // TODO: Implement equipment logic
}

void AAtlantisEonsCharacter::ContextMenuUse_ConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef,
    int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (!ItemInfoRef || !InventorySlotRef) return;
    
    // Apply recovery effects
    if (RecoverHP > 0)
    {
        RecoverHealth(RecoverHP);
    }
    
    if (RecoverMP > 0)
    {
        this->RecoverMP(RecoverMP);
    }
    
    // Remove item from inventory array
    int32 SlotIndex = InventorySlotRef->SlotIndex;
    if (SlotIndex >= 0 && SlotIndex < InventoryItems.Num())
    {
        // Check if stackable and has multiple items
        if (ItemInfoRef->bIsStackable && ItemInfoRef->StackNumber > 1)
        {
            // Reduce stack by 1
            ItemInfoRef->StackNumber -= 1;
            InventorySlotRef->UpdateSlot(ItemInfoRef);
        }
        else
        {
            // Remove the item completely
            InventoryItems[SlotIndex] = nullptr;
            InventorySlotRef->ClearSlot();
        }
        
        // Update inventory display
        UpdateInventorySlots();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ContextMenuUse_ConsumeItem: Invalid slot index %d"), SlotIndex);
    }
}

bool AAtlantisEonsCharacter::BuyingItem(int32 ItemIndex, int32 ItemStackNumber, int32 ItemPrice)
{
    // Check if player has enough gold
    int32 TotalCost = ItemPrice * ItemStackNumber;
    if (YourGold < TotalCost) return false;
    
    // Try to add item to inventory
    if (PickingItem(ItemIndex, ItemStackNumber))
    {
        // Deduct gold on successful purchase - keep both Gold properties synchronized
        YourGold -= TotalCost;
        Gold = YourGold; // Keep Gold synchronized with YourGold
        return true;
    }
    
    return false;
}

void AAtlantisEonsCharacter::EquipItemInSlot(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, const TSoftObjectPtr<UTexture2D>& Texture2D, int32 ItemIndex, UMaterialInterface* MaterialInterface, UMaterialInterface* MaterialInterface2)
{
    // Find the appropriate mesh component based on slot
    UStaticMeshComponent* TargetComponent = nullptr;
    switch (ItemEquipSlot)
    {
        case EItemEquipSlot::Head:
            TargetComponent = HeadMesh;
            break;
        case EItemEquipSlot::Body:
            TargetComponent = BodyMesh;
            break;
        case EItemEquipSlot::Weapon:
            TargetComponent = WeaponMesh;
            break;
        case EItemEquipSlot::Accessory:
            TargetComponent = AccessoryMesh;
            break;
        default:
            return;
    }

    if (TargetComponent)
    {
        // Load and set the mesh
        TargetComponent->SetStaticMesh(StaticMeshID.LoadSynchronous());
        
        // Apply materials if provided
        if (MaterialInterface)
            TargetComponent->SetMaterial(0, MaterialInterface);
        if (MaterialInterface2)
            TargetComponent->SetMaterial(1, MaterialInterface2);
    }
}

void AAtlantisEonsCharacter::HandleDisarmItem(EItemEquipSlot ItemEquipSlot, const TSoftObjectPtr<UStaticMesh>& StaticMeshID, int32 ItemIndex)
{
    // Find and clear the appropriate mesh component
    UStaticMeshComponent* TargetComponent = nullptr;
    switch (ItemEquipSlot)
    {
        case EItemEquipSlot::Head:
            TargetComponent = HeadMesh;
            break;
        case EItemEquipSlot::Body:
            TargetComponent = BodyMesh;
            break;
        case EItemEquipSlot::Weapon:
            TargetComponent = WeaponMesh;
            break;
        case EItemEquipSlot::Accessory:
            TargetComponent = AccessoryMesh;
            break;
        default:
            return;
    }

    if (TargetComponent)
    {
        TargetComponent->SetStaticMesh(nullptr);
    }
}

void AAtlantisEonsCharacter::ProcessEquipItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!ItemInfoRef) return;

    // Get the item's equipment slot and mesh
    EItemEquipSlot EquipSlot = ItemInfoRef->ItemEquipSlot;
    TSoftObjectPtr<UStaticMesh> MeshID = ItemInfoRef->MeshID;
    TSoftObjectPtr<UTexture2D> Thumbnail = ItemInfoRef->Thumbnail;
    int32 ItemIndex = ItemInfoRef->ItemIndex;
    UMaterialInterface* Material1 = ItemInfoRef->Material1;
    UMaterialInterface* Material2 = ItemInfoRef->Material2;

    // Equip the item
    EquipItemInSlot(EquipSlot, MeshID, Thumbnail, ItemIndex, Material1, Material2);

    // Update character stats
    AddingCharacterStatus(ItemIndex);
    UpdateAllStats();
}

void AAtlantisEonsCharacter::ProcessConsumeItem(UBP_ItemInfo* ItemInfoRef, UWBP_InventorySlot* InventorySlotRef, int32 RecoverHP, int32 RecoverMP, EItemType ItemType)
{
    if (!ItemInfoRef || !InventorySlotRef) return;

    // Apply recovery effects
    if (ItemType == EItemType::Consume_HP)
    {
        RecoverHealth(RecoverHP);
    }
    else if (ItemType == EItemType::Consume_MP)
    {
        this->RecoverMP(RecoverMP);
    }

    // Clear the inventory slot
    InventorySlotRef->ClearSlot();
}

void AAtlantisEonsCharacter::DragAndDropExchangeItem(UBP_ItemInfo* FromInventoryItemRef, UWBP_InventorySlot* FromInventorySlotRef,
    UBP_ItemInfo* ToInventoryItemRef, UWBP_InventorySlot* ToInventorySlotRef)
{
    if (!FromInventorySlotRef || !ToInventorySlotRef)
    {
        return;
    }

    int32 FromIndex = FromInventorySlotRef->SlotIndex;
    int32 ToIndex = ToInventorySlotRef->SlotIndex;

    // Validate indices
    if (FromIndex < 0 || ToIndex < 0 || FromIndex >= InventoryItems.Num() || ToIndex >= InventoryItems.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid slot indices for drag and drop: From=%d, To=%d"), FromIndex, ToIndex);
        return;
    }

    // Handle stacking if items are the same type
    if (FromInventoryItemRef && ToInventoryItemRef &&
        FromInventoryItemRef->ItemIndex == ToInventoryItemRef->ItemIndex &&
        FromInventoryItemRef->bIsStackable)
    {
        // Add stacks together
        int32 TotalStack = FromInventoryItemRef->StackNumber + ToInventoryItemRef->StackNumber;
        
        // Cap at 99
        if (TotalStack > 99)
        {
            ToInventoryItemRef->StackNumber = 99;
            FromInventoryItemRef->StackNumber = TotalStack - 99;
            
            // Update both slots
            FromInventorySlotRef->UpdateSlot(FromInventoryItemRef);
            ToInventorySlotRef->UpdateSlot(ToInventoryItemRef);
        }
        else
        {
            // Combine into destination slot
            ToInventoryItemRef->StackNumber = TotalStack;
            ToInventorySlotRef->UpdateSlot(ToInventoryItemRef);
            
            // Clear source slot
            InventoryItems[FromIndex] = nullptr;
            FromInventorySlotRef->ClearSlot();
        }
    }
    else
    {
        // Simple swap
        InventoryItems[FromIndex] = ToInventoryItemRef;
        InventoryItems[ToIndex] = FromInventoryItemRef;

        FromInventorySlotRef->UpdateSlot(ToInventoryItemRef);
        ToInventorySlotRef->UpdateSlot(FromInventoryItemRef);
    }

    // Play sound effect
    if (UGameplayStatics::GetCurrentLevelName(this) != TEXT(""))
    {
        UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")));
    }
}

void AAtlantisEonsCharacter::RecoverMP(int32 Amount)
{
    float RecoverAmount = static_cast<float>(Amount);
    CurrentMP = FMath::Clamp(CurrentMP + RecoverAmount, 0.0f, BaseMP);
    SettingCircularBar_MP();
}

void AAtlantisEonsCharacter::SettingCircularBar_HP()
{
    if (WBP_CharacterInfo)
    {
        WBP_CharacterInfo->UpdateHPBar();
    }
}

UBP_ItemInfo* AAtlantisEonsCharacter::GetInventoryItemRef() const
{
    // Create a new concrete item info instance
    UClass* ItemInfoClass = LoadClass<UBP_ConcreteItemInfo>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/BP_ConcreteItemInfo.BP_ConcreteItemInfo_C"));
    if (!ItemInfoClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to load BP_ConcreteItemInfo class"), *GetName());
        return nullptr;
    }
    
    UBP_ConcreteItemInfo* NewItemInfo = NewObject<UBP_ConcreteItemInfo>(const_cast<AAtlantisEonsCharacter*>(this), ItemInfoClass);
    if (!NewItemInfo)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to create BP_ConcreteItemInfo object"), *GetName());
        return nullptr;
    }
    
    return NewItemInfo;
}

AActor* AAtlantisEonsCharacter::FindClosestItem() const
{
    // Get all nearby items (BP_Item type actors)
    TArray<AActor*> FoundItems;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundItems);
    
    // Find the closest one within interaction range
    const float InteractionRange = 200.0f;
    AActor* ClosestItem = nullptr;
    float ClosestDistance = InteractionRange;
    
    for (AActor* Item : FoundItems)
    {
        // Skip non-interactable items (should be more specific in actual implementation)
        if (!Item->GetClass()->ImplementsInterface(UBP_ItemInterface::StaticClass()))
        {
            continue;
        }
        
        float Distance = FVector::Distance(GetActorLocation(), Item->GetActorLocation());
        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestItem = Item;
        }
    }
    
    return ClosestItem;
}

// Implementation of ForceEnableMovement
void AAtlantisEonsCharacter::ForceEnableMovement()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: ForceEnableMovement called"), *GetName());
    
    // Make sure movement component is working properly
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        // Enable movement
        MovementComp->SetMovementMode(MOVE_Walking);
        MovementComp->bOrientRotationToMovement = true;
        MovementComp->MaxWalkSpeed = BaseMovementSpeed;
        MovementComp->SetComponentTickEnabled(true);
        
        UE_LOG(LogTemp, Warning, TEXT("%s: CharacterMovement enabled, MaxWalkSpeed=%f"), 
            *GetName(), MovementComp->MaxWalkSpeed);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to get CharacterMovement component!"), *GetName());
    }
}

void AAtlantisEonsCharacter::ResetCharacterInput()
{
    UE_LOG(LogTemp, Warning, TEXT("%s: ResetCharacterInput called"), *GetName());
    
    // Make sure we have a controller
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: No PlayerController found!"), *GetName());
        return;
    }
    
    // Make sure input is enabled for both character and controller
    PC->EnableInput(PC);
    EnableInput(PC);
    
    // Set game-only input mode with no cursor
    FInputModeGameOnly GameOnlyMode;
    PC->SetInputMode(GameOnlyMode);
    PC->SetShowMouseCursor(false);
    
    // Clear keyboard focus from any widgets
    if (GEngine && GEngine->GameViewport)
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
    }
    
    // Reapply input mapping context
    if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PC->Player))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Clearing and re-adding input mapping context"), *GetName());
            
            // First clear any existing mappings
            Subsystem->ClearAllMappings();
            
            // Add our default mapping context
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
                UE_LOG(LogTemp, Warning, TEXT("%s: Successfully added mapping context"), *GetName());
            }
            else
            {
                // Try to load the default context if it's not set
                DefaultMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/AtlantisEons/Input/IMC_Default"));
                if (DefaultMappingContext)
                {
                    Subsystem->AddMappingContext(DefaultMappingContext, 0);
                    UE_LOG(LogTemp, Warning, TEXT("%s: Loaded and added IMC_Default"), *GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("%s: DefaultMappingContext is null and couldn't be loaded!"), *GetName());
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Failed to get Enhanced Input Subsystem"), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to get Local Player"), *GetName());
    }
    
    // Force movement component to be enabled
    ForceEnableMovement();
    
    // Apply a small movement to "wake up" the input system
    AddMovementInput(FVector(1, 0, 0), 0.01f);
    GetWorld()->GetTimerManager().SetTimer(
        CameraLagTimer, // Reusing an existing timer handle
        [this]() { 
            if (IsValid(this))
            {
                AddMovementInput(FVector(-1, 0, 0), 0.01f);
            }
        },
        0.1f,
        false
    );
}

void AAtlantisEonsCharacter::MeleeAttack(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("%s: MeleeAttack called"), *GetName());
    
    // Only attack if we're not already attacking and the cooldown is over
    if (!bIsAttacking && bCanAttack)
    {
        // Set attacking flag
        bIsAttacking = true;
        bCanAttack = false;
        
        // Play attack animation if montage exists
        if (MeleeAttackMontage)
        {
            PlayAnimMontage(MeleeAttackMontage);
            
            // Face nearest enemy if any
            FaceNearestEnemy();
        }
        
        // Set up attack cooldown
        GetWorld()->GetTimerManager().SetTimer(
            AttackCooldownTimer,
            FTimerDelegate::CreateUObject(this, &AAtlantisEonsCharacter::ResetAttack),
            AttackCooldown,
            false
        );
    }
}

void AAtlantisEonsCharacter::CloseInventoryImpl()
{
    if (bIsInventoryOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("CloseInventory - Restoring default mapping context"));
        
        // Get the HUD
        AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
        if (!HUD)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get HUD in CloseInventory"));
            return;
        }
        
        // Hide the inventory widget
        HUD->HideInventoryWidget();
        bIsInventoryOpen = false;
        
        // Restore all input mappings
        if (APlayerController* PC = Cast<APlayerController>(Controller))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            {
                // Clear all mappings first
                Subsystem->ClearAllMappings();
                
                // Then add back the default mapping context
                if (DefaultMappingContext)
                {
                    Subsystem->AddMappingContext(DefaultMappingContext, 0);
                }
            }
        }
        
        // Re-enable character movement
        if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
        {
            MovementComp->SetMovementMode(MOVE_Walking);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("====== INVENTORY CLOSE: CloseInventory COMPLETED ======"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get HUD"));
    }
}

void AAtlantisEonsCharacter::UpdateCharacterPreview()
{
    // Functionality removed
}

void AAtlantisEonsCharacter::ToggleInventory(const FInputActionValue& Value)
{
    // Check if toggle is locked
    if (bInventoryToggleLocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called but ignored due to toggle lock"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called. Current state: %s"), bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));
    
    // Get the HUD first
    AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
    if (!HUD)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get HUD in ToggleInventory"));
        return;
    }
    
    // Lock toggle to prevent rapid switching
    bInventoryToggleLocked = true;
    GetWorldTimerManager().SetTimer(InventoryToggleLockTimer, this, &AAtlantisEonsCharacter::UnlockInventoryToggle, 0.2f, false);
    
    // If inventory is open, close it
    if (bIsInventoryOpen)
    {
        if (HUD->IsInventoryWidgetVisible())
        {
            CloseInventory();
        }
        else
        {
            // Widget is not visible but state says it's open - fix the state
            bIsInventoryOpen = false;
        }
    }
    else
    {
        // Only try to open if it's not already visible
        if (!HUD->IsInventoryWidgetVisible())
        {
            OpenInventory();
        }
    }
}

void AAtlantisEonsCharacter::UnlockInventoryToggle()
{
    bInventoryToggleLocked = false;
}

void AAtlantisEonsCharacter::OpenInventory()
{
    // Get the HUD
    AAtlantisEonsHUD* HUD = Cast<AAtlantisEonsHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
    if (!HUD)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot open inventory - HUD is null"), *GetName());
        return;
    }

    // Show the inventory widget
    if (!HUD->ShowInventoryWidget())
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to show inventory widget"), *GetName());
        return;
    }

    // Get the main widget reference
    MainWidget = HUD->GetMainWidget();
    if (!MainWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Failed to get Main widget from HUD"), *GetName());
        return;
    }

    // Update the inventory state
    bIsInventoryOpen = true;
    
    // Force update all inventory slots immediately
    UpdateInventorySlots();
    
    // Also set up a delayed update to ensure it runs after any Blueprint logic
    ForceUpdateInventorySlotsAfterDelay();
    
    UE_LOG(LogTemp, Display, TEXT("%s: Inventory opened successfully"), *GetName());
}

void AAtlantisEonsCharacter::UpdateInventorySlots()
{
    if (!MainWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot update inventory slots - Main widget is null"), *GetName());
        return;
    }

    // Get the inventory widget from the main widget
    UWBP_Inventory* InventoryWidget = MainWidget->GetInventoryWidget();
    if (InventoryWidget)
    {
        // Get all inventory slot widgets
        TArray<UWBP_InventorySlot*> Slots = InventoryWidget->GetInventorySlotWidgets();
        
        // Update each slot with the corresponding inventory item
        for (int32 i = 0; i < Slots.Num(); ++i)
        {
            if (Slots[i])
            {
                // Set the slot index first
                Slots[i]->SetSlotIndex(i);
                
                // Bind context menu events to character handlers
                if (!Slots[i]->ContextMenuClickUse.IsBound())
                {
                    Slots[i]->ContextMenuClickUse.AddDynamic(this, &AAtlantisEonsCharacter::OnContextMenuUse);
                }
                if (!Slots[i]->ContextMenuClickThrow.IsBound())
                {
                    Slots[i]->ContextMenuClickThrow.AddDynamic(this, &AAtlantisEonsCharacter::OnContextMenuThrow);
                }
                
                // Check if we have an item for this slot
                if (i < InventoryItems.Num() && InventoryItems[i])
                {
                    Slots[i]->UpdateSlot(InventoryItems[i]);
                }
                else
                {
                    Slots[i]->ClearSlot();
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Cannot update inventory slots - Inventory widget not found"), *GetName());
    }
}

void AAtlantisEonsCharacter::DelayedUpdateInventorySlots()
{
    UpdateInventorySlots();
}

void AAtlantisEonsCharacter::ForceUpdateInventorySlotsAfterDelay()
{
    if (!bIsInventoryOpen)
    {
        return;
    }
    
    // Clear any existing timer
    GetWorldTimerManager().ClearTimer(InventoryUpdateTimer);
    
    // Set a small delay to ensure Blueprint logic completes first
    GetWorldTimerManager().SetTimer(
        InventoryUpdateTimer,
        this,
        &AAtlantisEonsCharacter::DelayedUpdateInventorySlots,
        0.1f,
        false
    );
}

void AAtlantisEonsCharacter::CloseInventory()
{
    // Use the existing implementation
    CloseInventoryImpl();
}

FStructure_ItemInfo AAtlantisEonsCharacter::CreateHardcodedItemData(int32 ItemIndex)
{
    FStructure_ItemInfo ItemInfo;
    
    switch (ItemIndex)
    {
        case 1: // Basic HP Potion
            ItemInfo.ItemIndex = 1;
            ItemInfo.ItemName = TEXT("Basic HP Potion");
            ItemInfo.ItemDescription = TEXT("Restores a small amount of HP.");
            ItemInfo.bIsValid = true;
            ItemInfo.bIsStackable = true;
            ItemInfo.StackNumber = 1;
            ItemInfo.ItemType = EItemType::Consume_HP;
            ItemInfo.ItemEquipSlot = EItemEquipSlot::None;
            ItemInfo.RecoveryHP = 30;
            ItemInfo.RecoveryMP = 0;
            ItemInfo.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicHealingPotion")));
            break;
            
        // Add more cases for other items as needed
            
        default:
            UE_LOG(LogTemp, Warning, TEXT("%s: Unknown item index %d, creating empty item"), *GetName(), ItemIndex);
            ItemInfo.bIsValid = false;
            break;
    }
    
    return ItemInfo;
}

bool AAtlantisEonsCharacter::AddItemToInventory(UBP_ItemInfo* ItemInfo)
{
    if (!ItemInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Attempted to add null item to inventory"), *GetName());
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("%s: AddItemToInventory - ItemIndex: %d, bIsValid: %s, StackNumber: %d"), 
        *GetName(), ItemInfo->ItemIndex, ItemInfo->bIsValid ? TEXT("true") : TEXT("false"), ItemInfo->StackNumber);

    // Validate item data
    if (!ItemInfo->bIsValid || ItemInfo->ItemIndex < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Invalid item data - Index: %d, Valid: %s"), 
            *GetName(), ItemInfo->ItemIndex, ItemInfo->bIsValid ? TEXT("true") : TEXT("false"));
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("%s: Item validation passed, attempting to add to inventory"), *GetName());

    // RESTORED ORIGINAL INVENTORY LOGIC
    bool bItemAdded = false;
    int32 RemainingStack = ItemInfo->StackNumber;
    const int32 MaxStackSize = 99;

    UE_LOG(LogTemp, Display, TEXT("%s: InventoryItems array size: %d"), *GetName(), InventoryItems.Num());

    // First try to stack with existing items if stackable
    if (ItemInfo->bIsStackable)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Item is stackable, checking for existing stacks"), *GetName());
        for (int32 i = 0; i < InventoryItems.Num(); ++i)
        {
            if (InventoryItems[i] && InventoryItems[i]->ItemIndex == ItemInfo->ItemIndex)
            {
                // Calculate how much we can add to this stack
                int32 SpaceInStack = MaxStackSize - InventoryItems[i]->StackNumber;
                if (SpaceInStack > 0)
                {
                    int32 AmountToAdd = FMath::Min(RemainingStack, SpaceInStack);
                    InventoryItems[i]->StackNumber += AmountToAdd;
                    RemainingStack -= AmountToAdd;
                    bItemAdded = true;

                    UE_LOG(LogTemp, Display, TEXT("%s: Added %d to existing stack at index %d, new total: %d"), 
                        *GetName(), AmountToAdd, i, InventoryItems[i]->StackNumber);

                    // Update UI for this slot
                    if (MainWidget)
                    {
                        if (UWBP_Inventory* InventoryWidget = MainWidget->GetInventoryWidget())
                        {
                            TArray<UWBP_InventorySlot*> Slots = InventoryWidget->GetInventorySlotWidgets();
                            if (Slots.IsValidIndex(i) && Slots[i])
                            {
                                Slots[i]->UpdateSlot(InventoryItems[i]);
                            }
                        }
                    }

                    if (RemainingStack <= 0)
                    {
                        break;
                    }
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Item is not stackable"), *GetName());
    }

    UE_LOG(LogTemp, Display, TEXT("%s: After stacking check - RemainingStack: %d"), *GetName(), RemainingStack);

    // If we still have items to add, find empty slots
    while (RemainingStack > 0)
    {
        bool bFoundEmptySlot = false;
        UE_LOG(LogTemp, Display, TEXT("%s: Looking for empty slot, RemainingStack: %d"), *GetName(), RemainingStack);
        
        for (int32 i = 0; i < InventoryItems.Num(); ++i)
        {
            if (!InventoryItems[i])
            {
                UE_LOG(LogTemp, Display, TEXT("%s: Found empty slot at index %d"), *GetName(), i);
                
                // Create a new item info object directly from C++ class
                UBP_ConcreteItemInfo* NewItemInfo = NewObject<UBP_ConcreteItemInfo>(this);
                if (!NewItemInfo)
                {
                    UE_LOG(LogTemp, Error, TEXT("%s: Failed to create new item info object"), *GetName());
                    continue;
                }

                // Copy data from original item using CopyFromItemInfo
                NewItemInfo->CopyFromItemInfo(ItemInfo);

                // If stackable, add up to max stack size
                if (ItemInfo->bIsStackable)
                {
                    NewItemInfo->StackNumber = FMath::Min(RemainingStack, MaxStackSize);
                    RemainingStack -= NewItemInfo->StackNumber;
                }
                else
                {
                    NewItemInfo->StackNumber = 1;
                    RemainingStack--;
                }

                // Add to inventory
                InventoryItems[i] = NewItemInfo;
                bItemAdded = true;
                bFoundEmptySlot = true;

                UE_LOG(LogTemp, Display, TEXT("%s: Added new item to slot %d with stack size %d"), 
                    *GetName(), i, NewItemInfo->StackNumber);

                // Update UI for this slot
                if (MainWidget)
                {
                    if (UWBP_Inventory* InventoryWidget = MainWidget->GetInventoryWidget())
                    {
                        TArray<UWBP_InventorySlot*> Slots = InventoryWidget->GetInventorySlotWidgets();
                        if (Slots.IsValidIndex(i) && Slots[i])
                        {
                            UE_LOG(LogTemp, Display, TEXT("%s: Updating UI slot %d"), *GetName(), i);
                            Slots[i]->UpdateSlot(NewItemInfo);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("%s: No valid UI slot found at index %d"), *GetName(), i);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("%s: InventoryWidget not found in MainWidget"), *GetName());
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("%s: MainWidget is null"), *GetName());
                }

                break;
            }
        }

        if (!bFoundEmptySlot)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: No empty slots found in inventory"), *GetName());
            break;
        }
    }

    // If we couldn't add all items, log a warning
    if (RemainingStack > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Could not add all items. %d items remaining"), *GetName(), RemainingStack);
    }

    UE_LOG(LogTemp, Display, TEXT("%s: AddItemToInventory returning: %s"), 
        *GetName(), bItemAdded ? TEXT("true") : TEXT("false"));

    return bItemAdded;
}

void AAtlantisEonsCharacter::SetMainWidget(UWBP_Main* NewWidget)
{
    if (NewWidget)
    {
        MainWidget = NewWidget;
        UE_LOG(LogTemp, Display, TEXT("%s: Main widget set successfully"), *GetName());
        
        // If inventory is currently open, update the slots immediately and with delay
        if (bIsInventoryOpen)
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Inventory is open, updating slots immediately and with delay"), *GetName());
            UpdateInventorySlots();
            ForceUpdateInventorySlotsAfterDelay();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Attempted to set null Main widget"), *GetName());
    }
}
