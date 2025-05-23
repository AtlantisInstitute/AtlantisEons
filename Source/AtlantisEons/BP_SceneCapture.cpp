#include "BP_SceneCapture.h"

ABP_SceneCapture::ABP_SceneCapture()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ABP_SceneCapture::BeginPlay()
{
    Super::BeginPlay();
}

void ABP_SceneCapture::BeginDestroy()
{
    Super::BeginDestroy();
}
