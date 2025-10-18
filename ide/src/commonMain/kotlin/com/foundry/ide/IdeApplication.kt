package com.foundry.ide

import androidx.compose.runtime.*
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application
import com.foundry.ide.ui.*
import kotlinx.coroutines.*

/**
 * Main IDE Application class
 * Manages the overall IDE state and coordinates between different components
 */
class IdeApplication {
    private val engineIntegration = EngineIntegrationFactory.createIntegration()
    private val pluginManager = PluginManager()
    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())

    // UI State
    private var currentProject: ProjectInfo? by mutableStateOf(null)
    private var isProjectRunning by mutableStateOf(false)
    private var buildProgress by mutableStateOf(0f)
    private var statusMessage by mutableStateOf("Ready")

    // Window management
    private val openWindows = mutableListOf<@Composable () -> Unit>()

    init {
        initializeIde()
    }

    private fun initializeIde() {
        scope.launch {
            // Initialize engine integration
            val config = EngineConfig(
                projectPath = System.getProperty("user.dir") ?: "./",
                engineVersion = "1.0.0",
                platformTargets = listOf("desktop", "web", "mobile"),
                buildSettings = BuildSettings()
            )

            if (!engineIntegration.initialize(config)) {
                statusMessage = "Failed to initialize engine integration"
                return@launch
            }

            // Initialize plugin manager
            pluginManager.initialize()

            statusMessage = "IDE initialized successfully"
        }
    }

    /**
     * Create a new project
     */
    fun createProject(name: String, path: String) {
        scope.launch {
            statusMessage = "Creating project..."

            val projectInfo = ProjectInfo(
                name = name,
                path = path,
                entities = emptyList(),
                components = emptyList(),
                systems = emptyList()
            )

            if (engineIntegration.createProject(projectInfo)) {
                currentProject = projectInfo
                statusMessage = "Project created successfully"
            } else {
                statusMessage = "Failed to create project"
            }
        }
    }

    /**
     * Load an existing project
     */
    fun loadProject(path: String) {
        scope.launch {
            statusMessage = "Loading project..."

            val project = engineIntegration.loadProject(path)
            if (project != null) {
                currentProject = project
                statusMessage = "Project loaded successfully"
            } else {
                statusMessage = "Failed to load project"
            }
        }
    }

    /**
     * Save the current project
     */
    fun saveProject() {
        scope.launch {
            currentProject?.let { project ->
                statusMessage = "Saving project..."

                if (engineIntegration.saveProject(project)) {
                    statusMessage = "Project saved successfully"
                } else {
                    statusMessage = "Failed to save project"
                }
            } ?: run {
                statusMessage = "No project to save"
            }
        }
    }

    /**
     * Build the current project for a specific target
     */
    fun buildProject(target: String): BuildResult {
        if (currentProject == null) {
            return BuildResult(success = false, errors = listOf("No project loaded"))
        }

        statusMessage = "Building for $target..."
        buildProgress = 0f

        val result = engineIntegration.buildProject(target)

        buildProgress = 1f
        statusMessage = if (result.success) "Build successful" else "Build failed"

        return result
    }

    /**
     * Run the current project
     */
    fun runProject(target: String) {
        scope.launch {
            if (isProjectRunning) {
                statusMessage = "Project already running"
                return@launch
            }

            statusMessage = "Starting project..."
            isProjectRunning = true

            if (engineIntegration.runProject(target)) {
                statusMessage = "Project running"
            } else {
                statusMessage = "Failed to start project"
                isProjectRunning = false
            }
        }
    }

    /**
     * Stop the running project
     */
    fun stopProject() {
        scope.launch {
            if (!isProjectRunning) {
                statusMessage = "No project running"
                return@launch
            }

            statusMessage = "Stopping project..."

            if (engineIntegration.stopProject()) {
                isProjectRunning = false
                statusMessage = "Project stopped"
            } else {
                statusMessage = "Failed to stop project"
            }
        }
    }

    /**
     * Get the current project information
     */
    fun getCurrentProject(): ProjectInfo? = currentProject

    /**
     * Get the current status message
     */
    fun getStatusMessage(): String = statusMessage

    /**
     * Get build progress (0.0 to 1.0)
     */
    fun getBuildProgress(): Float = buildProgress

    /**
     * Check if a project is currently running
     */
    fun isProjectRunning(): Boolean = isProjectRunning

    /**
     * Open plugin marketplace
     */
    fun openPluginMarketplace() {
        openWindows.add {
            PluginMarketplaceWindow(onClose = { openWindows.remove(it) })
        }
    }

    /**
     * Open character creator
     */
    fun openCharacterCreator() {
        openWindows.add {
            CharacterCreatorWindow(onClose = { openWindows.remove(it) })
        }
    }

    /**
     * Open embedded terminal
     */
    fun openEmbeddedTerminal() {
        openWindows.add {
            EmbeddedTerminalWindow(onClose = { openWindows.remove(it) })
        }
    }

    /**
     * Open MCP integration
     */
    fun openMCPIntegration() {
        openWindows.add {
            MCPIntegrationWindow(onClose = { openWindows.remove(it) })
        }
    }

    /**
     * Open game simulator
     */
    fun openGameSimulator() {
        openWindows.add {
            VideoGameSimulatorWindow(onClose = { openWindows.remove(it) })
        }
    }

    /**
     * Get all open windows
     */
    fun getOpenWindows(): List<@Composable () -> Unit> = openWindows.toList()

    /**
     * Shutdown the IDE
     */
    fun shutdown() {
        scope.launch {
            // Stop running project
            if (isProjectRunning) {
                engineIntegration.stopProject()
            }

            // Shutdown plugin manager
            pluginManager.shutdown()

            // Dispose engine integration
            engineIntegration.dispose()

            // Cancel all coroutines
            scope.cancel()
        }
    }
}

// Global IDE application instance
val ideApp = IdeApplication()

/**
 * Main entry point for desktop IDE
 */
fun main() = application {
    // Main IDE window
    Window(
        onCloseRequest = { ideApp.shutdown(); exitApplication() },
        title = "Foundry IDE v1.0.0"
    ) {
        IdeMainWindow()
    }

    // Additional windows
    ideApp.getOpenWindows().forEach { windowContent ->
        windowContent()
    }
}
