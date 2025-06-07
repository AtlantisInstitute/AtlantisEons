# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AtlantisEons is a third-person action RPG developed in Unreal Engine 5. The game features melee combat, character progression, inventory management, enemy AI, and a store system.

## Building and Running

### Building the Project

To build the project from source:

1. Right-click on the `.uproject` file and select "Generate Visual Studio project files" (Windows) or "Generate Xcode project files" (Mac)
2. Open the generated project file in the IDE and build

Alternatively, use Unreal Engine's command line:

```bash
# Windows
"C:/Program Files/Epic Games/UE_5.x/Engine/Build/BatchFiles/Build.bat" AtlantisEonsEditor Win64 Development -Project="path/to/AtlantisEons.uproject" -WaitMutex -FromMsBuild

# Mac
/path/to/UE_5.x/Engine/Build/BatchFiles/Mac/Build.sh AtlantisEonsEditor Mac Development -Project="/path/to/AtlantisEons.uproject"
```

### Running the Project

Open the project in Unreal Engine Editor:

1. Launch Unreal Engine
2. Browse to and open `AtlantisEons.uproject`
3. Use Play button to test in editor or use PIE (Play In Editor)

## Core Architecture

### Character System

- `AtlantisEonsCharacter` (C++) is the base character class, with `BP_Character` as its Blueprint extension
- Character stats include STR, DEX, INT which affect combat
- Enhanced Input system handles character controls
- Character has inventory and equipment slots affecting stats
- Dash mechanics with afterimage effects via `DashAfterimage` class

### Inventory System

- Inventory uses grid-based UI with drag-and-drop functionality
- Main classes:
  - `BP_Item`: Physical item in the world
  - `BP_ItemInfo`: Data container for item properties
  - `FStructure_ItemInfo`: Data structure for item information
  - `WBP_Inventory`: Main inventory widget
  - `WBP_InventorySlot`: Individual slot widget
- Item data stored in `Table_ItemList` data table
- Equipment slots: Head, Body, Weapon, Accessory
- Equipment preview uses `BP_SceneCapture` to show character with equipped items

### Enemy AI System

- Zombie enemies use behavior tree AI
- Key components:
  - `ZombieAIController`: Controls zombie behavior
  - `BT_Zombie`: Behavior tree asset
  - `BB_Zombie`: Blackboard for AI data
- Behavior tree tasks: `BTTask_AttackPlayer`, `BTTask_MoveToPlayer`, `BTTask_FindPatrolLocation`
- Services: `BTService_CheckAttackRange`, `BTService_UpdatePlayerLocation`
- Zombies have patrol, chase, and attack behaviors

### Combat System

- Melee attacks use animation montages
- Damage calculation based on character stats and equipment
- `DamageNumberSystem` shows floating damage numbers
- Critical hit system for increased damage
- Hitboxes and collision detection for attacks
- Health bars for player and enemies

### Store System

- Store interface in `WBP_Store` for buying items
- Gold currency system
- Item category filtering
- Purchase confirmation dialog
- Store inventory defined in data tables

## Working with the Codebase

### Blueprint and C++ Integration

The project uses a hybrid approach:
- Core systems implemented in C++
- Extended functionality in Blueprints
- UI primarily implemented with Widget Blueprints

### Item System

To add new items:
1. Update `Table_ItemList` data table
2. Configure item properties (type, stats, icon, etc.)
3. For equipment, ensure proper mesh/skeletal attachments
4. For consumables, implement usage effects

### Input Handling

Input is handled via Enhanced Input system:
- Input Actions defined in `Content/AtlantisEons/Input/Actions/`
- Input Mapping Context in `IMC_Default.uasset`
- Core inputs: Move, Look, Jump, Attack, Dodge, Inventory

### UI Widget Hierarchy

- `WBP_Main`: Root UI widget handling all sub-widgets
- `WBP_TopMenu`: Top UI bar for player stats
- `WBP_Inventory`: Inventory management
- `WBP_Store`: Store interface
- `WBP_CharacterInfo`: Character stats and equipment

When working with UI, respect the existing hierarchy and widget relationships.