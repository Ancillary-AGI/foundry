package com.foundry.ide

import kotlinx.coroutines.*
import kotlinx.serialization.json.Json
import java.io.*
import java.net.Socket
import java.nio.file.Files
import java.nio.file.Paths
import kotlin.concurrent.thread

/**
 * JVM implementation of Engine Integration
 * Uses sockets to communicate with the Foundry C++ engine
 */
class JvmEngineIntegration : EngineIntegration {
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
    }

    private var socket: Socket? = null
    private var output: PrintWriter? = null
    private var input: BufferedReader? = null
    private var messageListener: Thread? = null
    private var isConnected = false

    // JNI native methods for direct C++ integration
    private external fun nativeInitialize(configJson: String): Boolean
    private external fun nativeCreateProject(projectJson: String): Boolean
    private external fun nativeLoadProject(path: String): String?
    private external fun nativeSaveProject(projectJson: String): Boolean
    private external fun nativeBuildProject(target: String): String
    private external fun nativeRunProject(target: String): Boolean
    private external fun nativeStopProject(): Boolean
    private external fun nativeGetProjectInfo(): String?
    private external fun nativeCreateEntity(name: String, componentsJson: String): String?
    private external fun nativeRemoveEntity(entityId: String): Boolean
    private external fun nativeAddComponent(entityId: String, componentType: String): Boolean
    private external fun nativeRemoveComponent(entityId: String, componentId: String): Boolean
    private external fun nativeUpdateEntityTransform(entityId: String, transformJson: String): Boolean
    private external fun nativeGetAvailableComponents(): String
    private external fun nativeGetAvailableSystems(): String
    private external fun nativeDispose()

    companion object {
        init {
            // Load native library
            try {
                System.loadLibrary("FoundryEngineJNI")
            } catch (e: UnsatisfiedLinkError) {
                println("Warning: Native library not found, falling back to socket communication")
            }
        }
    }

    override fun initialize(config: EngineConfig): Boolean {
        return try {
            // Try native JNI first
            try {
                val configJson = EngineUtils.serializeConfig(config)
                if (nativeInitialize(configJson)) {
                    isConnected = true
                    return true
                }
            } catch (e: UnsatisfiedLinkError) {
                // Fall back to socket communication
            }

            // Socket-based communication fallback
            connectViaSocket()
            isConnected = true
        } catch (e: Exception) {
            println("Failed to initialize engine integration: ${e.message}")
            false
        }
    }

    private fun connectViaSocket(): Boolean {
        return try {
            socket = Socket("localhost", 12345)
            output = PrintWriter(socket!!.getOutputStream(), true)
            input = BufferedReader(InputStreamReader(socket!!.getInputStream()))

            // Start message listener thread
            messageListener = thread {
                try {
                    while (isConnected && !socket!!.isClosed) {
                        val message = input?.readLine() ?: break
                        processMessage(message)
                    }
                } catch (e: Exception) {
                    if (isConnected) {
                        println("Socket listener error: ${e.message}")
                    }
                }
            }

            // Send initialization command
            sendCommand("initialize", mapOf(
                "config" to EngineUtils.serializeConfig(EngineConfig(
                    projectPath = System.getProperty("user.dir") ?: "./",
                    engineVersion = "1.0.0"
                ))
            ))

            true
        } catch (e: Exception) {
            println("Failed to connect via socket: ${e.message}")
            false
        }
    }

    override fun createProject(projectInfo: ProjectInfo): Boolean {
        return try {
            if (isNativeAvailable()) {
                nativeCreateProject(EngineUtils.serializeProject(projectInfo))
            } else {
                sendCommand("createProject", mapOf(
                    "project" to EngineUtils.serializeProject(projectInfo)
                )) == "success"
            }
        } catch (e: Exception) {
            println("Failed to create project: ${e.message}")
            false
        }
    }

    override fun loadProject(path: String): ProjectInfo? {
        return try {
            val projectJson = if (isNativeAvailable()) {
                nativeLoadProject(path)
            } else {
                sendCommand("loadProject", mapOf("path" to path))
            }

            if (projectJson != null && projectJson != "null") {
                EngineUtils.deserializeProject(projectJson)
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
            if (isNativeAvailable()) {
                nativeSaveProject(EngineUtils.serializeProject(projectInfo))
            } else {
                sendCommand("saveProject", mapOf(
                    "project" to EngineUtils.serializeProject(projectInfo)
                )) == "success"
            }
        } catch (e: Exception) {
            println("Failed to save project: ${e.message}")
            false
        }
    }

    override fun buildProject(target: String): BuildResult {
        return try {
            val resultJson = if (isNativeAvailable()) {
                nativeBuildProject(target)
            } else {
                sendCommand("buildProject", mapOf("target" to target))
            }

            if (isNativeAvailable()) {
                // Parse native result (assuming it returns JSON)
                json.decodeFromString(BuildResult.serializer(), resultJson)
            } else {
                // Parse socket result
                val success = resultJson == "success"
                BuildResult(
                    success = success,
                    errors = if (!success) listOf("Build failed") else emptyList()
                )
            }
        } catch (e: Exception) {
            println("Failed to build project: ${e.message}")
            BuildResult(
                success = false,
                errors = listOf("Build failed: ${e.message}")
            )
        }
    }

    override fun runProject(target: String): Boolean {
        return try {
            if (isNativeAvailable()) {
                nativeRunProject(target)
            } else {
                sendCommand("runProject", mapOf("target" to target)) == "success"
            }
        } catch (e: Exception) {
            println("Failed to run project: ${e.message}")
            false
        }
    }

    override fun stopProject(): Boolean {
        return try {
            if (isNativeAvailable()) {
                nativeStopProject()
            } else {
                sendCommand("stopProject") == "success"
            }
        } catch (e: Exception) {
            println("Failed to stop project: ${e.message}")
            false
        }
    }

    override fun getProjectInfo(): ProjectInfo? {
        return try {
            val projectJson = if (isNativeAvailable()) {
                nativeGetProjectInfo()
            } else {
                sendCommand("getProjectInfo")
            }

            if (projectJson != null && projectJson != "null") {
                EngineUtils.deserializeProject(projectJson)
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
            if (isNativeAvailable()) {
                nativeCreateEntity(name, json.encodeToString(components))
            } else {
                sendCommand("createEntity", mapOf(
                    "name" to name,
                    "components" to components
                ))
            }
        } catch (e: Exception) {
            println("Failed to create entity: ${e.message}")
            null
        }
    }

    override fun removeEntity(entityId: String): Boolean {
        return try {
            if (isNativeAvailable()) {
                nativeRemoveEntity(entityId)
            } else {
                sendCommand("removeEntity", mapOf("entityId" to entityId)) == "success"
            }
        } catch (e: Exception) {
            println("Failed to remove entity: ${e.message}")
            false
        }
    }

    override fun addComponent(entityId: String, componentType: String): Boolean {
        return try {
            if (isNativeAvailable()) {
                nativeAddComponent(entityId, componentType)
            } else {
                sendCommand("addComponent", mapOf(
                    "entityId" to entityId,
                    "componentType" to componentType
                )) == "success"
            }
        } catch (e: Exception) {
            println("Failed to add component: ${e.message}")
            false
        }
    }

    override fun removeComponent(entityId: String, componentId: String): Boolean {
        return try {
            if (isNativeAvailable()) {
                nativeRemoveComponent(entityId, componentId)
            } else {
                sendCommand("removeComponent", mapOf(
                    "entityId" to entityId,
                    "componentId" to componentId
                )) == "success"
            }
        } catch (e: Exception) {
            println("Failed to remove component: ${e.message}")
            false
        }
    }

    override fun updateEntityTransform(entityId: String, transform: TransformData): Boolean {
        return try {
            if (isNativeAvailable()) {
                nativeUpdateEntityTransform(entityId, json.encodeToString(transform))
            } else {
                sendCommand("updateEntityTransform", mapOf(
                    "entityId" to entityId,
                    "transform" to transform
                )) == "success"
            }
        } catch (e: Exception) {
            println("Failed to update entity transform: ${e.message}")
            false
        }
    }

    override fun getAvailableComponents(): List<ComponentInfo> {
        return try {
            val componentsJson = if (isNativeAvailable()) {
                nativeGetAvailableComponents()
            } else {
                sendCommand("getAvailableComponents")
            }

            if (componentsJson != null && componentsJson != "null") {
                json.decodeFromString<List<ComponentInfo>>(componentsJson)
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
            val systemsJson = if (isNativeAvailable()) {
                nativeGetAvailableSystems()
            } else {
                sendCommand("getAvailableSystems")
            }

            if (systemsJson != null && systemsJson != "null") {
                json.decodeFromString<List<SystemInfo>>(systemsJson)
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
            isConnected = false
            messageListener?.interrupt()

            if (isNativeAvailable()) {
                nativeDispose()
            } else {
                sendCommand("dispose")
                socket?.close()
            }
        } catch (e: Exception) {
            println("Error during dispose: ${e.message}")
        }
    }

    private fun sendCommand(command: String, parameters: Map<String, Any> = emptyMap()): String? {
        return try {
            val message = mapOf(
                "command" to command,
                "parameters" to parameters,
                "timestamp" to System.currentTimeMillis()
            )

            val messageJson = json.encodeToString(Map.serializer(String.serializer(), Any.serializer()), message)
            output?.println(messageJson)
            output?.flush()

            // For synchronous commands, wait for response
            // In a real implementation, you'd want proper async handling
            input?.readLine()
        } catch (e: Exception) {
            println("Failed to send command: ${e.message}")
            null
        }
    }

    private fun processMessage(message: String) {
        try {
            // Process incoming messages from the engine
            val messageMap = json.decodeFromString<Map<String, Any>>(message)
            when (messageMap["type"] as? String) {
                "log" -> {
                    val level = messageMap["level"] as? String ?: "info"
                    val text = messageMap["text"] as? String ?: ""
                    println("Engine [$level]: $text")
                }
                "error" -> {
                    val error = messageMap["error"] as? String ?: "Unknown error"
                    println("Engine error: $error")
                }
                // Handle other message types as needed
            }
        } catch (e: Exception) {
            println("Failed to process message: ${e.message}")
        }
    }

    private fun isNativeAvailable(): Boolean {
        return try {
            // Try to call a simple native method to check availability
            System.getProperty("foundry.native.available") == "true"
        } catch (e: Exception) {
            false
        }
    }
}

/**
 * Factory implementation for JVM
 */
actual object EngineIntegrationFactory {
    actual fun createIntegration(): EngineIntegration {
        return JvmEngineIntegration()
    }
}
