# AtlantisEons Memory Management System

A comprehensive memory and context preservation system for Unreal Engine 5.5 development, designed to maintain knowledge across different context windows and development sessions.

## Overview

The memory system consists of three main components:

1. **Memory Manager** (`memory-manager.js`) - Core memory storage and retrieval
2. **Memory MCP Server** (`memory-mcp-server.js`) - Model Context Protocol server integration  
3. **Memory CLI** (`memory-cli.js`) - Command-line interface for easy interaction

## Features

- 🧠 **Persistent Memory**: Store insights, patterns, and context across sessions
- 🔍 **Smart Search**: Find relevant information using semantic search
- 📊 **Pattern Recognition**: Automatically identify and track code patterns
- ❓ **Problem Tracking**: Track problems and their solutions over time
- 📝 **Session Management**: Organize work into meaningful sessions
- 🔄 **Context Windows**: Save and retrieve context for future reference
- 📈 **Analytics**: Get insights into development patterns and progress
- 🔧 **MCP Integration**: Works with AI tools via Model Context Protocol

## Quick Start

### 1. Initialize the Memory System

```bash
# Initialize memory system for your project
npm run init "Starting AtlantisEons character system development"

# Or use the CLI directly
node memory-cli.js init "Starting character system work"
```

### 2. Start a Development Session

```bash
# Start a new session
node memory-cli.js session start "Working on inventory system bugs"

# View current sessions
node memory-cli.js session list
```

### 3. Save Context and Insights

```bash
# Save context from a specific file
node memory-cli.js save Source/AtlantisEons/AtlantisEonsCharacter.cpp "Main character implementation"

# Add development insights
node memory-cli.js insight "bug-fix" "Fixed inventory slot duplication issue in UpdateInventorySlots"

# Record code patterns
node memory-cli.js pattern "widget-naming" "All UI widgets use WBP_ prefix" WBP_Main WBP_Inventory WBP_CharacterInfo
```

### 4. Search and Retrieve

```bash
# Search across all memory categories
node memory-cli.js search "inventory"

# Search specific categories
node memory-cli.js search "character" insights patterns

# Get relevant context for current work
node memory-cli.js context get "inventory system"
```

### 5. Track Problems and Solutions

```bash
# Track a new problem
node memory-cli.js problem track "Inventory items not stacking properly"

# Add solution to a problem (get problem ID from 'problem list')
node memory-cli.js problem solve <problem-id> "Fixed by updating StackNumber comparison in AddItemToInventory"

# List recent problems
node memory-cli.js problem list
```

## Memory System Components

### Memory Manager

The core `MemoryManager` class provides:

- **Session Management**: Track development sessions with actions, insights, and outcomes
- **Context Windows**: Save snapshots of current context for future reference
- **Insights Storage**: Record important discoveries and decisions
- **Pattern Recognition**: Identify recurring code patterns and practices
- **Problem Tracking**: Maintain history of issues and their solutions
- **Search & Retrieval**: Find relevant information using multiple search strategies

### MCP Server Integration

The memory system integrates with AI development tools via the Model Context Protocol:

```bash
# Start the memory-enhanced MCP server
npm run memory-server

# Server runs on port 9011 by default
# Provides tools for saving context, searching memory, and analyzing code
```

### CLI Commands Reference

| Command | Description | Usage |
|---------|-------------|-------|
| `init` | Initialize memory system | `init [description]` |
| `session` | Manage development sessions | `session start\|end\|list [args]` |
| `save` | Save file context | `save <file-path> [description]` |
| `search` | Search memory | `search <query> [categories]` |
| `insight` | Add development insight | `insight <type> <description> [context-file]` |
| `pattern` | Record code pattern | `pattern <name> <description> [examples]` |
| `problem` | Track problems | `problem track\|solve\|list [args]` |
| `analyze` | Analyze code with memory | `analyze class\|system <target>` |
| `stats` | Show memory statistics | `stats [detail]` |
| `context` | Manage context windows | `context save\|get\|list [args]` |
| `export` | Export memory data | `export [output-file]` |
| `import` | Import memory data | `import <input-file>` |
| `cleanup` | Clean old data | `cleanup [days-old]` |

## AtlantisEons Project Integration

The memory system comes pre-configured with knowledge about your AtlantisEons project:

### Key Classes Tracked
- `AAtlantisEonsCharacter` - Main player character
- `AAtlantisEonsGameMode` - Game mode logic
- `AAtlantisEonsHUD` - User interface management
- `UAtlantisEonsGameInstance` - Game instance data
- `AZombieCharacter` - Enemy character
- `UDamageNumberSystem` - Damage display system

### Pre-defined Patterns
- **Widget Naming**: UI widgets use `WBP_` prefix
- **Blueprint Integration**: Classes use `UCLASS`, `UPROPERTY`, `UFUNCTION` macros
- **Inventory System**: Items use `BP_ItemInfo` for data and `BP_Item` for world representation

### System Analysis

Analyze major game systems:

```bash
# Analyze character system
node memory-cli.js analyze system character

# Analyze inventory system  
node memory-cli.js analyze system inventory

# Analyze specific classes
node memory-cli.js analyze class AAtlantisEonsCharacter
```

## Data Storage

Memory data is stored in the `.memory/` directory:

```
.memory/
├── project-memory.json      # Main memory database
├── sessions.json           # Session history
├── insights.json          # Development insights
├── patterns.json          # Code patterns
└── context-windows.json   # Saved contexts
```

## Best Practices

### 1. Regular Session Management
- Start sessions with descriptive names
- End sessions with summaries of what was accomplished
- Use sessions to organize related work

### 2. Context Preservation
- Save context when switching between major features
- Include descriptive comments with saved contexts
- Use context search to find related previous work

### 3. Insight Recording
- Record insights as soon as you discover them
- Use consistent insight types (e.g., "bug-fix", "optimization", "architecture")
- Include relevant context and code references

### 4. Pattern Documentation
- Document patterns as you identify them
- Include concrete examples with pattern definitions
- Update pattern frequency as you encounter them again

### 5. Problem Tracking
- Track problems immediately when encountered
- Include enough context to understand the problem later
- Document solutions with sufficient detail for future reference

## Advanced Usage

### Custom Insight Types
Create your own insight categories:
```bash
node memory-cli.js insight "performance" "Character movement optimization reduced frame drops"
node memory-cli.js insight "architecture" "Separated UI logic from game logic in inventory system"
node memory-cli.js insight "debugging" "Used UE_LOG with LogTemp category for inventory debugging"
```

### Pattern Evolution
Track how patterns evolve over time:
```bash
# Initial pattern
node memory-cli.js pattern "error-handling" "Basic null checks before function calls"

# Later, update frequency when you see it again
# (This happens automatically when the pattern is referenced)
```

### Context Queries
Use specific queries to find relevant context:
```bash
node memory-cli.js context get "input system"
node memory-cli.js context get "blueprint widget"
node memory-cli.js context get "character movement"
```

### Memory Analytics
Get detailed insights into your development:
```bash
# Basic statistics
node memory-cli.js stats

# Detailed analytics
node memory-cli.js stats detail

# Export for external analysis
node memory-cli.js export memory-backup-$(date +%Y%m%d).json
```

## Integration with Development Workflow

### Daily Workflow
1. Start session with day's goals
2. Save context when switching tasks
3. Record insights and patterns as discovered
4. Track any problems encountered
5. End session with summary

### Weekly Review
1. Review session summaries
2. Export memory for backup
3. Clean up old, irrelevant data
4. Analyze patterns and insights for trends

### Project Milestones
1. Export complete memory state
2. Document major patterns discovered
3. Review problem resolution effectiveness
4. Plan improvements based on insights

## Troubleshooting

### Memory File Issues
If memory files become corrupted:
```bash
# Backup current memory
node memory-cli.js export backup-$(date +%Y%m%d).json

# Clean up old data (keeps recent data)
node memory-cli.js cleanup 7

# Start fresh if needed
rm -rf .memory/
node memory-cli.js init "Fresh start after corruption"
```

### Performance Issues
If the memory system becomes slow:
```bash
# Clean old data
node memory-cli.js cleanup 30

# Check memory size
node memory-cli.js stats detail

# Export and import to optimize
node memory-cli.js export optimized.json
rm -rf .memory/
node memory-cli.js import optimized.json
```

## API Integration

The memory system can be integrated into other tools:

```javascript
const MemoryManager = require('./memory-manager');

const memory = new MemoryManager();

// Start a session
const sessionId = memory.startSession('Automated analysis');

// Add insights programmatically
memory.addInsight('automated', 'Found potential memory leak in character spawning', {
  file: 'Source/AtlantisEons/AtlantisEonsCharacter.cpp',
  line: 847
});

// Search for relevant context
const context = memory.getRelevantContext('character spawning', 5);

// Track problems automatically
const problemId = memory.addProblem('Memory leak detected in character spawning');
```

## Contributing

To extend the memory system:

1. **New Insight Types**: Add to `calculateRelevance()` method
2. **New Pattern Categories**: Extend `extractTags()` method  
3. **New Search Strategies**: Modify `search()` method
4. **New Analysis Tools**: Add to MCP server tools

## Support

For issues with the memory system:

1. Check the `.memory/` directory exists and is writable
2. Verify Node.js version (>= 14.0.0 required)
3. Run `node memory-cli.js stats` to check system health
4. Use `node memory-cli.js cleanup` to resolve data issues

The memory system is designed to be resilient and self-healing, automatically managing storage and providing graceful degradation if issues occur. 