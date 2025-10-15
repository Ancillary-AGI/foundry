package com.foundry.ide.managers

import com.foundry.ide.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File
import java.net.URL
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths

/**
 * Plugin Marketplace Manager for Foundry IDE
 * Handles plugin discovery, installation, and marketplace operations
 */
@Serializable
data class MarketplacePlugin(
    val id: String,
    val name: String,
    val version: String,
    val description: String,
    val author: String,
    val license: String,
    val category: String,
    val tags: List<String> = emptyList(),
    val rating: Float = 0.0f,
    val downloadCount: Int = 0,
    val price: Float = 0.0f,
    val isOfficial: Boolean = false,
    val isVerified: Boolean = false,
    val lastUpdated: String,
    val screenshots: List<String> = emptyList(),
    val reviews: List<PluginReview> = emptyList(),
    val dependencies: List<String> = emptyList(),
    val downloadUrl: String,
    val size: Long = 0L
)

@Serializable
data class PluginReview(
    val userName: String,
    val rating: Int,
    val comment: String,
    val date: String
)

@Serializable
data class MarketplaceSearchQuery(
    val query: String? = null,
    val category: String? = null,
    val author: String? = null,
    val tags: List<String> = emptyList(),
    val minRating: Float? = null,
    val maxPrice: Float? = null,
    val sortBy: SortOption = SortOption.RELEVANCE,
    val sortOrder: SortOrder = SortOrder.DESCENDING
)

enum class SortOption {
    RELEVANCE, RATING, DOWNLOADS, PRICE, UPDATED
}

enum class SortOrder {
    ASCENDING, DESCENDING
}

@Serializable
data class MarketplaceResponse<T>(
    val success: Boolean,
    val data: T? = null,
    val error: String? = null,
    val totalCount: Int = 0,
    val page: Int = 1,
    val pageSize: Int = 20
)

sealed class PluginInstallResult {
    data class Success(val pluginId: String) : PluginInstallResult()
    data class Failure(val message: String) : PluginInstallResult()
}

class PluginMarketplaceManager {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
        encodeDefaults = true
    }

    private val marketplaceUrl = "https://marketplace.foundry-ide.com/api"
    private val installedPluginsPath = Paths.get("ide/plugins/installed")
    private val cachePath = Paths.get("ide/cache/marketplace")

    init {
        ensureDirectoriesExist()
    }

    /**
     * Get featured plugins from marketplace
     */
    suspend fun getFeaturedPlugins(): MarketplaceResponse<List<MarketplacePlugin>> {
        return withContext(Dispatchers.IO) {
            try {
                // Try to get from cache first
                val cached = getCachedFeaturedPlugins()
                if (cached != null) {
                    return@withContext cached
                }

                // Fetch from remote
                val url = URL("$marketplaceUrl/plugins/featured")
                val response = url.readText()
                val plugins = json.decodeFromString<List<MarketplacePlugin>>(response)

                val result = MarketplaceResponse(
                    success = true,
                    data = plugins,
                    totalCount = plugins.size
                )

                // Cache the result
                cacheFeaturedPlugins(result)

                result
            } catch (e: Exception) {
                MarketplaceResponse(
                    success = false,
                    error = "Failed to fetch featured plugins: ${e.message}"
                )
            }
        }
    }

    /**
     * Search plugins in marketplace
     */
    suspend fun searchPlugins(query: MarketplaceSearchQuery): MarketplaceResponse<List<MarketplacePlugin>> {
        return withContext(Dispatchers.IO) {
            try {
                val params = buildSearchParams(query)
                val url = URL("$marketplaceUrl/plugins/search?$params")
                val response = url.readText()
                val plugins = json.decodeFromString<List<MarketplacePlugin>>(response)

                MarketplaceResponse(
                    success = true,
                    data = plugins,
                    totalCount = plugins.size
                )
            } catch (e: Exception) {
                MarketplaceResponse(
                    success = false,
                    error = "Search failed: ${e.message}"
                )
            }
        }
    }

    /**
     * Get plugin details
     */
    suspend fun getPluginDetails(pluginId: String): MarketplaceResponse<MarketplacePlugin> {
        return withContext(Dispatchers.IO) {
            try {
                val url = URL("$marketplaceUrl/plugins/$pluginId")
                val response = url.readText()
                val plugin = json.decodeFromString<MarketplacePlugin>(response)

                MarketplaceResponse(success = true, data = plugin)
            } catch (e: Exception) {
                MarketplaceResponse(
                    success = false,
                    error = "Failed to get plugin details: ${e.message}"
                )
            }
        }
    }

    /**
     * Install plugin from marketplace
     */
    suspend fun installPlugin(pluginId: String): PluginInstallResult {
        return withContext(Dispatchers.IO) {
            try {
                // Get plugin details
                val detailsResponse = getPluginDetails(pluginId)
                if (!detailsResponse.success || detailsResponse.data == null) {
                    return@withContext PluginInstallResult.Failure(
                        detailsResponse.error ?: "Failed to get plugin details"
                    )
                }

                val plugin = detailsResponse.data

                // Check if already installed
                if (isPluginInstalled(pluginId)) {
                    return@withContext PluginInstallResult.Failure("Plugin already installed")
                }

                // Check dependencies
                val dependencyCheck = checkDependencies(plugin.dependencies)
                if (!dependencyCheck.success) {
                    return@withContext PluginInstallResult.Failure(
                        "Dependency check failed: ${dependencyCheck.error}"
                    )
                }

                // Download plugin
                val downloadResult = downloadPlugin(plugin.downloadUrl, pluginId)
                if (!downloadResult.success) {
                    return@withContext PluginInstallResult.Failure(
                        downloadResult.error ?: "Download failed"
                    )
                }

                // Install plugin using PluginManager
                val pluginManager = ideApp.pluginManager
                val installResult = pluginManager.installPluginFromUrl(downloadResult.data!!)

                when (installResult) {
                    is com.foundry.ide.managers.PluginLoadResult.Success -> {
                        // Mark as installed
                        markPluginInstalled(pluginId, plugin)
                        PluginInstallResult.Success(pluginId)
                    }
                    is com.foundry.ide.managers.PluginLoadResult.Failure -> {
                        PluginInstallResult.Failure(installResult.message)
                    }
                }
            } catch (e: Exception) {
                PluginInstallResult.Failure("Installation failed: ${e.message}")
            }
        }
    }

    /**
     * Uninstall plugin
     */
    suspend fun uninstallPlugin(pluginId: String): Boolean {
        return withContext(Dispatchers.IO) {
            try {
                val pluginManager = ideApp.pluginManager
                val success = pluginManager.uninstallPlugin(pluginId)

                if (success) {
                    // Remove from installed list
                    unmarkPluginInstalled(pluginId)
                }

                success
            } catch (e: Exception) {
                println("Failed to uninstall plugin: ${e.message}")
                false
            }
        }
    }

    /**
     * Submit plugin review
     */
    suspend fun submitReview(
        pluginId: String,
        rating: Int,
        comment: String
    ): MarketplaceResponse<Unit> {
        return withContext(Dispatchers.IO) {
            try {
                // In a real implementation, this would send to the marketplace API
                // For now, just simulate success
                MarketplaceResponse(success = true, data = Unit)
            } catch (e: Exception) {
                MarketplaceResponse(
                    success = false,
                    error = "Failed to submit review: ${e.message}"
                )
            }
        }
    }

    /**
     * Get installed plugins
     */
    fun getInstalledPlugins(): List<MarketplacePlugin> {
        return try {
            if (!Files.exists(installedPluginsPath)) return emptyList()

            Files.list(installedPluginsPath)
                .filter { it.toString().endsWith(".json") }
                .mapNotNull { path ->
                    try {
                        val content = Files.readString(path)
                        json.decodeFromString<MarketplacePlugin>(content)
                    } catch (e: Exception) {
                        println("Failed to load installed plugin: ${e.message}")
                        null
                    }
                }
                .toList()
        } catch (e: Exception) {
            println("Failed to get installed plugins: ${e.message}")
            emptyList()
        }
    }

    /**
     * Check if plugin is installed
     */
    private fun isPluginInstalled(pluginId: String): Boolean {
        return Files.exists(installedPluginsPath.resolve("$pluginId.json"))
    }

    /**
     * Mark plugin as installed
     */
    private fun markPluginInstalled(pluginId: String, plugin: MarketplacePlugin) {
        try {
            Files.createDirectories(installedPluginsPath)
            val pluginFile = installedPluginsPath.resolve("$pluginId.json")
            val content = json.encodeToString(MarketplacePlugin.serializer(), plugin)
            Files.writeString(pluginFile, content)
        } catch (e: Exception) {
            println("Failed to mark plugin as installed: ${e.message}")
        }
    }

    /**
     * Unmark plugin as installed
     */
    private fun unmarkPluginInstalled(pluginId: String) {
        try {
            val pluginFile = installedPluginsPath.resolve("$pluginId.json")
            Files.deleteIfExists(pluginFile)
        } catch (e: Exception) {
            println("Failed to unmark plugin as installed: ${e.message}")
        }
    }

    /**
     * Check plugin dependencies
     */
    private suspend fun checkDependencies(dependencies: List<String>): MarketplaceResponse<Unit> {
        return withContext(Dispatchers.IO) {
            try {
                val pluginManager = ideApp.pluginManager

                for (dependency in dependencies) {
                    val installedPlugins = pluginManager.getLoadedPlugins()
                    val isInstalled = installedPlugins.any { it.id == dependency }

                    if (!isInstalled) {
                        return@withContext MarketplaceResponse(
                            success = false,
                            error = "Missing dependency: $dependency"
                        )
                    }
                }

                MarketplaceResponse(success = true, data = Unit)
            } catch (e: Exception) {
                MarketplaceResponse(
                    success = false,
                    error = "Dependency check failed: ${e.message}"
                )
            }
        }
    }

    /**
     * Download plugin from URL
     */
    private suspend fun downloadPlugin(url: String, pluginId: String): MarketplaceResponse<String> {
        return withContext(Dispatchers.IO) {
            try {
                val pluginUrl = URL(url)
                val tempFile = Files.createTempFile("plugin_$pluginId", ".jar")

                pluginUrl.openStream().use { input ->
                    Files.copy(input, tempFile)
                }

                MarketplaceResponse(success = true, data = tempFile.toString())
            } catch (e: Exception) {
                MarketplaceResponse(
                    success = false,
                    error = "Download failed: ${e.message}"
                )
            }
        }
    }

    /**
     * Get cached featured plugins
     */
    private fun getCachedFeaturedPlugins(): MarketplaceResponse<List<MarketplacePlugin>>? {
        return try {
            val cacheFile = cachePath.resolve("featured.json")
            if (!Files.exists(cacheFile)) return null

            // Check if cache is still valid (24 hours)
            val lastModified = Files.getLastModifiedTime(cacheFile).toMillis()
            val now = System.currentTimeMillis()
            if (now - lastModified > 24 * 60 * 60 * 1000) return null

            val content = Files.readString(cacheFile)
            json.decodeFromString<MarketplaceResponse<List<MarketplacePlugin>>>(content)
        } catch (e: Exception) {
            null
        }
    }

    /**
     * Cache featured plugins
     */
    private fun cacheFeaturedPlugins(response: MarketplaceResponse<List<MarketplacePlugin>>) {
        try {
            Files.createDirectories(cachePath)
            val cacheFile = cachePath.resolve("featured.json")
            val content = json.encodeToString(
                MarketplaceResponse.serializer(json.serializersModule.serializer()),
                response
            )
            Files.writeString(cacheFile, content)
        } catch (e: Exception) {
            println("Failed to cache featured plugins: ${e.message}")
        }
    }

    /**
     * Build search parameters
     */
    private fun buildSearchParams(query: MarketplaceSearchQuery): String {
        val params = mutableListOf<String>()

        query.query?.let { params.add("q=$it") }
        query.category?.let { params.add("category=$it") }
        query.author?.let { params.add("author=$it") }
        query.minRating?.let { params.add("minRating=$it") }
        query.maxPrice?.let { params.add("maxPrice=$it") }
        params.add("sortBy=${query.sortBy}")
        params.add("sortOrder=${query.sortOrder}")

        if (query.tags.isNotEmpty()) {
            params.add("tags=${query.tags.joinToString(",")}")
        }

        return params.joinToString("&")
    }

    /**
     * Ensure required directories exist
     */
    private fun ensureDirectoriesExist() {
        try {
            Files.createDirectories(installedPluginsPath)
            Files.createDirectories(cachePath)
        } catch (e: Exception) {
            println("Failed to create marketplace directories: ${e.message}")
        }
    }

    /**
     * Shutdown marketplace manager
     */
    fun shutdown() {
        scope.cancel()
    }
}

// Global marketplace manager instance
lateinit var marketplaceManager: PluginMarketplaceManager

/**
 * Initialize the marketplace manager
 */
fun initializeMarketplaceManager() {
    marketplaceManager = PluginMarketplaceManager()
}