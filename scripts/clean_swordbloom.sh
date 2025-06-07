#!/bin/bash

# Remove all SwordBloomWidget-related functions and code from AtlantisEonsCharacter.cpp

file="Source/AtlantisEons/AtlantisEonsCharacter.cpp"

# Create a backup
cp "$file" "${file}.backup"

# Remove lines containing SwordBloomWidget references in BeginPlay and PostInitializeComponents
sed -i '' '/Final validation of SwordBloomWidget component/,/SwordBloomWidget component is ready for use/d' "$file"
sed -i '' '/Validate SwordBloomWidget component after Blueprint initialization/,/SwordBloomWidget->SetWidgetClass/d' "$file"

# Remove function implementations
sed -i '' '/^void AAtlantisEonsCharacter::ShowSwordEffects/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::HideSwordEffects/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::ShowSwordSpark/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::HideSwordSpark/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::ShowSwordBloom/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::HideSwordBloom/,/^}/d' "$file"
sed -i '' '/^UWBP_SwordBloom\* AAtlantisEonsCharacter::GetSwordBloomUserWidget/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::UpdateSwordBloomAttachment/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::TestSwordBloomWidget/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::BloomCircleNotify/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::BloomSparkNotify/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::HideBloomEffectsNotify/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::TryTriggerSparkEffect/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::UpdateBloomScaling/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::StartBloomScaling/,/^}/d' "$file"
sed -i '' '/^void AAtlantisEonsCharacter::RecreateSwordBloomWidget/,/^}/d' "$file"

# Remove individual line references
sed -i '' '/UpdateSwordBloomAttachment/d' "$file"
sed -i '' '/TryTriggerSparkEffect/d' "$file"

echo "Cleaned SwordBloomWidget references from $file" 