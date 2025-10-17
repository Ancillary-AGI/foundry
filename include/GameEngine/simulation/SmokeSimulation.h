/**
 * @file SmokeSimulation.h
 * @brief High-performance smoke simulation with Brownian motion and GPU acceleration
 */

#pragma once

#include "../math/Vector3.h"
#include "../math/Vector2.h"
#include "../graphics/Renderer.h"
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

namespace Foundry {

/**
 * @brief Particle-based smoke simulation with Brownian motion
 * Time Complexity: O(n) per frame, Space Complexity: O(n)
 * GPU-accelerated for real-time performance
 */
class SmokeSimulation {
public:
    struct SmokeParticle {
        Vector3 position;
        Vector3 velocity;
        float density;
        float temperature;
        float age;
        float lifetime;
        Vector3 color;
        float size;

        SmokeParticle() :
            density(0.0f),
            temperature(0.0f),
            age(0.0f),
            lifetime(1.0f),
            color(Vector3(0.5f, 0.5f, 0.5f)),
            size(1.0f) {}
    };

    struct SimulationParameters {
        float timeStep = 1.0f / 60.0f;
        float diffusionRate = 0.1f;
        float buoyancy = 0.5f;
        float temperatureDecay = 0.95f;
        float densityDecay = 0.98f;
        float brownianMotionScale = 0.1f;
        int maxParticles = 10000;
        float spawnRate = 100.0f;
        Vector3 gravity = Vector3(0.0f, -9.81f, 0.0f);
        bool useGPUAcceleration = true;
    };

private:
    std::vector<SmokeParticle> particles_;
    SimulationParameters params_;
    std::atomic<bool> isRunning_;
    std::thread simulationThread_;

    // GPU compute resources
    void* gpuParticlesBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;
    void* gpuCommandBuffer_ = nullptr;

    // Spatial partitioning for efficiency
    struct GridCell {
        std::vector<size_t> particleIndices;
        float densitySum = 0.0f;
        Vector3 velocitySum = Vector3(0.0f);
    };

    std::vector<GridCell> spatialGrid_;
    Vector3 gridOrigin_;
    Vector3 gridSize_;
    int gridResolution_ = 32;

    // Brownian motion random number generator
    std::mt19937 rng_;
    std::normal_distribution<float> normalDist_;

public:
    SmokeSimulation();
    ~SmokeSimulation();

    /**
     * Initialize the smoke simulation
     * @param params Simulation parameters
     * @return true if initialization successful
     */
    bool initialize(const SimulationParameters& params);

    /**
     * Update simulation for one time step
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);

    /**
     * Render smoke particles
     * @param renderer Renderer to use for drawing
     */
    void render(Renderer* renderer);

    /**
     * Add smoke emitter at position
     * @param position Emitter position
     * @param intensity Emission intensity
     * @param temperature Initial temperature
     */
    void addEmitter(const Vector3& position, float intensity, float temperature);

    /**
     * Apply external force to smoke
     * @param position Force application position
     * @param force Force vector
     * @param radius Force influence radius
     */
    void applyForce(const Vector3& position, const Vector3& force, float radius);

    /**
     * Get current particle count
     * @return Number of active particles
     */
    size_t getParticleCount() const { return particles_.size(); }

    /**
     * Set simulation parameters
     * @param params New parameters
     */
    void setParameters(const SimulationParameters& params) { params_ = params; }

    /**
     * Get current parameters
     * @return Current simulation parameters
     */
    const SimulationParameters& getParameters() const { return params_; }

    /**
     * Clear all particles
     */
    void clear();

private:
    /**
     * Spawn new smoke particles
     * Time Complexity: O(1) amortized
     */
    void spawnParticles();

    /**
     * Update particle physics with Brownian motion
     * Time Complexity: O(n)
     */
    void updateParticles(float deltaTime);

    /**
     * Apply Brownian motion to particles
     * Time Complexity: O(n)
     */
    void applyBrownianMotion(float deltaTime);

    /**
     * Update spatial grid for collision detection
     * Time Complexity: O(n)
     */
    void updateSpatialGrid();

    /**
     * Handle particle collisions and merging
     * Time Complexity: O(n * k) where k is average particles per cell
     */
    void handleCollisions();

    /**
     * Apply diffusion to smooth density field
     * Time Complexity: O(grid_size)
     */
    void applyDiffusion();

    /**
     * Remove dead particles
     * Time Complexity: O(n)
     */
    void removeDeadParticles();

    /**
     * GPU-accelerated particle update
     * Time Complexity: O(n) on GPU
     */
    void updateParticlesGPU(float deltaTime);

    /**
     * Initialize GPU compute resources
     */
    bool initializeGPU();

    /**
     * Cleanup GPU resources
     */
    void cleanupGPU();

    /**
     * Get grid cell index for position
     */
    int getGridIndex(const Vector3& position) const;

    /**
     * Check if position is within grid bounds
     */
    bool isInGrid(const Vector3& position) const;
};

/**
 * @brief 2D Smoke simulation for efficiency
 * Optimized for 2D games with lower computational cost
 */
class SmokeSimulation2D {
public:
    struct SmokeParticle2D {
        Vector2 position;
        Vector2 velocity;
        float density;
        float temperature;
        float age;
        float lifetime;
        Vector3 color;
        float size;

        SmokeParticle2D() :
            density(0.0f),
            temperature(0.0f),
            age(0.0f),
            lifetime(1.0f),
            color(Vector3(0.5f, 0.5f, 0.5f)),
            size(1.0f) {}
    };

    struct SimulationParameters2D {
        float timeStep = 1.0f / 60.0f;
        float diffusionRate = 0.15f;
        float buoyancy = 0.3f;
        float temperatureDecay = 0.96f;
        float densityDecay = 0.99f;
        float brownianMotionScale = 0.08f;
        int maxParticles = 5000;
        float spawnRate = 50.0f;
        Vector2 gravity = Vector2(0.0f, -9.81f);
        bool useGPUAcceleration = true;
    };

private:
    std::vector<SmokeParticle2D> particles_;
    SimulationParameters2D params_;
    std::vector<std::vector<float>> densityGrid_;
    std::vector<std::vector<Vector2>> velocityGrid_;
    int gridWidth_ = 64;
    int gridHeight_ = 64;

public:
    SmokeSimulation2D();
    ~SmokeSimulation2D();

    bool initialize(const SimulationParameters2D& params);
    void update(float deltaTime);
    void render(Renderer* renderer);
    void addEmitter(const Vector2& position, float intensity, float temperature);
    void applyForce(const Vector2& position, const Vector2& force, float radius);
    size_t getParticleCount() const { return particles_.size(); }
    void setParameters(const SimulationParameters2D& params) { params_ = params; }
    const SimulationParameters2D& getParameters() const { return params_; }
    void clear();

private:
    void spawnParticles();
    void updateParticles(float deltaTime);
    void applyBrownianMotion(float deltaTime);
    void updateDensityGrid();
    void applyDiffusion();
    void advectVelocity();
    void removeDeadParticles();
};

} // namespace Foundry