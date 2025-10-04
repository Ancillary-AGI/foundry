package com.foundry.ide

import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import kotlinx.coroutines.*

/**
 * Core engine integration interface for the Foundry IDE
 * Provides communication between the IDE and the Foundry game engine
 */
@Serializable
data class EngineConfig(
    val projectPath: String,
    val engineVersion: String = "1.0.0",
    val platformTargets: List<String> = listOf("desktop", "web", "mobile"),
    val buildSettings: BuildSettings = BuildSettings()
)

@Serializable
data class BuildSettings(
    val optimizationLevel: Int = 2,
    val debugMode: Boolean = true,
    val enableProfiling: Boolean = false,
    val targetPlatforms: List<String> = listOf("windows", "linux", "web")
)

@Serializable
data class ProjectInfo(
    val name: String,
    val path: String,
    val entities: List<EntityInfo> = emptyList(),
    val components: List<ComponentInfo> = emptyList(),
    val systems: List<SystemInfo> = emptyList()
)

@Serializable
data class EntityInfo(
    val id: String,
    val name: String,
    val components: List<String> = emptyList(),
    val transform: TransformData? = null
)

@Serializable
data class ComponentInfo(
    val id: String,
    val name: String,
    val type: String,
    val properties: Map<String, String> = emptyMap()
)

@Serializable
data class SystemInfo(
    val id: String,
    val name: String,
    val type: String,
    val priority: Int = 0,
    val enabled: Boolean = true
)

@Serializable
data class TransformData(
    val position: Vector3 = Vector3(0.0, 0.0, 0.0),
    val rotation: Vector3 = Vector3(0.0, 0.0, 0.0),
    val scale: Vector3 = Vector3(1.0, 1.0, 1.0)
)

@Serializable
data class Vector3(
    val x: Double = 0.0,
    val y: Double = 0.0,
    val z: Double = 0.0
)

/**
 * Main engine integration class
 * Handles communication with the Foundry game engine
 */
expect class EngineIntegration {
    fun initialize(config: EngineConfig): Boolean
    fun createProject(projectInfo: ProjectInfo): Boolean
    fun loadProject(path: String): ProjectInfo?
    fun saveProject(projectInfo: ProjectInfo): Boolean
    fun buildProject(target: String): BuildResult
    fun runProject(target: String): Boolean
    fun stopProject(): Boolean
    fun getProjectInfo(): ProjectInfo?
    fun createEntity(name: String, components: List<String>): String?
    fun removeEntity(entityId: String): Boolean
    fun addComponent(entityId: String, componentType: String): Boolean
    fun removeComponent(entityId: String, componentId: String): Boolean
    fun updateEntityTransform(entityId: String, transform: TransformData): Boolean
    fun getAvailableComponents(): List<ComponentInfo>
    fun getAvailableSystems(): List<SystemInfo>
    fun dispose()
}

@Serializable
data class BuildResult(
    val success: Boolean,
    val outputPath: String? = null,
    val errors: List<String> = emptyList(),
    val warnings: List<String> = emptyList(),
    val buildTime: Long = 0L
)

/**
 * Platform-specific engine integration implementations
 */

// JVM implementation would use JNI or sockets to communicate with C++ engine
// JS implementation would use WebAssembly or WebSockets

/**
 * Common engine communication interface
 */
interface EngineCommunicator {
    suspend fun sendCommand(command: String, parameters: Map<String, Any> = emptyMap()): String
    suspend fun receiveMessage(): String?
    fun isConnected(): Boolean
    fun disconnect()
}

/**
 * Factory for creating platform-specific engine integration
 */
expect object EngineIntegrationFactory {
    fun createIntegration(): EngineIntegration
}

/**
 * Utility functions for engine integration
 */
object EngineUtils {
    val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
        encodeDefaults = true
    }

    fun serializeConfig(config: EngineConfig): String {
        return json.encodeToString(EngineConfig.serializer(), config)
    }

    fun deserializeConfig(jsonString: String): EngineConfig {
        return json.decodeFromString(EngineConfig.serializer(), jsonString)
    }

    fun serializeProject(project: ProjectInfo): String {
        return json.encodeToString(ProjectInfo.serializer(), project)
    }

    fun deserializeProject(jsonString: String): ProjectInfo {
        return json.decodeFromString(ProjectInfo.serializer(), jsonString)
    }
}
