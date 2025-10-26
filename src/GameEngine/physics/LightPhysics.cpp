/**
 * @file LightPhysics.cpp
 * @brief Implementation of advanced light physics simulation
 */

#include "GameEngine/physics/LightPhysics.h"
#include <algorithm>
#include <random>
#include <thread>
#include <execution>

namespace FoundryEngine {

class LightPhysics::LightPhysicsImpl {
public:
    LightPhysicsConfig config_;
    
    // Light sources
    std::vector<std::unique_ptr<LightSource>> lightSources_;
    std::atomic<uint32_t> nextLightId_{1};
    
    // Photon mapping
    PhotonMap globalPhotonMap_;
    PhotonMap causticsPhotonMap_;
    
    // Materials
    std::unordered_map<uint32_t, MaterialProperties> materials_;
    
    // Volumetric properties
    float volumetricDensity_ = 0.01f;
    float scatteringCoefficient_ = 0.1f;
    float absorptionCoefficient_ = 0.05f;
    
    // Performance stats
    LightPhysicsStats stats_;
    
    // Threading
    std::vector<std::thread> photonThreads_;
    std::atomic<bool> isRunning_{false};
    
    struct LightSource {
        uint32_t id;
        Vector3 position;
        Vector3 direction;
        Vector3 color;
        float intensity;
        LightType type;
        float angle; // For spot lights
        float width, height; // For area lights
        bool isActive;
    };
    
    struct MaterialProperties {
        float reflectance;
        float transmittance;
        float roughness;
        float ior; // Index of refraction
        Vector3 albedo;
    };
    
    enum class LightType {
        Point,
        Directional,
        Spot,
        Area
    };
};

LightPhysics::LightPhysics() : impl_(std::make_unique<LightPhysicsImpl>()) {}

LightPhysics::~LightPhysics() = default;

bool LightPhysics::initialize(const LightPhysicsConfig& config) {
    impl_->config_ = config;
    
    // Initialize photon maps
    impl_->globalPhotonMap_.positions.reserve(config.maxPhotons);
    impl_->globalPhotonMap_.directions.reserve(config.maxPhotons);
    impl_->globalPhotonMap_.colors.reserve(config.maxPhotons);
    impl_->globalPhotonMap_.energies.reserve(config.maxPhotons);
    impl_->globalPhotonMap_.bounceCount.reserve(config.maxPhotons);
    
    if (config.enableCaustics) {
        impl_->causticsPhotonMap_.positions.reserve(config.maxPhotons / 4);
        impl_->causticsPhotonMap_.directions.reserve(config.maxPhotons / 4);
        impl_->causticsPhotonMap_.colors.reserve(config.maxPhotons / 4);
        impl_->causticsPhotonMap_.energies.reserve(config.maxPhotons / 4);
        impl_->causticsPhotonMap_.bounceCount.reserve(config.maxPhotons / 4);
    }
    
    // Register default materials
    registerMaterial(0, 0.8f, 0.0f, 0.1f, 1.0f); // Default diffuse
    registerMaterial(1, 0.9f, 0.0f, 0.0f, 1.0f); // Mirror
    registerMaterial(2, 0.1f, 0.9f, 0.0f, 1.5f); // Glass
    
    impl_->isRunning_ = true;
    return true;
}

void LightPhysics::shutdown() {
    impl_->isRunning_ = false;
    
    // Wait for photon threads to finish
    for (auto& thread : impl_->photonThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    impl_->lightSources_.clear();
    impl_->globalPhotonMap_ = PhotonMap{};
    impl_->causticsPhotonMap_ = PhotonMap{};
}

void LightPhysics::update(float deltaTime) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Update light sources
    updateLightSources(deltaTime);
    
    // Generate photon map if needed
    if (shouldRegeneratePhotonMap()) {
        generatePhotonMap();
    }
    
    // Update caustics if enabled
    if (impl_->config_.enableCaustics) {
        generateCaustics();
    }
    
    // Update performance stats
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    impl_->stats_.computeTime = duration.count() / 1000.0f; // Convert to milliseconds
}

uint32_t LightPhysics::createPointLight(const Vector3& position, const Vector3& color, float intensity) {
    auto light = std::make_unique<LightPhysicsImpl::LightSource>();
    light->id = impl_->nextLightId_++;
    light->position = position;
    light->color = color;
    light->intensity = intensity;
    light->type = LightPhysicsImpl::LightType::Point;
    light->isActive = true;
    
    uint32_t lightId = light->id;
    impl_->lightSources_.push_back(std::move(light));
    impl_->stats_.activeLights++;
    
    return lightId;
}

uint32_t LightPhysics::createDirectionalLight(const Vector3& direction, const Vector3& color, float intensity) {
    auto light = std::make_unique<LightPhysicsImpl::LightSource>();
    light->id = impl_->nextLightId_++;
    light->direction = direction.normalized();
    light->color = color;
    light->intensity = intensity;
    light->type = LightPhysicsImpl::LightType::Directional;
    light->isActive = true;
    
    uint32_t lightId = light->id;
    impl_->lightSources_.push_back(std::move(light));
    impl_->stats_.activeLights++;
    
    return lightId;
}

uint32_t LightPhysics::createAreaLight(const Vector3& position, const Vector3& normal, 
                                      float width, float height, const Vector3& color, float intensity) {
    auto light = std::make_unique<LightPhysicsImpl::LightSource>();
    light->id = impl_->nextLightId_++;
    light->position = position;
    light->direction = normal.normalized();
    light->color = color;
    light->intensity = intensity;
    light->width = width;
    light->height = height;
    light->type = LightPhysicsImpl::LightType::Area;
    light->isActive = true;
    
    uint32_t lightId = light->id;
    impl_->lightSources_.push_back(std::move(light));
    impl_->stats_.activeLights++;
    
    return lightId;
}

void LightPhysics::generatePhotonMap() {
    impl_->globalPhotonMap_.positions.clear();
    impl_->globalPhotonMap_.directions.clear();
    impl_->globalPhotonMap_.colors.clear();
    impl_->globalPhotonMap_.energies.clear();
    impl_->globalPhotonMap_.bounceCount.clear();
    
    // Trace photons from all light sources
    for (const auto& light : impl_->lightSources_) {
        if (light->isActive) {
            tracePhotons(light->id, impl_->config_.maxPhotons / impl_->lightSources_.size());
        }
    }
    
    impl_->stats_.photonsTraced = impl_->globalPhotonMap_.positions.size();
}

void LightPhysics::tracePhotons(uint32_t lightId, uint32_t photonCount) {
    auto* light = findLight(lightId);
    if (!light) return;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    for (uint32_t i = 0; i < photonCount; ++i) {
        LightRay ray = generatePhotonRay(*light, gen, dis);
        tracePhotonRay(ray);
    }
}

Vector3 LightPhysics::traceRay(const LightRay& ray) const {
    Vector3 color(0, 0, 0);
    LightRay currentRay = ray;
    
    for (uint32_t bounce = 0; bounce < impl_->config_.maxBounces; ++bounce) {
        // Intersect with scene geometry
        IntersectionResult intersection = intersectScene(currentRay);
        
        if (!intersection.hit) {
            // Ray escaped to environment
            break;
        }
        
        // Calculate material interaction
        Vector3 materialContribution = calculateMaterialInteraction(
            currentRay, intersection.materialId, intersection.normal);
        
        color += materialContribution * currentRay.color * currentRay.intensity;
        
        // Generate reflected/refracted ray
        currentRay = generateSecondaryRay(currentRay, intersection);
        
        if (currentRay.intensity < 0.01f) {
            // Ray too weak to continue
            break;
        }
    }
    
    return color;
}

Vector3 LightPhysics::calculateGlobalIllumination(const Vector3& position, const Vector3& normal) const {
    Vector3 directIllumination = calculateDirectIllumination(position, normal);
    Vector3 indirectIllumination = calculateIndirectIllumination(position, normal);
    
    return directIllumination + indirectIllumination;
}

Vector3 LightPhysics::calculateDirectIllumination(const Vector3& position, const Vector3& normal) const {
    Vector3 illumination(0, 0, 0);
    
    for (const auto& light : impl_->lightSources_) {
        if (!light->isActive) continue;
        
        Vector3 lightContribution = calculateLightContribution(*light, position, normal);
        illumination += lightContribution;
    }
    
    return illumination;
}

Vector3 LightPhysics::calculateIndirectIllumination(const Vector3& position, const Vector3& normal) const {
    Vector3 illumination(0, 0, 0);
    
    // Sample photon map for indirect illumination
    const float searchRadius = 1.0f;
    std::vector<size_t> nearbyPhotons = findNearbyPhotons(position, searchRadius);
    
    for (size_t photonIndex : nearbyPhotons) {
        if (photonIndex >= impl_->globalPhotonMap_.positions.size()) continue;
        
        Vector3 photonPos = impl_->globalPhotonMap_.positions[photonIndex];
        Vector3 photonColor = impl_->globalPhotonMap_.colors[photonIndex];
        float photonEnergy = impl_->globalPhotonMap_.energies[photonIndex];
        
        float distance = Vector3::distance(position, photonPos);
        if (distance < searchRadius) {
            float weight = 1.0f - (distance / searchRadius);
            illumination += photonColor * photonEnergy * weight;
        }
    }
    
    return illumination;
}

void LightPhysics::generateCaustics() {
    if (!impl_->config_.enableCaustics) return;
    
    impl_->causticsPhotonMap_.positions.clear();
    impl_->causticsPhotonMap_.directions.clear();
    impl_->causticsPhotonMap_.colors.clear();
    impl_->causticsPhotonMap_.energies.clear();
    impl_->causticsPhotonMap_.bounceCount.clear();
    
    // Trace caustic photons (photons that have been refracted/reflected by specular surfaces)
    for (const auto& light : impl_->lightSources_) {
        if (light->isActive) {
            traceCausticPhotons(*light);
        }
    }
}

Vector3 LightPhysics::getCausticsContribution(const Vector3& position) const {
    if (!impl_->config_.enableCaustics) return Vector3(0, 0, 0);
    
    Vector3 caustics(0, 0, 0);
    const float searchRadius = 0.5f;
    
    // Sample caustics photon map
    for (size_t i = 0; i < impl_->causticsPhotonMap_.positions.size(); ++i) {
        Vector3 photonPos = impl_->causticsPhotonMap_.positions[i];
        float distance = Vector3::distance(position, photonPos);
        
        if (distance < searchRadius) {
            Vector3 photonColor = impl_->causticsPhotonMap_.colors[i];
            float photonEnergy = impl_->causticsPhotonMap_.energies[i];
            float weight = 1.0f - (distance / searchRadius);
            
            caustics += photonColor * photonEnergy * weight;
        }
    }
    
    return caustics;
}

Vector3 LightPhysics::calculateVolumetricScattering(const Vector3& start, const Vector3& end) const {
    if (!impl_->config_.enableVolumetricScattering) return Vector3(0, 0, 0);
    
    Vector3 scattering(0, 0, 0);
    Vector3 direction = (end - start).normalized();
    float distance = Vector3::distance(start, end);
    
    // Sample along the ray
    const int samples = 16;
    for (int i = 0; i < samples; ++i) {
        float t = (i + 0.5f) / samples;
        Vector3 samplePos = start + direction * (distance * t);
        
        // Calculate in-scattering from all light sources
        for (const auto& light : impl_->lightSources_) {
            if (!light->isActive) continue;
            
            Vector3 lightContribution = calculateVolumetricLightContribution(*light, samplePos);
            scattering += lightContribution * impl_->scatteringCoefficient_ * impl_->volumetricDensity_;
        }
    }
    
    // Apply Beer's law for out-scattering
    float extinction = impl_->absorptionCoefficient_ + impl_->scatteringCoefficient_;
    float transmittance = std::exp(-extinction * distance);
    
    return scattering * transmittance / samples;
}

void LightPhysics::registerMaterial(uint32_t materialId, float reflectance, float transmittance, 
                                   float roughness, float ior) {
    LightPhysicsImpl::MaterialProperties material;
    material.reflectance = reflectance;
    material.transmittance = transmittance;
    material.roughness = roughness;
    material.ior = ior;
    material.albedo = Vector3(0.8f, 0.8f, 0.8f); // Default albedo
    
    impl_->materials_[materialId] = material;
}

Vector3 LightPhysics::calculateMaterialInteraction(const LightRay& ray, uint32_t materialId, 
                                                  const Vector3& normal) const {
    auto it = impl_->materials_.find(materialId);
    if (it == impl_->materials_.end()) {
        return Vector3(0, 0, 0);
    }
    
    const auto& material = it->second;
    
    // Calculate Fresnel reflectance
    float cosTheta = std::abs(Vector3::dot(ray.direction, normal));
    float fresnelReflectance = calculateFresnelReflectance(cosTheta, material.ior);
    
    // Combine material properties
    Vector3 diffuseContribution = material.albedo * material.reflectance * (1.0f - fresnelReflectance);
    Vector3 specularContribution = Vector3(1, 1, 1) * fresnelReflectance;
    
    return (diffuseContribution + specularContribution) * ray.color * ray.intensity;
}

LightPhysics::LightPhysicsStats LightPhysics::getStats() const {
    return impl_->stats_;
}

void LightPhysics::resetStats() {
    impl_->stats_ = LightPhysicsStats{};
}

// Private helper methods
void LightPhysics::updateLightSources(float deltaTime) {
    // Update any animated light sources
    for (auto& light : impl_->lightSources_) {
        if (light->isActive) {
            // Update light properties if needed
        }
    }
}

bool LightPhysics::shouldRegeneratePhotonMap() const {
    // Regenerate if lights have changed or periodically
    static auto lastRegenTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRegenTime);
    
    return elapsed.count() > 1; // Regenerate every second
}

LightPhysicsImpl::LightSource* LightPhysics::findLight(uint32_t lightId) {
    auto it = std::find_if(impl_->lightSources_.begin(), impl_->lightSources_.end(),
        [lightId](const auto& light) { return light->id == lightId; });
    
    return (it != impl_->lightSources_.end()) ? it->get() : nullptr;
}

} // namespace FoundryEngine