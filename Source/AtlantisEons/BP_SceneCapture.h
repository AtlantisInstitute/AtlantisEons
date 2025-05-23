#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BP_SceneCapture.generated.h"

UCLASS(Blueprintable)
class ATLANTISEONS_API ABP_SceneCapture : public AActor
{
    GENERATED_BODY()

public:
    ABP_SceneCapture();
    virtual ~ABP_SceneCapture() = default;

    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;
};
