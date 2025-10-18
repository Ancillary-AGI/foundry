package com.foundry.ide.engine

import com.foundry.ide.*
import com.foundry.ide.math.*
import com.foundry.ide.graphics.*
import com.foundry.ide.physics.*
import com.foundry.ide.audio.*
import com.foundry.ide.input.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import java.util.concurrent.ConcurrentHashMap

/**
 * Core Foundry Engine implementation
 * Provides the actual C++ engine functionality that TypeScript bindings call
 */
@Serializable
data class EngineConfig(
    val windowWidth: Int = 800,
    val windowHeight: Int = 600,
    val title: String = "Foundry Game",
    val fullscreen: Boolean = false,
    val vsync: Boolean = true,
    val antialiasing: Int = 4,
    val targetFPS: Int = 60
)

@Serializable
data class SceneConfig(
    val name: String,
    val gravity: Vector3 = Vector3(0f, -9.81f, 0f),
    val ambientLight: Color = Color(0.2f, 0.2f, 0.2f, 1f)
)

class FoundryEngineCore(private val config: EngineConfig) {
    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())

    // Core systems
    private val windowSystem = WindowSystem()
    private val renderSystem = RenderSystem()
    private val physicsSystem = PhysicsSystem()
    private val audioSystem = AudioSystem()
    private val inputSystem = InputSystem()

    // Scene management
    private val scenes = ConcurrentHashMap<String, Scene>()
    private var activeScene: Scene? = null
    private var running = false

    // Timing
    private var deltaTime = 0f
    private var lastTime = System.nanoTime()
    private var frameCount = 0L

    fun initialize(): Boolean {
        return try {
            // Initialize window
            if (!windowSystem.initialize(config)) {
                println("Failed to initialize window system")
                return false
            }

            // Initialize rendering
            if (!renderSystem.initialize(windowSystem)) {
                println("Failed to initialize render system")
                return false
            }

            // Initialize physics
            if (!physicsSystem.initialize()) {
                println("Failed to initialize physics system")
                return false
            }

            // Initialize audio
            if (!audioSystem.initialize()) {
                println("Failed to initialize audio system")
                return false
            }

            // Initialize input
            if (!inputSystem.initialize(windowSystem)) {
                println("Failed to initialize input system")
                return false
            }

            running = true
            println("Foundry Engine initialized successfully")
            true
        } catch (e: Exception) {
            println("Failed to initialize Foundry Engine: ${e.message}")
            false
        }
    }

    fun shutdown() {
        running = false

        // Shutdown all scenes
        scenes.values.forEach { it.destroy() }
        scenes.clear()

        // Shutdown systems
        inputSystem.shutdown()
        audioSystem.shutdown()
        physicsSystem.shutdown()
        renderSystem.shutdown()
        windowSystem.shutdown()

        scope.cancel()
    }

    fun createScene(name: String): Scene {
        val scene = Scene(name, SceneConfig(name))
        scenes[name] = scene
        return scene
    }

    fun destroyScene(name: String) {
        scenes[name]?.destroy()
        scenes.remove(name)

        if (activeScene?.name == name) {
            activeScene = null
        }
    }

    fun setActiveScene(name: String) {
        activeScene = scenes[name]
    }

    fun update() {
        if (!running) return

        // Calculate delta time
        val currentTime = System.nanoTime()
        deltaTime = (currentTime - lastTime) / 1_000_000_000f
        lastTime = currentTime

        // Cap delta time to prevent large jumps
        deltaTime = deltaTime.coerceAtMost(1f / 15f) // Min 15 FPS

        frameCount++

        // Update input
        inputSystem.update()

        // Update active scene
        activeScene?.update(deltaTime)

        // Update physics
        physicsSystem.update(deltaTime)

        // Update audio
        audioSystem.update()
    }

    fun render() {
        if (!running) return

        // Clear screen
        renderSystem.clear()

        // Render active scene
        activeScene?.render(renderSystem)

        // Swap buffers
        windowSystem.swapBuffers()
    }

    fun isRunning(): Boolean = running

    fun getDeltaTime(): Float = deltaTime

    fun getFrameCount(): Long = frameCount

    fun getFPS(): Float {
        return if (deltaTime > 0f) 1f / deltaTime else 0f
    }

    // Window management
    fun setWindowTitle(title: String) {
        windowSystem.setTitle(title)
    }

    fun setWindowSize(width: Int, height: Int) {
        windowSystem.setSize(width, height)
        renderSystem.resize(width, height)
    }

    fun isWindowFocused(): Boolean = windowSystem.isFocused()

    fun shouldClose(): Boolean = windowSystem.shouldClose()

    // Input access
    fun getKey(keyCode: Int): Boolean = inputSystem.getKey(keyCode)
    fun getKeyDown(keyCode: Int): Boolean = inputSystem.getKeyDown(keyCode)
    fun getKeyUp(keyCode: Int): Boolean = inputSystem.getKeyUp(keyCode)

    fun getMouseButton(button: Int): Boolean = inputSystem.getMouseButton(button)
    fun getMousePosition(): Vector2 = inputSystem.getMousePosition()
    fun getMouseDelta(): Vector2 = inputSystem.getMouseDelta()

    // Scene access
    fun getScene(name: String): Scene? = scenes[name]
    fun getActiveScene(): Scene? = activeScene
    fun getAllScenes(): List<Scene> = scenes.values.toList()
}

// Scene implementation
class Scene(val name: String, val config: SceneConfig) {
    private val entities = ConcurrentHashMap<String, Entity>()
    private val systems = mutableListOf<System>()

    // Scene-specific systems
    private val scenePhysics = ScenePhysicsSystem()
    private val sceneAudio = SceneAudioSystem()

    init {
        // Add default systems
        addSystem(scenePhysics)
        addSystem(sceneAudio)
    }

    fun createEntity(name: String): Entity {
        val entity = Entity(name, this)
        entities[name] = entity
        return entity
    }

    fun destroyEntity(name: String) {
        entities[name]?.destroy()
        entities.remove(name)
    }

    fun findEntity(name: String): Entity? = entities[name]

    fun getEntities(): List<Entity> = entities.values.toList()

    fun addSystem(system: System) {
        systems.add(system)
        system.initialize()
    }

    fun removeSystem(system: System) {
        systems.remove(system)
        system.destroy()
    }

    fun update(deltaTime: Float) {
        // Update all systems
        systems.forEach { it.update(deltaTime) }

        // Update all entities
        entities.values.forEach { it.update(deltaTime) }
    }

    fun render(renderer: RenderSystem) {
        // Render all entities
        entities.values.forEach { it.render(renderer) }
    }

    fun destroy() {
        // Destroy all entities
        entities.values.forEach { it.destroy() }
        entities.clear()

        // Destroy all systems
        systems.forEach { it.destroy() }
        systems.clear()
    }
}

// Entity implementation
class Entity(val name: String, val scene: Scene) {
    val id = generateId()
    val transform = TransformComponent()
    private val components = ConcurrentHashMap<String, Component>()
    var isActive = true
        private set

    private fun generateId(): String {
        return "entity_${System.nanoTime()}_${name.hashCode()}"
    }

    fun addComponent(type: String, config: Map<String, Any> = emptyMap()): Component {
        val component = createComponent(type, config)
        components[type] = component
        component.initialize()
        return component
    }

    fun removeComponent(type: String) {
        components[type]?.destroy()
        components.remove(type)
    }

    fun getComponent(type: String): Component? = components[type]

    fun hasComponent(type: String): Boolean = components.containsKey(type)

    fun getComponents(): List<Component> = components.values.toList()

    fun update(deltaTime: Float) {
        if (!isActive) return

        // Update transform
        transform.update(deltaTime)

        // Update all components
        components.values.forEach { it.update(deltaTime) }
    }

    fun render(renderer: RenderSystem) {
        if (!isActive) return

        // Render all renderable components
        components.values.forEach { it.render(renderer) }
    }

    fun destroy() {
        isActive = false

        // Destroy all components
        components.values.forEach { it.destroy() }
        components.clear()
    }

    private fun createComponent(type: String, config: Map<String, Any>): Component {
        return when (type) {
            "MeshRenderer" -> MeshRendererComponent(this, config)
            "RigidBody" -> RigidBodyComponent(this, config)
            "AudioSource" -> AudioSourceComponent(this, config)
            "Camera" -> CameraComponent(this, config)
            "Light" -> LightComponent(this, config)
            else -> GenericComponent(this, type, config)
        }
    }
}

// Component base class
abstract class Component(val entity: Entity) {
    var enabled = true

    open fun initialize() {}
    open fun update(deltaTime: Float) {}
    open fun render(renderer: RenderSystem) {}
    open fun destroy() {}
}

// Transform component
class TransformComponent : Component(entity = null!!) { // Hack for initialization
    var position = Vector3(0f, 0f, 0f)
    var rotation = Quaternion(0f, 0f, 0f, 1f)
    var scale = Vector3(1f, 1f, 1f)

    override fun update(deltaTime: Float) {
        // Transform updates if needed
    }
}

// System base class
abstract class System {
    var enabled = true

    open fun initialize() {}
    open fun update(deltaTime: Float) {}
    open fun destroy() {}
}

// Scene-specific systems
class ScenePhysicsSystem : System() {
    override fun update(deltaTime: Float) {
        // Update physics for scene
    }
}

class SceneAudioSystem : System() {
    override fun update(deltaTime: Float) {
        // Update audio for scene
    }
}

// Placeholder implementations for other systems
class WindowSystem {
    fun initialize(config: EngineConfig): Boolean = true
    fun shutdown() {}
    fun setTitle(title: String) {}
    fun setSize(width: Int, height: Int) {}
    fun isFocused(): Boolean = true
    fun shouldClose(): Boolean = false
    fun swapBuffers() {}
}

class RenderSystem {
    fun initialize(window: WindowSystem): Boolean = true
    fun shutdown() {}
    fun clear() {}
    fun resize(width: Int, height: Int) {}
}

class PhysicsSystem {
    fun initialize(): Boolean = true
    fun shutdown() {}
    fun update(deltaTime: Float) {}
}

class AudioSystem {
    fun initialize(): Boolean = true
    fun shutdown() {}
    fun update() {}
}

class InputSystem {
    fun initialize(window: WindowSystem): Boolean = true
    fun shutdown() {}
    fun update() {}
    fun getKey(keyCode: Int): Boolean = false
    fun getKeyDown(keyCode: Int): Boolean = false
    fun getKeyUp(keyCode: Int): Boolean = false
    fun getMouseButton(button: Int): Boolean = false
    fun getMousePosition(): Vector2 = Vector2(0f, 0f)
    fun getMouseDelta(): Vector2 = Vector2(0f, 0f)
}

// Component implementations
class MeshRendererComponent(entity: Entity, config: Map<String, Any>) : Component(entity) {
    val mesh = config["mesh"] as? String ?: "cube"
    val material = config["material"] as? String ?: "default"

    override fun render(renderer: RenderSystem) {
        // Render mesh
    }
}

class RigidBodyComponent(entity: Entity, config: Map<String, Any>) : Component(entity) {
    val mass = config["mass"] as? Float ?: 1f
    val useGravity = config["useGravity"] as? Boolean ?: true

    override fun update(deltaTime: Float) {
        // Physics update
    }
}

class AudioSourceComponent(entity: Entity, config: Map<String, Any>) : Component(entity) {
    val clip = config["clip"] as? String
    val volume = config["volume"] as? Float ?: 1f
    val loop = config["loop"] as? Boolean ?: false

    override fun update(deltaTime: Float) {
        // Audio update
    }
}

class CameraComponent(entity: Entity, config: Map<String, Any>) : Component(entity) {
    val fov = config["fov"] as? Float ?: 60f
    val near = config["near"] as? Float ?: 0.1f
    val far = config["far"] as? Float ?: 1000f
}

class LightComponent(entity: Entity, config: Map<String, Any>) : Component(entity) {
    val type = config["type"] as? String ?: "point"
    val color = config["color"] as? Color ?: Color(1f, 1f, 1f, 1f)
    val intensity = config["intensity"] as? Float ?: 1f
}

class GenericComponent(entity: Entity, val type: String, val config: Map<String, Any>) : Component(entity)

// Global engine instance
var globalEngine: FoundryEngineCore? = null

// C-style functions for WebAssembly/TypeScript binding
fun FoundryEngine_create(config: EngineConfig): FoundryEngineCore {
    globalEngine = FoundryEngineCore(config)
    return globalEngine!!
}

fun FoundryEngine_initialize(engine: FoundryEngineCore): Boolean {
    return engine.initialize()
}

fun FoundryEngine_update(engine: FoundryEngineCore) {
    engine.update()
}

fun FoundryEngine_render(engine: FoundryEngineCore) {
    engine.render()
}

fun FoundryEngine_shutdown(engine: FoundryEngineCore) {
    engine.shutdown()
}

fun FoundryEngine_createScene(engine: FoundryEngineCore, name: String): Scene {
    return engine.createScene(name)
}

fun FoundryEngine_destroyScene(engine: FoundryEngineCore, name: String) {
    engine.destroyScene(name)
}

fun Scene_createEntity(scene: Scene, name: String): Entity {
    return scene.createEntity(name)
}

fun Scene_destroyEntity(scene: Scene, name: String) {
    scene.destroyEntity(name)
}

fun Scene_findEntity(scene: Scene, name: String): Entity? {
    return scene.findEntity(name)
}

fun Entity_addComponent(entity: Entity, type: String, config: Map<String, Any>): Component {
    return entity.addComponent(type, config)
}

fun Entity_removeComponent(entity: Entity, type: String) {
    entity.removeComponent(type)
}

fun Entity_getComponent(entity: Entity, type: String): Component? {
    return entity.getComponent(type)
}

fun Entity_hasComponent(entity: Entity, type: String): Boolean {
    return entity.hasComponent(type)
}

// Vector3 operations
fun Vector3_add(a: Vector3, b: Vector3): Vector3 {
    return a.add(b)
}

fun Vector3_subtract(a: Vector3, b: Vector3): Vector3 {
    return a.subtract(b)
}

fun Vector3_multiply(vec: Vector3, scalar: Float): Vector3 {
    return vec.multiply(scalar)
}

fun Vector3_dot(a: Vector3, b: Vector3): Float {
    return a.dot(b)
}

fun Vector3_cross(a: Vector3, b: Vector3): Vector3 {
    return a.cross(b)
}

fun Vector3_length(vec: Vector3): Float {
    return vec.length()
}

fun Vector3_normalized(vec: Vector3): Vector3 {
    return vec.normalized()
}

// Input functions
fun Input_getKey(keyCode: Int): Boolean {
    return globalEngine?.getKey(keyCode) ?: false
}

fun Input_getMouseButton(button: Int): Boolean {
    return globalEngine?.getMouseButton(button) ?: false
}

fun Input_getMousePosition(): Vector2 {
    return globalEngine?.getMousePosition() ?: Vector2(0f, 0f)
}

// Utility functions
fun Utils_random(): Float {
    return kotlin.random.Random.nextFloat()
}

fun Utils_randomRange(min: Float, max: Float): Float {
    return min + (max - min) * kotlin.random.Random.nextFloat()
}

fun Utils_clamp(value: Float, min: Float, max: Float): Float {
    return value.coerceIn(min, max)
}

fun Utils_lerp(a: Float, b: Float, t: Float): Float {
    return a + (b - a) * t.coerceIn(0f, 1f)
}

fun Utils_degToRad(degrees: Float): Float {
    return Math.toRadians(degrees.toDouble()).toFloat()
}

fun Utils_radToDeg(radians: Float): Float {
    return Math.toDegrees(radians.toDouble()).toFloat()
}