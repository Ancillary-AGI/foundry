/**
 * @file MCPClient.kt
 * @brief Model Context Protocol (MCP) client for AI tool integration
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 *
 * This file contains the MCP client implementation for integrating with
 * AI tools and services through the Model Context Protocol standard.
 */

package com.foundry.ide.mcp

import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.jsonObject
import kotlin.collections.mutableMapOf
import kotlin.collections.mutableListOf

/**
 * MCP Protocol version
 */
const val MCP_PROTOCOL_VERSION = "2024-11-05"

/**
 * MCP message types
 */
enum class MCPMessageType {
    REQUEST,
    RESPONSE,
    NOTIFICATION
}

/**
 * MCP request methods
 */
enum class MCPMethod {
    INITIALIZE,
    LIST_TOOLS,
    CALL_TOOL,
    LIST_RESOURCES,
    READ_RESOURCE,
    SUBSCRIBE,
    UNSUBSCRIBE,
    PING,
    PROGRESS,
    CANCELLED
}

/**
 * MCP message structure
 */
@Serializable
data class MCPMessage(
    val jsonrpc: String = "2.0",
    val id: String? = null,
    val method: String? = null,
    val params: JsonObject? = null,
    val result: JsonElement? = null,
    val error: MCPError? = null
)

/**
 * MCP error structure
 */
@Serializable
data class MCPError(
    val code: Int,
    val message: String,
    val data: JsonElement? = null
)

/**
 * MCP tool definition
 */
@Serializable
data class MCPTool(
    val name: String,
    val description: String,
    val inputSchema: JsonObject,
    val outputSchema: JsonObject? = null
)

/**
 * MCP resource definition
 */
@Serializable
data class MCPResource(
    val uri: String,
    val name: String,
    val description: String? = null,
    val mimeType: String? = null
)

/**
 * MCP server capabilities
 */
@Serializable
data class MCPServerCapabilities(
    val tools: MCPToolsCapability? = null,
    val resources: MCPResourcesCapability? = null,
    val prompts: MCPPromptsCapability? = null,
    val logging: MCPLoggingCapability? = null
)

@Serializable
data class MCPToolsCapability(
    val listChanged: Boolean? = null
)

@Serializable
data class MCPResourcesCapability(
    val subscribe: Boolean? = null,
    val listChanged: Boolean? = null
)

@Serializable
data class MCPPromptsCapability(
    val listChanged: Boolean? = null
)

@Serializable
data class MCPLoggingCapability(
    val level: String? = null
)

/**
 * MCP client capabilities
 */
@Serializable
data class MCPClientCapabilities(
    val experimental: Map<String, JsonElement>? = null,
    val sampling: Map<String, JsonElement>? = null
)

/**
 * MCP server configuration
 */
@Serializable
data class MCPServerConfig(
    val name: String,
    val command: String,
    val args: List<String> = emptyList(),
    val env: Map<String, String> = emptyMap(),
    val disabled: Boolean = false,
    val autoApprove: List<String> = emptyList()
)

/**
 * MCP client implementation
 */
class MCPClient {
    private val servers = mutableMapOf<String, MCPServer>()
    private val json = Json { 
        ignoreUnknownKeys = true
        encodeDefaults = true
    }
    private var isInitialized = false
    private val coroutineScope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    /**
     * Initialize the MCP client
     */
    suspend fun initialize(): Boolean {
        try {
            // Load server configurations
            val configs = loadServerConfigurations()
            
            // Initialize servers
            for (config in configs) {
                if (!config.disabled) {
                    val server = MCPServer(config, json)
                    if (server.initialize()) {
                        servers[config.name] = server
                        println("MCP server '${config.name}' initialized successfully")
                    } else {
                        println("Failed to initialize MCP server '${config.name}'")
                    }
                }
            }

            isInitialized = true
            println("MCP client initialized with ${servers.size} servers")
            return true
        } catch (e: Exception) {
            println("Failed to initialize MCP client: ${e.message}")
            return false
        }
    }

    /**
     * Shutdown the MCP client
     */
    suspend fun shutdown() {
        servers.values.forEach { server ->
            server.shutdown()
        }
        servers.clear()
        coroutineScope.cancel()
        isInitialized = false
        println("MCP client shut down")
    }

    /**
     * Get available tools from all servers
     */
    suspend fun getAvailableTools(): List<MCPTool> {
        if (!isInitialized) return emptyList()

        val allTools = mutableListOf<MCPTool>()
        
        for (server in servers.values) {
            try {
                val tools = server.listTools()
                allTools.addAll(tools)
            } catch (e: Exception) {
                println("Failed to get tools from server ${server.name}: ${e.message}")
            }
        }

        return allTools
    }

    /**
     * Execute a tool by name
     */
    suspend fun executeTool(toolName: String, parameters: Map<String, Any>): String {
        if (!isInitialized) {
            throw IllegalStateException("MCP client not initialized")
        }

        // Find the server that has this tool
        for (server in servers.values) {
            try {
                val tools = server.listTools()
                if (tools.any { it.name == toolName }) {
                    return server.callTool(toolName, parameters)
                }
            } catch (e: Exception) {
                println("Error checking tools on server ${server.name}: ${e.message}")
            }
        }

        throw IllegalArgumentException("Tool '$toolName' not found on any server")
    }

    /**
     * Get available resources from all servers
     */
    suspend fun getAvailableResources(): List<MCPResource> {
        if (!isInitialized) return emptyList()

        val allResources = mutableListOf<MCPResource>()
        
        for (server in servers.values) {
            try {
                val resources = server.listResources()
                allResources.addAll(resources)
            } catch (e: Exception) {
                println("Failed to get resources from server ${server.name}: ${e.message}")
            }
        }

        return allResources
    }

    /**
     * Read a resource by URI
     */
    suspend fun readResource(uri: String): String {
        if (!isInitialized) {
            throw IllegalStateException("MCP client not initialized")
        }

        // Find the server that has this resource
        for (server in servers.values) {
            try {
                val resources = server.listResources()
                if (resources.any { it.uri == uri }) {
                    return server.readResource(uri)
                }
            } catch (e: Exception) {
                println("Error checking resources on server ${server.name}: ${e.message}")
            }
        }

        throw IllegalArgumentException("Resource '$uri' not found on any server")
    }

    /**
     * Get server status
     */
    fun getServerStatus(): Map<String, Boolean> {
        return servers.mapValues { (_, server) -> server.isConnected() }
    }

    /**
     * Reconnect to a server
     */
    suspend fun reconnectServer(serverName: String): Boolean {
        val server = servers[serverName] ?: return false
        
        try {
            server.shutdown()
            return server.initialize()
        } catch (e: Exception) {
            println("Failed to reconnect to server '$serverName': ${e.message}")
            return false
        }
    }

    /**
     * Add a new server configuration
     */
    suspend fun addServer(config: MCPServerConfig): Boolean {
        if (servers.containsKey(config.name)) {
            println("Server '${config.name}' already exists")
            return false
        }

        try {
            val server = MCPServer(config, json)
            if (server.initialize()) {
                servers[config.name] = server
                println("MCP server '${config.name}' added successfully")
                return true
            } else {
                println("Failed to initialize new MCP server '${config.name}'")
                return false
            }
        } catch (e: Exception) {
            println("Failed to add MCP server '${config.name}': ${e.message}")
            return false
        }
    }

    /**
     * Remove a server
     */
    suspend fun removeServer(serverName: String): Boolean {
        val server = servers.remove(serverName)
        if (server != null) {
            server.shutdown()
            println("MCP server '$serverName' removed")
            return true
        }
        return false
    }

    private suspend fun loadServerConfigurations(): List<MCPServerConfig> {
        // This would load from configuration files
        // For now, return some default configurations
        
        return listOf(
            MCPServerConfig(
                name = "filesystem",
                command = "uvx",
                args = listOf("mcp-server-filesystem", "--"),
                autoApprove = listOf("read_file", "list_directory")
            ),
            MCPServerConfig(
                name = "git",
                command = "uvx",
                args = listOf("mcp-server-git", "--"),
                autoApprove = listOf("git_status", "git_log")
            ),
            MCPServerConfig(
                name = "web-search",
                command = "uvx",
                args = listOf("mcp-server-brave-search", "--"),
                env = mapOf("BRAVE_API_KEY" to System.getenv("BRAVE_API_KEY") ?: "")
            ),
            MCPServerConfig(
                name = "postgres",
                command = "uvx",
                args = listOf("mcp-server-postgres", "--"),
                env = mapOf("DATABASE_URL" to System.getenv("DATABASE_URL") ?: "")
            ),
            MCPServerConfig(
                name = "github",
                command = "uvx",
                args = listOf("mcp-server-github", "--"),
                env = mapOf("GITHUB_PERSONAL_ACCESS_TOKEN" to System.getenv("GITHUB_PERSONAL_ACCESS_TOKEN") ?: "")
            ),
            MCPServerConfig(
                name = "slack",
                command = "uvx",
                args = listOf("mcp-server-slack", "--"),
                env = mapOf("SLACK_BOT_TOKEN" to System.getenv("SLACK_BOT_TOKEN") ?: "")
            ),
            MCPServerConfig(
                name = "memory",
                command = "uvx",
                args = listOf("mcp-server-memory", "--")
            ),
            MCPServerConfig(
                name = "foundry-engine",
                command = "uvx",
                args = listOf("foundry-mcp-server", "--"),
                autoApprove = listOf(
                    "generate_code",
                    "analyze_bugs",
                    "generate_tests",
                    "optimize_performance",
                    "generate_documentation"
                )
            )
        )
    }
}

/**
 * MCP server connection
 */
class MCPServer(
    private val config: MCPServerConfig,
    private val json: Json
) {
    val name: String = config.name
    private var process: Process? = null
    private var connected = false
    private var nextRequestId = 1
    private val pendingRequests = mutableMapOf<String, CompletableDeferred<MCPMessage>>()
    private var serverCapabilities: MCPServerCapabilities? = null

    /**
     * Initialize connection to the server
     */
    suspend fun initialize(): Boolean {
        try {
            // Start the server process
            val processBuilder = ProcessBuilder(listOf(config.command) + config.args)
            
            // Set environment variables
            val env = processBuilder.environment()
            config.env.forEach { (key, value) ->
                env[key] = value
            }

            process = processBuilder.start()
            
            // Wait a moment for the process to start
            delay(1000)
            
            // Check if process is still running
            if (process?.isAlive != true) {
                println("MCP server process for '${config.name}' failed to start")
                return false
            }

            // Send initialize request
            val initializeResult = sendRequest(
                MCPMethod.INITIALIZE.name.lowercase(),
                mapOf(
                    "protocolVersion" to MCP_PROTOCOL_VERSION,
                    "capabilities" to MCPClientCapabilities(),
                    "clientInfo" to mapOf(
                        "name" to "FoundryEngine IDE",
                        "version" to "2.0.0"
                    )
                )
            )

            if (initializeResult.error != null) {
                println("Failed to initialize MCP server '${config.name}': ${initializeResult.error.message}")
                return false
            }

            // Parse server capabilities
            initializeResult.result?.let { result ->
                try {
                    val resultObj = result.jsonObject
                    val capabilitiesJson = resultObj["capabilities"]
                    if (capabilitiesJson != null) {
                        serverCapabilities = json.decodeFromJsonElement(MCPServerCapabilities.serializer(), capabilitiesJson)
                    }
                } catch (e: Exception) {
                    println("Failed to parse server capabilities: ${e.message}")
                }
            }

            connected = true
            println("MCP server '${config.name}' connected successfully")
            return true
        } catch (e: Exception) {
            println("Failed to initialize MCP server '${config.name}': ${e.message}")
            return false
        }
    }

    /**
     * Shutdown the server connection
     */
    fun shutdown() {
        connected = false
        
        // Cancel pending requests
        pendingRequests.values.forEach { deferred ->
            deferred.cancel()
        }
        pendingRequests.clear()

        // Terminate the process
        process?.let { proc ->
            if (proc.isAlive) {
                proc.destroyForcibly()
            }
        }
        process = null
        
        println("MCP server '${config.name}' shut down")
    }

    /**
     * Check if server is connected
     */
    fun isConnected(): Boolean {
        return connected && process?.isAlive == true
    }

    /**
     * List available tools
     */
    suspend fun listTools(): List<MCPTool> {
        if (!isConnected()) {
            throw IllegalStateException("Server not connected")
        }

        val response = sendRequest(MCPMethod.LIST_TOOLS.name.lowercase(), emptyMap())
        
        if (response.error != null) {
            throw Exception("Failed to list tools: ${response.error.message}")
        }

        return try {
            val result = response.result?.jsonObject
            val toolsArray = result?.get("tools")
            if (toolsArray != null) {
                json.decodeFromJsonElement<List<MCPTool>>(toolsArray)
            } else {
                emptyList()
            }
        } catch (e: Exception) {
            println("Failed to parse tools response: ${e.message}")
            emptyList()
        }
    }

    /**
     * Call a tool
     */
    suspend fun callTool(toolName: String, parameters: Map<String, Any>): String {
        if (!isConnected()) {
            throw IllegalStateException("Server not connected")
        }

        val response = sendRequest(
            MCPMethod.CALL_TOOL.name.lowercase(),
            mapOf(
                "name" to toolName,
                "arguments" to parameters
            )
        )

        if (response.error != null) {
            throw Exception("Tool call failed: ${response.error.message}")
        }

        return try {
            val result = response.result?.jsonObject
            val content = result?.get("content")
            content?.toString() ?: ""
        } catch (e: Exception) {
            println("Failed to parse tool response: ${e.message}")
            ""
        }
    }

    /**
     * List available resources
     */
    suspend fun listResources(): List<MCPResource> {
        if (!isConnected()) {
            throw IllegalStateException("Server not connected")
        }

        val response = sendRequest(MCPMethod.LIST_RESOURCES.name.lowercase(), emptyMap())
        
        if (response.error != null) {
            throw Exception("Failed to list resources: ${response.error.message}")
        }

        return try {
            val result = response.result?.jsonObject
            val resourcesArray = result?.get("resources")
            if (resourcesArray != null) {
                json.decodeFromJsonElement<List<MCPResource>>(resourcesArray)
            } else {
                emptyList()
            }
        } catch (e: Exception) {
            println("Failed to parse resources response: ${e.message}")
            emptyList()
        }
    }

    /**
     * Read a resource
     */
    suspend fun readResource(uri: String): String {
        if (!isConnected()) {
            throw IllegalStateException("Server not connected")
        }

        val response = sendRequest(
            MCPMethod.READ_RESOURCE.name.lowercase(),
            mapOf("uri" to uri)
        )

        if (response.error != null) {
            throw Exception("Failed to read resource: ${response.error.message}")
        }

        return try {
            val result = response.result?.jsonObject
            val contents = result?.get("contents")
            contents?.toString() ?: ""
        } catch (e: Exception) {
            println("Failed to parse resource response: ${e.message}")
            ""
        }
    }

    private suspend fun sendRequest(method: String, params: Map<String, Any>): MCPMessage {
        val requestId = (nextRequestId++).toString()
        
        val request = MCPMessage(
            id = requestId,
            method = method,
            params = json.encodeToJsonElement(params).jsonObject
        )

        val deferred = CompletableDeferred<MCPMessage>()
        pendingRequests[requestId] = deferred

        try {
            // Send request to process stdin
            val requestJson = json.encodeToString(MCPMessage.serializer(), request)
            process?.outputStream?.write((requestJson + "\n").toByteArray())
            process?.outputStream?.flush()

            // Wait for response with timeout
            return withTimeout(30000) { // 30 second timeout
                deferred.await()
            }
        } catch (e: TimeoutCancellationException) {
            pendingRequests.remove(requestId)
            throw Exception("Request timed out")
        } catch (e: Exception) {
            pendingRequests.remove(requestId)
            throw e
        }
    }

    // This would be implemented to read responses from the process
    // For now, we'll simulate responses
    private fun simulateResponse(requestId: String, result: JsonElement) {
        val deferred = pendingRequests.remove(requestId)
        if (deferred != null) {
            val response = MCPMessage(id = requestId, result = result)
            deferred.complete(response)
        }
    }
}