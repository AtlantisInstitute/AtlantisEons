#include "WBP_InventorySlot.h"
#include "WBP_ContextMenu.h"
#include "WBP_ItemDescription.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "AtlantisEonsCharacter.h"
#include "AtlantisEonsGameInstance.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "BP_Item.h"

UWBP_InventorySlot::UWBP_InventorySlot(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SlotEmpty = true;
    SlotIndex = -1;
    inventoryItemInfoRef = nullptr;
}

void UWBP_InventorySlot::SetSlotIndex(int32 NewIndex)
{
    if (SlotIndex == NewIndex)
    {
        return; // Avoid unnecessary updates
    }

    SlotIndex = NewIndex;
    
    // Create components if needed
    CreateDefaultComponents();
    
    // Special handling for slots 10-15
    if (SlotIndex >= 10 && SlotIndex <= 15)
    {
        // Ensure thumbnail exists and is properly configured
        if (ItemThumbnail)
        {
            // Set visibility based on whether we have an item
            ItemThumbnail->SetVisibility(SlotEmpty ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
            
            // Force proper layout
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ItemThumbnail->Slot))
            {
                CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
                CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
                CanvasSlot->SetSize(FVector2D(64.0f, 64.0f));
                CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
                CanvasSlot->SetZOrder(10);
            }
        }
        
        // Configure stack text
        if (SlotText)
        {
            SlotText->SetVisibility(ESlateVisibility::Hidden);
            
            // Force proper layout
            if (UCanvasPanelSlot* TextSlot = Cast<UCanvasPanelSlot>(SlotText->Slot))
            {
                TextSlot->SetAnchors(FAnchors(0.9f, 0.9f, 0.9f, 0.9f));
                TextSlot->SetAlignment(FVector2D(1.0f, 1.0f));
                TextSlot->SetSize(FVector2D(40.0f, 20.0f));
                TextSlot->SetPosition(FVector2D(-5.0f, -5.0f));
                TextSlot->SetZOrder(20);
            }
        }
    }
    
    // Force layout update
    ForceLayoutPrepass();
}

void UWBP_InventorySlot::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Find the widgets that the Blueprint system created for us
    CreateDefaultComponents();
    
    // Look for ContextMenuAnchor in the widget tree if not already set
    if (!ContextMenuAnchor)
    {
        TArray<UWidget*> AllChildren;
        if (WidgetTree)
        {
            WidgetTree->GetAllWidgets(AllChildren);
        }
        
        for (UWidget* Child : AllChildren)
        {
            if (UMenuAnchor* MenuAnchorWidget = Cast<UMenuAnchor>(Child))
            {
                ContextMenuAnchor = MenuAnchorWidget;
                break;
            }
        }
    }
    
    // Ensure context menu widget class is set
    if (!ContextMenuWidgetClass)
    {
        // Load the Blueprint class, not the C++ class
        ContextMenuWidgetClass = LoadClass<UWBP_ContextMenu>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/WBP_ContextMenu.WBP_ContextMenu_C"));
        if (!ContextMenuWidgetClass)
        {
            UE_LOG(LogTemp, Error, TEXT("Slot %d: Failed to load WBP_ContextMenu Blueprint class"), SlotIndex);
            ContextMenuWidgetClass = UWBP_ContextMenu::StaticClass();
        }
    }
    
    // Set up menu anchor if found
    if (ContextMenuAnchor)
    {
        ContextMenuAnchor->SetPlacement(MenuPlacement_ComboBox);
        ContextMenuAnchor->OnGetUserMenuContentEvent.BindUFunction(this, FName("GetUserMenuContent"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Slot %d: ContextMenuAnchor not found in Blueprint!"), SlotIndex);
    }
    
    // Initialize item description class if not set
    if (!ItemDescriptionClass)
    {
        ItemDescriptionClass = UWBP_ItemDescription::StaticClass();
    }
    
    if (ItemThumbnail && SlotText)
    {
        // Initialize with empty state
        if (!SlotEmpty && inventoryItemInfoRef && inventoryItemInfoRef->bIsValid)
        {
            UpdateSlot(inventoryItemInfoRef);
        }
        else
        {
            ItemThumbnail->SetVisibility(ESlateVisibility::Hidden);
            SlotText->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Slot %d: Failed to find required Blueprint widgets! Thumbnail: %s, Text: %s"), 
            SlotIndex,
            ItemThumbnail ? TEXT("Found") : TEXT("Missing"),
            SlotText ? TEXT("Found") : TEXT("Missing"));
    }
}

void UWBP_InventorySlot::HandleMouseLeftButtonDown()
{
    if (inventoryItemInfoRef)
    {
        // Show item description
        if (ItemDescription)
        {
            bool bFound = false;
            FStructure_ItemInfo ItemInfo;
            inventoryItemInfoRef->GetItemTableRow(bFound, ItemInfo);
            if (bFound)
            {
                ItemDescription->UpdateDescription(ItemInfo);
            }
        }
    }
    ContextMenuUse();
}

void UWBP_InventorySlot::HandleRemoveItemDescription()
{
    if (ItemDescription)
    {
        ItemDescription->RemoveFromParent();
    }
}

void UWBP_InventorySlot::ContextMenuUse()
{
    // Broadcast the ContextMenuClickUse event with the item info reference and this inventory slot
    if (inventoryItemInfoRef)
    {
        ContextMenuClickUse.Broadcast(inventoryItemInfoRef, this);
    }
}

void UWBP_InventorySlot::SetupDefaultFont()
{
    if (SlotText)
    {
        SlotText->SetVisibility(ESlateVisibility::Hidden);
    }
}

FReply UWBP_InventorySlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Store the current mouse position
    MouseButtonDownPosition = InMouseEvent.GetScreenSpacePosition();

    // Update mouse button states
    MouseLeftDown = InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
    MouseRightDown = InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);

    return FReply::Handled();
}

FReply UWBP_InventorySlot::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Check if left mouse button was down
    if (MouseLeftDown)
    {
        MouseLeftButtonDown();
    }
    // Check if right mouse button was down
    else if (MouseRightDown)
    {
        MouseRightButtonDown();
    }

    // Reset mouse button states
    MouseLeftDown = false;
    MouseRightDown = false;

    return FReply::Handled();
}

void UWBP_InventorySlot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    // Create drag and drop image widget
    UWBP_DragAndDropImage* DragVisual = CreateWidget<UWBP_DragAndDropImage>(GetOwningPlayer(), LoadClass<UWBP_DragAndDropImage>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/WBP_DragAndDropImage.WBP_DragAndDropImage_C")));
    
    if (DragVisual && ItemThumbnail)
    {
        // Set the drag visual's image brush from our thumbnail
        UImage* DragImage = DragVisual->DragandDropImage;
        if (DragImage)
        {
            DragImage->SetBrush(ItemThumbnail->GetBrush());
        }

        // Create the drag drop operation
        UBP_DragandDropOperation* DragDropOp = NewObject<UBP_DragandDropOperation>(GetTransientPackage(), LoadClass<UBP_DragandDropOperation>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/BP_DragandDropOperation.BP_DragandDropOperation_C")));
        if (DragDropOp)
        {
            DragDropOp->DefaultDragVisual = DragVisual;
            DragDropOp->Pivot = EDragPivot::CenterCenter;
            DragDropOp->OriginalInventorySlotWidget = this;
            bool bFound = false;
            FStructure_ItemInfo ItemInfo;
            if (inventoryItemInfoRef)
            {
                inventoryItemInfoRef->GetItemTableRow(bFound, ItemInfo);
                if (bFound)
                {
                    DragDropOp->ItemInfoRef = ItemInfo;
                }
            }
            OutOperation = DragDropOp;
        }
    }
}

bool UWBP_InventorySlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    // Cast operation to BP_DragandDropOperation
    UBP_DragandDropOperation* DragDropOp = Cast<UBP_DragandDropOperation>(InOperation);
    if (!DragDropOp)
    {
        return false;
    }

    // Check if we're dropping on the same slot we dragged from
    if (DragDropOp->OriginalInventorySlotWidget == this)
    {
        return true;
    }

    // Get player character and cast to BP_Character
    ACharacter* Character = UGameplayStatics::GetPlayerCharacter(this, 0);
    AAtlantisEonsCharacter* BPCharacter = Cast<AAtlantisEonsCharacter>(Character);
    if (!BPCharacter)
    {
        return false;
    }

    // Call the exchange item function
    bool bFound = false;
    FStructure_ItemInfo FromItemInfo;
    UBP_ItemInfo* FromItemInfoRef = nullptr;
    if (DragDropOp->OriginalInventorySlotWidget)
    {
        FromItemInfoRef = DragDropOp->OriginalInventorySlotWidget->GetInventoryItemInfoRef();
    }
    BPCharacter->DragAndDropExchangeItem(
        FromItemInfoRef,
        DragDropOp->OriginalInventorySlotWidget,
        inventoryItemInfoRef,
        this
    );

    return false;
}

void UWBP_InventorySlot::UpdateSlot(UBP_ItemInfo* NewItemInfo)
{
    if (!NewItemInfo) 
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateSlot called with null NewItemInfo for slot %d"), SlotIndex);
        return;
    }

    // Set the item info reference
    inventoryItemInfoRef = NewItemInfo;
    
    // Update empty state
    SlotEmpty = false;

    // Handle the thumbnail
    if (ItemThumbnail)
    {
        // First try to use the pre-configured brush
        if (NewItemInfo->ThumbnailBrush.GetResourceObject())
        {
            ItemThumbnail->SetBrush(NewItemInfo->ThumbnailBrush);
            ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
        }
        else if (!NewItemInfo->Thumbnail.IsNull())
        {
            // Load the texture and create a brush
            if (UTexture2D* LoadedTexture = NewItemInfo->Thumbnail.LoadSynchronous())
            {
                FSlateBrush NewBrush;
                NewBrush.SetResourceObject(LoadedTexture);
                NewBrush.ImageSize = FVector2D(64.0f, 64.0f);
                NewBrush.DrawAs = ESlateBrushDrawType::Image;
                NewBrush.Tiling = ESlateBrushTileType::NoTile;
                NewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
                
                ItemThumbnail->SetBrush(NewBrush);
                ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Slot %d: Failed to load thumbnail texture"), SlotIndex);
            }
        }
        else
        {
            // Load default texture
            UTexture2D* DefaultTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Engine/EditorResources/S_Actor")));
            if (DefaultTexture)
            {
                FSlateBrush NewBrush;
                NewBrush.SetResourceObject(DefaultTexture);
                NewBrush.ImageSize = FVector2D(64.0f, 64.0f);
                NewBrush.DrawAs = ESlateBrushDrawType::Image;
                NewBrush.Tiling = ESlateBrushTileType::NoTile;
                NewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
                
                ItemThumbnail->SetBrush(NewBrush);
                ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
            }
        }
        
        // Force layout update
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ItemThumbnail->Slot))
        {
            CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
            CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            CanvasSlot->SetSize(FVector2D(64.0f, 64.0f));
            CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
            CanvasSlot->SetZOrder(10);
        }
        else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ItemThumbnail->Slot))
        {
            // For overlay slots, ensure proper alignment and padding
            OverlaySlot->SetHorizontalAlignment(HAlign_Center);
            OverlaySlot->SetVerticalAlignment(VAlign_Center);
            OverlaySlot->SetPadding(FMargin(4.0f, 4.0f, 4.0f, 4.0f));
        }
        
        // Force size and ensure it's renderable
        ItemThumbnail->SetDesiredSizeOverride(FVector2D(48.0f, 48.0f));
        ItemThumbnail->SetRenderScale(FVector2D(1.0f, 1.0f));
        ItemThumbnail->SetRenderOpacity(1.0f);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Slot %d: ItemThumbnail is null!"), SlotIndex);
    }
    
    // Update stack text
    if (SlotText)
    {
        // Only show stack number if stackable and greater than 1
        if (NewItemInfo->bIsStackable && NewItemInfo->StackNumber > 1)
        {
            // Prevent stacking beyond 99
            int32 StackNumber = FMath::Min(NewItemInfo->StackNumber, 99);
            SlotText->SetText(FText::AsNumber(StackNumber));
            SlotText->SetVisibility(ESlateVisibility::Visible);
            
            // Force text layout
            if (UCanvasPanelSlot* TextSlot = Cast<UCanvasPanelSlot>(SlotText->Slot))
            {
                TextSlot->SetAnchors(FAnchors(0.9f, 0.9f, 0.9f, 0.9f));
                TextSlot->SetAlignment(FVector2D(1.0f, 1.0f));
                TextSlot->SetSize(FVector2D(40.0f, 20.0f));
                TextSlot->SetPosition(FVector2D(-5.0f, -5.0f));
                TextSlot->SetZOrder(20);
            }
        }
        else
        {
            SlotText->SetText(FText::GetEmpty());
            SlotText->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    
    // Final visibility check
    if (ItemThumbnail)
    {
        ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
        SetVisibility(ESlateVisibility::Visible);
        
        // Add a delayed visibility check to catch any late overrides
        if (UWorld* World = GetWorld())
        {
            FTimerHandle VisibilityCheckTimer;
            World->GetTimerManager().SetTimer(
                VisibilityCheckTimer,
                [this]() {
                    if (IsValid(this) && ItemThumbnail && !SlotEmpty && inventoryItemInfoRef)
                    {
                        ESlateVisibility CurrentVisibility = ItemThumbnail->GetVisibility();
                        if (CurrentVisibility != ESlateVisibility::Visible)
                        {
                            ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
                        }
                    }
                },
                0.1f, 
                false
            );
        }
    }
}

void UWBP_InventorySlot::ClearSlot()
{
    // Clear item reference
    inventoryItemInfoRef = nullptr;
    SlotEmpty = true;
    
    // Clear thumbnail if it exists
    if (ItemThumbnail)
    {
        ItemThumbnail->SetBrush(FSlateBrush());
        ItemThumbnail->SetVisibility(ESlateVisibility::Hidden);
    }
    
    // Clear stack text if it exists
    if (SlotText)
    {
        SlotText->SetText(FText::GetEmpty());
        SlotText->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UWBP_InventorySlot::UpdateStackNumber(int32 NewStackNumber)
{
    if (SlotText)
    {
        if (NewStackNumber > 0)
        {
            SlotText->SetVisibility(ESlateVisibility::Visible);
            SlotText->SetText(FText::FromString(FString::FromInt(NewStackNumber)));
        }
        else
        {
            SlotText->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    
    // Update the item info
    if (IsValidItemInfo())
    {
        FStructure_ItemInfo NewItemInfo = ItemInfo;
        NewItemInfo.StackNumber = NewStackNumber;
        UBP_ItemInfo* NewItemInfoObj = NewObject<UBP_ItemInfo>();
        NewItemInfoObj->CopyFromStructure(NewItemInfo);
        UpdateSlot(NewItemInfoObj);
    }
}

FText UWBP_InventorySlot::GetStackNumberText()
{
    // Check if we have a valid item info reference
    if (!IsValid(inventoryItemInfoRef))
    {
        return FText::GetEmpty();
    }

    // Get the item table row
    bool bFound;
    FStructure_ItemInfo ItemTableRow;
    inventoryItemInfoRef->GetItemTableRow(bFound, ItemTableRow);

    if (!bFound)
    {
        return FText::GetEmpty();
    }

    // Check if it's an equip type item
    if (ItemTableRow.ItemType == EItemType::Equip)
    {
        // Check if the item is equipped
        if (inventoryItemInfoRef->Equipped)
        {
            // Set font size to 11
            if (SlotText)
            {
                FSlateFontInfo FontInfo;
                FontInfo.FontObject = LoadObject<UObject>(nullptr, TEXT("/Game/AtlantisEons/Sources/Fonts/Aviano_Future_Bold_Font.Aviano_Future_Bold_Font"));
                FontInfo.Size = 11.0f;
                SlotText->SetFont(FontInfo);
            }
            return FText::FromString(TEXT("Equipped"));
        }
    }

    // Check stack number
    int32 StackNum = inventoryItemInfoRef->StackNumber;
    if (StackNum > 1)
    {
        // Set font size to 11
        if (SlotText)
        {
            FSlateFontInfo FontInfo;
            FontInfo.FontObject = LoadObject<UObject>(nullptr, TEXT("/Game/AtlantisEons/Sources/Fonts/Aviano_Future_Bold_Font.Aviano_Future_Bold_Font"));
            FontInfo.Size = 11.0f;
            SlotText->SetFont(FontInfo);
        }
        return FText::AsNumber(StackNum);
    }

    return FText::GetEmpty();
}

void UWBP_InventorySlot::SetItemThumb_Implementation(const TSoftObjectPtr<UTexture2D>& ItemThumb)
{
    if (!ItemThumbnail)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetItemThumb: ItemThumbnail widget is null for slot %d"), SlotIndex);
        return;
    }

    // Try to load the thumbnail
    UTexture2D* LoadedTexture = nullptr;
    
    // First try the provided thumbnail
    if (!ItemThumb.IsNull())
    {
        LoadedTexture = ItemThumb.LoadSynchronous();
        if (LoadedTexture)
        {
            UE_LOG(LogTemp, Display, TEXT("SetItemThumb: Successfully loaded provided thumbnail for slot %d"), SlotIndex);
        }
    }
    
    // If we couldn't load the provided thumbnail, try alternate paths
    if (!LoadedTexture)
    {
        TArray<FString> PossiblePaths = {
            FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_Item_%d"), SlotIndex),
            TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_BasicHealingPotion"),
            TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_DefaultItem"),
            TEXT("/Engine/EditorResources/S_Actor")
        };
        
        for (const FString& Path : PossiblePaths)
        {
            LoadedTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *Path));
            if (LoadedTexture)
            {
                UE_LOG(LogTemp, Warning, TEXT("SetItemThumb: Using thumbnail from path: %s for slot %d"), *Path, SlotIndex);
                break;
            }
        }
    }
    
    // If we still don't have a texture, use the engine default
    if (!LoadedTexture)
    {
        LoadedTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Engine/EditorResources/S_Actor")));
        UE_LOG(LogTemp, Warning, TEXT("SetItemThumb: Using engine default thumbnail for slot %d"), SlotIndex);
    }
    
    // Apply the texture
    if (LoadedTexture)
    {
        FSlateBrush NewBrush;
        NewBrush.SetResourceObject(LoadedTexture);
        NewBrush.ImageSize = FVector2D(64.0f, 64.0f);
        NewBrush.DrawAs = ESlateBrushDrawType::Image;
        NewBrush.Tiling = ESlateBrushTileType::NoTile;
        NewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
        ItemThumbnail->SetBrush(NewBrush);
        ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
        
        // Force layout update for special slots
        if (SlotIndex >= 10 && SlotIndex <= 15)
        {
            if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ItemThumbnail->Slot))
            {
                OverlaySlot->SetHorizontalAlignment(HAlign_Center);
                OverlaySlot->SetVerticalAlignment(VAlign_Center);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SetItemThumb: Failed to load any thumbnail for slot %d"), SlotIndex);
    }
}

void UWBP_InventorySlot::MakeSlotClean_Implementation()
{
    ClearSlot();
}

void UWBP_InventorySlot::RemoveItemDescription_Implementation()
{
    if (WidgetItemDescriptionRef)
    {
        WidgetItemDescriptionRef->RemoveFromParent();
        WidgetItemDescriptionRef = nullptr;
    }
    OnDescription = false;
}

void UWBP_InventorySlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (!SlotEmpty)
    {
        FVector2D MousePosition = InMouseEvent.GetScreenSpacePosition();
        UpdateItemDescription(ItemInfo, MousePosition);
    }
}

void UWBP_InventorySlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    RemoveItemDescription();
}

FReply UWBP_InventorySlot::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (MouseLeftDown)
    {
        // Calculate the distance moved since mouse down
        FVector2D currentPos = InMouseEvent.GetScreenSpacePosition();
        FVector2D delta = currentPos - MouseButtonDownPosition;
        float distance = FVector2D::Distance(currentPos, MouseButtonDownPosition);

        if (distance > 5.0f)
        {
            // Remove item description and reset mouse state
            RemoveItemDescription();
            MouseLeftDown = false;

            if (!SlotEmpty)
            {
                // Return handled with drag detection
                FReply Reply = FReply::Handled();
                FEventReply EventReply = UWidgetBlueprintLibrary::DetectDragIfPressed(FPointerEvent(), this, EKeys::LeftMouseButton);
                return EventReply.NativeReply;
            }
        }
    }

    return FReply::Unhandled();
}

void UWBP_InventorySlot::ShowContextMenuAtMousePosition()
{
    // Ensure we have the correct widget class
    if (!ContextMenuWidgetClass)
    {
        ContextMenuWidgetClass = LoadClass<UWBP_ContextMenu>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/WBP_ContextMenu.WBP_ContextMenu_C"));
        if (!ContextMenuWidgetClass)
        {
            UE_LOG(LogTemp, Error, TEXT("Slot %d: Failed to load WBP_ContextMenu Blueprint class"), SlotIndex);
            ContextMenuWidgetClass = UWBP_ContextMenu::StaticClass();
        }
    }
    
    // Create context menu if it doesn't exist
    if (!ContextMenuWidget && ContextMenuWidgetClass)
    {
        ContextMenuWidget = CreateWidget<UWBP_ContextMenu>(GetWorld(), ContextMenuWidgetClass);
        if (ContextMenuWidget)
        {
            // Set up the context menu references
            ContextMenuWidget->InventorySlotRef = this;
            ContextMenuWidget->MenuAnchorRef = ContextMenuAnchor;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Slot %d: Failed to create context menu widget"), SlotIndex);
            return;
        }
    }
    else if (ContextMenuWidget)
    {
        // Ensure references are still set correctly
        ContextMenuWidget->InventorySlotRef = this;
        ContextMenuWidget->MenuAnchorRef = ContextMenuAnchor;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Slot %d: ContextMenuWidgetClass is null - cannot create context menu"), SlotIndex);
        return;
    }

    if (ContextMenuWidget)
    {
        // Remove from viewport first if it's already there to avoid the warning
        if (ContextMenuWidget->IsInViewport())
        {
            ContextMenuWidget->RemoveFromParent();
        }
        
        // Get current mouse position
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController)
        {
            FVector2D MousePosition;
            PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y);
            
            // Add to viewport with high Z-order to ensure it appears on top
            ContextMenuWidget->AddToViewport(1000);
            
            // Set position in viewport
            ContextMenuWidget->SetPositionInViewport(MousePosition, false);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Slot %d: Could not get PlayerController for mouse position"), SlotIndex);
        }
    }
}

UUserWidget* UWBP_InventorySlot::GetUserMenuContent()
{
    UE_LOG(LogTemp, Warning, TEXT("Slot %d: GetUserMenuContent() called"), SlotIndex);
    
    // Ensure we have the correct widget class
    if (!ContextMenuWidgetClass)
    {
        ContextMenuWidgetClass = LoadClass<UWBP_ContextMenu>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/WBP_ContextMenu.WBP_ContextMenu_C"));
        if (!ContextMenuWidgetClass)
        {
            UE_LOG(LogTemp, Error, TEXT("Slot %d: Failed to load WBP_ContextMenu Blueprint class"), SlotIndex);
            ContextMenuWidgetClass = UWBP_ContextMenu::StaticClass();
        }
    }
    
    // Create context menu if it doesn't exist
    if (!ContextMenuWidget && ContextMenuWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Slot %d: Creating new context menu widget using class: %s"), SlotIndex, *ContextMenuWidgetClass->GetName());
        ContextMenuWidget = CreateWidget<UWBP_ContextMenu>(GetWorld(), ContextMenuWidgetClass);
        if (ContextMenuWidget)
        {
            UE_LOG(LogTemp, Warning, TEXT("Slot %d: Context menu widget created successfully - Class: %s"), SlotIndex, *ContextMenuWidget->GetClass()->GetName());
            // Set up the context menu
            ContextMenuWidget->InventorySlotRef = this;
            ContextMenuWidget->MenuAnchorRef = ContextMenuAnchor;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Slot %d: Failed to create context menu widget"), SlotIndex);
        }
    }
    else if (ContextMenuWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Slot %d: Using existing context menu widget - Class: %s"), SlotIndex, *ContextMenuWidget->GetClass()->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Slot %d: ContextMenuWidgetClass is null!"), SlotIndex);
    }

    return ContextMenuWidget;
}

void UWBP_InventorySlot::OnContextMenuUse()
{
    if (!SlotEmpty && inventoryItemInfoRef)
    {
        // Implement your use item logic here
        // For example:
        // inventoryItemInfoRef->UseItem();
        if (ContextMenuAnchor)
        {
            ContextMenuAnchor->Close();
        }
    }
}

void UWBP_InventorySlot::OnContextMenuThrow()
{
    // Broadcast the throw event with the item info reference and this inventory slot
    if (inventoryItemInfoRef)
    {
        ContextMenuClickThrow.Broadcast(inventoryItemInfoRef, this);
    }
}

ESlateVisibility UWBP_InventorySlot::GetItemThumbnailVisibility() const
{
    // Check if the slot is empty
    if (SlotEmpty)
    {
        return ESlateVisibility::Hidden;
    }
    
    return ESlateVisibility::Visible;
}

void UWBP_InventorySlot::MouseLeftButtonDown()
{
    if (!SlotEmpty && inventoryItemInfoRef)
    {
        // Only show context menu for inventory slots, not equipment slots
        if (SlotType == EInventorySlotType::Inventory)
        {
            // Create and show context menu directly at mouse position
            ShowContextMenuAtMousePosition();
        }
        
        // Always broadcast the slot clicked event for other systems that might need it
        OnSlotClicked.Broadcast(SlotIndex);
    }
}

void UWBP_InventorySlot::MouseRightButtonDown()
{
    // Right click now just removes item description
    // Context menu is now on left click
    RemoveItemDescription();
}

void UWBP_InventorySlot::OnAssetLoaded()
{
    if (ItemThumbnail && !ItemInfo.ItemThumbnail.IsNull())
    {
        if (UTexture2D* LoadedTexture = ItemInfo.ItemThumbnail.LoadSynchronous())
        {
            ItemThumbnail->SetBrushFromTexture(LoadedTexture);
            ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
        }
    }
}

void UWBP_InventorySlot::UpdateItemDescription(const FStructure_ItemInfo& ItemInfo, const FVector2D& MousePosition)
{
    if (!ItemDescriptionClass) return;

    if (!ItemDescription)
    {
        ItemDescription = CreateWidget<UWBP_ItemDescription>(GetWorld(), ItemDescriptionClass);
    }

    if (ItemDescription)
    {
        UpdateDescriptionStats(ItemDescription, ItemInfo);
        SetDescriptionPosition(MousePosition);
        
        // Only add to viewport if it's not already there
        if (!ItemDescription->IsInViewport())
        {
            ItemDescription->AddToViewport(100);
        }
    }
}

void UWBP_InventorySlot::SetDescriptionPosition(const FVector2D& MousePosition)
{
    if (!ItemDescription) return;

    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);

    FVector2D DescriptionSize = ItemDescription->GetDesiredSize();
    FVector2D NewPosition = MousePosition;

    // Adjust position if description would go off screen
    if (NewPosition.X + DescriptionSize.X > ViewportSize.X)
    {
        NewPosition.X = ViewportSize.X - DescriptionSize.X;
    }
    if (NewPosition.Y + DescriptionSize.Y > ViewportSize.Y)
    {
        NewPosition.Y = ViewportSize.Y - DescriptionSize.Y;
    }

    ItemDescription->SetPositionInViewport(NewPosition, false);
}

FString UWBP_InventorySlot::GetStatString(const FString& Prefix, int32 Value) const
{
    return FString::Printf(TEXT("%s: %d"), *Prefix, Value);
}

void UWBP_InventorySlot::UpdateDescriptionStats(UWBP_ItemDescription* Description, const FStructure_ItemInfo& ItemInfo)
{
    if (!Description) return;

    // Update description stats based on item info
    // Implementation depends on your specific needs
}

bool UWBP_InventorySlot::IsValidItemInfo()
{
    return inventoryItemInfoRef != nullptr;
}

void UWBP_InventorySlot::UpdateStackText(int32 NewStackNumber)
{
    if (SlotText)
    {
        if (IsValidItemInfo())
        {
            SlotText->SetText(NewStackNumber > 1 ? FText::AsNumber(NewStackNumber) : FText::GetEmpty());
        }
        else
        {
            SlotText->SetText(FText::GetEmpty());
        }
    }
}

void UWBP_InventorySlot::CreateDefaultComponents()
{
    // DON'T create our own widgets - find the ones that already exist from Blueprint
    // The Blueprint should already have created the necessary widgets
    
    // Look for existing ItemThumbnail in the widget tree
    if (!ItemThumbnail)
    {
        // Try to find by name in the widget tree
        TArray<UWidget*> AllChildren;
        if (WidgetTree)
        {
            WidgetTree->GetAllWidgets(AllChildren);
        }
        
        for (UWidget* Child : AllChildren)
        {
            // Look for Image widgets that could be our thumbnail
            if (UImage* ImageWidget = Cast<UImage>(Child))
            {
                FString WidgetName = Child->GetName();
                if (WidgetName.Contains("Thumbnail") || WidgetName.Contains("Image") || WidgetName.Contains("Icon"))
                {
                    ItemThumbnail = ImageWidget;
                    break;
                }
            }
        }
        
        // If still not found, look for any Image widget
        if (!ItemThumbnail)
        {
            for (UWidget* Child : AllChildren)
            {
                if (UImage* ImageWidget = Cast<UImage>(Child))
                {
                    ItemThumbnail = ImageWidget;
                    break;
                }
            }
        }
    }
    
    // Look for existing SlotText in the widget tree
    if (!SlotText)
    {
        TArray<UWidget*> AllChildren;
        if (WidgetTree)
        {
            WidgetTree->GetAllWidgets(AllChildren);
        }
        
        for (UWidget* Child : AllChildren)
        {
            // Look for TextBlock widgets that could be our text
            if (UTextBlock* TextWidget = Cast<UTextBlock>(Child))
            {
                FString WidgetName = Child->GetName();
                if (WidgetName.Contains("Text") || WidgetName.Contains("Number") || WidgetName.Contains("Stack"))
                {
                    SlotText = TextWidget;
                    break;
                }
            }
        }
        
        // If still not found, look for any TextBlock widget  
        if (!SlotText)
        {
            for (UWidget* Child : AllChildren)
            {
                if (UTextBlock* TextWidget = Cast<UTextBlock>(Child))
                {
                    SlotText = TextWidget;
                    break;
                }
            }
        }
    }
}

bool UWBP_InventorySlot::IsEquipmentType(const FStructure_ItemInfo& ItemInfo)
{
    return ItemInfo.ItemType == EItemType::Equip;
}

bool UWBP_InventorySlot::GetItemTableRow(bool& bFound, FStructure_ItemInfo& OutRow)
{
    // Get the data table
    static const FString TablePath = TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList");
    UDataTable* ItemDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
    
    if (!ItemDataTable)
    {
        bFound = false;
        return false;
    }

    // Create the row name with proper format
    FString RowName = FString::Printf(TEXT("Item_%d"), FMath::Max(0, ItemInfo.ItemIndex));
    
    // Use FindRowUnchecked to avoid structure type mismatch errors
    void* RowData = ItemDataTable->FindRowUnchecked(*RowName);
    if (RowData)
    {
        // Create fallback data since we can't properly read the structure
        OutRow = FStructure_ItemInfo();
        OutRow.ItemIndex = ItemInfo.ItemIndex;
        OutRow.ItemName = FString::Printf(TEXT("Item %d"), ItemInfo.ItemIndex);
        OutRow.ItemDescription = FString::Printf(TEXT("Description for item %d"), ItemInfo.ItemIndex);
        OutRow.bIsValid = true;
        OutRow.bIsStackable = true;
        OutRow.StackNumber = 1;
        OutRow.Price = 100 * (ItemInfo.ItemIndex + 1);
        
        // Try to load thumbnail from correct path
        FString ThumbnailPath = FString::Printf(TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/IMG_Item_%d"), ItemInfo.ItemIndex);
        OutRow.ItemThumbnail = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(*ThumbnailPath));
        
        bFound = true;
        return true;
    }
    
    bFound = false;
    return false;
}

void UWBP_InventorySlot::TestPopulateSlot()
{
    // Only proceed for slots 10-15
    if (SlotIndex >= 10 && SlotIndex <= 15)
    {
        UE_LOG(LogTemp, Warning, TEXT("TEST: Explicitly populating slot %d"), SlotIndex);
        
        // Create a test item
        UBP_ItemInfo* TestItem = NewObject<UBP_ItemInfo>();
        if (TestItem)
        {
            // Set basic properties
            TestItem->ItemName = FString::Printf(TEXT("Test Item %d"), SlotIndex);
            TestItem->bIsStackable = true;
            TestItem->StackNumber = 1; // Set to 1 to avoid stacking
            TestItem->ItemIndex = SlotIndex;
            
            // Load test texture and force it to be visible
            UTexture2D* TestTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Engine/EditorResources/S_Actor")));
            if (TestTexture)
            {
                TestItem->Thumbnail = TestTexture;
                UE_LOG(LogTemp, Warning, TEXT("TEST: Loaded test texture for slot %d"), SlotIndex);
                
                // Force slot to be non-empty
                SlotEmpty = false;
                inventoryItemInfoRef = TestItem;
                
                // Create components if needed
                CreateDefaultComponents();
                
                // Update the slot visuals
                if (ItemThumbnail)
                {
                    FSlateBrush NewBrush;
                    NewBrush.SetResourceObject(TestTexture);
                    NewBrush.ImageSize = FVector2D(64.0f, 64.0f);
                    NewBrush.DrawAs = ESlateBrushDrawType::Image;
                    NewBrush.Tiling = ESlateBrushTileType::NoTile;
                    NewBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
                    ItemThumbnail->SetBrush(NewBrush);
                    ItemThumbnail->SetVisibility(ESlateVisibility::Visible);
                    
                    UE_LOG(LogTemp, Warning, TEXT("TEST: Set thumbnail for slot %d and forced visibility"), SlotIndex);
                    
                    // Force layout update
                    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ItemThumbnail->Slot))
                    {
                        CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
                        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
                        CanvasSlot->SetSize(FVector2D(64.0f, 64.0f));
                        CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
                        CanvasSlot->SetZOrder(10);
                    }
                }
                
                // Update stack text
                if (SlotText)
                {
                    SlotText->SetText(FText::AsNumber(1));
                    SlotText->SetVisibility(ESlateVisibility::Visible);
                    UE_LOG(LogTemp, Warning, TEXT("TEST: Set stack text for slot %d"), SlotIndex);
                }
                
                // Force a layout pass
                ForceLayoutPrepass();
                
                UE_LOG(LogTemp, Warning, TEXT("TEST: Completed populating slot %d with test item"), SlotIndex);
            }
        }
    }
}
