#!/usr/bin/env node

/**
 * Memory CLI for AtlantisEons Project
 * Command line interface for memory management
 */

const MemoryManager = require('./memory-manager');
const path = require('path');
const fs = require('fs');

class MemoryCLI {
    constructor() {
        this.manager = new MemoryManager();
        this.commands = new Map();
        this.setupCommands();
    }

    setupCommands() {
        this.commands.set('init', {
            description: 'Initialize memory system for the project',
            usage: 'init [session-description]',
            handler: this.handleInit.bind(this)
        });

        this.commands.set('session', {
            description: 'Session management',
            usage: 'session <start|end|list> [description]',
            handler: this.handleSession.bind(this)
        });

        this.commands.set('save', {
            description: 'Save context window',
            usage: 'save <file-path> [description]',
            handler: this.handleSave.bind(this)
        });

        this.commands.set('search', {
            description: 'Search memory',
            usage: 'search <query> [categories]',
            handler: this.handleSearch.bind(this)
        });

        this.commands.set('insight', {
            description: 'Add development insight',
            usage: 'insight <type> <description> [context-file]',
            handler: this.handleInsight.bind(this)
        });

        this.commands.set('pattern', {
            description: 'Record code pattern',
            usage: 'pattern <name> <description> [examples...]',
            handler: this.handlePattern.bind(this)
        });

        this.commands.set('problem', {
            description: 'Track problem',
            usage: 'problem <action> [args...]',
            handler: this.handleProblem.bind(this)
        });

        this.commands.set('analyze', {
            description: 'Analyze code with memory context',
            usage: 'analyze <class|file|system> <target>',
            handler: this.handleAnalyze.bind(this)
        });

        this.commands.set('stats', {
            description: 'Show memory statistics',
            usage: 'stats [detail]',
            handler: this.handleStats.bind(this)
        });

        this.commands.set('export', {
            description: 'Export memory data',
            usage: 'export [output-file]',
            handler: this.handleExport.bind(this)
        });

        this.commands.set('import', {
            description: 'Import memory data',
            usage: 'import <input-file>',
            handler: this.handleImport.bind(this)
        });

        this.commands.set('cleanup', {
            description: 'Clean old memory data',
            usage: 'cleanup [days-old]',
            handler: this.handleCleanup.bind(this)
        });

        this.commands.set('context', {
            description: 'Context window management',
            usage: 'context <save|get|list> [args...]',
            handler: this.handleContext.bind(this)
        });
    }

    async handleInit(args) {
        const description = args.join(' ') || 'CLI initialization session';
        const sessionId = this.manager.startSession(description);
        
        // Add initial project insights
        this.manager.addInsight(
            'project-init',
            'Memory system initialized for AtlantisEons project',
            {
                sessionId,
                timestamp: new Date().toISOString(),
                projectType: 'Unreal Engine 5.5 Game'
            }
        );

        console.log(`✅ Memory system initialized`);
        console.log(`📝 Session started: ${sessionId}`);
        console.log(`💡 Added initial project insights`);
        
        return { success: true, sessionId };
    }

    async handleSession(args) {
        const action = args[0];
        
        switch (action) {
            case 'start':
                const description = args.slice(1).join(' ') || 'New development session';
                const sessionId = this.manager.startSession(description);
                console.log(`🚀 Session started: ${sessionId}`);
                console.log(`📝 Description: ${description}`);
                break;

            case 'end':
                const summary = args.slice(1).join(' ') || 'Session ended via CLI';
                this.manager.endSession(summary);
                console.log(`✅ Session ended`);
                console.log(`📋 Summary: ${summary}`);
                break;

            case 'list':
                const sessions = this.manager.memory.sessions;
                console.log(`📊 Total sessions: ${sessions.length}`);
                sessions.slice(-5).forEach(session => {
                    console.log(`  ${session.id} - ${session.description} (${session.started})`);
                });
                break;

            default:
                console.log('❌ Usage: session <start|end|list> [description]');
        }
    }

    async handleSave(args) {
        const filePath = args[0];
        const description = args.slice(1).join(' ');

        if (!filePath) {
            console.log('❌ Usage: save <file-path> [description]');
            return;
        }

        try {
            // Try to read and analyze the file
            const fullPath = path.resolve(filePath);
            const content = fs.readFileSync(fullPath, 'utf8');
            
            const contextData = {
                filePath: fullPath,
                fileName: path.basename(fullPath),
                fileSize: content.length,
                lineCount: content.split('\n').length,
                description: description || `Context from ${path.basename(fullPath)}`,
                timestamp: new Date().toISOString(),
                preview: content.substring(0, 500) + (content.length > 500 ? '...' : '')
            };

            const contextId = this.manager.saveContextWindow(contextData);
            console.log(`💾 Context saved: ${contextId}`);
            console.log(`📁 File: ${path.basename(fullPath)}`);
            console.log(`📏 Size: ${content.length} chars, ${contextData.lineCount} lines`);
            
        } catch (error) {
            console.log(`❌ Error saving context: ${error.message}`);
        }
    }

    async handleSearch(args) {
        const query = args[0];
        const categories = args.slice(1);

        if (!query) {
            console.log('❌ Usage: search <query> [categories...]');
            return;
        }

        const results = this.manager.search(query, categories.length ? categories : undefined);
        const totalResults = Object.values(results).reduce((sum, arr) => sum + arr.length, 0);

        console.log(`🔍 Search results for "${query}": ${totalResults} found`);
        
        if (results.insights.length > 0) {
            console.log(`\n💡 Insights (${results.insights.length}):`);
            results.insights.slice(0, 3).forEach(insight => {
                console.log(`  • ${insight.type}: ${insight.description}`);
            });
        }

        if (results.patterns.length > 0) {
            console.log(`\n🔄 Patterns (${results.patterns.length}):`);
            results.patterns.slice(0, 3).forEach(pattern => {
                console.log(`  • ${pattern.name}: ${pattern.description}`);
            });
        }

        if (results.problems.length > 0) {
            console.log(`\n❓ Problems (${results.problems.length}):`);
            results.problems.slice(0, 3).forEach(problem => {
                console.log(`  • ${problem.description} (${problem.status})`);
            });
        }

        if (results.contexts.length > 0) {
            console.log(`\n📝 Context Windows (${results.contexts.length}):`);
            results.contexts.slice(0, 3).forEach(ctx => {
                console.log(`  • ${ctx.summary}`);
            });
        }
    }

    async handleInsight(args) {
        const type = args[0];
        const description = args[1];
        const contextFile = args[2];

        if (!type || !description) {
            console.log('❌ Usage: insight <type> <description> [context-file]');
            return;
        }

        let context = {};
        if (contextFile) {
            try {
                const content = fs.readFileSync(contextFile, 'utf8');
                context = { 
                    file: contextFile,
                    preview: content.substring(0, 200) + '...'
                };
            } catch (error) {
                console.log(`⚠️  Could not read context file: ${error.message}`);
            }
        }

        const insightId = this.manager.addInsight(type, description, context);
        console.log(`💡 Insight added: ${insightId}`);
        console.log(`📝 Type: ${type}`);
        console.log(`📋 Description: ${description}`);
    }

    async handlePattern(args) {
        const name = args[0];
        const description = args[1];
        const examples = args.slice(2);

        if (!name || !description) {
            console.log('❌ Usage: pattern <name> <description> [examples...]');
            return;
        }

        const patternId = this.manager.addPattern(name, description, examples);
        console.log(`🔄 Pattern recorded: ${patternId}`);
        console.log(`📝 Name: ${name}`);
        console.log(`📋 Description: ${description}`);
        if (examples.length > 0) {
            console.log(`📚 Examples: ${examples.length}`);
        }
    }

    async handleProblem(args) {
        const action = args[0];

        switch (action) {
            case 'track':
                const description = args.slice(1).join(' ');
                if (!description) {
                    console.log('❌ Usage: problem track <description>');
                    return;
                }
                const problemId = this.manager.addProblem(description);
                console.log(`❓ Problem tracked: ${problemId}`);
                console.log(`📋 Description: ${description}`);
                break;

            case 'solve':
                const problemIdToSolve = args[1];
                const solution = args.slice(2).join(' ');
                if (!problemIdToSolve || !solution) {
                    console.log('❌ Usage: problem solve <problem-id> <solution>');
                    return;
                }
                this.manager.addSolution(problemIdToSolve, solution, true);
                console.log(`✅ Solution added to problem: ${problemIdToSolve}`);
                break;

            case 'list':
                const problems = this.manager.memory.problemsSolved;
                console.log(`❓ Total problems: ${problems.length}`);
                problems.slice(-5).forEach(problem => {
                    console.log(`  ${problem.id} - ${problem.description} (${problem.status})`);
                });
                break;

            default:
                console.log('❌ Usage: problem <track|solve|list> [args...]');
        }
    }

    async handleAnalyze(args) {
        const type = args[0];
        const target = args[1];

        if (!type || !target) {
            console.log('❌ Usage: analyze <class|file|system> <target>');
            return;
        }

        switch (type) {
            case 'class':
                const classInfo = this.manager.getClassInfo(target);
                const context = this.manager.getRelevantContext(target, 3);
                
                console.log(`🔍 Analyzing class: ${target}`);
                if (classInfo) {
                    console.log(`📁 File: ${classInfo.file}`);
                    console.log(`📏 Lines: ${classInfo.lineCount}`);
                    console.log(`🔧 Has Blueprint: ${classInfo.hasBlueprint ? 'Yes' : 'No'}`);
                    console.log(`📋 Relevant context: ${context.length} entries`);
                } else {
                    console.log(`❌ Class not found in memory`);
                }
                break;

            case 'system':
                if (target === 'character') {
                    const characterClasses = ['AAtlantisEonsCharacter', 'AZombieCharacter'];
                    console.log(`🔍 Analyzing character system`);
                    characterClasses.forEach(className => {
                        const info = this.manager.getClassInfo(className);
                        if (info) {
                            console.log(`  • ${className}: ${info.lineCount} lines`);
                        }
                    });
                } else if (target === 'inventory') {
                    const inventoryClasses = ['UBP_ItemInfo', 'ABP_Item', 'UWBP_Inventory'];
                    console.log(`🔍 Analyzing inventory system`);
                    inventoryClasses.forEach(className => {
                        const info = this.manager.getClassInfo(className);
                        if (info) {
                            console.log(`  • ${className}: ${info.lineCount} lines`);
                        }
                    });
                }
                break;

            default:
                console.log(`❌ Unknown analysis type: ${type}`);
        }
    }

    async handleStats(args) {
        const detail = args[0] === 'detail';
        const stats = this.manager.getMemoryStats();

        console.log(`📊 Memory Statistics:`);
        console.log(`📝 Sessions: ${stats.totalSessions}`);
        console.log(`💡 Insights: ${stats.totalInsights}`);
        console.log(`🔄 Patterns: ${stats.totalPatterns}`);
        console.log(`❓ Problems: ${stats.totalProblems} (${stats.solvedProblems} solved)`);
        console.log(`📋 Context Windows: ${stats.contextWindows}`);
        console.log(`🏗️  Key Classes: ${stats.keyClasses}`);
        console.log(`📁 Important Files: ${stats.importantFiles}`);
        console.log(`💾 Memory Size: ${(stats.memorySize / 1024).toFixed(1)} KB`);
        console.log(`🕒 Last Updated: ${new Date(stats.lastUpdated).toLocaleString()}`);

        if (detail) {
            const insights = this.manager.getInsights(null, 0.5);
            const patterns = this.manager.getPatterns(0.5);
            
            if (insights.length > 0) {
                console.log(`\n💡 Top Insights:`);
                insights.slice(0, 3).forEach(insight => {
                    console.log(`  • ${insight.type}: ${insight.description} (${insight.relevance.toFixed(2)})`);
                });
            }

            if (patterns.length > 0) {
                console.log(`\n🔄 Top Patterns:`);
                patterns.slice(0, 3).forEach(pattern => {
                    console.log(`  • ${pattern.name}: ${pattern.frequency} occurrences`);
                });
            }
        }
    }

    async handleExport(args) {
        const outputFile = args[0];
        const exportPath = this.manager.exportMemory(outputFile);
        
        if (exportPath) {
            console.log(`📤 Memory exported to: ${exportPath}`);
            const stats = fs.statSync(exportPath);
            console.log(`📏 File size: ${(stats.size / 1024).toFixed(1)} KB`);
        } else {
            console.log(`❌ Export failed`);
        }
    }

    async handleImport(args) {
        const inputFile = args[0];
        
        if (!inputFile) {
            console.log('❌ Usage: import <input-file>');
            return;
        }

        const success = this.manager.importMemory(inputFile);
        if (success) {
            console.log(`📥 Memory imported from: ${inputFile}`);
            const stats = this.manager.getMemoryStats();
            console.log(`📊 New totals: ${stats.totalInsights} insights, ${stats.totalPatterns} patterns`);
        } else {
            console.log(`❌ Import failed`);
        }
    }

    async handleCleanup(args) {
        const days = parseInt(args[0]) || 30;
        this.manager.cleanup(days);
        console.log(`🧹 Cleaned up data older than ${days} days`);
        
        const stats = this.manager.getMemoryStats();
        console.log(`📊 Current totals: ${stats.totalInsights} insights, ${stats.contextWindows} contexts`);
    }

    async handleContext(args) {
        const action = args[0];

        switch (action) {
            case 'save':
                const description = args.slice(1).join(' ') || 'Manual context save';
                const contextData = {
                    description,
                    timestamp: new Date().toISOString(),
                    source: 'CLI',
                    args: args.slice(1)
                };
                const contextId = this.manager.saveContextWindow(contextData);
                console.log(`💾 Context saved: ${contextId}`);
                break;

            case 'get':
                const query = args[1];
                if (!query) {
                    console.log('❌ Usage: context get <query>');
                    return;
                }
                const contexts = this.manager.getRelevantContext(query, 5);
                console.log(`📋 Found ${contexts.length} relevant contexts for "${query}"`);
                contexts.forEach(ctx => {
                    console.log(`  • ${ctx.summary} (${new Date(ctx.timestamp).toLocaleString()})`);
                });
                break;

            case 'list':
                const allContexts = this.manager.memory.contextWindows;
                console.log(`📋 Total context windows: ${allContexts.length}`);
                allContexts.slice(-5).forEach(ctx => {
                    console.log(`  ${ctx.id} - ${ctx.summary} (${new Date(ctx.timestamp).toLocaleString()})`);
                });
                break;

            default:
                console.log('❌ Usage: context <save|get|list> [args...]');
        }
    }

    showHelp() {
        console.log(`
🧠 AtlantisEons Memory CLI

Usage: node memory-cli.js <command> [args...]

Commands:
`);
        
        this.commands.forEach((cmd, name) => {
            console.log(`  ${name.padEnd(12)} ${cmd.description}`);
            console.log(`  ${' '.repeat(12)} Usage: ${cmd.usage}`);
            console.log();
        });

        console.log(`Examples:
  node memory-cli.js init "Starting character system work"
  node memory-cli.js save Source/AtlantisEons/AtlantisEonsCharacter.cpp "Main character class"
  node memory-cli.js search "inventory" insights patterns
  node memory-cli.js insight "bug-fix" "Fixed inventory slot duplication issue"
  node memory-cli.js pattern "widget-naming" "Widgets use WBP_ prefix" WBP_Main WBP_Inventory
  node memory-cli.js stats detail
`);
    }

    async run() {
        const args = process.argv.slice(2);
        const command = args[0];
        const commandArgs = args.slice(1);

        if (!command || command === 'help' || command === '--help' || command === '-h') {
            this.showHelp();
            return;
        }

        const cmd = this.commands.get(command);
        if (!cmd) {
            console.log(`❌ Unknown command: ${command}`);
            console.log(`💡 Run 'node memory-cli.js help' for available commands`);
            return;
        }

        try {
            await cmd.handler(commandArgs);
        } catch (error) {
            console.log(`❌ Error executing command: ${error.message}`);
            console.log(`💡 Usage: ${cmd.usage}`);
        }
    }
}

// Run CLI if this file is executed directly
if (require.main === module) {
    const cli = new MemoryCLI();
    cli.run().catch(error => {
        console.error('❌ CLI Error:', error.message);
        process.exit(1);
    });
}

module.exports = MemoryCLI; 