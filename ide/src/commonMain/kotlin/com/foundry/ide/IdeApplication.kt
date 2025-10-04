package com.foundry.ide

import kotlinx.coroutines.*
import kotlinx.serialization.Serializable

/**
 * Main IDE Application class
 * Coordinates all IDE components and manages the overall state
 */
@Serializable
data class IdeState(
    val currentProject: ProjectInfo? = null,
    val openedFiles: List<String> = emptyList(),
    val activeView: IdeView = IdeView.WELCOME,
    val buildStatus: BuildStatus = BuildStatus.IDLE,
    val recentProjects: List<String> = emptyList(),
    val extensions: List<ExtensionInfo> = emptyList()
)

enum class IdeView {
    WELCOME,
    PROJECT_BROWSER,
    WORLD_EDITOR,
    CODE_EDITOR,
    ASSET_BROWSER,
    COMPONENT_INSPECTOR,
    SYSTEM_VIEWER,
    BUILD_SETTINGS,
    DEBUG_VIEW,
    PROFILING_VIEW
}

enum class BuildStatus {
    IDLE,
    BUILDING,
    SUCCESS,
    FAILED,
    CANCELLING
}

@Serializable
data class ExtensionInfo(
    val id: String,
    val name: String,
    val version: String,
    val author: String,
    val description: String,
    val enabled: Boolean = true,
    val permissions: List<String> = emptyList()
)

/**
 * Main IDE Application
 */
class IdeApplication {
    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())
    private val engineIntegration = EngineIntegrationFactory.createIntegration()
    private var currentState = IdeState()

    // Core systems
    private val projectManager = ProjectManager()
    private val assetManager = AssetManager()
    private val buildManager = BuildManager()
    private val pluginManager = PluginManager()
    private val debugManager = DebugManager()

    // Event listeners
    private val stateListeners = mutableListOf<(IdeState) -> Unit>()
    private val projectListeners = mutableListOf<(ProjectInfo?) -> Unit>()
    private val buildListeners = mutableListOf<(BuildStatus) -> Unit>()
    private val debugListeners = mutableListOf<(DebugEvent) -> Unit>()

    init {
        // Initialize engine integration
        val config = EngineConfig(
            projectPath = System.getProperty("user.dir") ?: "./",
            engineVersion = "1.0.0"
        )
        engineIntegration.initialize(config)
    }

    /**
     * Get current IDE state
     */
    fun getCurrentState(): IdeState = currentState

    /**
     * Add state change listener
     */
    fun addStateListener(listener: (IdeState) -> Unit) {
        stateListeners.add(listener)
    }

    /**
     * Add project change listener
     */
    fun addProjectListener(listener: (ProjectInfo?) -> Unit) {
        projectListeners.add(listener)
    }

    /**
     * Update IDE state and notify listeners
     */
    private fun updateState(newState: IdeState) {
        currentState = newState
        stateListeners.forEach { it(newState) }
    }

    /**
     * Update project and notify listeners
     */
    private fun updateProject(project: ProjectInfo?) {
        currentState = currentState.copy(currentProject = project)
        projectListeners.forEach { it(project) }
        stateListeners.forEach { it(currentState) }
    }

    /**
     * Create a new project
     */
    fun createProject(name: String, path: String): Boolean {
        return try {
            val project = ProjectInfo(
                name = name,
                path = path,
                entities = emptyList(),
                components = getDefaultComponents(),
                systems = getDefaultSystems()
            )

            val success = engineIntegration.createProject(project)
            if (success) {
                updateProject(project)
                updateState(currentState.copy(
                    activeView = IdeView.WORLD_EDITOR,
                    recentProjects = listOf(path) + currentState.recentProjects.take(9)
                ))
            }
            success
        } catch (e: Exception) {
            println("Failed to create project: ${e.message}")
            false
        }
    }

    /**
     * Load an existing project
     */
    fun loadProject(path: String): Boolean {
        return try {
            val project = engineIntegration.loadProject(path)
            if (project != null) {
                updateProject(project)
                updateState(currentState.copy(
                    activeView = IdeView.WORLD_EDITOR,
                    recentProjects = listOf(path) + currentState.recentProjects.filter { it != path }.take(9)
                ))
                true
            } else {
                false
            }
        } catch (e: Exception) {
            println("Failed to load project: ${e.message}")
            false
        }
    }

    /**
     * Save current project
     */
    fun saveProject(): Boolean {
        return try {
            val project = currentState.currentProject
            if (project != null) {
                engineIntegration.saveProject(project)
            } else {
                false
            }
        } catch (e: Exception) {
            println("Failed to save project: ${e.message}")
            false
        }
    }

    /**
     * Build project for specified target
     */
    fun buildProject(target: String): BuildResult {
        updateState(currentState.copy(buildStatus = BuildStatus.BUILDING))

        return try {
            val result = engineIntegration.buildProject(target)
            updateState(currentState.copy(
                buildStatus = if (result.success) BuildStatus.SUCCESS else BuildStatus.FAILED
            ))
            result
        } catch (e: Exception) {
            updateState(currentState.copy(buildStatus = BuildStatus.FAILED))
            BuildResult(
                success = false,
                errors = listOf("Build failed: ${e.message}")
            )
        }
    }

    /**
     * Run project on specified target
     */
    fun runProject(target: String): Boolean {
        return try {
            engineIntegration.runProject(target)
        } catch (e: Exception) {
            println("Failed to run project: ${e.message}")
            false
        }
    }

    /**
     * Stop running project
     */
    fun stopProject(): Boolean {
        return try {
            engineIntegration.stopProject()
        } catch (e: Exception) {
            println("Failed to stop project: ${e.message}")
            false
        }
    }

    /**
     * Create a new entity in the current project
     */
    fun createEntity(name: String, componentTypes: List<String> = emptyList()): String? {
        return try {
            val entityId = engineIntegration.createEntity(name, componentTypes)
            if (entityId != null && currentState.currentProject != null) {
                val newEntity = EntityInfo(
                    id = entityId,
                    name = name,
                    components = componentTypes
                )
                val updatedProject = currentState.currentProject.copy(
                    entities = currentState.currentProject.entities + newEntity
                )
                updateProject(updatedProject)
            }
            entityId
        } catch (e: Exception) {
            println("Failed to create entity: ${e.message}")
            null
        }
    }

    /**
     * Remove entity from current project
     */
    fun removeEntity(entityId: String): Boolean {
        return try {
            val success = engineIntegration.removeEntity(entityId)
            if (success && currentState.currentProject != null) {
                val updatedProject = currentState.currentProject.copy(
                    entities = currentState.currentProject.entities.filter { it.id != entityId }
                )
                updateProject(updatedProject)
            }
            success
        } catch (e: Exception) {
            println("Failed to remove entity: ${e.message}")
            false
        }
    }

    /**
     * Switch to different IDE view
     */
    fun switchView(view: IdeView) {
        updateState(currentState.copy(activeView = view))
    }

    /**
     * Get available components for entity creation
     */
    fun getAvailableComponents(): List<ComponentInfo> {
        return engineIntegration.getAvailableComponents()
    }

    /**
     * Get available systems
     */
    fun getAvailableSystems(): List<SystemInfo> {
        return engineIntegration.getAvailableSystems()
    }

    /**
     * Install extension
     */
    fun installExtension(extensionInfo: ExtensionInfo): Boolean {
        return try {
            val updatedExtensions = currentState.extensions + extensionInfo
            updateState(currentState.copy(extensions = updatedExtensions))
            true
        } catch (e: Exception) {
            println("Failed to install extension: ${e.message}")
            false
        }
    }

    /**
     * Uninstall extension
     */
    fun uninstallExtension(extensionId: String): Boolean {
        return try {
            val updatedExtensions = currentState.extensions.filter { it.id != extensionId }
            updateState(currentState.copy(extensions = updatedExtensions))
            true
        } catch (e: Exception) {
            println("Failed to uninstall extension: ${e.message}")
            false
        }
    }

    /**
     * Shutdown the IDE
     */
    fun shutdown() {
        scope.cancel()
        engineIntegration.dispose()
    }

    /**
     * Get default components available in the engine
     */
    private fun getDefaultComponents(): List<ComponentInfo> {
        return listOf(
            ComponentInfo(
                id = "transform",
                name = "Transform",
                type = "TransformComponent",
                properties = mapOf(
                    "position" to "Vector3",
                    "rotation" to "Quaternion",
                    "scale" to "Vector3"
                )
            ),
            ComponentInfo(
                id = "mesh",
                name = "Mesh Renderer",
                type = "MeshRenderer",
                properties = mapOf(
                    "mesh" to "Mesh",
                    "material" to "Material"
                )
            ),
            ComponentInfo(
                id = "physics",
                name = "Physics Body",
                type = "PhysicsComponent",
                properties = mapOf(
                    "mass" to "Float",
                    "drag" to "Float",
                    "useGravity" to "Boolean"
                )
            ),
            ComponentInfo(
                id = "ai",
                name = "AI Component",
                type = "AIComponent",
                properties = mapOf(
                    "behaviorTree" to "BehaviorTree",
                    "navigation" to "NavigationMesh"
                )
            )
        )
    }

    /**
     * Get default systems available in the engine
     */
    private fun getDefaultSystems(): List<SystemInfo> {
        return listOf(
            SystemInfo(
                id = "physics",
                name = "Physics System",
                type = "PhysicsSystem",
                priority = 100,
                enabled = true
            ),
            SystemInfo(
                id = "rendering",
                name = "Rendering System",
                type = "RenderingSystem",
                priority = 200,
                enabled = true
            ),
            SystemInfo(
                id = "ai",
                name = "AI System",
                type = "AISystem",
                priority = 50,
                enabled = true
            ),
            SystemInfo(
                id = "animation",
                name = "Animation System",
                type = "AnimationSystem",
                priority = 150,
                enabled = true
            )
        )
    }
}

/**
 * Global IDE application instance
 */
lateinit var ideApp: IdeApplication

/**
 * Initialize the IDE application
 */
fun initializeIde() {
    ideApp = IdeApplication()
}
