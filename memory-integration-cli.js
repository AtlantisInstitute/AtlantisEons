#!/usr/bin/env node

/**
 * Memory Integration CLI for AtlantisEons
 * Provides command-line interface for integrated memory-MCP system
 */

const fs = require('fs');
const path = require('path');
const http = require('http');
const { program } = require('commander');

// Configuration
const projectRoot = '/Users/danielvargas/Documents/Unreal Projects/AtlantisEons';
const mcpServerUrl = 'http://localhost:9011';

class MemoryIntegrationCLI {
    constructor() {
        this.setupCommands();
    }

    setupCommands() {
        program
            .name('memory-integration')
            .description('Memory Integration CLI for AtlantisEons MCP Server')
            .version('2.0.0');

        // Memory sync commands
        program
            .command('sync')
            .description('Sync project knowledge with MCP server')
            .option('-f, --force', 'Force full resync')
            .action(async (options) => {
                await this.syncProjectKnowledge(options.force);
            });

        program
            .command('snapshot')
            .description('Create memory snapshot')
            .option('-d, --description <desc>', 'Snapshot description')
            .option('--no-code', 'Exclude code analysis')
            .option('--no-architecture', 'Exclude architecture mapping')
            .action(async (options) => {
                await this.createSnapshot(options);
            });

        // Memory query commands
        program
            .command('search <query>')
            .description('Search through project memories')
            .option('-c, --categories <categories>', 'Comma-separated categories')
            .option('-t, --time-range <range>', 'Time range: today, week, month, all', 'all')
            .option('-r, --relevance <threshold>', 'Minimum relevance threshold', '0.3')
            .action(async (query, options) => {
                await this.searchMemory(query, options);
            });

        program
            .command('overview')
            .description('Get comprehensive project overview')
            .option('--no-architecture', 'Exclude architecture overview')
            .option('--no-changes', 'Exclude recent changes')
            .option('--no-issues', 'Exclude current issues')
            .action(async (options) => {
                await this.getProjectOverview(options);
            });

        program
            .command('stats')
            .description('Get memory system statistics')
            .option('-d, --details', 'Include detailed breakdown')
            .action(async (options) => {
                await this.getMemoryStats(options.details);
            });

        // Memory management commands
        program
            .command('add-insight')
            .description('Add development insight')
            .requiredOption('-t, --type <type>', 'Insight type')
            .requiredOption('-d, --description <desc>', 'Insight description')
            .option('-i, --impact <level>', 'Impact level: low, medium, high, critical', 'medium')
            .option('-s, --status <status>', 'Implementation status: idea, planned, in-progress, completed', 'idea')
            .option('-c, --context <context>', 'Additional context (JSON string)')
            .action(async (options) => {
                await this.addInsight(options);
            });

        program
            .command('add-pattern')
            .description('Record code pattern')
            .requiredOption('-n, --name <name>', 'Pattern name')
            .requiredOption('-d, --description <desc>', 'Pattern description')
            .option('-e, --examples <examples>', 'Comma-separated examples')
            .option('-c, --category <category>', 'Pattern category: architectural, behavioral, creational, ui, gameplay, optimization')
            .option('--complexity <level>', 'Complexity level: simple, moderate, complex', 'moderate')
            .option('-u, --usage <count>', 'Usage frequency', '1')
            .action(async (options) => {
                await this.addPattern(options);
            });

        program
            .command('track-problem')
            .description('Track development problem')
            .requiredOption('-d, --description <desc>', 'Problem description')
            .option('-s, --severity <level>', 'Problem severity: low, medium, high, critical', 'medium')
            .option('-c, --category <category>', 'Problem category: bug, design, performance, architecture, ui, gameplay')
            .option('-a, --affected <systems>', 'Comma-separated affected systems')
            .option('--context <context>', 'Additional context (JSON string)')
            .action(async (options) => {
                await this.trackProblem(options);
            });

        program
            .command('dependencies <system>')
            .description('Analyze system dependencies')
            .option('-d, --depth <depth>', 'Dependency depth', '3')
            .action(async (system, options) => {
                await this.analyzeSystemDependencies(system, options.depth);
            });

        // Analysis commands
        program
            .command('analyze-class <className>')
            .description('Analyze class with memory context')
            .option('-d, --depth <level>', 'Analysis depth: basic, detailed, comprehensive', 'detailed')
            .option('--no-related', 'Exclude related classes analysis')
            .action(async (className, options) => {
                await this.analyzeClass(className, options);
            });

        // Watch commands
        program
            .command('watch')
            .description('Watch for memory changes')
            .option('-c, --categories <categories>', 'Comma-separated categories to watch')
            .option('--callback <url>', 'Callback URL for notifications')
            .action(async (options) => {
                await this.watchMemoryChanges(options);
            });

        // Server management
        program
            .command('health')
            .description('Check MCP server health')
            .action(async () => {
                await this.checkServerHealth();
            });

        program
            .command('start-server')
            .description('Start MCP server')
            .option('-p, --port <port>', 'Server port', '9011')
            .action(async (options) => {
                await this.startServer(options.port);
            });
    }

    async makeRequest(method, params = {}) {
        return new Promise((resolve, reject) => {
            const data = JSON.stringify({
                jsonrpc: '2.0',
                id: Date.now(),
                method: method,
                params: params
            });

            const options = {
                hostname: 'localhost',
                port: 9011,
                path: '/',
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Content-Length': Buffer.byteLength(data)
                }
            };

            const req = http.request(options, (res) => {
                let body = '';
                res.on('data', chunk => body += chunk);
                res.on('end', () => {
                    try {
                        const response = JSON.parse(body);
                        if (response.error) {
                            reject(new Error(response.error.message || 'Unknown error'));
                        } else {
                            resolve(response);
                        }
                    } catch (error) {
                        reject(error);
                    }
                });
            });

            req.on('error', reject);
            req.write(data);
            req.end();
        });
    }

    async syncProjectKnowledge(force = false) {
        try {
            console.log('🔄 Syncing project knowledge with MCP server...');
            const response = await this.makeRequest('tools/call', {
                name: 'sync_project_knowledge',
                arguments: { force }
            });
            
            const result = JSON.parse(response.content[0].text);
            console.log(`✅ ${result.message}`);
            
            if (result.stats) {
                console.log(`📊 Memory contains ${result.stats.totalItems} items`);
            }
        } catch (error) {
            console.error('❌ Sync failed:', error.message);
        }
    }

    async createSnapshot(options) {
        try {
            console.log('📸 Creating memory snapshot...');
            const response = await this.makeRequest('tools/call', {
                name: 'create_memory_snapshot',
                arguments: {
                    description: options.description || 'CLI snapshot',
                    includeCode: options.code !== false,
                    includeArchitecture: options.architecture !== false
                }
            });
            
            const result = JSON.parse(response.content[0].text);
            console.log(`✅ ${result.summary}`);
            console.log(`📝 Snapshot ID: ${result.snapshotId}`);
        } catch (error) {
            console.error('❌ Snapshot creation failed:', error.message);
        }
    }

    async searchMemory(query, options) {
        try {
            console.log(`🔍 Searching memories for: "${query}"`);
            const response = await this.makeRequest('tools/call', {
                name: 'search_memory',
                arguments: {
                    query,
                    categories: options.categories ? options.categories.split(',') : undefined,
                    timeRange: options.timeRange,
                    relevanceThreshold: parseFloat(options.relevance)
                }
            });
            
            const result = JSON.parse(response.content[0].text);
            console.log(`📋 Found ${result.totalFound} results`);
            
            if (result.filters) {
                console.log(`🔧 Filters applied: ${JSON.stringify(result.filters)}`);
            }
            
            // Display results by category
            Object.keys(result.results).forEach(category => {
                if (result.results[category].length > 0) {
                    console.log(`\n📂 ${category.toUpperCase()}:`);
                    result.results[category].forEach((item, index) => {
                        console.log(`  ${index + 1}. ${item.description || item.name || item.type}`);
                        if (item.timestamp) {
                            console.log(`     📅 ${new Date(item.timestamp).toLocaleDateString()}`);
                        }
                    });
                }
            });
        } catch (error) {
            console.error('❌ Search failed:', error.message);
        }
    }

    async getProjectOverview(options) {
        try {
            console.log('📊 Getting project overview...');
            const response = await this.makeRequest('tools/call', {
                name: 'get_project_overview',
                arguments: {
                    includeArchitecture: options.architecture !== false,
                    includeRecentChanges: options.changes !== false,
                    includeIssues: options.issues !== false
                }
            });
            
            const result = JSON.parse(response.content[0].text);
            const overview = result.overview;
            
            console.log(`\n🏗️  ${overview.projectName} (UE ${overview.engineVersion})`);
            console.log(`📅 Last updated: ${new Date(overview.timestamp).toLocaleString()}`);
            
            if (overview.summary) {
                console.log('\n📈 SUMMARY:');
                console.log(`  Sessions: ${overview.summary.totalSessions}`);
                console.log(`  Insights: ${overview.summary.totalInsights}`);
                console.log(`  Patterns: ${overview.summary.totalPatterns}`);
                console.log(`  Memory Health: ${overview.summary.memoryHealth.status} (${overview.summary.memoryHealth.score}/100)`);
            }
            
            if (overview.architecture) {
                console.log('\n🏛️  ARCHITECTURE:');
                console.log(`  Key Classes: ${overview.architecture.keyClasses.join(', ')}`);
                if (overview.architecture.knowledgeGraph) {
                    console.log(`  Knowledge Graph: ${overview.architecture.knowledgeGraph.metadata?.totalNodes || 0} nodes`);
                }
            }
            
            if (overview.issues) {
                console.log('\n🐛 ISSUES:');
                console.log(`  Open Problems: ${overview.issues.openProblems.length}`);
                console.log(`  Recently Solved: ${overview.issues.recentlySolved.length}`);
                console.log(`  Total Tracked: ${overview.issues.totalTracked}`);
            }
        } catch (error) {
            console.error('❌ Overview failed:', error.message);
        }
    }

    async getMemoryStats(includeDetails = false) {
        try {
            console.log('📊 Getting memory statistics...');
            const response = await this.makeRequest('tools/call', {
                name: 'memory_stats',
                arguments: { includeDetails }
            });
            
            const result = JSON.parse(response.content[0].text);
            console.log('\n📈 MEMORY STATISTICS:');
            console.log(`Session ID: ${result.sessionId}`);
            
            if (includeDetails && result.stats.memory && result.stats.cache) {
                console.log('\n💾 Memory System:');
                console.log(`  Total Items: ${result.stats.memory.totalItems}`);
                console.log(`  Total Sessions: ${result.stats.memory.totalSessions}`);
                console.log(`  Average Session Length: ${result.stats.memory.averageSessionLength}`);
                
                console.log('\n🔄 Cache System:');
                console.log(`  Last Sync: ${result.stats.cache.lastSync}`);
                console.log(`  Cached Items: ${result.stats.cache.cachedItems}`);
                console.log(`  Knowledge Graph Nodes: ${result.stats.cache.knowledgeGraphNodes}`);
            } else {
                Object.keys(result.stats).forEach(key => {
                    if (key !== 'sessionId') {
                        console.log(`  ${key}: ${result.stats[key]}`);
                    }
                });
            }
        } catch (error) {
            console.error('❌ Stats retrieval failed:', error.message);
        }
    }

    async addInsight(options) {
        try {
            console.log('💡 Adding development insight...');
            const response = await this.makeRequest('tools/call', {
                name: 'add_insight',
                arguments: {
                    type: options.type,
                    description: options.description,
                    impact: options.impact,
                    implementationStatus: options.status,
                    context: options.context ? JSON.parse(options.context) : {}
                }
            });
            
            const result = JSON.parse(response.content[0].text);
            console.log(`✅ ${result.message}`);
            console.log(`📝 Insight ID: ${result.insightId}`);
        } catch (error) {
            console.error('❌ Insight addition failed:', error.message);
        }
    }

    async addPattern(options) {
        try {
            console.log('🔄 Recording code pattern...');
            const response = await this.makeRequest('tools/call', {
                name: 'add_pattern',
                arguments: {
                    name: options.name,
                    description: options.description,
                    examples: options.examples ? options.examples.split(',') : [],
                    category: options.category,
                    complexity: options.complexity,
                    usageCount: parseInt(options.usage)
                }
            });
            
            const result = JSON.parse(response.content[0].text);
            console.log(`✅ ${result.message}`);
            console.log(`📝 Pattern ID: ${result.patternId}`);
        } catch (error) {
            console.error('❌ Pattern recording failed:', error.message);
        }
    }

    async trackProblem(options) {
        try {
            console.log('🐛 Tracking development problem...');
            const response = await this.makeRequest('tools/call', {
                name: 'track_problem',
                arguments: {
                    description: options.description,
                    severity: options.severity,
                    category: options.category,
                    affectedSystems: options.affected ? options.affected.split(',') : [],
                    context: options.context ? JSON.parse(options.context) : {}
                }
            });
            
            const result = JSON.parse(response.content[0].text);
            console.log(`✅ ${result.message}`);
            console.log(`📝 Problem ID: ${result.problemId}`);
        } catch (error) {
            console.error('❌ Problem tracking failed:', error.message);
        }
    }

    async analyzeSystemDependencies(system, depth) {
        try {
            console.log(`🔍 Analyzing dependencies for: ${system}`);
            const response = await this.makeRequest('tools/call', {
                name: 'analyze_system_dependencies',
                arguments: {
                    systemName: system,
                    depth: parseInt(depth)
                }
            });
            
            const result = JSON.parse(response.content[0].text);
            const deps = result.dependencies;
            
            console.log(`\n🏗️  DEPENDENCIES FOR ${deps.system}:`);
            console.log(`Analysis Depth: ${deps.depth}`);
            
            if (deps.directDependencies.length > 0) {
                console.log('\n📦 Direct Dependencies:');
                deps.directDependencies.forEach(dep => console.log(`  - ${dep}`));
            }
            
            if (deps.dependents.length > 0) {
                console.log('\n🔗 Dependents:');
                deps.dependents.forEach(dep => console.log(`  - ${dep}`));
            }
            
            console.log('\n⚠️  Risk Assessment:');
            console.log(`  Cyclic Dependencies: ${deps.riskAssessment.cyclicDependencies ? '❌' : '✅'}`);
            console.log(`  High Coupling: ${deps.riskAssessment.highCoupling ? '⚠️' : '✅'}`);
            console.log(`  Critical Path: ${deps.riskAssessment.criticalPath ? '⚠️' : '✅'}`);
        } catch (error) {
            console.error('❌ Dependency analysis failed:', error.message);
        }
    }

    async analyzeClass(className, options) {
        try {
            console.log(`🔍 Analyzing class: ${className}`);
            const response = await this.makeRequest('tools/call', {
                name: 'analyze_class_with_memory',
                arguments: {
                    className,
                    analysisDepth: options.depth,
                    includeRelated: options.related !== false
                }
            });
            
            const result = JSON.parse(response.content[0].text);
            
            console.log(`\n🏛️  CLASS ANALYSIS: ${className}`);
            console.log(`Analysis Depth: ${result.analysisDepth}`);
            console.log(`Include Related: ${result.includeRelated}`);
            
            if (result.classInfo) {
                console.log('\n📋 Class Information:');
                console.log(`  File: ${result.classInfo.file || 'Unknown'}`);
                console.log(`  Lines: ${result.classInfo.lineCount || 'Unknown'}`);
            }
            
            if (result.relevantContext && result.relevantContext.length > 0) {
                console.log('\n🧠 Relevant Context:');
                result.relevantContext.forEach((ctx, index) => {
                    console.log(`  ${index + 1}. ${ctx.summary}`);
                });
            }
            
            if (result.recommendations && result.recommendations.length > 0) {
                console.log('\n💡 Recommendations:');
                result.recommendations.forEach((rec, index) => {
                    console.log(`  ${index + 1}. ${rec}`);
                });
            }
        } catch (error) {
            console.error('❌ Class analysis failed:', error.message);
        }
    }

    async watchMemoryChanges(options) {
        try {
            console.log('👁️  Setting up memory change watcher...');
            const response = await this.makeRequest('tools/call', {
                name: 'watch_memory_changes',
                arguments: {
                    categories: options.categories ? options.categories.split(',') : undefined,
                    callback: options.callback
                }
            });
            
            const result = JSON.parse(response.content[0].text);
            console.log(`✅ ${result.message}`);
            
            if (result.categories) {
                console.log(`📂 Watching categories: ${result.categories.join(', ')}`);
            }
        } catch (error) {
            console.error('❌ Watch setup failed:', error.message);
        }
    }

    async checkServerHealth() {
        try {
            console.log('🏥 Checking MCP server health...');
            const response = await this.makeRequest('tools/call', {
                name: 'memory_stats',
                arguments: { includeDetails: true }
            });
            
            console.log('✅ MCP server is healthy and responding');
            
            const result = JSON.parse(response.content[0].text);
            if (result.stats.cache && result.stats.cache.lastSync) {
                const lastSync = new Date(result.stats.cache.lastSync);
                const timeSinceSync = Date.now() - lastSync.getTime();
                const minutesSinceSync = Math.floor(timeSinceSync / 60000);
                
                console.log(`🔄 Last sync: ${minutesSinceSync} minutes ago`);
                
                if (minutesSinceSync > 10) {
                    console.log('⚠️  Memory cache may be stale - consider running sync');
                }
            }
        } catch (error) {
            console.error('❌ Server health check failed:', error.message);
            console.log('💡 Make sure the MCP server is running on port 9011');
        }
    }

    async startServer(port) {
        const { spawn } = require('child_process');
        
        console.log(`🚀 Starting MCP server on port ${port}...`);
        
        const serverProcess = spawn('node', ['memory-mcp-server.js'], {
            cwd: projectRoot,
            stdio: 'inherit',
            env: { ...process.env, MCP_PORT: port }
        });
        
        serverProcess.on('close', (code) => {
            console.log(`📴 MCP server exited with code ${code}`);
        });
        
        serverProcess.on('error', (error) => {
            console.error('❌ Failed to start MCP server:', error.message);
        });
        
        // Give the server time to start
        setTimeout(() => {
            console.log('✅ MCP server should be running. Use "health" command to verify.');
        }, 2000);
    }

    run() {
        program.parse();
    }
}

// Run CLI if called directly
if (require.main === module) {
    const cli = new MemoryIntegrationCLI();
    cli.run();
}

module.exports = MemoryIntegrationCLI; 