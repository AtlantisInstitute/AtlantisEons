#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/MenuAnchor.h"
#include "WBP_ContextMenu.generated.h"

class UWBP_InventorySlot;
class UMenuAnchor;

UCLASS()
class ATLANTISEONS_API UWBP_ContextMenu : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* Use;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* Throw;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* Close;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Use_Text;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Throw_Text;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* Close_Text;

    UPROPERTY(BlueprintReadWrite, Category = "References")
    UMenuAnchor* MenuAnchorRef;

    UPROPERTY(BlueprintReadWrite, Category = "References")
    UWBP_InventorySlot* InventorySlotRef;

    UPROPERTY(BlueprintReadWrite, Category = "UI")
    FText UseText;

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnUseClicked();

    UFUNCTION()
    void OnThrowClicked();

    UFUNCTION()
    void OnCloseClicked();

    UFUNCTION(BlueprintCallable, Category = "Context Menu")
    void CloseContextMenu();
};
