#include "WBP_Inventory.h"
#include "WBP_InventorySlot.h"
#include "WBP_ItemDescription.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "AtlantisEonsCharacter.h"
#include "AtlantisEonsHUD.h"
#include "Engine/TextureRenderTarget2D.h"

UWBP_Inventory::UWBP_Inventory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UWBP_Inventory::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    CreateInventoryGrid();
    CreateCloseButton();
    CreateStorageText();
}

void UWBP_Inventory::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Initialize with default 30 slots (5x6 grid)
    InitializeInventory(30);
    
    // Ensure all components are created
    CreateInventoryComponents();
}

void UWBP_Inventory::NativeDestruct()
{
    Super::NativeDestruct();
    
    // Clean up inventory data
    for (FStructure_ItemInfo& ItemInfo : InventoryData)
    {
        ItemInfo = FStructure_ItemInfo();
    }
    
    // Clear slot references
    for (UWBP_InventorySlot* Slot : InventorySlots)
    {
        if (Slot)
        {
            Slot->ClearSlot();
        }
    }
    InventorySlots.Empty();
    
    // Clear widget references
    InventoryGrid = nullptr;
    CloseButton = nullptr;
    StorageText = nullptr;
    ItemDescriptionWidget = nullptr;
}

void UWBP_Inventory::CreateInventoryGrid()
{
    InventoryGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    if (InventoryGrid)
    {
        // Add the grid to the root
        if (UPanelWidget* RootWidget = Cast<UPanelWidget>(GetRootWidget()))
        {
            RootWidget->AddChild(InventoryGrid);
        }
    }
}

void UWBP_Inventory::CreateCloseButton()
{
    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    if (CloseButton)
    {
        // Add the button to the root
        if (UPanelWidget* RootWidget = Cast<UPanelWidget>(GetRootWidget()))
        {
            RootWidget->AddChild(CloseButton);
            CloseButton->OnClicked.AddDynamic(this, &UWBP_Inventory::OnCloseButtonClicked);
        }
    }
}

void UWBP_Inventory::CreateStorageText()
{
    StorageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (StorageText)
    {
        // Add the text block to the root
        if (UPanelWidget* RootWidget = Cast<UPanelWidget>(GetRootWidget()))
        {
            RootWidget->AddChild(StorageText);
            UpdateStorageText();
        }
    }
}

void UWBP_Inventory::CreateInventoryComponents()
{
    // Create inventory grid if needed
    if (!InventoryGrid)
    {
        CreateInventoryGrid();
    }
    
    // Create close button if needed
    if (!CloseButton)
    {
        CreateCloseButton();
    }
    
    // Create storage text if needed
    if (!StorageText)
    {
        CreateStorageText();
    }
    
    // Ensure all components are properly laid out
    if (InventoryGrid)
    {
        InventoryGrid->SetVisibility(ESlateVisibility::Visible);
    }
}

void UWBP_Inventory::InitializeInventory(int32 SlotCount)
{
    // Clear existing slots
    InventorySlots.Empty();
    InventoryData.Empty();
    
    UE_LOG(LogTemp, Warning, TEXT("InitializeInventory: SEARCHING FOR EXISTING BLUEPRINT SLOTS"));
    
    // Instead of creating new slots, find existing Blueprint slots
    TArray<UWidget*> AllWidgets;
    if (WidgetTree)
    {
        WidgetTree->GetAllWidgets(AllWidgets);
    }
    
    // Find all existing inventory slot widgets
    TArray<UWBP_InventorySlot*> FoundSlots;
    for (UWidget* Widget : AllWidgets)
    {
        if (UWBP_InventorySlot* SlotWidget = Cast<UWBP_InventorySlot>(Widget))
        {
            UE_LOG(LogTemp, Warning, TEXT("InitializeInventory: Found existing slot widget: %s"), *Widget->GetName());
            FoundSlots.Add(SlotWidget);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("InitializeInventory: Found %d existing Blueprint slots"), FoundSlots.Num());
    
    // If we found existing slots, use them
    if (FoundSlots.Num() > 0)
    {
        // Sort slots by their position in the grid or by name
        FoundSlots.Sort([](const UWBP_InventorySlot& A, const UWBP_InventorySlot& B) {
            return A.GetName() < B.GetName();
        });
        
        // Use the first SlotCount slots
        int32 SlotsToUse = FMath::Min(SlotCount, FoundSlots.Num());
        for (int32 i = 0; i < SlotsToUse; ++i)
        {
            UWBP_InventorySlot* ExistingSlot = FoundSlots[i];
            
            // Set the slot index properly
            ExistingSlot->SetSlotIndex(i);
            
            // Add to our arrays
            InventorySlots.Add(ExistingSlot);
            InventoryData.Add(FStructure_ItemInfo());
            
            UE_LOG(LogTemp, Warning, TEXT("InitializeInventory: Using existing slot %d: %s"), i, *ExistingSlot->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeInventory: No existing slots found, creating new ones"));
        
        // Fallback: Create new slots if no existing ones found
        // Ensure we have a grid panel
        if (!InventoryGrid)
        {
            UE_LOG(LogTemp, Warning, TEXT("InventoryGrid not set, creating a new one"));
            CreateInventoryGrid();
        }
        
        // Make sure the grid is cleared
        if (InventoryGrid)
        {
            InventoryGrid->ClearChildren();
        }
        
        // Check if we have a valid slot class
        if (!InventorySlotClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("InventorySlotClass not set, using default UWBP_InventorySlot class"));
            InventorySlotClass = UWBP_InventorySlot::StaticClass();
        }
        
        // Calculate grid size (5x6 layout)
        const int32 GridColumns = 5;
        const int32 GridRows = FMath::CeilToInt(static_cast<float>(SlotCount) / GridColumns);
        
        // Create slots
        for (int32 i = 0; i < SlotCount; ++i)
        {
            UWBP_InventorySlot* NewSlot = CreateWidget<UWBP_InventorySlot>(this, InventorySlotClass);
            if (NewSlot)
            {
                // Add to grid
                const int32 Row = i / GridColumns;
                const int32 Column = i % GridColumns;
                InventoryGrid->AddChildToUniformGrid(NewSlot, Row, Column);
                
                // Initialize slot index
                NewSlot->SetSlotIndex(i);
                
                // Add to arrays
                InventorySlots.Add(NewSlot);
                InventoryData.Add(FStructure_ItemInfo());
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("InitializeInventory: Initialized %d slots"), InventorySlots.Num());
    
    // Update storage text
    UpdateStorageText();
}

void UWBP_Inventory::OnSlotClicked(int32 SlotIndex)
{
    // Handle slot click event
    if (SlotIndex >= 0 && SlotIndex < InventoryData.Num())
    {
        // Notify any bound delegates
        OnInventorySlotClicked.Broadcast(SlotIndex);
    }
}

void UWBP_Inventory::OnCloseButtonClicked()
{
    RemoveFromParent();
}

void UWBP_Inventory::UpdateStorageText()
{
    if (StorageText)
    {
        StorageText->SetText(GetStorageNumber());
    }
}

FText UWBP_Inventory::GetStorageNumber() const
{
    // Initialize storage current number
    int32 StorageCurrentNumber = 0;
    
    // Iterate through inventory slots
    for (const UWBP_InventorySlot* Slot : InventorySlots)
    {
        // Check if slot is empty
        if (Slot && !Slot->SlotEmpty)
        {
            // Increment storage current number
            ++StorageCurrentNumber;
        }
    }
    
    // Get total number of slots
    int32 TotalSlots = InventorySlots.Num();
    
    // Create the final text using FText::Format
    return FText::Format(NSLOCTEXT("Inventory", "StorageFormat", "STORAGE {0}/{1}"),
        FText::AsNumber(StorageCurrentNumber), FText::AsNumber(TotalSlots));
}


bool UWBP_Inventory::AddItem(const FStructure_ItemInfo& ItemInfo)
{
    // First try to find a stackable slot
    int32 StackableSlotIndex = FindStackableSlot(ItemInfo);
    if (StackableSlotIndex != -1)
    {
        // Update existing stack
        InventoryData[StackableSlotIndex].StackNumber += ItemInfo.StackNumber;
        if (InventorySlots.IsValidIndex(StackableSlotIndex) && InventorySlots[StackableSlotIndex])
        {
            UBP_ItemInfo* NewItemInfo = NewObject<UBP_ItemInfo>();
            NewItemInfo->CopyFromStructure(InventoryData[StackableSlotIndex]);
            InventorySlots[StackableSlotIndex]->UpdateSlot(NewItemInfo);
        }
        return true;
    }

    // If no stackable slot found, find an empty slot
    int32 EmptySlotIndex = FindEmptySlot();
    if (EmptySlotIndex != -1)
    {
        // Add item to inventory data
        InventoryData[EmptySlotIndex] = ItemInfo;

        // Update slot UI
        if (InventorySlots.IsValidIndex(EmptySlotIndex) && InventorySlots[EmptySlotIndex])
        {
            UBP_ItemInfo* NewItemInfo = NewObject<UBP_ItemInfo>();
            NewItemInfo->CopyFromStructure(ItemInfo);
            InventorySlots[EmptySlotIndex]->UpdateSlot(NewItemInfo);
        }
        return true;
    }

    return false;
}

bool UWBP_Inventory::RemoveItem(int32 SlotIndex, int32 Amount)
{
    if (!InventoryData.IsValidIndex(SlotIndex) || !InventoryData[SlotIndex].bIsValid)
    {
        return false;
    }

    // If amount is greater than or equal to current amount, clear the slot
    if (Amount >= InventoryData[SlotIndex].StackNumber)
    {
        InventoryData[SlotIndex] = FStructure_ItemInfo();
        if (InventorySlots.IsValidIndex(SlotIndex) && InventorySlots[SlotIndex])
        {
            InventorySlots[SlotIndex]->ClearSlot();
        }
    }
    else
    {
        // Reduce the amount
        InventoryData[SlotIndex].StackNumber -= Amount;
        if (InventorySlots.IsValidIndex(SlotIndex) && InventorySlots[SlotIndex])
        {
            UBP_ItemInfo* NewItemInfo = NewObject<UBP_ItemInfo>();
            NewItemInfo->CopyFromStructure(InventoryData[SlotIndex]);
            InventorySlots[SlotIndex]->UpdateSlot(NewItemInfo);
        }
    }

    return true;
}

int32 UWBP_Inventory::FindEmptySlot() const
{
    for (int32 i = 0; i < InventoryData.Num(); ++i)
    {
        if (!InventoryData[i].bIsValid)
        {
            return i;
        }
    }
    return -1;
}

int32 UWBP_Inventory::FindStackableSlot(const FStructure_ItemInfo& ItemInfo) const
{
    // Find a slot with the same item type that isn't full
    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (InventorySlots[i] && !InventorySlots[i]->SlotEmpty)
        {
            // Check if the slot has the same item and isn't at max stack
            if (InventoryData[i].ItemName == ItemInfo.ItemName &&
                InventoryData[i].StackNumber < ItemInfo.StackNumber)
            {
                return i;
            }
        }
    }

    return -1;
}

UWBP_InventorySlot* UWBP_Inventory::GetEmptySlot()
{
    // Find the first empty slot
    for (UWBP_InventorySlot* Slot : InventorySlots)
    {
        if (Slot && Slot->SlotEmpty)
        {
            return Slot;
        }
    }

    return nullptr;
}

const TArray<UWBP_InventorySlot*>& UWBP_Inventory::GetInventorySlotWidgets() const
{
    static TArray<UWBP_InventorySlot*> EmptyArray;
    
    // Check if slots array is initialized
    if (InventorySlots.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetInventorySlotWidgets: InventorySlots array is empty!"));
        return EmptyArray;
    }
    
    // Validate each slot
    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (!InventorySlots[i])
        {
            UE_LOG(LogTemp, Warning, TEXT("GetInventorySlotWidgets: Null slot at index %d"), i);
        }
    }
    
    return InventorySlots;
}

