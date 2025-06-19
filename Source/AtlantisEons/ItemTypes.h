#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
    Equip       UMETA(DisplayName = "Equipment"),
    Consume_HP  UMETA(DisplayName = "HP Consumable"),
    Consume_MP  UMETA(DisplayName = "MP Consumable"),
    Collection  UMETA(DisplayName = "Collection Item")
};

UENUM(BlueprintType)
enum class EItemEquipSlot : uint8
{
    Consumable  UMETA(DisplayName = "Consumable"),
    Head        UMETA(DisplayName = "Head"),
    Body        UMETA(DisplayName = "Body"),
    Weapon      UMETA(DisplayName = "Weapon"),
    Accessory   UMETA(DisplayName = "Accessory")
};

// Properly named structure for C++ and Blueprint
USTRUCT(BlueprintType)
struct ATLANTISEONS_API FStructure_ItemInfo : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 ItemIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FString ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSoftObjectPtr<UStaticMesh> StaticMeshID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSoftObjectPtr<UTexture2D> ItemThumbnail;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FString ItemDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool bIsValid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool bIsStackable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 StackNumber;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EItemType ItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EItemEquipSlot ItemEquipSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 RecoveryHP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 RecoveryMP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 Damage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 Defence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 HP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 MP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 STR;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 DEX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 INT;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 Price;

    FStructure_ItemInfo()
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
};

// Add backward compatibility typedef
typedef FStructure_ItemInfo Structure_ItemInfo;
