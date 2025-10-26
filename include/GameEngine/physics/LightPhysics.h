/**
 * @file LightPhysics.h
 * @brief Advanced light physics simulation system
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include <memory>
#include <vector>

namespace FoundryEngine {

/**
 * @class LightPhysics
 * @brief Realistic light physics simulation with photon mapping and global illumination
 */
class LightPhysics : public System {
public:
    struct LightPhysicsConfig {
        uint32_t maxPhotons = 1000000;
        uint32_t maxBounces = 8;
        float photonEnergy = 1.0f;
        bool enableCaustics = true;
        bool enableVolumetricScattering = true;
        bool enableSpectralRendering = false;
        float lightSpeed = 299792458.0f; // m/s
    };
    
    struct PhotonMap {
        std::vector<Vector3> positions;
        std::vector<Vector3> directions;
        std::vector<Vector3> colors;
        std::vector<float> energies;
        std::vector<uint32_t> bounceCount;
    };
    
    struct LightRay {
        Vector3 origin;
        Vector3 direction;
        Vector3 color;
        float intensity;
        float wavelength; // For spectral rendering
        uint32_t bounces;
    };
    
    LightPhysics();
    ~LightPhysics();
    
    bool initialize(const LightPhysicsConfig& config = LightPhysicsConfig{}) override;
    void shutdown() override;
    void update(float deltaTime) override;
    
    // Light source management
    uint32_t createPointLight(const Vector3& position, const Vector3& color, float intensity);
    uint32_t createDirectionalLight(const Vector3& direction, const Vector3& color, float intensity);
    uint32_t createSpotLight(const Vector3& position, const Vector3& direction, 
                            const Vector3& color, float intensity, float angle);
    uint32_t createAreaLight(const Vector3& position, const Vector3& normal, 
                            float width, float height, const Vector3& color, float intensity);
    
    void destroyLight(uint32_t lightId);
    void setLightPosition(uint32_t lightId, const Vector3& position);
    void setLightColor(uint32_t lightId, const Vector3& color);
    void setLightIntensity(uint32_t lightId, float intensity);
    
    // Photon mapping
    void generatePhotonMap();
    void tracePhotons(uint32_t lightId, uint32_t photonCount);
    PhotonMap getPhotonMap() const;
    void clearPhotonMap();
    
    // Ray tracing
    Vector3 traceRay(const LightRay& ray) const;
    std::vector<LightRay> generateRays(uint32_t lightId, uint32_t rayCount) const;
    
    // Global illumination
    Vector3 calculateGlobalIllumination(const Vector3& position, const Vector3& normal) const;
    Vector3 calculateDirectIllumination(const Vector3& position, const Vector3& normal) const;
    Vector3 calculateIndirectIllumination(const Vector3& position, const Vector3& normal) const;
    
    // Caustics
    void generateCaustics();
    Vector3 getCausticsContribution(const Vector3& position) const;
    
    // Volumetric scattering
    Vector3 calculateVolumetricScattering(const Vector3& start, const Vector3& end) const;
    void setVolumetricProperties(float density, float scattering, float absorption);
    
    // Material interaction
    void registerMaterial(uint32_t materialId, float reflectance, float transmittance, 
                         float roughness, float ior);
    Vector3 calculateMaterialInteraction(const LightRay& ray, uint32_t materialId, 
                                       const Vector3& normal) const;
    
    // Performance
    struct LightPhysicsStats {
        uint32_t photonsTraced = 0;
        uint32_t raysTraced = 0;
        uint32_t lightBounces = 0;
        float computeTime = 0.0f;
        uint32_t activeLights = 0;
    };
    
    LightPhysicsStats getStats() const;
    void resetStats();

private:
    class LightPhysicsImpl;
    std::unique_ptr<LightPhysicsImpl> impl_;
};

} // namespace FoundryEngine