#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "DamageNumberWidgetComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATLANTISEONS_API UDamageNumberWidgetComponent : public UWidgetComponent
{
    GENERATED_BODY()

public:
    UDamageNumberWidgetComponent();
};
