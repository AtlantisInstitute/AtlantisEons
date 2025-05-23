#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "WBP_DragAndDropImage.generated.h"

UCLASS()
class ATLANTISEONS_API UWBP_DragAndDropImage : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* DragandDropImage;
};
