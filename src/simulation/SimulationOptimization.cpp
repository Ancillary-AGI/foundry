/**
 * @file SimulationOptimization.cpp
 * @brief Performance optimizations for simulation algorithms
 */

#include "../../include/GameEngine/simulation/SimulationOptimization.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// Thread pool for parallel processing
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop = false;

public:
    ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty()) return;

                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");

            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }
};

// Spatial partitioning optimizations
SpatialPartition::SpatialPartition(float cellSize, const Vector3& bounds)
    : cellSize_(cellSize), bounds_(bounds) {
    gridSize_.x = static_cast<int>(std::ceil(bounds.x / cellSize));
    gridSize_.y = static_cast<int>(std::ceil(bounds.y / cellSize));
    gridSize_.z = static_cast<int>(std::ceil(bounds.z / cellSize));

    int totalCells = gridSize_.x * gridSize_.y * gridSize_.z;
    cells_.resize(totalCells);
}

SpatialPartition::~SpatialPartition() {
}

void SpatialPartition::clear() {
    for (auto& cell : cells_) {
        cell.clear();
    }
}

void SpatialPartition::insert(const Vector3& position, int particleIndex) {
    int cellIndex = getCellIndex(position);
    if (cellIndex >= 0 && cellIndex < cells_.size()) {
        cells_[cellIndex].push_back(particleIndex);
    }
}

std::vector<int> SpatialPartition::query(const Vector3& position, float radius) const {
    std::vector<int> result;

    // Calculate affected cells
    int minX = std::max(0, static_cast<int>((position.x - radius) / cellSize_));
    int maxX = std::min(gridSize_.x - 1, static_cast<int>((position.x + radius) / cellSize_));
    int minY = std::max(0, static_cast<int>((position.y - radius) / cellSize_));
    int maxY = std::min(gridSize_.y - 1, static_cast<int>((position.y + radius) / cellSize_));
    int minZ = std::max(0, static_cast<int>((position.z - radius) / cellSize_));
    int maxZ = std::min(gridSize_.z - 1, static_cast<int>((position.z + radius) / cellSize_));

    for (int z = minZ; z <= maxZ; ++z) {
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                int cellIndex = getCellIndex(x, y, z);
                if (cellIndex >= 0 && cellIndex < cells_.size()) {
                    // Check each particle in the cell
                    for (int particleIndex : cells_[cellIndex]) {
                        // In practice, you'd check distance here
                        result.push_back(particleIndex);
                    }
                }
            }
        }
    }

    return result;
}

int SpatialPartition::getCellIndex(const Vector3& position) const {
    int x = static_cast<int>(position.x / cellSize_);
    int y = static_cast<int>(position.y / cellSize_);
    int z = static_cast<int>(position.z / cellSize_);

    return getCellIndex(x, y, z);
}

int SpatialPartition::getCellIndex(int x, int y, int z) const {
    if (x < 0 || x >= gridSize_.x || y < 0 || y >= gridSize_.y || z < 0 || z >= gridSize_.z) {
        return -1;
    }

    return x + y * gridSize_.x + z * gridSize_.x * gridSize_.y;
}

// SIMD-accelerated math operations
namespace SIMDMath {

#ifdef __AVX2__
// AVX2 implementations
__m256 vectorAdd(__m256 a, __m256 b) {
    return _mm256_add_ps(a, b);
}

__m256 vectorMultiply(__m256 a, __m256 b) {
    return _mm256_mul_ps(a, b);
}

__m256 vectorLength(__m256 x, __m256 y, __m256 z) {
    __m256 x2 = _mm256_mul_ps(x, x);
    __m256 y2 = _mm256_mul_ps(y, y);
    __m256 z2 = _mm256_mul_ps(z, z);
    __m256 sum = _mm256_add_ps(_mm256_add_ps(x2, y2), z2);
    return _mm256_sqrt_ps(sum);
}

float horizontalAdd(__m256 v) {
    __m128 hi = _mm256_extractf128_ps(v, 1);
    __m128 lo = _mm256_castps256_ps128(v);
    lo = _mm_add_ps(lo, hi);
    hi = _mm_movehl_ps(lo, lo);
    lo = _mm_add_ss(lo, hi);
    return _mm_cvtss_f32(lo);
}

#else
// Fallback scalar implementations
__m256 vectorAdd(__m256 a, __m256 b) {
    // Fallback to scalar operations
    float* a_ptr = reinterpret_cast<float*>(&a);
    float* b_ptr = reinterpret_cast<float*>(&b);
    float result[8];
    for (int i = 0; i < 8; ++i) {
        result[i] = a_ptr[i] + b_ptr[i];
    }
    return *reinterpret_cast<__m256*>(result);
}

__m256 vectorMultiply(__m256 a, __m256 b) {
    float* a_ptr = reinterpret_cast<float*>(&a);
    float* b_ptr = reinterpret_cast<float*>(&b);
    float result[8];
    for (int i = 0; i < 8; ++i) {
        result[i] = a_ptr[i] * b_ptr[i];
    }
    return *reinterpret_cast<__m256*>(result);
}

__m256 vectorLength(__m256 x, __m256 y, __m256 z) {
    float* x_ptr = reinterpret_cast<float*>(&x);
    float* y_ptr = reinterpret_cast<float*>(&y);
    float* z_ptr = reinterpret_cast<float*>(&z);
    float result[8];
    for (int i = 0; i < 8; ++i) {
        result[i] = std::sqrt(x_ptr[i] * x_ptr[i] + y_ptr[i] * y_ptr[i] + z_ptr[i] * z_ptr[i]);
    }
    return *reinterpret_cast<__m256*>(result);
}

float horizontalAdd(__m256 v) {
    float* v_ptr = reinterpret_cast<float*>(&v);
    float sum = 0.0f;
    for (int i = 0; i < 8; ++i) {
        sum += v_ptr[i];
    }
    return sum;
}
#endif

} // namespace SIMDMath

// Optimized particle system with spatial partitioning and SIMD
OptimizedParticleSystem::OptimizedParticleSystem(int maxParticles, float cellSize, const Vector3& bounds)
    : maxParticles_(maxParticles), spatialPartition_(cellSize, bounds) {
    particles_.resize(maxParticles);
    threadPool_ = std::make_unique<ThreadPool>(std::thread::hardware_concurrency());
}

OptimizedParticleSystem::~OptimizedParticleSystem() {
}

void OptimizedParticleSystem::update(float deltaTime) {
    // Clear spatial partition
    spatialPartition_.clear();

    // Insert particles into spatial partition
    for (int i = 0; i < activeParticles_; ++i) {
        spatialPartition_.insert(particles_[i].position, i);
    }

    // Update particles in parallel
    const int batchSize = 256;
    std::vector<std::future<void>> futures;

    for (int start = 0; start < activeParticles_; start += batchSize) {
        int end = std::min(start + batchSize, activeParticles_);

        futures.push_back(threadPool_->enqueue([this, start, end, deltaTime]() {
            updateParticleBatch(start, end, deltaTime);
        }));
    }

    // Wait for all batches to complete
    for (auto& future : futures) {
        future.wait();
    }

    // Remove dead particles
    removeDeadParticles();
}

void OptimizedParticleSystem::updateParticleBatch(int start, int end, float deltaTime) {
    for (int i = start; i < end; ++i) {
        Particle& particle = particles_[i];

        // Apply forces from nearby particles using spatial partitioning
        std::vector<int> nearbyParticles = spatialPartition_.query(particle.position, influenceRadius_);

        for (int j : nearbyParticles) {
            if (i != j) {
                applyParticleInteraction(particle, particles_[j], deltaTime);
            }
        }

        // Update physics
        particle.velocity = particle.velocity + (particle.acceleration * deltaTime);
        particle.position = particle.position + (particle.velocity * deltaTime);

        // Apply damping
        particle.velocity = particle.velocity * damping_;

        // Update life
        particle.life -= deltaTime;

        // Reset acceleration
        particle.acceleration = Vector3(0.0f, 0.0f, 0.0f);
    }
}

void OptimizedParticleSystem::applyParticleInteraction(Particle& a, const Particle& b, float deltaTime) {
    Vector3 diff = b.position - a.position;
    float distance = diff.length();

    if (distance > 0.0f && distance < influenceRadius_) {
        float force = interactionStrength_ / (distance * distance + 1.0f);
        Vector3 forceVector = diff.normalized() * force;

        a.acceleration = a.acceleration + forceVector;
    }
}

void OptimizedParticleSystem::removeDeadParticles() {
    int writeIndex = 0;
    for (int readIndex = 0; readIndex < activeParticles_; ++readIndex) {
        if (particles_[readIndex].life > 0.0f) {
            particles_[writeIndex++] = particles_[readIndex];
        }
    }
    activeParticles_ = writeIndex;
}

void OptimizedParticleSystem::addParticle(const Particle& particle) {
    if (activeParticles_ < maxParticles_) {
        particles_[activeParticles_++] = particle;
    }
}

// Memory pool for efficient allocation
MemoryPool::MemoryPool(size_t blockSize, size_t blockCount)
    : blockSize_(blockSize), blockCount_(blockCount) {
    pool_ = new char[blockSize_ * blockCount_];
    freeBlocks_ = new bool[blockCount_];

    for (size_t i = 0; i < blockCount_; ++i) {
        freeBlocks_[i] = true;
    }
}

MemoryPool::~MemoryPool() {
    delete[] pool_;
    delete[] freeBlocks_;
}

void* MemoryPool::allocate() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (size_t i = 0; i < blockCount_; ++i) {
        if (freeBlocks_[i]) {
            freeBlocks_[i] = false;
            return pool_ + i * blockSize_;
        }
    }

    return nullptr; // No free blocks
}

void MemoryPool::deallocate(void* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(mutex_);

    size_t offset = static_cast<char*>(ptr) - pool_;
    size_t blockIndex = offset / blockSize_;

    if (blockIndex < blockCount_) {
        freeBlocks_[blockIndex] = true;
    }
}

// Cache-friendly data structures
CacheFriendlyParticleBuffer::CacheFriendlyParticleBuffer(size_t maxParticles)
    : maxParticles_(maxParticles) {
    // Allocate SoA (Struct of Arrays) layout for better cache performance
    positionsX_ = new float[maxParticles];
    positionsY_ = new float[maxParticles];
    positionsZ_ = new float[maxParticles];

    velocitiesX_ = new float[maxParticles];
    velocitiesY_ = new float[maxParticles];
    velocitiesZ_ = new float[maxParticles];

    accelerationsX_ = new float[maxParticles];
    accelerationsY_ = new float[maxParticles];
    accelerationsZ_ = new float[maxParticles];

    masses_ = new float[maxParticles];
    lives_ = new float[maxParticles];
}

CacheFriendlyParticleBuffer::~CacheFriendlyParticleBuffer() {
    delete[] positionsX_;
    delete[] positionsY_;
    delete[] positionsZ_;

    delete[] velocitiesX_;
    delete[] velocitiesY_;
    delete[] velocitiesZ_;

    delete[] accelerationsX_;
    delete[] accelerationsY_;
    delete[] accelerationsZ_;

    delete[] masses_;
    delete[] lives_;
}

void CacheFriendlyParticleBuffer::updateParticlesSIMD(size_t count, float deltaTime) {
    const size_t vectorWidth = 8; // AVX2
    size_t vectorizedCount = (count / vectorWidth) * vectorWidth;

    // Vectorized update for position and velocity
    for (size_t i = 0; i < vectorizedCount; i += vectorWidth) {
        // Load data
        __m256 posX = _mm256_load_ps(&positionsX_[i]);
        __m256 posY = _mm256_load_ps(&positionsY_[i]);
        __m256 posZ = _mm256_load_ps(&positionsZ_[i]);

        __m256 velX = _mm256_load_ps(&velocitiesX_[i]);
        __m256 velY = _mm256_load_ps(&velocitiesY_[i]);
        __m256 velZ = _mm256_load_ps(&velocitiesZ_[i]);

        __m256 accX = _mm256_load_ps(&accelerationsX_[i]);
        __m256 accY = _mm256_load_ps(&accelerationsY_[i]);
        __m256 accZ = _mm256_load_ps(&accelerationsZ_[i]);

        __m256 dt = _mm256_set1_ps(deltaTime);

        // Update velocity: v = v + a * dt
        __m256 newVelX = SIMDMath::vectorAdd(velX, SIMDMath::vectorMultiply(accX, dt));
        __m256 newVelY = SIMDMath::vectorAdd(velY, SIMDMath::vectorMultiply(accY, dt));
        __m256 newVelZ = SIMDMath::vectorAdd(velZ, SIMDMath::vectorMultiply(accZ, dt));

        // Update position: p = p + v * dt
        __m256 newPosX = SIMDMath::vectorAdd(posX, SIMDMath::vectorMultiply(newVelX, dt));
        __m256 newPosY = SIMDMath::vectorAdd(posY, SIMDMath::vectorMultiply(newVelY, dt));
        __m256 newPosZ = SIMDMath::vectorAdd(posZ, SIMDMath::vectorMultiply(newVelZ, dt));

        // Store results
        _mm256_store_ps(&positionsX_[i], newPosX);
        _mm256_store_ps(&positionsY_[i], newPosY);
        _mm256_store_ps(&positionsZ_[i], newPosZ);

        _mm256_store_ps(&velocitiesX_[i], newVelX);
        _mm256_store_ps(&velocitiesY_[i], newVelY);
        _mm256_store_ps(&velocitiesZ_[i], newVelZ);

        // Reset accelerations
        __m256 zero = _mm256_setzero_ps();
        _mm256_store_ps(&accelerationsX_[i], zero);
        _mm256_store_ps(&accelerationsY_[i], zero);
        _mm256_store_ps(&accelerationsZ_[i], zero);
    }

    // Handle remaining particles with scalar operations
    for (size_t i = vectorizedCount; i < count; ++i) {
        velocitiesX_[i] += accelerationsX_[i] * deltaTime;
        velocitiesY_[i] += accelerationsY_[i] * deltaTime;
        velocitiesZ_[i] += accelerationsZ_[i] * deltaTime;

        positionsX_[i] += velocitiesX_[i] * deltaTime;
        positionsY_[i] += velocitiesY_[i] * deltaTime;
        positionsZ_[i] += velocitiesZ_[i] * deltaTime;

        accelerationsX_[i] = 0.0f;
        accelerationsY_[i] = 0.0f;
        accelerationsZ_[i] = 0.0f;
    }
}

// Adaptive quality system
AdaptiveQualitySystem::AdaptiveQualitySystem()
    : targetFrameTime_(1.0f / 60.0f), // 60 FPS
      currentQuality_(1.0f),
      minQuality_(0.1f),
      maxQuality_(1.0f) {
}

AdaptiveQualitySystem::~AdaptiveQualitySystem() {
}

void AdaptiveQualitySystem::update(float frameTime, float deltaTime) {
    // Calculate frame time ratio
    float frameTimeRatio = frameTime / targetFrameTime_;

    // Adjust quality based on performance
    if (frameTimeRatio > 1.2f) {
        // Too slow, reduce quality
        currentQuality_ = std::max(minQuality_, currentQuality_ * 0.95f);
    } else if (frameTimeRatio < 0.8f) {
        // Fast enough, can increase quality
        currentQuality_ = std::min(maxQuality_, currentQuality_ * 1.02f);
    }

    // Smooth quality changes
    static float smoothedQuality = currentQuality_;
    smoothedQuality = smoothedQuality * 0.9f + currentQuality_ * 0.1f;
    currentQuality_ = smoothedQuality;
}

float AdaptiveQualitySystem::getQualityFactor() const {
    return currentQuality_;
}

void AdaptiveQualitySystem::setQualitySettings(float particleCount, float simulationSteps, float renderDistance) {
    // Adjust simulation parameters based on quality
    adjustedParticleCount_ = static_cast<int>(particleCount * currentQuality_);
    adjustedSimulationSteps_ = static_cast<int>(simulationSteps * currentQuality_);
    adjustedRenderDistance_ = renderDistance * currentQuality_;
}

// Performance profiler
PerformanceProfiler::PerformanceProfiler() {
    frameTimes_.reserve(1000);
}

PerformanceProfiler::~PerformanceProfiler() {
}

void PerformanceProfiler::startFrame() {
    frameStart_ = std::chrono::high_resolution_clock::now();
}

void PerformanceProfiler::endFrame() {
    auto frameEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> frameDuration = frameEnd - frameStart_;

    frameTimes_.push_back(frameDuration.count());

    // Keep only recent frames
    if (frameTimes_.size() > 1000) {
        frameTimes_.erase(frameTimes_.begin());
    }
}

float PerformanceProfiler::getAverageFrameTime() const {
    if (frameTimes_.empty()) return 0.0f;

    float sum = 0.0f;
    for (float time : frameTimes_) {
        sum += time;
    }

    return sum / frameTimes_.size();
}

float PerformanceProfiler::getFPS() const {
    float avgFrameTime = getAverageFrameTime();
    return avgFrameTime > 0.0f ? 1.0f / avgFrameTime : 0.0f;
}

float PerformanceProfiler::getFrameTimePercentile(float percentile) const {
    if (frameTimes_.empty()) return 0.0f;

    std::vector<float> sortedTimes = frameTimes_;
    std::sort(sortedTimes.begin(), sortedTimes.end());

    size_t index = static_cast<size_t>(sortedTimes.size() * percentile / 100.0f);
    index = std::min(index, sortedTimes.size() - 1);

    return sortedTimes[index];
}

// Optimized rendering with LOD and culling
OptimizedRenderer::OptimizedRenderer(Renderer* baseRenderer)
    : baseRenderer_(baseRenderer) {
}

OptimizedRenderer::~OptimizedRenderer() {
}

void OptimizedRenderer::renderParticles(const std::vector<Particle>& particles, const Vector3& cameraPosition) {
    // Frustum culling
    std::vector<const Particle*> visibleParticles;
    visibleParticles.reserve(particles.size());

    for (const auto& particle : particles) {
        if (isVisible(particle.position, cameraPosition)) {
            visibleParticles.push_back(&particle);
        }
    }

    // LOD based on distance
    std::vector<std::vector<const Particle*>> lodLevels(4);

    for (const Particle* particle : visibleParticles) {
        float distance = (particle->position - cameraPosition).length();
        int lodLevel = calculateLODLevel(distance);
        if (lodLevel < lodLevels.size()) {
            lodLevels[lodLevel].push_back(particle);
        }
    }

    // Render each LOD level
    for (int lod = 0; lod < lodLevels.size(); ++lod) {
        if (!lodLevels[lod].empty()) {
            renderParticleLOD(lodLevels[lod], lod);
        }
    }
}

bool OptimizedRenderer::isVisible(const Vector3& position, const Vector3& cameraPosition) const {
    // Simple distance-based culling (in practice, use frustum culling)
    float distance = (position - cameraPosition).length();
    return distance < renderDistance_;
}

int OptimizedRenderer::calculateLODLevel(float distance) const {
    if (distance < 10.0f) return 0;      // High detail
    if (distance < 50.0f) return 1;      // Medium detail
    if (distance < 100.0f) return 2;     // Low detail
    return 3;                            // Very low detail
}

void OptimizedRenderer::renderParticleLOD(const std::vector<const Particle*>& particles, int lodLevel) {
    // Adjust rendering quality based on LOD
    float sizeMultiplier = 1.0f / (1.0f + lodLevel * 0.5f);
    int detailLevel = std::max(1, 8 >> lodLevel); // Reduce detail for distant particles

    for (const Particle* particle : particles) {
        baseRenderer_->renderParticle(particle->position,
                                    particle->size * sizeMultiplier,
                                    particle->color);
    }
}

// Factory functions
std::unique_ptr<SimulationOptimizer> SimulationOptimizer::create() {
    return std::make_unique<SimulationOptimizer>();
}

SimulationOptimizer::SimulationOptimizer() {
    threadPool_ = std::make_unique<ThreadPool>(std::thread::hardware_concurrency());
    profiler_ = std::make_unique<PerformanceProfiler>();
    adaptiveQuality_ = std::make_unique<AdaptiveQualitySystem>();
}

SimulationOptimizer::~SimulationOptimizer() {
}

void SimulationOptimizer::optimizeSimulation(SimulationSystem* system) {
    // Analyze current performance
    profiler_->startFrame();

    // Apply optimizations
    if (system->supportsParallelProcessing()) {
        enableParallelProcessing(system);
    }

    if (system->supportsSpatialPartitioning()) {
        enableSpatialPartitioning(system);
    }

    if (system->supportsSIMD()) {
        enableSIMD(system);
    }

    // Apply adaptive quality
    adaptiveQuality_->update(profiler_->getAverageFrameTime(), 1.0f / 60.0f);
    applyAdaptiveQuality(system);

    profiler_->endFrame();
}

void SimulationOptimizer::enableParallelProcessing(SimulationSystem* system) {
    // Configure thread pool usage
    system->setThreadPool(threadPool_.get());
}

void SimulationOptimizer::enableSpatialPartitioning(SimulationSystem* system) {
    // Create and configure spatial partition
    Vector3 bounds(100.0f, 100.0f, 100.0f);
    auto spatialPartition = std::make_unique<SpatialPartition>(2.0f, bounds);
    system->setSpatialPartition(std::move(spatialPartition));
}

void SimulationOptimizer::enableSIMD(SimulationSystem* system) {
    // Enable SIMD operations
    system->enableSIMD(true);
}

void SimulationOptimizer::applyAdaptiveQuality(SimulationSystem* system) {
    float quality = adaptiveQuality_->getQualityFactor();
    system->setQualityFactor(quality);
}