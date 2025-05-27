#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "TextureDiagnostic.generated.h"

/**
 * Diagnostic utility class for checking texture file existence and naming patterns
 */
UCLASS(BlueprintType)
class ATLANTISEONS_API UTextureDiagnostic : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Check if a texture exists at the given path
     */
    UFUNCTION(BlueprintCallable, Category = "Texture Diagnostic")
    static bool DoesTextureExist(const FString& TexturePath);

    /**
     * Try to load a texture and return whether it was successful
     */
    UFUNCTION(BlueprintCallable, Category = "Texture Diagnostic")
    static UTexture2D* TryLoadTexture(const FString& TexturePath);

    /**
     * Generate common texture path variations for an item
     */
    UFUNCTION(BlueprintCallable, Category = "Texture Diagnostic")
    static TArray<FString> GenerateTexturePathVariations(const FString& ItemName, int32 ItemIndex);

    /**
     * Find the correct texture path for an item by trying multiple variations
     */
    UFUNCTION(BlueprintCallable, Category = "Texture Diagnostic")
    static FString FindCorrectTexturePath(const FString& ItemName, int32 ItemIndex);

    /**
     * Log all available textures in the ItemThumbnail directory
     */
    UFUNCTION(BlueprintCallable, Category = "Texture Diagnostic")
    static void LogAvailableTextures();
}; 