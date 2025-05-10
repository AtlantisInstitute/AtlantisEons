#include "DamageNumberScreenManagerTickActor.h"
#include "DamageNumberScreenManager.h"
#include "Engine/World.h"

void ADamageNumberScreenManagerTickActor::BeginPlay()
{
	Super::BeginPlay();
	PrimaryActorTick.bCanEverTick = true;
}

void ADamageNumberScreenManagerTickActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetWorld())
	{
		UDamageNumberScreenManager::Get(GetWorld())->Tick(DeltaTime);
	}
}
