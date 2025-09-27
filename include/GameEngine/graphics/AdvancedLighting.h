#ifndef NEUTRAL_GAMEENGINE_ADVANCED_LIGHTING_H
#define NEUTRAL_GAMEENGINE_ADVANCED_LIGHTING_H

#include <vector>
#include <unordered_map>
#include "../../math/Vector3.h"
#include "../../math/Matrix4.h"
#include "../System.h"

namespace NeutralGameEngine {

// Light types
enum class LightType {
    DIRECTIONAL,
    POINT,
    SPOT,
    AREA_RECTANGLE,
    AREA_DISK,
    AREA_SPHERE
};

// Base light class
class Light {
public:
    LightType type;
    Vector3 color = Vector3(1.0f, 1.0f, 1.0f);
    float intensity = 1.0f;
    Vector3 position;
    Vector3 direction; // For directional and spot lights
    float range = 100.0f; // For point and spot lights

    // Area light parameters
    Vector3 size; // Rectangle: width/height, Disk: radius, Sphere: radius
    Vector3 normal; // For area lights

    // Shadow mapping
    bool castsShadows = true;
    float shadowBias = 0.005f;
    float shadowNormalBias = 0.1f;
    int shadowMapResolution = 1024;

    virtual Vector3 sampleLight(const Vector3& surfacePoint, Vector3& lightDir, float& pdf) const = 0;
};

// Directional light
class DirectionalLight : public Light {
public:
    DirectionalLight() { type = LightType::DIRECTIONAL; }

    Vector3 sampleLight(const Vector3& surfacePoint, Vector3& lightDir, float& pdf) const override {
        lightDir = -direction.normalized();
        pdf = 1.0f; // Deterministic
        return color * intensity;
    }
};

// Point light
class PointLight : public Light {
public:
    PointLight() { type = LightType::POINT; }

    Vector3 sampleLight(const Vector3& surfacePoint, Vector3& lightDir, float& pdf) const override {
        lightDir = (position - surfacePoint).normalized();
        float distance = (position - surfacePoint).magnitude();
        if (distance > range) return Vector3(0, 0, 0);

        float attenuation = 1.0f / (distance * distance);
        pdf = 1.0f; // Deterministic
        return color * intensity * attenuation;
    }
};

// Spot light
class SpotLight : public Light {
public:
    float innerAngle = 30.0f; // degrees
    float outerAngle = 45.0f; // degrees

    SpotLight() { type = LightType::SPOT; }

    Vector3 sampleLight(const Vector3& surfacePoint, Vector3& lightDir, float& pdf) const override {
        lightDir = (position - surfacePoint).normalized();
        float distance = (position - surfacePoint).magnitude();
        if (distance > range) return Vector3(0, 0, 0);

        float cosTheta = lightDir.dot(-direction.normalized());
        float cosInner = cos(innerAngle * 3.14159f / 180.0f);
        float cosOuter = cos(outerAngle * 3.14159f / 180.0f);

        if (cosTheta < cosOuter) return Vector3(0, 0, 0);

        float attenuation = 1.0f / (distance * distance);
        float spotlight = smoothstep(cosOuter, cosInner, cosTheta);

        pdf = 1.0f; // Deterministic
        return color * intensity * attenuation * spotlight;
    }

private:
    float smoothstep(float edge0, float edge1, float x) const {
        x = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
        return x * x * (3 - 2 * x);
    }
};

// Area light (rectangle)
class AreaLight : public Light {
public:
    AreaLight() { type = LightType::AREA_RECTANGLE; }

    Vector3 sampleLight(const Vector3& surfacePoint, Vector3& lightDir, float& pdf) const override {
        // Uniform sampling of rectangle
        float u = (rand() / (float)RAND_MAX) * 2 - 1; // [-1, 1]
        float v = (rand() / (float)RAND_MAX) * 2 - 1;

        Vector3 lightPoint = position + u * size.x * normal.cross(Vector3(1,0,0)).normalized() +
                            v * size.y * normal.cross(normal.cross(Vector3(1,0,0)).normalized());

        lightDir = (lightPoint - surfacePoint).normalized();
        float distance = (lightPoint - surfacePoint).magnitude();
        float cosTheta = normal.dot(-lightDir);

        if (cosTheta <= 0) return Vector3(0, 0, 0);

        float area = 4 * size.x * size.y; // Rectangle area
        pdf = distance * distance / (area * cosTheta);

        return color * intensity;
    }
};

// Dynamic Light Probes with parallax correction
class LightProbe {
public:
    Vector3 position;
    Vector3 shCoeffs[9]; // Spherical harmonics coefficients

    // Sample indirect lighting
    Vector3 sampleIrradiance(const Vector3& normal) const;

    // Update probe with GI solution
    void updateFromDDGI(const std::vector<Vector3>& probeData);
};

// Volumetric Lighting with scattering
class VolumetricLighting {
public:
    struct VolumeVoxel {
        Vector3 radiance;
        float density = 0.0f;
    };

    std::vector<VolumeVoxel> voxels;

    void simulateScattering(const Vector3& lightDir, float scatteringCoeff, float absorptionCoeff);

    Vector3 sampleVolumetricLight(const Vector3& rayOrigin, const Vector3& rayDir, float maxDistance) const;
};

// Light baking system with optimized lightmaps
class LightBaker {
public:
    struct LightmapUV {
        Vector2 uv;
        int meshIndex;
    };

    struct PackedLightmap {
        int width, height;
        std::vector<Vector3> diffuse;
        std::vector<Vector3> specular;
    };

    // Bake static lighting
    PackedLightmap bakeLightmaps(const std::vector<Vector3>& vertices,
                                const std::vector<std::vector<int>>& indices,
                                const std::vector<Light*>& staticLights,
                                int resolution = 512);

    // Optimized lightmap packing
    std::vector<Vector3> packLightmaps(const std::vector<PackedLightmap>& lightmaps, int atlasSize);

private:
    // Hemispherical sampling for GI
    Vector3 hemisphereSample(float u1, float u2) const;

    // Solid angle calculation
    float solidAngle(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& normal) const;
};

// Real-time Shadow Mapping with cascaded, PSSM, VSM/PCF
class ShadowMapRenderer {
public:
    enum class ShadowTechnique {
        BASIC,
        PCF,
        VSM,
        CSM_PCF,
        PSSM
    };

    ShadowTechnique technique = ShadowTechnique::PCF;

    // Cascaded shadow maps for directional lights
    struct Cascade {
        Matrix4 viewProjection;
        float splitDistance;
    };
    std::vector<Cascade> cascades;

    void renderShadowMaps(const std::vector<Light*>& lights,
                         const std::vector<Vector3>& vertices,
                         const std::vector<std::vector<int>>& indices);

    float sampleShadow(const Vector3& worldPos, const Light& light, float ndotl) const;

    // Percentage Closer Filtering
    float PCF(const Vector2& shadowCoord, float ndotl, int kernelSize = 4) const;

    // Variance Shadow Maps
    float VSM(const Vector2& shadowCoord, float ndotl) const;

    // Cascaded Shadow Maps sampling
    float CSM_PCF(const Vector3& worldPos, const DirectionalLight& light, float ndotl) const;

private:
    // Shadow map textures (simulated as 2D arrays)
    std::unordered_map<const Light*, std::vector<std::vector<float>>> shadowMaps_;
};

// Global Illumination: Dynamic Diffuse Global Illumination (DDGI)
class DDGI {
public:
    struct ProbeVolume {
        Vector3 origin;
        Vector3 extents;
        Vector3 probeSpacing;
        std::vector<LightProbe> probes;
        int probesPerDimension = 8;
    } volume;

    void initializeProbes();
    void updateProbes();

    Vector3 sampleDDGI(const Vector3& position, const Vector3& normal) const;

    // Compute irradiance from probes using tetrahedral interpolation
    Vector3 tetrahedralInterpolate(const Vector3& position, int probeIndices[4]) const;

private:
    // Radiance cache
    std::vector<std::vector<Vector3>> radianceCache;
};

} // namespace NeutralGameEngine

#endif // NEUTRAL_GAMEENGINE_ADVANCED_LIGHTING_H
