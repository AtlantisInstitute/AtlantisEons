# AtlantisEons

An action RPG developed with **Unreal Engine 5.5**, featuring deep character progression, dynamic combat systems, and immersive gameplay mechanics.

## 🎮 Game Overview

AtlantisEons is a third-person action RPG that combines fast-paced combat, character customization, and strategic gameplay elements. Players embark on an epic journey through mystical realms, battling enemies, collecting powerful equipment, and developing their character's abilities.

### Core Features

- ⚔️ **Dynamic Combat System**: Multi-hit combo chains with timing-based critical windows
- 🛡️ **Equipment & Visuals**: Real-time equipment swapping with visual mesh updates
- 📦 **Advanced Inventory**: 30-slot inventory with drag-and-drop and item stacking
- 📊 **Character Progression**: STR, DEX, INT stats with equipment-based bonuses
- 🎯 **Camera Stabilization**: Ultra-responsive camera system for smooth combat
- ✨ **Visual Effects**: Sword bloom effects, particle systems, and damage numbers
- 🧟 **AI Enemies**: Smart zombie AI with team-based combat mechanics
- 🎨 **Modern UI**: Responsive UMG interfaces with Blueprint integration

## 🏗️ Technical Architecture

### Component-Based Design

The project follows a clean component-based architecture for maintainability and modularity:

```
AtlantisEonsCharacter (Main Class)
├── StatsComponent           # Health, mana, character stats
├── EquipmentComponent       # Equipment logic and management  
├── EquipmentVisualsComponent # Visual mesh swapping and materials
├── InventoryComponent       # Item storage and management
├── CharacterUIManager       # UI updates and synchronization
└── CameraStabilizationComponent # Attack camera stabilization
```

### Key Systems

| System | Description | Status |
|--------|-------------|--------|
| **Combat** | 5-attack combo system with critical timing windows | ✅ Complete |
| **Equipment** | Helmet, weapon, shield visual swapping | ✅ Complete |
| **Inventory** | 30-slot system with drag-drop and stacking | ✅ Complete |
| **Stats** | STR/DEX/INT progression with equipment bonuses | ✅ Complete |
| **Camera** | Ultra-aggressive stabilization during attacks | ✅ Complete |
| **UI** | Health/mana bars, inventory, equipment slots | ✅ Complete |
| **AI** | Zombie enemy behavior and team combat | ✅ Complete |
| **VFX** | Sword effects, damage numbers, particles | ✅ Complete |

## 🛠️ Project Structure

```
Source/AtlantisEons/
├── AtlantisEonsCharacter.cpp/h      # Main player character with components
├── Components/
│   ├── CharacterStatsComponent      # Health, mana, stats management
│   ├── EquipmentComponent          # Equipment logic and validation
│   ├── EquipmentVisualsComponent   # Visual mesh swapping
│   ├── InventoryComponent          # Item storage and operations
│   ├── CharacterUIManager          # UI synchronization
│   └── CameraStabilizationComponent # Combat camera control
├── AI/
│   ├── ZombieCharacter.cpp/h       # Enemy AI character
│   └── BTTask_AttackPlayer.cpp/h   # Attack behavior tree task
├── UI/
│   ├── DamageNumberSystem.cpp/h    # Floating damage numbers
│   └── Widget Classes              # UMG interface components
├── Core/
│   ├── AtlantisEonsGameMode.cpp/h  # Game mode and rules
│   └── ItemTypes.h                 # Item and equipment enums
└── Blueprints/                     # Blueprint assets and UI
```

## ⚔️ Combat System

### Combo Chain Mechanics
- **5-Attack Sequence**: Each attack builds into the next with proper timing
- **Critical Windows**: Bloom circle indicates perfect timing for maximum damage
- **Visual Feedback**: Sword effects and particle systems activate during combos
- **Camera Stabilization**: Ultra-responsive camera during attack animations

### Equipment System
- **Visual Mesh Swapping**: Equipment instantly updates character appearance
- **Material Support**: Dual material system for complex equipment textures
- **Real-time Updates**: Seamless transitions between equipped items
- **Blueprint Integration**: Full compatibility with Blueprint item systems

## 📦 Inventory & Items

### Advanced Inventory Features
- **30-Slot Grid System**: Organized item storage with visual slots
- **Drag & Drop**: Intuitive item management interface
- **Item Stacking**: Automatic stacking for consumable items
- **Equipment Slots**: Head, Body, Weapon, Accessory dedicated slots
- **Context Menus**: Use, throw, and equip actions for items

### Item Types
- **Equipment**: Helmets, weapons, armor, accessories with stat bonuses
- **Consumables**: Health potions, mana potions with recovery effects
- **Materials**: Crafting components and upgrade materials
- **Quest Items**: Story-relevant items and collectibles

## 🎯 Technical Highlights

### Performance Optimizations
- **Component Delegation**: Clean separation of concerns for maintainability
- **Memory Management**: Object pooling for damage numbers and effects
- **Asset Loading**: TSoftObjectPtr for efficient mesh and texture loading
- **Blueprint Compatibility**: Zero breaking changes to existing Blueprint systems

### Code Quality
- **Comprehensive Logging**: Detailed debug information for all systems
- **Error Handling**: Robust validation and graceful failure handling
- **Documentation**: Extensive code comments and architectural notes
- **Modular Design**: Easy to extend and modify individual systems

## 🚀 Getting Started

### Prerequisites
- **Unreal Engine 5.5** installed
- **Xcode** (for Mac development)
- **Git** for version control

### Building the Project

```bash
# Navigate to project directory
cd "/Users/danielvargas/Documents/Unreal Projects/AtlantisEons"

# Build using Unreal Build Tool
"/Users/Shared/Epic Games/UE_5.5/Engine/Build/BatchFiles/Mac/Build.sh" AtlantisEonsEditor Development Mac -Project="AtlantisEons.uproject" -WaitMutex -FromMsBuild
```

### Development Workflow

1. **Open in Unreal Editor**: Launch `AtlantisEons.uproject`
2. **Play in Editor**: Test gameplay mechanics immediately
3. **Blueprint Integration**: Customize behavior in `BP_Character`
4. **Level Design**: Create content in `TestLevel` map

## 🧠 Development Tools

### Memory Management System

This project includes a comprehensive memory and context preservation system for development:

```bash
# Start development session
node memory-cli.js session start "Working on combat system"

# Save important context
node memory-cli.js save Source/AtlantisEons/AtlantisEonsCharacter.cpp "Main character updates"

# Record insights
node memory-cli.js insight "architecture" "Component delegation pattern working well"

# Search previous work
node memory-cli.js search "inventory system"
```

**Key Memory Tools:**
- `memory-cli.js` - Command-line interface for memory management
- `memory-mcp-server.js` - MCP server for AI tool integration  
- `MEMORY_SYSTEM.md` - Detailed documentation

## 🎨 Visual Systems

### Effects & Feedback
- **Sword Bloom Effects**: Visual timing indicators for combo attacks
- **Particle Systems**: Niagara effects for sword trails and magic
- **Damage Numbers**: Floating combat feedback with screen positioning
- **Dragon Manifestations**: Epic visual effects for final combo attacks

### UI Design
- **Modern Interface**: Clean, responsive UMG-based UI design
- **Real-time Updates**: Immediate feedback for all player actions
- **Health/Mana Bars**: Circular progress indicators with smooth animations
- **Equipment Visualization**: Live preview of equipped items

---

**Engine**: Unreal Engine 5.5  
**Platform**: Mac (Primary), Cross-platform capable  
**Language**: C++ with Blueprint integration  
**Architecture**: Component-based modular design
