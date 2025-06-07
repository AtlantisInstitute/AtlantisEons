#!/bin/bash

# Setup script for AtlantisEons Memory Management System
# This script initializes the memory system and configures it for the project

echo "🧠 Setting up AtlantisEons Memory Management System..."

# Check if Node.js is available
if ! command -v node &> /dev/null; then
    echo "❌ Node.js is required but not installed. Please install Node.js 14+ and try again."
    exit 1
fi

# Check Node.js version
NODE_VERSION=$(node -v | cut -d'v' -f2 | cut -d'.' -f1)
if [ "$NODE_VERSION" -lt 14 ]; then
    echo "❌ Node.js 14+ is required. Current version: $(node -v)"
    exit 1
fi

echo "✅ Node.js version check passed: $(node -v)"

# Make CLI executable
chmod +x memory-cli.js memory-manager.js memory-mcp-server.js

echo "✅ Made memory tools executable"

# Initialize memory system if not already done
if [ ! -d "logs-and-data" ]; then
    echo "📂 Initializing memory system..."
    node memory-cli.js init "Initial setup of AtlantisEons memory system"
else
    echo "📂 Memory system already initialized"
fi

# Save context for key project files
echo "💾 Saving context for key project files..."

# Save character file context if it exists
if [ -f "Source/AtlantisEons/AtlantisEonsCharacter.cpp" ]; then
    node memory-cli.js save Source/AtlantisEons/AtlantisEonsCharacter.cpp "Main character class - core gameplay systems"
fi

if [ -f "Source/AtlantisEons/AtlantisEonsCharacter.h" ]; then
    node memory-cli.js save Source/AtlantisEons/AtlantisEonsCharacter.h "Main character header - class definition"
fi

# Add initial insights about the project
echo "💡 Adding initial project insights..."

node memory-cli.js insight "project-structure" "AtlantisEons is an Unreal Engine 5.5 action RPG with character progression, inventory, and combat systems"

node memory-cli.js insight "inventory-system" "Inventory uses TArray<UBP_ItemInfo*> with 30 slots, supports drag-and-drop and item stacking"

node memory-cli.js insight "ui-architecture" "UI system uses UMG widgets with WBP_ prefix, managed through AAtlantisEonsHUD"

node memory-cli.js insight "input-system" "Enhanced Input System used with Input Mapping Context (IMC_Default)"

# Record common patterns
echo "🔄 Recording common code patterns..."

node memory-cli.js pattern "widget-naming" "UI widgets use WBP_ prefix for Blueprint widgets" "WBP_Main" "WBP_Inventory" "WBP_CharacterInfo"

node memory-cli.js pattern "blueprint-integration" "C++ classes exposed to Blueprint using UCLASS, UPROPERTY, UFUNCTION macros"

node memory-cli.js pattern "unreal-logging" "Logging uses UE_LOG with custom categories like LogTemplateCharacter"

# Show memory statistics
echo ""
echo "📊 Memory system setup complete! Current statistics:"
node memory-cli.js stats

echo ""
echo "🚀 Memory system is ready! Here are some useful commands:"
echo ""
echo "  📝 Start a session:    node memory-cli.js session start \"Working on inventory bugs\""
echo "  💾 Save file context:  node memory-cli.js save <file-path> \"Description\""  
echo "  🔍 Search memory:      node memory-cli.js search \"inventory\""
echo "  💡 Add insight:        node memory-cli.js insight \"bug-fix\" \"Fixed item duplication\""
echo "  📊 View statistics:    node memory-cli.js stats detail"
echo "  🆘 Get help:           node memory-cli.js help"
echo ""
echo "  🔧 Start MCP server:   npm run memory-server"
echo "  📖 Read documentation: cat MEMORY_SYSTEM.md"
echo ""

# Start MCP server in background if requested
if [ "$1" = "--start-server" ]; then
    echo "🔧 Starting memory-enhanced MCP server..."
    npm run memory-server &
    echo "   Server started on port 9011"
    echo "   Use 'ps aux | grep memory-mcp-server' to check status"
fi

echo "✅ Setup complete! The memory system is ready for use." 