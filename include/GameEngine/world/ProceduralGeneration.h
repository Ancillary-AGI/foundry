#ifndef NEUTRAL_GAMEENGINE_PROCEDURAL_GENERATION_H
#define NEUTRAL_GAMEENGINE_PROCEDURAL_GENERATION_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <random>
#include "../../math/Vector3.h"
#include "../../math/Vector2.h"
#include "../../math/Matrix4.h"
#include "../System.h"

namespace NeutralGameEngine {

// Advanced Noise Functions
class NoiseGenerator {
public:
    enum NoiseType { PERLIN, SIMPLEX, WORLEY, FRACTAL, TURBULENCE };

    NoiseGenerator(uint32_t seed = 123456);

    // Perlin noise (gradient noise)
    float perlinNoise(float x, float y, float z = 0.0f);
    Vector3 perlinNoiseVector(float x, float y, float z = 0.0f);

    // Simplex noise (improved Perlin)
    float simplexNoise(float x, float y, float z = 0.0f);

    // Worley noise (cellular noise)
    float worleyNoise(float x, float y, float z = 0.0f,
                     int distanceFunction = 0, // 0=euclidean, 1=manhattan, 2=chebyshev
                     int featurePointCount = 1);

    // Fractal noise (noise accumulation)
    float fractalNoise(float x, float y, float z = 0.0f,
                      int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f);

    // Turbulence (absolute value of fractal noise)
    float turbulence(float x, float y, float z = 0.0f,
                    int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f);

    // Ridged multifractal (sharp ridges)
    float ridgedMF(float x, float y, float z = 0.0f,
                  int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f);

    // Flow noise (time-varying noise following flow field)
    float flowNoise(float x, float y, float z = 0.0f, float time = 0.0f);

    // Domain warping
    Vector2 domainWarp(float x, float y, float amplitude = 1.0f, float frequency = 1.0f);

    // Custom gradient basis functions
    void setCustomGradients(const std::vector<Vector3>& gradients);

private:
    uint32_t seed_;
    std::mt19937 rng_;
    std::vector<Vector3> customGradients_;

    // Permutation tables for noise
    std::vector<int> permTable_;
    std::vector<int> permTableMod12_;

    void initializePermutationTable();
    Vector3 gradient3D(int hash);
    float fade(float t) const { return t * t * t * (t * (t * 6 - 15) + 10); }
    float lerp(float a, float b, float t) const { return a + t * (b - a); }
};

// Infinite Procedural Worlds
class InfiniteWorldGenerator {
public:
    struct TerrainPatch {
        Vector2 center;
        float size;
        std::vector<Vector3> vertices;
        std::vector<int> indices;
        std::vector<Vector3> normals;
        std::vector<Vector2> textureCoords;
        float minHeight, maxHeight;
        std::vector<float> heightMap;

        // LOD information
        int lodLevel;
        bool subdivided;
        std::vector<std::unique_ptr<TerrainPatch>> children;
    };

    NoiseGenerator noiseGen;
    int patchSize;
    float worldScale;
    std::unordered_map<std::string, TerrainPatch*> activePatches;

    // Generate infinite terrain patch
    TerrainPatch* generatePatch(Vector2 center, float size, int lodLevel = 0);

    // Terrain height function using fractal noise
    float terrainHeight(float x, float z);

    // Adapt LOD based on distance and view importance
    int calculateLOD(Vector2 patchCenter, Vector3 cameraPosition, float baseSize);

    // Streaming system for infinite worlds
    void updateStreaming(Vector3 cameraPosition, float viewDistance);

    // Terrain texturing based on slope and height
    void assignBiomeTexture(TerrainPatch* patch);

private:
    const float maxTerrainHeight = 100.0f;
    const int maxLODLevels = 6;
};

// Biome Systems with Ecosystem Simulation
class BiomeManager {
public:
    enum BiomeType {
        TUNDRA, TAIGA, TEMPERATE_FOREST, TROPICAL_FOREST,
        DESERT, SAVANNA, GRASSLAND, ALPINE, MARINE, URBAN
    };

    struct BiomeData {
        BiomeType type;
        float temperature;
        float humidity;
        float fertility;
        std::vector<std::string> nativeFlora;
        std::vector<std::string> nativeFauna;
        std::vector<Vector3> placementRules; // Color-coded density
    };

    std::unordered_map<BiomeType, BiomeData> biomeDefinitions;

    // Generate biome map using noise
    std::vector<std::vector<BiomeType>> generateBiomeMap(int width, int height,
                                                       float scale = 0.01f);

    // Biome blending for seamless transitions
    BiomeType blendBiomes(const std::vector<BiomeType>& nearbyBiomes,
                         const Vector3& weights);

    // Environmental transitions
    void simulateSeasonalChange(float time);
};

// Ecosystem Simulation with Food Chains
class EcosystemSimulator {
public:
    struct Species {
        std::string name;
        int population;
        float growthRate;
        float carryingCapacity; // Environmental limit
        std::vector<std::string> preySpecies; // Food sources
        std::vector<std::string> predatorSpecies;
        float adaptability; // Ability to survive environmental changes

        // Behavioral data
        float territoryRadius;
        float socialTendency;
        std::unordered_map<std::string, float> environmentalPrefs; // temp, humidity, etc.
    };

    struct FoodChain {
        std::vector<Species*> trophicLevels[4]; // Producers, Herbivores, Carnivores, Top Predators

        void simulateTrophicCascade(float disturbance);
        float stabilityIndex();
        std::vector<std::string> identifyKeystoneSpecies();
    };

    std::unordered_map<std::string, Species> speciesDatabase;
    FoodChain globalFoodChain;

    // Lotka-Volterra predator-prey equations
    void simulatePredatorPreyDynamics(float dt);

    // Environmental adaptation
    void simulateAdaptation(Species& species, const BiomeManager::BiomeData& biome, float dt);

    // Population dynamics with stochastic factors
    void updatePopulations(float dt);

    // Migration patterns
    void simulateMigration(const std::vector<Vector3>& migrationRoutes);

    // Ecological disturbances (fires, plagues, etc.)
    void applyDisturbance(const Vector3& location, float intensity, const std::string& type);

private:
    float KCarryingCapacity_ = 1000.0f; // Base carrying capacity
    std::mt19937 rng_; // For stochastic events
};

// Advanced Weather Systems
class WeatherEngine {
public:
    struct WeatherCell {
        Vector3 position;
        float temperature;
        float humidity;
        float pressure;
        Vector3 windVelocity;
        float cloudDensity;
        float precipitationRate;

        // Time derivatives for simulation
        Vector3 pressureGradient;
        float temperatureTendency;
        Vector3 windTendency;
    };

    struct AtmosphericLayer {
        float altitude; // meters above sea level
        float temperatureLapse; // temperature change with height
        float windShear; // wind speed variation
        float humidityProfile;
    };

    std::vector<WeatherCell> weatherGrid;
    std::vector<AtmosphericLayer> troposphere;

    // Numerical weather prediction
    void integrateWeather(float dt);

    // Atmospheric physics
    void calculateAdvection(const std::vector<WeatherCell>& current,
                           std::vector<Vector3>& advectedQuantities);

    // Condensation and precipitation
    float calculateCondensationRate(const WeatherCell& cell);

    // Terrain interaction with weather
    void applyTerrainInfluence(const std::vector<float>& heightMap,
                              std::vector<WeatherCell>& weatherGrid);

    // Weather-terrain feedback
    struct TerrainWeatherCoupling {
        float evaporationRate;
        float runoffRate;
        float soilMoisture;
        Vector3 windReduction; // Terrain blockage
    };

    std::vector<TerrainWeatherCoupling> terrainCoupling;

    // Weather events generation
    void generateExtremeWeather(float severity, const Vector3& center);

    // Long-term climate patterns
    void simulateClimatePatterns(float timeOfYear);

    // Weather rendering
    struct WeatherRendering {
        std::vector<Vector3> cloudPositions;
        std::vector<float> precipitationParticles;
        Vector3 sunDirection;
        float atmosphericScattering;

        void renderClouds();
        void renderPrecipitation();
        void renderSkybox();
    } weatherRendering;

private:
    const int gridResolution_ = 64; // 64x64 grid for weather simulation
    const float timeStep_ = 0.016f; // 60 FPS weather simulation
    const float diffusionCoeff_ = 0.1f; // Atmospheric mixing
};

// Dynamic Quest & Narrative System (19. Quest & Narrative)
class NarrativeEngine {
public:
    struct QuestNode {
        std::string id;
        std::string title;
        std::string description;
        std::vector<std::string> objectives;
        std::vector<std::string> prerequisites;
        std::unordered_map<std::string, std::string> conditions; // variable -> required_value

        // Branching logic
        std::vector<std::pair<std::string, std::string>> branches; // choice -> next_node
        std::vector<std::string> consequences;

        // Narrative elements
        std::string dialogueText;
        std::unordered_map<std::string, float> characterRelations; // NPC_ID -> affinity
    };

    struct NarrativeState {
        std::unordered_map<std::string, float> variables; // Global story variables
        std::unordered_map<std::string, std::string> flags; // Binary story flags
        std::vector<std::string> completedQuests;
        std::vector<std::string> activeQuests;

        // Butterfly effect system
        struct CausalEvent {
            std::string trigger;
            std::vector<std::string> affectedVariables;
            float probability;
            std::string consequence;
        };

        std::vector<CausalEvent> pendingEvents;
    };

    NarrativeState currentState;
    std::unordered_map<std::string, QuestNode> questGraph;

    // Dynamic quest generation
    QuestNode generateProceduralQuest(const std::string& archetype,
                                    const std::vector<std::string>& constraints);

    // Branching dialogue system
    struct DialogueTree {
        struct DialogueNode {
            std::string speakerId;
            std::string text;
            std::vector<std::string> choices;
            std::unordered_map<std::string, DialogueNode*> children;

            // Emotional states
            std::string emotion;
            float persuasionValue;
        };

        DialogueNode* root;
        std::string currentSpeaker;
        std::unordered_map<std::string, float> characterTrust;

        void traversePath(const std::vector<std::string>& choices);
    };

    DialogueTree activeDialogue;

    // Moral choice mechanics
    struct MoralChoice {
        std::string question;
        std::vector<std::pair<std::string, float>> consequences; // choice -> morality shift

        float resolveChoice(const std::string& choice,
                           std::unordered_map<std::string, float>& relationshipGraph);
    };

    // Memory systems for NPC relationships
    struct RelationshipNetwork {
        std::unordered_map<std::string, std::unordered_map<std::string, float>> affinities;

        float socialInfluence(const std::string& npc1, const std::string& npc2);
        void propagateInfluence(const std::string& triggerNpc, float influenceDelta);
    };

    RelationshipNetwork relationships;

    // Simulate butterfly effect from player choices
    void simulateButterflyEffect(const std::string& triggerEvent,
                               std::unordered_map<std::string, float>& worldState);

private:
    std::mt19937 rng_; // For procedural generation
    std::unordered_map<std::string, QuestNode> questTemplates;
};

// Visual & Asset Pipeline Tools (20-22. Development Tools Framework)
class DevelopmentTools {
public:
    // Node-based Material Editor (Substance-like)
    class MaterialEditor {
    public:
        struct MaterialNode {
            std::string type; // "noise", "blend", "math", "transform"
            Vector2 position;
            std::unordered_map<std::string, float> parameters;
            std::vector<std::string> inputConnections;
            std::string outputType;

            virtual void execute(std::unordered_map<std::string, std::any>& outputs) = 0;
        };

        struct MaterialGraph {
            std::vector<std::unique_ptr<MaterialNode>> nodes;
            std::unordered_map<std::string, std::vector<std::string>> connections;

            void executeGraph();
            void serialize(const std::string& filepath);
            void deserialize(const std::string& filepath);
        };

        MaterialGraph activeGraph;

        // Built-in nodes
        class NoiseNode : public MaterialNode {
            void execute(std::unordered_map<std::string, std::any>& outputs) override;
        };

        class BlendNode : public MaterialNode {
            void execute(std::unordered_map<std::string, std::any>& outputs) override;
        };

        // Live preview system
        void renderPreview(const Vector2& position, float scale);
    };

    // Visual Scripting System
    class VisualScripting {
    public:
        struct ScriptNode {
            std::string functionName;
            std::vector<std::string> parameters;
            std::vector<ScriptNode*> inputs;
            ScriptNode* output;

            virtual std::any execute() = 0;
        };

        struct ScriptingGraph {
            std::vector<std::unique_ptr<ScriptNode>> nodes;
            std::unordered_map<std::string, Variable> variables;

            std::any executeFlow();
            void addVariable(const std::string& name, Variable value);
        };

        ScriptingGraph mainGraph;

        // Event-driven execution
        struct ScriptEvent {
            std::string eventType;
            std::unordered_map<std::string, std::any> data;
            ScriptNode* handler;
        };

        void processEvent(const ScriptEvent& event);
    };

    // Animation Timeline Editor
    class AnimationEditor {
    public:
        struct Keyframe {
            float time;
            std::unordered_map<std::string, float> values; // bone_rotation_x, etc.
            std::string interpolationType; // linear, cubic, etc.
        };

        struct AnimationTrack {
            std::string target; // "character.bone_left_arm"
            std::vector<Keyframe> keyframes;
            std::string easingFunction;
        };

        std::vector<AnimationTrack> tracks;

        // Curve editing
        void modifyKeyframeTangents(const std::string& trackName, float time, Vector2 tangents);

        // Ghost animation for layering
        void addGhostTrack(const AnimationTrack& ghostTrack, float weight);

        // Export to runtime format
        void exportAnimation(const std::string& filepath);
    };

    // Performance Profiling & Debugging
    class PerformanceProfiler {
    public:
        struct PerformanceMetric {
            std::string category; // CPU, GPU, Memory, Network
            std::string name;
            float value;
            float target; // Optimal threshold
            uint64_t timestamp;
        };

        struct FrameData {
            float frameTime; // milliseconds
            uint32_t drawCalls;
            uint32_t trianglesRendered;
            uint32_t activeEntities;

            std::unordered_map<std::string, PerformanceMetric> metrics;
        };

        std::vector<FrameData> frameHistory;

        // Real-time flame graph generation
        struct FlameGraph {
            struct StackFrame {
                std::string functionName;
                uint64_t startTime;
                uint64_t endTime;
                uint32_t threadId;
            };

            std::vector<StackFrame> frames;
            void generateVisualization();
        };

        FlameGraph cpuFlameGraph;
        FlameGraph gpuFlameGraph;

        // Memory leak detection
        struct MemoryAllocation {
            /**
             * @brief Address of the allocated memory block.
             * @note Consider using smart pointers for ownership tracking.
             */
            void* address;
            size_t size;
            std::string file;
            int line;
            uint64_t allocationTime;
        };

        std::unordered_map<void*, MemoryAllocation> activeAllocations;

        void* trackAllocation(size_t size, const std::string& file, int line);
        void trackDeallocation(void* addr);

        // Automated optimization suggestions
        std::vector<std::string> analyzeBottlenecks(const FrameData& frame);
    };

    MaterialEditor materialEditor;
    VisualScripting visualScripting;
    AnimationEditor animationEditor;
    PerformanceProfiler profiler;

    // Asset Management & Optimization Pipeline
    class AssetPipeline {
    public:
        struct Asset {
            std::string guid;
            std::string type; // texture, model, audio, script
            std::string sourcePath;
            std::vector<std::string> dependencies;
            std::unordered_map<std::string, std::any> metadata;
            /**
             * @brief Cached processed data for runtime use.
             * @note Consider using std::unique_ptr<uint8_t[]> or a type-safe variant for better safety.
             */
            void* runtimeData;
        };

        std::unordered_map<std::string, Asset> assetDatabase;

        // Reference tracking to prevent memory leaks
        std::unordered_map<std::string, std::vector<std::string>> referenceGraph;

        // Asset optimization
        void optimizeTexture(const std::string& assetId, const std::string& format);
        void generateLODs(const std::string& meshAssetId, int maxLOD = 4);

        // Texture atlas packing with bin packing algorithms
        struct TextureAtlas {
            int width, height;
            std::vector<std::pair<Vector2, Vector2>> regions; // uv_min, uv_max
            std::vector<std::string> packedTextures;

            void packTextures(const std::vector<std::pair<int, int>>& textureSizes,
                            std::function<bool(int, Vector2, Vector2)> placementCallback);
        };

        TextureAtlas textureAtlas;
    };

    AssetPipeline assetPipeline;
};

// World Generation & Ecosystem Orchestrator
class WorldEngine : public System {
public:
    InfiniteWorldGenerator worldGenerator;
    BiomeManager biomeManager;
    EcosystemSimulator ecosystemSimulator;
    WeatherEngine weatherEngine;
    NarrativeEngine narrativeEngine;
    DevelopmentTools devTools;

    struct WorldParameters {
        int seed;
        float worldSize;
        int biomeResolution;
        int weatherGridSize;
        bool enableEcosystemSimulation;
        bool enableWeatherDynamics;
        bool proceduralQuestGen;
    } worldParams;

    void initialize(const WorldParameters& params);

    void update(float dt);

    // World-tied systems
    void synchronizeWeatherEco();

    // Player influence on world
    void applyPlayerImpact(uint32_t playerId, const Vector3& location, float intensity, const std::string& actionType);

    // Dynamic world adaptation
    void adaptWorldToPlayers(const std::vector<Vector3>& playerPositions);

private:
    uint64_t worldTime_ = 0; // In-game time in seconds
    uint32_t randomSeed_;
};
};
};

#endif // NEUTRAL_GAMEENGINE_PROCEDURAL_GENERATION_H
