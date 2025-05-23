#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "BP_Item.generated.h"

class UWBP_ItemNamePopUp;
class AAtlantisEonsCharacter;

UCLASS()
class ATLANTISEONS_API ABP_Item : public AActor
{
    friend class AAtlantisEonsCharacter;
    GENERATED_BODY()

public:
    ABP_Item();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Item")
    void SpawnItem(int32 InItemIndex, int32 InStackNumber);

    UFUNCTION(BlueprintCallable, Category = "Item")
    void RemoveItemNameWidget();

    UFUNCTION(BlueprintCallable, Category = "Item")
    void InitializeItem(int32 NewItemIndex, int32 NewStackNumber);

    UFUNCTION(BlueprintCallable, Category = "Item")
    void ShowItemName(bool bShow);

    UFUNCTION(BlueprintCallable, Category = "Item")
    static UTexture2D* GetDefaultTexture();

protected:
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* ItemMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* SphereCollision;

    // UI Components
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UWBP_ItemNamePopUp> ItemNameWidgetClass;

    UPROPERTY()
    UWBP_ItemNamePopUp* ItemNameWidget;

    // Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FStructure_ItemInfo ItemInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 ItemIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 StackNumber;

protected:
    // UI Components
    UPROPERTY(BlueprintReadWrite, Category = "UI")
    UWBP_ItemNamePopUp* ItemNameRef;

private:
    // Data
    UPROPERTY()
    UDataTable* ItemDataTable;

    // Helper Functions
    void UpdateItemNameWidget(class AAtlantisEonsCharacter* Character);
    void OnMeshLoaded();
    void SetupItemMesh();
    void ShowItemNamePopUp();
    void HideItemNamePopUp();
    void UpdateItemNamePosition();
    void SetDefaultItemInfo();

    FStreamableManager StreamableManager;

protected:
    // Overlap Events
    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
