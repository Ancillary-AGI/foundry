#ifndef NEUTRAL_GAMEENGINE_NAVIGATION_SYSTEM_H
#define NEUTRAL_GAMEENGINE_NAVIGATION_SYSTEM_H

#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include "../../math/Vector3.h"
#include "../../math/Vector2.h"
#include "../System.h"

namespace NeutralGameEngine {

// Navigation Mesh (NavMesh)
class NavMesh {
public:
    struct Triangle {
        Vector3 vertices[3];
        Vector3 normal;
        int neighbors[3]; // Adjacent triangle indices (-1 if none)
        float area;
        bool walkable;
    };

    std::vector<Triangle> triangles;
    std::vector<Vector3> vertices;

    // Build from geometry
    void buildFromMesh(const std::vector<Vector3>& meshVerts, const std::vector<int>& indices);

    // Dynamic nav mesh generation
    void recomputeRegions();

    // Raycasting for point validation
    bool raycast(const Vector3& start, const Vector3& end, Vector3& hitPoint, int& triangleIndex) const;

    // Find closest point on navmesh
    Vector3 findClosestPoint(const Vector3& point) const;

    // Get path triangles
    std::vector<int> getPathTriangles(int startTri, int endTri) const;

private:
    // Delaunay triangulation for navmesh generation
    std::vector<Triangle> delaunayTriangulation(const std::vector<Vector3>& points);

    // Validate triangles for walkability
    void validateTriangles();
};

// A* Pathfinding with hierarchical waypoints
class HierarchicalPathfinder {
public:
    struct Waypoint {
        Vector3 position;
        std::vector<int> connections;
        float radius;
        int level; // Hierarchical level
    };

    std::vector<Waypoint> waypoints;
    NavMesh navMesh;

    // Hierarchical A* with waypoint clustering
    std::vector<Vector3> findPath(const Vector3& start, const Vector3& end);

    // Smooth path using splines
    std::vector<Vector3> smoothPath(const std::vector<Vector3>& path);

    // Multi-level pathfinding
    std::vector<Vector3> findPathMultiLevel(const Vector3& start, const Vector3& end, int maxLevel);

private:
    // Distance heuristic for A*
    float heuristic(const Vector3& a, const Vector3& b) const;

    // A* node for priority queue
    struct AStarNode {
        int waypointIndex;
        float gCost, hCost, fCost;
        int parentIndex;

        bool operator>(const AStarNode& other) const { return fCost > other.fCost; }
    };

    // Funnel algorithm for string pulling
    std::vector<Vector3> stringPulling(const std::vector<Vector3>& path);
};

// Crowd Simulation with Psychological Forces
class CrowdSimulator {
public:
    struct Agent {
        Vector3 position;
        Vector3 velocity;
        Vector3 goal;
        float radius;
        float maxSpeed;
        std::vector<Vector3> preferredPath;
        int currentPathIndex;

        // Psychological parameters
        float anxiety;      // Stress level affecting decision making
        float cooperativeness; // Willingness to yield to others
        float patience;     // Time before becoming agitated

        // Long-term goals and planning
        std::vector<Vector3> longTermGoals;
        float goalSatisfaction; // How close to achieving overall objectives
    };

    struct Force {
        Vector3 direction;
        float magnitude;
        float decayDistance;
    };

    std::vector<Agent> agents;

    // Psychological forces
    Force socialForce;        // Force from nearby agents
    Force anxietyForce;       // Anxiety-induced force
    Force goalForce;         // Goal-directed force
    Force obstacleForce;     // Obstacle avoidance force

    void initializeAgents(int count, const Vector3& areaMin, const Vector3& areaMax);

    void simulate(float dt);

    // Dynamic path replanning
    void replanPaths();

    // Agent-to-agent psychological interactions
    Vector3 computeSocialForces(const Agent& agent, const std::vector<Agent>& others);

    // Predictive collision avoidance
    Vector3 predictiveAvoidance(const Agent& agent, const std::vector<Agent>& others, float predictionTime);

    // Group behavior (formations, leadership)
    void simulateGroupBehavior(const std::vector<int>& groupIndices);

private:
    // RVO (Reciprocal Velocity Obstacles) for collision avoidance
    Vector3 computeRVO(const Agent& agent, const Agent& obstacle);

    // Psychological state updates
    void updatePsychologicalStates(float dt);

    // Goal-oriented planning
    void updateGoalProgress();
};

// Predictive Collision Avoidance
class PredictiveCollisionAvoidance {
public:
    struct Trajectory {
        Vector3 startPosition;
        Vector3 startVelocity;
        Vector3 acceleration;
        float timeHorizon;
    };

    // Clear Path Algorithm
    bool findClearPath(Trajectory& agentTrajectory,
                      const std::vector<Trajectory>& obstacles,
                      Trajectory& clearTrajectory);

    // Velocity Obstacle computation
    std::vector<Vector3> computeVelocityObstacles(const Trajectory& agent,
                                                 const std::vector<Trajectory>& obstacles);

    // Sampling-based path planning
    Trajectory sampleValidTrajectory(const Trajectory& agent,
                                   const std::vector<Vector3>& velocityObstacles,
                                   int sampleCount);

    // Energy-based optimization
    Trajectory optimizeTrajectory(const Trajectory& initial,
                                const Trajectory& preferred,
                                const std::vector<Vector3>& velocityObstacles);
};

// Perception Systems for NPCs
class PerceptionSystem {
public:
    enum class PerceptionType {
        VISION,
        HEARING,
        SCENT,
        THERMAL,
        VIBRATION
    };

    struct PerceptionEvent {
        PerceptionType type;
        Vector3 position;
        Vector3 source;
        float intensity;
        float timestamp;
        int sourceEntityId;
    };

    struct Sensor {
        PerceptionType type;
        Vector3 position;
        Vector3 orientation;
        float range;
        float fieldOfView; // For vision sensors
        float sensitivity;

        // Custom perception filters
        std::function<float(const PerceptionEvent&)> filterFunction;
    };

    std::vector<Sensor> sensors;
    std::vector<PerceptionEvent> activeEvents;

    // Computer vision for NPC sight
    struct VisionResult {
        bool detected;
        Vector3 position;
        int entityId;
        float confidence;
        std::vector<std::string> recognizedFeatures;
    };

    VisionResult processVision(const Sensor& sensor, const std::vector<Vector3>& sceneEntities);

    // Spatial audio processing
    struct AudioSource {
        Vector3 position;
        float volume;
        float frequency;
        std::string signature;
    };

    AudioSource localizeSound(const Vector3& listenerPos, const Vector3& earSeparation,
                            const std::vector<float>& leftEarSamples,
                            const std::vector<float>& rightEarSamples);

    // Scent simulation
    struct ScentParticle {
        Vector3 position;
        Vector3 velocity;
        float concentration;
        float evaporation;
        std::string scentType;
    };

    void simulateScentDynamics(std::vector<ScentParticle>& particles, float dt);

    Vector3 trackScent(const Vector3& snifferPos, const std::vector<ScentParticle>& particles);

    // Thermal/vibration detection
    float detectThermalSignature(const Vector3& sensorPos, const Vector3& sourcePos,
                               float sourceTemperature, float ambientTemp);

    Vector3 detectVibrations(const Vector3& sensorPos, const std::vector<Vector3>& vibrationSources);
};

// Dynamic Navigation Mesh Generation
class DynamicNavMeshGenerator {
public:
    NavMesh baseNavMesh;
    std::vector<Vector3> dynamicObstacles;

    // Real-time mesh deformation
    void deformMeshAt(const Vector3& position, float radius, const Vector3& deformation);

    // Procedural mesh updates based on terrain changes
    void updateFromTerrain(const std::vector<std::vector<float>>& heightMap,
                          const Vector2& worldMin, const Vector2& worldMax);

    // Navigation mesh streaming for large worlds
    NavMesh generateTileMesh(int tileX, int tileZ, int tileSize);

    // Handle moving obstacles
    void updateMovingObstacles(const std::vector<Vector3>& obstaclePositions,
                             const std::vector<float>& obstacleRadii);

private:
    // Quadtree for terrain partitioning
    struct QuadNode {
        Vector2 center;
        float size;
        float minHeight, maxHeight;
        std::unique_ptr<QuadNode> children[4];
    };

    QuadNode* terrainQuadTree;
    void subdivideQuadNode(QuadNode* node, int maxDepth);
};

// Navigation System Main Class
class NavigationSystem : public System {
public:
    NavMesh navMesh;
    HierarchicalPathfinder pathfinder;
    CrowdSimulator crowdSim;
    PerceptionSystem perception;
    DynamicNavMeshGenerator dynamicGenerator;
    PredictiveCollisionAvoidance predictiveAvoidance;

    // World representation
    std::vector<Vector3> walkableAreas;
    std::vector<Vector3> obstacles;

    void initialize();

    void update(float dt);

    // Request path for agent
    std::vector<Vector3> requestPath(int agentId, const Vector3& start, const Vector3& end);

    // Update crowd simulation
    void updateCrowd(float dt);

    // Process perception events
    void processPerceptionEvents();

    // Handle dynamic obstacles
    void handleDynamicObstacle(int obstacleId, const Vector3& position, float radius);

    // Terrain analysis for navigation
    void analyzeTerrain(const std::vector<std::vector<float>>& heightMap,
                       const Vector2& worldSize);

private:
    std::unordered_map<int, std::vector<Vector3>> activePaths;
    std::queue<int> pathRequests;
};

} // namespace NeutralGameEngine

#endif // NEUTRAL_GAMEENGINE_NAVIGATION_SYSTEM_H
