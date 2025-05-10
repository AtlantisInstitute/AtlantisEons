#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterInfoWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResumeGameDelegate);

UCLASS()
class ATLANTISEONS_API UCharacterInfoWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Event dispatcher for when the resume button is clicked */
    UPROPERTY(BlueprintAssignable, Category = "Game")
    FOnResumeGameDelegate OnResumeGame;

    /** Function to handle resume button click */
    UFUNCTION(BlueprintCallable, Category = "Game")
    void HandleResumeClicked();
};
