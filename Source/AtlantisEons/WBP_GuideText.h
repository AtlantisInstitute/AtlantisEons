#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_GuideText.generated.h"

UCLASS()
class ATLANTISEONS_API UWBP_GuideText : public UUserWidget
{
    GENERATED_BODY()

public:
    UWBP_GuideText(const FObjectInitializer& ObjectInitializer);
};
