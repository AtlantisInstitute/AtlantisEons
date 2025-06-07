#!/usr/bin/env node

/**
 * Memory-Enhanced MCP Server for AtlantisEons
 * Fully integrated memory management with code analysis
 * Auto-syncs with memory system for real-time project knowledge
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
const serverVersion = '2.0.0';

// Initialize memory manager with auto-sync capabilities
const memoryManager = new MemoryManager(projectRoot);

// Start a new session
const sessionId = memoryManager.startSession('MCP Memory-Enhanced Analysis Session with Auto-Sync');

// Memory file watchers for real-time updates
const memoryWatchers = new Map();

// MCP Server Implementation with Enhanced Memory Integration
class MemoryMCPServer {
    constructor() {
        this.tools = new Map();
        this.resources = new Map();
        this.prompts = new Map();
        this.memoryCache = new Map();
        this.lastSyncTime = Date.now();
        
        this.initializeTools();
        this.initializeResources();
        this.initializePrompts();
        
        // Load existing project knowledge and set up watchers
        this.loadProjectKnowledge();
        this.setupMemoryWatchers();
        this.scheduleMemorySync();
    }

    initializeTools() {
        // Enhanced Memory Management Tools
        this.tools.set('save_context', {
            name: 'save_context',
            description: 'Save current context for future reference with auto-MCP integration',
            inputSchema: {
                type: 'object',
                properties: {
                    contextData: { type: 'object', description: 'Context data to save' },
                    description: { type: 'string', description: 'Description of the context' },
                    tags: { type: 'array', items: { type: 'string' }, description: 'Context tags' },
                    category: { type: 'string', description: 'Context category', enum: ['development', 'debugging', 'architecture', 'implementation', 'analysis'] }
                },
                required: ['contextData']
            }
        });

        this.tools.set('search_memory', {
            name: 'search_memory',
            description: 'Search through saved memories with enhanced filtering',
            inputSchema: {
                type: 'object',
                properties: {
                    query: { type: 'string', description: 'Search query' },
                    categories: { 
                        type: 'array', 
                        items: { type: 'string' },
                        description: 'Categories to search: insights, patterns, problems, history, contexts, architecture'
                    },
                    timeRange: { type: 'string', description: 'Time range: today, week, month, all', default: 'all' },
                    relevanceThreshold: { type: 'number', description: 'Minimum relevance score', default: 0.3 }
                },
                required: ['query']
            }
        });

        this.tools.set('create_memory_snapshot', {
            name: 'create_memory_snapshot',
            description: 'Create a comprehensive memory snapshot of current project state',
            inputSchema: {
                type: 'object',
                properties: {
                    includeCode: { type: 'boolean', description: 'Include code analysis', default: true },
                    includeArchitecture: { type: 'boolean', description: 'Include architecture mapping', default: true },
                    description: { type: 'string', description: 'Snapshot description' }
                }
            }
        });

        this.tools.set('sync_project_knowledge', {
            name: 'sync_project_knowledge',
            description: 'Manually sync project knowledge with memory system',
            inputSchema: {
                type: 'object',
                properties: {
                    force: { type: 'boolean', description: 'Force full resync', default: false }
                }
            }
        });

        this.tools.set('add_insight', {
            name: 'add_insight',
            description: 'Add a development insight with MCP integration',
            inputSchema: {
                type: 'object',
                properties: {
                    type: { type: 'string', description: 'Type of insight' },
                    description: { type: 'string', description: 'Insight description' },
                    context: { type: 'object', description: 'Additional context' },
                    impact: { type: 'string', description: 'Impact level', enum: ['low', 'medium', 'high', 'critical'], default: 'medium' },
                    implementationStatus: { type: 'string', description: 'Implementation status', enum: ['idea', 'planned', 'in-progress', 'completed'], default: 'idea' }
                },
                required: ['type', 'description']
            }
        });

        this.tools.set('add_pattern', {
            name: 'add_pattern',
            description: 'Record a code pattern with enhanced metadata',
            inputSchema: {
                type: 'object',
                properties: {
                    name: { type: 'string', description: 'Pattern name' },
                    description: { type: 'string', description: 'Pattern description' },
                    examples: { type: 'array', items: { type: 'string' }, description: 'Code examples' },
                    category: { type: 'string', description: 'Pattern category', enum: ['architectural', 'behavioral', 'creational', 'ui', 'gameplay', 'optimization'] },
                    complexity: { type: 'string', description: 'Complexity level', enum: ['simple', 'moderate', 'complex'], default: 'moderate' },
                    usageCount: { type: 'number', description: 'Usage frequency', default: 1 }
                },
                required: ['name', 'description']
            }
        });

        this.tools.set('track_problem', {
            name: 'track_problem',
            description: 'Track a development problem with enhanced tracking',
            inputSchema: {
                type: 'object',
                properties: {
                    description: { type: 'string', description: 'Problem description' },
                    context: { type: 'object', description: 'Problem context' },
                    severity: { type: 'string', description: 'Problem severity', enum: ['low', 'medium', 'high', 'critical'], default: 'medium' },
                    category: { type: 'string', description: 'Problem category', enum: ['bug', 'design', 'performance', 'architecture', 'ui', 'gameplay'] },
                    affectedSystems: { type: 'array', items: { type: 'string' }, description: 'Affected systems/classes' }
                },
                required: ['description']
            }
        });

        this.tools.set('add_solution', {
            name: 'add_solution',
            description: 'Add solution to a tracked problem with outcome tracking',
            inputSchema: {
                type: 'object',
                properties: {
                    problemId: { type: 'string', description: 'Problem ID' },
                    solution: { type: 'string', description: 'Solution description' },
                    successful: { type: 'boolean', description: 'Whether solution was successful' },
                    implementationTime: { type: 'number', description: 'Time to implement (hours)' },
                    sideEffects: { type: 'array', items: { type: 'string' }, description: 'Any side effects or new issues' }
                },
                required: ['problemId', 'solution']
            }
        });

        // Enhanced Analysis Tools with Memory Integration
        this.tools.set('analyze_class_with_memory', {
            name: 'analyze_class_with_memory',
            description: 'Analyze a class with comprehensive memory context',
            inputSchema: {
                type: 'object',
                properties: {
                    className: { type: 'string', description: 'Class name to analyze' },
                    analysisDepth: { type: 'string', description: 'Analysis depth', enum: ['basic', 'detailed', 'comprehensive'], default: 'detailed' },
                    includeRelated: { type: 'boolean', description: 'Include related classes analysis', default: true }
                },
                required: ['className']
            }
        });

        this.tools.set('get_relevant_context', {
            name: 'get_relevant_context',
            description: 'Get relevant context for current task with smart filtering',
            inputSchema: {
                type: 'object',
                properties: {
                    query: { type: 'string', description: 'Context query' },
                    limit: { type: 'number', description: 'Max results', default: 10 },
                    contextTypes: { type: 'array', items: { type: 'string' }, description: 'Types of context to include' },
                    recency: { type: 'boolean', description: 'Prioritize recent context', default: true }
                },
                required: ['query']
            }
        });

        this.tools.set('memory_stats', {
            name: 'memory_stats',
            description: 'Get comprehensive memory system statistics',
            inputSchema: {
                type: 'object',
                properties: {
                    includeDetails: { type: 'boolean', description: 'Include detailed breakdown', default: true }
                }
            }
        });

        // Project Knowledge Tools
        this.tools.set('get_project_overview', {
            name: 'get_project_overview',
            description: 'Get comprehensive project overview from memory',
            inputSchema: {
                type: 'object',
                properties: {
                    includeArchitecture: { type: 'boolean', description: 'Include architecture overview', default: true },
                    includeRecentChanges: { type: 'boolean', description: 'Include recent changes', default: true },
                    includeIssues: { type: 'boolean', description: 'Include current issues', default: true }
                }
            }
        });

        this.tools.set('analyze_system_dependencies', {
            name: 'analyze_system_dependencies',
            description: 'Analyze system dependencies using memory knowledge',
            inputSchema: {
                type: 'object',
                properties: {
                    systemName: { type: 'string', description: 'System or class name' },
                    depth: { type: 'number', description: 'Dependency depth', default: 3 }
                },
                required: ['systemName']
            }
        });

        // Real-time Integration Tools
        this.tools.set('watch_memory_changes', {
            name: 'watch_memory_changes',
            description: 'Set up real-time memory change monitoring',
            inputSchema: {
                type: 'object',
                properties: {
                    categories: { type: 'array', items: { type: 'string' }, description: 'Categories to watch' },
                    callback: { type: 'string', description: 'Callback endpoint for notifications' }
                }
            }
        });

        // Legacy tools for compatibility
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
            description: 'Persistent project memory and insights with real-time updates',
            mimeType: 'application/json'
        });

        this.resources.set('session_history', {
            uri: 'memory://sessions',
            name: 'Session History',
            description: 'Development session history with detailed tracking',
            mimeType: 'application/json'
        });

        this.resources.set('project_knowledge_graph', {
            uri: 'memory://knowledge-graph',
            name: 'Project Knowledge Graph',
            description: 'Dynamic project knowledge graph from memory system',
            mimeType: 'application/json'
        });

        this.resources.set('architecture_overview', {
            uri: 'memory://architecture',
            name: 'Architecture Overview',
            description: 'Current project architecture from memory analysis',
            mimeType: 'application/json'
        });

        this.resources.set('recent_insights', {
            uri: 'memory://insights/recent',
            name: 'Recent Insights',
            description: 'Most recent development insights and patterns',
            mimeType: 'application/json'
        });

        this.resources.set('problem_tracking', {
            uri: 'memory://problems',
            name: 'Problem Tracking',
            description: 'Current and resolved problems with solutions',
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

        // Sync memory cache
        await this.syncMemoryCache();
    }

    // Enhanced Memory Integration Methods
    setupMemoryWatchers() {
        const memoryDir = path.join(projectRoot, 'logs-and-data');
        const memoryFiles = [
            'project-memory.json',
            'sessions.json',
            'insights.json', 
            'patterns.json',
            'context-windows.json'
        ];

        memoryFiles.forEach(filename => {
            const filepath = path.join(memoryDir, filename);
            if (fs.existsSync(filepath)) {
                const watcher = fs.watchFile(filepath, { interval: 1000 }, () => {
                    console.log(`Memory file changed: ${filename}`);
                    this.syncMemoryCache();
                });
                memoryWatchers.set(filename, watcher);
            }
        });
    }

    scheduleMemorySync() {
        // Auto-sync every 30 seconds
        setInterval(() => {
            this.syncMemoryCache();
        }, 30000);
    }

    async syncMemoryCache() {
        try {
            const memoryData = memoryManager.memory;
            const stats = memoryManager.getMemoryStats();
            
            this.memoryCache.set('full_memory', memoryData);
            this.memoryCache.set('stats', stats);
            this.memoryCache.set('last_sync', Date.now());
            
            // Update project knowledge graph
            const knowledgeGraph = this.buildKnowledgeGraph(memoryData);
            this.memoryCache.set('knowledge_graph', knowledgeGraph);
            
            console.log(`Memory cache synced - ${stats.totalItems} items`);
        } catch (error) {
            console.error('Error syncing memory cache:', error);
        }
    }

    buildKnowledgeGraph(memoryData) {
        const graph = {
            nodes: [],
            edges: [],
            metadata: {
                generated: new Date().toISOString(),
                totalNodes: 0,
                totalEdges: 0
            }
        };

        // Add class nodes
        Object.keys(memoryData.keyClasses || {}).forEach(className => {
            graph.nodes.push({
                id: className,
                type: 'class',
                label: className,
                data: memoryData.keyClasses[className]
            });
        });

        // Add insight nodes
        (memoryData.insights || []).forEach((insight, index) => {
            const nodeId = `insight_${index}`;
            graph.nodes.push({
                id: nodeId,
                type: 'insight',
                label: insight.type,
                data: insight
            });
        });

        // Add pattern nodes
        (memoryData.patterns || []).forEach((pattern, index) => {
            const nodeId = `pattern_${index}`;
            graph.nodes.push({
                id: nodeId,
                type: 'pattern',
                label: pattern.name,
                data: pattern
            });
        });

        graph.metadata.totalNodes = graph.nodes.length;
        graph.metadata.totalEdges = graph.edges.length;

        return graph;
    }

    async createMemorySnapshot(options = {}) {
        const snapshot = {
            timestamp: new Date().toISOString(),
            sessionId: sessionId,
            options: options,
            data: {}
        };

        if (options.includeCode !== false) {
            snapshot.data.codeAnalysis = await this.generateCodeAnalysisSummary();
        }

        if (options.includeArchitecture !== false) {
            snapshot.data.architecture = this.memoryCache.get('knowledge_graph') || {};
        }

        snapshot.data.memory = memoryManager.memory;
        snapshot.data.stats = memoryManager.getMemoryStats();

        // Save snapshot
        const snapshotId = memoryManager.saveContextWindow({
            type: 'memory_snapshot',
            description: options.description || 'Automated memory snapshot',
            snapshot: snapshot
        });

        return {
            success: true,
            snapshotId: snapshotId,
            summary: `Created memory snapshot with ${snapshot.data.stats.totalItems} items`
        };
    }

    async generateCodeAnalysisSummary() {
        const sourceDir = path.join(projectRoot, 'Source');
        const files = await this.scanDirectory(sourceDir, ['.h', '.cpp']);
        
        return {
            totalFiles: files.length,
            lastAnalyzed: new Date().toISOString(),
            keyClasses: Object.keys(memoryManager.memory.keyClasses || {}),
            recentChanges: memoryManager.getDevelopmentHistory('code_change', 10)
        };
    }

    // Enhanced Helper Methods for New Functionality
    filterResultsByTimeRange(results, timeRange) {
        if (!timeRange || timeRange === 'all') return results;
        
        const now = new Date();
        let cutoffDate;
        
        switch (timeRange) {
            case 'today':
                cutoffDate = new Date(now.getFullYear(), now.getMonth(), now.getDate());
                break;
            case 'week':
                cutoffDate = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000);
                break;
            case 'month':
                cutoffDate = new Date(now.getTime() - 30 * 24 * 60 * 60 * 1000);
                break;
            default:
                return results;
        }

        const filteredResults = {};
        Object.keys(results).forEach(category => {
            filteredResults[category] = results[category].filter(item => {
                const itemDate = new Date(item.timestamp || item.created);
                return itemDate >= cutoffDate;
            });
        });

        return filteredResults;
    }

    async getProjectOverview(options = {}) {
        const overview = {
            projectName: 'AtlantisEons',
            engineVersion: '5.5',
            timestamp: new Date().toISOString(),
            summary: {}
        };

        if (options.includeArchitecture !== false) {
            overview.architecture = {
                keyClasses: Object.keys(memoryManager.memory.keyClasses || {}),
                knowledgeGraph: this.memoryCache.get('knowledge_graph'),
                coreSystemsMap: memoryManager.memory.architecture
            };
        }

        if (options.includeRecentChanges !== false) {
            overview.recentChanges = {
                sessions: memoryManager.memory.sessions.slice(-5),
                insights: memoryManager.getInsights(null, 0.3).slice(-10),
                patterns: memoryManager.getPatterns(0.5).slice(-5)
            };
        }

        if (options.includeIssues !== false) {
            const problems = memoryManager.memory.problemsSolved || [];
            overview.issues = {
                openProblems: problems.filter(p => !p.resolved),
                recentlySolved: problems.filter(p => p.resolved).slice(-5),
                totalTracked: problems.length
            };
        }

        overview.summary = {
            totalSessions: memoryManager.memory.sessions.length,
            totalInsights: memoryManager.memory.insights.length,
            totalPatterns: memoryManager.memory.patterns.length,
            memoryHealth: this.assessMemoryHealth()
        };

        return overview;
    }

    async analyzeSystemDependencies(systemName, depth = 3) {
        const dependencies = {
            system: systemName,
            depth: depth,
            directDependencies: [],
            indirectDependencies: [],
            dependents: [],
            riskAssessment: {}
        };

        // Find direct dependencies by analyzing class info
        const classInfo = memoryManager.getClassInfo(systemName);
        if (classInfo) {
            dependencies.directDependencies = this.extractDependenciesFromClass(classInfo);
        }

        // Find systems that depend on this one
        const allClasses = memoryManager.memory.keyClasses || {};
        Object.keys(allClasses).forEach(className => {
            const classData = allClasses[className];
            if (classData && classData.dependencies && classData.dependencies.includes(systemName)) {
                dependencies.dependents.push(className);
            }
        });

        // Risk assessment
        dependencies.riskAssessment = {
            cyclicDependencies: this.detectCyclicDependencies(systemName, dependencies.directDependencies),
            highCoupling: dependencies.directDependencies.length > 10,
            criticalPath: dependencies.dependents.length > 5
        };

        return dependencies;
    }

    extractDependenciesFromClass(classInfo) {
        const dependencies = [];
        
        // Extract from includes, inheritance, etc.
        if (classInfo.content) {
            const includeMatches = classInfo.content.match(/#include\s+"([^"]+)"/g);
            if (includeMatches) {
                includeMatches.forEach(match => {
                    const header = match.match(/#include\s+"([^"]+)"/)[1];
                    if (!header.startsWith('Engine/') && !header.startsWith('CoreMinimal')) {
                        dependencies.push(header.replace('.h', ''));
                    }
                });
            }
        }

        return dependencies;
    }

    detectCyclicDependencies(systemName, dependencies) {
        // Simple cycle detection - in production would be more sophisticated
        const visited = new Set();
        const recursionStack = new Set();
        
        const hasCycle = (node) => {
            visited.add(node);
            recursionStack.add(node);
            
            const classDeps = this.extractDependenciesFromClass(memoryManager.getClassInfo(node) || {});
            for (const dep of classDeps) {
                if (!visited.has(dep) && hasCycle(dep)) {
                    return true;
                }
                if (recursionStack.has(dep)) {
                    return true;
                }
            }
            
            recursionStack.delete(node);
            return false;
        };
        
        return hasCycle(systemName);
    }

    setupMemoryChangeWatcher(categories, callback) {
        if (!categories || categories.length === 0) {
            categories = ['insights', 'patterns', 'problems', 'contexts'];
        }

        // Store watcher configuration
        this.memoryChangeWatchers = this.memoryChangeWatchers || new Map();
        const watcherId = `watcher_${Date.now()}`;
        
        this.memoryChangeWatchers.set(watcherId, {
            categories: categories,
            callback: callback,
            lastCheck: Date.now()
        });

        return {
            watcherId: watcherId,
            message: `Memory change watcher set up for categories: ${categories.join(', ')}`
        };
    }

    assessMemoryHealth() {
        const stats = memoryManager.getMemoryStats();
        const health = {
            score: 100,
            issues: [],
            recommendations: []
        };

        // Check for health indicators
        if (stats.totalItems > 10000) {
            health.score -= 20;
            health.issues.push('Large memory size may impact performance');
            health.recommendations.push('Consider running memory cleanup');
        }

        if (this.memoryCache.size === 0) {
            health.score -= 30;
            health.issues.push('Memory cache is empty');
            health.recommendations.push('Run memory sync to populate cache');
        }

        const lastSync = this.memoryCache.get('last_sync');
        if (!lastSync || Date.now() - lastSync > 300000) { // 5 minutes
            health.score -= 15;
            health.issues.push('Memory cache is stale');
            health.recommendations.push('Sync memory cache');
        }

        if (health.score >= 80) health.status = 'excellent';
        else if (health.score >= 60) health.status = 'good';
        else if (health.score >= 40) health.status = 'fair';
        else health.status = 'poor';

        return health;
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
                const contextId = memoryManager.saveContextWindow({
                    ...args.contextData,
                    description: args.description,
                    tags: args.tags,
                    category: args.category
                });
                await this.syncMemoryCache(); // Auto-sync after context save
                return {
                    success: true,
                    contextId,
                    message: 'Context saved successfully and synced to MCP server'
                };

            case 'search_memory':
                const results = memoryManager.search(args.query, args.categories);
                const filteredResults = this.filterResultsByTimeRange(results, args.timeRange);
                return {
                    success: true,
                    results: filteredResults,
                    totalFound: Object.values(filteredResults).reduce((sum, arr) => sum + arr.length, 0),
                    filters: { timeRange: args.timeRange, relevanceThreshold: args.relevanceThreshold }
                };

            case 'create_memory_snapshot':
                const snapshot = await this.createMemorySnapshot(args);
                return snapshot;

            case 'sync_project_knowledge':
                if (args.force) {
                    await this.loadProjectKnowledge();
                }
                await this.syncMemoryCache();
                const stats = this.memoryCache.get('stats');
                return {
                    success: true,
                    message: `Project knowledge synced. Current memory contains ${stats?.totalItems || 0} items.`,
                    stats: stats
                };

            case 'add_insight':
                const insight = {
                    type: args.type,
                    description: args.description,
                    context: args.context,
                    impact: args.impact,
                    implementationStatus: args.implementationStatus
                };
                const insightId = memoryManager.addInsight(insight.type, insight.description, insight);
                await this.syncMemoryCache(); // Auto-sync after insight
                return {
                    success: true,
                    insightId,
                    message: `Enhanced insight added (Impact: ${args.impact}, Status: ${args.implementationStatus})`
                };

            case 'add_pattern':
                const pattern = {
                    name: args.name,
                    description: args.description,
                    examples: args.examples,
                    category: args.category,
                    complexity: args.complexity,
                    usageCount: args.usageCount
                };
                const patternId = memoryManager.addPattern(pattern.name, pattern.description, pattern.examples, pattern.usageCount);
                await this.syncMemoryCache(); // Auto-sync after pattern
                return {
                    success: true,
                    patternId,
                    message: `Enhanced pattern recorded (Category: ${args.category}, Complexity: ${args.complexity})`
                };

            case 'track_problem':
                const problem = {
                    description: args.description,
                    context: args.context,
                    severity: args.severity,
                    category: args.category,
                    affectedSystems: args.affectedSystems
                };
                const problemId = memoryManager.addProblem(problem.description, problem);
                await this.syncMemoryCache(); // Auto-sync after problem
                return {
                    success: true,
                    problemId,
                    message: `Enhanced problem tracked (Severity: ${args.severity}, Category: ${args.category})`
                };

            case 'add_solution':
                const solution = {
                    problemId: args.problemId,
                    solution: args.solution,
                    successful: args.successful,
                    implementationTime: args.implementationTime,
                    sideEffects: args.sideEffects
                };
                memoryManager.addSolution(solution.problemId, solution.solution, solution.successful);
                await this.syncMemoryCache(); // Auto-sync after solution
                return {
                    success: true,
                    message: `Enhanced solution added (Time: ${args.implementationTime}h)`
                };

            case 'analyze_class_with_memory':
                const classInfo = memoryManager.getClassInfo(args.className);
                const relevantContext = memoryManager.getRelevantContext(args.className, 3);
                
                return {
                    success: true,
                    classInfo,
                    relevantContext,
                    recommendations: this.generateClassRecommendations(args.className, classInfo, relevantContext),
                    analysisDepth: args.analysisDepth,
                    includeRelated: args.includeRelated
                };

            case 'get_relevant_context':
                let context = memoryManager.getRelevantContext(args.query, args.limit || 10);
                if (args.contextTypes) {
                    context = context.filter(c => args.contextTypes.includes(c.type));
                }
                if (args.recency) {
                    context.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));
                }
                return {
                    success: true,
                    context,
                    count: context.length,
                    filters: { contextTypes: args.contextTypes, recency: args.recency }
                };

            case 'memory_stats':
                const memStats = memoryManager.getMemoryStats();
                const cacheStats = {
                    lastSync: new Date(this.memoryCache.get('last_sync')).toISOString(),
                    cachedItems: this.memoryCache.size,
                    knowledgeGraphNodes: this.memoryCache.get('knowledge_graph')?.metadata?.totalNodes || 0
                };
                
                return {
                    success: true,
                    stats: args.includeDetails ? { memory: memStats, cache: cacheStats } : { ...memStats, ...cacheStats },
                    sessionId: sessionId
                };

            case 'get_project_overview':
                const overview = await this.getProjectOverview(args);
                return {
                    success: true,
                    overview
                };

            case 'analyze_system_dependencies':
                const dependencies = await this.analyzeSystemDependencies(args.systemName, args.depth);
                return {
                    success: true,
                    dependencies
                };

            case 'watch_memory_changes':
                const watchResult = this.setupMemoryChangeWatcher(args.categories, args.callback);
                return {
                    success: true,
                    message: watchResult.message,
                    categories: args.categories
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
        try {
            switch (uri) {
                case 'memory://project':
                    return {
                        contents: JSON.stringify(this.memoryCache.get('full_memory') || memoryManager.memory, null, 2),
                        mimeType: 'application/json'
                    };

                case 'memory://sessions':
                    return {
                        contents: JSON.stringify(memoryManager.memory.sessions, null, 2),
                        mimeType: 'application/json'
                    };

                case 'memory://knowledge-graph':
                    return {
                        contents: JSON.stringify(this.memoryCache.get('knowledge_graph') || {}, null, 2),
                        mimeType: 'application/json'
                    };

                case 'memory://architecture':
                    const archOverview = await this.getProjectOverview({ 
                        includeArchitecture: true, 
                        includeRecentChanges: false, 
                        includeIssues: false 
                    });
                    return {
                        contents: JSON.stringify(archOverview.architecture, null, 2),
                        mimeType: 'application/json'
                    };

                case 'memory://insights/recent':
                    const recentInsights = memoryManager.getInsights(null, 0.3).slice(-20);
                    return {
                        contents: JSON.stringify(recentInsights, null, 2),
                        mimeType: 'application/json'
                    };

                case 'memory://problems':
                    const problems = memoryManager.memory.problemsSolved || [];
                    const problemTracking = {
                        openProblems: problems.filter(p => !p.resolved),
                        resolvedProblems: problems.filter(p => p.resolved),
                        totalTracked: problems.length,
                        lastUpdated: new Date().toISOString()
                    };
                    return {
                        contents: JSON.stringify(problemTracking, null, 2),
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
        } catch (error) {
            console.error(`Error handling resource request for ${uri}:`, error);
            throw error;
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