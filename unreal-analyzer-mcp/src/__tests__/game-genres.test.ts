import { jest, describe, it, expect } from '@jest/globals';
import { GAME_GENRES, GameGenre, GenreFlag } from '../types/game-genres.js';

describe('Game Genres', () => {
  describe('Data Structure', () => {
    it('should have the main genre categories', () => {
      expect(GAME_GENRES).toHaveProperty('Action');
      expect(GAME_GENRES).toHaveProperty('RPG');
      expect(GAME_GENRES).toHaveProperty('Strategy');
    });

    it('should have valid genre structure', () => {
      Object.values(GAME_GENRES).forEach((genre: GameGenre) => {
        expect(genre).toHaveProperty('name');
        expect(genre).toHaveProperty('description');
        expect(genre).toHaveProperty('primaryFlags');
        expect(genre).toHaveProperty('optionalFlags');
        expect(genre).toHaveProperty('commonMechanics');
        expect(genre).toHaveProperty('designPatterns');
        expect(genre).toHaveProperty('technicalConsiderations');
        expect(genre).toHaveProperty('famousExamples');
        expect(genre).toHaveProperty('subgenres');
      });
    });

    it('should have valid flag structure', () => {
      Object.values(GAME_GENRES).forEach((genre: GameGenre) => {
        const allFlags = [...genre.primaryFlags, ...genre.optionalFlags];
        
        allFlags.forEach((flag: GenreFlag) => {
          expect(flag).toHaveProperty('name');
          expect(flag).toHaveProperty('description');
          expect(flag).toHaveProperty('impact');
          expect(flag.impact).toHaveProperty('gameplay');
          expect(flag.impact).toHaveProperty('design');
          expect(flag.impact).toHaveProperty('technical');
          expect(flag).toHaveProperty('unrealImplementation');
          expect(flag.unrealImplementation).toHaveProperty('requiredComponents');
          expect(flag.unrealImplementation).toHaveProperty('suggestedClasses');
          expect(flag.unrealImplementation).toHaveProperty('commonBlueprints');
          expect(flag.unrealImplementation).toHaveProperty('engineFeatures');
          expect(flag.unrealImplementation).toHaveProperty('performanceConsiderations');
          expect(flag).toHaveProperty('commonFeatures');
          expect(flag).toHaveProperty('examples');
        });
      });
    });
  });

  describe('Action Genre', () => {
    const action = GAME_GENRES['Action'];

    it('should have combat as a primary flag', () => {
      const combatFlag = action.primaryFlags.find(flag => flag.name === 'Combat');
      expect(combatFlag).toBeDefined();
      expect(combatFlag?.unrealImplementation.requiredComponents).toContain('UCapsuleComponent for character collision');
    });

    it('should have movement as a primary flag', () => {
      const movementFlag = action.primaryFlags.find(flag => flag.name === 'Movement');
      expect(movementFlag).toBeDefined();
      expect(movementFlag?.unrealImplementation.requiredComponents).toContain('UCharacterMovementComponent');
    });

    it('should have stealth as an optional flag', () => {
      const stealthFlag = action.optionalFlags.find(flag => flag.name === 'Stealth');
      expect(stealthFlag).toBeDefined();
      expect(stealthFlag?.unrealImplementation.requiredComponents).toContain('UAIPerceptionComponent');
    });
  });

  describe('RPG Genre', () => {
    const rpg = GAME_GENRES['RPG'];

    it('should have character progression as a primary flag', () => {
      const progressionFlag = rpg.primaryFlags.find(flag => flag.name === 'Character Progression');
      expect(progressionFlag).toBeDefined();
      expect(progressionFlag?.unrealImplementation.requiredComponents).toContain('UAttributeSet for stats');
    });

    it('should have inventory as a primary flag', () => {
      const inventoryFlag = rpg.primaryFlags.find(flag => flag.name === 'Inventory');
      expect(inventoryFlag).toBeDefined();
      expect(inventoryFlag?.unrealImplementation.requiredComponents).toContain('UInventoryComponent');
    });

    it('should have dialog as an optional flag', () => {
      const dialogFlag = rpg.optionalFlags.find(flag => flag.name === 'Dialog');
      expect(dialogFlag).toBeDefined();
      expect(dialogFlag?.unrealImplementation.requiredComponents).toContain('UDialogComponent');
    });
  });

  describe('Strategy Genre', () => {
    const strategy = GAME_GENRES['Strategy'];

    it('should have resource management as a primary flag', () => {
      const resourceFlag = strategy.primaryFlags.find(flag => flag.name === 'Resource Management');
      expect(resourceFlag).toBeDefined();
      expect(resourceFlag?.unrealImplementation.requiredComponents).toContain('UResourceComponent');
    });

    it('should have unit control as a primary flag', () => {
      const unitFlag = strategy.primaryFlags.find(flag => flag.name === 'Unit Control');
      expect(unitFlag).toBeDefined();
      expect(unitFlag?.unrealImplementation.requiredComponents).toContain('USelectionComponent');
    });

    it('should have base building as an optional flag', () => {
      const buildingFlag = strategy.optionalFlags.find(flag => flag.name === 'Base Building');
      expect(buildingFlag).toBeDefined();
      expect(buildingFlag?.unrealImplementation.requiredComponents).toContain('UGridComponent');
    });
  });

  describe('Data Validation', () => {
    it('should have non-empty arrays for all array properties', () => {
      Object.values(GAME_GENRES).forEach((genre: GameGenre) => {
        expect(genre.primaryFlags.length).toBeGreaterThan(0);
        expect(genre.commonMechanics.length).toBeGreaterThan(0);
        expect(genre.designPatterns.length).toBeGreaterThan(0);
        expect(genre.technicalConsiderations.length).toBeGreaterThan(0);
        expect(genre.famousExamples.length).toBeGreaterThan(0);
        expect(genre.subgenres.length).toBeGreaterThan(0);
      });
    });

    it('should have valid Unreal Engine component names', () => {
      const componentPattern = /^U[A-Z][a-zA-Z]*(?:Component)?$/;
      
      Object.values(GAME_GENRES).forEach((genre: GameGenre) => {
        const allFlags = [...genre.primaryFlags, ...genre.optionalFlags];
        
        allFlags.forEach((flag: GenreFlag) => {
          flag.unrealImplementation.requiredComponents.forEach(component => {
            const componentName = component.split(' ')[0];
            if (componentName.startsWith('U')) {
              expect(componentName).toMatch(componentPattern);
            }
          });
        });
      });
    });

    it('should have unique examples for each genre', () => {
      const allExamples = new Set<string>();
      
      Object.values(GAME_GENRES).forEach((genre: GameGenre) => {
        genre.famousExamples.forEach(example => {
          expect(allExamples.has(example)).toBeFalsy();
          allExamples.add(example);
        });
      });
    });
  });
});
