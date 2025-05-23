#!/usr/bin/env node

// A simple MCP server implementation for Unreal Engine project analysis
const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

// Configuration
const projectRoot = '/Users/danielvargas/Documents/Unreal Projects/AtlantisEons';
const serverName = 'unreal-analyzer';
const serverVersion = '0.1.0';

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

// Main server function
async function startServer() {
  console.log(`Starting ${serverName} MCP server (v${serverVersion})...`);
  console.log(`Project root: ${projectRoot}`);
  
  // Index source files
  console.log('Indexing source files...');
  const sourceDir = path.join(projectRoot, 'Source');
  const sourceFiles = await scanDirectory(sourceDir);
  console.log(`Found ${sourceFiles.length} source files`);

  // Create index file
  const indexData = {
    projectName: 'AtlantisEons',
    sourceFiles: sourceFiles,
    engineVersion: '5.5',
    indexed: new Date().toISOString(),
    classes: [],
    headerFiles: sourceFiles.filter(file => file.endsWith('.h')),
    implementationFiles: sourceFiles.filter(file => file.endsWith('.cpp')),
  };

  // Save index data
  const indexPath = path.join(projectRoot, '.mcp-index.json');
  fs.writeFileSync(indexPath, JSON.stringify(indexData, null, 2));
  console.log(`Index saved to ${indexPath}`);
  
  console.log('MCP server ready!');
}

// Start the server
startServer().catch(err => console.error('Error:', err));