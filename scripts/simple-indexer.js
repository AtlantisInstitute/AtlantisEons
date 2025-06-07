#!/usr/bin/env node

/**
 * Simple AtlantisEons Codebase Indexer
 * Focus on getting key classes and files into memory
 */

const fs = require('fs');
const path = require('path');
const MemoryManager = require('./memory-manager');

// Initialize memory manager
const memoryManager = new MemoryManager();
const sessionId = memoryManager.startSession('Simple Codebase Indexing - Key Classes and Files');

const sourceFiles = [
    // Main character and game classes
    { file: 'AtlantisEonsCharacter.cpp', classes: ['AAtlantisEonsCharacter'], type: 'Character_Class_Pattern' },
    { file: 'AtlantisEonsCharacter.h', classes: ['AAtlantisEonsCharacter'], type: 'Character_Class_Pattern' },
    { file: 'AtlantisEonsGameMode.cpp', classes: ['AAtlantisEonsGameMode'], type: 'GameMode_Pattern' },
    { file: 'AtlantisEonsGameMode.h', classes: ['AAtlantisEonsGameMode'], type: 'GameMode_Pattern' },
    { file: 'AtlantisEonsHUD.cpp', classes: ['AAtlantisEonsHUD'], type: 'HUD_Pattern' },
    { file: 'AtlantisEonsHUD.h', classes: ['AAtlantisEonsHUD'], type: 'HUD_Pattern' },
    { file: 'AtlantisEonsGameInstance.cpp', classes: ['UAtlantisEonsGameInstance'], type: 'GameInstance_Pattern' },
    { file: 'AtlantisEonsGameInstance.h', classes: ['UAtlantisEonsGameInstance'], type: 'GameInstance_Pattern' },
    
    // Enemy AI
    { file: 'ZombieCharacter.cpp', classes: ['AZombieCharacter'], type: 'Enemy_AI_Pattern' },
    { file: 'ZombieCharacter.h', classes: ['AZombieCharacter'], type: 'Enemy_AI_Pattern' },
    { file: 'ZombieAIController.cpp', classes: ['AZombieAIController'], type: 'AI_Controller_Pattern' },
    { file: 'ZombieAIController.h', classes: ['AZombieAIController'], type: 'AI_Controller_Pattern' },
    
    // Components
    { file: 'InventoryComponent.cpp', classes: ['UInventoryComponent'], type: 'Inventory_System_Pattern' },
    { file: 'InventoryComponent.h', classes: ['UInventoryComponent'], type: 'Inventory_System_Pattern' },
    { file: 'EquipmentComponent.cpp', classes: ['UEquipmentComponent'], type: 'Equipment_System_Pattern' },
    { file: 'EquipmentComponent.h', classes: ['UEquipmentComponent'], type: 'Equipment_System_Pattern' },
    { file: 'CharacterStatsComponent.cpp', classes: ['UCharacterStatsComponent'], type: 'Stats_System_Pattern' },
    { file: 'CharacterStatsComponent.h', classes: ['UCharacterStatsComponent'], type: 'Stats_System_Pattern' },
    
    // UI Widgets
    { file: 'WBP_Main.cpp', classes: ['UWBP_Main'], type: 'UE_Widget_Pattern' },
    { file: 'WBP_Main.h', classes: ['UWBP_Main'], type: 'UE_Widget_Pattern' },
    { file: 'WBP_CharacterInfo.cpp', classes: ['UWBP_CharacterInfo'], type: 'UE_Widget_Pattern' },
    { file: 'WBP_CharacterInfo.h', classes: ['UWBP_CharacterInfo'], type: 'UE_Widget_Pattern' },
    { file: 'WBP_Inventory.cpp', classes: ['UWBP_Inventory'], type: 'UE_Widget_Pattern' },
    { file: 'WBP_Inventory.h', classes: ['UWBP_Inventory'], type: 'UE_Widget_Pattern' },
    { file: 'WBP_InventorySlot.cpp', classes: ['UWBP_InventorySlot'], type: 'UE_Widget_Pattern' },
    { file: 'WBP_InventorySlot.h', classes: ['UWBP_InventorySlot'], type: 'UE_Widget_Pattern' },
    { file: 'WBP_Store.cpp', classes: ['UWBP_Store'], type: 'Store_System_Pattern' },
    { file: 'WBP_Store.h', classes: ['UWBP_Store'], type: 'Store_System_Pattern' },
    
    // Item System
    { file: 'BP_Item.cpp', classes: ['ABP_Item'], type: 'Item_System_Pattern' },
    { file: 'BP_Item.h', classes: ['ABP_Item'], type: 'Item_System_Pattern' },
    { file: 'BP_ItemInfo.cpp', classes: ['UBP_ItemInfo'], type: 'Item_System_Pattern' },
    { file: 'BP_ItemInfo.h', classes: ['UBP_ItemInfo'], type: 'Item_System_Pattern' },
    
    // Damage System
    { file: 'DamageNumberSystem.cpp', classes: ['UDamageNumberSystem'], type: 'Damage_System_Pattern' },
    { file: 'DamageNumberSystem.h', classes: ['UDamageNumberSystem'], type: 'Damage_System_Pattern' },
    { file: 'DamageNumberWidget.cpp', classes: ['UDamageNumberWidget'], type: 'Damage_System_Pattern' },
    { file: 'DamageNumberWidget.h', classes: ['UDamageNumberWidget'], type: 'Damage_System_Pattern' },
    
    // Data Systems
    { file: 'UniversalItemLoader.cpp', classes: ['UUniversalItemLoader'], type: 'Data_System_Pattern' },
    { file: 'UniversalItemLoader.h', classes: ['UUniversalItemLoader'], type: 'Data_System_Pattern' },
    { file: 'StoreSystemFix.cpp', classes: ['UStoreSystemFix'], type: 'Store_System_Pattern' },
    { file: 'StoreSystemFix.h', classes: ['UStoreSystemFix'], type: 'Store_System_Pattern' }
];

async function indexKeyFiles() {
    console.log('🚀 Starting simplified codebase indexing...');
    
    let processedCount = 0;
    let classCount = 0;
    
    for (const fileInfo of sourceFiles) {
        const filePath = `Source/AtlantisEons/${fileInfo.file}`;
        const fullPath = path.join('/Users/danielvargas/Documents/Unreal Projects/AtlantisEons', filePath);
        
        if (fs.existsSync(fullPath)) {
            try {
                console.log(`📄 Processing: ${fileInfo.file}`);
                
                const content = fs.readFileSync(fullPath, 'utf8');
                const lines = content.split('\n');
                const fileSize = content.length;
                
                // Store file information
                memoryManager.rememberFile(filePath, {
                    fileName: fileInfo.file,
                    fileType: fileInfo.file.endsWith('.h') ? 'Header' : 'Implementation',
                    lineCount: lines.length,
                    size: fileSize,
                    classes: fileInfo.classes,
                    pattern: fileInfo.type,
                    purpose: determinePurpose(fileInfo.file),
                    indexed: new Date().toISOString()
                });
                
                // Store class information
                fileInfo.classes.forEach(className => {
                    memoryManager.rememberClass(className, {
                        file: filePath,
                        lineCount: lines.length,
                        fileType: fileInfo.file.endsWith('.h') ? 'Header' : 'Implementation',
                        pattern: fileInfo.type,
                        purpose: determinePurpose(fileInfo.file),
                        isUEClass: className.startsWith('A') || className.startsWith('U') || className.startsWith('F'),
                        indexed: new Date().toISOString()
                    });
                    classCount++;
                });
                
                // Update pattern frequency
                memoryManager.updatePatternFrequency(fileInfo.type);
                
                processedCount++;
                
            } catch (error) {
                console.error(`❌ Error processing ${fileInfo.file}:`, error.message);
            }
        } else {
            console.log(`⏭️  File not found: ${fileInfo.file}`);
        }
    }
    
    // Add comprehensive insights
    memoryManager.addInsight(
        'simple-indexing-complete',
        `Simple codebase indexing completed: ${processedCount} files, ${classCount} classes indexed`,
        {
            filesProcessed: processedCount,
            classesIndexed: classCount,
            patterns: sourceFiles.map(f => f.type).filter((v, i, a) => a.indexOf(v) === i),
            timestamp: new Date().toISOString()
        }
    );
    
    memoryManager.addInsight(
        'architecture-overview',
        'AtlantisEons architecture indexed: Character system, UI widgets, AI controllers, component systems, and data management',
        {
            mainSystems: [
                'Character Movement and Combat',
                'Inventory and Equipment',
                'Store and Commerce',
                'Damage Display System',
                'Enemy AI and Behavior Trees',
                'UI Widget Framework',
                'Data Loading and Management'
            ]
        }
    );
    
    console.log('\n📊 SIMPLE INDEXING RESULTS:');
    console.log(`✅ Files processed: ${processedCount}`);
    console.log(`🔑 Classes indexed: ${classCount}`);
    console.log('💾 Data saved to memory system');
    
    // End session
    memoryManager.endSession(`Simple indexing completed: ${processedCount} files, ${classCount} classes`);
    
    return { processedCount, classCount };
}

function determinePurpose(fileName) {
    if (fileName.includes('Character')) return 'Character behavior and movement';
    if (fileName.includes('GameMode')) return 'Game rules and management';
    if (fileName.includes('HUD')) return 'Head-up display and UI management';
    if (fileName.includes('GameInstance')) return 'Game state persistence';
    if (fileName.includes('Zombie')) return 'Enemy AI and behavior';
    if (fileName.includes('Inventory')) return 'Inventory management system';
    if (fileName.includes('Equipment')) return 'Equipment and gear system';
    if (fileName.includes('Stats')) return 'Character statistics tracking';
    if (fileName.includes('WBP_')) return 'UI widget implementation';
    if (fileName.includes('Store')) return 'Store and commerce functionality';
    if (fileName.includes('Item')) return 'Item data and management';
    if (fileName.includes('Damage')) return 'Damage calculation and display';
    if (fileName.includes('Universal')) return 'Universal data loading system';
    return 'General game functionality';
}

// Run the indexer
indexKeyFiles().catch(console.error); 