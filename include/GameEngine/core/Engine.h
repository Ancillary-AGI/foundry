/**
 * @file Engine.h
 * @brief Main engine class and core functionality for FoundryEngine
 * @author FoundryEngine Team
 * @date 2024
 * @version 1.0.0
 *
 * This file contains the core Engine class that serves as the central hub of the FoundryEngine.
 * The Engine class manages the entire game lifecycle, coordinates all engine systems, and provides
 * the main game loop. It implements a singleton pattern to ensure global access while maintaining
 * thread safety and proper resource management.
 *
 * Key Responsibilities:
 * - System initialization and lifecycle management
 * - Main game loop execution (update/render cycle)
 * - Platform abstraction and cross-platform compatibility
 * - Memory management and resource cleanup
 * - Scene and world management coordination
 * - System registration and execution ordering
 *
 * Architecture:
 * The engine follows an Entity-Component-System (ECS) architecture with modular systems
 * that can be added/removed dynamically. All platform-specific code is abstracted through
 * the PlatformInterface, ensuring the core engine remains platform-agnostic.
 *
 * Usage Example:
 * @code
 * // Initialize engine
 * Engine& engine = Engine::getInstance();
 * if (!engine.initialize()) {
 *     return -1; // Initialization failed
 * }
 *
 * // Run main game loop
 * engine.run();
 *
 * // Cleanup
 * engine.shutdown();
 * @endcode
 *
 * Thread Safety:
 * - Singleton access is thread-safe
 * - Main game loop should run on single thread
 * - Systems may have their own threading requirements
 *
 * Dependencies:
 * - World for ECS management
 * - Renderer for graphics rendering
 * - AudioManager for audio processing
 * - InputManager for user input
 * - PhysicsWorld for physics simulation
 * - ScriptEngine for scripting support
 * - AssetManager for resource management
 * - SceneManager for scene hierarchy
 * - UIManager for user interface
 * - NetworkManager for multiplayer
 * - ProfileManager for performance monitoring
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace FoundryEngine {

class World;
class Renderer;
class AudioManager;
class InputManager;
class PhysicsWorld;
class ScriptEngine;
class AssetManager;
class SceneManager;
class UIManager;
class NetworkManager;
class ProfileManager;
class SplashScreen;

/**
 * @class Engine
 * @brief Core game engine class managing all systems and the main game loop
 *
 * The Engine class is the central component of FoundryEngine, responsible for:
 * - Initializing and managing all engine subsystems
 * - Running the main game loop with proper timing and frame rate control
 * - Coordinating updates and rendering across all systems
 * - Providing access to core engine functionality
 * - Managing scene transitions and world state
 * - Handling pause/resume and quit functionality
 *
 * This class implements the singleton pattern to ensure only one engine instance
 * exists throughout the application's lifetime. The engine coordinates the execution
 * of all registered systems in the correct order to maintain proper game simulation.
 *
 * Key Features:
 * - Singleton pattern for global engine access
 * - Modular system architecture with dynamic management
 * - Platform-agnostic core with abstracted platform layer
 * - Proper resource management and cleanup
 * - Frame-rate independent updates with delta time
 * - Scene management and world coordination
 * - Performance profiling and monitoring
 * - Pause/resume functionality for game state management
 */
class Engine {
public:
    /**
     * @brief Get the singleton engine instance
     * @return Reference to the global engine instance
     *
     * This method provides thread-safe access to the single engine instance.
     * The engine is created lazily on first access and persists for the
     * entire application lifetime.
     *
     * @note Thread-safe, can be called from any thread
     * @note Returns reference, never returns nullptr
     */
    static Engine& getInstance();

    /**
     * @brief Initialize the engine and all registered systems
     * @return true if initialization successful, false otherwise
     *
     * This method performs comprehensive initialization of the engine:
     * 1. Platform-specific setup and capability detection
     * 2. Memory system initialization and pool allocation
     * 3. Graphics system setup and context creation
     * 4. Audio system initialization and device enumeration
     * 5. Input system configuration and device detection
     * 6. Physics world creation and simulation setup
     * 7. ECS world and scene initialization
     * 8. Scripting engine setup and standard library loading
     * 9. Asset management system initialization
     * 10. UI system setup and theme loading
     * 11. Network system initialization and connection setup
     * 12. Profiling system setup and performance monitoring
     * 13. All registered systems initialization in dependency order
     *
     * If any step fails, the method performs cleanup of partially initialized
     * systems and returns false.
     *
     * @note This method must be called before run()
     * @note Can only be called once per engine instance
     * @note Blocking operation that may take several seconds
     * @note Check return value and handle initialization failures
     * @see run() to start the main game loop
     * @see shutdown() for proper cleanup
     */
    bool initialize();

    /**
     * @brief Run the main game loop until quit is requested
     *
     * This method starts the main game loop that continues until quit() is called
     * or a critical error occurs. The loop maintains the target frame rate and
     * handles timing, updates, and rendering in the correct order.
     *
     * Game Loop Process:
     * 1. Calculate delta time since last frame
     * 2. Process platform events (window, input, etc.)
     * 3. Update all systems if not paused
     * 4. Render the current frame
     * 5. Present to display and wait for next frame
     * 6. Repeat until quit condition met
     *
     * The loop automatically handles:
     * - Frame rate limiting to target FPS
     * - Delta time calculation for frame-rate independence
     * - Pause state (updates skipped when paused)
     * - Platform-specific event processing
     * - Error handling and recovery
     *
     * @note Blocking call that runs until quit() is called
     * @note Should be called after successful initialize()
     * @note Handles timing automatically, no manual frame management needed
     * @note Returns when game loop exits (quit or error)
     * @see quit() to exit the game loop programmatically
     * @see pause()/resume() for game state control
     */
    void run();

    /**
     * @brief Shutdown the engine and cleanup all resources
     *
     * This method performs orderly shutdown of all engine systems:
     * 1. Stops the main game loop if running
     * 2. Shuts down all registered systems in reverse order
     * 3. Cleans up ECS world and scenes
     * 4. Releases physics simulation resources
     * 5. Shuts down audio and input systems
     * 6. Destroys graphics context and resources
     * 7. Closes network connections
     * 8. Performs memory cleanup and leak detection
     * 9. Platform-specific cleanup
     *
     * After shutdown, the engine can be reinitialized if needed.
     *
     * @note Safe to call multiple times
     * @note Should be called before application exit
     * @note Blocking operation that ensures clean resource release
     * @note Automatically called if run() exits due to error
     * @see initialize() for engine startup
     */
    void shutdown();

    // Core systems accessors

    /**
     * @brief Get the ECS world containing all entities and components
     * @return Pointer to the engine's world instance, or nullptr if not initialized
     *
     * Provides access to the Entity-Component-System world where all
     * game entities, components, and their relationships are managed.
     * The world persists for the entire engine lifetime.
     *
     * @note Returns nullptr if engine not initialized
     * @note World is created during initialize() and destroyed during shutdown()
     * @note Thread-safe for read operations, write operations should be main thread
     * @see getScenes() for scene management access
     */
    World* getWorld() const { return world_.get(); }

    /**
     * @brief Get the graphics renderer for drawing operations
     * @return Pointer to the renderer instance, or nullptr if not initialized
     *
     * Provides access to the graphics rendering system responsible for:
     * - Drawing 3D models and sprites
     * - Managing shaders and materials
     * - Handling render targets and post-processing
     * - Coordinate system transformations
     *
     * @note Returns nullptr if engine not initialized
     * @note Renderer is created during initialize() and destroyed during shutdown()
     * @note All rendering operations should go through this interface
     */
    Renderer* getRenderer() const { return renderer_.get(); }

    /**
     * @brief Get the audio manager for sound and music playback
     * @return Pointer to the audio manager instance, or nullptr if not initialized
     *
     * Provides access to the audio system responsible for:
     * - Playing sound effects and background music
     * - 3D spatial audio positioning
     * - Audio format decoding and mixing
     * - Volume and effect control
     *
     * @note Returns nullptr if engine not initialized
     * @note Audio manager is created during initialize() and destroyed during shutdown()
     * @note Thread-safe for playback operations
     */
    AudioManager* getAudio() const { return audio_.get(); }

    /**
     * @brief Get the input manager for user input handling
     * @return Pointer to the input manager instance, or nullptr if not initialized
     *
     * Provides access to the input system responsible for:
     * - Keyboard, mouse, and gamepad input
     * - Input device enumeration and configuration
     * - Input mapping and key binding
     * - Gesture recognition on touch devices
     *
     * @note Returns nullptr if engine not initialized
     * @note Input manager is created during initialize() and destroyed during shutdown()
     * @note Should be polled each frame for current input state
     */
    InputManager* getInput() const { return input_.get(); }

    /**
     * @brief Get the physics world for physics simulation
     * @return Pointer to the physics world instance, or nullptr if not initialized
     *
     * Provides access to the physics simulation system responsible for:
     * - Rigid body dynamics and collision detection
     * - Constraint solving and joint simulation
     * - Ray casting and shape queries
     * - Force application and torque
     *
     * @note Returns nullptr if engine not initialized
     * @note Physics world is created during initialize() and destroyed during shutdown()
     * @note Physics simulation runs in fixed time steps for determinism
     */
    PhysicsWorld* getPhysics() const { return physics_.get(); }

    /**
     * @brief Get the scripting engine for runtime script execution
     * @return Pointer to the script engine instance, or nullptr if not initialized
     *
     * Provides access to the scripting system responsible for:
     * - Lua script loading and execution
     * - Script hot-reloading during development
     * - Script binding to engine systems
     * - Error handling and debugging
     *
     * @note Returns nullptr if engine not initialized
     * @note Script engine is created during initialize() and destroyed during shutdown()
     * @note Scripts can access all engine systems through bindings
     */
    ScriptEngine* getScripting() const { return scripting_.get(); }

    /**
     * @brief Get the asset manager for resource loading and management
     * @return Pointer to the asset manager instance, or nullptr if not initialized
     *
     * Provides access to the asset management system responsible for:
     * - Loading textures, models, audio, and other assets
     * - Asset caching and memory management
     * - Asynchronous loading and streaming
     * - Asset format conversion and optimization
     *
     * @note Returns nullptr if engine not initialized
     * @note Asset manager is created during initialize() and destroyed during shutdown()
     * @note Supports hot-reloading of assets during development
     */
    AssetManager* getAssets() const { return assets_.get(); }

    /**
     * @brief Get the scene manager for scene hierarchy management
     * @return Pointer to the scene manager instance, or nullptr if not initialized
     *
     * Provides access to the scene management system responsible for:
     * - Scene loading, saving, and switching
     * - Scene hierarchy and entity organization
     * - Scene serialization and deserialization
     * - Level streaming and management
     *
     * @note Returns nullptr if engine not initialized
     * @note Scene manager is created during initialize() and destroyed during shutdown()
     * @note Scenes contain entity hierarchies and their components
     */
    SceneManager* getScenes() const { return scenes_.get(); }

    /**
     * @brief Get the UI manager for user interface rendering and interaction
     * @return Pointer to the UI manager instance, or nullptr if not initialized
     *
     * Provides access to the user interface system responsible for:
     * - Rendering UI elements (buttons, text, panels)
     * - UI layout and styling
     * - Input handling for UI elements
     * - UI animation and transitions
     *
     * @note Returns nullptr if engine not initialized
     * @note UI manager is created during initialize() and destroyed during shutdown()
     * @note UI rendering happens after 3D rendering
     */
    UIManager* getUI() const { return ui_.get(); }

    /**
     * @brief Get the network manager for multiplayer functionality
     * @return Pointer to the network manager instance, or nullptr if not initialized
     *
     * Provides access to the networking system responsible for:
     * - Client-server communication
     * - State synchronization
     * - Latency compensation and prediction
     * - Connection management and security
     *
     * @note Returns nullptr if engine not initialized
     * @note Network manager is created during initialize() and destroyed during shutdown()
     * @note Supports both client and server modes
     */
    NetworkManager* getNetwork() const { return network_.get(); }

    /**
     * @brief Get the profiler for performance monitoring and optimization
     * @return Pointer to the profiler instance, or nullptr if not initialized
     *
     * Provides access to the performance profiling system responsible for:
     * - CPU and GPU performance monitoring
     * - Memory usage tracking
     * - Frame time analysis
     * - Bottleneck identification
     *
     * @note Returns nullptr if engine not initialized
     * @note Profiler is created during initialize() and destroyed during shutdown()
     * @note Minimal performance impact when profiling is disabled
     */
    ProfileManager* getProfiler() const { return profiler_.get(); }

    /**
     * @brief Get the splash screen system for Foundry branding
     * @return Pointer to the splash screen instance, or nullptr if not initialized
     *
     * Provides access to the splash screen system responsible for:
     * - Displaying Foundry Engine branding at startup
     * - Loading progress indication
     * - Version information display
     * - Custom messages and animations
     *
     * @note Returns nullptr if engine not initialized
     * @note Splash screen is created during initialize() and destroyed during shutdown()
     */
    SplashScreen* getSplashScreen() const { return splashScreen_.get(); }

    // Time management

    /**
     * @brief Get the time elapsed since the last frame
     * @return Delta time in seconds
     *
     * Returns the time difference between the current and previous frame,
     * used for frame-rate independent updates. This value is calculated
     * automatically by the engine's timing system.
     *
     * @note Value is updated each frame during the game loop
     * @note Typically ranges from ~0.016 (60 FPS) to ~0.001 (1000 FPS)
     * @note May be 0 if called before first frame
     * @note Used for smooth animation and physics interpolation
     * @see getTotalTime() for total elapsed time
     */
    float getDeltaTime() const { return deltaTime_; }

    /**
     * @brief Get the total time elapsed since engine initialization
     * @return Total time in seconds
     *
     * Returns the total time the engine has been running since initialize()
     * was called. This is monotonically increasing and never resets.
     *
     * @note Starts at 0 when initialize() completes
     * @note Continues to increase even when paused
     * @note Useful for timestamps and long-term timing
     * @see getDeltaTime() for frame-by-frame timing
     */
    float getTotalTime() const { return totalTime_; }

    /**
     * @brief Get the total number of frames rendered since engine start
     * @return Frame count (monotonically increasing)
     *
     * Returns the total number of frames that have been rendered since
     * the engine started. This can be used for frame-based logic or
     * performance statistics.
     *
     * @note Starts at 0, increments each rendered frame
     * @note Continues to increment even when paused
     * @note Useful for debugging and performance analysis
     */
    uint64_t getFrameCount() const { return frameCount_; }

    // Engine control

    /**
     * @brief Set the target frames per second for the game loop
     * @param fps Target frame rate (typically 30, 60, or 120)
     *
     * Sets the desired frame rate for the game loop. The engine will
     * attempt to maintain this frame rate by adjusting timing between
     * frames. Lower values reduce CPU/GPU usage but may appear choppy.
     *
     * @param fps Target FPS (must be positive, typically 30-120)
     * @note Default is 60 FPS
     * @note Actual FPS may vary based on system performance
     * @note Vsync may override this setting
     * @note Can be changed at runtime
     */
    void setTargetFPS(int fps) { targetFPS_ = fps; }

    /**
     * @brief Pause the game simulation (updates stop, rendering continues)
     *
     * Pauses the game state while keeping rendering active. This is useful
     * for pause menus, cutscenes, or when the application loses focus.
     * Time stops advancing but the engine continues to run.
     *
     * @note Updates are skipped while paused
     * @note Rendering continues (can show pause menu)
     * @note Audio may continue or be muted based on implementation
     * @note Can be called multiple times safely
     * @see resume() to unpause
     * @see isPaused() to check pause state
     */
    void pause() { paused_ = true; }

    /**
     * @brief Resume the game simulation after pausing
     *
     * Resumes normal game operation after a pause. Time continues from
     * where it left off, maintaining smooth gameplay.
     *
     * @note Only effective if currently paused
     * @note Updates resume immediately
     * @note No effect if not paused
     * @see pause() to pause the game
     */
    void resume() { paused_ = false; }

    /**
     * @brief Request the engine to quit and exit the main game loop
     *
     * Signals the engine to exit the main game loop gracefully. The run()
     * method will return after completing the current frame. This allows
     * for proper cleanup and state saving.
     *
     * @note Engine continues running until end of current frame
     * @note run() method will return after this call
     * @note Can be called from any thread
     * @note Safe to call multiple times
     * @see run() for the main game loop
     * @see shutdown() for complete engine cleanup
     */
    void quit() { running_ = false; }

private:
    /**
     * @brief Private constructor for singleton pattern
     *
     * Prevents direct instantiation of Engine. Use getInstance() instead.
     * Initializes all member variables to safe default states.
     */
    Engine() = default;

    /**
     * @brief Private destructor for singleton pattern
     *
     * Ensures proper cleanup when the engine is destroyed.
     * All resources should be cleaned up before destruction.
     */
    ~Engine() = default;

    /**
     * @brief Update all engine systems for one frame
     * @param deltaTime Time elapsed since last update in seconds
     *
     * This method advances the game simulation by one frame, updating all
     * registered systems in the correct dependency order. Called internally
     * by the main game loop.
     *
     * @param deltaTime Time step for this frame, used for frame-rate independence
     * @note Called automatically by run(), should not be called manually
     * @note Systems are updated in dependency order for correct simulation
     * @note Skipped when engine is paused
     * @see render() for the rendering counterpart
     */
    void update(float deltaTime);

    /**
     * @brief Render the current frame to the display
     *
     * This method renders the current game state to the screen through
     * the graphics system. Called internally by the main game loop.
     *
     * @note Called automatically by run(), should not be called manually
     * @note Always called, even when paused (for UI rendering)
     * @note Graphics API calls are abstracted through the renderer
     * @see update() for simulation updates
     */
    void render();

    // Core system managers
    std::unique_ptr<World> world_;              ///< ECS world containing all entities and components
    std::unique_ptr<Renderer> renderer_;        ///< Graphics rendering system
    std::unique_ptr<AudioManager> audio_;       ///< Audio playback and spatial audio
    std::unique_ptr<InputManager> input_;       ///< User input handling and device management
    std::unique_ptr<PhysicsWorld> physics_;     ///< Physics simulation and collision detection
    std::unique_ptr<ScriptEngine> scripting_;   ///< Lua scripting engine with hot-reloading
    std::unique_ptr<AssetManager> assets_;      ///< Resource loading, caching, and management
    std::unique_ptr<SceneManager> scenes_;      ///< Scene hierarchy and level management
    std::unique_ptr<UIManager> ui_;             ///< User interface rendering and interaction
    std::unique_ptr<NetworkManager> network_;   ///< Multiplayer networking and synchronization
    std::unique_ptr<ProfileManager> profiler_;  ///< Performance monitoring and optimization
    std::unique_ptr<SplashScreen> splashScreen_; ///< Foundry branding and splash screen system

    // Engine state
    bool running_ = false;                       ///< Whether the main game loop is running
    bool paused_ = false;                        ///< Whether the game simulation is paused
    int targetFPS_ = 60;                         ///< Target frames per second for the game loop
    float deltaTime_ = 0.0f;                     ///< Time elapsed since last frame in seconds
    float totalTime_ = 0.0f;                     ///< Total time elapsed since engine start
    uint64_t frameCount_ = 0;                    ///< Total number of frames rendered

    // Timing
    std::chrono::high_resolution_clock::time_point lastFrameTime_;  ///< Timestamp of last frame for delta calculation
};

} // namespace FoundryEngine
