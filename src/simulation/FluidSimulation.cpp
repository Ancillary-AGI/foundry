/**
 * @file FluidSimulation.cpp
 * @brief Implementation of advanced fluid simulation with SPH and FLIP solvers
 */

#include "../../include/GameEngine/simulation/FluidSimulation.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>

namespace Foundry {

// Compressible Fluid Simulation (SPH) Implementation
CompressibleFluidSimulation::CompressibleFluidSimulation() {
}

CompressibleFluidSimulation::~CompressibleFluidSimulation() {
    cleanupGPU();
}

bool CompressibleFluidSimulation::initialize(const SimulationParameters& params) {
    params_ = params;
    particles_.reserve(params_.maxParticles);

    // Initialize SPH acceleration structures
    neighborLists_.resize(params_.maxParticles);
    pressureForces_.resize(params_.maxParticles);
    viscosityForces_.resize(params_.maxParticles);
    surfaceTensionForces_.resize(params_.maxParticles);

    // Initialize spatial grid
    spatialGrid_.resize(gridResolution_ * gridResolution_ * gridResolution_);

    // Initialize GPU resources if requested
    if (params_.useGPUAcceleration) {
        if (!initializeGPU()) {
            std::cout << "GPU acceleration not available for SPH, falling back to CPU" << std::endl;
            params_.useGPUAcceleration = false;
        }
    }

    return true;
}

void CompressibleFluidSimulation::update(float deltaTime) {
    // Update neighbors
    updateNeighbors();

    // Compute density and pressure
    computeDensityAndPressure();

    // Compute forces
    computePressureForces();
    computeViscosityForces();
    computeSurfaceTensionForces();

    // Integrate
    integrate(deltaTime);

    // Handle boundaries
    handleBoundaries();
}

void CompressibleFluidSimulation::render(Renderer* renderer) {
    if (!renderer || particles_.empty()) return;

    // Render fluid particles
    for (const auto& particle : particles_) {
        Vector3 color = particle.color * (0.5f + 0.5f * particle.pressure / 100.0f);
        renderer->renderParticle(particle.position, 0.02f, color);
    }
}

void CompressibleFluidSimulation::addFluidVolume(const Vector3& center, const Vector3& size, float density) {
    // Create particles in a volume
    const float spacing = params_.smoothingRadius * 0.8f; // Slightly less than smoothing radius
    const int particlesPerAxis = static_cast<int>(size.x / spacing);

    for (int x = 0; x < particlesPerAxis; ++x) {
        for (int y = 0; y < particlesPerAxis; ++y) {
            for (int z = 0; z < particlesPerAxis; ++z) {
                if (particles_.size() >= params_.maxParticles) return;

                Vector3 offset(
                    (x - particlesPerAxis/2.0f) * spacing,
                    (y - particlesPerAxis/2.0f) * spacing,
                    (z - particlesPerAxis/2.0f) * spacing
                );

                FluidParticle particle;
                particle.position = center + offset;
                particle.velocity = Vector3(0.0f, 0.0f, 0.0f);
                particle.mass = density * spacing * spacing * spacing;
                particle.temperature = 293.15f; // Room temperature

                particles_.push_back(particle);
            }
        }
    }
}

void CompressibleFluidSimulation::applyForce(const Vector3& position, const Vector3& force, float radius) {
    for (auto& particle : particles_) {
        Vector3 diff = particle.position - position;
        float distance = diff.length();

        if (distance < radius && distance > 0.0f) {
            float strength = 1.0f - (distance / radius);
            strength = strength * strength;

            particle.acceleration = particle.acceleration + (force * strength / particle.mass);
        }
    }
}

void CompressibleFluidSimulation::clear() {
    particles_.clear();
    for (auto& cell : spatialGrid_) {
        cell.particleIndices.clear();
    }
}

void CompressibleFluidSimulation::updateNeighbors() {
    // Clear neighbor lists
    for (auto& neighbors : neighborLists_) {
        neighbors.clear();
    }

    // Clear spatial grid
    for (auto& cell : spatialGrid_) {
        cell.particleIndices.clear();
    }

    // Populate spatial grid
    for (size_t i = 0; i < particles_.size(); ++i) {
        int gridIndex = getGridIndex(particles_[i].position);
        if (gridIndex >= 0 && gridIndex < spatialGrid_.size()) {
            spatialGrid_[gridIndex].particleIndices.push_back(i);
        }
    }

    // Find neighbors for each particle
    for (size_t i = 0; i < particles_.size(); ++i) {
        const Vector3& pos = particles_[i].position;

        // Check neighboring grid cells
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    Vector3 cellPos = pos + Vector3(dx, dy, dz) * params_.smoothingRadius;
                    int cellIndex = getGridIndex(cellPos);

                    if (cellIndex >= 0 && cellIndex < spatialGrid_.size()) {
                        for (size_t j : spatialGrid_[cellIndex].particleIndices) {
                            if (i != j) {
                                Vector3 diff = particles_[j].position - pos;
                                float distance = diff.length();

                                if (distance < params_.smoothingRadius) {
                                    neighborLists_[i].push_back(j);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void CompressibleFluidSimulation::computeDensityAndPressure() {
    for (size_t i = 0; i < particles_.size(); ++i) {
        float density = 0.0f;

        // Self density contribution
        density += particles_[i].mass * poly6Kernel(0.0f);

        // Neighbor contributions
        for (size_t j : neighborLists_[i]) {
            Vector3 diff = particles_[j].position - particles_[i].position;
            float distance = diff.length();

            if (distance < params_.smoothingRadius) {
                density += particles_[j].mass * poly6Kernel(distance);
            }
        }

        particles_[i].density = density;
        particles_[i].pressure = params_.gasConstant * (density - params_.restDensity);
    }
}

void CompressibleFluidSimulation::computePressureForces() {
    for (size_t i = 0; i < particles_.size(); ++i) {
        Vector3 pressureForce(0.0f);

        for (size_t j : neighborLists_[i]) {
            Vector3 diff = particles_[j].position - particles_[i].position;
            float distance = diff.length();

            if (distance > 0.0f && distance < params_.smoothingRadius) {
                Vector3 gradient = spikyKernelGradient(diff, distance);
                float pressureTerm = (particles_[i].pressure + particles_[j].pressure) /
                                   (2.0f * particles_[j].density);

                pressureForce = pressureForce - (gradient * particles_[j].mass * pressureTerm);
            }
        }

        pressureForces_[i] = pressureForce;
    }
}

void CompressibleFluidSimulation::computeViscosityForces() {
    for (size_t i = 0; i < particles_.size(); ++i) {
        Vector3 viscosityForce(0.0f);

        for (size_t j : neighborLists_[i]) {
            Vector3 diff = particles_[j].position - particles_[i].position;
            float distance = diff.length();

            if (distance > 0.0f && distance < params_.smoothingRadius) {
                Vector3 velocityDiff = particles_[j].velocity - particles_[i].velocity;
                Vector3 gradient = viscosityKernelLaplacian(diff, distance);

                viscosityForce = viscosityForce + (velocityDiff * params_.viscosity *
                                                 particles_[j].mass / particles_[j].density *
                                                 gradient.length());
            }
        }

        viscosityForces_[i] = viscosityForce;
    }
}

void CompressibleFluidSimulation::computeSurfaceTensionForces() {
    for (size_t i = 0; i < particles_.size(); ++i) {
        Vector3 surfaceTensionForce(0.0f);
        float colorFieldLaplacian = 0.0f;
        Vector3 colorFieldGradient(0.0f);

        // Compute color field and its derivatives
        for (size_t j : neighborLists_[i]) {
            Vector3 diff = particles_[j].position - particles_[i].position;
            float distance = diff.length();

            if (distance > 0.0f && distance < params_.smoothingRadius) {
                colorFieldLaplacian += particles_[j].mass / particles_[j].density *
                                     poly6KernelLaplacian(distance);
                colorFieldGradient = colorFieldGradient + (diff.normalized() *
                                  particles_[j].mass / particles_[j].density *
                                  poly6Kernel(distance));
            }
        }

        if (colorFieldGradient.length() > 0.1f) { // Surface particle
            surfaceTensionForce = colorFieldGradient.normalized() * colorFieldLaplacian *
                                params_.surfaceTension * -1.0f;
        }

        surfaceTensionForces_[i] = surfaceTensionForce;
    }
}

void CompressibleFluidSimulation::integrate(float deltaTime) {
    for (size_t i = 0; i < particles_.size(); ++i) {
        // Reset acceleration
        particles_[i].acceleration = params_.gravity;

        // Add forces
        particles_[i].acceleration = particles_[i].acceleration +
                                   (pressureForces_[i] / particles_[i].mass);
        particles_[i].acceleration = particles_[i].acceleration +
                                   (viscosityForces_[i] / particles_[i].mass);
        particles_[i].acceleration = particles_[i].acceleration +
                                   (surfaceTensionForces_[i] / particles_[i].mass);

        // Integrate velocity
        particles_[i].velocity = particles_[i].velocity + (particles_[i].acceleration * deltaTime);

        // Apply damping
        particles_[i].velocity = particles_[i].velocity * params_.damping;

        // Integrate position
        particles_[i].position = particles_[i].position + (particles_[i].velocity * deltaTime);
    }
}

void CompressibleFluidSimulation::handleBoundaries() {
    // Simple boundary handling - reflect off boundaries
    const float boundary = 5.0f;

    for (auto& particle : particles_) {
        if (particle.position.x < -boundary) {
            particle.position.x = -boundary;
            particle.velocity.x *= -0.5f;
        } else if (particle.position.x > boundary) {
            particle.position.x = boundary;
            particle.velocity.x *= -0.5f;
        }

        if (particle.position.y < -boundary) {
            particle.position.y = -boundary;
            particle.velocity.y *= -0.5f;
        } else if (particle.position.y > boundary) {
            particle.position.y = boundary;
            particle.velocity.y *= -0.5f;
        }

        if (particle.position.z < -boundary) {
            particle.position.z = -boundary;
            particle.velocity.z *= -0.5f;
        } else if (particle.position.z > boundary) {
            particle.position.z = boundary;
            particle.velocity.z *= -0.5f;
        }
    }
}

// SPH Kernel functions
float CompressibleFluidSimulation::poly6Kernel(float r) const {
    if (r >= params_.smoothingRadius) return 0.0f;

    float h = params_.smoothingRadius;
    float ratio = (h * h - r * r) / (h * h * h);
    return 315.0f / (64.0f * M_PI) * ratio * ratio * ratio;
}

Vector3 CompressibleFluidSimulation::spikyKernelGradient(const Vector3& r, float distance) const {
    if (distance >= params_.smoothingRadius || distance <= 0.0f) return Vector3(0.0f);

    float h = params_.smoothingRadius;
    Vector3 gradient = r.normalized() * -45.0f / M_PI *
                      powf(h - distance, 2.0f) / (h * h * h * h * h * h);
    return gradient;
}

Vector3 CompressibleFluidSimulation::viscosityKernelLaplacian(const Vector3& r, float distance) const {
    if (distance >= params_.smoothingRadius) return Vector3(0.0f);

    float h = params_.smoothingRadius;
    float laplacian = 45.0f / M_PI * (h - distance) / (h * h * h * h * h * h);
    return r * laplacian;
}

float CompressibleFluidSimulation::poly6KernelLaplacian(float r) const {
    if (r >= params_.smoothingRadius) return 0.0f;

    float h = params_.smoothingRadius;
    return 945.0f / (32.0f * M_PI) * (h * h - r * r) *
           (3.0f * h * h - 7.0f * r * r) / (h * h * h * h * h * h * h * h * h);
}

void CompressibleFluidSimulation::updateParticlesGPU(float deltaTime) {
    // GPU implementation would go here
    update(deltaTime); // Fall back to CPU
}

bool CompressibleFluidSimulation::initializeGPU() {
    return false; // Not implemented yet
}

void CompressibleFluidSimulation::cleanupGPU() {
    // GPU cleanup
}

int CompressibleFluidSimulation::getGridIndex(const Vector3& position) const {
    Vector3 localPos = position - gridOrigin_;
    int ix = static_cast<int>((localPos.x / gridSize_.x) * gridResolution_);
    int iy = static_cast<int>((localPos.y / gridSize_.y) * gridResolution_);
    int iz = static_cast<int>((localPos.z / gridSize_.z) * gridResolution_);

    if (ix < 0 || ix >= gridResolution_ ||
        iy < 0 || iy >= gridResolution_ ||
        iz < 0 || iz >= gridResolution_) {
        return -1;
    }

    return ix + iy * gridResolution_ + iz * gridResolution_ * gridResolution_;
}

bool CompressibleFluidSimulation::isInGrid(const Vector3& position) const {
    return getGridIndex(position) >= 0;
}

// Incompressible Fluid Simulation (FLIP) Implementation
IncompressibleFluidSimulation::IncompressibleFluidSimulation() {
}

IncompressibleFluidSimulation::~IncompressibleFluidSimulation() {
    cleanupGPU();
}

bool IncompressibleFluidSimulation::initialize(const SimulationParameters& params) {
    params_ = params;
    particles_.reserve(params_.maxParticles);

    // Initialize FLIP grids
    int size = params_.gridResolution;
    velocityGridU_.assign(size + 1, std::vector<std::vector<float>>(size, std::vector<float>(size, 0.0f)));
    velocityGridV_.assign(size, std::vector<std::vector<float>>(size + 1, std::vector<float>(size, 0.0f)));
    velocityGridW_.assign(size, std::vector<std::vector<float>>(size, std::vector<float>(size + 1, 0.0f)));
    pressureGrid_.assign(size, std::vector<std::vector<float>>(size, std::vector<float>(size, 0.0f)));
    divergenceGrid_.assign(size, std::vector<std::vector<float>>(size, std::vector<float>(size, 0.0f)));
    densityGrid_.assign(size, std::vector<std::vector<float>>(size, std::vector<float>(size, 0.0f)));
    particleCountGrid_.assign(size, std::vector<std::vector<float>>(size, std::vector<float>(size, 0.0f)));

    // Initialize GPU resources if requested
    if (params_.useGPUAcceleration) {
        if (!initializeGPU()) {
            std::cout << "GPU acceleration not available for FLIP, falling back to CPU" << std::endl;
            params_.useGPUAcceleration = false;
        }
    }

    return true;
}

void IncompressibleFluidSimulation::update(float deltaTime) {
    // Particles to grid
    particlesToGrid();

    // Compute divergence
    computeDivergence();

    // Solve pressure
    solvePressure();

    // Grid to particles
    gridToParticles();

    // Advect particles
    advectParticles(deltaTime);

    // Handle boundaries
    handleBoundaries();
}

void IncompressibleFluidSimulation::render(Renderer* renderer) {
    if (!renderer || particles_.empty()) return;

    // Render fluid particles
    for (const auto& particle : particles_) {
        Vector3 color = particle.color * particle.density;
        renderer->renderParticle(particle.position, 0.02f, color);
    }
}

void IncompressibleFluidSimulation::addFluidVolume(const Vector3& center, const Vector3& size) {
    const float spacing = params_.gridSize / static_cast<float>(params_.gridResolution);
    const int particlesPerAxis = static_cast<int>(size.x / spacing);

    for (int x = 0; x < particlesPerAxis; ++x) {
        for (int y = 0; y < particlesPerAxis; ++y) {
            for (int z = 0; z < particlesPerAxis; ++z) {
                if (particles_.size() >= params_.maxParticles) return;

                Vector3 offset(
                    (x - particlesPerAxis/2.0f) * spacing,
                    (y - particlesPerAxis/2.0f) * spacing,
                    (z - particlesPerAxis/2.0f) * spacing
                );

                FluidParticle particle;
                particle.position = center + offset;
                particle.velocity = Vector3(0.0f, 0.0f, 0.0f);
                particle.density = 1000.0f;

                particles_.push_back(particle);
            }
        }
    }
}

void IncompressibleFluidSimulation::applyForce(const Vector3& position, const Vector3& force, float radius) {
    for (auto& particle : particles_) {
        Vector3 diff = particle.position - position;
        float distance = diff.length();

        if (distance < radius && distance > 0.0f) {
            float strength = 1.0f - (distance / radius);
            strength = strength * strength;

            particle.velocity = particle.velocity + (force * strength * params_.timeStep);
        }
    }
}

void IncompressibleFluidSimulation::clear() {
    particles_.clear();
    // Clear all grids
    int size = params_.gridResolution;
    for (auto& grid : {velocityGridU_, velocityGridV_, velocityGridW_,
                       pressureGrid_, divergenceGrid_, densityGrid_, particleCountGrid_}) {
        for (auto& plane : grid) {
            for (auto& row : plane) {
                std::fill(row.begin(), row.end(), 0.0f);
            }
        }
    }
}

void IncompressibleFluidSimulation::particlesToGrid() {
    // Clear grids
    int size = params_.gridResolution;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            for (int k = 0; k < size; ++k) {
                velocityGridU_[i][j][k] = 0.0f;
                velocityGridV_[i][j][k] = 0.0f;
                velocityGridW_[i][j][k] = 0.0f;
                densityGrid_[i][j][k] = 0.0f;
                particleCountGrid_[i][j][k] = 0.0f;
            }
        }
    }

    // Transfer particle data to grid
    for (const auto& particle : particles_) {
        int i, j, k;
        worldToGrid(particle.position, i, j, k);

        if (isValidGridCell(i, j, k)) {
            velocityGridU_[i][j][k] += particle.velocity.x;
            velocityGridV_[i][j][k] += particle.velocity.y;
            velocityGridW_[i][j][k] += particle.velocity.z;
            densityGrid_[i][j][k] += particle.density;
            particleCountGrid_[i][j][k] += 1.0f;
        }
    }

    // Normalize by particle count
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            for (int k = 0; k < size; ++k) {
                if (particleCountGrid_[i][j][k] > 0.0f) {
                    velocityGridU_[i][j][k] /= particleCountGrid_[i][j][k];
                    velocityGridV_[i][j][k] /= particleCountGrid_[i][j][k];
                    velocityGridW_[i][j][k] /= particleCountGrid_[i][j][k];
                    densityGrid_[i][j][k] /= particleCountGrid_[i][j][k];
                }
            }
        }
    }
}

void IncompressibleFluidSimulation::computeDivergence() {
    int size = params_.gridResolution;
    float dx = params_.gridSize / static_cast<float>(size);

    for (int i = 1; i < size - 1; ++i) {
        for (int j = 1; j < size - 1; ++j) {
            for (int k = 1; k < size - 1; ++k) {
                float du_dx = (velocityGridU_[i+1][j][k] - velocityGridU_[i][j][k]) / dx;
                float dv_dy = (velocityGridV_[i][j+1][k] - velocityGridV_[i][j][k]) / dx;
                float dw_dz = (velocityGridW_[i][j][k+1] - velocityGridW_[i][j][k]) / dx;

                divergenceGrid_[i][j][k] = -(du_dx + dv_dy + dw_dz);
            }
        }
    }
}

void IncompressibleFluidSimulation::solvePressure() {
    int size = params_.gridResolution;
    float dx = params_.gridSize / static_cast<float>(size);

    // Jacobi iteration for pressure
    for (int iter = 0; iter < params_.pressureIterations; ++iter) {
        for (int i = 1; i < size - 1; ++i) {
            for (int j = 1; j < size - 1; ++j) {
                for (int k = 1; k < size - 1; ++k) {
                    float pressureSum = pressureGrid_[i-1][j][k] + pressureGrid_[i+1][j][k] +
                                      pressureGrid_[i][j-1][k] + pressureGrid_[i][j+1][k] +
                                      pressureGrid_[i][j][k-1] + pressureGrid_[i][j][k+1];

                    pressureGrid_[i][j][k] = (pressureSum - divergenceGrid_[i][j][k] * dx * dx) / 6.0f;
                }
            }
        }
    }
}

void IncompressibleFluidSimulation::gridToParticles() {
    int size = params_.gridResolution;
    float dx = params_.gridSize / static_cast<float>(size);

    for (auto& particle : particles_) {
        int i, j, k;
        worldToGrid(particle.position, i, j, k);

        if (isValidGridCell(i, j, k)) {
            // Apply pressure gradient to velocity
            float dp_dx = (pressureGrid_[i+1][j][k] - pressureGrid_[i-1][j][k]) / (2.0f * dx);
            float dp_dy = (pressureGrid_[i][j+1][k] - pressureGrid_[i][j-1][k]) / (2.0f * dx);
            float dp_dz = (pressureGrid_[i][j][k+1] - pressureGrid_[i][j][k-1]) / (2.0f * dx);

            particle.velocity.x -= dp_dx * params_.timeStep;
            particle.velocity.y -= dp_dy * params_.timeStep;
            particle.velocity.z -= dp_dz * params_.timeStep;
        }
    }
}

void IncompressibleFluidSimulation::advectParticles(float deltaTime) {
    for (auto& particle : particles_) {
        // Apply gravity
        particle.velocity = particle.velocity + (params_.gravity * deltaTime);

        // Apply viscosity
        particle.velocity = particle.velocity * (1.0f - params_.viscosity * deltaTime);

        // Update position
        particle.position = particle.position + (particle.velocity * deltaTime);
    }
}

void IncompressibleFluidSimulation::handleBoundaries() {
    float halfSize = params_.gridSize * 0.5f;

    for (auto& particle : particles_) {
        // Reflect off boundaries
        if (particle.position.x < -halfSize) {
            particle.position.x = -halfSize;
            particle.velocity.x *= -0.5f;
        } else if (particle.position.x > halfSize) {
            particle.position.x = halfSize;
            particle.velocity.x *= -0.5f;
        }

        if (particle.position.y < -halfSize) {
            particle.position.y = -halfSize;
            particle.velocity.y *= -0.5f;
        } else if (particle.position.y > halfSize) {
            particle.position.y = halfSize;
            particle.velocity.y *= -0.5f;
        }

        if (particle.position.z < -halfSize) {
            particle.position.z = -halfSize;
            particle.velocity.z *= -0.5f;
        } else if (particle.position.z > halfSize) {
            particle.position.z = halfSize;
            particle.velocity.z *= -0.5f;
        }
    }
}

void IncompressibleFluidSimulation::updateFluidGPU(float deltaTime) {
    // GPU implementation would go here
    update(deltaTime); // Fall back to CPU
}

bool IncompressibleFluidSimulation::initializeGPU() {
    return false; // Not implemented yet
}

void IncompressibleFluidSimulation::cleanupGPU() {
    // GPU cleanup
}

Vector3 IncompressibleFluidSimulation::gridToWorld(int i, int j, int k) const {
    float halfSize = params_.gridSize * 0.5f;
    float cellSize = params_.gridSize / static_cast<float>(params_.gridResolution);

    return Vector3(
        -halfSize + (i + 0.5f) * cellSize,
        -halfSize + (j + 0.5f) * cellSize,
        -halfSize + (k + 0.5f) * cellSize
    );
}

void IncompressibleFluidSimulation::worldToGrid(const Vector3& worldPos, int& i, int& j, int& k) const {
    float halfSize = params_.gridSize * 0.5f;
    float cellSize = params_.gridSize / static_cast<float>(params_.gridResolution);

    i = static_cast<int>((worldPos.x + halfSize) / cellSize);
    j = static_cast<int>((worldPos.y + halfSize) / cellSize);
    k = static_cast<int>((worldPos.z + halfSize) / cellSize);
}

bool IncompressibleFluidSimulation::isValidGridCell(int i, int j, int k) const {
    int size = params_.gridResolution;
    return i >= 0 && i < size && j >= 0 && j < size && k >= 0 && k < size;
}

// 2D Fluid Simulation (Lattice Boltzmann) Implementation
FluidSimulation2D::FluidSimulation2D() {
}

FluidSimulation2D::~FluidSimulation2D() {
    cleanupGPU();
}

bool FluidSimulation2D::initialize(const SimulationParameters2D& params) {
    params_ = params;

    // Initialize distribution functions
    f_.assign(Q, std::vector<std::vector<std::vector<float>>>(
        params_.gridHeight, std::vector<std::vector<float>>(params_.gridWidth, std::vector<float>(Q, 0.0f))));

    density_.assign(params_.gridHeight, std::vector<float>(params_.gridWidth, 1.0f));
    velocity_.assign(params_.gridHeight, std::vector<Vector2>(params_.gridWidth, Vector2(0.0f)));
    obstacle_.assign(params_.gridHeight, std::vector<bool>(params_.gridWidth, false));

    initializeLattice();

    // Initialize GPU resources if requested
    if (params_.useGPUAcceleration) {
        if (!initializeGPU()) {
            std::cout << "GPU acceleration not available for 2D LBM, falling back to CPU" << std::endl;
            params_.useGPUAcceleration = false;
        }
    }

    return true;
}

void FluidSimulation2D::update(float deltaTime) {
    // LBM steps
    collide();
    stream();
    applyBoundaryConditions();
    computeMacroscopic();
}

void FluidSimulation2D::render(Renderer* renderer) {
    if (!renderer) return;

    // Render fluid density as colors
    for (int y = 0; y < params_.gridHeight; ++y) {
        for (int x = 0; x < params_.gridWidth; ++x) {
            if (!obstacle_[y][x]) {
                float density = std::min(density_[y][x], 2.0f) / 2.0f;
                Vector3 color(density, density, 1.0f - density * 0.5f);
                Vector2 position(x * params_.gridSize, y * params_.gridSize);
                renderer->renderRectangle(position, Vector2(params_.gridSize), color);
            }
        }
    }
}

void FluidSimulation2D::addFlow(const Vector2& position, const Vector2& velocity, float radius) {
    int centerX = static_cast<int>(position.x / params_.gridSize);
    int centerY = static_cast<int>(position.y / params_.gridSize);
    int radiusCells = static_cast<int>(radius / params_.gridSize);

    for (int y = centerY - radiusCells; y <= centerY + radiusCells; ++y) {
        for (int x = centerX - radiusCells; x <= centerX + radiusCells; ++x) {
            if (x >= 0 && x < params_.gridWidth && y >= 0 && y < params_.gridHeight) {
                Vector2 diff(x - centerX, y - centerY);
                float distance = diff.length();

                if (distance <= radiusCells) {
                    float strength = 1.0f - (distance / radiusCells);
                    velocity_[y][x] = velocity_[y][x] + (velocity * strength);
                }
            }
        }
    }
}

void FluidSimulation2D::addObstacle(const Vector2& position, float radius) {
    int centerX = static_cast<int>(position.x / params_.gridSize);
    int centerY = static_cast<int>(position.y / params_.gridSize);
    int radiusCells = static_cast<int>(radius / params_.gridSize);

    for (int y = centerY - radiusCells; y <= centerY + radiusCells; ++y) {
        for (int x = centerX - radiusCells; x <= centerX + radiusCells; ++x) {
            if (x >= 0 && x < params_.gridWidth && y >= 0 && y < params_.gridHeight) {
                Vector2 diff(x - centerX, y - centerY);
                if (diff.length() <= radiusCells) {
                    obstacle_[y][x] = true;
                }
            }
        }
    }
}

void FluidSimulation2D::clear() {
    // Reset to initial state
    initializeLattice();
    for (auto& row : density_) {
        std::fill(row.begin(), row.end(), 1.0f);
    }
    for (auto& row : velocity_) {
        std::fill(row.begin(), row.end(), Vector2(0.0f));
    }
    for (auto& row : obstacle_) {
        std::fill(row.begin(), row.end(), false);
    }
}

void FluidSimulation2D::initializeLattice() {
    for (int y = 0; y < params_.gridHeight; ++y) {
        for (int x = 0; x < params_.gridWidth; ++x) {
            for (int i = 0; i < Q; ++i) {
                f_[i][y][x] = weights[i];
            }
        }
    }
}

void FluidSimulation2D::collide() {
    for (int y = 0; y < params_.gridHeight; ++y) {
        for (int x = 0; x < params_.gridWidth; ++x) {
            if (obstacle_[y][x]) continue;

            // Calculate equilibrium distribution
            float rho = density_[y][x];
            Vector2 u = velocity_[y][x];

            for (int i = 0; i < Q; ++i) {
                float dot = cx[i] * u.x + cy[i] * u.y;
                float usqr = u.x * u.x + u.y * u.y;

                float feq = weights[i] * rho * (1.0f + 3.0f * dot + 4.5f * dot * dot - 1.5f * usqr);

                // BGK collision
                f_[i][y][x] += (feq - f_[i][y][x]) / params_.relaxation;
            }
        }
    }
}

void FluidSimulation2D::stream() {
    // Create temporary arrays for streaming
    auto f_temp = f_;

    for (int y = 0; y < params_.gridHeight; ++y) {
        for (int x = 0; x < params_.gridWidth; ++x) {
            for (int i = 0; i < Q; ++i) {
                int nx = x + cx[i];
                int ny = y + cy[i];

                // Periodic boundaries
                if (nx < 0) nx = params_.gridWidth - 1;
                if (nx >= params_.gridWidth) nx = 0;
                if (ny < 0) ny = params_.gridHeight - 1;
                if (ny >= params_.gridHeight) ny = 0;

                f_temp[i][ny][nx] = f_[i][y][x];
            }
        }
    }

    f_ = std::move(f_temp);
}

void FluidSimulation2D::applyBoundaryConditions() {
    // Bounce-back boundary conditions for obstacles
    for (int y = 0; y < params_.gridHeight; ++y) {
        for (int x = 0; x < params_.gridWidth; ++x) {
            if (obstacle_[y][x]) {
                // Bounce-back: reverse direction
                f_[1][y][x] = f_[3][y][x]; // East <-> West
                f_[3][y][x] = f_[1][y][x];
                f_[2][y][x] = f_[4][y][x]; // North <-> South
                f_[4][y][x] = f_[2][y][x];
                f_[5][y][x] = f_[7][y][x]; // NE <-> SW
                f_[7][y][x] = f_[5][y][x];
                f_[6][y][x] = f_[8][y][x]; // NW <-> SE
                f_[8][y][x] = f_[6][y][x];
            }
        }
    }
}

void FluidSimulation2D::computeMacroscopic() {
    for (int y = 0; y < params_.gridHeight; ++y) {
        for (int x = 0; x < params_.gridWidth; ++x) {
            if (obstacle_[y][x]) continue;

            // Compute density
            density_[y][x] = 0.0f;
            for (int i = 0; i < Q; ++i) {
                density_[y][x] += f_[i][y][x];
            }

            // Compute velocity
            float momentum_x = 0.0f;
            float momentum_y = 0.0f;
            for (int i = 0; i < Q; ++i) {
                momentum_x += f_[i][y][x] * cx[i];
                momentum_y += f_[i][y][x] * cy[i];
            }

            if (density_[y][x] > 0.0f) {
                velocity_[y][x].x = momentum_x / density_[y][x];
                velocity_[y][x].y = momentum_y / density_[y][x];
            } else {
                velocity_[y][x] = Vector2(0.0f);
            }
        }
    }
}

void FluidSimulation2D::updateLatticeGPU(float deltaTime) {
    // GPU implementation would go here
    update(deltaTime); // Fall back to CPU
}

bool FluidSimulation2D::initializeGPU() {
    return false; // Not implemented yet
}

void FluidSimulation2D::cleanupGPU() {
    // GPU cleanup
}

} // namespace Foundry