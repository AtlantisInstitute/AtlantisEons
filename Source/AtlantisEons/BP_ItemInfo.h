#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "BP_Item.h"
#include "BlueprintItemTypes.h"
#include "BP_ItemInfo.generated.h"

UCLASS(Blueprintable, BlueprintType, Abstract, CustomConstructor, meta=(DisplayName="Item Info Object", ShortTooltip="Base class for item information"))
class ATLANTISEONS_API UBP_ItemInfo : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Item")
    void CopyFrom(const FStructure_ItemInfo& Other)
    {
        ItemIndex = Other.ItemIndex;
        ItemName = Other.ItemName;
        ItemDescription = Other.ItemDescription;
        ItemType = Other.ItemType;
        ItemEquipSlot = Other.ItemEquipSlot;
        RecoveryHP = Other.RecoveryHP;
        RecoveryMP = Other.RecoveryMP;
        MeshID = Other.StaticMeshID;
        Thumbnail = Other.ItemThumbnail;
        bIsValid = Other.bIsValid;
        bIsStackable = Other.bIsStackable;
        StackNumber = Other.StackNumber;
        
        // Initialize thumbnail brush if we have a valid thumbnail
        if (!Thumbnail.IsNull())
        {
            if (UTexture2D* LoadedTexture = Thumbnail.LoadSynchronous())
            {
                FSlateBrush NewBrush;
                NewBrush.SetResourceObject(LoadedTexture);
                NewBrush.ImageSize = FVector2D(64.0f, 64.0f);
                NewBrush.DrawAs = ESlateBrushDrawType::Image;
                NewBrush.Tiling = ESlateBrushTileType::NoTile;
                NewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
                ThumbnailBrush = NewBrush;
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Item")
    virtual void CopyFromStructure(const FStructure_ItemInfo& Other)
    {
        ItemIndex = Other.ItemIndex;
        ItemName = Other.ItemName;
        ItemDescription = Other.ItemDescription;
        StackNumber = Other.StackNumber;
        ItemType = Other.ItemType;
        ItemEquipSlot = Other.ItemEquipSlot;
        RecoveryHP = Other.RecoveryHP;
        RecoveryMP = Other.RecoveryMP;
        MeshID = Other.StaticMeshID;
        Thumbnail = Other.ItemThumbnail;
        bIsValid = Other.bIsValid;
        bIsStackable = Other.bIsStackable;
        
        // Initialize thumbnail brush if we have a valid thumbnail
        if (!Thumbnail.IsNull())
        {
            if (UTexture2D* LoadedTexture = Thumbnail.LoadSynchronous())
            {
                FSlateBrush NewBrush;
                NewBrush.SetResourceObject(LoadedTexture);
                NewBrush.ImageSize = FVector2D(64.0f, 64.0f);
                NewBrush.DrawAs = ESlateBrushDrawType::Image;
                NewBrush.Tiling = ESlateBrushTileType::NoTile;
                NewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
                ThumbnailBrush = NewBrush;
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Item")
    void CopyFromItemInfo(UBP_ItemInfo* Other)
    {
        if (!Other) return;
        
        ItemIndex = Other->ItemIndex;
        ItemName = Other->ItemName;
        ItemDescription = Other->ItemDescription;
        StackNumber = Other->StackNumber;
        Equipped = Other->Equipped;
        ItemDataTable = Other->ItemDataTable;
        ItemType = Other->ItemType;
        ItemEquipSlot = Other->ItemEquipSlot;
        RecoveryHP = Other->RecoveryHP;
        RecoveryMP = Other->RecoveryMP;
        MeshID = Other->MeshID;
        Thumbnail = Other->Thumbnail;
        Material1 = Other->Material1;
        Material2 = Other->Material2;
        bIsValid = Other->bIsValid;
        bIsStackable = Other->bIsStackable;
        ThumbnailBrush = Other->ThumbnailBrush;
    }

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 ItemIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FString ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FString ItemDescription;

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
    TSoftObjectPtr<UStaticMesh> MeshID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSoftObjectPtr<UTexture2D> Thumbnail;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    UMaterialInterface* Material1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    UMaterialInterface* Material2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool Equipped;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool bIsValid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool bIsStackable;

    /** Reference to the item data table */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    UDataTable* ItemDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FSlateBrush ThumbnailBrush;

    UBP_ItemInfo();

public:

    /** Get item information from the data table based on ItemIndex */
    UFUNCTION(BlueprintCallable, Category = "Item")
    virtual void GetItemTableRow(bool& Find, FStructure_ItemInfo& ItemInfoStructure);

    /** Get the data table asset reference */
    UFUNCTION(BlueprintCallable, Category = "Item")
    UDataTable* GetItemDataTable() const;

    /** Set the data table asset reference */
    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetItemDataTable(UDataTable* NewDataTable);

    /** Get whether the item is equipped */
    UFUNCTION(BlueprintCallable, Category = "Item")
    bool GetEquipped() const { return Equipped; }

    /** Set whether the item is equipped */
    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetEquipped(bool bNewEquipped) { Equipped = bNewEquipped; }

    /** Get the item index */
    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetItemIndex() const { return ItemIndex; }

    /** Get the stack number */
    UFUNCTION(BlueprintCallable, Category = "Item")
    int32& GetStackNumber() { return StackNumber; }

protected:
    /** Creates hardcoded item data for testing when data table lookup fails */
    FStructure_ItemInfo CreateHardcodedItemData(int32 InItemIndex);
};
