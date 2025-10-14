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
actual class JsEngineIntegration : EngineIntegration {
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
        return try {
            val command = mapOf(
                "action" to "create_project",
                "project" to EngineUtils.serializeProject(projectInfo)
            )
            val response = sendCommand("create_project", command)
            response["success"] == "true"
        } catch (e: Exception) {
            console.error("Failed to create project: ${e.message}")
            false
        }
    }

    override fun loadProject(path: String): ProjectInfo? {
        return try {
            val command = mapOf(
                "action" to "load_project",
                "path" to path
            )
            val response = sendCommand("load_project", command)
            if (response["success"] == "true") {
                val projectJson = response["project"] ?: return null
                EngineUtils.deserializeProject(projectJson)
            } else {
                null
            }
        } catch (e: Exception) {
            console.error("Failed to load project: ${e.message}")
            null
        }
    }

    override fun saveProject(projectInfo: ProjectInfo): Boolean {
        return try {
            val command = mapOf(
                "action" to "save_project",
                "project" to EngineUtils.serializeProject(projectInfo)
            )
            val response = sendCommand("save_project", command)
            response["success"] == "true"
        } catch (e: Exception) {
            console.error("Failed to save project: ${e.message}")
            false
        }
    }

    override fun buildProject(target: String): BuildResult {
        return try {
            val command = mapOf(
                "action" to "build_project",
                "target" to target
            )
            val response = sendCommand("build_project", command)
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
            val response = sendCommand("run_project", command)
            response["success"] == "true"
        } catch (e: Exception) {
            console.error("Failed to run project: ${e.message}")
            false
        }
    }

    override fun stopProject(): Boolean {
        return try {
            val command = mapOf(
                "action" to "stop_project"
            )
            val response = sendCommand("stop_project", command)
            response["success"] == "true"
        } catch (e: Exception) {
            console.error("Failed to stop project: ${e.message}")
            false
        }
    }

    override fun getProjectInfo(): ProjectInfo? {
        return try {
            val command = mapOf(
                "action" to "get_project_info"
            )
            val response = sendCommand("get_project_info", command)
            if (response["success"] == "true") {
                val projectJson = response["project"] ?: return null
                EngineUtils.deserializeProject(projectJson)
            } else {
                null
            }
        } catch (e: Exception) {
            console.error("Failed to get project info: ${e.message}")
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
            val response = sendCommand("create_entity", command)
            if (response["success"] == "true") {
                response["entity_id"]
            } else {
                null
            }
        } catch (e: Exception) {
            console.error("Failed to create entity: ${e.message}")
            null
        }
    }

    override fun removeEntity(entityId: String): Boolean {
        return try {
            val command = mapOf(
                "action" to "remove_entity",
                "entity_id" to entityId
            )
            val response = sendCommand("remove_entity", command)
            response["success"] == "true"
        } catch (e: Exception) {
            console.error("Failed to remove entity: ${e.message}")
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
            val response = sendCommand("add_component", command)
            response["success"] == "true"
        } catch (e: Exception) {
            console.error("Failed to add component: ${e.message}")
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
            val response = sendCommand("remove_component", command)
            response["success"] == "true"
        } catch (e: Exception) {
            console.error("Failed to remove component: ${e.message}")
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
            val response = sendCommand("update_entity_transform", command)
            response["success"] == "true"
        } catch (e: Exception) {
            console.error("Failed to update entity transform: ${e.message}")
            false
        }
    }

    override fun getAvailableComponents(): List<ComponentInfo> {
        return try {
            val command = mapOf(
                "action" to "get_available_components"
            )
            val response = sendCommand("get_available_components", command)
            if (response["success"] == "true") {
                val componentsJson = response["components"] ?: return emptyList()
                json.decodeFromString(List.serializer(ComponentInfo.serializer()), componentsJson)
            } else {
                emptyList()
            }
        } catch (e: Exception) {
            console.error("Failed to get available components: ${e.message}")
            emptyList()
        }
    }

    override fun getAvailableSystems(): List<SystemInfo> {
        return try {
            val command = mapOf(
                "action" to "get_available_systems"
            )
            val response = sendCommand("get_available_systems", command)
            if (response["success"] == "true") {
                val systemsJson = response["systems"] ?: return emptyList()
                json.decodeFromString(List.serializer(SystemInfo.serializer()), systemsJson)
            } else {
                emptyList()
            }
        } catch (e: Exception) {
            console.error("Failed to get available systems: ${e.message}")
            emptyList()
        }
    }

    override fun dispose() {
        try {
            webSocket?.close()
            webSocket = null
            isConnected = false
        } catch (e: Exception) {
            console.error("Failed to dispose engine integration: ${e.message}")
        }
    }

    private fun sendCommand(action: String, parameters: Map<String, String>): Map<String, String> {
        if (!isConnected) {
            throw IllegalStateException("Not connected to engine")
        }

        val messageId = ++messageIdCounter
        val message = mapOf(
            "id" to messageId.toString(),
            "action" to action,
            "parameters" to json.encodeToString(Map.serializer(String.serializer(), String.serializer()), parameters)
        )

        val messageJson = json.encodeToString(Map.serializer(String.serializer(), String.serializer()), message)
        
        return if (wasmModule != null) {
            sendWasmMessage(messageJson)
        } else {
            sendWebSocketMessage(messageJson)
        }
    }

    private fun sendWebSocketMessage(message: String): Map<String, String> {
        val webSocket = this.webSocket ?: throw IllegalStateException("WebSocket not connected")
        
        webSocket.send(message)
        
        // For now, return a mock response
        // In a real implementation, this would wait for the response
        return mapOf(
            "success" to "true",
            "message" to "WebSocket message sent"
        )
    }

    private fun sendWasmMessage(message: String): Map<String, String> {
        // WASM implementation would go here
        // For now, return a mock response
        return mapOf(
            "success" to "true",
            "message" to "WASM message sent"
        )
    }

    private fun processMessage(message: String) {
        try {
            val messageData = json.decodeFromString(Map.serializer(String.serializer(), String.serializer()), message)
            val messageId = messageData["id"]
            val callback = messageCallbacks[messageId]
            callback?.invoke(message)
        } catch (e: Exception) {
            console.error("Failed to process message: ${e.message}")
        }
    }
}

/**
 * JavaScript Engine Integration Factory
 */
actual object EngineIntegrationFactory {
    actual fun createIntegration(): EngineIntegration {
        return JsEngineIntegration()
    }
}
