/**
 * @file Renderer.h
 * @brief Abstract rendering interface and platform-specific implementations
 * @author FoundryEngine Team
 * @date 2024
 * @version 1.0.0
 *
 * This file defines the core rendering architecture for FoundryEngine, providing
 * a platform-agnostic interface for 3D graphics rendering with multiple backend
 * implementations (Direct3D 11, OpenGL, Vulkan).
 *
 * Key Features:
 * - Abstract Renderer interface for cross-platform compatibility
 * - Multiple rendering pipelines (Forward, Deferred, Tiled, Clustered)
 * - Advanced rendering features (HDR, Bloom, SSAO, SSR, Volumetric Lighting)
 * - Platform-specific implementations with optimized performance
 * - Debug rendering utilities for development
 * - Comprehensive render statistics and profiling
 *
 * Architecture:
 * The rendering system uses an abstract base class (Renderer) with platform-specific
 * implementations. This allows the engine core to remain platform-agnostic while
 * providing optimal performance on each target platform.
 *
 * Rendering Pipeline Support:
 * - **Forward**: Simple single-pass rendering, good for transparency
 * - **Deferred**: Multi-pass with G-Buffer, optimal for complex lighting
 * - **Tiled Deferred**: Screen-space tiled lighting for mobile optimization
 * - **Clustered**: 3D volume-based lighting for advanced effects
 *
 * Usage Example:
 * @code
 * // Create platform-specific renderer
 * std::unique_ptr<Renderer> renderer;
 * #ifdef _WIN32
 *     renderer = std::make_unique<D3D11Renderer>();
 * #else
 *     renderer = std::make_unique<OpenGLRenderer>();
 * #endif
 *
 * // Initialize and configure
 * if (renderer->initialize()) {
 *     RenderSettings settings;
 *     settings.pipeline = RenderPipeline::Deferred;
 *     settings.enableHDR = true;
 *     renderer->setRenderSettings(settings);
 *
 *     // Render loop
 *     renderer->beginFrame();
 *     renderer->drawMesh(mesh, material, transform);
 *     renderer->endFrame();
 *     renderer->present();
 * }
 * @endcode
 *
 * Performance Considerations:
 * - Deferred rendering optimal for many lights
 * - Forward rendering better for transparent objects
 * - Anti-aliasing choice affects performance significantly
 * - Shadow mapping resolution impacts quality vs performance
 *
 * Thread Safety:
 * Rendering operations are not thread-safe. All rendering must occur on the main thread.
 * Resource creation may be thread-safe depending on the platform implementation.
 *
 * Dependencies:
 * - Matrix4 for transformation matrices
 * - Vector3 for 3D positions and colors
 * - Platform-specific graphics APIs (D3D11, OpenGL, Vulkan)
 */

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

/**
 * @brief Rendering pipeline types with different performance characteristics
 *
 * Each pipeline type has different strengths for various use cases:
 * - Forward: Best for scenes with few lights or heavy transparency
 * - Deferred: Best for scenes with many dynamic lights
 * - TiledDeferred: Mobile-optimized version of deferred rendering
 * - Clustered: Advanced pipeline for complex lighting scenarios
 */
enum class RenderPipeline {
    Forward,      ///< Single-pass forward rendering, good for transparency
    Deferred,     ///< Multi-pass deferred shading, optimal for many lights
    TiledDeferred,///< Screen-space tiled deferred, mobile-friendly
    Clustered     ///< 3D volume-based clustered shading, advanced lighting
};

/**
 * @brief Anti-aliasing techniques with quality vs performance trade-offs
 *
 * Different AA methods provide varying quality and performance characteristics:
 * - MSAA: Hardware-accelerated, good quality but high memory usage
 * - TAA: Temporal anti-aliasing, good for motion but may cause ghosting
 * - FXAA/SMAA: Post-processing AA, good performance but may soften image
 */
enum class AntiAliasing {
    None,     ///< No anti-aliasing (maximum performance)
    MSAA_2X,  ///< 2x Multi-Sample Anti-Aliasing
    MSAA_4X,  ///< 4x Multi-Sample Anti-Aliasing
    MSAA_8X,  ///< 8x Multi-Sample Anti-Aliasing
    TAA,      ///< Temporal Anti-Aliasing (motion-aware)
    FXAA,     ///< Fast Approximate Anti-Aliasing (post-process)
    SMAA      ///< Subpixel Morphological Anti-Aliasing (high quality)
};

/**
 * @brief Comprehensive render settings controlling visual quality and performance
 *
 * This structure contains all configurable rendering options. Settings can be
 * changed at runtime but may require frame buffer recreation for some options.
 */
struct RenderSettings {
    RenderPipeline pipeline = RenderPipeline::Deferred;  ///< Rendering pipeline type
    AntiAliasing antiAliasing = AntiAliasing::TAA;       ///< Anti-aliasing technique
    bool enableHDR = true;                               ///< High Dynamic Range rendering
    bool enableBloom = true;                             ///< Bloom post-processing effect
    bool enableSSAO = true;                              ///< Screen Space Ambient Occlusion
    bool enableSSR = true;                               ///< Screen Space Reflections
    bool enableVolumetricLighting = true;                ///< Volumetric light scattering
    bool enableShadows = true;                           ///< Dynamic shadow mapping
    int shadowMapSize = 2048;                            ///< Shadow map resolution (power of 2)
    float shadowDistance = 100.0f;                       ///< Maximum shadow rendering distance
    int cascadeCount = 4;                                ///< Number of shadow cascades (1-4)
};

/**
 * @brief Abstract base class for all renderer implementations
 *
 * The Renderer class defines the interface that all platform-specific rendering
 * implementations must follow. It provides a complete abstraction over graphics
 * APIs while exposing modern rendering features and performance monitoring.
 *
 * Key Responsibilities:
 * - Frame lifecycle management (begin/end/present)
 * - Mesh and material rendering with transformations
 * - Camera and viewport control
 * - Lighting and environment setup
 * - Render target management for off-screen rendering
 * - Post-processing effects and tone mapping
 * - Debug visualization tools
 * - Performance statistics and profiling
 *
 * Implementation Notes:
 * - All methods are pure virtual and must be implemented by derived classes
 * - Thread safety depends on the platform implementation
 * - Resource management (meshes, textures, etc.) is handled externally
 * - Coordinate system is right-handed with Y up by convention
 *
 * Performance Considerations:
 * - Minimize state changes between draw calls
 * - Batch similar operations when possible
 * - Use instanced rendering for repeated geometry
 * - Profile using the built-in statistics methods
 */
class Renderer {
public:
    virtual ~Renderer() = default;

    // Lifecycle management

    /**
     * @brief Initialize the renderer and create graphics context
     * @return true if initialization successful, false otherwise
     *
     * This method sets up the graphics API, creates the main window surface,
     * initializes shader pipelines, and prepares the renderer for drawing.
     * Must be called before any other rendering operations.
     *
     * @note Platform-specific setup occurs here (device creation, etc.)
     * @note May take several seconds on some platforms
     * @note Safe to call multiple times (no-op if already initialized)
     * @see shutdown() for cleanup
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown the renderer and release all resources
     *
     * Cleans up graphics context, destroys render targets, releases GPU memory,
     * and shuts down the graphics API. After shutdown, the renderer cannot be
     * used until reinitialized.
     *
     * @note All GPU resources become invalid after this call
     * @note Safe to call multiple times
     * @note Should be called before application exit
     * @see initialize() for setup
     */
    virtual void shutdown() = 0;

    // Frame rendering lifecycle

    /**
     * @brief Begin a new frame for rendering
     *
     * Prepares the renderer for drawing operations. This typically involves:
     * - Clearing frame buffers
     * - Setting up per-frame constants
     * - Resetting render statistics
     * - Preparing command buffers
     *
     * Must be called at the start of each frame, before any draw operations.
     *
     * @note Must be paired with endFrame()
     * @note Frame is not visible until present() is called
     * @see endFrame() to complete the frame
     * @see present() to display the frame
     */
    virtual void beginFrame() = 0;

    /**
     * @brief End the current frame and finalize rendering
     *
     * Completes all rendering operations for the current frame. This includes:
     * - Executing post-processing effects
     * - Resolving multi-sampled buffers
     * - Preparing frame for presentation
     *
     * Must be called after all draw operations but before present().
     *
     * @note Must be called after beginFrame()
     * @note Frame is not visible until present() is called
     * @see beginFrame() to start the frame
     * @see present() to display the frame
     */
    virtual void endFrame() = 0;

    /**
     * @brief Present the rendered frame to the display
     *
     * Makes the completed frame visible by swapping buffers or presenting
     * to the display. This is typically synchronized with the display's
     * refresh rate (vsync).
     *
     * @note Must be called after endFrame()
     * @note May block waiting for vsync
     * @note Frame becomes visible after this call
     * @see beginFrame() and endFrame() for frame rendering
     */
    virtual void present() = 0;

    // Render commands

    /**
     * @brief Draw a mesh with material and transformation
     * @param mesh The mesh geometry to render
     * @param material The material properties for shading
     * @param transform World transformation matrix
     *
     * Renders a single mesh instance using the specified material and
     * transformation. This is the primary rendering method for individual objects.
     *
     * @param mesh Pointer to mesh data (must be valid and loaded)
     * @param material Pointer to material (must be valid)
     * @param transform 4x4 transformation matrix in world space
     * @note Mesh and material must remain valid during rendering
     * @note Transform is applied in vertex shader
     * @see drawInstanced() for rendering multiple instances efficiently
     */
    virtual void drawMesh(Mesh* mesh, Material* material, const Matrix4& transform) = 0;

    /**
     * @brief Draw multiple mesh instances efficiently
     * @param mesh The mesh geometry to render
     * @param material The material properties for shading
     * @param transforms Array of world transformation matrices
     *
     * Renders multiple instances of the same mesh with different transformations.
     * This is much more efficient than calling drawMesh() multiple times for
     * identical geometry (particles, crowds, etc.).
     *
     * @param mesh Pointer to mesh data (must be valid and loaded)
     * @param material Pointer to material (must be valid)
     * @param transforms Vector of 4x4 transformation matrices
     * @note All transforms are in world space
     * @note GPU instancing is used when available
     * @note More efficient than multiple drawMesh() calls
     * @see drawMesh() for single instance rendering
     */
    virtual void drawInstanced(Mesh* mesh, Material* material, const std::vector<Matrix4>& transforms) = 0;

    /**
     * @brief Draw a skybox/environment background
     * @param skybox Cubemap texture for the skybox
     *
     * Renders a skybox or environment map as the scene background.
     * Typically rendered after opaque geometry but before transparent objects.
     *
     * @param skybox Pointer to cubemap texture (must be valid)
     * @note Skybox is rendered at infinite distance
     * @note Uses camera rotation only (no translation)
     * @note Should be called after main geometry but before UI
     */
    virtual void drawSkybox(Texture* skybox) = 0;

    /**
     * @brief Draw user interface elements
     *
     * Renders all UI elements (buttons, text, panels, etc.) on top of the 3D scene.
     * UI rendering uses orthographic projection and screen-space coordinates.
     *
     * @note Called after 3D rendering but before present()
     * @note Uses separate coordinate system (pixels/screen space)
     * @note May change render states significantly
     */
    virtual void drawUI() = 0;

    // Camera and viewport control

    /**
     * @brief Set the active camera for rendering
     * @param camera The camera to use for view/projection matrices
     *
     * Sets which camera's view and projection matrices will be used for
     * subsequent rendering operations. Camera parameters affect culling,
     * LOD, and matrix calculations.
     *
     * @param camera Pointer to camera object (must be valid)
     * @note Camera transform affects view matrix
     * @note Camera parameters affect frustum culling
     * @note Can be changed mid-frame for multi-camera rendering
     */
    virtual void setCamera(Camera* camera) = 0;

    /**
     * @brief Set the rendering viewport rectangle
     * @param x X coordinate of viewport in pixels
     * @param y Y coordinate of viewport in pixels
     * @param width Width of viewport in pixels
     * @param height Height of viewport in pixels
     *
     * Defines the rectangular region of the screen where rendering occurs.
     * Coordinates are relative to the render target (usually the screen).
     *
     * @param x Left edge of viewport
     * @param y Bottom edge of viewport (OpenGL convention)
     * @param width Viewport width in pixels
     * @param height Viewport height in pixels
     * @note Affects both 3D and UI rendering
     * @note Coordinates may be flipped on different platforms
     * @note Full screen is (0, 0, screenWidth, screenHeight)
     */
    virtual void setViewport(int x, int y, int width, int height) = 0;

    // Lighting and environment

    /**
     * @brief Set the active lights for the scene
     * @param lights Array of light objects affecting the scene
     *
     * Provides the renderer with all lights that should affect the current
     * frame. Lights are used for shading calculations in forward rendering
     * or stored in light buffers for deferred rendering.
     *
     * @param lights Vector of light pointers (all must be valid)
     * @note Lights are processed each frame
     * @note Order may affect rendering priority
     * @note Empty vector means no dynamic lighting
     */
    virtual void setLights(const std::vector<Light*>& lights) = 0;

    /**
     * @brief Set the environment map for image-based lighting
     * @param envMap Cubemap texture for environment lighting/reflections
     *
     * Sets the environment cubemap used for image-based lighting (IBL),
     * reflections, and ambient lighting. This provides realistic lighting
     * from the surrounding environment.
     *
     * @param envMap Pointer to cubemap texture (can be nullptr to disable)
     * @note Used for PBR material reflections
     * @note Affects ambient lighting calculations
     * @note Can be expensive to change frequently
     */
    virtual void setEnvironmentMap(Texture* envMap) = 0;

    // Render target management

    /**
     * @brief Create a new render target for off-screen rendering
     * @param width Width of the render target in pixels
     * @param height Height of the render target in pixels
     * @param hdr Whether to use HDR format (16-bit or 32-bit float)
     * @return Pointer to new render target, or nullptr on failure
     *
     * Creates a render target that can be used for off-screen rendering,
     * post-processing, or multi-pass effects. HDR targets support higher
     * dynamic range for advanced lighting effects.
     *
     * @param width Target width (must be power of 2 for some effects)
     * @param height Target height (must be power of 2 for some effects)
     * @param hdr true for HDR format, false for LDR
     * @return New render target or nullptr
     * @note Caller owns the returned pointer
     * @note Must be deleted to avoid memory leaks
     * @note HDR targets use more memory and bandwidth
     * @see setRenderTarget() to use the created target
     */
    virtual RenderTarget* createRenderTarget(int width, int height, bool hdr = false) = 0;

    /**
     * @brief Set the active render target for subsequent operations
     * @param target Render target to make active (nullptr for screen)
     *
     * Changes where subsequent rendering operations will occur. Pass nullptr
     * to render to the screen/backbuffer. Render targets enable multi-pass
     * rendering and post-processing effects.
     *
     * @param target Render target to activate, or nullptr for screen
     * @note Affects all subsequent draw operations
     * @note Screen target is active by default
     * @note Render targets must be created first
     * @see createRenderTarget() to create off-screen targets
     */
    virtual void setRenderTarget(RenderTarget* target) = 0;

    /**
     * @brief Clear the current render target
     * @param color Color to clear to (RGB components)
     * @param depth Depth value to clear to (0.0 = near, 1.0 = far)
     *
     * Clears the color and depth buffers of the current render target.
     * Should be called at the beginning of rendering to prevent artifacts
     * from previous frames.
     *
     * @param color RGB clear color (default black)
     * @param depth Depth clear value (default 1.0 = far plane)
     * @note Affects current render target (screen or off-screen)
     * @note Should be called after setRenderTarget()
     * @note Color values should be in linear space for HDR targets
     */
    virtual void clearRenderTarget(const Vector3& color = Vector3(0,0,0), float depth = 1.0f) = 0;

    // Post-processing effects

    /**
     * @brief Apply post-processing effects to the current frame
     *
     * Executes the configured post-processing pipeline including:
     * - Tone mapping and color grading
     * - Bloom and glow effects
     * - Anti-aliasing
     * - Color correction
     * - Final output formatting
     *
     * @note Called automatically during endFrame()
     * @note Uses settings from setRenderSettings()
     * @note Can be expensive depending on enabled effects
     * @see setRenderSettings() to configure effects
     */
    virtual void applyPostProcessing() = 0;

    /**
     * @brief Set the exposure level for HDR rendering
     * @param exposure Exposure multiplier (1.0 = default)
     *
     * Controls the brightness of the final image in HDR rendering.
     * Higher values make the image brighter, lower values darker.
     *
     * @param exposure Exposure multiplier (typically 0.1 to 10.0)
     * @note Only affects HDR rendering pipelines
     * @note Can be adjusted dynamically for day/night cycles
     * @note 1.0 is the neutral/default exposure
     */
    virtual void setExposure(float exposure) = 0;

    /**
     * @brief Set the gamma correction value
     * @param gamma Gamma value for color correction
     *
     * Applies gamma correction to the final output. Standard gamma is 2.2
     * for most displays, but this can be adjusted for artistic effect.
     *
     * @param gamma Gamma correction value (typically 1.8 to 2.4)
     * @note Applied after tone mapping
     * @note 2.2 is standard for sRGB displays
     * @note Affects the entire final image
     */
    virtual void setGamma(float gamma) = 0;

    // Configuration

    /**
     * @brief Apply new render settings
     * @param settings Complete render configuration
     *
     * Updates all rendering settings at once. Some changes may require
     * frame buffer recreation or shader recompilation.
     *
     * @param settings New render settings to apply
     * @note May be expensive for some setting changes
     * @note Takes effect immediately or next frame
     * @note Thread-safe depending on implementation
     * @see RenderSettings for available options
     */
    virtual void setRenderSettings(const RenderSettings& settings) = 0;

    /**
     * @brief Get the current render settings
     * @return Copy of current render configuration
     *
     * Returns the currently active render settings. Useful for saving
     * user preferences or debugging current configuration.
     *
     * @return Current render settings
     * @note Returns a copy, not a reference
     * @note Thread-safe depending on implementation
     * @see setRenderSettings() to modify settings
     */
    virtual RenderSettings getRenderSettings() const = 0;

    // Debug visualization

    /**
     * @brief Draw a debug line in 3D space
     * @param start Starting point of the line
     * @param end Ending point of the line
     * @param color Color of the line (RGB)
     *
     * Renders a line segment for debugging purposes. Useful for visualizing
     * vectors, rays, paths, collision shapes, etc. Lines are rendered on
     * top of the scene and ignore depth testing.
     *
     * @param start World-space starting point
     * @param end World-space ending point
     * @param color RGB color (0-1 range recommended)
     * @note Rendered in screen space on top of scene
     * @note Ignores depth testing for visibility
     * @note Only visible in debug/development builds
     * @see drawDebugSphere() and drawDebugBox() for other shapes
     */
    virtual void drawDebugLine(const Vector3& start, const Vector3& end, const Vector3& color) = 0;

    /**
     * @brief Draw a debug sphere in 3D space
     * @param center Center point of the sphere
     * @param radius Radius of the sphere
     * @param color Color of the sphere (RGB)
     *
     * Renders a wireframe sphere for debugging. Useful for visualizing
     * collision spheres, trigger volumes, or spatial extents.
     *
     * @param center World-space center point
     * @param radius Sphere radius in world units
     * @param color RGB color (0-1 range recommended)
     * @note Rendered as wireframe on top of scene
     * @note Ignores depth testing for visibility
     * @note Only visible in debug/development builds
     * @see drawDebugLine() and drawDebugBox() for other shapes
     */
    virtual void drawDebugSphere(const Vector3& center, float radius, const Vector3& color) = 0;

    /**
     * @brief Draw a debug box in 3D space
     * @param center Center point of the box
     * @param size Size of the box in each dimension
     * @param color Color of the box (RGB)
     *
     * Renders a wireframe box for debugging. Useful for visualizing
     * bounding boxes, collision volumes, or spatial partitions.
     *
     * @param center World-space center point
     * @param size Box dimensions (width, height, depth)
     * @param color RGB color (0-1 range recommended)
     * @note Rendered as wireframe on top of scene
     * @note Ignores depth testing for visibility
     * @note Only visible in debug/development builds
     * @see drawDebugLine() and drawDebugSphere() for other shapes
     */
    virtual void drawDebugBox(const Vector3& center, const Vector3& size, const Vector3& color) = 0;

    // Performance statistics

    /**
     * @brief Get the number of draw calls in the current frame
     * @return Number of draw calls executed
     *
     * Returns the count of individual draw operations (drawMesh, drawInstanced, etc.)
     * performed in the current frame. Useful for performance analysis and optimization.
     *
     * @return Draw call count for current frame
     * @note Reset at the start of each frame
     * @note Higher counts indicate more GPU work
     * @note Instanced draws count as one call
     * @see resetStats() to clear counters
     */
    virtual int getDrawCalls() const = 0;

    /**
     * @brief Get the number of triangles rendered in the current frame
     * @return Number of triangles processed
     *
     * Returns the total number of triangles sent to the GPU in the current frame.
     * This includes all geometry after culling and before rasterization.
     *
     * @return Triangle count for current frame
     * @note Reset at the start of each frame
     * @note Affected by LOD, culling, and tessellation
     * @note Higher counts indicate more geometry processing
     */
    virtual int getTriangles() const = 0;

    /**
     * @brief Get the number of vertices processed in the current frame
     * @return Number of vertices transformed
     *
     * Returns the total number of vertices processed by the vertex shader
     * in the current frame. This represents the raw geometry complexity.
     *
     * @return Vertex count for current frame
     * @note Reset at the start of each frame
     * @note Includes all mesh vertices after instancing
     * @note Affected by LOD and mesh complexity
     */
    virtual int getVertices() const = 0;

    /**
     * @brief Reset all performance statistics counters
     *
     * Clears all frame statistics counters (draw calls, triangles, vertices).
     * Typically called at the start of each frame to get per-frame metrics.
     *
     * @note Called automatically by beginFrame()
     * @note Can be called manually to reset counters
     * @note Useful for custom profiling sections
     * @see getDrawCalls(), getTriangles(), getVertices() for current values
     */
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
