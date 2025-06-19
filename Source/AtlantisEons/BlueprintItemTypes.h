#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "ItemTypes.h"
#include "BlueprintItemTypes.generated.h"

/**
 * This structure is specifically designed for Blueprint compatibility with FStructure_ItemInfo
 * It provides a way to access the same data from Blueprint and C++ code
 */
USTRUCT(BlueprintType)
struct ATLANTISEONS_API FBlueprintItemInfo : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ItemIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSoftObjectPtr<UStaticMesh> StaticMeshID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSoftObjectPtr<UTexture2D> ItemThumbnail;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsValid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsStackable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StackNumber;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemType ItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemEquipSlot ItemEquipSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RecoveryHP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RecoveryMP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Damage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Defence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 STR;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 DEX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 INT;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Price;

    FBlueprintItemInfo()
    {
        ItemIndex = -1;
        ItemName = TEXT("");
        ItemDescription = TEXT("");
        bIsValid = false;
        bIsStackable = false;
        StackNumber = 0;
        ItemType = EItemType::Equip;
        ItemEquipSlot = EItemEquipSlot::Consumable;
        RecoveryHP = 0;
        RecoveryMP = 0;
        Damage = 0;
        Defence = 0;
        HP = 0;
        MP = 0;
        STR = 0;
        DEX = 0;
        INT = 0;
        Price = 0;
    }

    // Conversion to FStructure_ItemInfo
    FStructure_ItemInfo ToOriginalStructure() const
    {
        FStructure_ItemInfo Result;
        Result.ItemIndex = ItemIndex;
        Result.ItemName = ItemName;
        Result.StaticMeshID = StaticMeshID;
        Result.ItemThumbnail = ItemThumbnail;
        Result.ItemDescription = ItemDescription;
        Result.bIsValid = bIsValid;
        Result.bIsStackable = bIsStackable;
        Result.StackNumber = StackNumber;
        Result.ItemType = ItemType;
        Result.ItemEquipSlot = ItemEquipSlot;
        Result.RecoveryHP = RecoveryHP;
        Result.RecoveryMP = RecoveryMP;
        Result.Damage = Damage;
        Result.Defence = Defence;
        Result.HP = HP;
        Result.MP = MP;
        Result.STR = STR;
        Result.DEX = DEX;
        Result.INT = INT;
        Result.Price = Price;
        return Result;
    }
};

// For backward compatibility
typedef FBlueprintItemInfo FFStructure_ItemInfo;
