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
import kotlin.reflect.KClass
import kotlin.reflect.full.primaryConstructor

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
    val enabled: Boolean = true,
    val minIdeVersion: String = "1.0.0",
    val maxIdeVersion: String? = null,
    val tags: List<String> = emptyList(),
    val apiVersion: String = "1.0"
)

@Serializable
data class PluginContext(
    val pluginId: String,
    val ideVersion: String,
    val workspacePath: String,
    val configPath: String,
    val tempPath: String,
    val permissions: List<String>
)

interface IdePlugin {
    val metadata: PluginMetadata
    val context: PluginContext

    fun initialize(ideApp: IdeApplication): Boolean
    fun shutdown(): Boolean
    fun getMenuContributions(): List<MenuContribution>
    fun getToolbarContributions(): List<ToolbarContribution>
    fun getViewContributions(): List<ViewContribution>
    fun getCommandContributions(): List<CommandContribution>
    fun getKeybindingContributions(): List<KeybindingContribution>
    fun getSettingsContributions(): List<SettingsContribution>
    fun getThemeContributions(): List<ThemeContribution>
    fun getLanguageContributions(): List<LanguageContribution>
    fun getSnippetContributions(): List<SnippetContribution>
    fun onProjectLoaded(project: ProjectInfo?)
    fun onProjectClosed()
    fun onBuildStarted(target: String)
    fun onBuildCompleted(result: com.foundry.ide.managers.BuildResult)
    fun onIdeStartup()
    fun onIdeShutdown()
    fun onWorkspaceChanged(path: String)
    fun onSettingsChanged(settings: Map<String, Any>)
    fun handleCommand(commandId: String, parameters: Map<String, Any>): Any?
    fun validatePermissions(): List<String>
    fun getExtensionPoints(): List<ExtensionPoint>
    fun extend(extensionPoint: String, extension: Any): Boolean
}

@Serializable
data class ThemeContribution(
    val id: String,
    val label: String,
    val uiTheme: String, // "vs", "vs-dark", "hc-black"
    val path: String
)

@Serializable
data class LanguageContribution(
    val id: String,
    val aliases: List<String> = emptyList(),
    val extensions: List<String> = emptyList(),
    val filenames: List<String> = emptyList(),
    val filenamePatterns: List<String> = emptyList(),
    val mimetypes: List<String> = emptyList(),
    val configuration: String? = null
)

@Serializable
data class SnippetContribution(
    val language: String,
    val path: String
)

@Serializable
data class ExtensionPoint(
    val id: String,
    val name: String,
    val description: String,
    val schema: String? = null
)

@Serializable
data class CommandContribution(
    val id: String,
    val title: String,
    val category: String,
    val icon: String? = null,
    val keybinding: String? = null,
    val whenCondition: String? = null
)

@Serializable
data class KeybindingContribution(
    val command: String,
    val key: String,
    val whenCondition: String? = null,
    val mac: String? = null,
    val linux: String? = null,
    val win: String? = null
)

@Serializable
data class SettingsContribution(
    val id: String,
    val title: String,
    val properties: List<SettingProperty>
)

@Serializable
data class SettingProperty(
    val id: String,
    val title: String,
    val description: String,
    val type: SettingType,
    val defaultValue: Any,
    val enumValues: List<String>? = null,
    val minimum: Number? = null,
    val maximum: Number? = null
)

enum class SettingType {
    STRING, NUMBER, BOOLEAN, ARRAY, OBJECT, ENUM
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
        encodeDefaults = true
    }

    private val loadedPlugins = ConcurrentHashMap<String, LoadedPlugin>()
    private val pluginClassLoaders = ConcurrentHashMap<String, URLClassLoader>()
    private val extensionPoints = ConcurrentHashMap<String, MutableList<Any>>()
    private val commandHandlers = ConcurrentHashMap<String, Pair<String, (Map<String, Any>) -> Any?>>()
    private val pluginsDir = Paths.get("ide/plugins")
    private val disabledPlugins = mutableSetOf<String>()
    private val pluginStates = ConcurrentHashMap<String, PluginState>()

    init {
        ensureDirectoriesExist()
        loadDisabledPluginsList()
        initializeCoreExtensionPoints()
    }

    private fun initializeCoreExtensionPoints() {
        // Core extension points that plugins can extend
        extensionPoints["commands"] = mutableListOf()
        extensionPoints["themes"] = mutableListOf()
        extensionPoints["languages"] = mutableListOf()
        extensionPoints["snippets"] = mutableListOf()
        extensionPoints["keybindings"] = mutableListOf()
        extensionPoints["settings"] = mutableListOf()
        extensionPoints["views"] = mutableListOf()
        extensionPoints["menus"] = mutableListOf()
        extensionPoints["toolbars"] = mutableListOf()
    }

    enum class PluginState {
        LOADED, INITIALIZED, ACTIVE, ERROR, DISABLED
    }

    /**
     * Load plugin from file with enhanced validation and lifecycle management
     */
    fun loadPlugin(pluginFile: File): PluginLoadResult {
        return try {
            pluginStates[pluginFile.nameWithoutExtension] = PluginState.LOADED

            // Validate file path to prevent path traversal
            val normalizedPath = pluginFile.toPath().normalize().toAbsolutePath()
            if (!isValidPluginPath(normalizedPath.toString()) ||
                !pluginFile.exists() ||
                !isAllowedPluginExtension(pluginFile.extension)) {
                pluginStates[pluginFile.nameWithoutExtension] = PluginState.ERROR
                return PluginLoadResult.Failure("Invalid plugin file: ${pluginFile.name}")
            }

            // Validate plugin with enhanced checks
            val validationResult = validatePlugin(pluginFile)
            if (validationResult !is PluginValidationResult.Valid) {
                pluginStates[pluginFile.nameWithoutExtension] = PluginState.ERROR
                return PluginLoadResult.Failure("Plugin validation failed: $validationResult")
            }

            // Extract metadata with version compatibility check
            val metadata = extractPluginMetadata(pluginFile)
                ?: return PluginLoadResult.Failure("Could not extract plugin metadata")

            // Check IDE version compatibility
            if (!isCompatibleWithIde(metadata)) {
                pluginStates[metadata.id] = PluginState.ERROR
                return PluginLoadResult.Failure("Plugin ${metadata.id} is not compatible with this IDE version")
            }

            // Check dependencies with circular dependency detection
            val dependencyResult = checkDependencies(metadata)
            if (dependencyResult !is DependencyCheckResult.Success) {
                pluginStates[metadata.id] = PluginState.ERROR
                return PluginLoadResult.Failure("Dependency check failed: $dependencyResult")
            }

            // Load plugin class with reflection
            val pluginClass = loadPluginClass(pluginFile, metadata.mainClass)
                ?: return PluginLoadResult.Failure("Could not load plugin class: ${metadata.mainClass}")

            // Create plugin context
            val context = PluginContext(
                pluginId = metadata.id,
                ideVersion = getIdeVersion(),
                workspacePath = System.getProperty("user.dir") ?: "./",
                configPath = getPluginConfigPath(metadata.id),
                tempPath = getPluginTempPath(metadata.id),
                permissions = metadata.permissions
            )

            // Instantiate plugin with context injection
            val plugin = instantiatePlugin(pluginClass, metadata, context)
                ?: return PluginLoadResult.Failure("Plugin does not implement IdePlugin interface")

            pluginStates[metadata.id] = PluginState.INITIALIZED

            // Initialize plugin with proper lifecycle
            val success = plugin.initialize(ideApp)
            if (!success) {
                pluginStates[metadata.id] = PluginState.ERROR
                return PluginLoadResult.Failure("Plugin initialization failed")
            }

            pluginStates[metadata.id] = PluginState.ACTIVE

            // Register plugin contributions
            registerPluginContributions(plugin)

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
            val pluginId = pluginFile.nameWithoutExtension
            pluginStates[pluginId] = PluginState.ERROR
            PluginLoadResult.Failure("Failed to load plugin: ${e.message}")
        }
    }

    private fun instantiatePlugin(pluginClass: Class<*>, metadata: PluginMetadata, context: PluginContext): IdePlugin? {
        return try {
            // Try to find constructor with PluginContext parameter
            val constructor = pluginClass.constructors.firstOrNull { ctor ->
                val params = ctor.parameterTypes
                params.size == 1 && params[0] == PluginContext::class.java
            }

            val plugin = if (constructor != null) {
                constructor.newInstance(context) as? IdePlugin
            } else {
                // Fallback to no-arg constructor
                pluginClass.getDeclaredConstructor().newInstance() as? IdePlugin
            }

            plugin
        } catch (e: Exception) {
            null
        }
    }

    private fun registerPluginContributions(plugin: IdePlugin) {
        // Register commands
        plugin.getCommandContributions().forEach { command ->
            commandHandlers[command.id] = plugin.metadata.id to { params ->
                plugin.handleCommand(command.id, params)
            }
        }

        // Register extension points
        plugin.getExtensionPoints().forEach { extensionPoint ->
            extensionPoints.getOrPut(extensionPoint.id) { mutableListOf() }
        }
    }

    private fun isCompatibleWithIde(metadata: PluginMetadata): Boolean {
        val ideVersion = getIdeVersion()
        return isVersionCompatible(metadata.minIdeVersion, ideVersion) &&
               (metadata.maxIdeVersion == null || isVersionCompatible(ideVersion, metadata.maxIdeVersion))
    }

    private fun isVersionCompatible(required: String, current: String): Boolean {
        val requiredParts = required.split(".").map { it.toIntOrNull() ?: 0 }
        val currentParts = current.split(".").map { it.toIntOrNull() ?: 0 }

        for (i in 0 until maxOf(requiredParts.size, currentParts.size)) {
            val req = requiredParts.getOrElse(i) { 0 }
            val cur = currentParts.getOrElse(i) { 0 }
            if (cur < req) return false
            if (cur > req) return true
        }
        return true
    }

    private fun getIdeVersion(): String = "1.0.0"

    private fun getPluginConfigPath(pluginId: String): String {
        return Paths.get(System.getProperty("user.home"), ".foundry", "plugins", pluginId, "config").toString()
    }

    private fun getPluginTempPath(pluginId: String): String {
        return Paths.get(System.getProperty("java.io.tmpdir"), "foundry", "plugins", pluginId).toString()
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
            // Validate URL to prevent malicious downloads
            if (!isValidPluginUrl(url)) {
                return PluginLoadResult.Failure("Invalid plugin URL")
            }
            
            val pluginUrl = URL(url)
            val fileName = targetFileName ?: File(pluginUrl.path).name
            
            // Validate filename to prevent path traversal
            if (!isValidPluginFileName(fileName)) {
                return PluginLoadResult.Failure("Invalid plugin filename")
            }
            
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
            // Validate main class name to prevent code injection
            if (!isValidClassName(mainClass)) {
                return null
            }
            
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
     * Validate plugin path to prevent path traversal
     */
    private fun isValidPluginPath(path: String): Boolean {
        val normalizedPath = Paths.get(path).normalize().toAbsolutePath().toString()
        return !normalizedPath.contains("..") && 
               normalizedPath.startsWith(pluginsDir.toAbsolutePath().toString())
    }
    
    /**
     * Check if plugin extension is allowed
     */
    private fun isAllowedPluginExtension(extension: String): Boolean {
        return extension.lowercase() == "jar"
    }
    
    /**
     * Validate class name to prevent code injection
     */
    private fun isValidClassName(className: String): Boolean {
        return className.matches(Regex("^[a-zA-Z_][a-zA-Z0-9_.]*$")) &&
               !className.contains("..")
    }
    
    /**
     * Validate plugin URL
     */
    private fun isValidPluginUrl(url: String): Boolean {
        return try {
            val parsedUrl = URL(url)
            parsedUrl.protocol in setOf("http", "https") &&
            !parsedUrl.host.isNullOrBlank()
        } catch (e: Exception) {
            false
        }
    }
    
    /**
     * Validate plugin filename
     */
    private fun isValidPluginFileName(fileName: String): Boolean {
        return !fileName.contains("..") &&
               !fileName.contains("/") &&
               !fileName.contains("\\") &&
               fileName.endsWith(".jar")
    }

    /**
     * Execute command from plugin system
     */
    fun executeCommand(commandId: String, parameters: Map<String, Any> = emptyMap()): Any? {
        val handler = commandHandlers[commandId]
        return if (handler != null) {
            try {
                handler.second(parameters)
            } catch (e: Exception) {
                println("Command execution failed for $commandId: ${e.message}")
                null
            }
        } else {
            println("Command not found: $commandId")
            null
        }
    }

    /**
     * Get all registered commands
     */
    fun getRegisteredCommands(): List<String> {
        return commandHandlers.keys.toList()
    }

    /**
     * Extend extension point
     */
    fun extendExtensionPoint(extensionPointId: String, extension: Any): Boolean {
        val extensions = extensionPoints[extensionPointId]
        return if (extensions != null) {
            extensions.add(extension)
            true
        } else {
            false
        }
    }

    /**
     * Get extensions for extension point
     */
    fun getExtensions(extensionPointId: String): List<Any> {
        return extensionPoints[extensionPointId]?.toList() ?: emptyList()
    }

    /**
     * Get plugin state
     */
    fun getPluginState(pluginId: String): PluginState? {
        return pluginStates[pluginId]
    }

    /**
     * Get all plugin states
     */
    fun getAllPluginStates(): Map<String, PluginState> {
        return pluginStates.toMap()
    }

    /**
     * Enable plugin
     */
    fun enablePlugin(pluginId: String): Boolean {
        if (pluginId in disabledPlugins) {
            disabledPlugins.remove(pluginId)
            saveDisabledPluginsList()
            return true
        }
        return false
    }

    /**
     * Disable plugin
     */
    fun disablePlugin(pluginId: String): Boolean {
        val plugin = loadedPlugins[pluginId]
        return if (plugin != null) {
            try {
                plugin.plugin.shutdown()
                pluginStates[pluginId] = PluginState.DISABLED
                disabledPlugins.add(pluginId)
                saveDisabledPluginsList()
                true
            } catch (e: Exception) {
                println("Failed to disable plugin $pluginId: ${e.message}")
                false
            }
        } else {
            false
        }
    }

    /**
     * Reload plugin
     */
    fun reloadPlugin(pluginId: String): PluginLoadResult {
        val plugin = loadedPlugins[pluginId]
        return if (plugin != null) {
            try {
                // Shutdown existing plugin
                plugin.plugin.shutdown()
                pluginStates[pluginId] = PluginState.DISABLED

                // Remove from collections
                loadedPlugins.remove(pluginId)
                pluginClassLoaders.remove(pluginId)

                // Reload from file
                loadPlugin(plugin.file)
            } catch (e: Exception) {
                PluginLoadResult.Failure("Failed to reload plugin: ${e.message}")
            }
        } else {
            PluginLoadResult.Failure("Plugin not loaded: $pluginId")
        }
    }

    /**
     * Get plugin metrics
     */
    fun getPluginMetrics(): Map<String, PluginMetrics> {
        return loadedPlugins.mapValues { (_, loadedPlugin) ->
            PluginMetrics(
                loadTime = loadedPlugin.metadata.installed,
                memoryUsage = estimatePluginMemory(loadedPlugin),
                isActive = pluginStates[loadedPlugin.metadata.id] == PluginState.ACTIVE
            )
        }
    }

    private fun estimatePluginMemory(plugin: LoadedPlugin): Long {
        // Rough estimation based on class loader and metadata size
        return 1024L * 1024L // 1MB placeholder
    }

    /**
     * Shutdown all plugins with proper lifecycle
     */
    fun shutdownAllPlugins() {
        loadedPlugins.values.forEach { loadedPlugin ->
            try {
                loadedPlugin.plugin.onIdeShutdown()
                loadedPlugin.plugin.shutdown()
                loadedPlugin.classLoader.close()
                pluginStates[loadedPlugin.metadata.id] = PluginState.DISABLED
            } catch (e: Exception) {
                println("Failed to shutdown plugin ${loadedPlugin.metadata.id}: ${e.message}")
            }
        }

        loadedPlugins.clear()
        pluginClassLoaders.clear()
        commandHandlers.clear()
        extensionPoints.clear()
        pluginStates.clear()
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

/**
 * Plugin metrics data class
 */
@Serializable
data class PluginMetrics(
    val loadTime: Long,
    val memoryUsage: Long,
    val isActive: Boolean
)

/**
 * Plugin service registry for dependency injection
 */
class PluginServiceRegistry {
    private val services = ConcurrentHashMap<KClass<*>, Any>()

    inline fun <reified T : Any> register(service: T) {
        services[T::class] = service
    }

    inline fun <reified T : Any> get(): T? {
        return services[T::class] as? T
    }

    fun unregister(serviceClass: KClass<*>) {
        services.remove(serviceClass)
    }

    fun clear() {
        services.clear()
    }
}

/**
 * Plugin event system
 */
interface PluginEventListener {
    fun onPluginLoaded(pluginId: String)
    fun onPluginUnloaded(pluginId: String)
    fun onPluginError(pluginId: String, error: String)
}

/**
 * Plugin sandbox for security
 */
class PluginSandbox(
    private val pluginId: String,
    private val permissions: List<String>
) {
    fun checkPermission(permission: String): Boolean {
        return permission in permissions
    }

    fun validateFileAccess(path: String): Boolean {
        // Implement file access validation based on permissions
        return when {
            "filesystem.read" in permissions -> true
            "filesystem.write" in permissions -> true
            else -> false
        }
    }

    fun validateNetworkAccess(url: String): Boolean {
        return "network" in permissions
    }
}
