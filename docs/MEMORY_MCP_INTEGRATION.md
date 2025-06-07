# AtlantisEons Memory-MCP Integration

## Overview

This integration merges the AtlantisEons memory system with the MCP (Model Context Protocol) server, creating a powerful, self-aware development environment that automatically learns and adapts to your project patterns.

## Key Features

### 🧠 **Intelligent Memory System**
- **Real-time Sync**: Memory automatically syncs with MCP server every 30 seconds
- **Auto-Integration**: New memories are immediately available to the MCP server
- **Knowledge Graph**: Dynamic project knowledge graph built from memory data
- **Smart Caching**: Optimized caching system for fast access to project knowledge

### 🔄 **Enhanced Memory Tools**
- **Enhanced Insights**: Include impact levels and implementation status
- **Pattern Recognition**: Categorized patterns with complexity levels
- **Problem Tracking**: Severity-based problem tracking with solution outcomes
- **Dependency Analysis**: Analyze system dependencies with risk assessment

### 📊 **Real-time Monitoring**
- **Memory Health**: Continuous health monitoring with recommendations
- **Change Watchers**: Real-time monitoring of memory changes
- **Project Overview**: Comprehensive project state with architecture insights
- **Snapshots**: Create point-in-time memory snapshots

## Quick Start

### 1. Start the Integration
```bash
# Start the complete system
./start-memory-integration.sh

# Or start on a custom port
MCP_PORT=9012 ./start-memory-integration.sh
```

### 2. Check System Status
```bash
./start-memory-integration.sh status
```

### 3. Use the CLI
```bash
# Search memories
./memory-integration-cli.js search "character system"

# Get project overview
./memory-integration-cli.js overview

# Add an insight
./memory-integration-cli.js add-insight -t "architecture" -d "Character system needs refactoring" -i "high"

# Analyze dependencies
./memory-integration-cli.js dependencies AAtlantisEonsCharacter
```

## Architecture

### Components

1. **Memory Manager** (`memory-manager.js`)
   - Core memory storage and retrieval
   - Session management
   - Pattern recognition
   - Problem tracking

2. **Enhanced MCP Server** (`memory-mcp-server.js`)
   - Integrated with memory system
   - Real-time synchronization
   - Enhanced tools and resources
   - Auto-sync capabilities

3. **Integration CLI** (`memory-integration-cli.js`)
   - Command-line interface
   - Memory management tools
   - Analysis commands
   - System monitoring

4. **Startup Script** (`start-memory-integration.sh`)
   - Automated system initialization
   - Environment validation
   - Health monitoring
   - Log management

### Data Flow

```
Project Code Changes
        ↓
Memory System (Auto-capture)
        ↓
Real-time Sync (Every 30s)
        ↓
MCP Server (Enhanced Tools)
        ↓
AI Assistant (Full Context)
```

## Available Tools

### Memory Management
- `save_context` - Save development context with auto-MCP integration
- `search_memory` - Search memories with enhanced filtering
- `create_memory_snapshot` - Create comprehensive project snapshots
- `sync_project_knowledge` - Manual sync with force option

### Enhanced Insights
- `add_insight` - Add insights with impact and implementation status
- `add_pattern` - Record patterns with categories and complexity
- `track_problem` - Track problems with severity and affected systems
- `add_solution` - Add solutions with implementation time and side effects

### Analysis Tools
- `analyze_class_with_memory` - Comprehensive class analysis
- `get_project_overview` - Full project state overview
- `analyze_system_dependencies` - Dependency analysis with risk assessment
- `memory_stats` - Detailed memory system statistics

### Real-time Features
- `watch_memory_changes` - Monitor memory changes in real-time
- `get_relevant_context` - Smart context retrieval with filtering

## Resources

The integration provides several MCP resources:

- `memory://project` - Complete project memory with real-time updates
- `memory://knowledge-graph` - Dynamic project knowledge graph
- `memory://architecture` - Current project architecture overview
- `memory://insights/recent` - Most recent development insights
- `memory://problems` - Problem tracking with status
- `memory://sessions` - Development session history

## CLI Commands

### System Management
```bash
# Start/stop/restart
./start-memory-integration.sh start
./start-memory-integration.sh stop
./start-memory-integration.sh restart

# Monitor
./start-memory-integration.sh status
./start-memory-integration.sh logs

# Backup
./start-memory-integration.sh backup
```

### Memory Operations
```bash
# Sync project knowledge
./memory-integration-cli.js sync --force

# Create snapshot
./memory-integration-cli.js snapshot -d "Before major refactor"

# Search memories
./memory-integration-cli.js search "inventory system" -c "insights,patterns" -t "week"

# Get overview
./memory-integration-cli.js overview --no-issues

# Check statistics
./memory-integration-cli.js stats --details
```

### Development Insights
```bash
# Add insight
./memory-integration-cli.js add-insight \
  -t "performance" \
  -d "Character movement needs optimization" \
  -i "high" \
  -s "planned"

# Record pattern
./memory-integration-cli.js add-pattern \
  -n "UE5-Widget-Pattern" \
  -d "Standard widget initialization pattern" \
  -c "ui" \
  --complexity "simple"

# Track problem
./memory-integration-cli.js track-problem \
  -d "Inventory UI lag when opening" \
  -s "medium" \
  -c "performance" \
  -a "InventoryWidget,WBP_Inventory"
```

### Analysis
```bash
# Analyze class
./memory-integration-cli.js analyze-class AAtlantisEonsCharacter -d "comprehensive"

# Check dependencies
./memory-integration-cli.js dependencies InventoryComponent --depth 5

# Health check
./memory-integration-cli.js health
```

## Configuration

### Environment Variables
- `MCP_PORT` - Server port (default: 9011)

### File Locations
- Memory data: `.memory/`
- Server logs: `memory-integration.log`
- Backups: `.memory/backups/`
- PID file: `.mcp-server.pid`

## Integration Benefits

### For Development
1. **Context Awareness**: AI assistant has complete project context
2. **Pattern Recognition**: Automatically identifies and suggests code patterns
3. **Problem Resolution**: Learns from past solutions to suggest fixes
4. **Architecture Insights**: Provides architectural recommendations

### For Project Management
1. **Progress Tracking**: Monitors development sessions and progress
2. **Knowledge Preservation**: Preserves insights across development cycles
3. **Risk Assessment**: Identifies architectural and dependency risks
4. **Health Monitoring**: Continuous project health assessment

### For Team Collaboration
1. **Shared Knowledge**: Team insights are preserved and accessible
2. **Best Practices**: Automatically captures and promotes best practices
3. **Onboarding**: New team members can quickly understand project patterns
4. **Documentation**: Auto-generates living documentation from memory

## Troubleshooting

### Common Issues

1. **Server won't start**
   ```bash
   # Check if port is in use
   lsof -i :9011
   
   # Force stop and restart
   ./start-memory-integration.sh stop
   ./start-memory-integration.sh start
   ```

2. **Memory sync issues**
   ```bash
   # Force resync
   ./memory-integration-cli.js sync --force
   
   # Check health
   ./memory-integration-cli.js health
   ```

3. **Performance issues**
   ```bash
   # Check memory stats
   ./memory-integration-cli.js stats --details
   
   # Create backup and cleanup
   ./start-memory-integration.sh backup
   ```

### Logs and Debugging
- Server logs: `tail -f memory-integration.log`
- Memory directory: `.memory/`
- Health check: `./memory-integration-cli.js health`

## Auto-Sync Features

The integration automatically:
- Syncs memory to MCP server every 30 seconds
- Updates knowledge graph when new insights are added
- Monitors file changes in the memory directory
- Creates backups before major operations
- Maintains memory health and provides recommendations

## Future Enhancements

- [ ] Web dashboard for visual memory exploration
- [ ] Integration with Unreal Engine editor
- [ ] Automated code analysis and pattern detection
- [ ] Machine learning for intelligent suggestions
- [ ] Team collaboration features
- [ ] Integration with version control systems

---

*The AtlantisEons Memory-MCP Integration creates a self-aware development environment that learns from your patterns and helps you build better code faster.* 