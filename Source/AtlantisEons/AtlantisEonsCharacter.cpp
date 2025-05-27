// Copyright Epic Games, Inc. All Rights Reserved.

#include "AtlantisEonsCharacter.h"
#include "WBP_CharacterInfo.h"
#include "Engine/LocalPlayer.h"
#include "WBP_Main.h"
#include "ZombieCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
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
#include "StoreSystemFix.h"
#include "UniversalItemLoader.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AAtlantisEonsCharacter

AAtlantisEonsCharacter::AAtlantisEonsCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Initialize gold amount - increased for testing store purchases
    YourGold = 100000000;  // 100 million gold for testing
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

    // Create equipment components with proper socket attachments
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
    // Start with base stats
    int32 TotalSTR = BaseSTR;
    int32 TotalDEX = BaseDEX;
    int32 TotalINT = BaseINT;
    int32 TotalDefence = BaseDefence;
    int32 TotalDamage = BaseDamage;
    float TotalHealth = BaseHealth;
    int32 TotalMP = BaseMP;
    
    // Add equipment bonuses from all equipped items
    for (int32 i = 0; i < EquipmentSlots.Num(); ++i)
    {
        if (EquipmentSlots[i])
        {
            bool bFound = false;
            FStructure_ItemInfo ItemData;
            EquipmentSlots[i]->GetItemTableRow(bFound, ItemData);
            
            if (bFound)
            {
                TotalSTR += ItemData.STR;
                TotalDEX += ItemData.DEX;
                TotalINT += ItemData.INT;
                TotalDefence += ItemData.Defence;
                TotalDamage += ItemData.Damage;
                TotalHealth += ItemData.HP;
                TotalMP += ItemData.MP;
                
                UE_LOG(LogTemp, Log, TEXT("Equipment bonus from '%s': STR+%d, DEX+%d, INT+%d, DEF+%d, DMG+%d, HP+%d, MP+%d"), 
                       *ItemData.ItemName, ItemData.STR, ItemData.DEX, ItemData.INT, 
                       ItemData.Defence, ItemData.Damage, ItemData.HP, ItemData.MP);
            }
        }
    }
    
    // Update current stats
    CurrentSTR = TotalSTR;
    CurrentDEX = TotalDEX;
    CurrentINT = TotalINT;
    CurrentDefence = TotalDefence;
    CurrentDamage = TotalDamage;
    
    // Update health and MP maximums
    float PreviousMaxHealth = MaxHealth;
    MaxHealth = TotalHealth;
    
    // Adjust current health proportionally if max health changed
    if (PreviousMaxHealth > 0.0f && MaxHealth != PreviousMaxHealth)
    {
        float HealthRatio = CurrentHealth / PreviousMaxHealth;
        CurrentHealth = FMath::Min(MaxHealth * HealthRatio, MaxHealth);
    }
    
    // Update MP maximum
    int32 PreviousMaxMP = MaxMP;
    MaxMP = TotalMP;
    
    // Adjust current MP proportionally if max MP changed
    if (PreviousMaxMP > 0 && MaxMP != PreviousMaxMP)
    {
        float MPRatio = static_cast<float>(CurrentMP) / static_cast<float>(PreviousMaxMP);
        CurrentMP = FMath::Min(FMath::RoundToInt(MaxMP * MPRatio), MaxMP);
    }
    
    // Clamp values to valid ranges
    CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
    CurrentMP = FMath::Clamp(CurrentMP, 0, MaxMP);
    
    // Broadcast stat changes
    OnStatsUpdated.Broadcast();
    
    // Update character info widget if it exists
    if (WBP_CharacterInfo)
    {
        WBP_CharacterInfo->UpdateAllStats();
    }
    
    UE_LOG(LogTemp, Log, TEXT("Stats updated - STR: %d, DEX: %d, INT: %d, DEF: %d, DMG: %d, HP: %.1f/%.1f, MP: %d/%d"), 
           CurrentSTR, CurrentDEX, CurrentINT, CurrentDefence, CurrentDamage, CurrentHealth, MaxHealth, CurrentMP, MaxMP);
}

void AAtlantisEonsCharacter::RefreshStatsDisplay()
{
    // Force update the character info widget display
    if (WBP_CharacterInfo)
    {
        WBP_CharacterInfo->UpdateAllStats();
    }
    
    // Update circular bars
    SettingCircularBar_HP();
    SettingCircularBar_MP();
    
    UE_LOG(LogTemp, Log, TEXT("Stats display refreshed"));
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
    
    // Get item info from data table or hardcoded data
    FStructure_ItemInfo ItemInfo;
    
    // Try to get from store system first (most reliable)
    bool bFoundItemData = UStoreSystemFix::GetItemData(ItemIndex, ItemInfo);
    
    if (!bFoundItemData)
    {
        // Try to get from game instance as fallback
        UAtlantisEonsGameInstance* GameInstance = Cast<UAtlantisEonsGameInstance>(GetGameInstance());
        if (GameInstance && GameInstance->GetItemInfo(ItemIndex, ItemInfo))
        {
            bFoundItemData = true;
        }
    }
    
    if (!bFoundItemData)
    {
        ItemInfo = CreateHardcodedItemData(ItemIndex);
    }
    
    // Create a new item info object directly from C++ class
    UBP_ConcreteItemInfo* NewItemInfo = NewObject<UBP_ConcreteItemInfo>(this);
    if (!NewItemInfo)
    {
        return false;
    }
    
    // Copy data to the new object using CopyFromStructure since ItemInfo is a FStructure_ItemInfo
    NewItemInfo->CopyFromStructure(ItemInfo);
    
    // IMPORTANT: Set the stack number AFTER copying to ensure it doesn't get overridden
    NewItemInfo->StackNumber = ActualStackNumber;
    
    // Load thumbnail using Universal Item Loader for consistency with store system
    if (NewItemInfo->ThumbnailBrush.GetResourceObject() == nullptr)
    {
        UTexture2D* LoadedTexture = UUniversalItemLoader::LoadItemTexture(ItemInfo);
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
        }
    }
    
    // Add to inventory and force update the UI
    bool bAdded = AddItemToInventory(NewItemInfo);
    if (bAdded)
    {
        UE_LOG(LogTemp, Log, TEXT("✅ Added %s to inventory"), *ItemInfo.ItemName);
        // Force update inventory slots
        UpdateInventorySlots();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ Failed to add item to inventory"));
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
    
    // Get item information from the data table
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfoRef->GetItemTableRow(bFound, ItemData);
    
    if (!bFound)
    {
        UE_LOG(LogTemp, Error, TEXT("ContextMenuUse_EquipItem: Failed to get item data for item index %d"), ItemInfoRef->ItemIndex);
        return;
    }
    
    // Verify this is an equipable item
    if (ItemData.ItemType != EItemType::Equip)
    {
        return;
    }
    
    // Debug: Log the equipment slot value from data table
    UE_LOG(LogTemp, Warning, TEXT("ContextMenuUse_EquipItem: Item '%s' has ItemEquipSlot = %d"), 
           *ItemData.ItemName, static_cast<int32>(ItemData.ItemEquipSlot));
    
    // HOTFIX: Correct equipment slots for specific items that have wrong data table values
    // Comprehensive weapon detection based on item names and known indices
    bool bIsWeapon = false;
    bool bIsShield = false;
    bool bIsHelmet = false;
    bool bIsBodyArmor = false;
    
    // Check by item index (known weapon items)
    if (ItemInfoRef->ItemIndex == 7 || ItemInfoRef->ItemIndex == 11 || ItemInfoRef->ItemIndex == 12 || 
        ItemInfoRef->ItemIndex == 13 || ItemInfoRef->ItemIndex == 14 || ItemInfoRef->ItemIndex == 15 || 
        ItemInfoRef->ItemIndex == 16)
    {
        bIsWeapon = true;
    }
    // Check by item name patterns for weapons
    else if (ItemData.ItemName.Contains(TEXT("Sword")) || ItemData.ItemName.Contains(TEXT("Axe")) || 
             ItemData.ItemName.Contains(TEXT("Pistol")) || ItemData.ItemName.Contains(TEXT("Rifle")) || 
             ItemData.ItemName.Contains(TEXT("Spike")) || ItemData.ItemName.Contains(TEXT("Laser")) ||
             ItemData.ItemName.Contains(TEXT("Blade")) || ItemData.ItemName.Contains(TEXT("Gun")) ||
             ItemData.ItemName.Contains(TEXT("Weapon")) || ItemData.ItemName.Contains(TEXT("Bow")) ||
             ItemData.ItemName.Contains(TEXT("Staff")) || ItemData.ItemName.Contains(TEXT("Wand")) ||
             ItemData.ItemName.Contains(TEXT("Hammer")) || ItemData.ItemName.Contains(TEXT("Mace")) ||
             ItemData.ItemName.Contains(TEXT("Spear")) || ItemData.ItemName.Contains(TEXT("Dagger")) ||
             ItemData.ItemName.Contains(TEXT("Katana")) || ItemData.ItemName.Contains(TEXT("Scythe")))
    {
        bIsWeapon = true;
    }
    // Check for shields/accessories
    else if (ItemInfoRef->ItemIndex == 17 || ItemInfoRef->ItemIndex == 18 ||
             ItemData.ItemName.Contains(TEXT("Shield")) || ItemData.ItemName.Contains(TEXT("Buckler")))
    {
        bIsShield = true;
    }
    // Check for helmets/head gear
    else if (ItemInfoRef->ItemIndex == 19 || ItemInfoRef->ItemIndex == 20 || ItemInfoRef->ItemIndex == 21 || ItemInfoRef->ItemIndex == 22 ||
             ItemData.ItemName.Contains(TEXT("Helmet")) || ItemData.ItemName.Contains(TEXT("Hat")) || 
             ItemData.ItemName.Contains(TEXT("Cap")) || ItemData.ItemName.Contains(TEXT("Crown")) ||
             ItemData.ItemName.Contains(TEXT("Mask")) || ItemData.ItemName.Contains(TEXT("Hood")))
    {
        bIsHelmet = true;
    }
    // Check for body armor
    else if (ItemInfoRef->ItemIndex == 23 || ItemInfoRef->ItemIndex == 24 || ItemInfoRef->ItemIndex == 25 || ItemInfoRef->ItemIndex == 26 ||
             ItemData.ItemName.Contains(TEXT("Suit")) || ItemData.ItemName.Contains(TEXT("Armor")) || 
             ItemData.ItemName.Contains(TEXT("Chestplate")) || ItemData.ItemName.Contains(TEXT("Vest")) ||
             ItemData.ItemName.Contains(TEXT("Robe")) || ItemData.ItemName.Contains(TEXT("Tunic")))
    {
        bIsBodyArmor = true;
    }
    
    // Apply the corrections
    if (bIsWeapon)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Weapon;
        UE_LOG(LogTemp, Warning, TEXT("HOTFIX: Corrected '%s' (Index: %d) to Weapon slot"), *ItemData.ItemName, ItemInfoRef->ItemIndex);
    }
    else if (bIsShield)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Accessory;
        UE_LOG(LogTemp, Warning, TEXT("HOTFIX: Corrected '%s' (Index: %d) to Accessory slot"), *ItemData.ItemName, ItemInfoRef->ItemIndex);
    }
    else if (bIsHelmet)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Head;
        UE_LOG(LogTemp, Warning, TEXT("HOTFIX: Corrected '%s' (Index: %d) to Head slot"), *ItemData.ItemName, ItemInfoRef->ItemIndex);
    }
    else if (bIsBodyArmor)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Body;
        UE_LOG(LogTemp, Warning, TEXT("HOTFIX: Corrected '%s' (Index: %d) to Body slot"), *ItemData.ItemName, ItemInfoRef->ItemIndex);
    }
    
    // Check if the item has a valid equipment slot
    if (ItemData.ItemEquipSlot == EItemEquipSlot::None)
    {
        return;
    }
    
    // FIXED: Correct equipment slot index calculation
    // Equipment slots array: [0]=Head, [1]=Body, [2]=Weapon, [3]=Accessory
    // Enum values: None=0, Head=1, Body=2, Weapon=3, Accessory=4
    int32 EquipSlotIndex = -1;
    switch (ItemData.ItemEquipSlot)
    {
        case EItemEquipSlot::Head:
            EquipSlotIndex = 0;
            break;
        case EItemEquipSlot::Body:
            EquipSlotIndex = 1;
            break;
        case EItemEquipSlot::Weapon:
            EquipSlotIndex = 2;
            break;
        case EItemEquipSlot::Accessory:
            EquipSlotIndex = 3;
            break;
        default:
            return;
    }
    
    // Check if there's already an item equipped in this slot
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num() && EquipmentSlots[EquipSlotIndex])
    {
        // Unequip the current item first - move it back to inventory
        UBP_ItemInfo* CurrentEquippedItem = EquipmentSlots[EquipSlotIndex];
        if (CurrentEquippedItem)
        {
            // Find an empty inventory slot for the unequipped item
            bool bFoundEmptySlot = false;
            for (int32 i = 0; i < InventoryItems.Num(); ++i)
            {
                if (!InventoryItems[i])
                {
                    InventoryItems[i] = CurrentEquippedItem;
                    bFoundEmptySlot = true;
                    break;
                }
            }
            
            if (!bFoundEmptySlot)
            {
                return; // No space to unequip current item
            }
            
            // Handle visual unequipping
            HandleDisarmItem(ItemData.ItemEquipSlot, CurrentEquippedItem->MeshID, CurrentEquippedItem->ItemIndex);
            
            // Clear equipment slot UI
            ClearEquipmentSlotUI(ItemData.ItemEquipSlot);
            
            // Remove stat bonuses from the unequipped item
            SubtractingCharacterStatus(CurrentEquippedItem->ItemIndex);
        }
    }
    
    // Find the item in the inventory and remove it
    bool bRemovedFromInventory = false;
    for (int32 i = 0; i < InventoryItems.Num(); ++i)
    {
        if (InventoryItems[i] == ItemInfoRef)
        {
            InventoryItems[i] = nullptr;
            bRemovedFromInventory = true;
            break;
        }
    }
    
    if (!bRemovedFromInventory)
    {
        return;
    }
    
    // Equip the new item
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num())
    {
        EquipmentSlots[EquipSlotIndex] = ItemInfoRef;
        
        // Handle visual equipping
        EquipItemInSlot(ItemData.ItemEquipSlot, ItemData.StaticMeshID, ItemData.ItemThumbnail, 
                       ItemInfoRef->ItemIndex, nullptr, nullptr);
        
        // Apply stat bonuses from the equipped item
        AddingCharacterStatus(ItemInfoRef->ItemIndex);
        
        // Update inventory display
        UpdateInventorySlots();
        
        // Update equipment slot UI
        UpdateEquipmentSlotUI(ItemData.ItemEquipSlot, ItemInfoRef);
        
        // Update character stats
        UpdateAllStats();
        
        // Force refresh the stats display
        RefreshStatsDisplay();
        
        UE_LOG(LogTemp, Log, TEXT("✅ Successfully equipped '%s' in %s slot"), 
               *ItemData.ItemName, 
               ItemData.ItemEquipSlot == EItemEquipSlot::Weapon ? TEXT("Weapon") :
               ItemData.ItemEquipSlot == EItemEquipSlot::Head ? TEXT("Head") :
               ItemData.ItemEquipSlot == EItemEquipSlot::Body ? TEXT("Body") : TEXT("Accessory"));
    }
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
    FString SlotName;
    
    switch (ItemEquipSlot)
    {
        case EItemEquipSlot::Head:
            TargetComponent = Helmet;
            SlotName = TEXT("Head");
            break;
        case EItemEquipSlot::Body:
            TargetComponent = BodyMesh;
            SlotName = TEXT("Body");
            break;
        case EItemEquipSlot::Weapon:
            TargetComponent = Weapon;
            SlotName = TEXT("Weapon");
            break;
        case EItemEquipSlot::Accessory:
            TargetComponent = Shield;
            SlotName = TEXT("Shield/Accessory");
            break;
        default:
            UE_LOG(LogTemp, Warning, TEXT("EquipItemInSlot: Unknown equipment slot %d"), static_cast<int32>(ItemEquipSlot));
            return;
    }

    if (TargetComponent)
    {
        // Load and set the mesh
        UStaticMesh* LoadedMesh = StaticMeshID.LoadSynchronous();
        if (LoadedMesh)
        {
            TargetComponent->SetStaticMesh(LoadedMesh);
            TargetComponent->SetVisibility(true);
        }
        
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
    FString SlotName;
    
    switch (ItemEquipSlot)
    {
        case EItemEquipSlot::Head:
            TargetComponent = Helmet;
            SlotName = TEXT("Head");
            break;
        case EItemEquipSlot::Body:
            TargetComponent = BodyMesh;
            SlotName = TEXT("Body");
            break;
        case EItemEquipSlot::Weapon:
            TargetComponent = Weapon;
            SlotName = TEXT("Weapon");
            break;
        case EItemEquipSlot::Accessory:
            TargetComponent = Shield;
            SlotName = TEXT("Shield/Accessory");
            break;
        default:
            return;
    }

    if (TargetComponent)
    {
        TargetComponent->SetStaticMesh(nullptr);
        TargetComponent->SetVisibility(false);
    }
}

void AAtlantisEonsCharacter::ProcessEquipItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!ItemInfoRef) return;

    // Get item information from the data table
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfoRef->GetItemTableRow(bFound, ItemData);
    
    if (!bFound)
    {
        UE_LOG(LogTemp, Error, TEXT("ProcessEquipItem: Failed to get item data for item index %d"), ItemInfoRef->ItemIndex);
        return;
    }

    // Verify this is an equipable item
    if (ItemData.ItemType != EItemType::Equip)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessEquipItem: Item '%s' is not equipable"), *ItemData.ItemName);
        return;
    }

    // Get the item's equipment slot and mesh from the data table
    EItemEquipSlot EquipSlot = ItemData.ItemEquipSlot;
    TSoftObjectPtr<UStaticMesh> MeshID = ItemData.StaticMeshID;
    TSoftObjectPtr<UTexture2D> Thumbnail = ItemData.ItemThumbnail;
    int32 ItemIndex = ItemInfoRef->ItemIndex;
    UMaterialInterface* Material1 = nullptr; // Materials not available in data structure
    UMaterialInterface* Material2 = nullptr; // Materials not available in data structure

    // Equip the item visually
    EquipItemInSlot(EquipSlot, MeshID, Thumbnail, ItemIndex, Material1, Material2);

    // Update character stats
    AddingCharacterStatus(ItemIndex);
    UpdateAllStats();
    
    // Force refresh the stats display
    RefreshStatsDisplay();
    
    UE_LOG(LogTemp, Log, TEXT("ProcessEquipItem: Successfully processed equipment for '%s'"), *ItemData.ItemName);
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

void AAtlantisEonsCharacter::ResetAttack()
{
    bCanAttack = true;
    bIsAttacking = false;
    UE_LOG(LogTemp, Warning, TEXT("Attack cooldown reset. Can attack again."));
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
        
        // Get the player controller
        APlayerController* PC = Cast<APlayerController>(Controller);
        if (!PC)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get PlayerController in CloseInventory"));
            return;
        }
        
        // Clear keyboard focus from any widgets and reset input mode
        if (GEngine && GEngine->GameViewport)
        {
            FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
            FSlateApplication::Get().SetAllUserFocus(GEngine->GameViewport->GetGameViewportWidget());
        }
        
        // Set input mode back to game only
        FInputModeGameOnly GameOnlyMode;
        PC->SetInputMode(GameOnlyMode);
        PC->SetShowMouseCursor(false);
        PC->FlushPressedKeys();
        
        // Restore all input mappings
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            // Clear all mappings first
            Subsystem->ClearAllMappings();
            
            // Then add back the default mapping context
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
                UE_LOG(LogTemp, Warning, TEXT("CloseInventory - Restored default mapping context"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("CloseInventory - DefaultMappingContext is null!"));
            }
        }
        
        // Re-enable character movement
        if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
        {
            MovementComp->SetMovementMode(MOVE_Walking);
            MovementComp->bOrientRotationToMovement = true;
            MovementComp->MaxWalkSpeed = BaseMovementSpeed;
        }
        
        // Force enable input for both controller and character
        PC->EnableInput(PC);
        EnableInput(PC);
        
        // Reset character input to ensure everything works
        ResetCharacterInput();
        
        UE_LOG(LogTemp, Warning, TEXT("====== INVENTORY CLOSE: CloseInventory COMPLETED ======"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CloseInventory called but inventory was not open"));
    }
}

void AAtlantisEonsCharacter::ToggleInventory(const FInputActionValue& Value)
{
    // Check if toggle is locked (but allow closing even if locked for emergency situations)
    if (bInventoryToggleLocked && !bIsInventoryOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called but ignored due to toggle lock (opening blocked)"));
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
    
    // Lock toggle to prevent rapid switching (but shorter duration)
    bInventoryToggleLocked = true;
    GetWorldTimerManager().SetTimer(InventoryToggleLockTimer, this, &AAtlantisEonsCharacter::UnlockInventoryToggle, 0.1f, false);
    
    // If inventory is open, close it
    if (bIsInventoryOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: Closing inventory"));
        CloseInventory();
    }
    else
    {
        // Only try to open if it's not already visible
        if (!HUD->IsInventoryWidgetVisible())
        {
            UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: Opening inventory"));
            OpenInventory();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: Widget already visible, syncing state"));
            bIsInventoryOpen = true;
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
                
                // Set slot type to Inventory to enable context menu
                Slots[i]->SetInventorySlotType(EInventorySlotType::Inventory);
                
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

    // Validate item data
    if (!ItemInfo->bIsValid || ItemInfo->ItemIndex < 0)
    {
        return false;
    }

    // RESTORED ORIGINAL INVENTORY LOGIC
    bool bItemAdded = false;
    int32 RemainingStack = ItemInfo->StackNumber;
    const int32 MaxStackSize = 99;

    // First try to stack with existing items if stackable
    if (ItemInfo->bIsStackable)
    {
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

    // If we still have items to add, find empty slots
    while (RemainingStack > 0)
    {
        bool bFoundEmptySlot = false;
        
        for (int32 i = 0; i < InventoryItems.Num(); ++i)
        {
            if (!InventoryItems[i])
            {
                
                // Create a new item info object directly from C++ class
                UBP_ConcreteItemInfo* NewItemInfo = NewObject<UBP_ConcreteItemInfo>(this);
                if (!NewItemInfo)
                {
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

                // Update UI for this slot
                if (MainWidget)
                {
                    if (UWBP_Inventory* InventoryWidget = MainWidget->GetInventoryWidget())
                    {
                        TArray<UWBP_InventorySlot*> Slots = InventoryWidget->GetInventorySlotWidgets();
                        if (Slots.IsValidIndex(i) && Slots[i])
                        {
                            Slots[i]->UpdateSlot(NewItemInfo);
                        }
                    }
                }

                break;
            }
        }

        if (!bFoundEmptySlot)
        {
            break;
        }
    }

    return bItemAdded;
}

void AAtlantisEonsCharacter::SetMainWidget(UWBP_Main* NewWidget)
{
    if (NewWidget)
    {
        MainWidget = NewWidget;
        UE_LOG(LogTemp, Display, TEXT("%s: Main widget set successfully"), *GetName());
        
        // Get the character info widget from the main widget
        if (MainWidget->GetCharacterInfoWidget())
        {
            WBP_CharacterInfo = MainWidget->GetCharacterInfoWidget();
            UE_LOG(LogTemp, Display, TEXT("%s: Character info widget set from main widget"), *GetName());
        }
        
        // If inventory is currently open, update the slots immediately and with delay
        if (bIsInventoryOpen)
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Inventory is open, updating slots immediately and with delay"), *GetName());
            UpdateInventorySlots();
            ForceUpdateInventorySlotsAfterDelay();
        }
        
        // Initialize equipment slot references and update UI
        InitializeEquipmentSlotReferences();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Attempted to set null Main widget"), *GetName());
    }
}

void AAtlantisEonsCharacter::UpdateEquipmentSlotUI(EItemEquipSlot EquipSlot, UBP_ItemInfo* ItemInfo)
{
    UWBP_InventorySlot* TargetSlot = nullptr;
    FString SlotName;
    
    // Get the appropriate UI slot based on equipment type
    switch (EquipSlot)
    {
        case EItemEquipSlot::Head:
            TargetSlot = HeadSlot;
            SlotName = TEXT("Head");
            break;
        case EItemEquipSlot::Body:
            TargetSlot = SuitSlot;
            SlotName = TEXT("Suit");
            break;
        case EItemEquipSlot::Weapon:
            TargetSlot = WeaponSlot;
            SlotName = TEXT("Weapon");
            break;
        case EItemEquipSlot::Accessory:
            TargetSlot = CollectableSlot;
            SlotName = TEXT("Collectable");
            break;
        default:
            UE_LOG(LogTemp, Warning, TEXT("UpdateEquipmentSlotUI: Unknown equipment slot %d"), static_cast<int32>(EquipSlot));
            return;
    }
    
    if (TargetSlot)
    {
        if (ItemInfo)
        {
            // Update the slot with the equipped item
            TargetSlot->UpdateSlot(ItemInfo);
            UE_LOG(LogTemp, Log, TEXT("✅ Updated %s equipment slot UI with item"), *SlotName);
        }
        else
        {
            // Clear the slot
            TargetSlot->ClearSlot();
            UE_LOG(LogTemp, Log, TEXT("🔄 Cleared %s equipment slot UI"), *SlotName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateEquipmentSlotUI: %s slot widget is null"), *SlotName);
    }
}

void AAtlantisEonsCharacter::ClearEquipmentSlotUI(EItemEquipSlot EquipSlot)
{
    UpdateEquipmentSlotUI(EquipSlot, nullptr);
}

void AAtlantisEonsCharacter::UpdateAllEquipmentSlotsUI()
{
    // Update all equipment slots based on currently equipped items
    for (int32 i = 0; i < EquipmentSlots.Num(); ++i)
    {
        EItemEquipSlot SlotType = EItemEquipSlot::None;
        
        // Map array index to equipment slot type
        switch (i)
        {
            case 0:
                SlotType = EItemEquipSlot::Head;
                break;
            case 1:
                SlotType = EItemEquipSlot::Body;
                break;
            case 2:
                SlotType = EItemEquipSlot::Weapon;
                break;
            case 3:
                SlotType = EItemEquipSlot::Accessory;
                break;
            default:
                continue;
        }
        
        // Update the UI slot with the equipped item (or clear if none)
        UpdateEquipmentSlotUI(SlotType, EquipmentSlots[i]);
    }
    
    UE_LOG(LogTemp, Log, TEXT("🔄 Updated all equipment slot UIs"));
}

void AAtlantisEonsCharacter::InitializeEquipmentSlotReferences()
{
    // Get equipment slot references from the character info widget
    if (WBP_CharacterInfo)
    {
        HeadSlot = WBP_CharacterInfo->HeadSlot;
        WeaponSlot = WBP_CharacterInfo->WeaponSlot;
        SuitSlot = WBP_CharacterInfo->SuitSlot;
        CollectableSlot = WBP_CharacterInfo->CollectableSlot;
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Initialized equipment slot references from CharacterInfo widget"));
        UE_LOG(LogTemp, Warning, TEXT("   HeadSlot: %s"), HeadSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   WeaponSlot: %s"), WeaponSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   SuitSlot: %s"), SuitSlot ? TEXT("Valid") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("   CollectableSlot: %s"), CollectableSlot ? TEXT("Valid") : TEXT("NULL"));
        
        // Set up slot indices for equipment slots (different from inventory slots)
        if (HeadSlot) 
        {
            HeadSlot->SetSlotIndex(100); // Use high numbers to distinguish from inventory
            // Set slot type to Equipment to disable context menu
            HeadSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            // Bind click event for unequipping
            HeadSlot->OnSlotClicked.AddDynamic(this, &AAtlantisEonsCharacter::OnHeadSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   HeadSlot configured as Equipment slot"));
        }
        if (WeaponSlot) 
        {
            WeaponSlot->SetSlotIndex(101);
            // Set slot type to Equipment to disable context menu
            WeaponSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            WeaponSlot->OnSlotClicked.AddDynamic(this, &AAtlantisEonsCharacter::OnWeaponSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   WeaponSlot configured as Equipment slot"));
        }
        if (SuitSlot) 
        {
            SuitSlot->SetSlotIndex(102);
            // Set slot type to Equipment to disable context menu
            SuitSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            SuitSlot->OnSlotClicked.AddDynamic(this, &AAtlantisEonsCharacter::OnSuitSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   SuitSlot configured as Equipment slot"));
        }
        if (CollectableSlot) 
        {
            CollectableSlot->SetSlotIndex(103);
            // Set slot type to Equipment to disable context menu
            CollectableSlot->SetInventorySlotType(EInventorySlotType::Equipment);
            CollectableSlot->OnSlotClicked.AddDynamic(this, &AAtlantisEonsCharacter::OnCollectableSlotClicked);
            UE_LOG(LogTemp, Warning, TEXT("   CollectableSlot configured as Equipment slot"));
        }
        
        // Update all equipment slots after initialization
        UpdateAllEquipmentSlotsUI();
        
        // Update stats to ensure they reflect current equipment
        UpdateAllStats();
        
        // Force refresh the stats display
        RefreshStatsDisplay();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeEquipmentSlotReferences: WBP_CharacterInfo is null"));
    }
}

void AAtlantisEonsCharacter::OnEquipmentSlotClicked(EItemEquipSlot EquipSlot)
{
    // Get the equipment slot index
    int32 EquipSlotIndex = -1;
    switch (EquipSlot)
    {
        case EItemEquipSlot::Head:
            EquipSlotIndex = 0;
            break;
        case EItemEquipSlot::Body:
            EquipSlotIndex = 1;
            break;
        case EItemEquipSlot::Weapon:
            EquipSlotIndex = 2;
            break;
        case EItemEquipSlot::Accessory:
            EquipSlotIndex = 3;
            break;
        default:
            UE_LOG(LogTemp, Warning, TEXT("OnEquipmentSlotClicked: Invalid equipment slot"));
            return;
    }
    
    // Check if there's an item equipped in this slot
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num() && EquipmentSlots[EquipSlotIndex])
    {
        UBP_ItemInfo* EquippedItem = EquipmentSlots[EquipSlotIndex];
        
        // Find an empty inventory slot to move the item to
        bool bFoundEmptySlot = false;
        for (int32 i = 0; i < InventoryItems.Num(); ++i)
        {
            if (!InventoryItems[i])
            {
                // Move the item to inventory
                InventoryItems[i] = EquippedItem;
                bFoundEmptySlot = true;
                
                UE_LOG(LogTemp, Log, TEXT("✅ Moved equipped item to inventory slot %d"), i);
                break;
            }
        }
        
        if (!bFoundEmptySlot)
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ Cannot unequip item - inventory is full"));
            // Show message to player that inventory is full
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Inventory is full! Cannot unequip item."));
            }
            return;
        }
        
        // Get item data for logging and visual updates
        bool bFound = false;
        FStructure_ItemInfo ItemData;
        EquippedItem->GetItemTableRow(bFound, ItemData);
        
        // Clear the equipment slot
        EquipmentSlots[EquipSlotIndex] = nullptr;
        
        // Handle visual unequipping
        if (bFound)
        {
            HandleDisarmItem(EquipSlot, ItemData.StaticMeshID, EquippedItem->ItemIndex);
            
            // Remove stat bonuses from the unequipped item
            SubtractingCharacterStatus(EquippedItem->ItemIndex);
            
            UE_LOG(LogTemp, Log, TEXT("✅ Successfully unequipped '%s' from %s slot"), 
                   *ItemData.ItemName,
                   EquipSlot == EItemEquipSlot::Weapon ? TEXT("Weapon") :
                   EquipSlot == EItemEquipSlot::Head ? TEXT("Head") :
                   EquipSlot == EItemEquipSlot::Body ? TEXT("Body") : TEXT("Accessory"));
        }
        
        // Clear equipment slot UI
        ClearEquipmentSlotUI(EquipSlot);
        
        // Update inventory display
        UpdateInventorySlots();
        
        // Update character stats
        UpdateAllStats();
        
        // Force refresh the stats display
        RefreshStatsDisplay();
        
        // Play unequip sound
        if (UGameplayStatics::GetCurrentLevelName(this) != TEXT(""))
        {
            UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")));
        }
        
        // Show success message
        if (GEngine && bFound)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
                FString::Printf(TEXT("Unequipped %s"), *ItemData.ItemName));
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("OnEquipmentSlotClicked: No item equipped in this slot"));
    }
}

void AAtlantisEonsCharacter::OnHeadSlotClicked(int32 SlotIndex)
{
    UE_LOG(LogTemp, Log, TEXT("Head slot clicked (SlotIndex: %d)"), SlotIndex);
    OnEquipmentSlotClicked(EItemEquipSlot::Head);
}

void AAtlantisEonsCharacter::OnWeaponSlotClicked(int32 SlotIndex)
{
    UE_LOG(LogTemp, Log, TEXT("Weapon slot clicked (SlotIndex: %d)"), SlotIndex);
    OnEquipmentSlotClicked(EItemEquipSlot::Weapon);
}

void AAtlantisEonsCharacter::OnSuitSlotClicked(int32 SlotIndex)
{
    UE_LOG(LogTemp, Log, TEXT("Suit slot clicked (SlotIndex: %d)"), SlotIndex);
    OnEquipmentSlotClicked(EItemEquipSlot::Body);
}

void AAtlantisEonsCharacter::OnCollectableSlotClicked(int32 SlotIndex)
{
    UE_LOG(LogTemp, Log, TEXT("Collectable slot clicked (SlotIndex: %d)"), SlotIndex);
    OnEquipmentSlotClicked(EItemEquipSlot::Accessory);
}
