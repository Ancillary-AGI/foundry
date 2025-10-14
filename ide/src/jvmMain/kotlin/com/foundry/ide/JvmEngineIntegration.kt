package com.foundry.ide

import kotlinx.coroutines.*
import kotlinx.serialization.json.Json
import java.io.*
import java.net.*
import java.nio.file.Files
import java.nio.file.Paths

/**
 * JVM implementation of Engine Integration
 * Uses JNI or sockets to communicate with the C++ engine
 */
actual class JvmEngineIntegration : EngineIntegration {
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
    }

    private var socket: Socket? = null
    private var isConnected = false
    private val messageCallbacks = mutableMapOf<String, (String) -> Unit>()
    private var messageIdCounter = 0

    override fun initialize(config: EngineConfig): Boolean {
        return try {
            // Try to connect to engine process via socket
            if (initializeSocket()) {
                isConnected = true
                return true
            }

            // Fall back to JNI if available
            if (initializeJNI()) {
                isConnected = true
                return true
            }

            false
        } catch (e: Exception) {
            println("Failed to initialize engine integration: ${e.message}")
            false
        }
    }

    private fun initializeSocket(): Boolean {
        return try {
            // Connect to engine process on localhost:8080
            socket = Socket("localhost", 8080)
            isConnected = true
            true
        } catch (e: Exception) {
            println("Socket connection failed: ${e.message}")
            false
        }
    }

    private fun initializeJNI(): Boolean {
        return try {
            // Load native library
            System.loadLibrary("foundry_engine")
            true
        } catch (e: Exception) {
            println("JNI initialization failed: ${e.message}")
            false
        }
    }

    override fun createProject(projectInfo: ProjectInfo): Boolean {
        return try {
            val command = mapOf(
                "action" to "create_project",
                "project" to json.encodeToString(ProjectInfo.serializer(), projectInfo)
            )
            val response = sendCommand(command)
            response["success"] == "true"
        } catch (e: Exception) {
            println("Failed to create project: ${e.message}")
            false
        }
    }

    override fun loadProject(path: String): ProjectInfo? {
        return try {
            val command = mapOf(
                "action" to "load_project",
                "path" to path
            )
            val response = sendCommand(command)
            if (response["success"] == "true") {
                val projectJson = response["project"] ?: return null
                json.decodeFromString(ProjectInfo.serializer(), projectJson)
            } else {
                null
            }
        } catch (e: Exception) {
            println("Failed to load project: ${e.message}")
            null
        }
    }

    override fun saveProject(projectInfo: ProjectInfo): Boolean {
        return try {
            val command = mapOf(
                "action" to "save_project",
                "project" to json.encodeToString(ProjectInfo.serializer(), projectInfo)
            )
            val response = sendCommand(command)
            response["success"] == "true"
        } catch (e: Exception) {
            println("Failed to save project: ${e.message}")
            false
        }
    }

    override fun buildProject(target: String): BuildResult {
        return try {
            val command = mapOf(
                "action" to "build_project",
                "target" to target
            )
            val response = sendCommand(command)
            if (response["success"] == "true") {
                BuildResult(
                    success = true,
                    outputPath = response["output_path"],
                    buildTime = response["build_time"]?.toLongOrNull() ?: 0L
                )
            } else {
                BuildResult(
                    success = false,
                    errors = listOf(response["error"] ?: "Build failed")
                )
            }
        } catch (e: Exception) {
            BuildResult(
                success = false,
                errors = listOf("Build failed: ${e.message}")
            )
        }
    }

    override fun runProject(target: String): Boolean {
        return try {
            val command = mapOf(
                "action" to "run_project",
                "target" to target
            )
            val response = sendCommand(command)
            response["success"] == "true"
        } catch (e: Exception) {
            println("Failed to run project: ${e.message}")
            false
        }
    }

    override fun stopProject(): Boolean {
        return try {
            val command = mapOf(
                "action" to "stop_project"
            )
            val response = sendCommand(command)
            response["success"] == "true"
        } catch (e: Exception) {
            println("Failed to stop project: ${e.message}")
            false
        }
    }

    override fun getProjectInfo(): ProjectInfo? {
        return try {
            val command = mapOf(
                "action" to "get_project_info"
            )
            val response = sendCommand(command)
            if (response["success"] == "true") {
                val projectJson = response["project"] ?: return null
                json.decodeFromString(ProjectInfo.serializer(), projectJson)
            } else {
                null
            }
        } catch (e: Exception) {
            println("Failed to get project info: ${e.message}")
            null
        }
    }

    override fun createEntity(name: String, components: List<String>): String? {
        return try {
            val command = mapOf(
                "action" to "create_entity",
                "name" to name,
                "components" to json.encodeToString(List.serializer(String.serializer()), components)
            )
            val response = sendCommand(command)
            if (response["success"] == "true") {
                response["entity_id"]
            } else {
                null
            }
        } catch (e: Exception) {
            println("Failed to create entity: ${e.message}")
            null
        }
    }

    override fun removeEntity(entityId: String): Boolean {
        return try {
            val command = mapOf(
                "action" to "remove_entity",
                "entity_id" to entityId
            )
            val response = sendCommand(command)
            response["success"] == "true"
        } catch (e: Exception) {
            println("Failed to remove entity: ${e.message}")
            false
        }
    }

    override fun addComponent(entityId: String, componentType: String): Boolean {
        return try {
            val command = mapOf(
                "action" to "add_component",
                "entity_id" to entityId,
                "component_type" to componentType
            )
            val response = sendCommand(command)
            response["success"] == "true"
        } catch (e: Exception) {
            println("Failed to add component: ${e.message}")
            false
        }
    }

    override fun removeComponent(entityId: String, componentId: String): Boolean {
        return try {
            val command = mapOf(
                "action" to "remove_component",
                "entity_id" to entityId,
                "component_id" to componentId
            )
            val response = sendCommand(command)
            response["success"] == "true"
        } catch (e: Exception) {
            println("Failed to remove component: ${e.message}")
            false
        }
    }

    override fun updateEntityTransform(entityId: String, transform: TransformData): Boolean {
        return try {
            val command = mapOf(
                "action" to "update_entity_transform",
                "entity_id" to entityId,
                "transform" to json.encodeToString(TransformData.serializer(), transform)
            )
            val response = sendCommand(command)
            response["success"] == "true"
        } catch (e: Exception) {
            println("Failed to update entity transform: ${e.message}")
            false
        }
    }

    override fun getAvailableComponents(): List<ComponentInfo> {
        return try {
            val command = mapOf(
                "action" to "get_available_components"
            )
            val response = sendCommand(command)
            if (response["success"] == "true") {
                val componentsJson = response["components"] ?: return emptyList()
                json.decodeFromString(List.serializer(ComponentInfo.serializer()), componentsJson)
            } else {
                emptyList()
            }
        } catch (e: Exception) {
            println("Failed to get available components: ${e.message}")
            emptyList()
        }
    }

    override fun getAvailableSystems(): List<SystemInfo> {
        return try {
            val command = mapOf(
                "action" to "get_available_systems"
            )
            val response = sendCommand(command)
            if (response["success"] == "true") {
                val systemsJson = response["systems"] ?: return emptyList()
                json.decodeFromString(List.serializer(SystemInfo.serializer()), systemsJson)
            } else {
                emptyList()
            }
        } catch (e: Exception) {
            println("Failed to get available systems: ${e.message}")
            emptyList()
        }
    }

    override fun dispose() {
        try {
            socket?.close()
            socket = null
            isConnected = false
        } catch (e: Exception) {
            println("Failed to dispose engine integration: ${e.message}")
        }
    }

    private fun sendCommand(command: Map<String, String>): Map<String, String> {
        if (!isConnected) {
            throw IllegalStateException("Not connected to engine")
        }

        val messageId = ++messageIdCounter
        val message = mapOf(
            "id" to messageId.toString(),
            "command" to json.encodeToString(Map.serializer(String.serializer(), String.serializer()), command)
        )

        val messageJson = json.encodeToString(Map.serializer(String.serializer(), String.serializer()), message)
        
        return if (socket != null) {
            sendSocketMessage(messageJson)
        } else {
            sendJNIMessage(messageJson)
        }
    }

    private fun sendSocketMessage(message: String): Map<String, String> {
        val socket = this.socket ?: throw IllegalStateException("Socket not connected")
        
        val output = PrintWriter(socket.getOutputStream(), true)
        val input = BufferedReader(InputStreamReader(socket.getInputStream()))
        
        output.println(message)
        val response = input.readLine()
        
        return json.decodeFromString(Map.serializer(String.serializer(), String.serializer()), response)
    }

    private fun sendJNIMessage(message: String): Map<String, String> {
        // JNI implementation would go here
        // For now, return a mock response
        return mapOf(
            "success" to "true",
            "message" to "JNI not implemented"
        )
    }
}

/**
 * JVM Engine Integration Factory
 */
actual object EngineIntegrationFactory {
    actual fun createIntegration(): EngineIntegration {
        return JvmEngineIntegration()
    }
}