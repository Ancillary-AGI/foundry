package com.foundry.ide.publishing

import com.foundry.ide.*
import com.foundry.ide.auth.*
import com.foundry.ide.managers.*
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import kotlinx.coroutines.*
import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import java.security.MessageDigest
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

/**
 * Plugin publishing system for Foundry IDE
 * Handles plugin packaging, signing, and marketplace publishing
 */
@Serializable
data class PluginPackage(
    val metadata: PluginMetadata,
    val files: List<String>,
    val checksum: String,
    val signature: String? = null,
    val signedBy: String? = null
)

@Serializable
data class PublishRequest(
    val pluginId: String,
    val version: String,
    val packageData: String, // Base64 encoded
    val signature: String,
    val changelog: String? = null,
    val screenshots: List<String> = emptyList(),
    val tags: List<String> = emptyList()
)

@Serializable
data class PublishResponse(
    val success: Boolean,
    val pluginId: String? = null,
    val version: String? = null,
    val message: String? = null,
    val error: String? = null
)

enum class PublishStatus {
    DRAFT, SUBMITTED, REVIEWING, APPROVED, PUBLISHED, REJECTED
}

class PluginPublisher {
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
    }

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    /**
     * Package plugin for publishing
     */
    suspend fun packagePlugin(
        pluginPath: String,
        outputPath: String,
        signPackage: Boolean = true
    ): Result<PluginPackage> {
        return try {
            val pluginDir = File(pluginPath)
            if (!pluginDir.exists() || !pluginDir.isDirectory) {
                return Result.failure(IllegalArgumentException("Plugin directory does not exist"))
            }

            // Read plugin metadata
            val metadataFile = File(pluginDir, "plugin.json")
            if (!metadataFile.exists()) {
                return Result.failure(IllegalArgumentException("plugin.json not found"))
            }

            val metadata = json.decodeFromString(
                PluginMetadata.serializer(),
                metadataFile.readText()
            )

            // Collect all plugin files
            val files = collectPluginFiles(pluginDir)

            // Create package data
            val packageData = createPackageData(pluginDir, files)

            // Calculate checksum
            val checksum = calculateChecksum(packageData)

            // Sign package if requested
            val signature = if (signPackage) {
                signPackage(packageData, metadata.id)
            } else null

            val pluginPackage = PluginPackage(
                metadata = metadata,
                files = files,
                checksum = checksum,
                signature = signature,
                signedBy = if (signature != null) authManager.getCurrentUser()?.username else null
            )

            // Save package to output path
            savePackage(pluginPackage, packageData, outputPath)

            Result.success(pluginPackage)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    /**
     * Publish plugin to marketplace
     */
    suspend fun publishPlugin(
        packagePath: String,
        changelog: String? = null,
        screenshots: List<String> = emptyList(),
        tags: List<String> = emptyList()
    ): Result<PublishResponse> {
        return try {
            // Check authentication
            if (!authManager.isAuthenticated()) {
                return Result.failure(IllegalStateException("User must be authenticated to publish plugins"))
            }

            // Check permissions
            if (!authManager.hasPermission(Permission.PLUGIN_PUBLISH)) {
                return Result.failure(IllegalStateException("User does not have permission to publish plugins"))
            }

            // Load package
            val packageFile = File(packagePath)
            if (!packageFile.exists()) {
                return Result.failure(IllegalArgumentException("Package file does not exist"))
            }

            val packageData = packageFile.readBytes()
            val packageJson = String(packageData)
            val pluginPackage = json.decodeFromString(PluginPackage.serializer(), packageJson)

            // Validate package
            val validationResult = validatePackage(pluginPackage, packageData)
            if (validationResult.isFailure) {
                return validationResult as Result<PublishResponse>
            }

            // Create publish request
            val publishRequest = PublishRequest(
                pluginId = pluginPackage.metadata.id,
                version = pluginPackage.metadata.version,
                packageData = java.util.Base64.getEncoder().encodeToString(packageData),
                signature = pluginPackage.signature ?: "",
                changelog = changelog,
                screenshots = screenshots,
                tags = tags
            )

            // Submit to marketplace
            val response = submitToMarketplace(publishRequest)

            if (response.success) {
                Result.success(response)
            } else {
                Result.failure(Exception(response.error ?: "Publishing failed"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    /**
     * Update existing plugin
     */
    suspend fun updatePlugin(
        pluginId: String,
        packagePath: String,
        changelog: String? = null
    ): Result<PublishResponse> {
        return try {
            // Check if user owns the plugin
            val publisherProfile = authManager.getPublisherProfile(authManager.getCurrentUser()?.username ?: "")
            if (publisherProfile.isFailure || pluginId !in publisherProfile.getOrThrow().plugins) {
                return Result.failure(IllegalStateException("User does not own this plugin"))
            }

            // Publish as update
            publishPlugin(packagePath, changelog)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    /**
     * Check publishing status
     */
    suspend fun checkPublishStatus(pluginId: String, version: String): Result<PublishStatus> {
        return try {
            val response = makeAuthenticatedRequest(
                "GET",
                "/plugins/$pluginId/versions/$version/status"
            )

            if (response.success) {
                val status = when (response.data["status"] as String) {
                    "draft" -> PublishStatus.DRAFT
                    "submitted" -> PublishStatus.SUBMITTED
                    "reviewing" -> PublishStatus.REVIEWING
                    "approved" -> PublishStatus.APPROVED
                    "published" -> PublishStatus.PUBLISHED
                    "rejected" -> PublishStatus.REJECTED
                    else -> PublishStatus.DRAFT
                }
                Result.success(status)
            } else {
                Result.failure(Exception(response.error ?: "Failed to check status"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    /**
     * Get publishing statistics
     */
    suspend fun getPublishingStats(): Result<PublisherStats> {
        return try {
            val response = makeAuthenticatedRequest("GET", "/publishers/stats")

            if (response.success) {
                val stats = PublisherStats(
                    totalPlugins = response.data["totalPlugins"] as Int,
                    totalDownloads = response.data["totalDownloads"] as Long,
                    averageRating = response.data["averageRating"] as Double,
                    reputation = response.data["reputation"] as Double,
                    verified = response.data["verified"] as Boolean
                )
                Result.success(stats)
            } else {
                Result.failure(Exception(response.error ?: "Failed to get stats"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // Private helper methods

    private fun collectPluginFiles(pluginDir: File): List<String> {
        val files = mutableListOf<String>()

        pluginDir.walkTopDown().forEach { file ->
            if (file.isFile && file.name != ".DS_Store" && !file.name.startsWith(".")) {
                val relativePath = pluginDir.toPath().relativize(file.toPath()).toString()
                files.add(relativePath)
            }
        }

        return files.sorted()
    }

    private fun createPackageData(pluginDir: File, files: List<String>): ByteArray {
        val baos = java.io.ByteArrayOutputStream()
        ZipOutputStream(baos).use { zos ->
            files.forEach { filePath ->
                val file = File(pluginDir, filePath)
                if (file.exists()) {
                    zos.putNextEntry(ZipEntry(filePath))
                    file.inputStream().use { it.copyTo(zos) }
                    zos.closeEntry()
                }
            }
        }
        return baos.toByteArray()
    }

    private fun calculateChecksum(data: ByteArray): String {
        val digest = MessageDigest.getInstance("SHA-256")
        val hash = digest.digest(data)
        return hash.joinToString("") { "%02x".format(it) }
    }

    private fun signPackage(packageData: ByteArray, pluginId: String): String {
        // Get signing key for this plugin/user
        val keys = authManager.generateSigningKeys() // In practice, get existing key
        return authManager.signPlugin(packageData, keys.keyId)
    }

    private fun savePackage(pluginPackage: PluginPackage, packageData: ByteArray, outputPath: String) {
        val outputFile = File(outputPath)
        outputFile.parentFile?.mkdirs()

        // Save package metadata
        val metadataJson = json.encodeToString(PluginPackage.serializer(), pluginPackage)
        File(outputFile.parent, "${pluginPackage.metadata.id}-${pluginPackage.metadata.version}.json")
            .writeText(metadataJson)

        // Save package data
        outputFile.writeBytes(packageData)
    }

    private fun validatePackage(pluginPackage: PluginPackage, packageData: ByteArray): Result<PublishResponse> {
        // Validate metadata
        if (pluginPackage.metadata.id.isBlank()) {
            return Result.failure(IllegalArgumentException("Plugin ID is required"))
        }

        if (pluginPackage.metadata.name.isBlank()) {
            return Result.failure(IllegalArgumentException("Plugin name is required"))
        }

        if (pluginPackage.metadata.mainClass.isBlank()) {
            return Result.failure(IllegalArgumentException("Main class is required"))
        }

        // Validate checksum
        val calculatedChecksum = calculateChecksum(packageData)
        if (calculatedChecksum != pluginPackage.checksum) {
            return Result.failure(IllegalArgumentException("Package checksum mismatch"))
        }

        // Validate signature if present
        if (pluginPackage.signature != null && pluginPackage.signedBy != null) {
            // In practice, verify against publisher's public key
            val isValid = authManager.verifyPluginSignature(
                packageData,
                pluginPackage.signature,
                "dummy-public-key" // Would get from publisher profile
            )

            if (!isValid) {
                return Result.failure(IllegalArgumentException("Invalid package signature"))
            }
        }

        return Result.success(PublishResponse(success = true))
    }

    private suspend fun submitToMarketplace(request: PublishRequest): PublishResponse {
        return try {
            val response = makeAuthenticatedRequest(
                "POST",
                "/plugins/publish",
                mapOf(
                    "pluginId" to request.pluginId,
                    "version" to request.version,
                    "packageData" to request.packageData,
                    "signature" to request.signature,
                    "changelog" to (request.changelog ?: ""),
                    "screenshots" to json.encodeToString(List.serializer(String.serializer()), request.screenshots),
                    "tags" to json.encodeToString(List.serializer(String.serializer()), request.tags)
                )
            )

            PublishResponse(
                success = response.success,
                pluginId = response.data["pluginId"] as? String,
                version = response.data["version"] as? String,
                message = response.data["message"] as? String,
                error = response.error
            )
        } catch (e: Exception) {
            PublishResponse(success = false, error = e.message)
        }
    }

    // Network request helpers
    private data class ApiResponse(
        val success: Boolean,
        val data: Map<String, Any> = emptyMap(),
        val error: String? = null
    )

    private suspend fun makeAuthenticatedRequest(method: String, endpoint: String, data: Map<String, Any> = emptyMap()): ApiResponse {
        // Implementation for making authenticated HTTP requests to marketplace
        return ApiResponse(success = false, error = "Not implemented")
    }
}

@Serializable
data class PublisherStats(
    val totalPlugins: Int,
    val totalDownloads: Long,
    val averageRating: Double,
    val reputation: Double,
    val verified: Boolean
)

// Global plugin publisher instance
val pluginPublisher = PluginPublisher()