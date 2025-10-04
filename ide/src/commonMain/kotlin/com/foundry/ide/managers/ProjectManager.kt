package com.foundry.ide.managers

import com.foundry.ide.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.security.MessageDigest

/**
 * Project management system for Foundry IDE
 * Handles project creation, loading, saving, and validation
 */
@Serializable
data class ProjectMetadata(
    val name: String,
    val version: String,
    val engineVersion: String,
    val created: Long,
    val modified: Long,
    val author: String,
    val description: String,
    val tags: List<String> = emptyList(),
    val checksum: String = ""
)

class ProjectManager {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
    }

    private val recentProjectsFile = File(System.getProperty("user.home"), ".foundry/ide/recent_projects.json")
    private val templatesDir = Paths.get("ide/templates")

    init {
        ensureDirectoriesExist()
    }

    /**
     * Create a new project from template
     */
    fun createProject(
        name: String,
        path: String,
        template: String = "default-project"
    ): ProjectInfo? {
        return try {
            val projectDir = Paths.get(path)
            val templateDir = templatesDir.resolve(template)

            if (!Files.exists(templateDir)) {
                throw IllegalArgumentException("Template '$template' not found")
            }

            // Create project directory
            Files.createDirectories(projectDir)

            // Copy template files
            copyTemplateFiles(templateDir, projectDir)

            // Create project configuration
            val projectInfo = ProjectInfo(
                name = name,
                path = path,
                entities = emptyList(),
                components = getDefaultComponents(),
                systems = getDefaultSystems()
            )

            // Save project file
            saveProjectFile(projectDir.resolve("foundry.json"), projectInfo)

            // Create metadata
            createProjectMetadata(projectDir, name)

            // Add to recent projects
            addToRecentProjects(path)

            projectInfo
        } catch (e: Exception) {
            println("Failed to create project: ${e.message}")
            null
        }
    }

    /**
     * Load existing project
     */
    fun loadProject(path: String): ProjectInfo? {
        return try {
            val projectFile = Paths.get(path, "foundry.json")

            if (!Files.exists(projectFile)) {
                throw IllegalArgumentException("Project file not found: $projectFile")
            }

            val projectJson = Files.readString(projectFile)
            val projectInfo = json.decodeFromString<ProjectInfo>(ProjectInfo.serializer(), projectJson)

            // Validate project integrity
            if (!validateProjectIntegrity(projectInfo, path)) {
                throw IllegalStateException("Project integrity check failed")
            }

            // Add to recent projects
            addToRecentProjects(path)

            projectInfo
        } catch (e: Exception) {
            println("Failed to load project: ${e.message}")
            null
        }
    }

    /**
     * Save project with backup
     */
    fun saveProject(projectInfo: ProjectInfo): Boolean {
        return try {
            val projectDir = Paths.get(projectInfo.path)

            // Create backup
            createBackup(projectDir, "foundry.json")

            // Update metadata
            updateProjectMetadata(projectDir)

            // Save project file
            saveProjectFile(projectDir.resolve("foundry.json"), projectInfo)

            true
        } catch (e: Exception) {
            println("Failed to save project: ${e.message}")
            false
        }
    }

    /**
     * Get list of recent projects
     */
    fun getRecentProjects(): List<String> {
        return try {
            if (!recentProjectsFile.exists()) return emptyList()

            val recentJson = recentProjectsFile.readText()
            json.decodeFromString<List<String>>(recentJson)
        } catch (e: Exception) {
            println("Failed to load recent projects: ${e.message}")
            emptyList()
        }
    }

    /**
     * Get available project templates
     */
    fun getAvailableTemplates(): List<String> {
        return try {
            if (!Files.exists(templatesDir)) return emptyList()

            Files.list(templatesDir)
                .filter { Files.isDirectory(it) }
                .map { it.fileName.toString() }
                .toList()
        } catch (e: Exception) {
            println("Failed to list templates: ${e.message}")
            emptyList()
        }
    }

    /**
     * Validate project structure and integrity
     */
    private fun validateProjectIntegrity(projectInfo: ProjectInfo, path: String): Boolean {
        return try {
            // Check if project directory exists
            if (!Files.exists(Paths.get(path))) return false

            // Check if required files exist
            val requiredFiles = listOf("foundry.json")
            requiredFiles.forEach { file ->
                if (!Files.exists(Paths.get(path, file))) return false
            }

            // Validate checksum if available
            val metadataFile = Paths.get(path, ".foundry/metadata.json")
            if (Files.exists(metadataFile)) {
                val metadata = loadProjectMetadata(path)
                if (metadata?.checksum?.isNotEmpty() == true) {
                    val currentChecksum = calculateProjectChecksum(path)
                    if (currentChecksum != metadata.checksum) {
                        println("Warning: Project checksum mismatch")
                    }
                }
            }

            true
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Copy template files to project directory
     */
    private fun copyTemplateFiles(templateDir: Path, projectDir: Path) {
        Files.walk(templateDir).use { paths ->
            paths.filter { Files.isRegularFile(it) }
                .forEach { sourceFile ->
                    val relativePath = templateDir.relativize(sourceFile)
                    val targetFile = projectDir.resolve(relativePath)

                    Files.createDirectories(targetFile.parent)
                    Files.copy(sourceFile, targetFile)
                }
        }
    }

    /**
     * Save project file with atomic write
     */
    private fun saveProjectFile(filePath: Path, projectInfo: ProjectInfo) {
        val tempFile = filePath.parent.resolve("${filePath.fileName}.tmp")

        try {
            val projectJson = json.encodeToString(ProjectInfo.serializer(), projectInfo)
            Files.writeString(tempFile, projectJson)

            // Atomic move
            Files.move(tempFile, filePath)
        } catch (e: Exception) {
            Files.deleteIfExists(tempFile)
            throw e
        }
    }

    /**
     * Create project metadata
     */
    private fun createProjectMetadata(projectDir: Path, name: String) {
        val metadataDir = projectDir.resolve(".foundry")
        Files.createDirectories(metadataDir)

        val metadata = ProjectMetadata(
            name = name,
            version = "1.0.0",
            engineVersion = "1.0.0",
            created = System.currentTimeMillis(),
            modified = System.currentTimeMillis(),
            author = System.getProperty("user.name") ?: "Unknown",
            description = "Project created with Foundry IDE",
            checksum = calculateProjectChecksum(projectDir)
        )

        val metadataFile = metadataDir.resolve("metadata.json")
        val metadataJson = json.encodeToString(ProjectMetadata.serializer(), metadata)
        Files.writeString(metadataFile, metadataJson)
    }

    /**
     * Update project metadata
     */
    private fun updateProjectMetadata(projectDir: Path) {
        val metadataFile = projectDir.resolve(".foundry/metadata.json")
        if (Files.exists(metadataFile)) {
            val metadata = loadProjectMetadata(projectDir.path)
            if (metadata != null) {
                val updatedMetadata = metadata.copy(
                    modified = System.currentTimeMillis(),
                    checksum = calculateProjectChecksum(projectDir)
                )

                val metadataJson = json.encodeToString(ProjectMetadata.serializer(), updatedMetadata)
                Files.writeString(metadataFile, metadataJson)
            }
        }
    }

    /**
     * Load project metadata
     */
    private fun loadProjectMetadata(path: String): ProjectMetadata? {
        return try {
            val metadataFile = Paths.get(path, ".foundry/metadata.json")
            if (!Files.exists(metadataFile)) return null

            val metadataJson = Files.readString(metadataFile)
            json.decodeFromString(ProjectMetadata.serializer(), metadataJson)
        } catch (e: Exception) {
            null
        }
    }

    /**
     * Calculate project checksum for integrity verification
     */
    private fun calculateProjectChecksum(projectDir: Path): String {
        return try {
            val digest = MessageDigest.getInstance("SHA-256")
            val projectFile = projectDir.resolve("foundry.json")

            if (Files.exists(projectFile)) {
                val content = Files.readAllBytes(projectFile)
                val hash = digest.digest(content)
                hash.joinToString("") { "%02x".format(it) }
            } else {
                ""
            }
        } catch (e: Exception) {
            ""
        }
    }

    /**
     * Create backup of project file
     */
    private fun createBackup(projectDir: Path, fileName: String) {
        val sourceFile = projectDir.resolve(fileName)
        if (Files.exists(sourceFile)) {
            val backupDir = projectDir.resolve(".foundry/backups")
            Files.createDirectories(backupDir)

            val timestamp = System.currentTimeMillis()
            val backupFile = backupDir.resolve("$fileName.$timestamp.bak")

            Files.copy(sourceFile, backupFile)
        }
    }

    /**
     * Add project to recent projects list
     */
    private fun addToRecentProjects(path: String) {
        try {
            val recentProjects = getRecentProjects().toMutableList()
            recentProjects.remove(path) // Remove if already exists
            recentProjects.add(0, path) // Add to beginning

            // Keep only last 10 projects
            val limitedProjects = recentProjects.take(10)

            recentProjectsFile.parentFile.mkdirs()
            val recentJson = json.encodeToString(List.serializer(String.serializer()), limitedProjects)
            recentProjectsFile.writeText(recentJson)
        } catch (e: Exception) {
            println("Failed to update recent projects: ${e.message}")
        }
    }

    /**
     * Ensure required directories exist
     */
    private fun ensureDirectoriesExist() {
        Files.createDirectories(Paths.get(System.getProperty("user.home"), ".foundry/ide"))
    }

    /**
     * Get default components
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
                id = "camera",
                name = "Camera",
                type = "CameraComponent",
                properties = mapOf(
                    "fov" to "Float",
                    "nearPlane" to "Float",
                    "farPlane" to "Float"
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
                    "useGravity" to "Boolean",
                    "isKinematic" to "Boolean"
                )
            ),
            ComponentInfo(
                id = "audio",
                name = "Audio Source",
                type = "AudioSource",
                properties = mapOf(
                    "clip" to "AudioClip",
                    "volume" to "Float",
                    "loop" to "Boolean"
                )
            ),
            ComponentInfo(
                id = "light",
                name = "Light",
                type = "LightComponent",
                properties = mapOf(
                    "type" to "LightType",
                    "color" to "Color",
                    "intensity" to "Float",
                    "range" to "Float"
                )
            )
        )
    }

    /**
     * Get default systems
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
                id = "audio",
                name = "Audio System",
                type = "AudioSystem",
                priority = 150,
                enabled = true
            ),
            SystemInfo(
                id = "animation",
                name = "Animation System",
                type = "AnimationSystem",
                priority = 180,
                enabled = true
            ),
            SystemInfo(
                id = "input",
                name = "Input System",
                type = "InputSystem",
                priority = 50,
                enabled = true
            ),
            SystemInfo(
                id = "ai",
                name = "AI System",
                type = "AISystem",
                priority = 120,
                enabled = true
            )
        )
    }
}
