/**
 * @file FluidSimulation.h
 * @brief Advanced fluid simulation with compressible and incompressible solvers
 */

#pragma once

#include "../math/Vector3.h"
#include "../math/Vector2.h"
#include "../graphics/Renderer.h"
#include <vector>
#include <memory>
#include <atomic>

namespace Foundry {

/**
 * @brief Compressible fluid simulation using SPH (Smoothed Particle Hydrodynamics)
 * Time Complexity: O(n²) worst case, O(n) with spatial optimization
 * Space Complexity: O(n) for particles + O(grid_size) for acceleration structures
 */
class CompressibleFluidSimulation {
public:
    struct FluidParticle {
        Vector3 position;
        Vector3 velocity;
        Vector3 acceleration;
        float density;
        float pressure;
        float mass;
        float temperature;
        Vector3 color;
        float age;

        FluidParticle() :
            density(1000.0f), // Water density
            pressure(0.0f),
            mass(0.001f), // 1 gram particles
            temperature(293.15f), // Room temperature
            color(Vector3(0.2f, 0.4f, 0.8f)),
            age(0.0f) {}
    };

    struct SimulationParameters {
        float timeStep = 1.0f / 120.0f; // Higher frequency for stability
        float restDensity = 1000.0f; // Water density
        float gasConstant = 20.0f; // Stiffness parameter
        float viscosity = 0.01f; // Viscosity coefficient
        float surfaceTension = 0.0728f; // Surface tension coefficient
        float damping = 0.99f; // Velocity damping
        float smoothingRadius = 0.1f; // SPH smoothing radius
        int maxParticles = 5000;
        Vector3 gravity = Vector3(0.0f, -9.81f, 0.0f);
        bool useGPUAcceleration = true;
        int solverIterations = 4; // Pressure solver iterations
    };

private:
    std::vector<FluidParticle> particles_;
    SimulationParameters params_;

    // SPH acceleration structures
    std::vector<std::vector<size_t>> neighborLists_;
    std::vector<Vector3> pressureForces_;
    std::vector<Vector3> viscosityForces_;
    std::vector<Vector3> surfaceTensionForces_;

    // Spatial grid for neighbor finding
    struct GridCell {
        std::vector<size_t> particleIndices;
    };
    std::vector<GridCell> spatialGrid_;
    Vector3 gridOrigin_;
    Vector3 gridSize_;
    int gridResolution_ = 64;

    // GPU compute resources
    void* gpuParticlesBuffer_ = nullptr;
    void* gpuNeighborsBuffer_ = nullptr;
    void* gpuForcesBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    CompressibleFluidSimulation();
    ~CompressibleFluidSimulation();

    bool initialize(const SimulationParameters& params);
    void update(float deltaTime);
    void render(Renderer* renderer);
    void addFluidVolume(const Vector3& center, const Vector3& size, float density);
    void applyForce(const Vector3& position, const Vector3& force, float radius);
    size_t getParticleCount() const { return particles_.size(); }
    void setParameters(const SimulationParameters& params) { params_ = params; }
    const SimulationParameters& getParameters() const { return params_; }
    void clear();

private:
    void updateNeighbors();
    void computeDensityAndPressure();
    void computePressureForces();
    void computeViscosityForces();
    void computeSurfaceTensionForces();
    void integrate(float deltaTime);
    void handleBoundaries();

    // GPU-accelerated SPH computation
    void updateParticlesGPU(float deltaTime);
    bool initializeGPU();
    void cleanupGPU();

    int getGridIndex(const Vector3& position) const;
    bool isInGrid(const Vector3& position) const;
};

/**
 * @brief Incompressible fluid simulation using FLIP (Fluid Implicit Particle)
 * Time Complexity: O(n) per frame with grid-based solver
 * Space Complexity: O(grid_size) for pressure projection
 */
class IncompressibleFluidSimulation {
public:
    struct FluidParticle {
        Vector3 position;
        Vector3 velocity;
        float density;
        Vector3 color;
        float age;

        FluidParticle() :
            density(1000.0f),
            color(Vector3(0.2f, 0.4f, 0.8f)),
            age(0.0f) {}
    };

    struct SimulationParameters {
        float timeStep = 1.0f / 60.0f;
        float viscosity = 0.01f;
        float surfaceTension = 0.0728f;
        float damping = 0.99f;
        int gridResolution = 64;
        float gridSize = 1.0f;
        int maxParticles = 10000;
        Vector3 gravity = Vector3(0.0f, -9.81f, 0.0f);
        bool useGPUAcceleration = true;
        int pressureIterations = 20; // Jacobi iterations for pressure
    };

private:
    std::vector<FluidParticle> particles_;
    SimulationParameters params_;

    // FLIP grid data
    std::vector<std::vector<std::vector<float>>> velocityGridU_; // X-component
    std::vector<std::vector<std::vector<float>>> velocityGridV_; // Y-component
    std::vector<std::vector<std::vector<float>>> velocityGridW_; // Z-component
    std::vector<std::vector<std::vector<float>>> pressureGrid_;
    std::vector<std::vector<std::vector<float>>> divergenceGrid_;
    std::vector<std::vector<std::vector<float>>> densityGrid_;

    // Particle-to-grid transfer data
    std::vector<std::vector<std::vector<float>>> particleCountGrid_;

    // GPU compute resources
    void* gpuVelocityBuffers_[3] = {nullptr, nullptr, nullptr}; // U, V, W
    void* gpuPressureBuffer_ = nullptr;
    void* gpuDivergenceBuffer_ = nullptr;
    void* gpuParticlesBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    IncompressibleFluidSimulation();
    ~IncompressibleFluidSimulation();

    bool initialize(const SimulationParameters& params);
    void update(float deltaTime);
    void render(Renderer* renderer);
    void addFluidVolume(const Vector3& center, const Vector3& size);
    void applyForce(const Vector3& position, const Vector3& force, float radius);
    size_t getParticleCount() const { return particles_.size(); }
    void setParameters(const SimulationParameters& params) { params_ = params; }
    const SimulationParameters& getParameters() const { return params_; }
    void clear();

private:
    void particlesToGrid();
    void computeDivergence();
    void solvePressure();
    void gridToParticles();
    void advectParticles(float deltaTime);
    void handleBoundaries();

    // GPU-accelerated FLIP computation
    void updateFluidGPU(float deltaTime);
    bool initializeGPU();
    void cleanupGPU();

    Vector3 gridToWorld(int i, int j, int k) const;
    void worldToGrid(const Vector3& worldPos, int& i, int& j, int& k) const;
    bool isValidGridCell(int i, int j, int k) const;
};

/**
 * @brief 2D Fluid simulation optimized for performance
 * Uses lattice Boltzmann method for incompressible fluids
 */
class FluidSimulation2D {
public:
    struct SimulationParameters2D {
        float timeStep = 1.0f / 60.0f;
        float viscosity = 0.01f;
        float relaxation = 0.6f; // LBM relaxation parameter
        int gridWidth = 128;
        int gridHeight = 128;
        float gridSize = 1.0f;
        Vector2 gravity = Vector2(0.0f, -9.81f);
        bool useGPUAcceleration = true;
        int maxIterations = 4;
    };

private:
    SimulationParameters2D params_;

    // Lattice Boltzmann distribution functions
    std::vector<std::vector<std::vector<float>>> f_; // 9 velocities for D2Q9
    std::vector<std::vector<float>> density_;
    std::vector<std::vector<Vector2>> velocity_;
    std::vector<std::vector<bool>> obstacle_;

    // GPU compute resources
    void* gpuDistributionsBuffer_ = nullptr;
    void* gpuDensityBuffer_ = nullptr;
    void* gpuVelocityBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

    // LBM constants
    static constexpr int Q = 9; // D2Q9 lattice
    static constexpr float weights[Q] = {4.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
                                        1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f};
    static constexpr int cx[Q] = {0, 1, 0, -1, 0, 1, -1, -1, 1};
    static constexpr int cy[Q] = {0, 0, 1, 0, -1, 1, 1, -1, -1};

public:
    FluidSimulation2D();
    ~FluidSimulation2D();

    bool initialize(const SimulationParameters2D& params);
    void update(float deltaTime);
    void render(Renderer* renderer);
    void addFlow(const Vector2& position, const Vector2& velocity, float radius);
    void addObstacle(const Vector2& position, float radius);
    void setParameters(const SimulationParameters2D& params) { params_ = params; }
    const SimulationParameters2D& getParameters() const { return params_; }
    void clear();

private:
    void initializeLattice();
    void collide();
    void stream();
    void applyBoundaryConditions();
    void computeMacroscopic();

    // GPU-accelerated LBM
    void updateLatticeGPU(float deltaTime);
    bool initializeGPU();
    void cleanupGPU();
};

} // namespace Foundry