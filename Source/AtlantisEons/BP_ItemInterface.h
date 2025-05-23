#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BP_ItemInterface.generated.h"

UINTERFACE(MinimalAPI)
class UBP_ItemInterface : public UInterface
{
    GENERATED_BODY()
};

class IBP_ItemInterface
{
    GENERATED_BODY()

public:
    // Add interface functions here
    virtual void OnItemPickedUp() = 0;
    virtual void OnItemDropped() = 0;
};
