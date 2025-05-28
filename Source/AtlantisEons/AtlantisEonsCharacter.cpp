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
#include "Engine/OverlapResult.h"

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
    BaseMP = 100;
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
    // UE_LOG(LogTemp, Warning, TEXT("SetInventoryToggleLock called with bLock: %s, UnlockDelay: %f"),
    //     bLock ? TEXT("true") : TEXT("false"), UnlockDelay);

    bool bPrevLocked = bInventoryToggleLocked;
    bInventoryToggleLocked = bLock;

    // UE_LOG(LogTemp, Warning, TEXT("Inventory toggle lock changed from %s to %s"),
    //     bPrevLocked ? TEXT("Locked") : TEXT("Unlocked"),
    //     bInventoryToggleLocked ? TEXT("Locked") : TEXT("Unlocked"));

    if (UnlockDelay > 0.0f)
    {
        // UE_LOG(LogTemp, Warning, TEXT("Setting up unlock timer for %f seconds"), UnlockDelay);
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
        // UE_LOG(LogTemp, Warning, TEXT("No unlock delay, state will remain: %s"),
        //     bInventoryToggleLocked ? TEXT("Locked") : TEXT("Unlocked"));
    }

    // Log current inventory state
    // UE_LOG(LogTemp, Warning, TEXT("Current inventory state - Open: %s, ToggleLocked: %s"),
    //     bIsInventoryOpen ? TEXT("true") : TEXT("false"),
    //     bInventoryToggleLocked ? TEXT("true") : TEXT("false"));
}

void AAtlantisEonsCharacter::ForceSetInventoryState(bool bNewIsOpen)
{
    // UE_LOG(LogTemp, Warning, TEXT("ForceSetInventoryState called with bNewIsOpen: %s"), bNewIsOpen ? TEXT("true") : TEXT("false"));
    
    bool bPrevState = bIsInventoryOpen;
    bIsInventoryOpen = bNewIsOpen;
    
    // UE_LOG(LogTemp, Warning, TEXT("Character inventory state changed from %s to %s"), 
    //     bPrevState ? TEXT("Open") : TEXT("Closed"),
    //     bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));

    // Log widget state
    if (InventoryWidget)
    {
        // UE_LOG(LogTemp, Warning, TEXT("InventoryWidget exists, visibility: %s"), 
        //     InventoryWidget->IsVisible() ? TEXT("Visible") : TEXT("Hidden"));
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

    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    // Initialize team ID for AI perception
    TeamId = FGenericTeamId(1); // Player team

    // Store original materials for invulnerability effects
    StoreOriginalMaterials();

    // Initialize UI
    InitializeUI();

    // Ensure player starts without invulnerability
    bIsInvulnerable = false;
    bIsDead = false;
    
    // Initialize stats
    UpdateAllStats();
    
    UE_LOG(LogTemp, Warning, TEXT("Player BeginPlay: Invulnerable=%s, Dead=%s, Health=%.1f"), 
           bIsInvulnerable ? TEXT("Yes") : TEXT("No"),
           bIsDead ? TEXT("Yes") : TEXT("No"),
           CurrentHealth);
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
    CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
    SettingCircularBar_HP();
}

void AAtlantisEonsCharacter::RecoverHP(int32 Amount)
{
    CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
    SettingCircularBar_HP();
}

void AAtlantisEonsCharacter::ApplyDamage(float InDamageAmount)
{
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyDamage: Character is already dead, ignoring damage"));
        return;
    }

    if (InDamageAmount > 0.0f && !bIsDead)
    {
        PlayHitReactMontage();
    }

    if (CurrentHealth > 0.0f)
    {
        CurrentHealth = FMath::Max(CurrentHealth - InDamageAmount, 0.0f);
        UE_LOG(LogTemp, Warning, TEXT("💔 Player: Health reduced from %.1f to %.1f (damage: %.1f)"), 
               CurrentHealth + InDamageAmount, CurrentHealth, InDamageAmount);

        // IMPROVED: Spawn damage number with better positioning and error handling
        if (IsValid(this) && GetWorld() && InDamageAmount > 0.0f)
        {
            // Get damage number location above player's head
            FVector DamageLocation = GetActorLocation() + FVector(0, 0, 120);
            
            // Try to find damage number system
            ADamageNumberSystem* DamageSystem = nullptr;
            
            // Method 1: Try GetInstance
            DamageSystem = ADamageNumberSystem::GetInstance(GetWorld());
            
            // Method 2: Search for it if GetInstance failed
            if (!DamageSystem)
            {
                TArray<AActor*> FoundActors;
                UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADamageNumberSystem::StaticClass(), FoundActors);
                
                if (FoundActors.Num() > 0)
                {
                    DamageSystem = Cast<ADamageNumberSystem>(FoundActors[0]);
                }
            }
            
            if (DamageSystem && IsValid(DamageSystem))
            {
                // Use SpawnDamageNumberAtLocation for better positioning
                DamageSystem->SpawnDamageNumberAtLocation(this, DamageLocation, InDamageAmount, true);
                UE_LOG(LogTemp, Warning, TEXT("💥 Player: Spawned damage number for %.1f damage at %.1f,%.1f,%.1f"), 
                       InDamageAmount, DamageLocation.X, DamageLocation.Y, DamageLocation.Z);
                
                // Draw debug sphere to show where damage number spawned
                DrawDebugSphere(GetWorld(), DamageLocation, 15.0f, 8, FColor::Red, false, 5.0f, 0, 2.0f);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ Player: Could not find DamageNumberSystem for damage numbers!"));
            }
        }
        
        // Update UI with safety checks
        if (IsValid(this))
        {
            SettingCircularBar_HP();
        }
    }
    
    // Check if health is zero or less and character is not already dead
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("💀 Player: Health reached zero, calling HandleDeath. CurrentHealth: %f, bIsDead: %d"), 
            CurrentHealth, bIsDead ? 1 : 0);
        HandleDeath();
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath call completed"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("Player ApplyDamage completed: DamageAmount=%f, NewHealth=%f"), InDamageAmount, CurrentHealth);
}

void AAtlantisEonsCharacter::SettingCircularBar_HP()
{
    // Early exit if we don't have a valid controller (prevents crashes during thumbnail generation)
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_HP: No valid PlayerController"));
        return;
    }
    
    // Try multiple approaches to update the HP bar
    bool bUpdated = false;
    
    // Method 1: Use WBP_CharacterInfo if available
    if (WBP_CharacterInfo)
    {
        WBP_CharacterInfo->UpdateHPBar();
        bUpdated = true;
        UE_LOG(LogTemp, Log, TEXT("Updated HP bar via WBP_CharacterInfo"));
    }
    
    // Method 2: Use Main widget if available
    if (Main)
    {
        // Update HP bar progress
        float HPPercentage = MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f;
        if (HPBar)
        {
            HPBar->SetPercent(HPPercentage);
            bUpdated = true;
            UE_LOG(LogTemp, Log, TEXT("Updated HP bar via Main widget: %.2f%%"), HPPercentage * 100.0f);
        }
    }
    
    if (!bUpdated)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_HP: Could not update HP bar - no valid UI references"));
    }
}

void AAtlantisEonsCharacter::SettingCircularBar_MP()
{
    // Early exit if we don't have a valid controller (prevents crashes during thumbnail generation)
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_MP: No valid PlayerController"));
        return;
    }
    
    // Try multiple approaches to update the MP bar
    bool bUpdated = false;
    
    // Method 1: Use WBP_CharacterInfo if available
    if (WBP_CharacterInfo)
    {
        float MPPercentage = MaxMP > 0 ? CurrentMP / (float)MaxMP : 0.0f;
        WBP_CharacterInfo->UpdateMPBar(MPPercentage);
        bUpdated = true;
        UE_LOG(LogTemp, Log, TEXT("Updated MP bar via WBP_CharacterInfo"));
    }
    
    // Method 2: Use Main widget if available
    if (Main)
    {
        // Update MP bar progress
        float MPPercentage = MaxMP > 0 ? CurrentMP / (float)MaxMP : 0.0f;
        if (MPBar)
        {
            MPBar->SetPercent(MPPercentage);
            bUpdated = true;
            UE_LOG(LogTemp, Log, TEXT("Updated MP bar via Main widget: %.2f%%"), MPPercentage * 100.0f);
        }
    }
    
    if (!bUpdated)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("SettingCircularBar_MP: Could not update MP bar - no valid UI references"));
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
    
    // Force update circular bars to reflect new max values
    SettingCircularBar_HP();
    SettingCircularBar_MP();
    
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
    CurrentMP = FMath::Clamp(CurrentMP + Amount, 0, MaxMP);
    SettingCircularBar_MP();
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
    // Reset character input by applying a tiny movement to ensure input system is responsive
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
    UE_LOG(LogTemp, Warning, TEXT("🗡️ MeleeAttack called. Can attack: %s"), bCanAttack ? TEXT("Yes") : TEXT("No"));
    
    if (!bCanAttack)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Cannot attack - on cooldown"));
        return;
    }
    
    if (bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Cannot attack - character is dead"));
        return;
    }
    
    // Check if we have a valid mesh and animation instance
    if (!GetMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("🗡️ Character mesh is null!"));
        return;
    }
    
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("🗡️ Animation instance is null!"));
        return;
    }
    
    bCanAttack = false;
    bIsAttacking = true;
    
    FaceNearestEnemy(); // Snap to face nearest enemy
    
    // Debug montage information
    if (MeleeAttackMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ MeleeAttackMontage asset found: %s"), *MeleeAttackMontage->GetName());
        UE_LOG(LogTemp, Warning, TEXT("🗡️ MeleeAttackMontage duration: %f"), MeleeAttackMontage->GetPlayLength());
        
        // Try to play the montage
        float MontageLength = PlayAnimMontage(MeleeAttackMontage);
        
        if (MontageLength > 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Successfully playing attack montage with duration: %f"), MontageLength);
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Current animation state: IsPlaying=%d"), AnimInstance->IsAnyMontagePlaying());
            
            // CRITICAL FIX: Add a backup timer to call OnMeleeAttackNotify since animation notify may not be set up
            // This ensures damage application happens even without animation notify setup
            float NotifyTiming = MontageLength * 0.6f; // Call at 60% through the animation
            FTimerHandle AttackNotifyTimer;
            GetWorld()->GetTimerManager().SetTimer(
                AttackNotifyTimer,
                FTimerDelegate::CreateUObject(this, &AAtlantisEonsCharacter::OnMeleeAttackNotify),
                NotifyTiming,
                false
            );
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Set backup timer to call OnMeleeAttackNotify in %.2f seconds"), NotifyTiming);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("🗡️ Failed to play attack montage. Current anim mode: %d"), GetMesh()->GetAnimationMode());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🗡️ MeleeAttackMontage is null! Please assign in Blueprint."));
    }
    
    // Set timer for cooldown reset
    GetWorld()->GetTimerManager().SetTimer(
        AttackCooldownTimer,
        FTimerDelegate::CreateUObject(this, &AAtlantisEonsCharacter::ResetAttack),
        AttackCooldown,
        false
    );
}

void AAtlantisEonsCharacter::ResetAttack()
{
    bCanAttack = true;
    bIsAttacking = false;
    UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack cooldown reset. Can attack again."));
}

void AAtlantisEonsCharacter::CloseInventoryImpl()
{
    if (bIsInventoryOpen)
    {
        // UE_LOG(LogTemp, Warning, TEXT("CloseInventory - Restoring default mapping context"));
        
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
                // UE_LOG(LogTemp, Warning, TEXT("CloseInventory - Restored default mapping context"));
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
        
        // UE_LOG(LogTemp, Warning, TEXT("====== INVENTORY CLOSE: CloseInventory COMPLETED ======"));
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("CloseInventory called but inventory was not open"));
    }
}

void AAtlantisEonsCharacter::ToggleInventory(const FInputActionValue& Value)
{
    // Check if toggle is locked (but allow closing even if locked for emergency situations)
    if (bInventoryToggleLocked && !bIsInventoryOpen)
    {
        // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called but ignored due to toggle lock (opening blocked)"));
        return;
    }
    
    // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called. Current state: %s"), bIsInventoryOpen ? TEXT("Open") : TEXT("Closed"));
    
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
        // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: Closing inventory"));
        CloseInventory();
    }
    else
    {
        // Only try to open if it's not already visible
        if (!HUD->IsInventoryWidgetVisible())
        {
            // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: Opening inventory"));
            OpenInventory();
        }
        else
        {
            // UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: Widget already visible, syncing state"));
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
    
    // UE_LOG(LogTemp, Display, TEXT("%s: Inventory opened successfully"), *GetName());
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
        // UE_LOG(LogTemp, Warning, TEXT("InitializeEquipmentSlotReferences: WBP_CharacterInfo is null"));
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
        
        // Get item data for logging and visual updates
        bool bFound = false;
        FStructure_ItemInfo ItemData;
        EquippedItem->GetItemTableRow(bFound, ItemData);
        
        // Find the item in the inventory and mark it as unequipped (instead of creating duplicate)
        bool bFoundInInventory = false;
        for (int32 i = 0; i < InventoryItems.Num(); ++i)
        {
            if (InventoryItems[i] && InventoryItems[i]->ItemIndex == EquippedItem->ItemIndex && InventoryItems[i]->Equipped)
            {
                // Mark as unequipped (keeping it in inventory)
                InventoryItems[i]->Equipped = false;
                bFoundInInventory = true;
                
                UE_LOG(LogTemp, Log, TEXT("✅ Marked item in inventory slot %d as unequipped"), i);
                break;
            }
        }
        
        if (!bFoundInInventory)
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ Could not find equipped item in inventory - this shouldn't happen"));
            // Fallback: mark the equipped item as unequipped
            EquippedItem->Equipped = false;
        }
        
        // Clear the equipment slot
        EquipmentSlots[EquipSlotIndex] = nullptr;
        
        // Handle visual unequipping
        if (bFound)
        {
            HandleDisarmItem(EquipSlot, ItemData.StaticMeshID, EquippedItem->ItemIndex);
            
            // Remove stat bonuses from the unequipped item
            SubtractingCharacterStatus(EquippedItem->ItemIndex);
            
            UE_LOG(LogTemp, Log, TEXT("✅ Successfully unequipped '%s' from %s slot (keeping in inventory)"), 
                   *ItemData.ItemName,
                   EquipSlot == EItemEquipSlot::Weapon ? TEXT("Weapon") :
                   EquipSlot == EItemEquipSlot::Head ? TEXT("Head") :
                   EquipSlot == EItemEquipSlot::Body ? TEXT("Body") : TEXT("Accessory"));
        }
        
        // Clear equipment slot UI
        ClearEquipmentSlotUI(EquipSlot);
        
        // Update inventory display to remove "EQUIPPED" text
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
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
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

void AAtlantisEonsCharacter::EquipInventoryItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!ItemInfoRef) return;
    
    // Get item information from the data table
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfoRef->GetItemTableRow(bFound, ItemData);
    
    if (!bFound)
    {
        UE_LOG(LogTemp, Error, TEXT("EquipInventoryItem: Failed to get item data for item index %d"), ItemInfoRef->ItemIndex);
        return;
    }
    
    // Verify this is an equipable item
    if (ItemData.ItemType != EItemType::Equip)
    {
        return;
    }
    
    // Apply the same hotfixes for equipment slots as in the original function
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
    }
    else if (bIsShield)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Accessory;
    }
    else if (bIsHelmet)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Head;
    }
    else if (bIsBodyArmor)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Body;
    }
    
    // Check if the item has a valid equipment slot
    if (ItemData.ItemEquipSlot == EItemEquipSlot::None)
    {
        return;
    }
    
    // Calculate equipment slot index
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
        // Unequip the current item first - but keep it in inventory and mark as unequipped
        UBP_ItemInfo* CurrentEquippedItem = EquipmentSlots[EquipSlotIndex];
        if (CurrentEquippedItem)
        {
            // Mark the current item as unequipped
            CurrentEquippedItem->Equipped = false;
            
            // Handle visual unequipping
            HandleDisarmItem(ItemData.ItemEquipSlot, CurrentEquippedItem->MeshID, CurrentEquippedItem->ItemIndex);
            
            // Clear equipment slot UI
            ClearEquipmentSlotUI(ItemData.ItemEquipSlot);
            
            // Remove stat bonuses from the unequipped item
            SubtractingCharacterStatus(CurrentEquippedItem->ItemIndex);
        }
    }
    
    // Mark the item as equipped (DON'T remove from inventory)
    ItemInfoRef->Equipped = true;
    
    // Equip the new item
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num())
    {
        EquipmentSlots[EquipSlotIndex] = ItemInfoRef;
        
        // Handle visual equipping
        EquipItemInSlot(ItemData.ItemEquipSlot, ItemData.StaticMeshID, ItemData.ItemThumbnail, 
                       ItemInfoRef->ItemIndex, nullptr, nullptr);
        
        // Apply stat bonuses from the equipped item
        AddingCharacterStatus(ItemInfoRef->ItemIndex);
        
        // Update inventory display to show "Equipped" text
        UpdateInventorySlots();
        
        // Update equipment slot UI
        UpdateEquipmentSlotUI(ItemData.ItemEquipSlot, ItemInfoRef);
        
        // Update character stats
        UpdateAllStats();
        
        // Force refresh the stats display
        RefreshStatsDisplay();
        
        // Play equip sound
        if (UGameplayStatics::GetCurrentLevelName(this) != TEXT(""))
        {
            UGameplayStatics::PlaySound2D(this, LoadObject<USoundBase>(nullptr, TEXT("/Game/AtlantisEons/Sources/Sounds/S_Equip_Cue2")));
        }
        
        // Show success message
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
                FString::Printf(TEXT("Equipped %s"), *ItemData.ItemName));
        }
        
        UE_LOG(LogTemp, Log, TEXT("✅ Successfully equipped '%s' in %s slot (keeping in inventory)"), 
               *ItemData.ItemName, 
               ItemData.ItemEquipSlot == EItemEquipSlot::Weapon ? TEXT("Weapon") :
               ItemData.ItemEquipSlot == EItemEquipSlot::Head ? TEXT("Head") :
               ItemData.ItemEquipSlot == EItemEquipSlot::Body ? TEXT("Body") : TEXT("Accessory"));
    }
}

void AAtlantisEonsCharacter::UnequipInventoryItem(UBP_ItemInfo* ItemInfoRef)
{
    if (!ItemInfoRef) return;
    
    // Get item information from the data table
    bool bFound = false;
    FStructure_ItemInfo ItemData;
    ItemInfoRef->GetItemTableRow(bFound, ItemData);
    
    if (!bFound)
    {
        UE_LOG(LogTemp, Error, TEXT("UnequipInventoryItem: Failed to get item data for item index %d"), ItemInfoRef->ItemIndex);
        return;
    }
    
    // Verify this is an equipable item and is currently equipped
    if (ItemData.ItemType != EItemType::Equip || !ItemInfoRef->Equipped)
    {
        return;
    }
    
    // Apply the same hotfixes for equipment slots
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
    }
    else if (bIsShield)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Accessory;
    }
    else if (bIsHelmet)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Head;
    }
    else if (bIsBodyArmor)
    {
        ItemData.ItemEquipSlot = EItemEquipSlot::Body;
    }
    
    // Calculate equipment slot index
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
    
    // Mark the item as unequipped (but keep in inventory)
    ItemInfoRef->Equipped = false;
    
    // Clear the equipment slot
    if (EquipSlotIndex >= 0 && EquipSlotIndex < EquipmentSlots.Num())
    {
        EquipmentSlots[EquipSlotIndex] = nullptr;
        
        // Handle visual unequipping
        HandleDisarmItem(ItemData.ItemEquipSlot, ItemData.StaticMeshID, ItemInfoRef->ItemIndex);
        
        // Remove stat bonuses from the unequipped item
        SubtractingCharacterStatus(ItemInfoRef->ItemIndex);
        
        // Clear equipment slot UI
        ClearEquipmentSlotUI(ItemData.ItemEquipSlot);
        
        // Update inventory display to remove "Equipped" text
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
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
                FString::Printf(TEXT("Unequipped %s"), *ItemData.ItemName));
        }
        
        UE_LOG(LogTemp, Log, TEXT("✅ Successfully unequipped '%s' from %s slot (keeping in inventory)"), 
               *ItemData.ItemName,
               ItemData.ItemEquipSlot == EItemEquipSlot::Weapon ? TEXT("Weapon") :
               ItemData.ItemEquipSlot == EItemEquipSlot::Head ? TEXT("Head") :
               ItemData.ItemEquipSlot == EItemEquipSlot::Body ? TEXT("Body") : TEXT("Accessory"));
    }
}

void AAtlantisEonsCharacter::OnMeleeAttackNotify()
{
    UE_LOG(LogTemp, Warning, TEXT("🗡️ OnMeleeAttackNotify called - Player attacking"));
    
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Character is dead, skipping attack"));
        return;
    }
    
    // Prevent multiple calls in the same attack sequence
    if (bAttackNotifyInProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack notify already in progress, skipping duplicate call"));
        return;
    }
    
    bAttackNotifyInProgress = true;
    
    // Reset the flag after a short delay to allow for the next attack
    FTimerHandle ResetNotifyTimer;
    GetWorld()->GetTimerManager().SetTimer(
        ResetNotifyTimer,
        [this]() { 
            bAttackNotifyInProgress = false; // Reset the flag
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Attack notify flag reset - ready for next attack"));
        },
        0.5f, // Reset after 0.5 seconds
        false
    );
    
    // Calculate damage based on character stats and equipped weapon
    float BaseDamage = static_cast<float>(CurrentDamage); // Use character's current damage stat
    float WeaponDamage = 0.0f;
    
    // Check if we have a weapon equipped and get its damage bonus
    if (EquipmentSlots.Num() > 2 && EquipmentSlots[2]) // Weapon slot is index 2
    {
        UBP_ItemInfo* WeaponItem = EquipmentSlots[2];
        if (WeaponItem)
        {
            // Get weapon data from store system
            FStructure_ItemInfo WeaponData;
            if (UStoreSystemFix::GetItemData(WeaponItem->GetItemIndex(), WeaponData))
            {
                WeaponDamage = static_cast<float>(WeaponData.Damage);
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Equipped weapon '%s' adds %f damage"), 
                       *WeaponData.ItemName, WeaponDamage);
            }
        }
    }
    
    float TotalDamage = BaseDamage + WeaponDamage;
    UE_LOG(LogTemp, Warning, TEXT("🗡️ Total calculated damage: %f (Base: %f + Weapon: %f)"), 
           TotalDamage, BaseDamage, WeaponDamage);
    
    // IMPROVED ATTACK DETECTION: Use multiple detection methods for maximum reliability
    FVector StartLocation = GetActorLocation();
    FVector ForwardVector = GetActorForwardVector();
    float AttackRange = 300.0f; // Reasonable attack range
    float AttackRadius = 200.0f; // Larger radius for better detection
    
    // Track which actors we've already hit to prevent multiple hits
    TSet<AActor*> AlreadyHitActors;
    bool bSuccessfulHit = false;
    
    // METHOD 1: Direct sphere overlap at player location (most reliable)
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(AttackRadius);
    
    bool bHitDetected = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        StartLocation, // Use player location directly instead of calculated center
        FQuat::Identity,
        ECollisionChannel::ECC_Pawn,
        SphereShape
    );
    
    UE_LOG(LogTemp, Warning, TEXT("🗡️ Sphere overlap found %d results"), OverlapResults.Num());
    
    if (bHitDetected)
    {
        for (const FOverlapResult& Result : OverlapResults)
        {
            AActor* HitActor = Result.GetActor();
            if (!HitActor || HitActor == this)
                continue;
                
            // Skip if we already hit this actor
            if (AlreadyHitActors.Contains(HitActor))
            {
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Skipping already hit actor: %s"), *HitActor->GetName());
                continue;
            }
            
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Checking actor: %s (Class: %s)"), 
                   *HitActor->GetName(), *HitActor->GetClass()->GetName());
            
            // Check if it's a zombie by class name (most reliable method)
            bool bIsZombie = HitActor->GetClass()->GetName().Contains(TEXT("ZombieCharacter"));
            
            if (bIsZombie)
            {
                UE_LOG(LogTemp, Warning, TEXT("🗡️ ✓ Found zombie by class name: %s"), 
                       *HitActor->GetClass()->GetName());
                
                // Calculate distance and direction for additional validation
                FVector ToTarget = HitActor->GetActorLocation() - GetActorLocation();
                float DistanceToTarget = ToTarget.Size();
                float DotProduct = FVector::DotProduct(ForwardVector, ToTarget.GetSafeNormal());
                
                UE_LOG(LogTemp, Warning, TEXT("🗡️ Zombie %s - Distance: %.1f, Dot: %.2f"), 
                       *HitActor->GetName(), DistanceToTarget, DotProduct);
                
                // More lenient distance and direction check for better hit detection
                if (DistanceToTarget <= AttackRange && DotProduct > -0.5f) // Allow hits from wider angles
                {
                    // Mark this actor as hit
                    AlreadyHitActors.Add(HitActor);
                    
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚔️ ATTACKING zombie %s with %.1f damage!"), 
                           *HitActor->GetName(), TotalDamage);
                    
                    // Apply damage to the zombie
                    float ActualDamage = HitActor->TakeDamage(TotalDamage, FDamageEvent(), GetController(), this);
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ ✅ TakeDamage returned: %.1f"), ActualDamage);
                    
                    bSuccessfulHit = true;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ Zombie %s out of range or wrong direction (Distance: %.1f, Dot: %.2f)"), 
                           *HitActor->GetName(), DistanceToTarget, DotProduct);
                }
            }
        }
    }
    
    // METHOD 2: If no hits found, try a forward sweep as backup
    if (!bSuccessfulHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ No hits from sphere overlap, trying forward sweep backup"));
        
        TArray<FHitResult> HitResults;
        FVector EndLocation = StartLocation + (ForwardVector * AttackRange);
        
        bool bSweepHit = GetWorld()->SweepMultiByChannel(
            HitResults,
            StartLocation,
            EndLocation,
            FQuat::Identity,
            ECollisionChannel::ECC_Pawn,
            FCollisionShape::MakeSphere(AttackRadius * 0.8f) // Slightly smaller for sweep
        );
        
        if (bSweepHit)
        {
            UE_LOG(LogTemp, Warning, TEXT("🗡️ Forward sweep found %d results"), HitResults.Num());
            
            for (const FHitResult& Hit : HitResults)
            {
                AActor* HitActor = Hit.GetActor();
                if (!HitActor || HitActor == this || AlreadyHitActors.Contains(HitActor))
                    continue;
                
                bool bIsZombie = HitActor->GetClass()->GetName().Contains(TEXT("ZombieCharacter"));
                if (bIsZombie)
                {
                    AlreadyHitActors.Add(HitActor);
                    
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ ⚔️ SWEEP ATTACKING zombie %s with %.1f damage!"), 
                           *HitActor->GetName(), TotalDamage);
                    
                    float ActualDamage = HitActor->TakeDamage(TotalDamage, FDamageEvent(), GetController(), this);
                    UE_LOG(LogTemp, Warning, TEXT("🗡️ ✅ Sweep TakeDamage returned: %.1f"), ActualDamage);
                    
                    bSuccessfulHit = true;
                    break; // Only hit one enemy per attack
                }
            }
        }
    }
    
    if (bSuccessfulHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ✅ Successfully hit at least one enemy!"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🗡️ ❌ No valid enemies found in attack range"));
    }
}

void AAtlantisEonsCharacter::StoreOriginalMaterials()
{
    if (OriginalMaterials.Num() == 0 && GetMesh())
    {
        OriginalMaterials.Empty();
        TArray<UMaterialInterface*> Materials = GetMesh()->GetMaterials();
        OriginalMaterials.Append(Materials);
    }
}

void AAtlantisEonsCharacter::ApplyInvulnerabilityEffects()
{
    // Store original materials if not already stored
    StoreOriginalMaterials();

    // Apply invulnerability material if available
    if (InvulnerabilityMaterial && GetMesh())
    {
        for (int32 i = 0; i < GetMesh()->GetNumMaterials(); i++)
        {
            GetMesh()->SetMaterial(i, InvulnerabilityMaterial);
        }
    }

    // Spawn invulnerability particle effect
    if (InvulnerabilityEffect && !InvulnerabilityPSC)
    {
        InvulnerabilityPSC = UGameplayStatics::SpawnEmitterAttached(
            InvulnerabilityEffect,
            GetMesh(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            FVector(1.0f),
            EAttachLocation::SnapToTarget,
            true
        );
    }
}

void AAtlantisEonsCharacter::RemoveInvulnerabilityEffects()
{
    // Restore original materials
    if (GetMesh() && OriginalMaterials.Num() > 0)
    {
        for (int32 i = 0; i < OriginalMaterials.Num(); i++)
        {
            if (OriginalMaterials[i])
            {
                GetMesh()->SetMaterial(i, OriginalMaterials[i]);
            }
        }
    }

    // Remove invulnerability particle effect
    if (InvulnerabilityPSC)
    {
        InvulnerabilityPSC->DestroyComponent();
        InvulnerabilityPSC = nullptr;
    }
}

float AAtlantisEonsCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    UE_LOG(LogTemplateCharacter, Warning, TEXT("Player TakeDamage called: %.1f damage from %s, Invulnerable: %s, Dead: %s"), 
           DamageAmount, 
           DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"),
           bIsInvulnerable ? TEXT("Yes") : TEXT("No"),
           bIsDead ? TEXT("Yes") : TEXT("No"));

    // Don't take damage if already dead
    if (bIsDead)
    {
        UE_LOG(LogTemplateCharacter, Warning, TEXT("Player is dead, ignoring damage"));
        return 0.0f;
    }

    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    
    UE_LOG(LogTemplateCharacter, Warning, TEXT("Player Super::TakeDamage returned: %.1f"), ActualDamage);

    // Apply damage if not invulnerable (for now, always apply damage to fix combat issues)
    // TODO: Implement proper invulnerability frames later
    ApplyDamage(ActualDamage);
    UE_LOG(LogTemplateCharacter, Warning, TEXT("Player took %.1f damage from %s"), ActualDamage, DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"));

    return ActualDamage;
}

void AAtlantisEonsCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Function entered. bIsDead: %d"), bIsDead ? 1 : 0);
    
    if (bIsDead) 
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Character is already dead, returning early"));
        return;
    }

    // Safety check to ensure we're still valid
    if (!IsValid(this))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleDeath: Character is no longer valid, aborting"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Setting bIsDead to true and disabling movement"));
    bIsDead = true;

    // Disable movement with safety checks
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->DisableMovement();
        MovementComp->StopMovementImmediately();
    }

    // Disable input with safety checks
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Disabled player input"));
    }

    // Play death animation if available
    if (DeathMontage && GetMesh())
    {
        PlayAnimMontage(DeathMontage);
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Playing death animation"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: No death montage assigned or no mesh!"));
    }

    // Log death
    UE_LOG(LogTemplateCharacter, Log, TEXT("Character has died"));

    // Start respawn timer with safety checks
    if (GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleDeath: Starting respawn timer for %f seconds"), RespawnDelay);
        GetWorld()->GetTimerManager().SetTimer(
            RespawnTimerHandle,
            this,
            &AAtlantisEonsCharacter::ResetCharacter,
            RespawnDelay,
            false
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("HandleDeath: Cannot start respawn timer - invalid world"));
    }
}

void AAtlantisEonsCharacter::ResetCharacter()
{
    UE_LOG(LogTemp, Warning, TEXT("ResetCharacter: Resetting character after death"));
    
    // Reset health
    CurrentHealth = MaxHealth;
    bIsDead = false;

    // Stop any ongoing animations
    if (DeathMontage)
    {
        StopAnimMontage(DeathMontage);
    }

    // Re-enable movement
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    // Re-enable input
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        EnableInput(PC);
    }

    // Respawn at appropriate location
    RespawnCharacter();

    // Update UI
    SettingCircularBar_HP();
    SettingCircularBar_MP();

    // Log respawn
    UE_LOG(LogTemplateCharacter, Log, TEXT("Character has respawned with full health"));
}

void AAtlantisEonsCharacter::RespawnCharacter()
{
    UE_LOG(LogTemp, Warning, TEXT("RespawnCharacter: Finding respawn location"));
    
    // Find a player start to respawn at
    AActor* StartSpot = nullptr;
    APlayerController* PC = Cast<APlayerController>(GetController());

    if (PC)
    {
        // Try to find a Player Start actor
        TArray<AActor*> PlayerStarts;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

        if (PlayerStarts.Num() > 0)
        {
            // Use the first player start found
            StartSpot = PlayerStarts[0];
            UE_LOG(LogTemp, Warning, TEXT("RespawnCharacter: Found PlayerStart at %s"), *StartSpot->GetActorLocation().ToString());
        }
    }

    if (StartSpot)
    {
        // Teleport to the start spot
        SetActorLocation(StartSpot->GetActorLocation());
        SetActorRotation(StartSpot->GetActorRotation());
        UE_LOG(LogTemp, Warning, TEXT("RespawnCharacter: Teleported to PlayerStart"));
    }
    else
    {
        // If no player start is found, just reset to origin
        SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
        SetActorRotation(FRotator(0.0f));
        UE_LOG(LogTemplateCharacter, Warning, TEXT("No PlayerStart found - respawning at origin"));
    }
}

void AAtlantisEonsCharacter::FaceNearestEnemy()
{
    UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: Looking for nearest enemy to face"));
    
    // Find the nearest enemy within a reasonable range
    const float SearchRadius = 500.0f; // 5 meter search radius
    AActor* NearestEnemy = nullptr;
    float NearestDistance = SearchRadius;
    
    // Get all actors in the world
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
    
    FGenericTeamId MyTeamId = GetGenericTeamId();
    FVector MyLocation = GetActorLocation();
    
    for (AActor* Actor : AllActors)
    {
        if (!Actor || Actor == this || !IsValid(Actor)) continue;
        
        // Check if this is an enemy
        bool bIsEnemy = false;
        
        // Check 1: Class name contains "Zombie"
        if (Actor->GetClass()->GetName().Contains("Zombie"))
        {
            bIsEnemy = true;
        }
        // Check 2: Actor has enemy tags
        else if (Actor->ActorHasTag(TEXT("AdvancedZombieEnemy")) || Actor->ActorHasTag(TEXT("Enemy")))
        {
            bIsEnemy = true;
        }
        // Check 3: Different team ID
        else
        {
            IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
            if (TeamAgent && TeamAgent->GetGenericTeamId() != MyTeamId)
            {
                bIsEnemy = true;
            }
        }
        
        if (bIsEnemy)
        {
            float Distance = FVector::Dist(MyLocation, Actor->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestEnemy = Actor;
            }
        }
    }
    
    // If we found an enemy, face towards it
    if (NearestEnemy)
    {
        FVector DirectionToEnemy = (NearestEnemy->GetActorLocation() - MyLocation).GetSafeNormal();
        FRotator TargetRotation = DirectionToEnemy.Rotation();
        
        // Only rotate on the Z axis (yaw) to keep the character upright
        TargetRotation.Pitch = 0.0f;
        TargetRotation.Roll = 0.0f;
        
        // FIXED: Use immediate rotation instead of interpolation for better combat responsiveness
        // This ensures the player immediately faces the enemy when attacking
        SetActorRotation(TargetRotation);
        
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: Immediately rotated to face enemy %s at distance %.1f"), 
               *NearestEnemy->GetName(), NearestDistance);
        
        // Draw debug line to show targeting
        DrawDebugLine(GetWorld(), MyLocation, NearestEnemy->GetActorLocation(), FColor::Orange, false, 1.0f, 0, 2.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FaceNearestEnemy: No enemies found within range"));
    }
}


void AAtlantisEonsCharacter::PlayHitReactMontage()
{
    if (HitReactMontage && GetMesh())
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->Montage_Play(HitReactMontage);
            UE_LOG(LogTemp, Warning, TEXT("Playing hit react montage"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HitReactMontage is null or no mesh found"));
    }
}
