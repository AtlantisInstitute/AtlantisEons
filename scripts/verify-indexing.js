#!/usr/bin/env node

/**
 * Verify Codebase Indexing Results
 * Check what data was actually stored in the memory system
 */

const fs = require('fs');
const path = require('path');

const projectRoot = '/Users/danielvargas/Documents/Unreal Projects/AtlantisEons';
const memoryFile = path.join(projectRoot, '.memory', 'project-memory.json');

function verifyIndexing() {
    console.log('🔍 Verifying codebase indexing results...');
    
    if (!fs.existsSync(memoryFile)) {
        console.error('❌ Memory file not found!');
        return;
    }
    
    const memoryData = JSON.parse(fs.readFileSync(memoryFile, 'utf8'));
    
    console.log('\n📊 MEMORY SYSTEM OVERVIEW:');
    console.log(`Project: ${memoryData.projectName}`);
    console.log(`Sessions: ${memoryData.sessions.length}`);
    console.log(`Insights: ${memoryData.insights.length}`);
    console.log(`Patterns tracked: ${Object.keys(memoryData.patternFrequency || {}).length}`);
    
    // Check for the latest indexing session
    const latestSessions = memoryData.sessions.slice(-3);
    console.log('\n🔄 RECENT SESSIONS:');
    latestSessions.forEach(session => {
        console.log(`  ${session.id}: ${session.description}`);
        console.log(`    Started: ${new Date(session.started).toLocaleString()}`);
        console.log(`    Insights: ${session.insights.length}`);
        console.log(`    Actions: ${session.actions?.length || 0}`);
    });
    
    // Check for file indexing insights
    const indexingInsights = memoryData.insights.filter(insight => 
        insight.type === 'file-analysis' || 
        insight.type === 'indexing-summary' ||
        insight.type === 'architecture-analysis'
    );
    
    console.log('\n🏗️  INDEXING INSIGHTS:');
    indexingInsights.forEach(insight => {
        console.log(`  ${insight.type}: ${insight.description}`);
        if (insight.context) {
            console.log(`    Context: ${JSON.stringify(insight.context)}`);
        }
    });
    
    // Check keyClasses data
    console.log('\n🔑 KEY CLASSES:');
    const keyClasses = memoryData.keyClasses || {};
    const classCount = Object.keys(keyClasses).length;
    console.log(`  Total classes indexed: ${classCount}`);
    
    if (classCount > 0) {
        console.log('\n  Sample classes:');
        const sampleClasses = Object.keys(keyClasses).slice(0, 10);
        sampleClasses.forEach(className => {
            const classData = keyClasses[className];
            console.log(`    ${className}: ${classData.file} (${classData.lineCount} lines)`);
        });
    }
    
    // Check importantFiles data
    console.log('\n📄 IMPORTANT FILES:');
    const importantFiles = memoryData.importantFiles || {};
    const fileCount = Object.keys(importantFiles).length;
    console.log(`  Total files indexed: ${fileCount}`);
    
    if (fileCount > 0) {
        console.log('\n  Sample files:');
        const sampleFiles = Object.keys(importantFiles).slice(0, 10);
        sampleFiles.forEach(fileName => {
            const fileData = importantFiles[fileName];
            console.log(`    ${fileName}: ${fileData.purpose || 'No purpose defined'}`);
        });
    }
    
    // Check pattern frequency
    console.log('\n🎯 PATTERNS IDENTIFIED:');
    const patterns = memoryData.patternFrequency || {};
    Object.entries(patterns).forEach(([pattern, frequency]) => {
        console.log(`  ${pattern}: ${frequency} occurrences`);
    });
    
    // Check for the most recent memory entries
    console.log('\n📝 RECENT MEMORY ENTRIES:');
    const recentEntries = memoryData.memoryEntries?.slice(-5) || [];
    recentEntries.forEach(entry => {
        console.log(`  ${entry.timestamp}: ${entry.summary}`);
        console.log(`    Tags: ${entry.tags?.join(', ') || 'none'}`);
    });
    
    console.log('\n✅ Verification complete!');
}

// Run verification
verifyIndexing(); 