package com.foundry.ide.managers

import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File
import java.nio.file.*
import java.security.MessageDigest
import java.util.concurrent.ConcurrentHashMap

/**
 * Asset management system for Foundry IDE
 * Handles asset import, organization, and optimization
 */
@Serializable
data class AssetInfo(
    val id: String,
    val name: String,
    val type: AssetType,
    val path: String,
    val size: Long,
    val hash: String,
    val imported: Long,
    val modified: Long,
    val metadata: Map<String, String> = emptyMap()
)

enum class AssetType {
    MESH, TEXTURE, AUDIO, SCRIPT, MATERIAL, PREFAB, SCENE, OTHER
}

@Serializable
data class AssetImportSettings(
    val generateMipmaps: Boolean = true,
    val compressTextures: Boolean = true,
    val convertAudio: Boolean = true,
    val optimizeMeshes: Boolean = true,
    val importAnimations: Boolean = true
)

class AssetManager {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
    }

    private val assetCache = ConcurrentHashMap<String, AssetInfo>()
    private val assetWatchers = ConcurrentHashMap<String, FileWatcher>()
    private val importJobs = ConcurrentHashMap<String, Job>()

    private val assetExtensions = mapOf(
        AssetType.MESH to setOf("obj", "fbx", "gltf", "dae", "3ds"),
        AssetType.TEXTURE to setOf("png", "jpg", "jpeg", "tga", "bmp", "tiff"),
        AssetType.AUDIO to setOf("wav", "mp3", "ogg", "flac"),
        AssetType.SCRIPT to setOf("kt", "java", "cpp", "h", "glsl"),
        AssetType.MATERIAL to setOf("mat", "shader"),
        AssetType.SCENE to setOf("scene", "prefab"),
        AssetType.PREFAB to setOf("prefab")
    )

    /**
     * Import asset from file system
     */
    fun importAsset(
        sourcePath: String,
        targetPath: String,
        settings: AssetImportSettings = AssetImportSettings()
    ): AssetInfo? {
        return try {
            // Validate paths to prevent path traversal
            val normalizedSourcePath = Paths.get(sourcePath).normalize().toAbsolutePath()
            val normalizedTargetPath = Paths.get(targetPath).normalize().toAbsolutePath()
            
            if (!isValidAssetPath(normalizedSourcePath.toString()) || 
                !isValidAssetPath(normalizedTargetPath.toString())) {
                return null
            }
            
            val sourceFile = normalizedSourcePath.toFile()
            if (!sourceFile.exists() || !isAllowedAssetExtension(sourceFile.extension)) {
                return null
            }

            val assetType = detectAssetType(sourceFile)
            val assetId = generateAssetId(sourceFile, assetType)

            // Check if already imported
            val existingAsset = assetCache[assetId]
            if (existingAsset != null && existingAsset.hash == calculateFileHash(sourceFile)) {
                return existingAsset
            }

            // Import asset
            val importedAsset = when (assetType) {
                AssetType.MESH -> importMesh(sourceFile, targetPath, settings)
                AssetType.TEXTURE -> importTexture(sourceFile, targetPath, settings)
                AssetType.AUDIO -> importAudio(sourceFile, targetPath, settings)
                AssetType.SCRIPT -> importScript(sourceFile, targetPath, settings)
                else -> importGeneric(sourceFile, targetPath, assetType, settings)
            }

            // Cache asset info
            assetCache[assetId] = importedAsset

            // Save asset metadata
            saveAssetMetadata(targetPath, importedAsset)

            importedAsset
        } catch (e: Exception) {
            println("Failed to import asset: ${e.message}")
            null
        }
    }

    /**
     * Get asset information
     */
    fun getAssetInfo(assetPath: String): AssetInfo? {
        val assetId = generateAssetId(File(assetPath))
        return assetCache[assetId] ?: loadAssetMetadata(assetPath)
    }

    /**
     * Watch asset file for changes
     */
    fun watchAsset(assetPath: String, callback: (AssetInfo) -> Unit): Boolean {
        return try {
            val assetFile = File(assetPath)
            if (!assetFile.exists()) return false

            val watcher = FileWatcher(assetFile) { file ->
                val updatedAsset = getAssetInfo(file.path)
                if (updatedAsset != null) {
                    callback(updatedAsset)
                }
            }

            assetWatchers[assetPath] = watcher
            watcher.start()

            true
        } catch (e: Exception) {
            println("Failed to watch asset: ${e.message}")
            false
        }
    }

    /**
     * Stop watching asset
     */
    fun stopWatchingAsset(assetPath: String) {
        assetWatchers[assetPath]?.stop()
        assetWatchers.remove(assetPath)
    }

    /**
     * Optimize asset
     */
    fun optimizeAsset(assetPath: String, settings: AssetImportSettings): Boolean {
        return try {
            val assetInfo = getAssetInfo(assetPath) ?: return false

            when (assetInfo.type) {
                AssetType.TEXTURE -> optimizeTexture(assetPath, settings)
                AssetType.MESH -> optimizeMesh(assetPath, settings)
                AssetType.AUDIO -> optimizeAudio(assetPath, settings)
                else -> false
            }
        } catch (e: Exception) {
            println("Failed to optimize asset: ${e.message}")
            false
        }
    }

    /**
     * Detect asset type from file
     */
    private fun detectAssetType(file: File): AssetType {
        val extension = file.extension.lowercase()

        return assetExtensions.entries.firstOrNull { (_, extensions) ->
            extension in extensions
        }?.key ?: AssetType.OTHER
    }

    /**
     * Generate unique asset ID
     */
    private fun generateAssetId(file: File, type: AssetType? = null): String {
        val assetType = type ?: detectAssetType(file)
        val hash = calculateFileHash(file)
        return "${assetType.name.lowercase()}_${hash.take(8)}"
    }

    /**
     * Calculate file hash for change detection
     */
    private fun calculateFileHash(file: File): String {
        return try {
            val digest = MessageDigest.getInstance("SHA-256")
            val fileBytes = Files.readAllBytes(file.toPath())
            val hash = digest.digest(fileBytes)
            hash.joinToString("") { "%02x".format(it) }
        } catch (e: Exception) {
            System.currentTimeMillis().toString()
        }
    }

    /**
     * Import mesh asset
     */
    private fun importMesh(
        sourceFile: File,
        targetPath: String,
        settings: AssetImportSettings
    ): AssetInfo {
        // Validate target path
        val normalizedTargetPath = Paths.get(targetPath).normalize().toAbsolutePath()
        if (!isValidAssetPath(normalizedTargetPath.toString())) {
            throw SecurityException("Invalid target path")
        }
        
        // In a real implementation, this would use assimp or similar library
        // For now, just copy the file
        val targetFile = normalizedTargetPath.toFile()
        Files.createDirectories(targetFile.parentFile.toPath())
        Files.copy(sourceFile.toPath(), targetFile.toPath(), StandardCopyOption.REPLACE_EXISTING)

        return AssetInfo(
            id = generateAssetId(sourceFile, AssetType.MESH),
            name = sourceFile.nameWithoutExtension,
            type = AssetType.MESH,
            path = targetPath,
            size = sourceFile.length(),
            hash = calculateFileHash(sourceFile),
            imported = System.currentTimeMillis(),
            modified = sourceFile.lastModified(),
            metadata = mapOf(
                "vertices" to "0", // Would be populated by mesh loader
                "triangles" to "0",
                "materials" to "1"
            )
        )
    }

    /**
     * Import texture asset
     */
    private fun importTexture(
        sourceFile: File,
        targetPath: String,
        settings: AssetImportSettings
    ): AssetInfo {
        val normalizedTargetPath = Paths.get(targetPath).normalize().toAbsolutePath()
        if (!isValidAssetPath(normalizedTargetPath.toString())) {
            throw SecurityException("Invalid target path")
        }
        
        val targetFile = normalizedTargetPath.toFile()
        Files.createDirectories(targetFile.parentFile.toPath())
        Files.copy(sourceFile.toPath(), targetFile.toPath(), StandardCopyOption.REPLACE_EXISTING)

        return AssetInfo(
            id = generateAssetId(sourceFile, AssetType.TEXTURE),
            name = sourceFile.nameWithoutExtension,
            type = AssetType.TEXTURE,
            path = targetPath,
            size = sourceFile.length(),
            hash = calculateFileHash(sourceFile),
            imported = System.currentTimeMillis(),
            modified = sourceFile.lastModified(),
            metadata = mapOf(
                "width" to "0", // Would be populated by image loader
                "height" to "0",
                "format" to sourceFile.extension,
                "hasAlpha" to "false"
            )
        )
    }

    /**
     * Import audio asset
     */
    private fun importAudio(
        sourceFile: File,
        targetPath: String,
        settings: AssetImportSettings
    ): AssetInfo {
        val normalizedTargetPath = Paths.get(targetPath).normalize().toAbsolutePath()
        if (!isValidAssetPath(normalizedTargetPath.toString())) {
            throw SecurityException("Invalid target path")
        }
        
        val targetFile = normalizedTargetPath.toFile()
        Files.createDirectories(targetFile.parentFile.toPath())
        Files.copy(sourceFile.toPath(), targetFile.toPath(), StandardCopyOption.REPLACE_EXISTING)

        return AssetInfo(
            id = generateAssetId(sourceFile, AssetType.AUDIO),
            name = sourceFile.nameWithoutExtension,
            type = AssetType.AUDIO,
            path = targetPath,
            size = sourceFile.length(),
            hash = calculateFileHash(sourceFile),
            imported = System.currentTimeMillis(),
            modified = sourceFile.lastModified(),
            metadata = mapOf(
                "duration" to "0.0", // Would be populated by audio loader
                "channels" to "2",
                "sampleRate" to "44100",
                "format" to sourceFile.extension
            )
        )
    }

    /**
     * Import script asset
     */
    private fun importScript(
        sourceFile: File,
        targetPath: String,
        settings: AssetImportSettings
    ): AssetInfo {
        val normalizedTargetPath = Paths.get(targetPath).normalize().toAbsolutePath()
        if (!isValidAssetPath(normalizedTargetPath.toString())) {
            throw SecurityException("Invalid target path")
        }
        
        val targetFile = normalizedTargetPath.toFile()
        Files.createDirectories(targetFile.parentFile.toPath())
        Files.copy(sourceFile.toPath(), targetFile.toPath(), StandardCopyOption.REPLACE_EXISTING)

        return AssetInfo(
            id = generateAssetId(sourceFile, AssetType.SCRIPT),
            name = sourceFile.nameWithoutExtension,
            type = AssetType.SCRIPT,
            path = targetPath,
            size = sourceFile.length(),
            hash = calculateFileHash(sourceFile),
            imported = System.currentTimeMillis(),
            modified = sourceFile.lastModified(),
            metadata = mapOf(
                "language" to detectScriptLanguage(sourceFile),
                "lines" to countLines(sourceFile).toString(),
                "encoding" to "UTF-8"
            )
        )
    }

    /**
     * Import generic asset
     */
    private fun importGeneric(
        sourceFile: File,
        targetPath: String,
        type: AssetType,
        settings: AssetImportSettings
    ): AssetInfo {
        val normalizedTargetPath = Paths.get(targetPath).normalize().toAbsolutePath()
        if (!isValidAssetPath(normalizedTargetPath.toString())) {
            throw SecurityException("Invalid target path")
        }
        
        val targetFile = normalizedTargetPath.toFile()
        Files.createDirectories(targetFile.parentFile.toPath())
        Files.copy(sourceFile.toPath(), targetFile.toPath(), StandardCopyOption.REPLACE_EXISTING)

        return AssetInfo(
            id = generateAssetId(sourceFile, type),
            name = sourceFile.nameWithoutExtension,
            type = type,
            path = targetPath,
            size = sourceFile.length(),
            hash = calculateFileHash(sourceFile),
            imported = System.currentTimeMillis(),
            modified = sourceFile.lastModified()
        )
    }

    /**
     * Optimize texture asset
     */
    private fun optimizeTexture(path: String, settings: AssetImportSettings): Boolean {
        // In a real implementation, this would use image optimization libraries
        println("Optimizing texture: $path")
        return true
    }

    /**
     * Optimize mesh asset
     */
    private fun optimizeMesh(path: String, settings: AssetImportSettings): Boolean {
        // In a real implementation, this would optimize mesh geometry
        println("Optimizing mesh: $path")
        return true
    }

    /**
     * Optimize audio asset
     */
    private fun optimizeAudio(path: String, settings: AssetImportSettings): Boolean {
        // In a real implementation, this would compress/convert audio
        println("Optimizing audio: $path")
        return true
    }

    /**
     * Detect script language
     */
    private fun detectScriptLanguage(file: File): String {
        return when (file.extension.lowercase()) {
            "kt" -> "Kotlin"
            "java" -> "Java"
            "cpp", "cc", "cxx" -> "C++"
            "h", "hpp" -> "C++ Header"
            "glsl", "vert", "frag" -> "GLSL"
            else -> "Unknown"
        }
    }

    /**
     * Count lines in file
     */
    private fun countLines(file: File): Int {
        return try {
            Files.readAllLines(file.toPath()).size
        } catch (e: Exception) {
            0
        }
    }

    /**
     * Save asset metadata
     */
    private fun saveAssetMetadata(assetPath: String, assetInfo: AssetInfo) {
        try {
            val assetFile = File(assetPath)
            val metadataDir = assetFile.parentFile.resolve(".foundry")
            Files.createDirectories(metadataDir.toPath())

            val metadataFile = metadataDir.resolve("${assetFile.name}.meta")
            val metadataJson = json.encodeToString(AssetInfo.serializer(), assetInfo)
            Files.writeString(metadataFile.toPath(), metadataJson)
        } catch (e: Exception) {
            println("Failed to save asset metadata: ${e.message}")
        }
    }

    /**
     * Load asset metadata
     */
    private fun loadAssetMetadata(assetPath: String): AssetInfo? {
        return try {
            val assetFile = File(assetPath)
            val metadataFile = assetFile.parentFile.resolve(".foundry/${assetFile.name}.meta")

            if (!metadataFile.exists()) return null

            val metadataJson = Files.readString(metadataFile.toPath())
            json.decodeFromString(AssetInfo.serializer(), metadataJson)
        } catch (e: Exception) {
            null
        }
    }

    /**
     * File watcher for asset changes
     */
    private class FileWatcher(
        private val file: File,
        private val callback: (File) -> Unit
    ) {
        private var watcherJob: Job? = null
        private var lastModified = file.lastModified()

        fun start() {
            watcherJob = CoroutineScope(Dispatchers.IO).launch {
                while (isActive) {
                    try {
                        delay(1000) // Check every second

                        if (file.lastModified() > lastModified) {
                            lastModified = file.lastModified()
                            callback(file)
                        }
                    } catch (e: Exception) {
                        break
                    }
                }
            }
        }

        fun stop() {
            watcherJob?.cancel()
        }
    }
    
    /**
     * Validate asset path to prevent path traversal
     */
    private fun isValidAssetPath(path: String): Boolean {
        val normalizedPath = Paths.get(path).normalize().toAbsolutePath().toString()
        return !normalizedPath.contains("..") && 
               !normalizedPath.startsWith("/etc") &&
               !normalizedPath.startsWith("/proc") &&
               !normalizedPath.startsWith("/sys")
    }
    
    /**
     * Check if asset extension is allowed
     */
    private fun isAllowedAssetExtension(extension: String): Boolean {
        val allowedExtensions = setOf(
            "obj", "fbx", "gltf", "dae", "3ds", // Mesh
            "png", "jpg", "jpeg", "tga", "bmp", "tiff", // Texture
            "wav", "mp3", "ogg", "flac", // Audio
            "kt", "java", "cpp", "h", "glsl", // Script
            "mat", "shader", "scene", "prefab" // Other
        )
        return extension.lowercase() in allowedExtensions
    }
}
