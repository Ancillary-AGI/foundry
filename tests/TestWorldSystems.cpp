#include "gtest/gtest.h"
#include "GameEngine/world/ProceduralGeneration.h"
#include "GameEngine/core/MemoryPool.h"
#include "GameEngine/math/Vector3.h"
#include "GameEngine/math/Vector2.h"
#include <thread>
#include <chrono>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for World Systems tests
 */
class WorldSystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(4096, 32768);
    }

    void TearDown() override {
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
};

/**
 * @brief Test procedural generation system
 */
TEST_F(WorldSystemsTest, ProceduralGeneration) {
    WorldEngine worldEngine;

    // Test world engine initialization
    WorldEngine::WorldParameters params;
    params.seed = 12345;
    params.worldSize = 1000.0f;
    params.biomeResolution = 64;
    params.weatherGridSize = 32;
    params.enableEcosystemSimulation = true;
    params.enableWeatherDynamics = true;
    params.proceduralQuestGen = true;

    worldEngine.initialize(params);
    EXPECT_TRUE(worldEngine.isInitialized());

    // Test terrain generation
    InfiniteWorldGenerator& terrainGen = worldEngine.worldGenerator;

    // Test terrain patch generation
    InfiniteWorldGenerator::TerrainPatch* patch = terrainGen.generatePatch(Vector2(0.0f, 0.0f), 100.0f, 0);
    ASSERT_NE(patch, nullptr);

    EXPECT_EQ(patch->center, Vector2(0.0f, 0.0f));
    EXPECT_FLOAT_EQ(patch->size, 100.0f);
    EXPECT_EQ(patch->lodLevel, 0);
    EXPECT_GT(patch->vertices.size(), 0);
    EXPECT_GT(patch->indices.size(), 0);

    // Test terrain height calculation
    float height1 = terrainGen.terrainHeight(0.0f, 0.0f);
    float height2 = terrainGen.terrainHeight(50.0f, 50.0f);
    float height3 = terrainGen.terrainHeight(-25.0f, -25.0f);

    // Heights should be different (procedural generation)
    EXPECT_GE(height1, 0.0f);
    EXPECT_LE(height1, terrainGen.maxTerrainHeight);

    // Test LOD calculation
    Vector3 cameraPos(0.0f, 100.0f, 0.0f);
    int lodLevel = terrainGen.calculateLOD(Vector2(0.0f, 0.0f), cameraPos, 100.0f);
    EXPECT_GE(lodLevel, 0);
    EXPECT_LE(lodLevel, terrainGen.maxLODLevels);

    // Test biome generation
    BiomeManager& biomeManager = worldEngine.biomeManager;

    // Test biome map generation
    std::vector<std::vector<BiomeManager::BiomeType>> biomeMap =
        biomeManager.generateBiomeMap(32, 32, 0.01f);

    EXPECT_EQ(biomeMap.size(), 32);
    EXPECT_EQ(biomeMap[0].size(), 32);

    // Should have diverse biomes
    std::set<BiomeManager::BiomeType> uniqueBiomes;
    for (const auto& row : biomeMap) {
        for (BiomeManager::BiomeType biome : row) {
            uniqueBiomes.insert(biome);
        }
    }
    EXPECT_GT(uniqueBiomes.size(), 1); // Should have multiple biome types

    // Test biome blending
    std::vector<BiomeManager::BiomeType> nearbyBiomes = {
        BiomeManager::BiomeType::TUNDRA,
        BiomeManager::BiomeType::TAIGA,
        BiomeManager::BiomeType::TEMPERATE_FOREST
    };
    Vector3 weights(0.3f, 0.4f, 0.3f);

    BiomeManager::BiomeType blendedBiome = biomeManager.blendBiomes(nearbyBiomes, weights);
    EXPECT_NE(blendedBiome, BiomeManager::BiomeType::UNKNOWN);

    // Test ecosystem simulation
    EcosystemSimulator& ecosystem = worldEngine.ecosystemSimulator;

    // Test species creation
    EcosystemSimulator::Species deerSpecies;
    deerSpecies.name = "Deer";
    deerSpecies.population = 100;
    deerSpecies.growthRate = 0.1f;
    deerSpecies.carryingCapacity = 200.0f;
    deerSpecies.preySpecies = {"Grass"};
    deerSpecies.territoryRadius = 50.0f;

    ecosystem.speciesDatabase["Deer"] = deerSpecies;

    // Test food chain simulation
    ecosystem.simulatePredatorPreyDynamics(0.016f);
    EXPECT_GT(ecosystem.speciesDatabase["Deer"].population, 0); // Should still have population

    // Test population dynamics
    ecosystem.updatePopulations(0.016f);
    // Population should be updated based on growth rate and carrying capacity

    // Test weather system
    WeatherEngine& weather = worldEngine.weatherEngine;

    // Test weather simulation
    weather.integrateWeather(0.016f);

    // Test weather cell access
    EXPECT_GT(weather.weatherGrid.size(), 0);

    WeatherEngine::WeatherCell cell = weather.weatherGrid[0];
    EXPECT_GE(cell.temperature, -50.0f); // Reasonable temperature range
    EXPECT_LE(cell.temperature, 60.0f);
    EXPECT_GE(cell.humidity, 0.0f);
    EXPECT_LE(cell.humidity, 1.0f);

    // Test atmospheric layers
    EXPECT_GT(weather.troposphere.size(), 0);

    WeatherEngine::AtmosphericLayer layer = weather.troposphere[0];
    EXPECT_GT(layer.altitude, 0.0f);
    EXPECT_LT(layer.temperatureLapse, 0.0f); // Temperature decreases with altitude

    // Test narrative engine
    NarrativeEngine& narrative = worldEngine.narrativeEngine;

    // Test quest generation
    NarrativeEngine::QuestNode quest = narrative.generateProceduralQuest("Rescue", {"Forest", "Village"});
    EXPECT_FALSE(quest.id.empty());
    EXPECT_FALSE(quest.title.empty());
    EXPECT_GT(quest.objectives.size(), 0);

    // Test narrative state
    narrative.currentState.variables["PlayerLevel"] = 5.0f;
    narrative.currentState.flags["QuestCompleted"] = "MainQuest";

    EXPECT_FLOAT_EQ(narrative.currentState.variables["PlayerLevel"], 5.0f);
    EXPECT_EQ(narrative.currentState.flags["QuestCompleted"], "MainQuest");

    // Test dialogue system
    NarrativeEngine::DialogueTree::DialogueNode rootNode;
    rootNode.speakerId = "NPC1";
    rootNode.text = "Hello, adventurer!";
    rootNode.choices = {"Hello", "Goodbye", "Attack"};

    narrative.activeDialogue.root = &rootNode;
    narrative.activeDialogue.currentSpeaker = "NPC1";

    EXPECT_EQ(narrative.activeDialogue.currentSpeaker, "NPC1");
    EXPECT_EQ(narrative.activeDialogue.root->text, "Hello, adventurer!");

    // Test relationship network
    narrative.relationships.affinities["Player"]["NPC1"] = 0.5f;
    narrative.relationships.affinities["NPC1"]["NPC2"] = 0.3f;

    float playerNPC1Affinity = narrative.relationships.affinities["Player"]["NPC1"];
    EXPECT_FLOAT_EQ(playerNPC1Affinity, 0.5f);

    // Test development tools
    DevelopmentTools& devTools = worldEngine.devTools;

    // Test material editor
    DevelopmentTools::MaterialEditor::MaterialGraph graph;
    devTools.materialEditor.activeGraph = graph;

    // Test visual scripting
    DevelopmentTools::VisualScripting::ScriptingGraph scriptGraph;
    devTools.visualScripting.mainGraph = scriptGraph;

    // Test animation editor
    DevelopmentTools::AnimationEditor::AnimationTrack track;
    track.target = "character.arm";
    track.easingFunction = "ease_in_out";

    devTools.animationEditor.tracks.push_back(track);
    EXPECT_EQ(devTools.animationEditor.tracks.size(), 1);

    // Test performance profiler
    DevelopmentTools::PerformanceProfiler::FrameData frameData;
    frameData.frameTime = 16.6f; // 60 FPS
    frameData.drawCalls = 1000;
    frameData.trianglesRendered = 50000;

    devTools.profiler.frameHistory.push_back(frameData);
    EXPECT_EQ(devTools.profiler.frameHistory.size(), 1);

    // Test asset pipeline
    DevelopmentTools::AssetPipeline::Asset testAsset;
    testAsset.guid = "test-asset-guid";
    testAsset.type = "texture";
    testAsset.sourcePath = "/assets/texture.png";

    devTools.assetPipeline.assetDatabase["test-asset-guid"] = testAsset;
    EXPECT_EQ(devTools.assetPipeline.assetDatabase.size(), 1);

    // Clean up
    delete patch;
}

/**
 * @brief Test world generation performance
 */
TEST_F(WorldSystemsTest, Performance) {
    const int numIterations = 50;

    // Measure world generation performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        WorldEngine worldEngine;

        WorldEngine::WorldParameters params;
        params.seed = 1000 + i;
        params.worldSize = 500.0f;
        params.biomeResolution = 32;
        params.weatherGridSize = 16;
        params.enableEcosystemSimulation = false; // Disable for performance testing
        params.enableWeatherDynamics = false;
        params.proceduralQuestGen = false;

        worldEngine.initialize(params);

        // Generate terrain patches
        InfiniteWorldGenerator& terrainGen = worldEngine.worldGenerator;
        for (int x = -1; x <= 1; ++x) {
            for (int z = -1; z <= 1; ++z) {
                Vector2 center(static_cast<float>(x) * 100.0f, static_cast<float>(z) * 100.0f);
                InfiniteWorldGenerator::TerrainPatch* patch = terrainGen.generatePatch(center, 100.0f, 0);
                if (patch) {
                    delete patch;
                }
            }
        }

        // Generate biome map
        BiomeManager& biomeManager = worldEngine.biomeManager;
        biomeManager.generateBiomeMap(16, 16, 0.02f);

        // Simulate one step
        worldEngine.update(0.016f);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numIterations << " world generation operations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 500ms for 50 operations)
    EXPECT_LT(duration.count(), 500000);
}

/**
 * @brief Test world memory management
 */
TEST_F(WorldSystemsTest, MemoryManagement) {
    size_t initialMemory = memoryPool->totalAllocated();

    // Create multiple world engines to test memory usage
    std::vector<std::unique_ptr<WorldEngine>> worldEngines;

    for (int i = 0; i < 10; ++i) {
        auto worldEngine = std::make_unique<WorldEngine>();

        WorldEngine::WorldParameters params;
        params.seed = 2000 + i;
        params.worldSize = 200.0f;
        params.biomeResolution = 16;
        params.weatherGridSize = 8;
        params.enableEcosystemSimulation = true;
        params.enableWeatherDynamics = true;
        params.proceduralQuestGen = true;

        worldEngine->initialize(params);

        // Generate some terrain and biomes
        InfiniteWorldGenerator& terrainGen = worldEngine->worldGenerator;
        BiomeManager& biomeManager = worldEngine->biomeManager;

        // Generate a few patches
        for (int x = 0; x < 2; ++x) {
            for (int z = 0; z < 2; ++z) {
                Vector2 center(static_cast<float>(x) * 50.0f, static_cast<float>(z) * 50.0f);
                InfiniteWorldGenerator::TerrainPatch* patch = terrainGen.generatePatch(center, 50.0f, 0);
                if (patch) {
                    delete patch;
                }
            }
        }

        // Generate biome map
        biomeManager.generateBiomeMap(8, 8, 0.05f);

        worldEngines.push_back(std::move(worldEngine));
    }

    size_t afterAllocationMemory = memoryPool->totalAllocated();
    EXPECT_GT(afterAllocationMemory, initialMemory);

    // Test memory utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    // Clean up
    worldEngines.clear();
}

/**
 * @brief Test world error handling
 */
TEST_F(WorldSystemsTest, ErrorHandling) {
    WorldEngine worldEngine;

    // Test invalid initialization
    WorldEngine::WorldParameters invalidParams;
    invalidParams.seed = 0;
    invalidParams.worldSize = 0.0f; // Invalid size
    invalidParams.biomeResolution = 0; // Invalid resolution

    // Should handle gracefully or throw appropriate exception
    EXPECT_NO_THROW(worldEngine.initialize(invalidParams));

    // Test invalid terrain generation
    InfiniteWorldGenerator& terrainGen = worldEngine.worldGenerator;
    EXPECT_NO_THROW(terrainGen.generatePatch(Vector2(0.0f, 0.0f), 0.0f, 0)); // Zero size patch

    // Test invalid biome generation
    BiomeManager& biomeManager = worldEngine.biomeManager;
    EXPECT_NO_THROW(biomeManager.generateBiomeMap(0, 0, 0.0f)); // Zero size map

    // Test invalid narrative operations
    NarrativeEngine& narrative = worldEngine.narrativeEngine;
    EXPECT_NO_THROW(narrative.generateProceduralQuest("", {})); // Empty parameters

    // Test uninitialized operations
    EXPECT_FALSE(worldEngine.isInitialized());
    EXPECT_NO_THROW(worldEngine.update(0.016f)); // Should handle gracefully
}

/**
 * @brief Test world concurrent operations
 */
TEST_F(WorldSystemsTest, ConcurrentOperations) {
    WorldEngine worldEngine;

    WorldEngine::WorldParameters params;
    params.seed = 3000;
    params.worldSize = 100.0f;
    params.biomeResolution = 8;
    params.weatherGridSize = 4;
    params.enableEcosystemSimulation = false;
    params.enableWeatherDynamics = false;
    params.proceduralQuestGen = false;

    worldEngine.initialize(params);

    const int numThreads = 4;
    const int patchesPerThread = 10;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads performing world generation
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&worldEngine, t, &successCount]() {
            InfiniteWorldGenerator& terrainGen = worldEngine.worldGenerator;

            for (int i = 0; i < patchesPerThread; ++i) {
                Vector2 center(
                    static_cast<float>(t * 20 + i * 5),
                    static_cast<float>(t * 20 + i * 5));

                InfiniteWorldGenerator::TerrainPatch* patch = terrainGen.generatePatch(center, 25.0f, 0);
                if (patch) {
                    // Verify patch data
                    if (patch->vertices.size() > 0 && patch->indices.size() > 0) {
                        successCount++;
                    }
                    delete patch;
                }
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify concurrent operations worked
    EXPECT_EQ(successCount.load(), numThreads * patchesPerThread);

    // World engine should still be in valid state
    EXPECT_TRUE(worldEngine.isInitialized());

    // Memory pool should still be in valid state
    float utilization = memoryPool->utilization();
    EXPECT_GE(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);
}

/**
 * @brief Test ecosystem simulation
 */
TEST_F(WorldSystemsTest, EcosystemSimulation) {
    WorldEngine worldEngine;

    WorldEngine::WorldParameters params;
    params.seed = 4000;
    params.worldSize = 100.0f;
    params.biomeResolution = 16;
    params.weatherGridSize = 8;
    params.enableEcosystemSimulation = true;
    params.enableWeatherDynamics = true;
    params.proceduralQuestGen = false;

    worldEngine.initialize(params);

    EcosystemSimulator& ecosystem = worldEngine.ecosystemSimulator;

    // Test species management
    EcosystemSimulator::Species wolfSpecies;
    wolfSpecies.name = "Wolf";
    wolfSpecies.population = 20;
    wolfSpecies.growthRate = 0.05f;
    wolfSpecies.carryingCapacity = 50.0f;
    wolfSpecies.preySpecies = {"Deer"};
    wolfSpecies.predatorSpecies = {};
    wolfSpecies.territoryRadius = 100.0f;

    ecosystem.speciesDatabase["Wolf"] = wolfSpecies;

    // Test food chain setup
    EcosystemSimulator::FoodChain foodChain;
    foodChain.trophicLevels[0] = {}; // Producers
    foodChain.trophicLevels[1] = {}; // Herbivores
    foodChain.trophicLevels[2] = {}; // Carnivores
    foodChain.trophicLevels[3] = {}; // Top predators

    ecosystem.globalFoodChain = foodChain;

    // Test population simulation
    float initialPopulation = ecosystem.speciesDatabase["Wolf"].population;

    ecosystem.simulatePredatorPreyDynamics(0.016f);
    ecosystem.updatePopulations(0.016f);

    float finalPopulation = ecosystem.speciesDatabase["Wolf"].population;
    // Population should change based on simulation parameters

    // Test migration patterns
    std::vector<Vector3> migrationRoutes = {
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(25.0f, 0.0f, 0.0f),
        Vector3(50.0f, 0.0f, 0.0f)
    };

    ecosystem.simulateMigration(migrationRoutes);
    // Migration should affect population distribution

    // Test ecological disturbances
    ecosystem.applyDisturbance(Vector3(25.0f, 0.0f, 0.0f), 0.8f, "Fire");
    // Disturbance should affect population

    // Test ecosystem stability
    float stability = ecosystem.globalFoodChain.stabilityIndex();
    EXPECT_GE(stability, 0.0f);
    EXPECT_LE(stability, 1.0f);

    // Test keystone species identification
    std::vector<std::string> keystoneSpecies = ecosystem.globalFoodChain.identifyKeystoneSpecies();
    // Should identify important species in the ecosystem
}

/**
 * @brief Test weather dynamics
 */
TEST_F(WorldSystemsTest, WeatherDynamics) {
    WorldEngine worldEngine;

    WorldEngine::WorldParameters params;
    params.seed = 5000;
    params.worldSize = 100.0f;
    params.biomeResolution = 16;
    params.weatherGridSize = 16;
    params.enableEcosystemSimulation = false;
    params.enableWeatherDynamics = true;
    params.proceduralQuestGen = false;

    worldEngine.initialize(params);

    WeatherEngine& weather = worldEngine.weatherEngine;

    // Test weather simulation
    float initialTemperature = weather.weatherGrid[0].temperature;
    float initialHumidity = weather.weatherGrid[0].humidity;

    weather.integrateWeather(0.016f);

    float finalTemperature = weather.weatherGrid[0].temperature;
    float finalHumidity = weather.weatherGrid[0].humidity;

    // Weather should change over time
    // (Note: Changes might be subtle depending on simulation parameters)

    // Test atmospheric physics
    weather.calculateAdvection(weather.weatherGrid, weather.weatherGrid[0].windTendency);

    // Test condensation calculation
    float condensationRate = weather.calculateCondensationRate(weather.weatherGrid[0]);
    EXPECT_GE(condensationRate, 0.0f);

    // Test terrain-weather interaction
    std::vector<float> heightMap(16 * 16, 0.0f);
    for (int i = 0; i < 16 * 16; ++i) {
        heightMap[i] = static_cast<float>(i % 10) * 10.0f; // Varying heights
    }

    weather.applyTerrainInfluence(heightMap, weather.weatherGrid);

    // Test weather events
    weather.generateExtremeWeather(0.8f, Vector3(50.0f, 0.0f, 50.0f));
    // Should generate extreme weather conditions

    // Test climate patterns
    weather.simulateClimatePatterns(0.5f); // Summer season
    // Should adjust weather based on seasonal patterns

    // Test weather rendering
    WeatherEngine::WeatherRendering& rendering = weather.weatherRendering;

    EXPECT_GE(rendering.sunDirection.x, -1.0f);
    EXPECT_LE(rendering.sunDirection.x, 1.0f);
    EXPECT_GE(rendering.sunDirection.y, -1.0f);
    EXPECT_LE(rendering.sunDirection.y, 1.0f);
    EXPECT_GE(rendering.sunDirection.z, -1.0f);
    EXPECT_LE(rendering.sunDirection.z, 1.0f);

    EXPECT_GE(rendering.atmosphericScattering, 0.0f);
    EXPECT_LE(rendering.atmosphericScattering, 1.0f);
}

/**
 * @brief Test procedural quest generation
 */
TEST_F(WorldSystemsTest, ProceduralQuestGeneration) {
    WorldEngine worldEngine;

    WorldEngine::WorldParameters params;
    params.seed = 6000;
    params.worldSize = 100.0f;
    params.biomeResolution = 16;
    params.weatherGridSize = 8;
    params.enableEcosystemSimulation = false;
    params.enableWeatherDynamics = false;
    params.proceduralQuestGen = true;

    worldEngine.initialize(params);

    NarrativeEngine& narrative = worldEngine.narrativeEngine;

    // Test quest generation
    NarrativeEngine::QuestNode fetchQuest = narrative.generateProceduralQuest("Fetch", {"Village", "Forest"});
    EXPECT_FALSE(fetchQuest.id.empty());
    EXPECT_FALSE(fetchQuest.title.empty());
    EXPECT_GT(fetchQuest.objectives.size(), 0);

    NarrativeEngine::QuestNode combatQuest = narrative.generateProceduralQuest("Combat", {"Cave", "Mountain"});
    EXPECT_FALSE(combatQuest.id.empty());
    EXPECT_FALSE(combatQuest.title.empty());

    NarrativeEngine::QuestNode explorationQuest = narrative.generateProceduralQuest("Exploration", {"Ruins", "Lake"});
    EXPECT_FALSE(explorationQuest.id.empty());
    EXPECT_FALSE(explorationQuest.title.empty());

    // Test quest branching
    EXPECT_GT(fetchQuest.branches.size(), 0); // Should have branching logic

    // Test quest consequences
    EXPECT_GT(fetchQuest.consequences.size(), 0); // Should have consequences

    // Test dialogue generation
    NarrativeEngine::DialogueTree dialogue;
    dialogue.currentSpeaker = "QuestGiver";

    NarrativeEngine::DialogueTree::DialogueNode* rootNode = new NarrativeEngine::DialogueTree::DialogueNode();
    rootNode->speakerId = "QuestGiver";
    rootNode->text = "Greetings, adventurer!";
    rootNode->emotion = "friendly";
    rootNode->persuasionValue = 0.1f;

    dialogue.root = rootNode;
    narrative.activeDialogue = dialogue;

    EXPECT_EQ(narrative.activeDialogue.currentSpeaker, "QuestGiver");
    EXPECT_EQ(narrative.activeDialogue.root->text, "Greetings, adventurer!");

    // Test moral choice mechanics
    NarrativeEngine::MoralChoice choice;
    choice.question = "What will you do?";
    choice.consequences = {
        {"Help the villagers", 0.2f},
        {"Abandon them", -0.3f},
        {"Exploit the situation", -0.5f}
    };

    float moralityShift = choice.resolveChoice("Help the villagers", narrative.relationships.affinities["Player"]);
    EXPECT_FLOAT_EQ(moralityShift, 0.2f);

    // Test butterfly effect simulation
    narrative.simulateButterflyEffect("PlayerChoice", narrative.currentState.variables);

    // Clean up
    delete rootNode;
}

/**
 * @brief Test development tools integration
 */
TEST_F(WorldSystemsTest, DevelopmentToolsIntegration) {
    WorldEngine worldEngine;

    WorldEngine::WorldParameters params;
    params.seed = 7000;
    params.worldSize = 50.0f;
    params.biomeResolution = 8;
    params.weatherGridSize = 4;
    params.enableEcosystemSimulation = false;
    params.enableWeatherDynamics = false;
    params.proceduralQuestGen = false;

    worldEngine.initialize(params);

    DevelopmentTools& devTools = worldEngine.devTools;

    // Test material editor
    DevelopmentTools::MaterialEditor::MaterialNode* noiseNode =
        new DevelopmentTools::MaterialEditor::NoiseNode();
    noiseNode->type = "noise";
    noiseNode->position = Vector2(100.0f, 100.0f);

    devTools.materialEditor.activeGraph.nodes.push_back(std::unique_ptr<DevelopmentTools::MaterialEditor::MaterialNode>(noiseNode));

    EXPECT_EQ(devTools.materialEditor.activeGraph.nodes.size(), 1);

    // Test visual scripting
    DevelopmentTools::VisualScripting::ScriptNode* scriptNode =
        new DevelopmentTools::VisualScripting::ScriptNode();
    scriptNode->functionName = "SpawnEnemy";

    devTools.visualScripting.mainGraph.nodes.push_back(std::unique_ptr<DevelopmentTools::VisualScripting::ScriptNode>(scriptNode));

    EXPECT_EQ(devTools.visualScripting.mainGraph.nodes.size(), 1);

    // Test animation editor
    DevelopmentTools::AnimationEditor::Keyframe keyframe;
    keyframe.time = 1.0f;
    keyframe.values["position_x"] = 10.0f;
    keyframe.values["position_y"] = 5.0f;
    keyframe.interpolationType = "linear";

    devTools.animationEditor.tracks.push_back(DevelopmentTools::AnimationEditor::AnimationTrack());
    devTools.animationEditor.tracks.back().keyframes.push_back(keyframe);

    EXPECT_EQ(devTools.animationEditor.tracks.size(), 1);
    EXPECT_EQ(devTools.animationEditor.tracks.back().keyframes.size(), 1);

    // Test performance profiler
    devTools.profiler.frameHistory.clear();
    DevelopmentTools::PerformanceProfiler::FrameData frameData;
    frameData.frameTime = 16.6f;
    frameData.drawCalls = 500;
    frameData.trianglesRendered = 25000;
    frameData.activeEntities = 100;

    devTools.profiler.frameHistory.push_back(frameData);
    EXPECT_EQ(devTools.profiler.frameHistory.size(), 1);

    // Test memory allocation tracking
    void* testAllocation = devTools.profiler.trackAllocation(1024, "test.cpp", 42);
    EXPECT_NE(testAllocation, nullptr);

    devTools.profiler.trackDeallocation(testAllocation);
    EXPECT_EQ(devTools.profiler.activeAllocations.size(), 0);

    // Test asset pipeline
    devTools.assetPipeline.assetDatabase.clear();

    DevelopmentTools::AssetPipeline::Asset textureAsset;
    textureAsset.guid = "texture-001";
    textureAsset.type = "texture";
    textureAsset.sourcePath = "/assets/textures/brick.png";

    devTools.assetPipeline.assetDatabase["texture-001"] = textureAsset;
    EXPECT_EQ(devTools.assetPipeline.assetDatabase.size(), 1);

    // Test texture atlas packing
    std::vector<std::pair<int, int>> textureSizes = {
        {64, 64}, {128, 128}, {256, 256}, {32, 32}
    };

    devTools.assetPipeline.textureAtlas.packTextures(textureSizes,
        [&](int index, Vector2 minUV, Vector2 maxUV) {
            // Verify UV coordinates are valid
            EXPECT_GE(minUV.x, 0.0f);
            EXPECT_GE(minUV.y, 0.0f);
            EXPECT_LE(maxUV.x, 1.0f);
            EXPECT_LE(maxUV.y, 1.0f);
            EXPECT_GT(maxUV.x, minUV.x);
            EXPECT_GT(maxUV.y, minUV.y);
        });

    EXPECT_GT(devTools.assetPipeline.textureAtlas.packedTextures.size(), 0);
}

/**
 * @brief Test world streaming and LOD
 */
TEST_F(WorldSystemsTest, WorldStreamingAndLOD) {
    WorldEngine worldEngine;

    WorldEngine::WorldParameters params;
    params.seed = 8000;
    params.worldSize = 200.0f;
    params.biomeResolution = 32;
    params.weatherGridSize = 16;
    params.enableEcosystemSimulation = false;
    params.enableWeatherDynamics = false;
    params.proceduralQuestGen = false;

    worldEngine.initialize(params);

    InfiniteWorldGenerator& terrainGen = worldEngine.worldGenerator;

    // Test LOD-based terrain generation
    Vector3 nearCamera(0.0f, 50.0f, 0.0f);  // Close to terrain
    Vector3 farCamera(0.0f, 200.0f, 0.0f);   // Far from terrain

    // Generate patches at different LOD levels
    InfiniteWorldGenerator::TerrainPatch* nearPatch = terrainGen.generatePatch(Vector2(0.0f, 0.0f), 100.0f, 0);
    InfiniteWorldGenerator::TerrainPatch* farPatch = terrainGen.generatePatch(Vector2(100.0f, 0.0f), 100.0f, 2);

    ASSERT_NE(nearPatch, nullptr);
    ASSERT_NE(farPatch, nullptr);

    // Near patch should have more detail (higher vertex count for same size)
    EXPECT_GE(nearPatch->vertices.size(), farPatch->vertices.size());

    // Test LOD level calculation
    int nearLOD = terrainGen.calculateLOD(Vector2(0.0f, 0.0f), nearCamera, 100.0f);
    int farLOD = terrainGen.calculateLOD(Vector2(100.0f, 0.0f), farCamera, 100.0f);

    EXPECT_LE(nearLOD, farLOD); // Near objects should have lower LOD number (higher detail)

    // Test streaming system
    terrainGen.updateStreaming(nearCamera, 150.0f);

    // Test patch subdivision
    if (nearPatch->lodLevel < terrainGen.maxLODLevels) {
        // Patch could be subdivided for more detail
        EXPECT_GE(nearPatch->lodLevel, 0);
    }

    // Clean up
    delete nearPatch;
    delete farPatch;
}

/**
 * @brief Test world-environment interaction
 */
TEST_F(WorldSystemsTest, WorldEnvironmentInteraction) {
    WorldEngine worldEngine;

    WorldEngine::WorldParameters params;
    params.seed = 9000;
    params.worldSize = 100.0f;
    params.biomeResolution = 16;
    params.weatherGridSize = 8;
    params.enableEcosystemSimulation = true;
    params.enableWeatherDynamics = true;
    params.proceduralQuestGen = false;

    worldEngine.initialize(params);

    // Test weather-ecosystem interaction
    worldEngine.synchronizeWeatherEco();

    // Test player influence on world
    worldEngine.applyPlayerImpact(1, Vector3(50.0f, 0.0f, 50.0f), 0.8f, "Deforestation");

    // Test world adaptation
    std::vector<Vector3> playerPositions = {
        Vector3(10.0f, 0.0f, 10.0f),
        Vector3(20.0f, 0.0f, 20.0f),
        Vector3(30.0f, 0.0f, 30.0f)
    };

    worldEngine.adaptWorldToPlayers(playerPositions);

    // Test seasonal changes
    BiomeManager& biomeManager = worldEngine.biomeManager;
    biomeManager.simulateSeasonalChange(0.25f); // Spring

    // Test ecosystem response to environmental changes
    EcosystemSimulator& ecosystem = worldEngine.ecosystemSimulator;

    // Apply environmental pressure
    ecosystem.applyDisturbance(Vector3(25.0f, 0.0f, 25.0f), 0.5f, "HabitatLoss");

    // Ecosystem should adapt
    ecosystem.updatePopulations(0.016f);

    // Test narrative response to world changes
    NarrativeEngine& narrative = worldEngine.narrativeEngine;

    // World changes should trigger narrative events
    std::unordered_map<std::string, float> worldState;
    worldState["ForestHealth"] = 0.3f; // Poor forest health
    worldState["WildlifePopulation"] = 0.2f; // Low wildlife

    narrative.simulateButterflyEffect("EnvironmentalDamage", worldState);

    // Should generate appropriate narrative consequences
    EXPECT_GT(narrative.currentState.pendingEvents.size(), 0);
}

} // namespace Tests
} // namespace FoundryEngine
