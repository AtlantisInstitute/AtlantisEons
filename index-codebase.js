#!/usr/bin/env node

/**
 * AtlantisEons Codebase Indexer
 * Comprehensive indexing of all source files into the memory system
 */

const fs = require('fs');
const path = require('path');
const MemoryManager = require('./memory-manager');

// Configuration
const projectRoot = '/Users/danielvargas/Documents/Unreal Projects/AtlantisEons';
const sourceDir = path.join(projectRoot, 'Source');

// Initialize memory manager
const memoryManager = new MemoryManager(projectRoot);

// Start indexing session
const sessionId = memoryManager.startSession('Comprehensive Codebase Indexing - Full Project Analysis');

class CodebaseIndexer {
    constructor() {
        this.fileExtensions = ['.cpp', '.h', '.cs'];
        this.ignoredFiles = ['.DS_Store', '.original'];
        this.statistics = {
            totalFiles: 0,
            processedFiles: 0,
            skippedFiles: 0,
            headerFiles: 0,
            implementationFiles: 0,
            buildFiles: 0,
            totalLines: 0,
            errors: []
        };
    }

    async indexEntireCodebase() {
        console.log('🚀 Starting comprehensive codebase indexing...');
        
        try {
            // Scan and process all source files
            await this.scanDirectory(sourceDir);
            
            // Add project-level insights
            await this.addProjectInsights();
            
            // Generate summary
            await this.generateIndexingSummary();
            
            console.log('✅ Codebase indexing completed successfully!');
            
        } catch (error) {
            console.error('❌ Indexing failed:', error);
            this.statistics.errors.push(error.message);
        }
        
        // End session with summary
        memoryManager.endSession(this.generateSessionSummary());
        
        return this.statistics;
    }

    async scanDirectory(dirPath) {
        const items = fs.readdirSync(dirPath);
        
        for (const item of items) {
            const fullPath = path.join(dirPath, item);
            const stat = fs.statSync(fullPath);
            
            if (stat.isDirectory()) {
                console.log(`📁 Scanning directory: ${path.relative(projectRoot, fullPath)}`);
                await this.scanDirectory(fullPath);
            } else if (stat.isFile()) {
                await this.processFile(fullPath);
            }
        }
    }

    async processFile(filePath) {
        const relativePath = path.relative(projectRoot, filePath);
        const fileName = path.basename(filePath);
        const extension = path.extname(filePath);
        
        // Skip ignored files
        if (this.ignoredFiles.some(ignored => fileName.includes(ignored))) {
            this.statistics.skippedFiles++;
            console.log(`⏭️  Skipping: ${relativePath}`);
            return;
        }
        
        // Only process relevant files
        if (!this.fileExtensions.includes(extension)) {
            this.statistics.skippedFiles++;
            return;
        }
        
        this.statistics.totalFiles++;
        
        try {
            console.log(`📄 Processing: ${relativePath}`);
            
            const content = fs.readFileSync(filePath, 'utf8');
            const analysis = await this.analyzeFile(filePath, content);
            
            // Create memory entry for this file
            await this.createFileMemory(filePath, analysis);
            
            // Update statistics
            this.statistics.processedFiles++;
            this.statistics.totalLines += analysis.lineCount;
            
            if (extension === '.h') {
                this.statistics.headerFiles++;
            } else if (extension === '.cpp') {
                this.statistics.implementationFiles++;
            } else if (extension === '.cs') {
                this.statistics.buildFiles++;
            }
            
            // Log action
            memoryManager.logAction('file_indexed', {
                file: relativePath,
                type: analysis.fileType,
                classes: analysis.classes.length,
                functions: analysis.functions.length,
                lineCount: analysis.lineCount
            });
            
        } catch (error) {
            console.error(`❌ Error processing ${relativePath}:`, error.message);
            this.statistics.errors.push(`${relativePath}: ${error.message}`);
        }
    }

    async analyzeFile(filePath, content) {
        const relativePath = path.relative(projectRoot, filePath);
        const fileName = path.basename(filePath);
        const extension = path.extname(filePath);
        const lines = content.split('\n');
        
        const analysis = {
            file: relativePath,
            fileName: fileName,
            extension: extension,
            fileType: this.determineFileType(fileName, content),
            lineCount: lines.length,
            size: content.length,
            classes: [],
            functions: [],
            includes: [],
            dependencies: [],
            patterns: [],
            complexity: 'unknown',
            purpose: '',
            keyFeatures: [],
            unrealSpecific: {
                isUEClass: false,
                inheritance: [],
                macros: [],
                blueprintExposed: false,
                components: []
            }
        };

        // Analyze content
        analysis.classes = this.extractClasses(content);
        analysis.functions = this.extractFunctions(content);
        analysis.includes = this.extractIncludes(content);
        analysis.dependencies = this.extractDependencies(content);
        analysis.unrealSpecific = this.analyzeUnrealFeatures(content);
        analysis.patterns = this.identifyPatterns(content, fileName);
        analysis.complexity = this.assessComplexity(content, lines.length);
        analysis.purpose = this.determinePurpose(fileName, content, analysis.classes);
        analysis.keyFeatures = this.extractKeyFeatures(content, analysis);

        return analysis;
    }

    determineFileType(fileName, content) {
        if (fileName.endsWith('.h')) {
            if (content.includes('UCLASS') || content.includes('USTRUCT')) {
                return 'UE_Header';
            }
            return 'Header';
        } else if (fileName.endsWith('.cpp')) {
            if (content.includes('#include') && content.includes('IMPLEMENT_')) {
                return 'UE_Implementation';
            }
            return 'Implementation';
        } else if (fileName.endsWith('.cs')) {
            return 'Build_Script';
        }
        return 'Unknown';
    }

    extractClasses(content) {
        const classes = [];
        
        // C++ class patterns
        const classRegex = /class\s+([A-Z_][A-Z0-9_]*)\s*(?::\s*public\s+([A-Z_][A-Z0-9_:]*))?\s*{/gi;
        let match;
        
        while ((match = classRegex.exec(content)) !== null) {
            classes.push({
                name: match[1],
                inheritance: match[2] || null,
                isUEClass: match[1].startsWith('A') || match[1].startsWith('U') || match[1].startsWith('F'),
                line: content.substring(0, match.index).split('\n').length
            });
        }
        
        // UCLASS patterns
        const uclassRegex = /UCLASS\([^)]*\)\s*class\s+[A-Z_]+\s+([A-Z_][A-Z0-9_]*)/gi;
        while ((match = uclassRegex.exec(content)) !== null) {
            const existing = classes.find(c => c.name === match[1]);
            if (existing) {
                existing.isUEClass = true;
                existing.hasUCLASS = true;
            } else {
                classes.push({
                    name: match[1],
                    isUEClass: true,
                    hasUCLASS: true,
                    line: content.substring(0, match.index).split('\n').length
                });
            }
        }
        
        return classes;
    }

    extractFunctions(content) {
        const functions = [];
        
        // Function patterns (simplified)
        const funcRegex = /(?:virtual\s+)?(?:static\s+)?(?:UFUNCTION\([^)]*\)\s+)?([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\([^)]*\)\s*(?:const\s*)?(?:override\s*)?[{;]/gi;
        let match;
        
        while ((match = funcRegex.exec(content)) !== null) {
            const returnType = match[1];
            const funcName = match[2];
            
            // Skip constructors, destructors, and common keywords
            if (!['if', 'for', 'while', 'switch', 'class', 'struct'].includes(funcName) && 
                !funcName.includes('~') && returnType !== 'class') {
                functions.push({
                    name: funcName,
                    returnType: returnType,
                    isUFunction: content.substring(Math.max(0, match.index - 100), match.index).includes('UFUNCTION'),
                    line: content.substring(0, match.index).split('\n').length
                });
            }
        }
        
        return functions.slice(0, 20); // Limit to prevent overwhelming data
    }

    extractIncludes(content) {
        const includes = [];
        const includeRegex = /#include\s+["<]([^">]+)[">]/g;
        let match;
        
        while ((match = includeRegex.exec(content)) !== null) {
            includes.push(match[1]);
        }
        
        return includes;
    }

    extractDependencies(content) {
        const dependencies = [];
        
        // Extract class dependencies from includes and forward declarations
        const includes = this.extractIncludes(content);
        
        includes.forEach(include => {
            if (!include.startsWith('Engine/') && !include.startsWith('CoreMinimal') && 
                !include.includes('std') && !include.includes('Windows.h')) {
                const depName = path.basename(include, '.h');
                if (depName && !dependencies.includes(depName)) {
                    dependencies.push(depName);
                }
            }
        });
        
        // Forward declarations
        const forwardRegex = /class\s+([A-Z_][A-Z0-9_]*)\s*;/g;
        let match;
        while ((match = forwardRegex.exec(content)) !== null) {
            if (!dependencies.includes(match[1])) {
                dependencies.push(match[1]);
            }
        }
        
        return dependencies.slice(0, 15); // Limit dependencies
    }

    analyzeUnrealFeatures(content) {
        const ueFeatures = {
            isUEClass: false,
            inheritance: [],
            macros: [],
            blueprintExposed: false,
            components: []
        };
        
        // Check for UE class inheritance
        if (content.includes(': public A') || content.includes(': public U') || content.includes(': public F')) {
            ueFeatures.isUEClass = true;
        }
        
        // Extract UE macros
        const macros = ['UCLASS', 'USTRUCT', 'UFUNCTION', 'UPROPERTY', 'UENUM', 'GENERATED_BODY'];
        macros.forEach(macro => {
            if (content.includes(macro)) {
                ueFeatures.macros.push(macro);
            }
        });
        
        // Check for Blueprint exposure
        if (content.includes('BlueprintCallable') || content.includes('BlueprintImplementableEvent')) {
            ueFeatures.blueprintExposed = true;
        }
        
        // Extract components
        const componentRegex = /UPROPERTY[^)]*\)\s*[A-Za-z_][A-Za-z0-9_]*\s*\*\s*([A-Za-z_][A-Za-z0-9_]*Component)/g;
        let match;
        while ((match = componentRegex.exec(content)) !== null) {
            ueFeatures.components.push(match[1]);
        }
        
        return ueFeatures;
    }

    identifyPatterns(content, fileName) {
        const patterns = [];
        
        // Widget pattern
        if (fileName.startsWith('WBP_')) {
            patterns.push('UE_Widget_Pattern');
        }
        
        // Character pattern
        if (fileName.includes('Character')) {
            patterns.push('Character_Class_Pattern');
        }
        
        // Component pattern
        if (fileName.includes('Component')) {
            patterns.push('Component_Pattern');
        }
        
        // AI patterns
        if (fileName.includes('BT') || fileName.includes('AI')) {
            patterns.push('AI_Behavior_Pattern');
        }
        
        // Damage system pattern
        if (fileName.includes('Damage')) {
            patterns.push('Damage_System_Pattern');
        }
        
        // Store system pattern
        if (fileName.includes('Store') || fileName.includes('Shop')) {
            patterns.push('Store_System_Pattern');
        }
        
        // Inventory pattern
        if (fileName.includes('Inventory')) {
            patterns.push('Inventory_System_Pattern');
        }
        
        return patterns;
    }

    assessComplexity(content, lineCount) {
        if (lineCount < 100) return 'simple';
        if (lineCount < 500) return 'moderate';
        if (lineCount < 1500) return 'complex';
        return 'very_complex';
    }

    determinePurpose(fileName, content, classes) {
        // Determine file purpose based on name and content
        if (fileName.startsWith('WBP_')) {
            return 'UI Widget implementation';
        } else if (fileName.includes('Character')) {
            return 'Character class definition and behavior';
        } else if (fileName.includes('Component')) {
            return 'Component system functionality';
        } else if (fileName.includes('GameMode')) {
            return 'Game mode and rules implementation';
        } else if (fileName.includes('HUD')) {
            return 'HUD and user interface management';
        } else if (fileName.includes('AI') || fileName.includes('BT')) {
            return 'AI behavior and decision making';
        } else if (fileName.includes('Damage')) {
            return 'Damage calculation and display system';
        } else if (fileName.includes('Store') || fileName.includes('Shop')) {
            return 'Store and commerce system';
        } else if (fileName.includes('Inventory')) {
            return 'Inventory management system';
        } else if (fileName.includes('Item')) {
            return 'Item system and data structures';
        } else if (fileName.endsWith('.Build.cs')) {
            return 'Build configuration and dependencies';
        } else if (fileName.endsWith('.Target.cs')) {
            return 'Build target configuration';
        }
        
        return 'General implementation file';
    }

    extractKeyFeatures(content, analysis) {
        const features = [];
        
        // Based on UE features
        if (analysis.unrealSpecific.blueprintExposed) {
            features.push('Blueprint Integration');
        }
        
        if (analysis.unrealSpecific.components.length > 0) {
            features.push('Component Architecture');
        }
        
        // Based on content analysis
        if (content.includes('TArray')) {
            features.push('Dynamic Arrays');
        }
        
        if (content.includes('FVector') || content.includes('FTransform')) {
            features.push('3D Mathematics');
        }
        
        if (content.includes('UWidget') || content.includes('UserWidget')) {
            features.push('UI System');
        }
        
        if (content.includes('Multicast') || content.includes('Server') || content.includes('Client')) {
            features.push('Networking');
        }
        
        if (content.includes('Timer') || content.includes('Delay')) {
            features.push('Timing System');
        }
        
        if (content.includes('Animation') || content.includes('Montage')) {
            features.push('Animation System');
        }
        
        return features;
    }

    async createFileMemory(filePath, analysis) {
        const relativePath = path.relative(projectRoot, filePath);
        
        // Store class information
        if (analysis.classes.length > 0) {
            analysis.classes.forEach(classInfo => {
                memoryManager.rememberClass(classInfo.name, {
                    file: relativePath,
                    lineCount: analysis.lineCount,
                    fileType: analysis.fileType,
                    inheritance: classInfo.inheritance,
                    isUEClass: classInfo.isUEClass,
                    functions: analysis.functions.length,
                    dependencies: analysis.dependencies,
                    purpose: analysis.purpose,
                    complexity: analysis.complexity,
                    patterns: analysis.patterns,
                    keyFeatures: analysis.keyFeatures,
                    content: null // Don't store full content to save space
                });
            });
        }
        
        // Store file information
        memoryManager.rememberFile(relativePath, {
            fileName: analysis.fileName,
            fileType: analysis.fileType,
            lineCount: analysis.lineCount,
            size: analysis.size,
            classes: analysis.classes.map(c => c.name),
            functions: analysis.functions.map(f => f.name),
            includes: analysis.includes,
            dependencies: analysis.dependencies,
            patterns: analysis.patterns,
            complexity: analysis.complexity,
            purpose: analysis.purpose,
            keyFeatures: analysis.keyFeatures,
            unrealFeatures: analysis.unrealSpecific,
            indexed: new Date().toISOString()
        });
        
        // Add patterns to memory
        analysis.patterns.forEach(pattern => {
            memoryManager.updatePatternFrequency(pattern);
        });
        
        // Add insight for significant files
        if (analysis.lineCount > 1000 || analysis.classes.length > 0) {
            memoryManager.addInsight(
                'file-analysis',
                `Indexed ${analysis.fileType} file: ${relativePath} - ${analysis.purpose}`,
                {
                    file: relativePath,
                    classes: analysis.classes.length,
                    functions: analysis.functions.length,
                    complexity: analysis.complexity,
                    keyFeatures: analysis.keyFeatures
                }
            );
        }
    }

    async addProjectInsights() {
        console.log('📝 Adding project-level insights...');
        
        // Architecture insight
        memoryManager.addInsight(
            'architecture-analysis',
            `AtlantisEons codebase indexed: ${this.statistics.processedFiles} files processed with ${this.statistics.totalLines} total lines of code`,
            {
                files: this.statistics.processedFiles,
                totalLines: this.statistics.totalLines,
                headerFiles: this.statistics.headerFiles,
                implementationFiles: this.statistics.implementationFiles,
                buildFiles: this.statistics.buildFiles
            }
        );
        
        // Complexity insight
        const avgLinesPerFile = Math.round(this.statistics.totalLines / this.statistics.processedFiles);
        memoryManager.addInsight(
            'complexity-analysis',
            `Project complexity assessment: Average ${avgLinesPerFile} lines per file`,
            {
                averageLinesPerFile: avgLinesPerFile,
                totalFiles: this.statistics.processedFiles,
                complexity: avgLinesPerFile > 500 ? 'high' : avgLinesPerFile > 200 ? 'medium' : 'low'
            }
        );
        
        // Technology stack insight
        memoryManager.addInsight(
            'technology-stack',
            'AtlantisEons built with Unreal Engine 5.5 using C++ and Blueprint integration',
            {
                engine: 'UE 5.5',
                language: 'C++',
                buildSystem: 'UnrealBuildTool',
                uiSystem: 'UMG Widgets',
                aiSystem: 'Behavior Trees'
            }
        );
    }

    async generateIndexingSummary() {
        console.log('\n📊 INDEXING SUMMARY:');
        console.log(`✅ Total files processed: ${this.statistics.processedFiles}`);
        console.log(`📄 Header files: ${this.statistics.headerFiles}`);
        console.log(`⚙️  Implementation files: ${this.statistics.implementationFiles}`);
        console.log(`🔧 Build files: ${this.statistics.buildFiles}`);
        console.log(`📏 Total lines of code: ${this.statistics.totalLines.toLocaleString()}`);
        console.log(`⏭️  Files skipped: ${this.statistics.skippedFiles}`);
        
        if (this.statistics.errors.length > 0) {
            console.log(`❌ Errors encountered: ${this.statistics.errors.length}`);
            this.statistics.errors.forEach(error => console.log(`   - ${error}`));
        }
        
        // Add summary insight
        memoryManager.addInsight(
            'indexing-summary',
            `Codebase indexing completed: ${this.statistics.processedFiles} files, ${this.statistics.totalLines} lines of code`,
            {
                timestamp: new Date().toISOString(),
                statistics: this.statistics
            }
        );
    }

    generateSessionSummary() {
        return `Comprehensive codebase indexing completed. Processed ${this.statistics.processedFiles} files with ${this.statistics.totalLines} lines of code. Added detailed memory entries for all classes, functions, and architectural patterns.`;
    }
}

// Main execution
async function main() {
    console.log('🚀 AtlantisEons Codebase Indexer Starting...');
    console.log(`📁 Project Root: ${projectRoot}`);
    console.log(`📂 Source Directory: ${sourceDir}`);
    
    const indexer = new CodebaseIndexer();
    const results = await indexer.indexEntireCodebase();
    
    console.log('\n🎉 Indexing completed! All source files have been added to the memory system.');
    console.log('💾 Memory data saved and ready for MCP server integration.');
    
    return results;
}

// Run if called directly
if (require.main === module) {
    main().catch(console.error);
}

module.exports = CodebaseIndexer; 