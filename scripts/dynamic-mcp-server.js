#!/usr/bin/env node

// Dynamic MCP server for Unreal Engine project analysis
const fs = require('fs');
const path = require('path');
const { exec, spawn } = require('child_process');
const http = require('http');

// Configuration
const projectRoot = '/Users/danielvargas/Documents/Unreal Projects/AtlantisEons';
const serverName = 'unreal-analyzer';
const serverVersion = '0.2.0';
const serverPort = process.env.PORT ? parseInt(process.env.PORT) : 9010;

// In-memory cache
const codeCache = new Map();
const classCache = new Map();
const hierarchyCache = new Map();
const referenceCache = new Map();

// Helper function to scan directories
async function scanDirectory(dir, fileTypes = ['.h', '.cpp']) {
  return new Promise((resolve, reject) => {
    exec(`find "${dir}" -type f -name "*${fileTypes.join('" -o -name "*')}"`, (error, stdout, stderr) => {
      if (error) {
        reject(error);
        return;
      }
      resolve(stdout.trim().split('\n').filter(file => file));
    });
  });
}

// Read file content with caching
async function readFileContent(filePath) {
  if (codeCache.has(filePath)) {
    return codeCache.get(filePath);
  }
  
  return new Promise((resolve, reject) => {
    fs.readFile(filePath, 'utf8', (err, data) => {
      if (err) {
        reject(err);
        return;
      }
      codeCache.set(filePath, data);
      resolve(data);
    });
  });
}

// Find class definition in file
async function findClassDefinition(className, fileContent, filePath) {
  // Look for class declaration
  const classRegex = new RegExp(`class\\s+\\w+\\s+${className}\\s*(?::\\s*\\w+)?\\s*{`, 'g');
  const match = classRegex.exec(fileContent);
  
  if (match) {
    // Extract class block - simplified approach
    const startPos = match.index;
    let braceCount = 0;
    let endPos = startPos;
    
    for (let i = startPos; i < fileContent.length; i++) {
      if (fileContent[i] === '{') braceCount++;
      if (fileContent[i] === '}') {
        braceCount--;
        if (braceCount === 0) {
          endPos = i;
          break;
        }
      }
    }
    
    return {
      name: className,
      file: filePath,
      content: fileContent.substring(startPos, endPos + 1),
      lineNumber: fileContent.substring(0, startPos).split('\n').length
    };
  }
  
  return null;
}

// Extract superclasses from class definition
function extractSuperclasses(classDefinition) {
  if (!classDefinition) return [];
  
  const inheritanceRegex = /class\s+\w+\s+\w+\s*:\s*(?:public\s+|private\s+|protected\s+)?(\w+)/;
  const match = inheritanceRegex.exec(classDefinition.content);
  
  if (match && match[1]) {
    return [match[1]];
  }
  
  return [];
}

// Find class hierarchy
async function findClassHierarchy(className) {
  if (hierarchyCache.has(className)) {
    return hierarchyCache.get(className);
  }
  
  const sourceDir = path.join(projectRoot, 'Source');
  const files = await scanDirectory(sourceDir);
  
  for (const file of files) {
    if (path.extname(file) === '.h') {
      const content = await readFileContent(file);
      const classDefinition = await findClassDefinition(className, content, file);
      
      if (classDefinition) {
        const superclasses = extractSuperclasses(classDefinition);
        const hierarchy = [];
        
        for (const superclass of superclasses) {
          hierarchy.push(superclass);
          const parentHierarchy = await findClassHierarchy(superclass);
          hierarchy.push(...parentHierarchy);
        }
        
        hierarchyCache.set(className, hierarchy);
        return hierarchy;
      }
    }
  }
  
  return [];
}

// Find class references
async function findReferences(identifier) {
  if (referenceCache.has(identifier)) {
    return referenceCache.get(identifier);
  }
  
  const sourceDir = path.join(projectRoot, 'Source');
  const files = await scanDirectory(sourceDir);
  const references = [];
  
  for (const file of files) {
    const content = await readFileContent(file);
    const regex = new RegExp(`\\b${identifier}\\b`, 'g');
    let match;
    
    while ((match = regex.exec(content)) !== null) {
      const lineNumber = content.substring(0, match.index).split('\n').length;
      const line = content.split('\n')[lineNumber - 1].trim();
      
      references.push({
        file,
        lineNumber,
        context: line
      });
    }
  }
  
  referenceCache.set(identifier, references);
  return references;
}

// Search code
async function searchCode(query, filePattern = '*.{h,cpp}') {
  const sourceDir = path.join(projectRoot, 'Source');
  const fileTypeRegex = filePattern.replace(/\*/g, '.*').replace(/\./g, '\\.').replace(/{|}/g, m => m === '{' ? '(' : ')');
  const files = await scanDirectory(sourceDir);
  const filteredFiles = files.filter(file => new RegExp(fileTypeRegex).test(path.basename(file)));
  const results = [];
  
  for (const file of filteredFiles) {
    const content = await readFileContent(file);
    const regex = new RegExp(query, 'g');
    let match;
    
    while ((match = regex.exec(content)) !== null) {
      const lineNumber = content.substring(0, match.index).split('\n').length;
      const line = content.split('\n')[lineNumber - 1].trim();
      
      results.push({
        file,
        lineNumber,
        context: line
      });
    }
  }
  
  return results;
}

// Analyze class
async function analyzeClass(className) {
  if (classCache.has(className)) {
    return classCache.get(className);
  }
  
  const sourceDir = path.join(projectRoot, 'Source');
  const files = await scanDirectory(sourceDir);
  
  for (const file of files) {
    if (path.extname(file) === '.h') {
      const content = await readFileContent(file);
      const classDefinition = await findClassDefinition(className, content, file);
      
      if (classDefinition) {
        // Extract methods
        const methodRegex = /UFUNCTION\s*\(.*?\)\s*(?:virtual\s+)?(?:static\s+)?(?:\w+\s+)+(\w+)\s*\([^)]*\)/g;
        const methods = [];
        let methodMatch;
        
        while ((methodMatch = methodRegex.exec(classDefinition.content)) !== null) {
          methods.push(methodMatch[1]);
        }
        
        // Extract properties
        const propertyRegex = /UPROPERTY\s*\(.*?\)\s*(?:virtual\s+)?(?:static\s+)?(?:\w+\s+)+(\w+)\s*;/g;
        const properties = [];
        let propertyMatch;
        
        while ((propertyMatch = propertyRegex.exec(classDefinition.content)) !== null) {
          properties.push(propertyMatch[1]);
        }
        
        const result = {
          name: className,
          file: classDefinition.file,
          lineNumber: classDefinition.lineNumber,
          superclasses: extractSuperclasses(classDefinition),
          methods,
          properties
        };
        
        classCache.set(className, result);
        return result;
      }
    }
  }
  
  return null;
}

// Initialize index
async function initializeIndex() {
  console.log('Initializing source index...');
  const sourceDir = path.join(projectRoot, 'Source');
  const sourceFiles = await scanDirectory(sourceDir);
  
  console.log(`Found ${sourceFiles.length} source files`);
  
  // Preliminary scan to identify classes
  for (const file of sourceFiles) {
    if (path.extname(file) === '.h') {
      const content = await readFileContent(file);
      const classRegex = /class\s+\w+\s+(\w+)(?:\s*:\s*(?:public|private|protected)\s+\w+)?/g;
      let match;
      
      while ((match = classRegex.exec(content)) !== null) {
        const className = match[1];
        if (!className.startsWith('U') && !className.startsWith('A') && !className.startsWith('F')) {
          continue; // Skip non-Unreal classes for efficiency
        }
        
        // Queue class for analysis
        analyzeClass(className).catch(console.error);
      }
    }
  }
  
  // Create index file
  const indexData = {
    projectName: 'AtlantisEons',
    sourceFiles: sourceFiles,
    engineVersion: '5.5',
    indexed: new Date().toISOString(),
    classes: Array.from(classCache.keys()),
    headerFiles: sourceFiles.filter(file => file.endsWith('.h')),
    implementationFiles: sourceFiles.filter(file => file.endsWith('.cpp'))
  };

  // Save index data
  const indexPath = path.join(projectRoot, '.mcp-index.json');
  fs.writeFileSync(indexPath, JSON.stringify(indexData, null, 2));
  console.log(`Index saved to ${indexPath}`);
}

// Create HTTP server to handle MCP requests
function createServer() {
  const server = http.createServer((req, res) => {
    if (req.method === 'POST') {
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
              await initializeIndex();
              response = { success: true };
              break;
            
            case 'analyze_class':
              response = await analyzeClass(request.params.className);
              break;
            
            case 'find_class_hierarchy':
              response = await findClassHierarchy(request.params.className);
              break;
            
            case 'find_references':
              response = await findReferences(request.params.identifier);
              break;
            
            case 'search_code':
              response = await searchCode(request.params.query, request.params.filePattern);
              break;
            
            default:
              response = { error: 'Unknown method' };
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
      res.end(`${serverName} MCP server v${serverVersion} running`);
    }
  });
  
  return server;
}

// Main server function
async function startServer() {
  console.log(`Starting ${serverName} MCP server (v${serverVersion})...`);
  console.log(`Project root: ${projectRoot}`);
  
  // Initialize index
  await initializeIndex();
  
  // Start HTTP server
  const server = createServer();
  server.listen(serverPort, () => {
    console.log(`MCP server listening on port ${serverPort}`);
  });
  
  // Handle signals
  process.on('SIGINT', () => {
    console.log('Shutting down MCP server...');
    server.close();
    process.exit(0);
  });
}

// Start the server
startServer().catch(err => console.error('Error:', err));