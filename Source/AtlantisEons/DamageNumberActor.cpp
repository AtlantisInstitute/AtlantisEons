#include "DamageNumberActor.h"
#include "DamageNumberWidget.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"

ADamageNumberActor::ADamageNumberActor()
{
    PrimaryActorTick.bCanEverTick = true;
    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageNumberWidgetComponent"));
    RootComponent = WidgetComponent;
    WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    WidgetComponent->SetDrawSize(FVector2D(400.0f, 200.0f));
    WidgetComponent->SetTwoSided(true);
    WidgetComponent->SetTranslucentSortPriority(1000);
    WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WidgetComponent->SetVisibility(true);
    WidgetComponent->SetRelativeLocation(FVector(0,0,0));
    Lifetime = 1.0f;
    Elapsed = 0.0f;
}

void ADamageNumberActor::BeginPlay()
{
    Super::BeginPlay();
}

void ADamageNumberActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Elapsed += DeltaTime;
    if (WidgetComponent)
    {
        // Always face the local player's camera (billboard effect)
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC && PC->PlayerCameraManager)
        {
            FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
            FVector ActorLoc = GetActorLocation();
            FVector ToCamera = CamLoc - ActorLoc;
            FRotator FaceCameraRot = FRotationMatrix::MakeFromX(ToCamera).Rotator();
            // Only yaw (horizontal) and pitch (vertical) should be set, roll should be zero
            FaceCameraRot.Roll = 0.0f;
            WidgetComponent->SetWorldRotation(FaceCameraRot);
        }
    }
    if (Elapsed >= Lifetime)
    {
        Destroy();
    }
}

void ADamageNumberActor::Init(float DamageAmount, TSubclassOf<UUserWidget> DamageWidgetClass, bool bIsPlayerDamage)
{
    if (WidgetComponent && DamageWidgetClass)
    {
        WidgetComponent->SetWidgetClass(DamageWidgetClass);
        if (UUserWidget* UserWidget = WidgetComponent->GetUserWidgetObject())
        {
            if (UDamageNumberWidget* DamageWidget = Cast<UDamageNumberWidget>(UserWidget))
            {
                DamageWidget->InitializeDamageNumber(DamageAmount, bIsPlayerDamage);
            }
        }
    }
}
