#pragma once

#include "CoreMinimal.h"
#include "BP_ItemInfo.h"
#include "BP_ConcreteItemInfo.generated.h"

UCLASS(Blueprintable, BlueprintType)
class ATLANTISEONS_API UBP_ConcreteItemInfo : public UBP_ItemInfo
{
    GENERATED_BODY()

public:
    UBP_ConcreteItemInfo()
    {
        // Initialize with default values
        ItemIndex = -1;
        ItemName = TEXT("");
        ItemDescription = TEXT("");
        StackNumber = 0;
        Equipped = false;
        bIsValid = false;
        bIsStackable = false;
        ItemType = EItemType::Equip;
        ItemEquipSlot = EItemEquipSlot::None;
        RecoveryHP = 0;
        RecoveryMP = 0;
    }

    // Override GetItemTableRow to provide concrete implementation
    virtual void GetItemTableRow(bool& Find, FStructure_ItemInfo& ItemInfoStructure) override;

    // Function to copy data from a structure
    virtual void CopyFromStructure(const FStructure_ItemInfo& ItemInfoStructure) override;
}; 