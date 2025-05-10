#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"
#include "AtlantisEonsCharacter.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "AtlantisEonsHUD.generated.h"

UCLASS()
class ATLANTISEONS_API AAtlantisEonsHUD : public AHUD
{
    GENERATED_BODY()

public:
    AAtlantisEonsHUD();

    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> MainMenuWidgetClass;

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ResumeGame();
    
    /**
     * Safe version of ResumeGame for use with UI buttons to prevent multiple firings.
     * If you're binding this to a button's OnClicked event, use this instead of ResumeGame.
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ResumeGameButtonHandler();
    
    /** Force close inventory and resume game - specifically for inventory's resume button */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ForceCloseInventoryAndResume();
    
    /**
     * Universal resume function that handles both main menu and inventory cases
     * This is the only function you need to bind to ANY resume button
     * @param SourceWidgetClass The widget class of the calling widget (to determine context)
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UniversalResumeGame(TSubclassOf<UUserWidget> SourceWidgetClass);
    
    /**
     * CRITICAL: Use this function directly in the Resume button click event.
     * This ensures the main menu and inventory are closed in one operation.
     * This function is designed to fix issues with the Resume button requiring multiple clicks.
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void CompleteGameResume();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowMainMenu();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleMainMenu();

    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsMainMenuVisible() const;

    /** Show the inventory widget */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    bool ShowInventoryWidget();
    
    /** Hide the inventory widget */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    void HideInventoryWidget();
    
    /** Check if the inventory widget is visible */
    UFUNCTION(BlueprintPure, Category = "UI|Inventory")
    bool IsInventoryWidgetVisible() const;
    
    /** Get the inventory widget class */
    UFUNCTION(BlueprintPure, Category = "UI|Inventory")
    TSubclassOf<UUserWidget> GetInventoryWidgetClass() const { return InventoryWidgetClass; }

protected:
    UPROPERTY()
    class UUserWidget* MainMenuWidget;
    
    /** The inventory widget class */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inventory", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> InventoryWidgetClass;
    
    /** The currently active inventory widget */
    UPROPERTY()
    class UUserWidget* InventoryWidget;

    void SetGameInputMode(bool bGameMode);

    bool bIsGameStarted;
    
    /** Flag to prevent multiple ResumeGame calls in a single frame */
    bool bIsProcessingResume;
    
    /** Timer handle for resume cooldown */
    FTimerHandle ResumeCooldownTimer;
};
