#include "BP_Item.h"
#include "WBP_ItemNamePopUp.h"
#include "AtlantisEonsGameInstance.h"
#include "AtlantisEonsCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Blueprint/UserWidget.h"
#include "StoreSystemFix.h"

ABP_Item::ABP_Item()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create and setup the static mesh component
    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    RootComponent = ItemMesh;

    // Create and setup the sphere collision
    SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
    SphereCollision->SetupAttachment(RootComponent);
    SphereCollision->SetSphereRadius(100.0f);
    SphereCollision->SetCollisionProfileName(TEXT("OverlapAll"));

    // Initialize variables
    ItemIndex = 0;
    StackNumber = 0;

    // Load the item data table
    static ConstructorHelpers::FObjectFinder<UDataTable> ItemDataTableObj(TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/Table_ItemList"));
    if (ItemDataTableObj.Succeeded())
    {
        ItemDataTable = ItemDataTableObj.Object;
    }
}

// Called when the game starts or when spawned
void ABP_Item::BeginPlay()
{
    Super::BeginPlay();

    // Bind overlap events
    SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &ABP_Item::OnSphereBeginOverlap);
    SphereCollision->OnComponentEndOverlap.AddDynamic(this, &ABP_Item::OnSphereEndOverlap);

    // Initialize with default values if needed
    InitializeItem(ItemIndex, StackNumber);
}

void ABP_Item::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update item name widget position if it exists
    if (ItemNameWidget)
    {
        UpdateItemNamePosition();
    }
}

void ABP_Item::InitializeItem(int32 NewItemIndex, int32 NewStackNumber)
{
    ItemIndex = NewItemIndex;
    StackNumber = NewStackNumber;
    
    UE_LOG(LogTemp, Display, TEXT("%s: InitializeItem - ItemIndex=%d, StackNumber=%d"), *GetName(), ItemIndex, StackNumber);
    
    // Use the new store system to get proper item data
    if (UStoreSystemFix::GetItemData(ItemIndex, ItemInfo))
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Successfully loaded item data for %d: %s"), *GetName(), ItemIndex, *ItemInfo.ItemName);
        
        // Update stack number
        ItemInfo.StackNumber = StackNumber;
        
        // Try to load the 3D mesh from the item data
        if (!ItemInfo.StaticMeshID.IsNull())
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Attempting to load mesh: %s"), *GetName(), *ItemInfo.StaticMeshID.ToString());
            
            UStaticMesh* LoadedMesh = ItemInfo.StaticMeshID.LoadSynchronous();
            if (LoadedMesh)
            {
                ItemMesh->SetStaticMesh(LoadedMesh);
                UE_LOG(LogTemp, Display, TEXT("%s: ✅ Successfully set mesh from data table: %s"), *GetName(), *LoadedMesh->GetName());
                return; // Success! No need for fallbacks
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("%s: ❌ Failed to load mesh from data table path: %s"), *GetName(), *ItemInfo.StaticMeshID.ToString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: ❌ No StaticMeshID in data table for item %d"), *GetName(), ItemIndex);
        }
        
        // If we reach here, the data table mesh failed - this should NOT happen with a complete data pipeline
        UE_LOG(LogTemp, Error, TEXT("%s: CRITICAL: Data table mesh failed for item %d (%s) - data pipeline incomplete!"), *GetName(), ItemIndex, *ItemInfo.ItemName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: ❌ Failed to load item data for %d - data pipeline broken!"), *GetName(), ItemIndex);
    }
    
    // If we reach here, something is wrong with the data pipeline
    UE_LOG(LogTemp, Error, TEXT("%s: FATAL: Complete data pipeline failure for item %d"), *GetName(), ItemIndex);
}

void ABP_Item::SetDefaultMesh()
{
    // REMOVED: We want complete data pipeline, no fallbacks
    UE_LOG(LogTemp, Error, TEXT("%s: SetDefaultMesh called - this should not happen with complete data pipeline"), *GetName());
}

void ABP_Item::SetDefaultItemInfo()
{
    // REMOVED: We want complete data pipeline, no fallbacks  
    UE_LOG(LogTemp, Error, TEXT("%s: SetDefaultItemInfo called - this should not happen with complete data pipeline"), *GetName());
}

void ABP_Item::HideItemNamePopUp()
{
	if (ItemNameWidget)
	{
		ItemNameWidget->PlayHideAnimation();
		ItemNameWidget->RemoveFromParent();
		ItemNameWidget = nullptr;
	}
}

void ABP_Item::OnMeshLoaded()
{
    // Get item data from data table
    if (ItemDataTable)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: OnMeshLoaded - Setting up mesh for item %d"), *GetName(), ItemIndex);
        
        // Mesh should be loaded from the item info that was already set
        if (!ItemInfo.StaticMeshID.IsNull())
        {
            UStaticMesh* LoadedMesh = ItemInfo.StaticMeshID.Get();
            if (LoadedMesh)
            {
                UE_LOG(LogTemp, Display, TEXT("%s: Setting mesh: %s"), *GetName(), *LoadedMesh->GetName());
                ItemMesh->SetStaticMesh(LoadedMesh);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("%s: Failed to load mesh for item %d"), *GetName(), ItemIndex);
            }
        }
    }
}

void ABP_Item::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(OtherActor);
    if (Character)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Player entered item overlap area"), *GetName());
        ShowItemNamePopUp();
    }
}

void ABP_Item::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    AAtlantisEonsCharacter* Character = Cast<AAtlantisEonsCharacter>(OtherActor);
    if (Character)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Player left item overlap area"), *GetName());
        HideItemNamePopUp();
    }
}

void ABP_Item::RemoveItemNameWidget()
{
    if (ItemNameWidget)
    {
        ItemNameWidget->RemoveFromParent();
        ItemNameWidget = nullptr;
    }
}

void ABP_Item::SpawnItem(int32 NewItemIndex, int32 NewStackNumber)
{
    ItemIndex = NewItemIndex;
    StackNumber = NewStackNumber;

    // Get item data from data table
    if (ItemDataTable)
    {
        FStructure_ItemInfo* ItemInfo = ItemDataTable->FindRow<FStructure_ItemInfo>(*FString::FromInt(ItemIndex), TEXT(""));
        if (ItemInfo)
        {
            // Load and set the static mesh
            if (!ItemInfo->StaticMeshID.IsNull())
            {
                StreamableManager.RequestAsyncLoad(ItemInfo->StaticMeshID.ToSoftObjectPath(),
                    FStreamableDelegate::CreateUObject(this, &ABP_Item::OnMeshLoaded));
            }
        }
    }
}

void ABP_Item::SetupItemMesh()
{
    if (ItemDataTable)
    {
        FString RowName = FString::FromInt(ItemIndex);
        Structure_ItemInfo* ItemData = ItemDataTable->FindRow<Structure_ItemInfo>(FName(*RowName), TEXT(""));
        if (ItemData)
        {
            // Load and set the static mesh
            if (!ItemData->StaticMeshID.IsNull())
            {
                ItemMesh->SetStaticMesh(ItemData->StaticMeshID.Get());
            }
        }
    }
}

void ABP_Item::ShowItemNamePopUp()
{
    if (!ItemNameWidget && GetWorld())
    {
        APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PlayerController)
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Creating item name popup widget"), *GetName());
            
            // Try to load the widget class - use Class LoadObject instead of LoadClass for better error reporting
            UClass* WidgetClass = LoadObject<UClass>(nullptr, TEXT("/Game/AtlantisEons/Blueprints/InventoryandEquipment/WBP_ItemNamePopUp.WBP_ItemNamePopUp_C"));
            
            if (!WidgetClass)
            {
                // Try alternate paths
                UE_LOG(LogTemp, Warning, TEXT("%s: First path failed, trying alternate path"), *GetName());
                WidgetClass = LoadObject<UClass>(nullptr, TEXT("/Game/AtlantisEons/UI/WBP_ItemNamePopUp.WBP_ItemNamePopUp_C"));
            }
            
            if (!WidgetClass)
            {
                // Try one more path variation
                UE_LOG(LogTemp, Warning, TEXT("%s: Second path failed, trying one more path"), *GetName());
                WidgetClass = LoadObject<UClass>(nullptr, TEXT("/Game/AtlantisEons/WBP_ItemNamePopUp.WBP_ItemNamePopUp_C"));
            }
            
            if (WidgetClass)
            {
                UE_LOG(LogTemp, Display, TEXT("%s: Successfully loaded widget class: %s"), *GetName(), *WidgetClass->GetName());
                
                // Create the widget with the loaded class
                ItemNameWidget = CreateWidget<UWBP_ItemNamePopUp>(PlayerController, WidgetClass);
                if (ItemNameWidget)
                {
                    FString DisplayName;
                    
                    // Use item info name if it's valid, otherwise use a default name
                    if (!ItemInfo.ItemName.IsEmpty())
                    {
                        DisplayName = ItemInfo.ItemName;
                        if (StackNumber > 1 || ItemInfo.StackNumber > 1)
                        {
                            int32 DisplayStack = (StackNumber > 1) ? StackNumber : ItemInfo.StackNumber;
                            DisplayName = FString::Printf(TEXT("%s [%d]"), *DisplayName, DisplayStack);
                        }
                    }
                    else
                    {
                        DisplayName = FString::Printf(TEXT("Item #%d"), ItemIndex);
                        if (StackNumber > 1)
                        {
                            DisplayName.Append(FString::Printf(TEXT(" [%d]"), StackNumber));
                        }
                    }
                    
                    UE_LOG(LogTemp, Display, TEXT("%s: Showing popup with name: %s"), *GetName(), *DisplayName);
                    
                    // Set the item name with increased visibility
                    ItemNameWidget->SetItemName(DisplayName);
                    
                    // Position the widget appropriately and ensure it's visible
                    ItemNameWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));  // Center alignment
                    ItemNameWidget->SetPositionInViewport(FVector2D(GEngine->GameViewport->Viewport->GetSizeXY()) / 2.0f);
                    ItemNameWidget->SetVisibility(ESlateVisibility::Visible);
                    ItemNameWidget->AddToViewport(999); // Higher ZOrder to ensure visibility
                    
                    // Force redraw to ensure visibility
                    ItemNameWidget->ForceLayoutPrepass();
                    
                    UE_LOG(LogTemp, Display, TEXT("%s: Item name widget added to viewport with high Z-order"), *GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("%s: Failed to create ItemNameWidget instance"), *GetName());
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("%s: Failed to load WBP_ItemNamePopUp class from any path"), *GetName());
                
                // Create a debug on-screen message
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
                    FString::Printf(TEXT("Failed to load item popup widget for: %s"), *GetName()));
            }
        }
    }
}

void ABP_Item::UpdateItemNamePosition()
{
    if (ItemNameWidget)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            FVector2D ScreenLocation;
            if (PC->ProjectWorldLocationToScreen(GetActorLocation(), ScreenLocation))
            {
                // Adjust Y position upward to make text more visible
                ScreenLocation.Y -= 50.0f;
                ItemNameWidget->SetPositionInViewport(ScreenLocation, true);
                
                // Make sure visibility is maintained
                ItemNameWidget->SetVisibility(ESlateVisibility::Visible);
            }
        }
    }
}

UTexture2D* ABP_Item::GetDefaultTexture()
{
    // First try to load a standard texture
    UTexture2D* DefaultTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorResources/S_Actor"));
    
    // Always create a colored texture as fallback
    if (!DefaultTexture)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load default texture, creating a colored texture"));
        
        // Create a simple colored texture
        const int32 TextureSize = 64;
        DefaultTexture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8);
        
        if (DefaultTexture)
        {
            // Fill with a blue color for health potions
            FColor DefaultColor = FColor(50, 100, 255, 255); // Blue color
            
            // Lock the texture for writing
            uint8* MipData = static_cast<uint8*>(DefaultTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
            
            // Fill all pixels with the same color
            for (int32 y = 0; y < TextureSize; ++y)
            {
                for (int32 x = 0; x < TextureSize; ++x)
                {
                    int32 offset = (y * TextureSize + x) * 4;
                    MipData[offset] = DefaultColor.B;     // Blue
                    MipData[offset + 1] = DefaultColor.G; // Green
                    MipData[offset + 2] = DefaultColor.R; // Red
                    MipData[offset + 3] = DefaultColor.A; // Alpha
                }
            }
            
            // Unlock the texture
            DefaultTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
            
            // Update the texture resource
            DefaultTexture->UpdateResource();
            
            UE_LOG(LogTemp, Display, TEXT("Created default blue texture for items"));
        }
    }
    
    // If we still don't have a texture (very unlikely), create a simple colored texture
    if (!DefaultTexture)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create transient texture, using emergency fallback"));
        
        // Create an even simpler texture as last resort
        const int32 EmergencySize = 16;
        DefaultTexture = UTexture2D::CreateTransient(EmergencySize, EmergencySize, PF_B8G8R8A8);
        
        if (DefaultTexture)
        {
            // Use a bright red for emergency visibility
            FColor EmergencyColor = FColor::Red;
            
            // Fill with a simple solid color
            uint8* EmergencyData = static_cast<uint8*>(DefaultTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
            
            // Fill the texture with red
            FMemory::Memset(EmergencyData, 255, EmergencySize * EmergencySize * 4);
            
            // Unlock the texture
            DefaultTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
            
            // Update the texture resource
            DefaultTexture->UpdateResource();
            
            UE_LOG(LogTemp, Warning, TEXT("Created emergency red texture for items"));
        }
    }
    
    return DefaultTexture;
}

void ABP_Item::ShowItemName(bool bShow)
{
    if (bShow)
    {
        ShowItemNamePopUp();
    }
    else
    {
        HideItemNamePopUp();
    }
}

