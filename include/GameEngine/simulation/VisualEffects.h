/**
 * @file VisualEffects.h
 * @brief Advanced visual effects system with GPU acceleration
 */

#pragma once

#include "../math/Vector3.h"
#include "../math/Vector2.h"
#include "../graphics/Renderer.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace Foundry {

/**
 * @brief Particle system for visual effects
 * Time Complexity: O(n) per frame, Space Complexity: O(n)
 * GPU-accelerated for high-performance effects
 */
class ParticleSystem {
public:
    struct Particle {
        Vector3 position;
        Vector3 velocity;
        Vector3 acceleration;
        float life;
        float maxLife;
        Vector3 color;
        float size;
        float rotation;
        float rotationSpeed;

        Particle() :
            life(0.0f),
            maxLife(1.0f),
            color(Vector3(1.0f, 1.0f, 1.0f)),
            size(1.0f),
            rotation(0.0f),
            rotationSpeed(0.0f) {}
    };

    struct Emitter {
        Vector3 position;
        Vector3 direction;
        float spread;
        float rate;
        float speed;
        float speedVariation;
        float life;
        float lifeVariation;
        Vector3 color;
        float size;
        float sizeVariation;
        bool active;
    };

    struct SimulationParameters {
        float timeStep = 1.0f / 60.0f;
        Vector3 gravity = Vector3(0.0f, -9.81f, 0.0f);
        float damping = 0.99f;
        int maxParticles = 10000;
        bool useGPUAcceleration = true;
    };

private:
    std::vector<Particle> particles_;
    std::vector<Emitter> emitters_;
    SimulationParameters params_;

    // GPU compute resources
    void* gpuParticlesBuffer_ = nullptr;
    void* gpuEmittersBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    ParticleSystem();
    ~ParticleSystem();

    bool initialize(const SimulationParameters& params);
    void update(float deltaTime);
    void render(Renderer* renderer);

    void addEmitter(const Emitter& emitter);
    void removeEmitter(size_t index);
    void clearEmitters();

    size_t getParticleCount() const { return particles_.size(); }
    size_t getEmitterCount() const { return emitters_.size(); }

    void setParameters(const SimulationParameters& params) { params_ = params; }
    const SimulationParameters& getParameters() const { return params_; }

    void clear();

private:
    void emitParticles(float deltaTime);
    void updateParticles(float deltaTime);
    void removeDeadParticles();

    void updateParticlesGPU(float deltaTime);
    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Volumetric lighting and fog effects
 * Time Complexity: O(width * height * depth), Space Complexity: O(width * height * depth)
 */
class VolumetricEffects {
public:
    struct FogParameters {
        Vector3 color;
        float density;
        float startDistance;
        float endDistance;
        bool useNoise;
        float noiseScale;
        float noiseSpeed;
    };

    struct LightShaftParameters {
        Vector3 lightPosition;
        Vector3 lightDirection;
        Vector3 lightColor;
        float intensity;
        float scattering;
        int samples;
    };

private:
    FogParameters fogParams_;
    LightShaftParameters lightShaftParams_;

    // 3D texture for volumetric data
    std::vector<float> volumeData_;
    int volumeWidth_;
    int volumeHeight_;
    int volumeDepth_;

    // GPU compute resources
    void* gpuVolumeBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    VolumetricEffects();
    ~VolumetricEffects();

    bool initialize(int width, int height, int depth);
    void update(float deltaTime);
    void render(Renderer* renderer, const Vector3& cameraPosition);

    void setFogParameters(const FogParameters& params) { fogParams_ = params; }
    void setLightShaftParameters(const LightShaftParameters& params) { lightShaftParams_ = params; }

    const FogParameters& getFogParameters() const { return fogParams_; }
    const LightShaftParameters& getLightShaftParameters() const { return lightShaftParams_; }

private:
    void updateVolumeData(float deltaTime);
    void computeLightShafts();
    void applyFog();

    void updateVolumeGPU(float deltaTime);
    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Screen space ambient occlusion (SSAO)
 * Time Complexity: O(width * height), Space Complexity: O(width * height)
 */
class SSAOEffect {
public:
    struct SSAOParameters {
        float radius;
        float bias;
        float intensity;
        int kernelSize;
        int blurSize;
    };

private:
    SSAOParameters params_;

    // SSAO textures
    std::vector<float> ssaoBuffer_;
    std::vector<float> blurBuffer_;
    std::vector<Vector3> kernel_;
    std::vector<float> noise_;

    // GPU compute resources
    void* gpuDepthBuffer_ = nullptr;
    void* gpuNormalBuffer_ = nullptr;
    void* gpuSSAOBuffers_[2] = {nullptr, nullptr};
    void* gpuKernelBuffer_ = nullptr;
    void* gpuNoiseBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    SSAOEffect();
    ~SSAOEffect();

    bool initialize(int width, int height);
    void computeSSAO(const std::vector<float>& depthBuffer,
                    const std::vector<Vector3>& normalBuffer,
                    const Matrix4& projectionMatrix,
                    const Vector3& cameraPosition);
    void render(Renderer* renderer);

    void setParameters(const SSAOParameters& params) { params_ = params; }
    const SSAOParameters& getParameters() const { return params_; }

private:
    void generateKernel();
    void generateNoise();
    void blurSSAO();

    void computeSSAOGPU(const std::vector<float>& depthBuffer,
                       const std::vector<Vector3>& normalBuffer,
                       const Matrix4& projectionMatrix,
                       const Vector3& cameraPosition);
    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Screen space reflections (SSR)
 * Time Complexity: O(width * height * maxSteps), Space Complexity: O(width * height)
 */
class SSREffect {
public:
    struct SSRParameters {
        int maxSteps;
        float stepSize;
        float maxDistance;
        float fadeStart;
        float fadeEnd;
        float roughnessThreshold;
    };

private:
    SSRParameters params_;

    // SSR textures
    std::vector<Vector3> reflectionBuffer_;
    std::vector<float> roughnessBuffer_;

    // GPU compute resources
    void* gpuColorBuffer_ = nullptr;
    void* gpuDepthBuffer_ = nullptr;
    void* gpuNormalBuffer_ = nullptr;
    void* gpuRoughnessBuffer_ = nullptr;
    void* gpuReflectionBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    SSREffect();
    ~SSREffect();

    bool initialize(int width, int height);
    void computeSSR(const std::vector<Vector3>& colorBuffer,
                   const std::vector<float>& depthBuffer,
                   const std::vector<Vector3>& normalBuffer,
                   const std::vector<float>& roughnessBuffer,
                   const Matrix4& projectionMatrix,
                   const Matrix4& viewMatrix);
    void render(Renderer* renderer);

    void setParameters(const SSRParameters& params) { params_ = params; }
    const SSRParameters& getParameters() const { return params_; }

private:
    Vector3 traceReflection(const Vector3& position, const Vector3& reflectionDir,
                           const std::vector<float>& depthBuffer,
                           const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const;

    void computeSSR_GPU(const std::vector<Vector3>& colorBuffer,
                       const std::vector<float>& depthBuffer,
                       const std::vector<Vector3>& normalBuffer,
                       const std::vector<float>& roughnessBuffer,
                       const Matrix4& projectionMatrix,
                       const Matrix4& viewMatrix);
    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Motion blur effect
 * Time Complexity: O(width * height), Space Complexity: O(width * height)
 */
class MotionBlurEffect {
public:
    struct MotionBlurParameters {
        float intensity;
        int samples;
        bool useVelocityBuffer;
    };

private:
    MotionBlurParameters params_;

    // Motion blur textures
    std::vector<Vector3> velocityBuffer_;
    std::vector<Vector3> blurBuffer_;

    // GPU compute resources
    void* gpuColorBuffer_ = nullptr;
    void* gpuVelocityBuffer_ = nullptr;
    void* gpuBlurBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    MotionBlurEffect();
    ~MotionBlurEffect();

    bool initialize(int width, int height);
    void computeMotionBlur(const std::vector<Vector3>& colorBuffer,
                          const std::vector<Vector3>& velocityBuffer,
                          const Matrix4& previousViewProjection,
                          const Matrix4& currentViewProjection);
    void render(Renderer* renderer);

    void setParameters(const MotionBlurParameters& params) { params_ = params; }
    const MotionBlurParameters& getParameters() const { return params_; }

private:
    void computeVelocityFromMatrices(const Matrix4& previousVP, const Matrix4& currentVP);
    void applyMotionBlur();

    void computeMotionBlurGPU(const std::vector<Vector3>& colorBuffer,
                             const std::vector<Vector3>& velocityBuffer);
    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Depth of field effect
 * Time Complexity: O(width * height * kernel_size), Space Complexity: O(width * height)
 */
class DepthOfFieldEffect {
public:
    struct DOFParameters {
        float focusDistance;
        float focusRange;
        float blurRadius;
        int kernelSize;
        bool useBokeh;
    };

private:
    DOFParameters params_;

    // DOF textures
    std::vector<Vector3> cocBuffer_; // Circle of confusion
    std::vector<Vector3> blurBuffer_;

    // GPU compute resources
    void* gpuColorBuffer_ = nullptr;
    void* gpuDepthBuffer_ = nullptr;
    void* gpuCOCBuffer_ = nullptr;
    void* gpuBlurBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    DepthOfFieldEffect();
    ~DepthOfFieldEffect();

    bool initialize(int width, int height);
    void computeDOF(const std::vector<Vector3>& colorBuffer,
                   const std::vector<float>& depthBuffer,
                   const Matrix4& projectionMatrix);
    void render(Renderer* renderer);

    void setParameters(const DOFParameters& params) { params_ = params; }
    const DOFParameters& getParameters() const { return params_; }

private:
    void computeCircleOfConfusion(const std::vector<float>& depthBuffer, const Matrix4& projectionMatrix);
    void applyGaussianBlur();
    void applyBokehBlur();

    void computeDOFGPU(const std::vector<Vector3>& colorBuffer,
                      const std::vector<float>& depthBuffer,
                      const Matrix4& projectionMatrix);
    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Visual effects manager
 * Coordinates multiple visual effects with optimal performance
 */
class VisualEffectsManager {
private:
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unique_ptr<VolumetricEffects> volumetricEffects_;
    std::unique_ptr<SSAOEffect> ssaoEffect_;
    std::unique_ptr<SSREffect> ssrEffect_;
    std::unique_ptr<MotionBlurEffect> motionBlurEffect_;
    std::unique_ptr<DepthOfFieldEffect> dofEffect_;

    bool effectsEnabled_;
    float globalQuality_;

public:
    VisualEffectsManager();
    ~VisualEffectsManager();

    bool initialize(int width, int height, int depth = 32);
    void update(float deltaTime);
    void render(Renderer* renderer, const Vector3& cameraPosition);

    // Effect management
    void enableEffect(const std::string& effectName, bool enabled);
    bool isEffectEnabled(const std::string& effectName) const;

    void setGlobalQuality(float quality) { globalQuality_ = quality; }
    float getGlobalQuality() const { return globalQuality_; }

    // Access to individual effects
    ParticleSystem* getParticleSystem() const { return particleSystem_.get(); }
    VolumetricEffects* getVolumetricEffects() const { return volumetricEffects_.get(); }
    SSAOEffect* getSSAOEffect() const { return ssaoEffect_.get(); }
    SSREffect* getSSREffect() const { return ssrEffect_.get(); }
    MotionBlurEffect* getMotionBlurEffect() const { return motionBlurEffect_.get(); }
    DepthOfFieldEffect* getDOFEffect() const { return dofEffect_.get(); }

private:
    void updateQualitySettings();
};

} // namespace Foundry