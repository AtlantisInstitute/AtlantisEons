#!/usr/bin/env node

/**
 * Memory-Enhanced MCP Server for AtlantisEons
 * Integrates memory management with code analysis
 */

const fs = require('fs');
const path = require('path');
const http = require('http');
const { exec } = require('child_process');
const MemoryManager = require('./memory-manager');

// Configuration
const projectRoot = '/Users/danielvargas/Documents/Unreal Projects/AtlantisEons';
const serverPort = process.env.MCP_PORT ? parseInt(process.env.MCP_PORT) : 9011;
const serverName = 'memory-enhanced-analyzer';
const serverVersion = '1.0.0';

// Initialize memory manager
const memoryManager = new MemoryManager(projectRoot);

// Start a new session
const sessionId = memoryManager.startSession('MCP Memory-Enhanced Analysis Session');

// MCP Server Implementation
class MemoryMCPServer {
    constructor() {
        this.tools = new Map();
        this.resources = new Map();
        this.prompts = new Map();
        
        this.initializeTools();
        this.initializeResources();
        this.initializePrompts();
        
        // Load existing project knowledge
        this.loadProjectKnowledge();
    }

    initializeTools() {
        // Memory tools
        this.tools.set('save_context', {
            name: 'save_context',
            description: 'Save current context for future reference',
            inputSchema: {
                type: 'object',
                properties: {
                    contextData: { type: 'object', description: 'Context data to save' },
                    description: { type: 'string', description: 'Description of the context' }
                },
                required: ['contextData']
            }
        });

        this.tools.set('search_memory', {
            name: 'search_memory',
            description: 'Search through saved memories',
            inputSchema: {
                type: 'object',
                properties: {
                    query: { type: 'string', description: 'Search query' },
                    categories: { 
                        type: 'array', 
                        items: { type: 'string' },
                        description: 'Categories to search: insights, patterns, problems, history, contexts'
                    }
                },
                required: ['query']
            }
        });

        this.tools.set('add_insight', {
            name: 'add_insight',
            description: 'Add a development insight',
            inputSchema: {
                type: 'object',
                properties: {
                    type: { type: 'string', description: 'Type of insight' },
                    description: { type: 'string', description: 'Insight description' },
                    context: { type: 'object', description: 'Additional context' }
                },
                required: ['type', 'description']
            }
        });

        this.tools.set('add_pattern', {
            name: 'add_pattern',
            description: 'Record a code pattern',
            inputSchema: {
                type: 'object',
                properties: {
                    name: { type: 'string', description: 'Pattern name' },
                    description: { type: 'string', description: 'Pattern description' },
                    examples: { type: 'array', items: { type: 'string' }, description: 'Code examples' }
                },
                required: ['name', 'description']
            }
        });

        this.tools.set('track_problem', {
            name: 'track_problem',
            description: 'Track a development problem',
            inputSchema: {
                type: 'object',
                properties: {
                    description: { type: 'string', description: 'Problem description' },
                    context: { type: 'object', description: 'Problem context' }
                },
                required: ['description']
            }
        });

        this.tools.set('add_solution', {
            name: 'add_solution',
            description: 'Add solution to a tracked problem',
            inputSchema: {
                type: 'object',
                properties: {
                    problemId: { type: 'string', description: 'Problem ID' },
                    solution: { type: 'string', description: 'Solution description' },
                    successful: { type: 'boolean', description: 'Whether solution was successful' }
                },
                required: ['problemId', 'solution']
            }
        });

        this.tools.set('analyze_class_with_memory', {
            name: 'analyze_class_with_memory',
            description: 'Analyze a class with memory context',
            inputSchema: {
                type: 'object',
                properties: {
                    className: { type: 'string', description: 'Class name to analyze' }
                },
                required: ['className']
            }
        });

        this.tools.set('get_relevant_context', {
            name: 'get_relevant_context',
            description: 'Get relevant context for current task',
            inputSchema: {
                type: 'object',
                properties: {
                    query: { type: 'string', description: 'Context query' },
                    limit: { type: 'number', description: 'Max results', default: 5 }
                },
                required: ['query']
            }
        });

        this.tools.set('memory_stats', {
            name: 'memory_stats',
            description: 'Get memory system statistics',
            inputSchema: {
                type: 'object',
                properties: {}
            }
        });

        // Enhanced code analysis tools
        this.tools.set('analyze_character_system', {
            name: 'analyze_character_system',
            description: 'Analyze the character system with memory context',
            inputSchema: {
                type: 'object',
                properties: {
                    focus: { type: 'string', description: 'Specific aspect to focus on' }
                }
            }
        });

        this.tools.set('analyze_inventory_system', {
            name: 'analyze_inventory_system',
            description: 'Analyze the inventory system with memory context',
            inputSchema: {
                type: 'object',
                properties: {
                    includeUI: { type: 'boolean', description: 'Include UI analysis', default: true }
                }
            }
        });
    }

    initializeResources() {
        this.resources.set('project_memory', {
            uri: 'memory://project',
            name: 'Project Memory',
            description: 'Persistent project memory and insights',
            mimeType: 'application/json'
        });

        this.resources.set('session_history', {
            uri: 'memory://sessions',
            name: 'Session History',
            description: 'Development session history',
            mimeType: 'application/json'
        });

        this.resources.set('code_patterns', {
            uri: 'memory://patterns',
            name: 'Code Patterns',
            description: 'Discovered code patterns',
            mimeType: 'application/json'
        });
    }

    initializePrompts() {
        this.prompts.set('analyze_with_context', {
            name: 'analyze_with_context',
            description: 'Analyze code with historical context',
            arguments: [
                {
                    name: 'code_snippet',
                    description: 'Code to analyze',
                    required: true
                },
                {
                    name: 'context_query',
                    description: 'Query for relevant context',
                    required: false
                }
            ]
        });

        this.prompts.set('suggest_solution', {
            name: 'suggest_solution',
            description: 'Suggest solution based on similar past problems',
            arguments: [
                {
                    name: 'problem_description',
                    description: 'Current problem description',
                    required: true
                }
            ]
        });
    }

    async loadProjectKnowledge() {
        // Load key information about the AtlantisEons project
        const keyClasses = [
            'AAtlantisEonsCharacter',
            'AAtlantisEonsGameMode',
            'AAtlantisEonsHUD',
            'UAtlantisEonsGameInstance',
            'AZombieCharacter',
            'UDamageNumberSystem'
        ];

        for (const className of keyClasses) {
            const existing = memoryManager.getClassInfo(className);
            if (!existing) {
                await this.analyzeAndRememberClass(className);
            }
        }

        // Record initial insights
        memoryManager.addInsight(
            'project-structure',
            'AtlantisEons is an Unreal Engine 5.5 action RPG with character progression, inventory management, and combat systems',
            {
                engine: 'UE 5.5',
                genre: 'Action RPG',
                keyFeatures: ['character progression', 'inventory', 'combat', 'UI systems']
            }
        );

        memoryManager.addPattern(
            'unreal-widget-pattern',
            'Widgets prefixed with WBP_ follow Blueprint widget naming convention',
            ['WBP_Main', 'WBP_Inventory', 'WBP_CharacterInfo'],
            5
        );
    }

    async analyzeAndRememberClass(className) {
        try {
            // Simple class analysis - in a real implementation, this would be more sophisticated
            const sourceDir = path.join(projectRoot, 'Source');
            const files = await this.scanDirectory(sourceDir, ['.h', '.cpp']);
            
            for (const file of files) {
                if (file.includes(className)) {
                    const content = fs.readFileSync(file, 'utf8');
                    const classInfo = {
                        name: className,
                        file: file,
                        isHeader: file.endsWith('.h'),
                        isImplementation: file.endsWith('.cpp'),
                        hasBlueprint: content.includes('UCLASS'),
                        hasComponents: content.includes('UPROPERTY'),
                        hasFunctions: content.includes('UFUNCTION'),
                        lineCount: content.split('\n').length
                    };
                    
                    memoryManager.rememberClass(className, classInfo);
                    memoryManager.logAction('class_analyzed', { className, file });
                    break;
                }
            }
        } catch (error) {
            console.error(`Failed to analyze class ${className}:`, error.message);
        }
    }

    async scanDirectory(dir, extensions) {
        return new Promise((resolve, reject) => {
            const extPattern = extensions.map(ext => `*${ext}`).join('" -o -name "');
            exec(`find "${dir}" -type f \\( -name "${extPattern}" \\)`, (error, stdout, stderr) => {
                if (error) {
                    reject(error);
                    return;
                }
                resolve(stdout.trim().split('\n').filter(file => file));
            });
        });
    }

    async handleToolCall(name, args) {
        memoryManager.logAction('tool_call', { name, args });

        switch (name) {
            case 'save_context':
                const contextId = memoryManager.saveContextWindow(args.contextData);
                return {
                    success: true,
                    contextId,
                    message: 'Context saved successfully'
                };

            case 'search_memory':
                const results = memoryManager.search(args.query, args.categories);
                return {
                    success: true,
                    results,
                    totalFound: Object.values(results).reduce((sum, arr) => sum + arr.length, 0)
                };

            case 'add_insight':
                const insightId = memoryManager.addInsight(args.type, args.description, args.context);
                return {
                    success: true,
                    insightId,
                    message: 'Insight added successfully'
                };

            case 'add_pattern':
                const patternId = memoryManager.addPattern(args.name, args.description, args.examples);
                return {
                    success: true,
                    patternId,
                    message: 'Pattern recorded successfully'
                };

            case 'track_problem':
                const problemId = memoryManager.addProblem(args.description, args.context);
                return {
                    success: true,
                    problemId,
                    message: 'Problem tracked successfully'
                };

            case 'add_solution':
                memoryManager.addSolution(args.problemId, args.solution, args.successful);
                return {
                    success: true,
                    message: 'Solution added successfully'
                };

            case 'analyze_class_with_memory':
                const classInfo = memoryManager.getClassInfo(args.className);
                const relevantContext = memoryManager.getRelevantContext(args.className, 3);
                
                return {
                    success: true,
                    classInfo,
                    relevantContext,
                    recommendations: this.generateClassRecommendations(args.className, classInfo, relevantContext)
                };

            case 'get_relevant_context':
                const context = memoryManager.getRelevantContext(args.query, args.limit || 5);
                return {
                    success: true,
                    context,
                    count: context.length
                };

            case 'memory_stats':
                return {
                    success: true,
                    stats: memoryManager.getMemoryStats(),
                    sessionId: sessionId
                };

            case 'analyze_character_system':
                return await this.analyzeCharacterSystem(args.focus);

            case 'analyze_inventory_system':
                return await this.analyzeInventorySystem(args.includeUI);

            default:
                return {
                    success: false,
                    error: `Unknown tool: ${name}`
                };
        }
    }

    generateClassRecommendations(className, classInfo, relevantContext) {
        const recommendations = [];
        
        if (!classInfo) {
            recommendations.push(`Class ${className} not found in memory. Consider analyzing it first.`);
            return recommendations;
        }

        if (classInfo.lineCount > 2000) {
            recommendations.push('Consider breaking down this large class into smaller, more focused components.');
        }

        if (className.includes('Character') && !classInfo.hasComponents) {
            recommendations.push('Character classes typically benefit from component-based architecture.');
        }

        if (relevantContext.length > 0) {
            recommendations.push(`Found ${relevantContext.length} relevant context entries that might be helpful.`);
        }

        return recommendations;
    }

    async analyzeCharacterSystem(focus) {
        const characterClasses = ['AAtlantisEonsCharacter', 'AZombieCharacter'];
        const analysis = {
            classes: {},
            patterns: [],
            insights: [],
            recommendations: []
        };

        for (const className of characterClasses) {
            const classInfo = memoryManager.getClassInfo(className);
            const context = memoryManager.getRelevantContext(className, 2);
            
            analysis.classes[className] = {
                info: classInfo,
                context: context.length,
                patterns: memoryManager.getPatterns(0.3).filter(p => 
                    p.name.toLowerCase().includes('character') || 
                    p.description.toLowerCase().includes(className.toLowerCase())
                )
            };
        }

        // Add character system insights
        memoryManager.addInsight(
            'character-system',
            `Character system analysis completed focusing on: ${focus || 'general overview'}`,
            { focus, classesAnalyzed: characterClasses }
        );

        return {
            success: true,
            analysis,
            timestamp: new Date().toISOString()
        };
    }

    async analyzeInventorySystem(includeUI = true) {
        const inventoryClasses = [
            'UBP_ItemInfo',
            'ABP_Item',
            'UWBP_Inventory',
            'UWBP_InventorySlot'
        ];

        const analysis = {
            classes: {},
            patterns: [],
            insights: [],
            recommendations: []
        };

        for (const className of inventoryClasses) {
            const classInfo = memoryManager.getClassInfo(className);
            const context = memoryManager.getRelevantContext(className, 2);
            
            analysis.classes[className] = {
                info: classInfo,
                context: context.length
            };
        }

        // Record inventory system patterns
        memoryManager.addPattern(
            'inventory-item-pattern',
            'Items use BP_ItemInfo for data and BP_Item for world representation',
            ['UBP_ItemInfo', 'ABP_Item'],
            3
        );

        if (includeUI) {
            memoryManager.addInsight(
                'inventory-ui',
                'Inventory UI uses widget-based architecture with drag-and-drop support',
                { widgets: ['UWBP_Inventory', 'UWBP_InventorySlot'] }
            );
        }

        return {
            success: true,
            analysis,
            includeUI,
            timestamp: new Date().toISOString()
        };
    }

    async handleResourceRequest(uri) {
        switch (uri) {
            case 'memory://project':
                return {
                    contents: JSON.stringify(memoryManager.memory, null, 2),
                    mimeType: 'application/json'
                };

            case 'memory://sessions':
                return {
                    contents: JSON.stringify(memoryManager.memory.sessions, null, 2),
                    mimeType: 'application/json'
                };

            case 'memory://patterns':
                return {
                    contents: JSON.stringify(memoryManager.memory.patterns, null, 2),
                    mimeType: 'application/json'
                };

            default:
                throw new Error(`Unknown resource: ${uri}`);
        }
    }

    async handlePrompt(name, args) {
        switch (name) {
            case 'analyze_with_context':
                const context = args.context_query ? 
                    memoryManager.getRelevantContext(args.context_query, 3) : [];
                
                return {
                    prompt: `Analyze the following code with the provided context:

Code:
${args.code_snippet}

Relevant Context:
${context.map(c => `- ${c.summary}`).join('\n') || 'No relevant context found'}

Please provide analysis considering the historical context and patterns from this project.`
                };

            case 'suggest_solution':
                const similarProblems = memoryManager.getSimilarProblems(args.problem_description);
                
                return {
                    prompt: `Suggest a solution for the following problem:

Problem: ${args.problem_description}

Similar problems solved in the past:
${similarProblems.map(p => `- ${p.description} (${p.status})`).join('\n') || 'No similar problems found'}

Based on the project's history and patterns, what would you recommend?`
                };

            default:
                throw new Error(`Unknown prompt: ${name}`);
        }
    }
}

// Create HTTP server
function createServer() {
    const server = new MemoryMCPServer();
    
    const httpServer = http.createServer(async (req, res) => {
        res.setHeader('Content-Type', 'application/json');
        res.setHeader('Access-Control-Allow-Origin', '*');
        res.setHeader('Access-Control-Allow-Methods', 'POST, GET, OPTIONS');
        res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

        if (req.method === 'OPTIONS') {
            res.writeHead(200);
            res.end();
            return;
        }

        if (req.method !== 'POST') {
            res.writeHead(405);
            res.end(JSON.stringify({ error: 'Method not allowed' }));
            return;
        }

        let body = '';
        req.on('data', chunk => {
            body += chunk.toString();
        });

        req.on('end', async () => {
            try {
                const request = JSON.parse(body);
                let response;

                switch (request.method) {
                    case 'initialize':
                        response = {
                            protocolVersion: '2024-11-05',
                            capabilities: {
                                tools: { listChanged: true },
                                resources: { subscribe: true, listChanged: true },
                                prompts: { listChanged: true }
                            },
                            serverInfo: {
                                name: serverName,
                                version: serverVersion
                            }
                        };
                        break;

                    case 'tools/list':
                        response = {
                            tools: Array.from(server.tools.values())
                        };
                        break;

                    case 'tools/call':
                        const result = await server.handleToolCall(
                            request.params.name,
                            request.params.arguments || {}
                        );
                        response = {
                            content: [{
                                type: 'text',
                                text: JSON.stringify(result, null, 2)
                            }]
                        };
                        break;

                    case 'resources/list':
                        response = {
                            resources: Array.from(server.resources.values())
                        };
                        break;

                    case 'resources/read':
                        const resourceResult = await server.handleResourceRequest(request.params.uri);
                        response = {
                            contents: [{
                                uri: request.params.uri,
                                mimeType: resourceResult.mimeType,
                                text: resourceResult.contents
                            }]
                        };
                        break;

                    case 'prompts/list':
                        response = {
                            prompts: Array.from(server.prompts.values())
                        };
                        break;

                    case 'prompts/get':
                        const promptResult = await server.handlePrompt(
                            request.params.name,
                            request.params.arguments || {}
                        );
                        response = {
                            description: `Generated prompt for ${request.params.name}`,
                            messages: [{
                                role: 'user',
                                content: {
                                    type: 'text',
                                    text: promptResult.prompt
                                }
                            }]
                        };
                        break;

                    default:
                        throw new Error(`Unknown method: ${request.method}`);
                }

                res.writeHead(200);
                res.end(JSON.stringify(response));

            } catch (error) {
                console.error('Server error:', error);
                res.writeHead(500);
                res.end(JSON.stringify({
                    error: error.message,
                    code: 'INTERNAL_ERROR'
                }));
            }
        });
    });

    return httpServer;
}

// Start server
const server = createServer();

server.listen(serverPort, () => {
    console.log(`Memory-Enhanced MCP Server running on port ${serverPort}`);
    console.log(`Session ID: ${sessionId}`);
    console.log('Available at: http://localhost:' + serverPort);
    
    // Record server start
    memoryManager.addDevelopmentEvent(
        'server-start',
        'Memory-Enhanced MCP Server started',
        ['memory-mcp-server.js'],
        'low'
    );
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\nShutting down gracefully...');
    
    // End the session
    memoryManager.endSession('Server shutdown via SIGINT');
    memoryManager.addDevelopmentEvent(
        'server-stop',
        'Memory-Enhanced MCP Server stopped',
        ['memory-mcp-server.js'],
        'low'
    );
    
    server.close(() => {
        console.log('Server closed');
        process.exit(0);
    });
});

module.exports = { MemoryMCPServer }; 