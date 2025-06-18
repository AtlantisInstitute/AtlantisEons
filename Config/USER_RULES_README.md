# AtlantisEons MCP Server - User Rules System

## Overview

The AtlantisEons MCP Server now includes a comprehensive user rules management system that allows you to store, update, and access your coding preferences and project guidelines across multiple context windows. This system ensures consistency in your development workflow and provides AI assistants with your specific preferences.

## Features

- **Persistent Storage**: Rules are stored in `config/user-rules.json` and persist across server restarts
- **Categorized Organization**: Rules are organized into logical categories for easy management
- **Multiple Formats**: Access rules in JSON, Markdown, or Plain Text formats
- **Dynamic Updates**: Add, update, or remove rules without restarting the server
- **Backup Support**: Automatic backup creation when resetting rules
- **Resource Access**: Rules are available as MCP resources for easy integration

## Rule Categories

### 1. Code Style (`codeStyle`)
- **indentation**: Indentation preferences (4 spaces for most languages, 2 for web)
- **conventions**: Language-specific conventions (PEP 8, Google Style, etc.)
- **naming**: Variable and function naming guidelines
- **comments**: Commenting standards
- **lineLength**: Line length limits (100 characters)
- **preservation**: Code preservation guidelines
- **modification**: Code modification preferences
- **headers**: Header file inclusion requirements

### 2. Project Structure (`projectStructure`)
- **organization**: File organization principles
- **naming**: File naming conventions
- **proximity**: File proximity guidelines
- **separation**: Code separation standards
- **engine**: Engine version (Unreal Engine 5.5)
- **platform**: Platform-specific guidelines (Mac terminal usage)

### 3. Communication Preferences (`communicationPreferences`)
- **explanations**: Explanation style preferences
- **decisions**: Decision documentation requirements
- **formatting**: Formatting preferences (Markdown)
- **implementation**: Implementation approach (direct code edits)
- **issues**: Issue highlighting requirements

### 4. Task Approach (`taskApproach`)
- **breakdown**: Problem breakdown methodology
- **priorities**: Code quality priorities
- **performance**: Performance consideration guidelines
- **memory**: Memory analysis preferences
- **clarification**: Clarification protocols
- **assumptions**: Assumption-making guidelines
- **building**: Build process preferences
- **analysis**: Code analysis requirements
- **implementation**: Implementation preferences (C++ over Blueprints)
- **inspection**: Code inspection requirements
- **fileSize**: File size limitations
- **characterFile**: Specific file handling rules
- **editorCheck**: Editor instance checking
- **codeIntegrity**: Code integrity requirements

## Available MCP Tools

### `get_user_rules`
Retrieve current user rules configuration.

**Parameters:**
- `category` (optional): Specific category to retrieve ('codeStyle', 'projectStructure', 'communicationPreferences', 'taskApproach', or 'all')
- `format` (optional): Return format ('json', 'markdown', 'plain')

**Example:**
```json
{
  "method": "tools/call",
  "params": {
    "name": "get_user_rules",
    "arguments": {
      "category": "codeStyle",
      "format": "markdown"
    }
  }
}
```

### `update_user_rules`
Update multiple rules in a category.

**Parameters:**
- `category` (required): Rule category
- `rules` (required): Object containing rules to update
- `merge` (optional): Whether to merge with existing rules (default: true)

**Example:**
```json
{
  "method": "tools/call",
  "params": {
    "name": "update_user_rules",
    "arguments": {
      "category": "codeStyle",
      "rules": {
        "lineLength": "120 characters for C++ files",
        "newRule": "Always use const correctness"
      },
      "merge": true
    }
  }
}
```

### `add_user_rule`
Add a single new rule to a category.

**Parameters:**
- `category` (required): Rule category
- `key` (required): Rule key/name
- `value` (required): Rule value/description
- `description` (optional): Additional rule description

**Example:**
```json
{
  "method": "tools/call",
  "params": {
    "name": "add_user_rule",
    "arguments": {
      "category": "taskApproach",
      "key": "testing",
      "value": "Always include unit tests for new functionality"
    }
  }
}
```

### `remove_user_rule`
Remove a specific rule from a category.

**Parameters:**
- `category` (required): Rule category
- `key` (required): Rule key to remove

**Example:**
```json
{
  "method": "tools/call",
  "params": {
    "name": "remove_user_rule",
    "arguments": {
      "category": "codeStyle",
      "key": "oldRule"
    }
  }
}
```

### `reset_user_rules`
Reset rules to default configuration.

**Parameters:**
- `category` (optional): Specific category to reset or 'all' (default: 'all')
- `backup` (optional): Create backup before reset (default: true)

**Example:**
```json
{
  "method": "tools/call",
  "params": {
    "name": "reset_user_rules",
    "arguments": {
      "category": "all",
      "backup": true
    }
  }
}
```

### `export_user_rules`
Export user rules to a file or return formatted text.

**Parameters:**
- `format` (optional): Export format ('json', 'markdown', 'yaml')
- `filePath` (optional): File path to save to
- `includeMetadata` (optional): Include metadata like timestamps (default: true)

**Example:**
```json
{
  "method": "tools/call",
  "params": {
    "name": "export_user_rules",
    "arguments": {
      "format": "markdown",
      "filePath": "/path/to/exported-rules.md"
    }
  }
}
```

## Available MCP Resources

### `rules://user-rules`
Current user rules in JSON format.

### `rules://user-rules/markdown`
Current user rules in Markdown format.

### `rules://user-rules/plain`
Current user rules in plain text format.

## Usage Examples

### Using curl to interact with the server:

```bash
# Get all rules in JSON format
curl -X POST -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"get_user_rules","arguments":{"format":"json"}}}' \
  http://localhost:9011

# Add a new rule
curl -X POST -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"add_user_rule","arguments":{"category":"codeStyle","key":"newRule","value":"Use smart pointers for memory management"}}}' \
  http://localhost:9011

# Access rules as a resource
curl -X POST -H "Content-Type: application/json" \
  -d '{"method":"resources/read","params":{"uri":"rules://user-rules/markdown"}}' \
  http://localhost:9011
```

## File Structure

```
config/
├── user-rules.json          # Main rules configuration file
└── backups/                 # Automatic backups
    ├── user-rules-backup-*.json
    └── ...
```

## Current Rules Summary

Your current rules include comprehensive guidelines for:
- Code style and formatting preferences
- Project structure and organization
- Communication and documentation standards
- Task approach and development methodology
- Unreal Engine 5.5 specific requirements
- Build process automation
- File size and code organization limits

## Integration with MCP

The user rules system is fully integrated with the MCP protocol, making it accessible to any AI assistant or tool that supports MCP. This ensures your preferences are consistently applied across different contexts and tools.

## Backup and Recovery

- Automatic backups are created when using the `reset_user_rules` tool
- Backups are stored in `config/backups/` with timestamps
- Only the last 10 backups are kept to prevent excessive disk usage
- Manual backups can be created using the `export_user_rules` tool

## Server Integration

The user rules system is automatically initialized when the MCP server starts and is immediately available for use. Changes are persisted to disk and available across server restarts. 