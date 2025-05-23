# AtlantisEons

An action RPG developed with Unreal Engine 5.5, featuring character progression, inventory management, and combat systems.

## 🧠 Memory Management System

This project includes a comprehensive memory and context preservation system designed to maintain knowledge across different development sessions and AI context windows.

### Quick Start

```bash
# Setup the memory system
./setup-memory.sh

# Or initialize manually
npm run init "Starting development session"

# Start working with memory
node memory-cli.js session start "Working on inventory system"
node memory-cli.js save Source/AtlantisEons/AtlantisEonsCharacter.cpp "Main character class"
node memory-cli.js insight "bug-fix" "Fixed inventory slot duplication"
```

### Key Features

- 🧠 **Persistent Memory**: Store insights, patterns, and context across sessions
- 🔍 **Smart Search**: Find relevant information using semantic search
- 📊 **Pattern Recognition**: Automatically identify and track code patterns
- ❓ **Problem Tracking**: Track problems and their solutions over time
- 📝 **Session Management**: Organize work into meaningful sessions
- 🔧 **MCP Integration**: Works with AI tools via Model Context Protocol

### Memory Tools

| Tool | Description |
|------|-------------|
| `memory-cli.js` | Command-line interface for memory management |
| `memory-mcp-server.js` | MCP server for AI tool integration |
| `memory-manager.js` | Core memory storage and retrieval engine |

### Documentation

- **[Memory System Guide](MEMORY_SYSTEM.md)** - Comprehensive documentation
- **Setup Script**: `./setup-memory.sh` - Quick initialization
- **CLI Help**: `node memory-cli.js help` - Available commands

## 🎮 Game Development

### Key Systems

- **Character System**: Player and enemy character management with combat mechanics
- **Inventory System**: 30-slot inventory with drag-and-drop functionality and item stacking
- **UI System**: UMG-based interface with Blueprint widget integration
- **Input System**: Enhanced Input System with configurable input mapping
- **Damage System**: Visual damage numbers and combat feedback

### Project Structure

```
Source/AtlantisEons/
├── AtlantisEonsCharacter.cpp/h    # Main player character
├── AtlantisEonsGameMode.cpp/h     # Game mode logic
├── AtlantisEonsHUD.cpp/h          # User interface management
├── ZombieCharacter.cpp/h          # Enemy AI character
├── DamageNumberSystem.cpp/h       # Combat damage display
└── UI/                            # User interface components
```

## 🛠️ Development Tools

### Memory Commands

```bash
# Session management
npm run memory session start "Feature development"
npm run memory session end "Completed inventory fixes"

# Context preservation  
npm run memory save <file-path> "Description"
npm run memory search "inventory"

# Insights and patterns
npm run memory insight "architecture" "Separated UI from game logic"
npm run memory pattern "error-handling" "Null checks before function calls"

# Analysis
npm run memory analyze system character
npm run memory stats detail
```

### MCP Server

```bash
# Start memory-enhanced MCP server
npm run memory-server

# Server provides AI tools for:
# - Saving and retrieving context
# - Searching project memory
# - Analyzing code with historical context
# - Tracking problems and solutions
```

## 📊 Project Memory

The memory system tracks:

- **Development Sessions**: Organized work periods with goals and outcomes
- **Code Patterns**: Recurring design patterns and best practices  
- **Problem Solutions**: Issues encountered and how they were resolved
- **Context Windows**: Snapshots of development context for future reference
- **Project Insights**: Important discoveries and architectural decisions

All memory data is stored in the `.memory/` directory and can be exported/imported for backup and sharing.

## 🚀 Getting Started

1. **Initialize Memory System**:
   ```bash
   ./setup-memory.sh
   ```

2. **Start Development Session**:
   ```bash
   node memory-cli.js session start "Your work description"
   ```

3. **Save Important Context**:
   ```bash
   node memory-cli.js save Source/AtlantisEons/YourFile.cpp "What you're working on"
   ```

4. **Record Insights**:
   ```bash
   node memory-cli.js insight "discovery" "What you learned"
   ```

5. **Search Memory**:
   ```bash
   node memory-cli.js search "inventory system"
   ```

The memory system helps maintain context and knowledge across development sessions, making it easier to work on complex game systems over time.

---

**Engine**: Unreal Engine 5.5  
**Platform**: Mac (with cross-platform memory tools)  
**Language**: C++ with Blueprint integration
