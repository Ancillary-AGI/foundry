#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include "../math/Matrix4.h"
#include "../math/Vector3.h"

namespace FoundryEngine {

class Mesh;
class Material;
class Texture;
class Shader;
class Camera;
class Light;
class RenderTarget;

enum class RenderPipeline {
    Forward,
    Deferred,
    TiledDeferred,
    Clustered
};

enum class AntiAliasing {
    None,
    MSAA_2X,
    MSAA_4X,
    MSAA_8X,
    TAA,
    FXAA,
    SMAA
};

struct RenderSettings {
    RenderPipeline pipeline = RenderPipeline::Deferred;
    AntiAliasing antiAliasing = AntiAliasing::TAA;
    bool enableHDR = true;
    bool enableBloom = true;
    bool enableSSAO = true;
    bool enableSSR = true;
    bool enableVolumetricLighting = true;
    bool enableShadows = true;
    int shadowMapSize = 2048;
    float shadowDistance = 100.0f;
    int cascadeCount = 4;
};

class Renderer {
public:
    virtual ~Renderer() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    
    // Frame rendering
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void present() = 0;
    
    // Render commands
    virtual void drawMesh(Mesh* mesh, Material* material, const Matrix4& transform) = 0;
    virtual void drawInstanced(Mesh* mesh, Material* material, const std::vector<Matrix4>& transforms) = 0;
    virtual void drawSkybox(Texture* skybox) = 0;
    virtual void drawUI() = 0;
    
    // Camera and viewport
    virtual void setCamera(Camera* camera) = 0;
    virtual void setViewport(int x, int y, int width, int height) = 0;
    
    // Lighting
    virtual void setLights(const std::vector<Light*>& lights) = 0;
    virtual void setEnvironmentMap(Texture* envMap) = 0;
    
    // Render targets
    virtual RenderTarget* createRenderTarget(int width, int height, bool hdr = false) = 0;
    virtual void setRenderTarget(RenderTarget* target) = 0;
    virtual void clearRenderTarget(const Vector3& color = Vector3(0,0,0), float depth = 1.0f) = 0;
    
    // Post-processing
    virtual void applyPostProcessing() = 0;
    virtual void setExposure(float exposure) = 0;
    virtual void setGamma(float gamma) = 0;
    
    // Settings
    virtual void setRenderSettings(const RenderSettings& settings) = 0;
    virtual RenderSettings getRenderSettings() const = 0;
    
    // Debug rendering
    virtual void drawDebugLine(const Vector3& start, const Vector3& end, const Vector3& color) = 0;
    virtual void drawDebugSphere(const Vector3& center, float radius, const Vector3& color) = 0;
    virtual void drawDebugBox(const Vector3& center, const Vector3& size, const Vector3& color) = 0;
    
    // Statistics
    virtual int getDrawCalls() const = 0;
    virtual int getTriangles() const = 0;
    virtual int getVertices() const = 0;
    virtual void resetStats() = 0;
};

// Platform-specific renderer implementations
class D3D11Renderer : public Renderer {
public:
    bool initialize() override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void present() override;
    void drawMesh(Mesh* mesh, Material* material, const Matrix4& transform) override;
    void drawInstanced(Mesh* mesh, Material* material, const std::vector<Matrix4>& transforms) override;
    void drawSkybox(Texture* skybox) override;
    void drawUI() override;
    void setCamera(Camera* camera) override;
    void setViewport(int x, int y, int width, int height) override;
    void setLights(const std::vector<Light*>& lights) override;
    void setEnvironmentMap(Texture* envMap) override;
    RenderTarget* createRenderTarget(int width, int height, bool hdr) override;
    void setRenderTarget(RenderTarget* target) override;
    void clearRenderTarget(const Vector3& color, float depth) override;
    void applyPostProcessing() override;
    void setExposure(float exposure) override;
    void setGamma(float gamma) override;
    void setRenderSettings(const RenderSettings& settings) override;
    RenderSettings getRenderSettings() const override;
    void drawDebugLine(const Vector3& start, const Vector3& end, const Vector3& color) override;
    void drawDebugSphere(const Vector3& center, float radius, const Vector3& color) override;
    void drawDebugBox(const Vector3& center, const Vector3& size, const Vector3& color) override;
    int getDrawCalls() const override;
    int getTriangles() const override;
    int getVertices() const override;
    void resetStats() override;
};

class OpenGLRenderer : public Renderer {
public:
    bool initialize() override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void present() override;
    void drawMesh(Mesh* mesh, Material* material, const Matrix4& transform) override;
    void drawInstanced(Mesh* mesh, Material* material, const std::vector<Matrix4>& transforms) override;
    void drawSkybox(Texture* skybox) override;
    void drawUI() override;
    void setCamera(Camera* camera) override;
    void setViewport(int x, int y, int width, int height) override;
    void setLights(const std::vector<Light*>& lights) override;
    void setEnvironmentMap(Texture* envMap) override;
    RenderTarget* createRenderTarget(int width, int height, bool hdr) override;
    void setRenderTarget(RenderTarget* target) override;
    void clearRenderTarget(const Vector3& color, float depth) override;
    void applyPostProcessing() override;
    void setExposure(float exposure) override;
    void setGamma(float gamma) override;
    void setRenderSettings(const RenderSettings& settings) override;
    RenderSettings getRenderSettings() const override;
    void drawDebugLine(const Vector3& start, const Vector3& end, const Vector3& color) override;
    void drawDebugSphere(const Vector3& center, float radius, const Vector3& color) override;
    void drawDebugBox(const Vector3& center, const Vector3& size, const Vector3& color) override;
    int getDrawCalls() const override;
    int getTriangles() const override;
    int getVertices() const override;
    void resetStats() override;
};

class VulkanRenderer : public Renderer {
public:
    bool initialize() override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void present() override;
    void drawMesh(Mesh* mesh, Material* material, const Matrix4& transform) override;
    void drawInstanced(Mesh* mesh, Material* material, const std::vector<Matrix4>& transforms) override;
    void drawSkybox(Texture* skybox) override;
    void drawUI() override;
    void setCamera(Camera* camera) override;
    void setViewport(int x, int y, int width, int height) override;
    void setLights(const std::vector<Light*>& lights) override;
    void setEnvironmentMap(Texture* envMap) override;
    RenderTarget* createRenderTarget(int width, int height, bool hdr) override;
    void setRenderTarget(RenderTarget* target) override;
    void clearRenderTarget(const Vector3& color, float depth) override;
    void applyPostProcessing() override;
    void setExposure(float exposure) override;
    void setGamma(float gamma) override;
    void setRenderSettings(const RenderSettings& settings) override;
    RenderSettings getRenderSettings() const override;
    void drawDebugLine(const Vector3& start, const Vector3& end, const Vector3& color) override;
    void drawDebugSphere(const Vector3& center, float radius, const Vector3& color) override;
    void drawDebugBox(const Vector3& center, const Vector3& size, const Vector3& color) override;
    int getDrawCalls() const override;
    int getTriangles() const override;
    int getVertices() const override;
    void resetStats() override;
};

} // namespace FoundryEngine
