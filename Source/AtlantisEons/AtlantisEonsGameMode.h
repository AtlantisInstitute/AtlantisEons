// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "AtlantisEonsGameMode.generated.h"

UCLASS(Blueprintable)
class ATLANTISEONS_API AAtlantisEonsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAtlantisEonsGameMode();
	virtual void BeginPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category = "Game")
	void RestartCurrentPlayer();
	
private:
	// Timer for resetting movement in BeginPlay
	FTimerHandle MovementResetTimer;
};
