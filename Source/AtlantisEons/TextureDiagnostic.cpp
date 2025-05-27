#include "TextureDiagnostic.h"
#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

bool UTextureDiagnostic::DoesTextureExist(const FString& TexturePath)
{
    UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath));
    return Texture != nullptr;
}

UTexture2D* UTextureDiagnostic::TryLoadTexture(const FString& TexturePath)
{
    UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath));
    if (Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("TextureDiagnostic: ✅ Successfully loaded: %s"), *TexturePath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TextureDiagnostic: ❌ Failed to load: %s"), *TexturePath);
    }
    return Texture;
}

TArray<FString> UTextureDiagnostic::GenerateTexturePathVariations(const FString& ItemName, int32 ItemIndex)
{
    TArray<FString> Variations;
    
    FString CleanName = ItemName.Replace(TEXT(" "), TEXT(""));
    FString CleanNameWithDash = ItemName.Replace(TEXT(" "), TEXT("-"));
    FString CleanNameWithUnderscore = ItemName.Replace(TEXT(" "), TEXT("_"));
    
    // Base path
    FString BasePath = TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/");
    
    // Common patterns
    Variations.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *CleanName));
    Variations.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *CleanNameWithDash));
    Variations.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *CleanNameWithUnderscore));
    Variations.Add(BasePath + FString::Printf(TEXT("IMG_%s_105"), *CleanName));
    
    // HP/MP potion patterns
    if (ItemName.Contains(TEXT("HP")))
    {
        FString HPName = CleanName.Replace(TEXT("HP"), TEXT("Healing"));
        Variations.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *HPName));
        
        // Handle typos
        FString TypoName = HPName.Replace(TEXT("Potion"), TEXT("Ption"));
        Variations.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *TypoName));
    }
    
    if (ItemName.Contains(TEXT("MP")))
    {
        FString MPName = CleanName.Replace(TEXT("MP"), TEXT("Mana"));
        Variations.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *MPName));
        
        // Handle typos
        FString TypoName = MPName.Replace(TEXT("Potion"), TEXT("Ption"));
        Variations.Add(BasePath + FString::Printf(TEXT("IMG_%s"), *TypoName));
    }
    
    // Index-based patterns
    Variations.Add(BasePath + FString::Printf(TEXT("IMG_Item_%d"), ItemIndex));
    Variations.Add(BasePath + FString::Printf(TEXT("Item_%d"), ItemIndex));
    
    return Variations;
}

FString UTextureDiagnostic::FindCorrectTexturePath(const FString& ItemName, int32 ItemIndex)
{
    UE_LOG(LogTemp, Warning, TEXT("TextureDiagnostic: Finding texture for Item %d: %s"), ItemIndex, *ItemName);
    
    TArray<FString> Variations = GenerateTexturePathVariations(ItemName, ItemIndex);
    
    for (const FString& Path : Variations)
    {
        if (DoesTextureExist(Path))
        {
            UE_LOG(LogTemp, Warning, TEXT("TextureDiagnostic: ✅ Found texture at: %s"), *Path);
            return Path;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TextureDiagnostic: ❌ No texture found for Item %d: %s"), ItemIndex, *ItemName);
    return FString();
}

void UTextureDiagnostic::LogAvailableTextures()
{
    UE_LOG(LogTemp, Warning, TEXT("TextureDiagnostic: === AVAILABLE TEXTURES ==="));
    
    // List of known texture files based on our directory listing
    TArray<FString> KnownTextures = {
        TEXT("IMG_Diamond"),
        TEXT("IMG_WoodShield"),
        TEXT("IMG_SWATHelmet"),
        TEXT("IMG_SciFiSuitS4_105"),
        TEXT("IMG_Spike"),
        TEXT("IMG_SciFiSuitS2_105"),
        TEXT("IMG_SciFiSuitS3_105"),
        TEXT("IMG_SciFiSuitS1_105"),
        TEXT("IMG_SciFiRifle_105"),
        TEXT("IMG_SciFiShield_105"),
        TEXT("IMG_MedievalHelmet"),
        TEXT("IMG_SciFiPistol_105"),
        TEXT("IMG_LongAxe"),
        TEXT("IMG_LargeManaPotion"),
        TEXT("IMG_LaserSword"),
        TEXT("IMG_Gasmask"),
        TEXT("IMG_LargeHealingPotion"),
        TEXT("IMG_BasicSword"),
        TEXT("IMG_CaptainHat"),
        TEXT("IMG_BasicHealingPtion"),
        TEXT("IMG_BasicManaPtion"),
        TEXT("IMG_AdvancedManaPotion"),
        TEXT("IMG_AdvancedHealingPotion")
    };
    
    FString BasePath = TEXT("/Game/AtlantisEons/Sources/Images/ItemThumbnail/");
    
    for (const FString& TextureName : KnownTextures)
    {
        FString FullPath = BasePath + TextureName;
        bool bExists = DoesTextureExist(FullPath);
        UE_LOG(LogTemp, Warning, TEXT("TextureDiagnostic: %s %s"), 
            bExists ? TEXT("✅") : TEXT("❌"), *FullPath);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TextureDiagnostic: === END AVAILABLE TEXTURES ==="));
} 