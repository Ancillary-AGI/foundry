#ifndef FOUNDRY_GAMEENGINE_MATERIAL_H
#define FOUNDRY_GAMEENGINE_MATERIAL_H

#include <vector>
#include <memory>
#include "../../math/Vector3.h"
#include "../../math/Quaternion.h"

namespace NeutralGameEngine {

// Physically Based Rendering (PBR) Material with Disney BRDF
class Material {
public:
    Material() = default;

    // Disney BRDF parameters
    Vector3 baseColor = Vector3(0.8f, 0.8f, 0.8f);
    float metallic = 0.0f;        // 0 = dielectric, 1 = metallic
    float roughness = 0.5f;       // 0 = smooth, 1 = rough
    float specular = 0.5f;        // Specular intensity
    float specularTint = 0.0f;    // Specular tint
    float clearcoat = 0.0f;       // Clearcoat strength
    float clearcoatGloss = 1.0f;  // Clearcoat glossiness
    float anisotropy = 0.0f;      // Anisotropy for brushed metals
    float sheen = 0.0f;           // Sheen for fabrics
    float sheenTint = 0.5f;       // Sheen tint

    // Subsurface scattering parameters
    Vector3 subsurfaceColor = Vector3(1.0f, 1.0f, 1.0f);
    float subsurface = 0.0f;      // Subsurface scattering strength

    // Emission
    Vector3 emission = Vector3(0.0f, 0.0f, 0.0f);
    float emissionStrength = 0.0f;

    // Transparency
    float transparency = 0.0f;
    float refractiveIndex = 1.5f;
    float opacity = 1.0f;

    // Textures
    std::shared_ptr<class Texture> baseColorTexture;
    std::shared_ptr<class Texture> normalTexture;
    std::shared_ptr<class Texture> metallicRoughnessTexture;
    std::shared_ptr<class Texture> emissionTexture;
    std::shared_ptr<class Texture> occlusionTexture;

    // Disney BRDF evaluation
    Vector3 evaluateBRDF(const Vector3& viewDir, const Vector3& lightDir, const Vector3& normal) const;

    // Subsurface scattering approximation
    Vector3 subsurfaceApprox(const Vector3& viewDir, const Vector3& lightDir, const Vector3& normal) const;

    // Anisotropic GGX distribution (for brushed metals)
    float anisotropicGGX(const Vector3& h, const Vector3& n, const Vector3& t, const Vector3& b, float ax, float ay) const;

    // Clearcoat term
    Vector3 clearcoatBRDF(const Vector3& viewDir, const Vector3& lightDir, const Vector3& normal) const;

    // Sheen term for fabrics
    Vector3 sheenBRDF(const Vector3& viewDir, const Vector3& lightDir, const Vector3& normal) const;

private:
    // Fresnel-Schlick approximation
    Vector3 fresnelSchlick(float cosTheta, const Vector3& F0) const;

    // Smith GGX masking/shadowing
    float smithGGX(float NdotL, float NdotV, float alpha) const;

    // Trowbridge-Reitz GGX normal distribution
    float ggx(float NdotH, float alpha) const;

    // Lambertian diffuse
    Vector3 diffuseLobe(float NdotL, float NdotV) const;
};

// Layered material system for complex surfaces
class LayeredMaterial {
public:
    struct MaterialLayer {
        Material material;
        float blendFactor = 1.0f;
        std::shared_ptr<Texture> maskTexture;
        Vector3 blendMode; // e.g., Vector3(add, multiply, overlay)
    };

    std::vector<MaterialLayer> layers;

    Vector3 evaluateLayeredBRDF(const Vector3& viewDir, const Vector3& lightDir, const Vector3& normal, const Vector3& uv) const;

    // Material layering with PBR blending
    Vector3 blendMaterials(const Vector3& base, const Vector3& layer, float factor, const Vector3& blendMode) const;
};

// Procedural material generation with Substance-like designer
class ProceduralMaterialGenerator {
public:
    struct ProceduralNode {
        std::string type; // "noise", "gradient", "blend", etc.
        std::vector<float> parameters;
        std::vector<std::shared_ptr<ProceduralNode>> inputs;

        Vector3 evaluate(const Vector3& uv, float time = 0.0f) const;
    };

    Material generateMaterial(const std::shared_ptr<ProceduralNode>& rootNode);

private:
    Vector3 noisePerlin(const Vector3& uv, float scale = 1.0f) const;
    Vector3 gradientLinear(const Vector3& uv, const Vector3& start, const Vector3& end) const;
    Vector3 blendMultiply(const Vector3& a, const Vector3& b) const;
    Vector3 blendAdd(const Vector3& a, const Vector3& b) const;
    Vector3 blendOverlay(const Vector3& a, const Vector3& b) const;
};

// Texture class for material maps
class Texture {
public:
    Texture(int width, int height);
    virtual ~Texture() = default;

    Vector3 sample(const Vector3& uv) const;
    void setPixel(int x, int y, const Vector3& color);

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

protected:
    int width_, height_;
    std::vector<Vector3> data_;
};

// HDR Environment Map for IBL
class EnvironmentMap : public Texture {
public:
    EnvironmentMap(int width, int height) : Texture(width, height) {}

    Vector3 sample(const Vector3& direction) const override;

    // Importance sampling for GGX
    void importanceSampleGGX(const Vector3& normal, const Vector3& viewDir, Vector3& lightDir, float& pdf, float roughness = 0.5f) const;

    // Precompute irradiance map for diffuse IBL
    EnvironmentMap* generateIrradianceMap() const;

    // Precompute prefiltered environment map for specular IBL
    EnvironmentMap* generatePrefilteredEnvMap(int mipLevels) const;

private:
    // Spherical harmonics for irradiance
    Vector3 shCoeffs[9];
    void computeSHCoeffs();
};

// Multi-scattering subsurface scattering
class SubsurfaceRenderer {
public:
    void computeMultiScattering(const Material& mat, const Vector3& lightDir, const Vector3& normal);

    Vector3 multiscatterBRDF(float NdotL, float NdotV) const;

private:
    std::vector<Vector3> albedoProfile;
    std::vector<float> meanFreePath;
};
};
};

#endif // FOUNDRY_GAMEENGINE_MATERIAL_H
