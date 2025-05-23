#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/MenuAnchor.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "BP_Item.h"
#include "WBP_DragAndDropImage.h"
#include "BP_DragandDropOperation.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

UENUM(BlueprintType)
enum class EInventorySlotType : uint8
{
    None UMETA(DisplayName = "None"),
    Equipment UMETA(DisplayName = "Equipment"),
    Inventory UMETA(DisplayName = "Inventory")
};

class UBP_ItemInfo;
class UWBP_ItemDescription;
class UWBP_ContextMenu;
class AAtlantisEonsCharacter;

#include "WBP_InventorySlot.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotClickedSignature, int32, SlotIndex);

DECLARE_DYNAMIC_DELEGATE_RetVal(UUserWidget*, FOnGetUserMenuContentSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FContextMenuClickUseSignature, UBP_ItemInfo*, ItemInfoRef, UWBP_InventorySlot*, InventorySlot);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FContextMenuClickThrowSignature, UBP_ItemInfo*, ItemInfoRef, UWBP_InventorySlot*, InventorySlot);

UCLASS()
class ATLANTISEONS_API UWBP_InventorySlot : public UUserWidget
{
    GENERATED_BODY()

public:
    EInventorySlotType GetInventorySlotType() const { return SlotType; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    EInventorySlotType SlotType;

public:
    UFUNCTION()
    void HandleMouseLeftButtonDown();

    UFUNCTION()
    void HandleRemoveItemDescription();

    UFUNCTION()
    void ContextMenuUse();

    UWBP_InventorySlot(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    // Root hierarchy widgets
    UPROPERTY(Transient, meta=(BindWidget))
    UCanvasPanel* CanvasPanel;
    
    UPROPERTY(Transient, meta=(BindWidget))
    USizeBox* SizeBox;
    
    UPROPERTY(Transient, meta=(BindWidget))
    UOverlay* Overlay;
    
    UPROPERTY(Transient, meta=(BindWidget))
    UImage* SlotBackground;
    
    UPROPERTY(Transient, meta=(BindWidget))
    UImage* ItemThumbnail;
    
    UPROPERTY(Transient, meta=(BindWidget))
    UTextBlock* SlotText;

    // State Variables
    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    bool SlotEmpty;

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    bool OnDescription;

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    UBP_ItemInfo* inventoryItemInfoRef;

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    UWBP_ItemDescription* WidgetItemDescriptionRef;

    UPROPERTY()
    UWBP_ItemDescription* ItemDescription;

    UPROPERTY()
    FTimerHandle AsyncLoadTimerHandle;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    TSubclassOf<UWBP_ItemDescription> ItemDescriptionClass;

    // Stack count is handled by SlotText

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    bool bIsEmpty;

    UPROPERTY(BlueprintReadWrite)
    UWBP_ContextMenu* ContextMenuWidget;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UWBP_ContextMenu> ContextMenuWidgetClass;

    UFUNCTION()
    void OnAssetLoaded();

    UFUNCTION()
    void UpdateItemDescription(const FStructure_ItemInfo& ItemInfo, const FVector2D& MousePosition);

    UFUNCTION()
    void SetDescriptionPosition(const FVector2D& MousePosition);

    UFUNCTION()
    FString GetStatString(const FString& Prefix, int32 Value) const;

    UFUNCTION()
    void UpdateDescriptionStats(UWBP_ItemDescription* Description, const FStructure_ItemInfo& ItemInfo);

    // Blueprint Events
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
    void SetItemThumb(const TSoftObjectPtr<UTexture2D>& ItemThumb);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
    void MakeSlotClean();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
    void RemoveItemDescription();

    // Mouse Events
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(BlueprintReadWrite, Category = "Mouse")
    bool MouseLeftDown;

    UPROPERTY(BlueprintReadWrite, Category = "Mouse")
    bool MouseRightDown;

    UPROPERTY(BlueprintReadWrite, Category = "Mouse")
    FVector2D MouseButtonDownPosition;

    UFUNCTION(BlueprintCallable, Category = "Mouse")
    void MouseLeftButtonDown();

    UFUNCTION(BlueprintCallable, Category = "Mouse")
    void MouseRightButtonDown();

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    int32 SlotIndex;

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    FStructure_ItemInfo ItemInfo;

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnSlotClickedSignature OnSlotClicked;

    // Context Menu
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UMenuAnchor* ContextMenuAnchor;

    UFUNCTION(BlueprintCallable, Category = "Context Menu")
    void ShowContextMenuAtMousePosition();

    UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Context Menu")
    UUserWidget* GetUserMenuContent();

    UFUNCTION(BlueprintCallable, Category = "Context Menu")
    void OnContextMenuUse();

    UFUNCTION(BlueprintCallable, Category = "Context Menu")
    void OnContextMenuThrow();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UBP_ItemInfo* GetInventoryItemInfoRef() const { return inventoryItemInfoRef; }

    UFUNCTION(BlueprintPure, Category = "UI")
    ESlateVisibility GetItemThumbnailVisibility() const;

    // Functions
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UpdateSlot(UBP_ItemInfo* NewItemInfo);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ClearSlot();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UpdateStackText(int32 NewStackNumber);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SetSlotIndex(int32 NewIndex);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    FText GetStackNumberText();

    // Font settings
    UPROPERTY()
    FSlateFontInfo DefaultFont;

    // Context Menu References
    UPROPERTY()
    UWBP_ContextMenu* WidgetContextMenuRef;

    UPROPERTY(BlueprintAssignable, Category = "Context Menu")
    FContextMenuClickUseSignature ContextMenuClickUse;

    UPROPERTY(BlueprintAssignable, Category = "Context Menu")
    FContextMenuClickThrowSignature ContextMenuClickThrow;

    // Helper functions
    void SetupDefaultFont();
    bool IsValidItemInfo();
    bool IsEquipmentType(const FStructure_ItemInfo& ItemInfo);
    bool ShouldShowStackNumber(const FStructure_ItemInfo& ItemInfo);
    FText GetStackNumberDisplay(const FStructure_ItemInfo& ItemInfo);
    void UpdateFontSize(UTextBlock* TextBlock, float Size = 11.0f);
    void UpdateStackNumber(int32 NewStackNumber);
    void CreateDefaultComponents();

    // Gets the item data from the data table
    bool GetItemTableRow(bool& bFound, FStructure_ItemInfo& OutRow);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void TestPopulateSlot();
};
