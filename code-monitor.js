#!/usr/bin/env node

/**
 * Unreal Code Monitor
 * A real-time monitoring tool for Unreal Engine projects that integrates with the MPC server
 */

const fs = require('fs');
const path = require('path');
const http = require('http');
const { spawn } = require('child_process');
const chokidar = require('chokidar');

// Configuration
const projectRoot = process.cwd();
const sourceDir = path.join(projectRoot, 'Source');
const buildDir = path.join(projectRoot, 'Intermediate');
const analyzerPort = 9020;
const monitorPort = 9001;
const serverName = 'unreal-code-monitor';
const serverVersion = '0.1.0';

// Analysis cache
const fileCache = new Map();
const analysisCache = new Map();
const issuesCache = new Map();

// Start the MCP server in the background
let mcpServer = null;
let httpServer = null;
let watcher = null;

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
async function analyzeFile(filePath) {
  try {
    // Skip non-source files
    if (!filePath.endsWith('.h') && !filePath.endsWith('.cpp')) {
      return null;
    }
    
    const fileContent = fs.readFileSync(filePath, 'utf8');
    const fileHash = Buffer.from(fileContent).toString('base64').slice(0, 20);
    
    // Check if we already analyzed this file version
    if (fileCache.get(filePath) === fileHash) {
      return analysisCache.get(filePath);
    }
    
    // Store file hash for caching
    fileCache.set(filePath, fileHash);
    
    // Detect patterns
    const patterns = await makeRequest('detect_patterns', { filePath });
    
    // Find all classes in the file
    const classRegex = /class\s+\w+\s+(\w+)(?:\s*:\s*(?:public|private|protected)\s+\w+)?/g;
    const classes = [];
    const content = fileContent.toString();
    let match;
    
    while ((match = classRegex.exec(content)) !== null) {
      const className = match[1];
      if (className.startsWith('U') || className.startsWith('A') || className.startsWith('F')) {
        classes.push(className);
      }
    }
    
    // Analyze each class
    const classAnalyses = [];
    for (const className of classes) {
      try {
        const classInfo = await makeRequest('analyze_class', { className });
        if (classInfo) {
          classAnalyses.push(classInfo);
        }
      } catch (error) {
        console.error(`Error analyzing class ${className}:`, error.message);
      }
    }
    
    // Basic static analysis checks
    const issues = [];
    
    // Check for magic numbers
    const magicNumberRegex = /(?<!\w)[0-9]+\.[0-9]+f?(?!\w)|(?<!\w)[1-9][0-9]*(?!\w|\.)/g;
    while ((match = magicNumberRegex.exec(content)) !== null) {
      const line = content.substring(0, match.index).split('\n').length;
      const lineContent = content.split('\n')[line - 1].trim();
      
      // Skip if in a comment
      if (lineContent.trim().startsWith('//')) continue;
      
      issues.push({
        type: 'magic_number',
        line,
        message: `Magic number ${match[0]} found. Consider using a named constant.`,
        severity: 'warning',
        context: lineContent
      });
    }
    
    // Check for long functions
    const functionBodyRegex = /\w+\s+\w+::\w+\([^)]*\)\s*(?:const)?\s*{([\s\S]*?)}/g;
    while ((match = functionBodyRegex.exec(content)) !== null) {
      const functionBody = match[1];
      const lineCount = functionBody.split('\n').length;
      
      if (lineCount > 50) {
        const line = content.substring(0, match.index).split('\n').length;
        const lineContent = content.split('\n')[line - 1].trim();
        
        issues.push({
          type: 'long_function',
          line,
          message: `Function is ${lineCount} lines long. Consider refactoring.`,
          severity: 'warning',
          context: lineContent
        });
      }
    }
    
    // Store analysis results
    const analysis = {
      file: filePath,
      classes: classAnalyses,
      patterns,
      issues
    };
    
    analysisCache.set(filePath, analysis);
    issuesCache.set(filePath, issues);
    
    return analysis;
  } catch (error) {
    console.error(`Error analyzing file ${filePath}:`, error.message);
    return null;
  }
}

// File watcher
function setupWatcher() {
  watcher = chokidar.watch([
    path.join(sourceDir, '**/*.h'),
    path.join(sourceDir, '**/*.cpp')
  ], {
    ignored: /(^|[\/\\])\../,
    persistent: true
  });
  
  console.log(`Watching source files in ${sourceDir}`);
  
  watcher
    .on('add', async filePath => {
      console.log(`File added: ${filePath}`);
      await analyzeFile(filePath);
    })
    .on('change', async filePath => {
      console.log(`File changed: ${filePath}`);
      await analyzeFile(filePath);
    })
    .on('unlink', filePath => {
      console.log(`File removed: ${filePath}`);
      fileCache.delete(filePath);
      analysisCache.delete(filePath);
      issuesCache.delete(filePath);
    });
}

// HTTP server for real-time monitoring
function setupHttpServer() {
  httpServer = http.createServer((req, res) => {
    if (req.url === '/status') {
      res.writeHead(200, { 'Content-Type': 'application/json' });
      
      const status = {
        name: serverName,
        version: serverVersion,
        filesWatched: fileCache.size,
        filesWithIssues: Array.from(issuesCache.entries())
          .filter(([_, issues]) => issues.length > 0)
          .map(([file, issues]) => ({
            file,
            issueCount: issues.length,
            issues
          }))
      };
      
      res.end(JSON.stringify(status, null, 2));
    } else if (req.url === '/analyze') {
      let body = '';
      
      req.on('data', chunk => {
        body += chunk.toString();
      });
      
      req.on('end', async () => {
        try {
          const request = JSON.parse(body);
          let response;
          
          if (request.file) {
            const filePath = request.file;
            const analysis = await analyzeFile(filePath);
            
            response = {
              success: true,
              analysis
            };
          } else {
            response = {
              success: false,
              error: 'File path required'
            };
          }
          
          res.writeHead(200, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify(response));
        } catch (error) {
          res.writeHead(500, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ error: error.message }));
        }
      });
    } else {
      res.writeHead(200, { 'Content-Type': 'text/plain' });
      res.end(`${serverName} v${serverVersion} running`);
    }
  });
  
  httpServer.listen(monitorPort, () => {
    console.log(`HTTP server listening on port ${monitorPort}`);
    console.log(`Status URL: http://localhost:${monitorPort}/status`);
  });
}

// Initial analysis of all source files
async function initialAnalysis() {
  console.log('Performing initial analysis of all source files...');
  
  const getAllFiles = (dir, fileList = []) => {
    const files = fs.readdirSync(dir);
    
    files.forEach(file => {
      const filePath = path.join(dir, file);
      
      if (fs.statSync(filePath).isDirectory()) {
        getAllFiles(filePath, fileList);
      } else if (file.endsWith('.h') || file.endsWith('.cpp')) {
        fileList.push(filePath);
      }
    });
    
    return fileList;
  };
  
  const sourceFiles = getAllFiles(sourceDir);
  console.log(`Found ${sourceFiles.length} source files`);
  
  let analyzed = 0;
  let withIssues = 0;
  
  for (const file of sourceFiles) {
    const analysis = await analyzeFile(file);
    analyzed++;
    
    if (analysis && analysis.issues.length > 0) {
      withIssues++;
    }
    
    if (analyzed % 10 === 0) {
      console.log(`Analyzed ${analyzed}/${sourceFiles.length} files...`);
    }
  }
  
  console.log(`Initial analysis complete: ${analyzed} files analyzed, ${withIssues} files with issues`);
}

// Main function
async function main() {
  console.log(`Starting ${serverName} (v${serverVersion})...`);
  
  try {
    // Install chokidar if not already installed
    if (!fs.existsSync(path.join(projectRoot, 'node_modules', 'chokidar'))) {
      console.log('Installing required dependencies...');
      
      const npmInstall = spawn('npm', ['install', '--no-save', 'chokidar'], {
        stdio: 'inherit'
      });
      
      await new Promise((resolve, reject) => {
        npmInstall.on('close', code => {
          if (code === 0) {
            resolve();
          } else {
            reject(new Error(`npm install failed with code ${code}`));
          }
        });
      });
    }
    
    // Start the dynamic MCP server
    console.log('Starting MCP server on port ' + analyzerPort + '...');
    
    mcpServer = spawn('node', ['dynamic-mcp-server.js'], {
      stdio: 'pipe',
      detached: false,
      env: {
        ...process.env,
        PORT: analyzerPort.toString()
      }
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
    
    // Set up file watcher
    setupWatcher();
    
    // Set up HTTP server
    setupHttpServer();
    
    // Perform initial analysis
    await initialAnalysis();
    
    console.log(`${serverName} is running. Press Ctrl+C to exit.`);
  } catch (error) {
    console.error(`Error: ${error.message}`);
    cleanup();
    process.exit(1);
  }
}

// Cleanup function
function cleanup() {
  console.log('Cleaning up...');
  
  if (watcher) {
    watcher.close();
  }
  
  if (httpServer) {
    httpServer.close();
  }
  
  if (mcpServer) {
    process.kill(-mcpServer.pid);
  }
}

// Handle signals
process.on('SIGINT', () => {
  console.log('Shutting down...');
  cleanup();
  process.exit(0);
});

// Run the main function
main().catch(err => {
  console.error('Fatal error:', err);
  cleanup();
  process.exit(1);
}); 