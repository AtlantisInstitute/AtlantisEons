#!/usr/bin/env node

/**
 * Unreal Code Analyzer - MPC Integration
 * A dynamic code analysis tool for Unreal Engine projects
 */

const fs = require('fs');
const path = require('path');
const http = require('http');
const { spawn } = require('child_process');

// Configuration
const projectRoot = process.cwd();
const sourceDir = path.join(projectRoot, 'Source');
const analyzerPort = 9010;
const serverName = 'unreal-analyzer';
const serverVersion = '0.2.0';

// Start the MCP server in the background
let mcpServer = null;

// Helper function to make HTTP requests to the MCP server
async function makeRequest(method, params = {}) {
  return new Promise((resolve, reject) => {
    const req = http.request({
      hostname: 'localhost',
      port: analyzerPort,
      path: '/',
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      }
    }, (res) => {
      let data = '';
      res.on('data', (chunk) => {
        data += chunk;
      });
      res.on('end', () => {
        try {
          const response = JSON.parse(data);
          resolve(response);
        } catch (error) {
          reject(new Error(`Failed to parse response: ${error.message}`));
        }
      });
    });

    req.on('error', (error) => {
      reject(error);
    });

    const requestBody = JSON.stringify({
      method,
      params
    });

    req.write(requestBody);
    req.end();
  });
}

// Analysis functions
async function analyzeClass(className) {
  return makeRequest('analyze_class', { className });
}

async function findClassHierarchy(className) {
  return makeRequest('find_class_hierarchy', { className });
}

async function findReferences(identifier, type = 'class') {
  return makeRequest('find_references', { identifier, type });
}

async function searchCode(query, filePattern = '*.{h,cpp}') {
  return makeRequest('search_code', { query, filePattern });
}

async function detectPatternsInFile(filePath) {
  const absPath = path.isAbsolute(filePath) ? filePath : path.join(projectRoot, filePath);
  
  if (!fs.existsSync(absPath)) {
    throw new Error(`File not found: ${absPath}`);
  }
  
  return makeRequest('detect_patterns', { filePath: absPath });
}

// Main function
async function main() {
  // Start the MCP server
  console.log(`Starting ${serverName} (v${serverVersion})...`);
  
  try {
    // Start the dynamic MCP server in a child process
    mcpServer = spawn('node', ['dynamic-mcp-server.js'], {
      stdio: 'pipe',
      detached: false
    });
    
    mcpServer.stdout.on('data', (data) => {
      console.log(`[MCP Server] ${data.toString().trim()}`);
    });
    
    mcpServer.stderr.on('data', (data) => {
      console.error(`[MCP Server Error] ${data.toString().trim()}`);
    });
    
    mcpServer.on('close', (code) => {
      console.log(`MCP Server exited with code ${code}`);
    });
    
    // Wait for server to start
    console.log('Waiting for MCP server to initialize...');
    await new Promise(resolve => setTimeout(resolve, 2000));
    
    // Process command line arguments
    const args = process.argv.slice(2);
    
    if (args.length === 0) {
      console.log(`
${serverName} (v${serverVersion}) - Unreal Engine Code Analyzer

Usage:
  node unreal-code-analyzer.js <command> [options]

Commands:
  analyze-class <className>            Get detailed information about a class
  find-hierarchy <className>           Get the inheritance hierarchy for a class
  find-references <identifier> [type]  Find all references to a symbol
  search <query> [filePattern]         Search through code with a query
  detect-patterns <filePath>           Detect Unreal Engine patterns in a file
  analyze-project                      Analyze the entire project

Examples:
  node unreal-code-analyzer.js analyze-class AMyCharacter
  node unreal-code-analyzer.js find-references TakeDamage function
  node unreal-code-analyzer.js search "UPROPERTY\\(.*?BlueprintReadWrite"
  node unreal-code-analyzer.js detect-patterns Source/MyProject/MyClass.h
  node unreal-code-analyzer.js analyze-project
      `);
      process.exit(0);
    }
    
    const command = args[0];
    
    switch (command) {
      case 'analyze-class':
        if (args.length < 2) {
          console.error('Error: Class name required');
          process.exit(1);
        }
        const classInfo = await analyzeClass(args[1]);
        console.log(JSON.stringify(classInfo, null, 2));
        break;
        
      case 'find-hierarchy':
        if (args.length < 2) {
          console.error('Error: Class name required');
          process.exit(1);
        }
        const hierarchy = await findClassHierarchy(args[1]);
        console.log(JSON.stringify(hierarchy, null, 2));
        break;
        
      case 'find-references':
        if (args.length < 2) {
          console.error('Error: Identifier required');
          process.exit(1);
        }
        const type = args.length > 2 ? args[2] : 'class';
        const references = await findReferences(args[1], type);
        console.log(JSON.stringify(references, null, 2));
        break;
        
      case 'search':
        if (args.length < 2) {
          console.error('Error: Search query required');
          process.exit(1);
        }
        const filePattern = args.length > 2 ? args[2] : '*.{h,cpp}';
        const results = await searchCode(args[1], filePattern);
        console.log(JSON.stringify(results, null, 2));
        break;
        
      case 'detect-patterns':
        if (args.length < 2) {
          console.error('Error: File path required');
          process.exit(1);
        }
        const patterns = await detectPatternsInFile(args[1]);
        console.log(JSON.stringify(patterns, null, 2));
        break;
        
      case 'analyze-project':
        console.log('Analyzing entire project...');
        // Implement project-wide analysis here
        // This would involve scanning all source files and running various analyses
        
        break;
        
      default:
        console.error(`Unknown command: ${command}`);
        process.exit(1);
    }
  } catch (error) {
    console.error(`Error: ${error.message}`);
  } finally {
    // Clean up
    if (mcpServer) {
      console.log('Shutting down MCP server...');
      process.kill(-mcpServer.pid);
    }
  }
}

// Handle signals
process.on('SIGINT', () => {
  console.log('Shutting down...');
  if (mcpServer) {
    process.kill(-mcpServer.pid);
  }
  process.exit(0);
});

// Run the main function
main().catch(err => {
  console.error('Fatal error:', err);
  if (mcpServer) {
    process.kill(-mcpServer.pid);
  }
  process.exit(1);
}); 