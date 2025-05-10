#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "AtlantisEonsPlayerController.generated.h"

UCLASS()
class ATLANTISEONS_API AAtlantisEonsPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:

public:
    AAtlantisEonsPlayerController();

    virtual void BeginPlay() override;
};
