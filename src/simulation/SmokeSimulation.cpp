/**
 * @file SmokeSimulation.cpp
 * @brief Implementation of high-performance smoke simulation with Brownian motion
 */

#include "../../include/GameEngine/simulation/SmokeSimulation.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

namespace Foundry {

SmokeSimulation::SmokeSimulation() :
    isRunning_(false) {
}

SmokeSimulation::~SmokeSimulation() {
    if (isRunning_) {
        shutdown();
    }
    cleanupGPU();
}

bool SmokeSimulation::initialize(const SimulationParameters& params) {
    params_ = params;
    particles_.reserve(params_.maxParticles);

    // Initialize spatial grid
    gridSize_ = Vector3(10.0f, 10.0f, 10.0f); // 10x10x10 meter simulation space
    gridOrigin_ = Vector3(-5.0f, -5.0f, -5.0f);
    spatialGrid_.resize(gridResolution_ * gridResolution_ * gridResolution_);

    // Initialize random number generator for Brownian motion
    rng_.seed(std::random_device{}());
    normalDist_ = std::normal_distribution<float>(0.0f, 1.0f);

    // Initialize GPU resources if requested
    if (params_.useGPUAcceleration) {
        if (!initializeGPU()) {
            std::cout << "GPU acceleration not available, falling back to CPU" << std::endl;
            params_.useGPUAcceleration = false;
        }
    }

    isRunning_ = true;
    return true;
}

void SmokeSimulation::shutdown() {
    isRunning_ = false;
    if (simulationThread_.joinable()) {
        simulationThread_.join();
    }
    clear();
}

void SmokeSimulation::update(float deltaTime) {
    if (!isRunning_) return;

    // Spawn new particles
    spawnParticles();

    // Update simulation
    if (params_.useGPUAcceleration) {
        updateParticlesGPU(deltaTime);
    } else {
        updateParticles(deltaTime);
    }

    // Update spatial grid
    updateSpatialGrid();

    // Handle collisions and merging
    handleCollisions();

    // Apply diffusion
    applyDiffusion();

    // Remove dead particles
    removeDeadParticles();
}

void SmokeSimulation::render(Renderer* renderer) {
    if (!renderer || particles_.empty()) return;

    // Render particles as billboards or point sprites
    for (const auto& particle : particles_) {
        if (particle.density > 0.01f) { // Only render visible particles
            // Set particle color based on temperature and density
            Vector3 color = particle.color;
            color = color * particle.density * (0.5f + 0.5f * particle.temperature);

            // Render particle (implementation depends on renderer API)
            renderer->renderParticle(particle.position, particle.size, color);
        }
    }
}

void SmokeSimulation::addEmitter(const Vector3& position, float intensity, float temperature) {
    // Add particles at emitter position
    int numParticles = static_cast<int>(intensity * params_.spawnRate * params_.timeStep);

    for (int i = 0; i < numParticles; ++i) {
        if (particles_.size() >= params_.maxParticles) break;

        SmokeParticle particle;
        particle.position = position;

        // Add some initial randomness to position
        particle.position.x += (normalDist_(rng_) * 0.1f);
        particle.position.y += (normalDist_(rng_) * 0.1f);
        particle.position.z += (normalDist_(rng_) * 0.1f);

        particle.velocity = Vector3(0.0f, 0.0f, 0.0f);
        particle.density = intensity;
        particle.temperature = temperature;
        particle.age = 0.0f;
        particle.lifetime = 2.0f + normalDist_(rng_) * 0.5f; // 2-2.5 seconds lifetime
        particle.size = 0.1f + normalDist_(rng_) * 0.05f;

        particles_.push_back(particle);
    }
}

void SmokeSimulation::applyForce(const Vector3& position, const Vector3& force, float radius) {
    for (auto& particle : particles_) {
        Vector3 diff = particle.position - position;
        float distance = diff.length();

        if (distance < radius && distance > 0.0f) {
            float strength = 1.0f - (distance / radius);
            strength = strength * strength; // Quadratic falloff

            particle.velocity = particle.velocity + (force * strength * params_.timeStep);
        }
    }
}

void SmokeSimulation::clear() {
    particles_.clear();
    for (auto& cell : spatialGrid_) {
        cell.particleIndices.clear();
        cell.densitySum = 0.0f;
        cell.velocitySum = Vector3(0.0f);
    }
}

void SmokeSimulation::spawnParticles() {
    // This method is called when emitters are active
    // Particles are spawned via addEmitter method
}

void SmokeSimulation::updateParticles(float deltaTime) {
    for (auto& particle : particles_) {
        // Apply gravity
        particle.velocity = particle.velocity + (params_.gravity * deltaTime);

        // Apply buoyancy based on temperature
        float buoyancyForce = params_.buoyancy * (particle.temperature - 0.5f);
        particle.velocity.y += buoyancyForce * deltaTime;

        // Apply Brownian motion
        applyBrownianMotionToParticle(particle, deltaTime);

        // Update position
        particle.position = particle.position + (particle.velocity * deltaTime);

        // Update temperature and density
        particle.temperature *= params_.temperatureDecay;
        particle.density *= params_.densityDecay;

        // Update age
        particle.age += deltaTime;

        // Update color based on temperature
        float tempFactor = std::min(particle.temperature, 1.0f);
        particle.color = Vector3(
            0.5f + tempFactor * 0.5f,  // Red increases with temperature
            0.5f - tempFactor * 0.3f,  // Green decreases with temperature
            0.8f - tempFactor * 0.6f   // Blue decreases with temperature
        );
    }
}

void SmokeSimulation::applyBrownianMotion(float deltaTime) {
    for (auto& particle : particles_) {
        applyBrownianMotionToParticle(particle, deltaTime);
    }
}

void SmokeSimulation::applyBrownianMotionToParticle(SmokeParticle& particle, float deltaTime) {
    // Generate random Brownian motion
    Vector3 brownianForce(
        normalDist_(rng_) * params_.brownianMotionScale,
        normalDist_(rng_) * params_.brownianMotionScale,
        normalDist_(rng_) * params_.brownianMotionScale
    );

    // Scale by temperature (hotter particles move more)
    brownianForce = brownianForce * (0.5f + particle.temperature);

    // Apply to velocity
    particle.velocity = particle.velocity + (brownianForce * deltaTime);
}

void SmokeSimulation::updateSpatialGrid() {
    // Clear grid
    for (auto& cell : spatialGrid_) {
        cell.particleIndices.clear();
        cell.densitySum = 0.0f;
        cell.velocitySum = Vector3(0.0f);
    }

    // Populate grid
    for (size_t i = 0; i < particles_.size(); ++i) {
        int gridIndex = getGridIndex(particles_[i].position);
        if (gridIndex >= 0 && gridIndex < spatialGrid_.size()) {
            spatialGrid_[gridIndex].particleIndices.push_back(i);
            spatialGrid_[gridIndex].densitySum += particles_[i].density;
            spatialGrid_[gridIndex].velocitySum = spatialGrid_[gridIndex].velocitySum + particles_[i].velocity;
        }
    }
}

void SmokeSimulation::handleCollisions() {
    // Simple collision handling - merge nearby particles
    for (auto& cell : spatialGrid_) {
        if (cell.particleIndices.size() > 1) {
            // Calculate average position and properties
            Vector3 avgPosition(0.0f);
            Vector3 avgVelocity(0.0f);
            float totalDensity = 0.0f;
            float totalTemperature = 0.0f;
            float totalMass = 0.0f;

            for (size_t particleIndex : cell.particleIndices) {
                const auto& particle = particles_[particleIndex];
                avgPosition = avgPosition + (particle.position * particle.density);
                avgVelocity = avgVelocity + (particle.velocity * particle.density);
                totalDensity += particle.density;
                totalTemperature += particle.temperature * particle.density;
                totalMass += particle.density;
            }

            if (totalMass > 0.0f) {
                avgPosition = avgPosition / totalMass;
                avgVelocity = avgVelocity / totalMass;
                totalTemperature /= totalMass;

                // Create merged particle
                SmokeParticle mergedParticle;
                mergedParticle.position = avgPosition;
                mergedParticle.velocity = avgVelocity;
                mergedParticle.density = std::min(totalDensity, 2.0f); // Cap density
                mergedParticle.temperature = totalTemperature;
                mergedParticle.age = 0.0f;
                mergedParticle.lifetime = 3.0f; // Merged particles live longer
                mergedParticle.size = 0.15f; // Slightly larger
                mergedParticle.color = Vector3(0.4f, 0.4f, 0.4f); // Darker color

                // Remove old particles and add merged one
                for (auto it = cell.particleIndices.rbegin(); it != cell.particleIndices.rend(); ++it) {
                    particles_.erase(particles_.begin() + *it);
                }
                particles_.push_back(mergedParticle);
            }
        }
    }
}

void SmokeSimulation::applyDiffusion() {
    // Simple diffusion using spatial grid
    for (auto& cell : spatialGrid_) {
        if (!cell.particleIndices.empty()) {
            // Diffuse density to neighboring cells
            float avgDensity = cell.densitySum / cell.particleIndices.size();
            float diffusionAmount = avgDensity * params_.diffusionRate * params_.timeStep;

            // Apply diffusion to particles in this cell
            for (size_t particleIndex : cell.particleIndices) {
                if (particleIndex < particles_.size()) {
                    particles_[particleIndex].density += diffusionAmount * 0.1f;
                    particles_[particleIndex].density = std::max(0.0f, particles_[particleIndex].density);
                }
            }
        }
    }
}

void SmokeSimulation::removeDeadParticles() {
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
            [this](const SmokeParticle& particle) {
                return particle.age >= particle.lifetime || particle.density < 0.001f;
            }),
        particles_.end()
    );
}

void SmokeSimulation::updateParticlesGPU(float deltaTime) {
    // GPU implementation would go here
    // For now, fall back to CPU implementation
    updateParticles(deltaTime);
}

bool SmokeSimulation::initializeGPU() {
    // GPU initialization would go here
    // Return false to indicate GPU not available
    return false;
}

void SmokeSimulation::cleanupGPU() {
    // GPU cleanup would go here
}

int SmokeSimulation::getGridIndex(const Vector3& position) const {
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

bool SmokeSimulation::isInGrid(const Vector3& position) const {
    return getGridIndex(position) >= 0;
}

// 2D Smoke Simulation Implementation
SmokeSimulation2D::SmokeSimulation2D() {
}

SmokeSimulation2D::~SmokeSimulation2D() {
    cleanupGPU();
}

bool SmokeSimulation2D::initialize(const SimulationParameters2D& params) {
    params_ = params;
    particles_.reserve(params_.maxParticles);

    // Initialize density and velocity grids
    densityGrid_.resize(gridHeight_, std::vector<float>(gridWidth_, 0.0f));
    velocityGrid_.resize(gridHeight_, std::vector<Vector2>(gridWidth_, Vector2(0.0f)));

    // Initialize GPU resources if requested
    if (params_.useGPUAcceleration) {
        if (!initializeGPU()) {
            std::cout << "GPU acceleration not available for 2D smoke, falling back to CPU" << std::endl;
            params_.useGPUAcceleration = false;
        }
    }

    return true;
}

void SmokeSimulation2D::update(float deltaTime) {
    // Spawn new particles
    spawnParticles();

    // Update particles
    updateParticles(deltaTime);

    // Apply Brownian motion
    applyBrownianMotion(deltaTime);

    // Update density grid
    updateDensityGrid();

    // Apply diffusion
    applyDiffusion();

    // Advect velocity field
    advectVelocity();

    // Remove dead particles
    removeDeadParticles();
}

void SmokeSimulation2D::render(Renderer* renderer) {
    if (!renderer || particles_.empty()) return;

    // Render particles
    for (const auto& particle : particles_) {
        if (particle.density > 0.01f) {
            Vector3 color = particle.color * particle.density;
            renderer->renderParticle2D(Vector2(particle.position.x, particle.position.y),
                                     particle.size, color);
        }
    }
}

void SmokeSimulation2D::addEmitter(const Vector2& position, float intensity, float temperature) {
    int numParticles = static_cast<int>(intensity * params_.spawnRate * params_.timeStep);

    for (int i = 0; i < numParticles; ++i) {
        if (particles_.size() >= params_.maxParticles) break;

        SmokeParticle2D particle;
        particle.position = position;
        particle.velocity = Vector2(0.0f, 0.0f);
        particle.density = intensity;
        particle.temperature = temperature;
        particle.age = 0.0f;
        particle.lifetime = 2.0f;
        particle.size = 0.1f;

        particles_.push_back(particle);
    }
}

void SmokeSimulation2D::applyForce(const Vector2& position, const Vector2& force, float radius) {
    for (auto& particle : particles_) {
        Vector2 diff = particle.position - position;
        float distance = diff.length();

        if (distance < radius && distance > 0.0f) {
            float strength = 1.0f - (distance / radius);
            strength = strength * strength;

            particle.velocity = particle.velocity + (force * strength * params_.timeStep);
        }
    }
}

void SmokeSimulation2D::clear() {
    particles_.clear();
    for (auto& row : densityGrid_) {
        std::fill(row.begin(), row.end(), 0.0f);
    }
    for (auto& row : velocityGrid_) {
        std::fill(row.begin(), row.end(), Vector2(0.0f));
    }
}

void SmokeSimulation2D::spawnParticles() {
    // Particles spawned via addEmitter
}

void SmokeSimulation2D::updateParticles(float deltaTime) {
    for (auto& particle : particles_) {
        // Apply gravity
        particle.velocity = particle.velocity + (params_.gravity * deltaTime);

        // Apply buoyancy
        float buoyancyForce = params_.buoyancy * (particle.temperature - 0.5f);
        particle.velocity.y += buoyancyForce * deltaTime;

        // Update position
        particle.position = particle.position + (particle.velocity * deltaTime);

        // Update properties
        particle.temperature *= params_.temperatureDecay;
        particle.density *= params_.densityDecay;
        particle.age += deltaTime;
    }
}

void SmokeSimulation2D::applyBrownianMotion(float deltaTime) {
    std::mt19937 rng(std::random_device{}());
    std::normal_distribution<float> dist(0.0f, 1.0f);

    for (auto& particle : particles_) {
        Vector2 brownianForce(
            dist(rng) * params_.brownianMotionScale,
            dist(rng) * params_.brownianMotionScale
        );

        brownianForce = brownianForce * (0.5f + particle.temperature);
        particle.velocity = particle.velocity + (brownianForce * deltaTime);
    }
}

void SmokeSimulation2D::updateDensityGrid() {
    // Clear grid
    for (auto& row : densityGrid_) {
        std::fill(row.begin(), row.end(), 0.0f);
    }

    // Populate grid from particles
    float cellSize = 1.0f; // Assume 1 unit per cell
    for (const auto& particle : particles_) {
        int ix = static_cast<int>(particle.position.x / cellSize);
        int iy = static_cast<int>(particle.position.y / cellSize);

        if (ix >= 0 && ix < gridWidth_ && iy >= 0 && iy < gridHeight_) {
            densityGrid_[iy][ix] += particle.density;
        }
    }
}

void SmokeSimulation2D::applyDiffusion() {
    std::vector<std::vector<float>> newDensity = densityGrid_;

    for (int y = 1; y < gridHeight_ - 1; ++y) {
        for (int x = 1; x < gridWidth_ - 1; ++x) {
            // Simple Laplacian diffusion
            float laplacian = densityGrid_[y][x-1] + densityGrid_[y][x+1] +
                            densityGrid_[y-1][x] + densityGrid_[y+1][x] -
                            4.0f * densityGrid_[y][x];

            newDensity[y][x] += laplacian * params_.diffusionRate * params_.timeStep;
        }
    }

    densityGrid_ = std::move(newDensity);
}

void SmokeSimulation2D::advectVelocity() {
    // Semi-Lagrangian advection for velocity field
    std::vector<std::vector<Vector2>> newVelocity = velocityGrid_;

    for (int y = 1; y < gridHeight_ - 1; ++y) {
        for (int x = 1; x < gridWidth_ - 1; ++x) {
            // Get velocity at current position
            Vector2 vel = velocityGrid_[y][x];

            // Calculate back-traced position
            float dt = params_.timeStep;
            Vector2 backPos = Vector2(static_cast<float>(x), static_cast<float>(y)) - vel * dt;

            // Bilinear interpolation to get velocity at back-traced position
            int x0 = static_cast<int>(std::floor(backPos.x));
            int y0 = static_cast<int>(std::floor(backPos.y));
            int x1 = x0 + 1;
            int y1 = y0 + 1;

            // Clamp to grid bounds
            x0 = std::max(0, std::min(gridWidth_ - 1, x0));
            x1 = std::max(0, std::min(gridWidth_ - 1, x1));
            y0 = std::max(0, std::min(gridHeight_ - 1, y0));
            y1 = std::max(0, std::min(gridHeight_ - 1, y1));

            float fx = backPos.x - std::floor(backPos.x);
            float fy = backPos.y - std::floor(backPos.y);

            // Interpolate velocity
            Vector2 v00 = velocityGrid_[y0][x0];
            Vector2 v10 = velocityGrid_[y0][x1];
            Vector2 v01 = velocityGrid_[y1][x0];
            Vector2 v11 = velocityGrid_[y1][x1];

            Vector2 velX0 = v00 * (1.0f - fx) + v10 * fx;
            Vector2 velX1 = v01 * (1.0f - fx) + v11 * fx;
            newVelocity[y][x] = velX0 * (1.0f - fy) + velX1 * fy;
        }
    }

    velocityGrid_ = std::move(newVelocity);
}

void SmokeSimulation2D::removeDeadParticles() {
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
            [this](const SmokeParticle2D& particle) {
                return particle.age >= particle.lifetime || particle.density < 0.001f;
            }),
        particles_.end()
    );
}

bool SmokeSimulation2D::initializeGPU() {
    // GPU initialization for 2D smoke
    return false; // Not implemented yet
}

void SmokeSimulation2D::cleanupGPU() {
    // GPU cleanup
}

} // namespace Foundry