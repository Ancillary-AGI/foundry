package com.foundry.ide.managers

import com.foundry.ide.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File
import java.net.URL
import java.net.URLClassLoader
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.security.MessageDigest
import java.util.concurrent.ConcurrentHashMap
import java.util.jar.JarFile

/**
 * Plugin management system for Foundry IDE
 * Handles plugin loading, validation, and lifecycle management
 */
@Serializable
data class PluginMetadata(
    val id: String,
    val name: String,
    val version: String,
    val description: String,
    val author: String,
    val license: String,
    val homepage: String? = null,
    val repository: String? = null,
    val dependencies: List<String> = emptyList(),
    val permissions: List<String> = emptyList(),
    val mainClass: String,
    val checksum: String,
    val installed: Long,
    val enabled: Boolean = true
)

interface IdePlugin {
    val metadata: PluginMetadata
    fun initialize(ideApp: IdeApplication): Boolean
    fun shutdown(): Boolean
    fun getMenuContributions(): List<MenuContribution>
    fun getToolbarContributions(): List<ToolbarContribution>
    fun getViewContributions(): List<ViewContribution>
    fun onProjectLoaded(project: ProjectInfo?)
    fun onProjectClosed()
    fun onBuildStarted(target: String)
    fun onBuildCompleted(result: com.foundry.ide.managers.BuildResult)
}

@Serializable
data class MenuContribution(
    val menu: String,
    val items: List<MenuItem>
)

@Serializable
data class MenuItem(
    val id: String,
    val label: String,
    val action: String,
    val shortcut: String? = null,
    val icon: String? = null
)

@Serializable
data class ToolbarContribution(
    val id: String,
    val position: String,
    val component: String
)

@Serializable
data class ViewContribution(
    val id: String,
    val name: String,
    val category: String,
    val component: String
)

class PluginManager {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
    }

    private val loadedPlugins = ConcurrentHashMap<String, LoadedPlugin>()
    private val pluginClassLoaders = ConcurrentHashMap<String, URLClassLoader>()
    private val pluginsDir = Paths.get("ide/plugins")
    private val disabledPlugins = mutableSetOf<String>()

    init {
        ensureDirectoriesExist()
        loadDisabledPluginsList()
    }

    /**
     * Load plugin from file
     */
    fun loadPlugin(pluginFile: File): PluginLoadResult {
        return try {
            if (!pluginFile.exists() || !pluginFile.name.endsWith(".jar")) {
                return PluginLoadResult.Failure("Invalid plugin file: ${pluginFile.name}")
            }

            // Validate plugin
            val validationResult = validatePlugin(pluginFile)
            if (validationResult !is PluginValidationResult.Valid) {
                return PluginLoadResult.Failure("Plugin validation failed: $validationResult")
            }

            // Extract metadata
            val metadata = extractPluginMetadata(pluginFile)
                ?: return PluginLoadResult.Failure("Could not extract plugin metadata")

            // Check dependencies
            val dependencyResult = checkDependencies(metadata)
            if (dependencyResult !is DependencyCheckResult.Success) {
                return PluginLoadResult.Failure("Dependency check failed: $dependencyResult")
            }

            // Load plugin class
            val pluginClass = loadPluginClass(pluginFile, metadata.mainClass)
                ?: return PluginLoadResult.Failure("Could not load plugin class: ${metadata.mainClass}")

            // Instantiate plugin
            val plugin = pluginClass.getDeclaredConstructor().newInstance() as? IdePlugin
                ?: return PluginLoadResult.Failure("Plugin does not implement IdePlugin interface")

            // Initialize plugin
            val success = plugin.initialize(ideApp)
            if (!success) {
                return PluginLoadResult.Failure("Plugin initialization failed")
            }

            // Register loaded plugin
            val loadedPlugin = LoadedPlugin(
                metadata = metadata,
                plugin = plugin,
                file = pluginFile,
                classLoader = pluginClassLoaders[metadata.id]!!
            )

            loadedPlugins[metadata.id] = loadedPlugin

            PluginLoadResult.Success(metadata)
        } catch (e: Exception) {
            PluginLoadResult.Failure("Failed to load plugin: ${e.message}")
        }
    }

    /**
     * Unload plugin
     */
    fun unloadPlugin(pluginId: String): Boolean {
        return try {
            val loadedPlugin = loadedPlugins[pluginId] ?: return false

            // Shutdown plugin
            loadedPlugin.plugin.shutdown()

            // Close class loader
            loadedPlugin.classLoader.close()

            // Remove from loaded plugins
            loadedPlugins.remove(pluginId)
            pluginClassLoaders.remove(pluginId)

            true
        } catch (e: Exception) {
            println("Failed to unload plugin $pluginId: ${e.message}")
            false
        }
    }

    /**
     * Enable plugin
     */
    fun enablePlugin(pluginId: String): Boolean {
        return try {
            val loadedPlugin = loadedPlugins[pluginId] ?: return false

            if (loadedPlugin.metadata.enabled) return true

            // Update metadata
            val updatedMetadata = loadedPlugin.metadata.copy(enabled = true)
            val updatedPlugin = loadedPlugin.copy(metadata = updatedMetadata)
            loadedPlugins[pluginId] = updatedPlugin

            // Save updated metadata
            savePluginMetadata(pluginId, updatedMetadata)

            // Remove from disabled list
            disabledPlugins.remove(pluginId)

            true
        } catch (e: Exception) {
            println("Failed to enable plugin $pluginId: ${e.message}")
            false
        }
    }

    /**
     * Disable plugin
     */
    fun disablePlugin(pluginId: String): Boolean {
        return try {
            val loadedPlugin = loadedPlugins[pluginId] ?: return false

            if (!loadedPlugin.metadata.enabled) return true

            // Update metadata
            val updatedMetadata = loadedPlugin.metadata.copy(enabled = false)
            val updatedPlugin = loadedPlugin.copy(metadata = updatedMetadata)
            loadedPlugins[pluginId] = updatedPlugin

            // Save updated metadata
            savePluginMetadata(pluginId, updatedMetadata)

            // Add to disabled list
            disabledPlugins.add(pluginId)

            true
        } catch (e: Exception) {
            println("Failed to disable plugin $pluginId: ${e.message}")
            false
        }
    }

    /**
     * Get loaded plugins
     */
    fun getLoadedPlugins(): List<PluginMetadata> {
        return loadedPlugins.values.map { it.metadata }
    }

    /**
     * Get available plugins in plugins directory
     */
    fun getAvailablePlugins(): List<PluginMetadata> {
        return try {
            if (!Files.exists(pluginsDir)) return emptyList()

            Files.list(pluginsDir)
                .filter { it.toString().endsWith(".jar") }
                .mapNotNull { pluginFile ->
                    try {
                        extractPluginMetadata(pluginFile.toFile())
                    } catch (e: Exception) {
                        println("Failed to read plugin metadata from ${pluginFile.fileName}: ${e.message}")
                        null
                    }
                }
                .toList()
        } catch (e: Exception) {
            println("Failed to list available plugins: ${e.message}")
            emptyList()
        }
    }

    /**
     * Install plugin from URL
     */
    fun installPluginFromUrl(url: String, targetFileName: String? = null): PluginLoadResult {
        return try {
            val pluginUrl = URL(url)
            val fileName = targetFileName ?: File(pluginUrl.path).name
            val targetFile = pluginsDir.resolve(fileName).toFile()

            // Download plugin
            pluginUrl.openStream().use { input ->
                Files.createDirectories(targetFile.parentFile.toPath())
                Files.copy(input, targetFile.toPath())
            }

            // Load the downloaded plugin
            loadPlugin(targetFile)
        } catch (e: Exception) {
            PluginLoadResult.Failure("Failed to install plugin from URL: ${e.message}")
        }
    }

    /**
     * Validate plugin file
     */
    private fun validatePlugin(pluginFile: File): PluginValidationResult {
        return try {
            // Check if file exists and is readable
            if (!pluginFile.exists() || !pluginFile.canRead()) {
                return PluginValidationResult.Invalid("Plugin file is not accessible")
            }

            // Check file size (reasonable limits)
            val fileSize = pluginFile.length()
            if (fileSize > 100 * 1024 * 1024) { // 100MB limit
                return PluginValidationResult.Invalid("Plugin file is too large: ${fileSize} bytes")
            }

            if (fileSize < 1024) { // 1KB minimum
                return PluginValidationResult.Invalid("Plugin file is too small: ${fileSize} bytes")
            }

            // Check JAR structure
            try {
                JarFile(pluginFile).use { jar ->
                    // Check for required files
                    val entries = jar.entries().toList()
                    val hasMainClass = entries.any { it.name.endsWith(".class") }
                    val hasMetadata = entries.any { it.name == "plugin.json" }

                    if (!hasMainClass) {
                        return PluginValidationResult.Invalid("Plugin does not contain Java classes")
                    }

                    if (!hasMetadata) {
                        return PluginValidationResult.Invalid("Plugin does not contain metadata (plugin.json)")
                    }
                }
            } catch (e: Exception) {
                return PluginValidationResult.Invalid("Plugin is not a valid JAR file")
            }

            // Check checksum
            val checksum = calculateFileChecksum(pluginFile)
            // In a real implementation, you might check against known good checksums

            PluginValidationResult.Valid
        } catch (e: Exception) {
            PluginValidationResult.Invalid("Plugin validation failed: ${e.message}")
        }
    }

    /**
     * Extract plugin metadata from JAR
     */
    private fun extractPluginMetadata(pluginFile: File): PluginMetadata? {
        return try {
            JarFile(pluginFile).use { jar ->
                val metadataEntry = jar.getEntry("plugin.json")
                    ?: return null

                jar.getInputStream(metadataEntry).use { input ->
                    val metadataJson = String(input.readBytes())
                    json.decodeFromString(PluginMetadata.serializer(), metadataJson)
                }
            }
        } catch (e: Exception) {
            println("Failed to extract plugin metadata: ${e.message}")
            null
        }
    }

    /**
     * Load plugin class using custom class loader
     */
    private fun loadPluginClass(pluginFile: File, mainClass: String): Class<*>? {
        return try {
            val jarUrl = pluginFile.toURI().toURL()
            val classLoader = URLClassLoader(arrayOf(jarUrl), this::class.java.classLoader)
            pluginClassLoaders[File(pluginFile).nameWithoutExtension] = classLoader

            classLoader.loadClass(mainClass)
        } catch (e: Exception) {
            println("Failed to load plugin class $mainClass: ${e.message}")
            null
        }
    }

    /**
     * Check plugin dependencies
     */
    private fun checkDependencies(metadata: PluginMetadata): DependencyCheckResult {
        return try {
            metadata.dependencies.forEach { dependency ->
                // Check if dependency is available
                val dependencyPlugin = loadedPlugins.values.find { it.metadata.id == dependency }
                if (dependencyPlugin == null) {
                    return DependencyCheckResult.Failure("Missing dependency: $dependency")
                }

                if (!dependencyPlugin.metadata.enabled) {
                    return DependencyCheckResult.Failure("Dependency disabled: $dependency")
                }
            }

            DependencyCheckResult.Success
        } catch (e: Exception) {
            DependencyCheckResult.Failure("Dependency check failed: ${e.message}")
        }
    }

    /**
     * Calculate file checksum
     */
    private fun calculateFileChecksum(file: File): String {
        return try {
            val digest = MessageDigest.getInstance("SHA-256")
            val fileBytes = Files.readAllBytes(file.toPath())
            val hash = digest.digest(fileBytes)
            hash.joinToString("") { "%02x".format(it) }
        } catch (e: Exception) {
            ""
        }
    }

    /**
     * Save plugin metadata
     */
    private fun savePluginMetadata(pluginId: String, metadata: PluginMetadata) {
        try {
            val metadataFile = pluginsDir.resolve("$pluginId.json")
            val metadataJson = json.encodeToString(PluginMetadata.serializer(), metadata)
            Files.writeString(metadataFile, metadataJson)
        } catch (e: Exception) {
            println("Failed to save plugin metadata: ${e.message}")
        }
    }

    /**
     * Load disabled plugins list
     */
    private fun loadDisabledPluginsList() {
        try {
            val disabledFile = pluginsDir.resolve("disabled.txt")
            if (Files.exists(disabledFile)) {
                val disabledContent = Files.readString(disabledFile)
                disabledPlugins.addAll(disabledContent.lines().filter { it.isNotBlank() })
            }
        } catch (e: Exception) {
            println("Failed to load disabled plugins list: ${e.message}")
        }
    }

    /**
     * Save disabled plugins list
     */
    private fun saveDisabledPluginsList() {
        try {
            val disabledFile = pluginsDir.resolve("disabled.txt")
            val disabledContent = disabledPlugins.joinToString("\n")
            Files.writeString(disabledFile, disabledContent)
        } catch (e: Exception) {
            println("Failed to save disabled plugins list: ${e.message}")
        }
    }

    /**
     * Ensure required directories exist
     */
    private fun ensureDirectoriesExist() {
        Files.createDirectories(pluginsDir)
    }

    /**
     * Get plugin by ID
     */
    fun getPlugin(pluginId: String): IdePlugin? {
        return loadedPlugins[pluginId]?.plugin
    }

    /**
     * Get all menu contributions from loaded plugins
     */
    fun getAllMenuContributions(): Map<String, List<MenuItem>> {
        return loadedPlugins.values
            .filter { it.metadata.enabled }
            .flatMap { plugin ->
                plugin.plugin.getMenuContributions().map { contribution ->
                    contribution.menu to contribution.items
                }
            }
            .groupBy { it.first }
            .mapValues { (_, contributions) ->
                contributions.flatMap { it.second }
            }
    }

    /**
     * Get all toolbar contributions from loaded plugins
     */
    fun getAllToolbarContributions(): List<ToolbarContribution> {
        return loadedPlugins.values
            .filter { it.metadata.enabled }
            .flatMap { it.plugin.getToolbarContributions() }
    }

    /**
     * Get all view contributions from loaded plugins
     */
    fun getAllViewContributions(): List<ViewContribution> {
        return loadedPlugins.values
            .filter { it.metadata.enabled }
            .flatMap { it.plugin.getViewContributions() }
    }

    /**
     * Notify plugins of project events
     */
    fun notifyProjectLoaded(project: ProjectInfo?) {
        loadedPlugins.values.forEach { loadedPlugin ->
            if (loadedPlugin.metadata.enabled) {
                try {
                    loadedPlugin.plugin.onProjectLoaded(project)
                } catch (e: Exception) {
                    println("Plugin ${loadedPlugin.metadata.id} failed to handle project loaded event: ${e.message}")
                }
            }
        }
    }

    /**
     * Notify plugins of build events
     */
    fun notifyBuildStarted(target: String) {
        loadedPlugins.values.forEach { loadedPlugin ->
            if (loadedPlugin.metadata.enabled) {
                try {
                    loadedPlugin.plugin.onBuildStarted(target)
                } catch (e: Exception) {
                    println("Plugin ${loadedPlugin.metadata.id} failed to handle build started event: ${e.message}")
                }
            }
        }
    }

    /**
     * Notify plugins of build completion
     */
    fun notifyBuildCompleted(result: com.foundry.ide.managers.BuildResult) {
        loadedPlugins.values.forEach { loadedPlugin ->
            if (loadedPlugin.metadata.enabled) {
                try {
                    loadedPlugin.plugin.onBuildCompleted(result)
                } catch (e: Exception) {
                    println("Plugin ${loadedPlugin.metadata.id} failed to handle build completed event: ${e.message}")
                }
            }
        }
    }

    /**
     * Shutdown all plugins
     */
    fun shutdownAllPlugins() {
        loadedPlugins.values.forEach { loadedPlugin ->
            try {
                loadedPlugin.plugin.shutdown()
                loadedPlugin.classLoader.close()
            } catch (e: Exception) {
                println("Failed to shutdown plugin ${loadedPlugin.metadata.id}: ${e.message}")
            }
        }

        loadedPlugins.clear()
        pluginClassLoaders.clear()
    }
}

/**
 * Plugin load result
 */
sealed class PluginLoadResult {
    data class Success(val metadata: PluginMetadata) : PluginLoadResult()
    data class Failure(val message: String) : PluginLoadResult()
}

/**
 * Plugin validation result
 */
sealed class PluginValidationResult {
    object Valid : PluginValidationResult()
    data class Invalid(val reason: String) : PluginValidationResult()
}

/**
 * Dependency check result
 */
sealed class DependencyCheckResult {
    object Success : DependencyCheckResult()
    data class Failure(val reason: String) : DependencyCheckResult()
}

/**
 * Loaded plugin data class
 */
private data class LoadedPlugin(
    val metadata: PluginMetadata,
    val plugin: IdePlugin,
    val file: File,
    val classLoader: URLClassLoader
)
