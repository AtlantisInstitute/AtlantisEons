#!/usr/bin/env node

/**
 * Memory Manager for AtlantisEons Project
 * Persistent memory storage system for context windows
 * Stores project insights, patterns, and development history
 */

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

class MemoryManager {
    constructor(projectRoot = process.cwd()) {
        this.projectRoot = projectRoot;
        this.memoryDir = path.join(projectRoot, 'logs-and-data');
        this.memoryFile = path.join(this.memoryDir, 'project-memory.json');
        this.sessionsFile = path.join(this.memoryDir, 'sessions.json');
        this.insightsFile = path.join(this.memoryDir, 'insights.json');
        this.patternsFile = path.join(this.memoryDir, 'patterns.json');
        this.contextFile = path.join(this.memoryDir, 'context-windows.json');
        
        this.memory = this.loadMemory();
        this.ensureMemoryDir();
    }

    ensureMemoryDir() {
        if (!fs.existsSync(this.memoryDir)) {
            fs.mkdirSync(this.memoryDir, { recursive: true });
        }
    }

    loadMemory() {
        try {
            if (fs.existsSync(this.memoryFile)) {
                return JSON.parse(fs.readFileSync(this.memoryFile, 'utf8'));
            }
        } catch (error) {
            console.warn('Could not load memory file:', error.message);
        }
        
        return {
            projectName: 'AtlantisEons',
            created: new Date().toISOString(),
            lastUpdated: new Date().toISOString(),
            version: '1.0.0',
            sessions: [],
            insights: [],
            patterns: [],
            contextWindows: [],
            keyClasses: {},
            importantFiles: {},
            developmentHistory: [],
            codePatterns: {},
            problemsSolved: [],
            bestPractices: [],
            technicalDebt: [],
            architecture: {
                mainCharacter: 'AAtlantisEonsCharacter',
                gameMode: 'AAtlantisEonsGameMode',
                gameInstance: 'UAtlantisEonsGameInstance',
                hud: 'AAtlantisEonsHUD',
                keySystemClasses: [
                    'AAtlantisEonsCharacter',
                    'AAtlantisEonsGameMode', 
                    'AAtlantisEonsHUD',
                    'UAtlantisEonsGameInstance',
                    'AZombieCharacter',
                    'UDamageNumberSystem'
                ]
            }
        };
    }

    saveMemory() {
        try {
            this.memory.lastUpdated = new Date().toISOString();
            fs.writeFileSync(this.memoryFile, JSON.stringify(this.memory, null, 2));
            return true;
        } catch (error) {
            console.error('Failed to save memory:', error.message);
            return false;
        }
    }

    // Session Management
    startSession(description = '') {
        const sessionId = crypto.randomUUID();
        const session = {
            id: sessionId,
            started: new Date().toISOString(),
            description,
            actions: [],
            filesModified: [],
            insights: [],
            problems: [],
            solutions: []
        };
        
        this.memory.sessions.push(session);
        this.currentSession = session;
        this.saveMemory();
        
        console.log(`Started new session: ${sessionId}`);
        return sessionId;
    }

    endSession(summary = '') {
        if (this.currentSession) {
            this.currentSession.ended = new Date().toISOString();
            this.currentSession.summary = summary;
            this.currentSession = null;
            this.saveMemory();
        }
    }

    logAction(action, details = {}) {
        if (this.currentSession) {
            this.currentSession.actions.push({
                timestamp: new Date().toISOString(),
                action,
                details
            });
            this.saveMemory();
        }
    }

    // Context Window Management
    saveContextWindow(contextData) {
        const contextId = crypto.randomUUID();
        const contextWindow = {
            id: contextId,
            timestamp: new Date().toISOString(),
            sessionId: this.currentSession?.id,
            data: contextData,
            summary: this.generateContextSummary(contextData),
            tags: this.extractTags(contextData)
        };
        
        this.memory.contextWindows.push(contextWindow);
        
        // Keep only last 50 context windows to prevent file bloat
        if (this.memory.contextWindows.length > 50) {
            this.memory.contextWindows = this.memory.contextWindows.slice(-50);
        }
        
        this.saveMemory();
        return contextId;
    }

    getRelevantContext(query, limit = 5) {
        return this.memory.contextWindows
            .filter(ctx => this.isContextRelevant(ctx, query))
            .sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp))
            .slice(0, limit);
    }

    isContextRelevant(context, query) {
        const queryLower = query.toLowerCase();
        return context.summary.toLowerCase().includes(queryLower) ||
               context.tags.some(tag => tag.toLowerCase().includes(queryLower)) ||
               JSON.stringify(context.data).toLowerCase().includes(queryLower);
    }

    generateContextSummary(contextData) {
        // Generate a meaningful summary of the context data
        const keys = Object.keys(contextData);
        const summary = keys.slice(0, 3).join(', ');
        return `Context involving: ${summary}`;
    }

    extractTags(contextData) {
        const tags = [];
        
        // Extract class names, function names, etc.
        const text = JSON.stringify(contextData);
        const classMatches = text.match(/class\s+([A-Z][A-Za-z0-9_]*)/g);
        const functionMatches = text.match(/\b([A-Za-z_][A-Za-z0-9_]*)\s*\(/g);
        
        if (classMatches) {
            classMatches.forEach(match => {
                const className = match.replace('class ', '');
                if (!tags.includes(className)) tags.push(className);
            });
        }
        
        if (functionMatches) {
            functionMatches.forEach(match => {
                const funcName = match.replace(/\s*\($/, '');
                if (!tags.includes(funcName) && funcName.length > 3) {
                    tags.push(funcName);
                }
            });
        }
        
        return tags.slice(0, 10); // Limit to 10 tags
    }

    // Insights Management
    addInsight(type, description, context = {}) {
        const insight = {
            id: crypto.randomUUID(),
            timestamp: new Date().toISOString(),
            sessionId: this.currentSession?.id,
            type,
            description,
            context,
            relevance: this.calculateRelevance(description, context)
        };
        
        this.memory.insights.push(insight);
        this.saveMemory();
        
        if (this.currentSession) {
            this.currentSession.insights.push(insight.id);
        }
        
        return insight.id;
    }

    calculateRelevance(description, context) {
        // Simple relevance calculation based on keywords
        const keywords = ['character', 'inventory', 'damage', 'ui', 'input', 'memory', 'performance'];
        const text = (description + JSON.stringify(context)).toLowerCase();
        
        let score = 0;
        keywords.forEach(keyword => {
            if (text.includes(keyword)) score++;
        });
        
        return Math.min(score / keywords.length, 1.0);
    }

    getInsights(type = null, minRelevance = 0.3) {
        return this.memory.insights
            .filter(insight => 
                (!type || insight.type === type) && 
                insight.relevance >= minRelevance
            )
            .sort((a, b) => b.relevance - a.relevance);
    }

    // Pattern Recognition
    addPattern(name, description, examples = [], frequency = 1) {
        const pattern = {
            id: crypto.randomUUID(),
            name,
            description,
            examples,
            frequency,
            firstSeen: new Date().toISOString(),
            lastSeen: new Date().toISOString(),
            confidence: Math.min(frequency / 10, 1.0)
        };
        
        this.memory.patterns.push(pattern);
        this.saveMemory();
        return pattern.id;
    }

    updatePatternFrequency(patternName) {
        const pattern = this.memory.patterns.find(p => p.name === patternName);
        if (pattern) {
            pattern.frequency++;
            pattern.lastSeen = new Date().toISOString();
            pattern.confidence = Math.min(pattern.frequency / 10, 1.0);
            this.saveMemory();
        }
    }

    getPatterns(minConfidence = 0.5) {
        return this.memory.patterns
            .filter(pattern => pattern.confidence >= minConfidence)
            .sort((a, b) => b.confidence - a.confidence);
    }

    // File and Class Memory
    rememberClass(className, info) {
        this.memory.keyClasses[className] = {
            ...info,
            lastUpdated: new Date().toISOString()
        };
        this.saveMemory();
    }

    getClassInfo(className) {
        return this.memory.keyClasses[className] || null;
    }

    rememberFile(filePath, info) {
        this.memory.importantFiles[filePath] = {
            ...info,
            lastUpdated: new Date().toISOString()
        };
        this.saveMemory();
    }

    getFileInfo(filePath) {
        return this.memory.importantFiles[filePath] || null;
    }

    // Problem and Solution Tracking
    addProblem(description, context = {}) {
        const problem = {
            id: crypto.randomUUID(),
            timestamp: new Date().toISOString(),
            sessionId: this.currentSession?.id,
            description,
            context,
            status: 'open',
            attempts: []
        };
        
        this.memory.problemsSolved.push(problem);
        this.saveMemory();
        
        if (this.currentSession) {
            this.currentSession.problems.push(problem.id);
        }
        
        return problem.id;
    }

    addSolution(problemId, solution, successful = true) {
        const problem = this.memory.problemsSolved.find(p => p.id === problemId);
        if (problem) {
            problem.attempts.push({
                timestamp: new Date().toISOString(),
                solution,
                successful
            });
            
            if (successful) {
                problem.status = 'solved';
                problem.solution = solution;
            }
            
            this.saveMemory();
            
            if (this.currentSession) {
                this.currentSession.solutions.push({ problemId, solution, successful });
            }
        }
    }

    getSimilarProblems(description) {
        return this.memory.problemsSolved
            .filter(problem => 
                this.calculateSimilarity(problem.description, description) > 0.5
            )
            .sort((a, b) => 
                this.calculateSimilarity(b.description, description) - 
                this.calculateSimilarity(a.description, description)
            );
    }

    calculateSimilarity(text1, text2) {
        // Simple word-based similarity
        const words1 = text1.toLowerCase().split(/\s+/);
        const words2 = text2.toLowerCase().split(/\s+/);
        
        const commonWords = words1.filter(word => words2.includes(word));
        return commonWords.length / Math.max(words1.length, words2.length);
    }

    // Development History
    addDevelopmentEvent(type, description, files = [], impact = 'medium') {
        const event = {
            id: crypto.randomUUID(),
            timestamp: new Date().toISOString(),
            sessionId: this.currentSession?.id,
            type,
            description,
            files,
            impact
        };
        
        this.memory.developmentHistory.push(event);
        this.saveMemory();
        return event.id;
    }

    getDevelopmentHistory(type = null, limit = 20) {
        return this.memory.developmentHistory
            .filter(event => !type || event.type === type)
            .sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp))
            .slice(0, limit);
    }

    // Memory Search and Retrieval
    search(query, categories = ['insights', 'patterns', 'problems', 'history']) {
        const results = {
            insights: [],
            patterns: [],
            problems: [],
            history: [],
            contexts: []
        };
        
        const queryLower = query.toLowerCase();
        
        if (categories.includes('insights')) {
            results.insights = this.memory.insights.filter(insight =>
                insight.description.toLowerCase().includes(queryLower) ||
                JSON.stringify(insight.context).toLowerCase().includes(queryLower)
            );
        }
        
        if (categories.includes('patterns')) {
            results.patterns = this.memory.patterns.filter(pattern =>
                pattern.name.toLowerCase().includes(queryLower) ||
                pattern.description.toLowerCase().includes(queryLower)
            );
        }
        
        if (categories.includes('problems')) {
            results.problems = this.memory.problemsSolved.filter(problem =>
                problem.description.toLowerCase().includes(queryLower) ||
                JSON.stringify(problem.context).toLowerCase().includes(queryLower)
            );
        }
        
        if (categories.includes('history')) {
            results.history = this.memory.developmentHistory.filter(event =>
                event.description.toLowerCase().includes(queryLower) ||
                event.files.some(file => file.toLowerCase().includes(queryLower))
            );
        }
        
        if (categories.includes('contexts')) {
            results.contexts = this.getRelevantContext(query, 10);
        }
        
        return results;
    }

    // Export and Import
    exportMemory(filePath = null) {
        const exportPath = filePath || path.join(this.memoryDir, `memory-export-${Date.now()}.json`);
        try {
            fs.writeFileSync(exportPath, JSON.stringify(this.memory, null, 2));
            console.log(`Memory exported to: ${exportPath}`);
            return exportPath;
        } catch (error) {
            console.error('Failed to export memory:', error.message);
            return null;
        }
    }

    importMemory(filePath) {
        try {
            const importedMemory = JSON.parse(fs.readFileSync(filePath, 'utf8'));
            this.memory = { ...this.memory, ...importedMemory };
            this.saveMemory();
            console.log(`Memory imported from: ${filePath}`);
            return true;
        } catch (error) {
            console.error('Failed to import memory:', error.message);
            return false;
        }
    }

    // Statistics and Analytics
    getMemoryStats() {
        return {
            totalSessions: this.memory.sessions.length,
            totalInsights: this.memory.insights.length,
            totalPatterns: this.memory.patterns.length,
            totalProblems: this.memory.problemsSolved.length,
            solvedProblems: this.memory.problemsSolved.filter(p => p.status === 'solved').length,
            contextWindows: this.memory.contextWindows.length,
            keyClasses: Object.keys(this.memory.keyClasses).length,
            importantFiles: Object.keys(this.memory.importantFiles).length,
            developmentEvents: this.memory.developmentHistory.length,
            memorySize: JSON.stringify(this.memory).length,
            lastUpdated: this.memory.lastUpdated
        };
    }

    // Cleanup
    cleanup(daysOld = 30) {
        const cutoff = new Date();
        cutoff.setDate(cutoff.getDate() - daysOld);
        
        const cutoffTime = cutoff.getTime();
        
        // Clean old sessions
        this.memory.sessions = this.memory.sessions.filter(session =>
            new Date(session.started).getTime() > cutoffTime
        );
        
        // Clean old context windows
        this.memory.contextWindows = this.memory.contextWindows.filter(ctx =>
            new Date(ctx.timestamp).getTime() > cutoffTime
        );
        
        // Clean low relevance insights
        this.memory.insights = this.memory.insights.filter(insight =>
            insight.relevance > 0.3 || new Date(insight.timestamp).getTime() > cutoffTime
        );
        
        this.saveMemory();
        console.log(`Cleaned up memory older than ${daysOld} days`);
    }
}

module.exports = MemoryManager;

// CLI Usage
if (require.main === module) {
    const manager = new MemoryManager();
    const args = process.argv.slice(2);
    const command = args[0];
    
    switch (command) {
        case 'start':
            const sessionId = manager.startSession(args[1] || 'New development session');
            console.log(`Session started: ${sessionId}`);
            break;
            
        case 'stats':
            console.log('Memory Statistics:');
            console.log(JSON.stringify(manager.getMemoryStats(), null, 2));
            break;
            
        case 'search':
            if (args[1]) {
                const results = manager.search(args[1]);
                console.log('Search Results:');
                console.log(JSON.stringify(results, null, 2));
            } else {
                console.log('Usage: node memory-manager.js search <query>');
            }
            break;
            
        case 'export':
            const exportPath = manager.exportMemory(args[1]);
            if (exportPath) {
                console.log(`Memory exported to: ${exportPath}`);
            }
            break;
            
        case 'cleanup':
            const days = parseInt(args[1]) || 30;
            manager.cleanup(days);
            break;
            
        default:
            console.log(`
Memory Manager for AtlantisEons Project

Usage:
  node memory-manager.js start [description]     Start a new session
  node memory-manager.js stats                   Show memory statistics  
  node memory-manager.js search <query>          Search memory
  node memory-manager.js export [file]           Export memory
  node memory-manager.js cleanup [days]          Clean old data
            `);
    }
} 