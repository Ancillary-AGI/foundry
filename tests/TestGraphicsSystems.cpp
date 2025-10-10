#include "gtest/gtest.h"
#include "GameEngine/graphics/Renderer.h"
#include "GameEngine/graphics/Material.h"
#include "GameEngine/graphics/AdvancedLighting.h"
#include "GameEngine/graphics/RayTracer.h"
#include "GameEngine/graphics/Rasterizer.h"
#include "GameEngine/graphics/SpriteRenderer.h"
#include "GameEngine/graphics/PostProcessing.h"
#include "GameEngine/graphics/NeRFRenderer.h"
#include "GameEngine/graphics/PointCloudRenderer.h"
#include "GameEngine/graphics/MultimediaEditor.h"
#include "GameEngine/core/MemoryPool.h"
#include "GameEngine/math/Vector3.h"
#include "GameEngine/math/Matrix4.h"
#include <thread>
#include <chrono>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Graphics Systems tests
 */
class GraphicsSystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(4096, 32768);
    }

    void TearDown() override {
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
};

/**
 * @brief Test renderer functionality
 */
TEST_F(GraphicsSystemsTest, RendererSystem) {
    // Test renderer creation and initialization
    Renderer renderer;
    EXPECT_TRUE(renderer.initialize());
    EXPECT_TRUE(renderer.isInitialized());

    // Test render target management
    renderer.setRenderTarget(nullptr); // Test with null target
    EXPECT_FALSE(renderer.hasActiveRenderTarget());

    // Test viewport management
    renderer.setViewport(0, 0, 1920, 1080);
    int x, y, width, height;
    renderer.getViewport(x, y, width, height);
    EXPECT_EQ(width, 1920);
    EXPECT_EQ(height, 1080);

    // Test clear color
    renderer.setClearColor(Vector3(0.2f, 0.3f, 0.8f));
    Vector3 clearColor = renderer.getClearColor();
    EXPECT_FLOAT_EQ(clearColor.x, 0.2f);
    EXPECT_FLOAT_EQ(clearColor.y, 0.3f);
    EXPECT_FLOAT_EQ(clearColor.z, 0.8f);

    // Test rendering state
    renderer.enableDepthTest(true);
    EXPECT_TRUE(renderer.isDepthTestEnabled());

    renderer.enableDepthTest(false);
    EXPECT_FALSE(renderer.isDepthTestEnabled());

    // Test cleanup
    renderer.shutdown();
    EXPECT_FALSE(renderer.isInitialized());
}

/**
 * @brief Test material system
 */
TEST_F(GraphicsSystemsTest, MaterialSystem) {
    Material material;

    // Test material properties
    material.setDiffuseColor(Vector3(1.0f, 0.0f, 0.0f));
    material.setSpecularColor(Vector3(0.8f, 0.8f, 0.8f));
    material.setShininess(32.0f);
    material.setOpacity(0.9f);

    EXPECT_EQ(material.getDiffuseColor(), Vector3(1.0f, 0.0f, 0.0f));
    EXPECT_EQ(material.getSpecularColor(), Vector3(0.8f, 0.8f, 0.8f));
    EXPECT_FLOAT_EQ(material.getShininess(), 32.0f);
    EXPECT_FLOAT_EQ(material.getOpacity(), 0.9f);

    // Test texture management
    material.setDiffuseTexture(nullptr);
    material.setNormalTexture(nullptr);
    material.setSpecularTexture(nullptr);

    EXPECT_EQ(material.getDiffuseTexture(), nullptr);
    EXPECT_EQ(material.getNormalTexture(), nullptr);
    EXPECT_EQ(material.getSpecularTexture(), nullptr);

    // Test shader management
    material.setVertexShader(nullptr);
    material.setFragmentShader(nullptr);

    EXPECT_EQ(material.getVertexShader(), nullptr);
    EXPECT_EQ(material.getFragmentShader(), nullptr);

    // Test material validation
    EXPECT_TRUE(material.isValid()); // Should be valid even without textures/shaders
}

/**
 * @brief Test advanced lighting system
 */
TEST_F(GraphicsSystemsTest, AdvancedLighting) {
    AdvancedLighting lighting;

    // Test light creation and management
    LightID light1 = lighting.createLight(LightType::Directional);
    LightID light2 = lighting.createLight(LightType::Point);
    LightID light3 = lighting.createLight(LightType::Spot);

    EXPECT_NE(light1, light2);
    EXPECT_NE(light2, light3);
    EXPECT_GT(light1, 0);
    EXPECT_GT(light2, 0);
    EXPECT_GT(light3, 0);

    // Test light properties
    lighting.setLightColor(light1, Vector3(1.0f, 1.0f, 1.0f));
    lighting.setLightIntensity(light1, 1.0f);
    lighting.setLightPosition(light1, Vector3(0.0f, 10.0f, 0.0f));
    lighting.setLightDirection(light1, Vector3(0.0f, -1.0f, 0.0f));

    Vector3 color = lighting.getLightColor(light1);
    float intensity = lighting.getLightIntensity(light1);
    Vector3 position = lighting.getLightPosition(light1);
    Vector3 direction = lighting.getLightDirection(light1);

    EXPECT_EQ(color, Vector3(1.0f, 1.0f, 1.0f));
    EXPECT_FLOAT_EQ(intensity, 1.0f);
    EXPECT_EQ(position, Vector3(0.0f, 10.0f, 0.0f));
    EXPECT_EQ(direction, Vector3(0.0f, -1.0f, 0.0f));

    // Test shadow mapping
    lighting.enableShadows(light1, true);
    EXPECT_TRUE(lighting.hasShadowsEnabled(light1));

    lighting.enableShadows(light1, false);
    EXPECT_FALSE(lighting.hasShadowsEnabled(light1));

    // Test light culling
    lighting.setLightCulling(light1, true);
    EXPECT_TRUE(lighting.isLightCulled(light1));

    // Test global illumination
    lighting.enableGlobalIllumination(true);
    EXPECT_TRUE(lighting.isGlobalIlluminationEnabled());

    lighting.enableGlobalIllumination(false);
    EXPECT_FALSE(lighting.isGlobalIlluminationEnabled());

    // Test light cleanup
    lighting.destroyLight(light1);
    lighting.destroyLight(light2);
    lighting.destroyLight(light3);

    EXPECT_EQ(lighting.getLightCount(), 0);
}

/**
 * @brief Test ray tracing system
 */
TEST_F(GraphicsSystemsTest, RayTracing) {
    RayTracer rayTracer;

    // Test ray tracer initialization
    EXPECT_TRUE(rayTracer.initialize());
    EXPECT_TRUE(rayTracer.isInitialized());

    // Test scene management
    rayTracer.setMaxBounces(8);
    EXPECT_EQ(rayTracer.getMaxBounces(), 8);

    rayTracer.setSamplesPerPixel(16);
    EXPECT_EQ(rayTracer.getSamplesPerPixel(), 16);

    // Test acceleration structure
    rayTracer.enableBVH(true);
    EXPECT_TRUE(rayTracer.isBVHEnabled());

    rayTracer.enableBVH(false);
    EXPECT_FALSE(rayTracer.isBVHEnabled());

    // Test denoising
    rayTracer.enableDenoising(true);
    EXPECT_TRUE(rayTracer.isDenoisingEnabled());

    rayTracer.setDenoiseStrength(0.8f);
    EXPECT_FLOAT_EQ(rayTracer.getDenoiseStrength(), 0.8f);

    // Test cleanup
    rayTracer.shutdown();
    EXPECT_FALSE(rayTracer.isInitialized());
}

/**
 * @brief Test rasterizer system
 */
TEST_F(GraphicsSystemsTest, Rasterizer) {
    Rasterizer rasterizer;

    // Test rasterizer initialization
    EXPECT_TRUE(rasterizer.initialize());
    EXPECT_TRUE(rasterizer.isInitialized());

    // Test rasterization modes
    rasterizer.setFillMode(FillMode::Solid);
    EXPECT_EQ(rasterizer.getFillMode(), FillMode::Solid);

    rasterizer.setFillMode(FillMode::Wireframe);
    EXPECT_EQ(rasterizer.getFillMode(), FillMode::Wireframe);

    // Test culling modes
    rasterizer.setCullMode(CullMode::Back);
    EXPECT_EQ(rasterizer.getCullMode(), CullMode::Back);

    rasterizer.setCullMode(CullMode::Front);
    EXPECT_EQ(rasterizer.getCullMode(), CullMode::Front);

    rasterizer.setCullMode(CullMode::None);
    EXPECT_EQ(rasterizer.getCullMode(), CullMode::None);

    // Test depth testing
    rasterizer.enableDepthTest(true);
    EXPECT_TRUE(rasterizer.isDepthTestEnabled());

    rasterizer.enableDepthTest(false);
    EXPECT_FALSE(rasterizer.isDepthTestEnabled());

    // Test blending
    rasterizer.enableBlending(true);
    EXPECT_TRUE(rasterizer.isBlendingEnabled());

    rasterizer.setBlendMode(BlendMode::Alpha);
    EXPECT_EQ(rasterizer.getBlendMode(), BlendMode::Alpha);

    // Test cleanup
    rasterizer.shutdown();
    EXPECT_FALSE(rasterizer.isInitialized());
}

/**
 * @brief Test sprite renderer
 */
TEST_F(GraphicsSystemsTest, SpriteRenderer) {
    SpriteRenderer spriteRenderer;

    // Test sprite renderer initialization
    EXPECT_TRUE(spriteRenderer.initialize());
    EXPECT_TRUE(spriteRenderer.isInitialized());

    // Test sprite batching
    spriteRenderer.beginBatch();
    EXPECT_TRUE(spriteRenderer.isBatching());

    spriteRenderer.endBatch();
    EXPECT_FALSE(spriteRenderer.isBatching());

    // Test sprite properties
    spriteRenderer.setSpriteSize(Vector2(64.0f, 64.0f));
    Vector2 spriteSize = spriteRenderer.getSpriteSize();
    EXPECT_EQ(spriteSize, Vector2(64.0f, 64.0f));

    spriteRenderer.setSpriteColor(Vector3(1.0f, 1.0f, 1.0f));
    Vector3 spriteColor = spriteRenderer.getSpriteColor();
    EXPECT_EQ(spriteColor, Vector3(1.0f, 1.0f, 1.0f));

    // Test sprite animation
    spriteRenderer.setAnimationFPS(30.0f);
    EXPECT_FLOAT_EQ(spriteRenderer.getAnimationFPS(), 30.0f);

    spriteRenderer.playAnimation("walk");
    EXPECT_EQ(spriteRenderer.getCurrentAnimation(), "walk");
    EXPECT_TRUE(spriteRenderer.isAnimationPlaying());

    spriteRenderer.stopAnimation();
    EXPECT_FALSE(spriteRenderer.isAnimationPlaying());

    // Test cleanup
    spriteRenderer.shutdown();
    EXPECT_FALSE(spriteRenderer.isInitialized());
}

/**
 * @brief Test post-processing effects
 */
TEST_F(GraphicsSystemsTest, PostProcessing) {
    PostProcessing postProcessing;

    // Test post-processing initialization
    EXPECT_TRUE(postProcessing.initialize());
    EXPECT_TRUE(postProcessing.isInitialized());

    // Test effect management
    postProcessing.enableEffect(PostProcessEffect::Bloom);
    EXPECT_TRUE(postProcessing.isEffectEnabled(PostProcessEffect::Bloom));

    postProcessing.disableEffect(PostProcessEffect::Bloom);
    EXPECT_FALSE(postProcessing.isEffectEnabled(PostProcessEffect::Bloom));

    // Test effect parameters
    postProcessing.setEffectIntensity(PostProcessEffect::Bloom, 0.5f);
    EXPECT_FLOAT_EQ(postProcessing.getEffectIntensity(PostProcessEffect::Bloom), 0.5f);

    postProcessing.setEffectIntensity(PostProcessEffect::Blur, 0.3f);
    EXPECT_FLOAT_EQ(postProcessing.getEffectIntensity(PostProcessEffect::Blur), 0.3f);

    // Test multiple effects
    postProcessing.enableEffect(PostProcessEffect::HDR);
    postProcessing.enableEffect(PostProcessEffect::ColorGrading);
    postProcessing.enableEffect(PostProcessEffect::Vignette);

    EXPECT_TRUE(postProcessing.isEffectEnabled(PostProcessEffect::HDR));
    EXPECT_TRUE(postProcessing.isEffectEnabled(PostProcessEffect::ColorGrading));
    EXPECT_TRUE(postProcessing.isEffectEnabled(PostProcessEffect::Vignette));

    // Test effect counts
    EXPECT_EQ(postProcessing.getEnabledEffectCount(), 3);

    // Test cleanup
    postProcessing.shutdown();
    EXPECT_FALSE(postProcessing.isInitialized());
}

/**
 * @brief Test NeRF renderer
 */
TEST_F(GraphicsSystemsTest, NeRFRenderer) {
    NeRFRenderer nerfRenderer;

    // Test NeRF renderer initialization
    EXPECT_TRUE(nerfRenderer.initialize());
    EXPECT_TRUE(nerfRenderer.isInitialized());

    // Test NeRF scene management
    nerfRenderer.setSceneBounds(Vector3(-10.0f, -10.0f, -10.0f), Vector3(10.0f, 10.0f, 10.0f));
    Vector3 minBounds, maxBounds;
    nerfRenderer.getSceneBounds(minBounds, maxBounds);
    EXPECT_EQ(minBounds, Vector3(-10.0f, -10.0f, -10.0f));
    EXPECT_EQ(maxBounds, Vector3(10.0f, 10.0f, 10.0f));

    // Test NeRF training
    nerfRenderer.setTrainingSamples(1000);
    EXPECT_EQ(nerfRenderer.getTrainingSamples(), 1000);

    nerfRenderer.setNetworkLayers(8);
    EXPECT_EQ(nerfRenderer.getNetworkLayers(), 8);

    // Test NeRF rendering
    nerfRenderer.enableDensityOptimization(true);
    EXPECT_TRUE(nerfRenderer.isDensityOptimizationEnabled());

    nerfRenderer.setRenderingQuality(RenderingQuality::High);
    EXPECT_EQ(nerfRenderer.getRenderingQuality(), RenderingQuality::High);

    // Test cleanup
    nerfRenderer.shutdown();
    EXPECT_FALSE(nerfRenderer.isInitialized());
}

/**
 * @brief Test point cloud renderer
 */
TEST_F(GraphicsSystemsTest, PointCloudRenderer) {
    PointCloudRenderer pointCloudRenderer;

    // Test point cloud renderer initialization
    EXPECT_TRUE(pointCloudRenderer.initialize());
    EXPECT_TRUE(pointCloudRenderer.isInitialized());

    // Test point cloud data management
    std::vector<Vector3> points = {
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(1.0f, 0.0f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f),
        Vector3(0.0f, 0.0f, 1.0f)
    };

    pointCloudRenderer.setPointCloud(points);
    EXPECT_EQ(pointCloudRenderer.getPointCount(), 4);

    // Test point cloud properties
    pointCloudRenderer.setPointSize(2.0f);
    EXPECT_FLOAT_EQ(pointCloudRenderer.getPointSize(), 2.0f);

    pointCloudRenderer.setPointColor(Vector3(0.5f, 0.8f, 1.0f));
    Vector3 pointColor = pointCloudRenderer.getPointColor();
    EXPECT_EQ(pointColor, Vector3(0.5f, 0.8f, 1.0f));

    // Test point cloud rendering modes
    pointCloudRenderer.setRenderingMode(PointCloudMode::Points);
    EXPECT_EQ(pointCloudRenderer.getRenderingMode(), PointCloudMode::Points);

    pointCloudRenderer.setRenderingMode(PointCloudMode::Spheres);
    EXPECT_EQ(pointCloudRenderer.getRenderingMode(), PointCloudMode::Spheres);

    // Test LOD management
    pointCloudRenderer.enableLOD(true);
    EXPECT_TRUE(pointCloudRenderer.isLODEnabled());

    pointCloudRenderer.setLODLevels(5);
    EXPECT_EQ(pointCloudRenderer.getLODLevels(), 5);

    // Test cleanup
    pointCloudRenderer.shutdown();
    EXPECT_FALSE(pointCloudRenderer.isInitialized());
}

/**
 * @brief Test multimedia editor
 */
TEST_F(GraphicsSystemsTest, MultimediaEditor) {
    MultimediaEditor editor;

    // Test editor initialization
    EXPECT_TRUE(editor.initialize());
    EXPECT_TRUE(editor.isInitialized());

    // Test timeline management
    editor.setTimelineDuration(60.0f); // 60 seconds
    EXPECT_FLOAT_EQ(editor.getTimelineDuration(), 60.0f);

    editor.setCurrentTime(30.0f); // Jump to 30 seconds
    EXPECT_FLOAT_EQ(editor.getCurrentTime(), 30.0f);

    // Test track management
    TrackID videoTrack = editor.createTrack(TrackType::Video);
    TrackID audioTrack = editor.createTrack(TrackType::Audio);

    EXPECT_NE(videoTrack, audioTrack);
    EXPECT_GT(videoTrack, 0);
    EXPECT_GT(audioTrack, 0);

    // Test clip management
    ClipID clip1 = editor.createClip(videoTrack, 0.0f, 10.0f);
    ClipID clip2 = editor.createClip(audioTrack, 5.0f, 15.0f);

    EXPECT_NE(clip1, clip2);
    EXPECT_GT(clip1, 0);
    EXPECT_GT(clip2, 0);

    // Test playback controls
    editor.play();
    EXPECT_TRUE(editor.isPlaying());

    editor.pause();
    EXPECT_FALSE(editor.isPlaying());

    editor.stop();
    EXPECT_FALSE(editor.isPlaying());

    // Test rendering
    editor.setRenderResolution(1920, 1080);
    int width, height;
    editor.getRenderResolution(width, height);
    EXPECT_EQ(width, 1920);
    EXPECT_EQ(height, 1080);

    // Test cleanup
    editor.shutdown();
    EXPECT_FALSE(editor.isInitialized());
}

/**
 * @brief Test graphics performance
 */
TEST_F(GraphicsSystemsTest, Performance) {
    const int numIterations = 100;

    // Measure rendering performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        // Simulate graphics operations
        Renderer renderer;
        renderer.initialize();

        Material material;
        material.setDiffuseColor(Vector3(1.0f, 0.0f, 0.0f));

        AdvancedLighting lighting;
        lighting.createLight(LightType::Directional);

        renderer.shutdown();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numIterations << " graphics operations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 100ms for 100 operations)
    EXPECT_LT(duration.count(), 100000);
}

/**
 * @brief Test graphics memory management
 */
TEST_F(GraphicsSystemsTest, MemoryManagement) {
    size_t initialMemory = memoryPool->totalAllocated();

    // Create multiple graphics objects to test memory usage
    std::vector<std::unique_ptr<Material>> materials;
    std::vector<LightID> lights;

    AdvancedLighting lighting;

    for (int i = 0; i < 50; ++i) {
        auto material = std::make_unique<Material>();
        material->setDiffuseColor(Vector3(static_cast<float>(i) / 50.0f, 0.5f, 0.8f));
        materials.push_back(std::move(material));

        LightID light = lighting.createLight(LightType::Point);
        lights.push_back(light);
    }

    size_t afterAllocationMemory = memoryPool->totalAllocated();
    EXPECT_GT(afterAllocationMemory, initialMemory);

    // Test memory utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    // Clean up
    for (LightID light : lights) {
        lighting.destroyLight(light);
    }
    materials.clear();
}

/**
 * @brief Test graphics error handling
 */
TEST_F(GraphicsSystemsTest, ErrorHandling) {
    Renderer renderer;

    // Test invalid operations
    EXPECT_NO_THROW(renderer.setViewport(-1, -1, 0, 0)); // Invalid viewport should handle gracefully
    EXPECT_NO_THROW(renderer.setClearColor(Vector3(-1.0f, 2.0f, -0.5f))); // Invalid color should clamp

    // Test uninitialized operations
    EXPECT_FALSE(renderer.isInitialized());
    EXPECT_NO_THROW(renderer.shutdown()); // Should handle multiple shutdowns

    // Test material error handling
    Material material;
    EXPECT_NO_THROW(material.setShininess(-1.0f)); // Invalid shininess should handle gracefully
    EXPECT_NO_THROW(material.setOpacity(2.0f)); // Invalid opacity should clamp
}

/**
 * @brief Test graphics concurrent operations
 */
TEST_F(GraphicsSystemsTest, ConcurrentOperations) {
    const int numThreads = 4;
    const int operationsPerThread = 25;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads performing graphics operations
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&successCount, t]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                // Perform graphics operations
                Material material;
                material.setDiffuseColor(Vector3(
                    static_cast<float>(t) / numThreads,
                    static_cast<float>(i) / operationsPerThread,
                    0.5f));

                if (material.isValid()) {
                    successCount++;
                }
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify concurrent operations worked
    EXPECT_EQ(successCount.load(), numThreads * operationsPerThread);

    // Memory pool should still be in valid state
    float utilization = memoryPool->utilization();
    EXPECT_GE(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);
}

} // namespace Tests
} // namespace FoundryEngine
