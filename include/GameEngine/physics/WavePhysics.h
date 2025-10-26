/**
 * @file WavePhysics.h
 * @brief Advanced wave physics simulation system
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include <memory>
#include <vector>
#include <complex>

namespace FoundryEngine {

/**
 * @class WavePhysics
 * @brief Comprehensive wave physics simulation including sound, water, electromagnetic waves
 */
class WavePhysics : public System {
public:
    enum class WaveType {
        Sound,
        Water,
        Electromagnetic,
        Seismic,
        Quantum
    };
    
    struct WaveConfig {
        WaveType type = WaveType::Sound;
        float frequency = 440.0f; // Hz
        float amplitude = 1.0f;
        float wavelength = 1.0f; // meters
        float speed = 343.0f; // m/s (sound in air)
        Vector3 direction = Vector3(1, 0, 0);
        bool enableInterference = true;
        bool enableDiffraction = true;
        bool enableReflection = true;
        bool enableRefraction = true;
    };
    
    struct WaveField {
        std::vector<std::complex<float>> amplitudes;
        std::vector<Vector3> positions;
        std::vector<float> phases;
        uint32_t gridWidth;
        uint32_t gridHeight;
        uint32_t gridDepth;
        float cellSize;
    };
    
    struct WaveSource {
        uint32_t id;
        Vector3 position;
        WaveConfig config;
        bool isActive;
        float currentPhase;
        float timeActive;
    };
    
    WavePhysics();
    ~WavePhysics();
    
    bool initialize() override;
    void shutdown() override;
    void update(float deltaTime) override;
    
    // Wave source management
    uint32_t createWaveSource(const Vector3& position, const WaveConfig& config);
    void destroyWaveSource(uint32_t sourceId);
    void setWaveSourcePosition(uint32_t sourceId, const Vector3& position);
    void setWaveSourceFrequency(uint32_t sourceId, float frequency);
    void setWaveSourceAmplitude(uint32_t sourceId, float amplitude);
    void activateWaveSource(uint32_t sourceId, bool active);
    
    // Wave field simulation
    void initializeWaveField(uint32_t width, uint32_t height, uint32_t depth, float cellSize);
    void updateWaveField(float deltaTime);
    WaveField getWaveField() const;
    
    // Wave propagation
    void propagateWaves(float deltaTime);
    std::complex<float> getWaveAmplitudeAt(const Vector3& position) const;
    Vector3 getWaveVelocityAt(const Vector3& position) const;
    float getWaveIntensityAt(const Vector3& position) const;
    
    // Wave interactions
    void calculateInterference();
    void calculateDiffraction();
    void calculateReflection(const Vector3& surfaceNormal, float reflectivity);
    void calculateRefraction(float refractiveIndex1, float refractiveIndex2);
    
    // Sound waves specific
    void setSoundSpeed(float speed); // Temperature dependent
    void setAirDensity(float density);
    float calculateDopplerShift(const Vector3& sourceVel, const Vector3& observerVel, 
                               const Vector3& sourcePos, const Vector3& observerPos) const;
    
    // Water waves specific
    void setWaterDepth(float depth);
    void setWaterDensity(float density);
    void calculateWaterWaveDispersion();
    Vector3 calculateWaterParticleMotion(const Vector3& position, float time) const;
    
    // Electromagnetic waves specific
    void setPermittivity(float permittivity);
    void setPermeability(float permeability);
    Vector3 calculateElectricField(const Vector3& position, float time) const;
    Vector3 calculateMagneticField(const Vector3& position, float time) const;
    
    // FFT-based ocean simulation
    void initializeOceanFFT(uint32_t resolution, float oceanSize);
    void updateOceanFFT(float time);
    float getOceanHeightAt(float x, float z, float time) const;
    Vector3 getOceanNormalAt(float x, float z, float time) const;
    
    // Wave equation solvers
    void solveWaveEquation1D(float* field, uint32_t size, float deltaTime, float speed);
    void solveWaveEquation2D(float* field, uint32_t width, uint32_t height, 
                            float deltaTime, float speed);
    void solveWaveEquation3D(float* field, uint32_t width, uint32_t height, uint32_t depth,
                            float deltaTime, float speed);
    
    // Boundary conditions
    void setAbsorbingBoundary(bool enable);
    void setReflectiveBoundary(bool enable);
    void setPeriodicBoundary(bool enable);
    
    // Performance and visualization
    struct WavePhysicsStats {
        uint32_t activeSources = 0;
        uint32_t waveFieldSize = 0;
        float computeTime = 0.0f;
        uint32_t fftOperations = 0;
        float maxAmplitude = 0.0f;
        float totalEnergy = 0.0f;
    };
    
    WavePhysicsStats getStats() const;
    void resetStats();
    
    // Visualization helpers
    std::vector<Vector3> getWaveVisualizationData() const;
    std::vector<float> getWaveHeightField() const;

private:
    class WavePhysicsImpl;
    std::unique_ptr<WavePhysicsImpl> impl_;
};

} // namespace FoundryEngine