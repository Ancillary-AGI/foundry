package com.foundry.ide

import kotlinx.coroutines.*
import kotlinx.serialization.json.Json
import kotlinx.browser.window
import kotlinx.browser.document
import org.w3c.dom.WebSocket
import org.w3c.dom.events.Event
import kotlin.js.Promise

/**
 * JavaScript/WebAssembly implementation of Engine Integration
 * Uses WebSockets and WebAssembly to communicate with the Foundry engine
 */
class JsEngineIntegration : EngineIntegration {
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
    }

    private var webSocket: WebSocket? = null
    private var wasmModule: dynamic = null
    private var isConnected = false
    private val messageCallbacks = mutableMapOf<String, (String) -> Unit>()
    private var messageIdCounter = 0

    override fun initialize(config: EngineConfig): Boolean {
        return try {
            // Try WebAssembly first
            if (initializeWasm()) {
                isConnected = true
                return true
            }

            // Fall back to WebSocket communication
            initializeWebSocket()
            isConnected = true
        } catch (e: Exception) {
            console.error("Failed to initialize engine integration: ${e.message}")
            false
        }
    }

    private fun initializeWasm(): Boolean {
        return try {
            // Load WebAssembly module
            window.asDynamic().Module = window.asDynamic().Module ?: object {}
            window.asDynamic().Module.locateFile = { fileName: String ->
                "assets/wasm/$fileName"
            }

            // Initialize WASM module (this would be generated from the C++ engine)
            // For now, this is a placeholder
            wasmModule = window.asDynamic().FoundryEngine()
            true
        } catch (e: Exception) {
            console.warn("WebAssembly not available, falling back to WebSocket")
            false
        }
    }

    private fun initializeWebSocket(): Boolean {
        return try {
            webSocket = WebSocket("ws://localhost:8080/engine")

            webSocket?.onopen = { event ->
                console.log("Connected to Foundry engine")
                isConnected = true

                // Send initialization command
                sendCommand("initialize", mapOf(
                    "config" to EngineUtils.serializeConfig(EngineConfig(
                        projectPath = "./",
                        engineVersion = "1.0.0"
                    ))
                ))
            }

            webSocket?.onmessage = { event ->
                val message = event.data as String
                processMessage(message)
            }

            webSocket?.onclose = { event ->
                console.log("Disconnected from Foundry engine")
                isConnected = false
            }

            webSocket?.onerror = { event ->
                console.error("WebSocket error: ${event.type}")
            }

            true
        } catch (e: Exception) {
            console.error("Failed to initialize WebSocket: ${e.message}")
            false
        }
    }

    override fun createProject(projectInfo: ProjectInfo): Boolean {
        return sendCommandWithResponse("createProject", mapOf(
            "project" to EngineUtils.serializeProject(projectInfo)
        )) == "success"
    }

    override fun loadProject(path: String): ProjectInfo? {
        val projectJson = sendCommandWithResponse("loadProject", mapOf("path" to path))
        return if (projectJson != null && projectJson != "null") {
            EngineUtils.deserializeProject(projectJson)
        } else {
            null
        }
    }

    override fun saveProject(projectInfo: ProjectInfo): Boolean {
        return sendCommandWithResponse("saveProject", mapOf(
            "project" to EngineUtils.serializeProject(projectInfo)
        )) == "success"
    }

    override fun buildProject(target: String): BuildResult {
        val resultJson = sendCommandWithResponse("buildProject", mapOf("target" to target))

        return try {
            if (wasmModule != null) {
                // Use WebAssembly for building
                val result = wasmModule.buildProject(target)
                json.decodeFromString(BuildResult.serializer(), result.toString())
            } else {
                // Parse WebSocket result
                val success = resultJson == "success"
                BuildResult(
                    success = success,
                    errors = if (!success) listOf("Build failed") else emptyList()
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
        return sendCommandWithResponse("runProject", mapOf("target" to target)) == "success"
    }

    override fun stopProject(): Boolean {
        return sendCommandWithResponse("stopProject") == "success"
    }

    override fun getProjectInfo(): ProjectInfo? {
        val projectJson = sendCommandWithResponse("getProjectInfo")
        return if (projectJson != null && projectJson != "null") {
            EngineUtils.deserializeProject(projectJson)
        } else {
            null
        }
    }

    override fun createEntity(name: String, components: List<String>): String? {
        return sendCommandWithResponse("createEntity", mapOf(
            "name" to name,
            "components" to components
        ))
    }

    override fun removeEntity(entityId: String): Boolean {
        return sendCommandWithResponse("removeEntity", mapOf("entityId" to entityId)) == "success"
    }

    override fun addComponent(entityId: String, componentType: String): Boolean {
        return sendCommandWithResponse("addComponent", mapOf(
            "entityId" to entityId,
            "componentType" to componentType
        )) == "success"
    }

    override fun removeComponent(entityId: String, componentId: String): Boolean {
        return sendCommandWithResponse("removeComponent", mapOf(
            "entityId" to entityId,
            "componentId" to componentId
        )) == "success"
    }

    override fun updateEntityTransform(entityId: String, transform: TransformData): Boolean {
        return sendCommandWithResponse("updateEntityTransform", mapOf(
            "entityId" to entityId,
            "transform" to transform
        )) == "success"
    }

    override fun getAvailableComponents(): List<ComponentInfo> {
        val componentsJson = sendCommandWithResponse("getAvailableComponents")
        return if (componentsJson != null && componentsJson != "null") {
            try {
                json.decodeFromString<List<ComponentInfo>>(componentsJson)
            } catch (e: Exception) {
                console.error("Failed to parse components: ${e.message}")
                emptyList()
            }
        } else {
            emptyList()
        }
    }

    override fun getAvailableSystems(): List<SystemInfo> {
        val systemsJson = sendCommandWithResponse("getAvailableSystems")
        return if (systemsJson != null && systemsJson != "null") {
            try {
                json.decodeFromString<List<SystemInfo>>(systemsJson)
            } catch (e: Exception) {
                console.error("Failed to parse systems: ${e.message}")
                emptyList()
            }
        } else {
            emptyList()
        }
    }

    override fun dispose() {
        try {
            isConnected = false
            webSocket?.close()
            wasmModule = null
        } catch (e: Exception) {
            console.error("Error during dispose: ${e.message}")
        }
    }

    private fun sendCommand(command: String, parameters: Map<String, Any> = emptyMap()): String? {
        val messageId = generateMessageId()
        val message = mapOf(
            "id" to messageId,
            "command" to command,
            "parameters" to parameters,
            "timestamp" to Date.now().toLong()
        )

        val messageJson = json.encodeToString(Map.serializer(String.serializer(), Any.serializer()), message)

        return if (wasmModule != null) {
            // Use WebAssembly
            try {
                wasmModule.sendCommand(messageJson).toString()
            } catch (e: Exception) {
                console.error("WASM command failed: ${e.message}")
                null
            }
        } else {
            // Use WebSocket
            try {
                webSocket?.send(messageJson)
                null // WebSocket is async, no immediate response
            } catch (e: Exception) {
                console.error("WebSocket send failed: ${e.message}")
                null
            }
        }
    }

    private fun sendCommandWithResponse(command: String, parameters: Map<String, Any> = emptyMap()): String? {
        return if (wasmModule != null) {
            // WebAssembly synchronous call
            try {
                val messageJson = json.encodeToString(Map.serializer(String.serializer(), Any.serializer()),
                    mapOf("command" to command, "parameters" to parameters))
                wasmModule.sendCommand(messageJson).toString()
            } catch (e: Exception) {
                console.error("WASM command failed: ${e.message}")
                null
            }
        } else {
            // WebSocket with promise-based response
            val messageId = generateMessageId()
            val promise = Promise<String> { resolve, reject ->
                messageCallbacks[messageId] = { response ->
                    resolve(response)
                }

                val message = mapOf(
                    "id" to messageId,
                    "command" to command,
                    "parameters" to parameters,
                    "timestamp" to Date.now().toLong()
                )

                val messageJson = json.encodeToString(Map.serializer(String.serializer(), Any.serializer()), message)

                try {
                    webSocket?.send(messageJson)
                } catch (e: Exception) {
                    reject(e)
                }
            }

            // For now, return null as this would need proper async handling
            // In a real implementation, this would return a promise
            return null
        }
    }

    private fun processMessage(messageJson: String) {
        try {
            val message = json.decodeFromString<Map<String, Any>>(messageJson)

            when (val type = message["type"] as? String) {
                "response" -> {
                    val messageId = message["id"] as? String
                    val response = message["response"] as? String

                    if (messageId != null && response != null) {
                        messageCallbacks[messageId]?.invoke(response)
                        messageCallbacks.remove(messageId)
                    }
                }
                "log" -> {
                    val level = message["level"] as? String ?: "info"
                    val text = message["text"] as? String ?: ""
                    console.log("Engine [$level]: $text")
                }
                "error" -> {
                    val error = message["error"] as? String ?: "Unknown error"
                    console.error("Engine error: $error")
                }
                else -> {
                    console.log("Unknown message type: $type")
                }
            }
        } catch (e: Exception) {
            console.error("Failed to process message: ${e.message}")
        }
    }

    private fun generateMessageId(): String {
        return "msg_${messageIdCounter++}_${Date.now().toLong()}"
    }
}

/**
 * Factory implementation for JavaScript
 */
actual object EngineIntegrationFactory {
    actual fun createIntegration(): EngineIntegration {
        return JsEngineIntegration()
    }
}

// Console polyfill for better logging
external fun console.log(message: Any?)
external fun console.error(message: Any?)
external fun console.warn(message: Any?)

// Date.now() polyfill
external val Date: dynamic
