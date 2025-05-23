#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.h"
#include "Blueprint/DragDropOperation.h"
#include "BP_Item.h"
#include "BP_DragandDropOperation.generated.h"

class UWBP_InventorySlot;

UCLASS()
class ATLANTISEONS_API UBP_DragandDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Drag and Drop")
    UWBP_InventorySlot* OriginalInventorySlotWidget;

    UPROPERTY(BlueprintReadWrite, Category = "Drag and Drop")
    FStructure_ItemInfo ItemInfoRef;
};
