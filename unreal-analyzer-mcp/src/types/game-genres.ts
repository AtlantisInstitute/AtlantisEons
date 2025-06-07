export interface GenreFlag {
  name: string;
  description: string;
  impact: {
    gameplay: string[];
    design: string[];
    technical: string[];
  };
  unrealImplementation: {
    requiredComponents: string[];
    suggestedClasses: string[];
    commonBlueprints: string[];
    engineFeatures: string[];
    performanceConsiderations: string[];
  };
  commonFeatures: string[];
  examples: string[];
}

export interface GameGenre {
  name: string;
  description: string;
  primaryFlags: GenreFlag[];
  optionalFlags: GenreFlag[];
  commonMechanics: string[];
  designPatterns: string[];
  technicalConsiderations: string[];
  famousExamples: string[];
  subgenres: string[];
}

// Core game genres with their flags and characteristics
export const GAME_GENRES: { [key: string]: GameGenre } = {
  "Action": {
    name: "Action",
    description: "Games focused on physical challenges, including hand-eye coordination and reaction time",
    primaryFlags: [
      {
        name: "Combat",
        description: "Real-time conflict resolution through physical or projectile-based combat",
        impact: {
          gameplay: ["Fast-paced interactions", "Skill-based mechanics", "Immediate feedback"],
          design: ["Clear hit feedback", "Enemy variety", "Combat pacing"],
          technical: ["Physics calculations", "Collision detection", "Animation blending"]
        },
        unrealImplementation: {
          requiredComponents: [
            "UCapsuleComponent for character collision",
            "USkeletalMeshComponent for character model",
            "UCharacterMovementComponent for movement",
            "UAnimInstance for animation control"
          ],
          suggestedClasses: [
            "ACharacter as base class for player and enemies",
            "UGameplayAbility for combat abilities",
            "UDamageType for different attack types",
            "UAnimNotify for combat events in animations"
          ],
          commonBlueprints: [
            "BP_CombatComponent for managing combat state",
            "BP_WeaponBase for weapon functionality",
            "BP_DamageCalculator for damage logic",
            "BT_EnemyAI for enemy behavior"
          ],
          engineFeatures: [
            "Physics simulation for impacts",
            "Animation Blueprint for combat moves",
            "Gameplay Ability System for powers",
            "Niagara for combat effects"
          ],
          performanceConsiderations: [
            "Batch animation updates",
            "Optimize collision checks",
            "LOD for combat effects",
            "Network optimization for multiplayer"
          ]
        },
        commonFeatures: ["Weapon systems", "Health management", "Combo systems"],
        examples: ["God of War", "Devil May Cry"]
      },
      {
        name: "Movement",
        description: "Complex character movement and traversal mechanics",
        impact: {
          gameplay: ["Platform navigation", "Dodge mechanics", "Movement abilities"],
          design: ["Level layout", "Movement flow", "Environmental hazards"],
          technical: ["Character controller", "Physics integration", "Animation systems"]
        },
        unrealImplementation: {
          requiredComponents: [
            "UCharacterMovementComponent",
            "UCapsuleComponent",
            "USkeletalMeshComponent"
          ],
          suggestedClasses: [
            "ACharacter for player control",
            "UCharacterMovementComponent custom implementation",
            "UAnimInstance for movement animations"
          ],
          commonBlueprints: [
            "BP_MovementComponent for custom movement",
            "BP_ClimbingSystem for traversal",
            "BP_ParkourSystem for advanced movement",
            "ANS_MovementTransitions for smooth blending"
          ],
          engineFeatures: [
            "Character Movement Component",
            "Physics simulation",
            "Animation Blueprint system",
            "Navigation system"
          ],
          performanceConsiderations: [
            "Movement prediction for multiplayer",
            "Animation compression settings",
            "Physics simulation complexity",
            "Navigation mesh optimization"
          ]
        },
        commonFeatures: ["Jump mechanics", "Sprint abilities", "Dodge rolls"],
        examples: ["Mirror's Edge", "Assassin's Creed"]
      }
    ],
    optionalFlags: [
      {
        name: "Stealth",
        description: "Mechanics for avoiding detection and strategic approach",
        impact: {
          gameplay: ["Enemy awareness systems", "Silent takedowns", "Cover mechanics"],
          design: ["AI patrol patterns", "Light/shadow systems", "Sound propagation"],
          technical: ["AI perception", "Visibility calculations", "Sound detection"]
        },
        unrealImplementation: {
          requiredComponents: [
            "UAIPerceptionComponent",
            "UPawnSensingComponent",
            "UAudioComponent"
          ],
          suggestedClasses: [
            "APerceptionManager for AI awareness",
            "UStealthComponent for stealth mechanics",
            "UCoverComponent for cover system"
          ],
          commonBlueprints: [
            "BP_AIPerceptionManager",
            "BP_StealthMechanics",
            "BP_CoverSystem",
            "BT_StealthAI"
          ],
          engineFeatures: [
            "AI Perception System",
            "Navigation System",
            "Audio System",
            "Environment Query System"
          ],
          performanceConsiderations: [
            "Perception update frequency",
            "Line of sight calculations",
            "Sound propagation complexity",
            "AI decision making optimization"
          ]
        },
        commonFeatures: ["Visibility indicators", "Distraction mechanics", "Silent weapons"],
        examples: ["Metal Gear Solid", "Splinter Cell"]
      }
    ],
    commonMechanics: ["Health systems", "Power-ups", "Checkpoints"],
    designPatterns: ["Progressive difficulty", "Combat arenas", "Tutorial sequences"],
    technicalConsiderations: ["Input responsiveness", "Frame-perfect timing", "Animation canceling"],
    famousExamples: ["Super Mario Bros", "Mega Man", "Bayonetta"],
    subgenres: ["Beat 'em up", "Platformer", "Shooter"]
  },
  "RPG": {
    name: "RPG",
    description: "Games focusing on character development, storytelling, and strategic decision-making",
    primaryFlags: [
      {
        name: "Character Progression",
        description: "Systems for developing and customizing characters over time",
        impact: {
          gameplay: ["Level-up systems", "Skill trees", "Attribute points"],
          design: ["Power curve", "Build variety", "Progression pacing"],
          technical: ["Save systems", "Stat calculations", "Character persistence"]
        },
        unrealImplementation: {
          requiredComponents: [
            "UAttributeSet for stats",
            "UAbilitySystemComponent",
            "USaveGame for progression"
          ],
          suggestedClasses: [
            "UProgressionComponent",
            "UAttributeSet custom implementation",
            "UGameplayEffect for modifiers"
          ],
          commonBlueprints: [
            "BP_ProgressionManager",
            "BP_SkillTree",
            "BP_AttributeCalculator",
            "BP_ExperienceManager"
          ],
          engineFeatures: [
            "Gameplay Ability System",
            "Save Game System",
            "Data Tables for progression data",
            "Blueprint Interface System"
          ],
          performanceConsiderations: [
            "Attribute calculation batching",
            "Save data optimization",
            "Skill tree UI performance",
            "Network replication of stats"
          ]
        },
        commonFeatures: ["Experience points", "Character classes", "Skill unlocks"],
        examples: ["Final Fantasy", "Diablo"]
      },
      {
        name: "Inventory",
        description: "Systems for managing items, equipment, and resources",
        impact: {
          gameplay: ["Item collection", "Equipment management", "Resource planning"],
          design: ["Loot tables", "Item balance", "Storage limitations"],
          technical: ["Database management", "UI systems", "Item persistence"]
        },
        unrealImplementation: {
          requiredComponents: [
            "UInventoryComponent",
            "UGameplayTagsComponent",
            "USaveGame for items"
          ],
          suggestedClasses: [
            "AItem base class",
            "UItemStack for stackable items",
            "UEquipmentSlot for equipment"
          ],
          commonBlueprints: [
            "BP_InventoryManager",
            "BP_ItemBase",
            "BP_LootGenerator",
            "WBP_InventoryUI"
          ],
          engineFeatures: [
            "UMG for inventory UI",
            "Gameplay Tags",
            "Data Tables for items",
            "Save Game System"
          ],
          performanceConsiderations: [
            "Inventory sorting optimization",
            "Item replication for multiplayer",
            "UI widget pooling",
            "Item data caching"
          ]
        },
        commonFeatures: ["Equipment slots", "Item categories", "Storage systems"],
        examples: ["World of Warcraft", "Path of Exile"]
      }
    ],
    optionalFlags: [
      {
        name: "Dialog",
        description: "Branching conversation systems and narrative choices",
        impact: {
          gameplay: ["Choice consequences", "Relationship systems", "Quest branching"],
          design: ["Dialog trees", "Character relationships", "Story branches"],
          technical: ["Dialog systems", "Quest tracking", "Choice persistence"]
        },
        unrealImplementation: {
          requiredComponents: [
            "UDialogComponent",
            "UQuestComponent",
            "USaveGame for choices"
          ],
          suggestedClasses: [
            "UDialogManager",
            "UQuestManager",
            "URelationshipComponent"
          ],
          commonBlueprints: [
            "BP_DialogSystem",
            "BP_QuestManager",
            "BP_RelationshipTracker",
            "WBP_DialogUI"
          ],
          engineFeatures: [
            "UMG for dialog UI",
            "Data Tables for dialog",
            "Save Game System",
            "Localization System"
          ],
          performanceConsiderations: [
            "Dialog tree memory management",
            "Choice history optimization",
            "UI transition smoothness",
            "Voice line streaming"
          ]
        },
        commonFeatures: ["Dialog options", "Character relationships", "Quest logs"],
        examples: ["Mass Effect", "Dragon Age"]
      }
    ],
    commonMechanics: ["Questing", "Character customization", "Party management"],
    designPatterns: ["Hub areas", "Side quests", "Boss encounters"],
    technicalConsiderations: ["Save/load systems", "Character persistence", "Quest tracking"],
    famousExamples: ["The Elder Scrolls", "Dragon Quest", "Dark Souls"],
    subgenres: ["Action RPG", "JRPG", "Tactical RPG"]
  },
  "Strategy": {
    name: "Strategy",
    description: "Games emphasizing tactical thinking and resource management",
    primaryFlags: [
      {
        name: "Resource Management",
        description: "Systems for collecting and managing various resources",
        impact: {
          gameplay: ["Resource gathering", "Economy management", "Production chains"],
          design: ["Resource balance", "Economic progression", "Scarcity mechanics"],
          technical: ["Economy simulation", "Resource tracking", "Production queues"]
        },
        unrealImplementation: {
          requiredComponents: [
            "UResourceComponent",
            "UProductionComponent",
            "UEconomyComponent"
          ],
          suggestedClasses: [
            "AResourceManager",
            "UResourceType",
            "UProductionQueue"
          ],
          commonBlueprints: [
            "BP_ResourceSystem",
            "BP_ProductionManager",
            "BP_EconomySimulator",
            "WBP_ResourceUI"
          ],
          engineFeatures: [
            "Data Tables for resources",
            "Timer Manager for production",
            "Replication Graph for multiplayer",
            "UMG for resource UI"
          ],
          performanceConsiderations: [
            "Resource update batching",
            "Production queue optimization",
            "Economic simulation throttling",
            "UI update frequency"
          ]
        },
        commonFeatures: ["Resource types", "Gathering mechanics", "Storage systems"],
        examples: ["Age of Empires", "StarCraft"]
      },
      {
        name: "Unit Control",
        description: "Systems for controlling and coordinating multiple units",
        impact: {
          gameplay: ["Unit selection", "Formation control", "Combat tactics"],
          design: ["Unit balance", "Counter systems", "Pathfinding"],
          technical: ["Unit AI", "Pathfinding systems", "Formation logic"]
        },
        unrealImplementation: {
          requiredComponents: [
            "USelectionComponent",
            "UFormationComponent",
            "UAIController"
          ],
          suggestedClasses: [
            "AUnitBase",
            "UFormationManager",
            "UUnitController"
          ],
          commonBlueprints: [
            "BP_UnitManager",
            "BP_FormationHandler",
            "BP_SelectionSystem",
            "BT_UnitAI"
          ],
          engineFeatures: [
            "Navigation System",
            "AI Controller System",
            "Behavior Tree System",
            "Environment Query System"
          ],
          performanceConsiderations: [
            "Unit selection optimization",
            "Formation calculation batching",
            "Pathfinding optimization",
            "AI decision making frequency"
          ]
        },
        commonFeatures: ["Unit groups", "Formation controls", "Command queuing"],
        examples: ["Command & Conquer", "Total War"]
      }
    ],
    optionalFlags: [
      {
        name: "Base Building",
        description: "Systems for constructing and managing bases or cities",
        impact: {
          gameplay: ["Construction mechanics", "Base defense", "Tech progression"],
          design: ["Building placement", "Tech trees", "Defense layouts"],
          technical: ["Construction systems", "Building grids", "Tech progression"]
        },
        unrealImplementation: {
          requiredComponents: [
            "UGridComponent",
            "UConstructionComponent",
            "UBuildingComponent"
          ],
          suggestedClasses: [
            "ABuildingManager",
            "UConstructionValidator",
            "UBuildingPlacement"
          ],
          commonBlueprints: [
            "BP_BuildingSystem",
            "BP_ConstructionManager",
            "BP_GridManager",
            "WBP_BuildMenu"
          ],
          engineFeatures: [
            "Procedural Mesh Component",
            "Navigation System",
            "Physics System",
            "UMG for building UI"
          ],
          performanceConsiderations: [
            "Grid system optimization",
            "Building mesh instancing",
            "Construction validation caching",
            "Tech tree computation"
          ]
        },
        commonFeatures: ["Building types", "Construction queues", "Tech trees"],
        examples: ["SimCity", "Factorio"]
      }
    ],
    commonMechanics: ["Resource gathering", "Unit production", "Tech progression"],
    designPatterns: ["Tech trees", "Counter systems", "Fog of war"],
    technicalConsiderations: ["Pathfinding", "AI behavior", "Economy simulation"],
    famousExamples: ["Civilization", "Warcraft", "XCOM"],
    subgenres: ["RTS", "4X", "Tower Defense"]
  }
};
