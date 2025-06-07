import { jest, describe, it, expect, beforeEach } from '@jest/globals';
import { UnrealCodeAnalyzer } from '../analyzer.js';
import * as fs from 'fs';
import * as path from 'path';
import { mockTreeSitter, mockCppBindings, mockGlob } from './setup.js';

import { PathLike } from 'fs';

// Mock fs module
jest.mock('fs');

// Get the mocked fs module
const mockedFs = jest.mocked(fs, { shallow: false });

jest.mock('glob', () => ({
  sync: mockGlob.sync
}));

jest.mock('tree-sitter', () => {
  return jest.fn(() => mockTreeSitter);
});

jest.mock('tree-sitter-cpp', () => {
  return jest.fn(() => mockCppBindings);
});

jest.mock('tree-sitter-cpp/bindings/node', () => mockCppBindings);

describe('UnrealCodeAnalyzer', () => {
  let analyzer: UnrealCodeAnalyzer;

  beforeEach(() => {
    jest.clearAllMocks();
    // Reset mocks
    jest.clearAllMocks();
    
    // Setup fs mocks
    mockedFs.existsSync.mockImplementation((path: PathLike) => {
      const pathStr = path.toString();
      return !pathStr.includes('invalid');
    });
    mockedFs.readFileSync.mockReturnValue('');
    analyzer = new UnrealCodeAnalyzer();
  });

  describe('initialization', () => {
    it('should start uninitialized', () => {
      expect(analyzer.isInitialized()).toBe(false);
    });

    it('should initialize with valid Unreal Engine path', async () => {
      const mockPath = '/valid/path';
      mockedFs.existsSync.mockReturnValue(true);

      await analyzer.initialize(mockPath);
      expect(analyzer.isInitialized()).toBe(true);
    });

    it('should throw error with invalid Unreal Engine path', async () => {
      const mockPath = '/invalid/path';
      mockedFs.existsSync.mockReturnValue(false);

      await expect(analyzer.initialize(mockPath))
        .rejects
        .toThrow('Invalid Unreal Engine path: Directory does not exist');
    });

    it('should initialize with valid custom codebase path', async () => {
      const mockPath = '/valid/custom/path';
      mockedFs.existsSync.mockReturnValue(true);

      await analyzer.initializeCustomCodebase(mockPath);
      expect(analyzer.isInitialized()).toBe(true);
    });

    it('should throw error with invalid custom codebase path', async () => {
      const mockPath = '/invalid/custom/path';
      mockedFs.existsSync.mockReturnValue(false);

      await expect(analyzer.initializeCustomCodebase(mockPath))
        .rejects
        .toThrow('Invalid custom codebase path: Directory does not exist');
    });
  });

  describe('class analysis', () => {
    beforeEach(async () => {
      mockedFs.existsSync.mockReturnValue(true);
      await analyzer.initializeCustomCodebase('/mock/path');
    });

    it('should throw error when analyzing class without initialization', async () => {
      const uninitializedAnalyzer = new UnrealCodeAnalyzer();
      await expect(uninitializedAnalyzer.analyzeClass('TestClass'))
        .rejects
        .toThrow('Analyzer not initialized');
    });

    it('should analyze a class successfully', async () => {
      const mockFileContent = `
        class TestClass {
          public:
            void TestMethod();
            int TestProperty;
        };
      `;
      mockedFs.readFileSync.mockReturnValue(mockFileContent);

      const result = await analyzer.analyzeClass('TestClass');
      expect(result).toHaveProperty('name', 'TestClass');
      expect(result).toHaveProperty('methods');
      expect(result).toHaveProperty('properties');
    });
  });

  describe('reference finding', () => {
    beforeEach(async () => {
      mockedFs.existsSync.mockReturnValue(true);
      await analyzer.initializeCustomCodebase('/mock/path');
    });

    it('should find references to a class', async () => {
      const mockContent = 'TestClass instance;';
      mockedFs.readFileSync.mockReturnValue(mockContent);

      const references = await analyzer.findReferences('TestClass', 'class');
      expect(references).toBeInstanceOf(Array);
      expect(references.length).toBeGreaterThanOrEqual(0);
    });

    it('should throw error when finding references without initialization', async () => {
      const uninitializedAnalyzer = new UnrealCodeAnalyzer();
      await expect(uninitializedAnalyzer.findReferences('TestClass'))
        .rejects
        .toThrow('Analyzer not initialized');
    });
  });

  describe('code search', () => {
    beforeEach(async () => {
      mockedFs.existsSync.mockReturnValue(true);
      await analyzer.initializeCustomCodebase('/mock/path');
    });

    it('should search code with query', async () => {
      const mockContent = 'void TestFunction() { }';
      mockedFs.readFileSync.mockReturnValue(mockContent);

      const results = await analyzer.searchCode('TestFunction');
      expect(results).toBeInstanceOf(Array);
      expect(results.length).toBeGreaterThanOrEqual(0);
    });

    it('should respect file pattern in search', async () => {
      const mockContent = 'test content';
      mockedFs.readFileSync.mockReturnValue(mockContent);

      const results = await analyzer.searchCode('test', '*.cpp');
      expect(results).toBeInstanceOf(Array);
    });

    it('should handle comment inclusion setting', async () => {
      const mockContent = '// Test comment\ncode';
      mockedFs.readFileSync.mockReturnValue(mockContent);

      const resultsWithComments = await analyzer.searchCode('Test', '*.cpp', true);
      const resultsWithoutComments = await analyzer.searchCode('Test', '*.cpp', false);

      expect(resultsWithComments.length).toBeGreaterThanOrEqual(resultsWithoutComments.length);
    });
  });

  describe('subsystem analysis', () => {
    beforeEach(async () => {
      mockedFs.existsSync.mockReturnValue(true);
      await analyzer.initialize('/mock/unreal/path');
    });

    it('should analyze a valid subsystem', async () => {
      const mockContent = 'class RenderingClass { };';
      mockedFs.readFileSync.mockReturnValue(mockContent);

      const result = await analyzer.analyzeSubsystem('Rendering');
      expect(result).toHaveProperty('name', 'Rendering');
      expect(result).toHaveProperty('mainClasses');
      expect(result).toHaveProperty('sourceFiles');
    });

    it('should throw error for unknown subsystem', async () => {
      await expect(analyzer.analyzeSubsystem('InvalidSubsystem'))
        .rejects
        .toThrow('Unknown subsystem: InvalidSubsystem');
    });
  });
});
