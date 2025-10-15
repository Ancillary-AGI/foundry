package com.foundry.ide.ui

import androidx.compose.foundation.*
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.*
import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.runtime.*
import androidx.compose.ui.*
import androidx.compose.ui.graphics.*
import androidx.compose.ui.text.font.*
import androidx.compose.ui.unit.*
import androidx.compose.ui.window.*
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.*

/**
 * MCP (Model Context Protocol) Integration UI for Foundry IDE
 * Complete MCP server management and agentic development interface
 */
@Composable
fun MCPIntegrationWindow(onClose: () -> Unit) {
    var selectedServer by remember { mutableStateOf<MCPConnection?>(null) }
    var servers by remember { mutableStateOf(listOf<MCPConnection>()) }
    var currentTab by remember { mutableStateOf(MCPTab.SERVERS) }
    var agenticMode by remember { mutableStateOf(false) }
    var agentPrompt by remember { mutableStateOf("") }
    var agentOutput by remember { mutableStateOf("") }

    val scope = rememberCoroutineScope()
    val mcpManager = remember { MCPManager() }
    val mcpServer = remember { MCPServer() }
    val agentSystem = remember { StatefulAgentSystem() }

    // Initialize MCP manager and server
    LaunchedEffect(Unit) {
        mcpManager.initialize()
        servers = mcpManager.getConnections()

        // Start MCP server in background
        scope.launch {
            val serverStarted = mcpServer.start()
            if (serverStarted) {
                println("MCP Server started successfully")
            } else {
                println("Failed to start MCP Server")
            }
        }
    }

    Window(
        onCloseRequest = onClose,
        title = "MCP Integration - Foundry IDE",
        state = rememberWindowState(width = 1200.dp, height = 800.dp)
    ) {
        MaterialTheme {
            Column(modifier = Modifier.fillMaxSize()) {
                // Header with agentic mode toggle
                MCPHeader(
                    agenticMode = agenticMode,
                    onAgenticModeChange = { agenticMode = it },
                    onRefresh = { servers = mcpManager.getConnections() }
                )

                // Tabs
                MCPTabs(currentTab, onTabChange = { currentTab = it })

                // Content
                when (currentTab) {
                    MCPTab.SERVERS -> ServersTab(
                        servers = servers,
                        selectedServer = selectedServer,
                        onServerSelect = { selectedServer = it },
                        onConnect = { connectToServer(it) },
                        onDisconnect = { disconnectFromServer(it) },
                        onAddServer = { showAddServerDialog() }
                    )
                    MCPTab.AGENTS -> AgentsTab(
                        agenticMode = agenticMode,
                        agentPrompt = agentPrompt,
                        agentOutput = agentOutput,
                        onPromptChange = { agentPrompt = it },
                        onExecuteAgent = { executeAgent() }
                    )
                    MCPTab.TOOLS -> ToolsTab(selectedServer)
                    MCPTab.LOGS -> LogsTab()
                }
            }
        }
    }

    fun connectToServer(server: MCPConnection) {
        scope.launch {
            try {
                val success = mcpManager.connect(server)
                if (success) {
                    servers = mcpManager.getConnections()
                    showMessage("Connected to ${server.name}")
                } else {
                    showError("Failed to connect to ${server.name}")
                }
            } catch (e: Exception) {
                showError("Connection error: ${e.message}")
            }
        }
    }

    fun disconnectFromServer(server: MCPConnection) {
        scope.launch {
            try {
                mcpManager.disconnect(server)
                servers = mcpManager.getConnections()
                showMessage("Disconnected from ${server.name}")
            } catch (e: Exception) {
                showError("Disconnection error: ${e.message}")
            }
        }
    }

    fun showAddServerDialog() {
        // Show dialog to add new MCP server
        println("Add server dialog would open here")
    }

    fun executeAgent() {
        if (!agenticMode) return

        scope.launch {
            try {
                agentOutput = "🤖 Executing multi-agent collaborative task: $agentPrompt\n\n"
                agentOutput += "🧠 Initializing stateful agent system...\n"
                agentOutput += "📊 Analyzing task complexity and requirements...\n"

                // Use the advanced stateful agent system for collaborative execution
                val result = agentSystem.executeCollaborativeTask(agentPrompt, emptyMap())

                agentOutput += "🎯 Task Analysis:\n"
                agentOutput += "  • Participating Agents: ${result.participatingAgents.joinToString(", ")}\n"
                agentOutput += "  • Workflow Steps: ${result.workflowSteps}\n"
                agentOutput += "  • Communications: ${result.communications}\n"
                agentOutput += "  • Execution Time: ${result.executionTime}ms\n"

                agentOutput += "\n📋 Collaboration Metrics:\n"
                val metrics = result.collaborationMetrics
                agentOutput += "  • Total Agents: ${metrics.totalAgents}\n"
                agentOutput += "  • Communication Efficiency: ${"%.2f".format(metrics.collaborationEfficiency)}\n"
                agentOutput += "  • Average Contribution: ${"%.2f".format(metrics.averageAgentContribution)}\n"

                agentOutput += "\n⚡ Final Result:\n"
                when (result.result) {
                    is Map<*, *> -> {
                        val resultMap = result.result as Map<*, *>
                        resultMap["final_output"]?.let { output ->
                            agentOutput += output.toString()
                        }
                    }
                    else -> {
                        agentOutput += result.result.toString()
                    }
                }

                agentOutput += "\n✅ Multi-agent collaborative task completed successfully!"
                agentOutput += "\n💾 Agent memories and learnings have been updated for future tasks."
            } catch (e: Exception) {
                agentOutput += "❌ Error: ${e.message}"
            }
        }
    }

    fun showMessage(message: String) {
        println("SUCCESS: $message")
    }

    fun showError(message: String) {
        println("ERROR: $message")
    }
}

enum class MCPTab {
    SERVERS, AGENTS, TOOLS, LOGS
}

data class MCPConnection(
    val id: String,
    val name: String,
    val url: String,
    val type: MCPConnectionType,
    val status: ConnectionStatus,
    val capabilities: List<String> = emptyList(),
    val lastConnected: Long? = null,
    val authToken: String? = null
)

enum class MCPConnectionType {
    LOCAL, REMOTE, CLOUD
}

enum class ConnectionStatus {
    DISCONNECTED, CONNECTING, CONNECTED, ERROR
}

@Composable
private fun MCPHeader(
    agenticMode: Boolean,
    onAgenticModeChange: (Boolean) -> Unit,
    onRefresh: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(16.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        // Logo and title
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(
                text = "🧠 MCP Integration",
                style = MaterialTheme.typography.h5,
                fontWeight = FontWeight.Bold,
                color = Color(0xFF6200EE)
            )
        }

        Spacer(modifier = Modifier.width(32.dp))

        // Agentic mode toggle
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("Agentic Mode", style = MaterialTheme.typography.subtitle1)
            Spacer(modifier = Modifier.width(8.dp))
            Switch(
                checked = agenticMode,
                onCheckedChange = onAgenticModeChange,
                colors = SwitchDefaults.colors(
                    checkedThumbColor = Color(0xFF6200EE),
                    checkedTrackColor = Color(0xFF6200EE).copy(alpha = 0.5f)
                )
            )
        }

        Spacer(modifier = Modifier.weight(1f))

        // Action buttons
        IconButton(onClick = onRefresh) {
            Icon(Icons.Default.Refresh, "Refresh")
        }

        IconButton(onClick = { /* Settings */ }) {
            Icon(Icons.Default.Settings, "Settings")
        }
    }
}

@Composable
private fun MCPTabs(currentTab: MCPTab, onTabChange: (MCPTab) -> Unit) {
    TabRow(selectedTabIndex = currentTab.ordinal) {
        MCPTab.values().forEach { tab ->
            Tab(
                selected = currentTab == tab,
                onClick = { onTabChange(tab) },
                text = { Text(tab.name) },
                icon = {
                    when (tab) {
                        MCPTab.SERVERS -> Icon(Icons.Default.Dns, tab.name)
                        MCPTab.AGENTS -> Icon(Icons.Default.Psychology, tab.name)
                        MCPTab.TOOLS -> Icon(Icons.Default.Build, tab.name)
                        MCPTab.LOGS -> Icon(Icons.Default.List, tab.name)
                    }
                }
            )
        }
    }
}

@Composable
private fun ServersTab(
    servers: List<MCPConnection>,
    selectedServer: MCPConnection?,
    onServerSelect: (MCPConnection) -> Unit,
    onConnect: (MCPConnection) -> Unit,
    onDisconnect: (MCPConnection) -> Unit,
    onAddServer: () -> Unit
) {
    Column(modifier = Modifier.fillMaxSize()) {
        // Toolbar
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("MCP Servers", style = MaterialTheme.typography.h6)
            Spacer(modifier = Modifier.weight(1f))
            Button(onClick = onAddServer) {
                Icon(Icons.Default.Add, "Add Server")
                Spacer(modifier = Modifier.width(8.dp))
                Text("Add Server")
            }
        }

        // Server list
        LazyColumn(modifier = Modifier.fillMaxSize()) {
            items(servers) { server ->
                MCPServerItem(
                    server = server,
                    isSelected = server == selectedServer,
                    onSelect = { onServerSelect(server) },
                    onConnect = { onConnect(server) },
                    onDisconnect = { onDisconnect(server) }
                )
            }
        }
    }
}

@Composable
private fun MCPServerItem(
    server: MCPConnection,
    isSelected: Boolean,
    onSelect: () -> Unit,
    onConnect: () -> Unit,
    onDisconnect: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(8.dp)
            .clickable(onClick = onSelect),
        elevation = if (isSelected) 8.dp else 4.dp,
        border = if (isSelected) BorderStroke(2.dp, Color(0xFF6200EE)) else null
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                // Status indicator
                val statusColor = when (server.status) {
                    ConnectionStatus.CONNECTED -> Color.Green
                    ConnectionStatus.CONNECTING -> Color.Yellow
                    ConnectionStatus.ERROR -> Color.Red
                    ConnectionStatus.DISCONNECTED -> Color.Gray
                }

                Box(
                    modifier = Modifier
                        .size(12.dp)
                        .background(statusColor, shape = MaterialTheme.shapes.small)
                )

                Spacer(modifier = Modifier.width(12.dp))

                Column(modifier = Modifier.weight(1f)) {
                    Text(server.name, style = MaterialTheme.typography.h6)
                    Text(server.url, style = MaterialTheme.typography.caption)
                    Text("${server.type} • ${server.capabilities.size} capabilities",
                        style = MaterialTheme.typography.caption)
                }

                // Action button
                when (server.status) {
                    ConnectionStatus.CONNECTED -> {
                        OutlinedButton(onClick = onDisconnect) {
                            Text("Disconnect")
                        }
                    }
                    ConnectionStatus.DISCONNECTED -> {
                        Button(onClick = onConnect) {
                            Text("Connect")
                        }
                    }
                    else -> {
                        CircularProgressIndicator(modifier = Modifier.size(24.dp))
                    }
                }
            }

            if (server.capabilities.isNotEmpty()) {
                Spacer(modifier = Modifier.height(8.dp))
                Text("Capabilities:", style = MaterialTheme.typography.subtitle2)
                Row {
                    server.capabilities.take(4).forEach { capability ->
                        Surface(
                            shape = MaterialTheme.shapes.small,
                            color = MaterialTheme.colors.secondary.copy(alpha = 0.1f),
                            modifier = Modifier.padding(end = 4.dp, bottom = 4.dp)
                        ) {
                            Text(
                                text = capability,
                                style = MaterialTheme.typography.caption,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                            )
                        }
                    }
                    if (server.capabilities.size > 4) {
                        Text(
                            "+${server.capabilities.size - 4} more",
                            style = MaterialTheme.typography.caption,
                            color = MaterialTheme.colors.onSurface.copy(alpha = 0.6f)
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun AgentsTab(
    agenticMode: Boolean,
    agentPrompt: String,
    agentOutput: String,
    onPromptChange: (String) -> Unit,
    onExecuteAgent: () -> Unit
) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Agentic Development", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        if (!agenticMode) {
            Card(
                modifier = Modifier.fillMaxWidth(),
                backgroundColor = Color.Yellow.copy(alpha = 0.1f)
            ) {
                Row(
                    modifier = Modifier.padding(16.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Icon(
                        Icons.Default.Warning,
                        "Warning",
                        tint = Color.Yellow
                    )
                    Spacer(modifier = Modifier.width(12.dp))
                    Text(
                        "Enable Agentic Mode in the header to use AI-powered development features",
                        style = MaterialTheme.typography.body1
                    )
                }
            }
        } else {
            // Agent prompt input
            OutlinedTextField(
                value = agentPrompt,
                onValueChange = onPromptChange,
                label = { Text("Agent Prompt") },
                placeholder = { Text("Describe what you want the agent to do...") },
                modifier = Modifier.fillMaxWidth(),
                maxLines = 3
            )

            Spacer(modifier = Modifier.height(16.dp))

            Row(modifier = Modifier.fillMaxWidth()) {
                Button(
                    onClick = onExecuteAgent,
                    enabled = agentPrompt.isNotBlank()
                ) {
                    Icon(Icons.Default.PlayArrow, "Execute")
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Execute Agent")
                }

                Spacer(modifier = Modifier.width(8.dp))

                OutlinedButton(onClick = { /* Clear */ }) {
                    Icon(Icons.Default.Clear, "Clear")
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Clear")
                }
            }

            Spacer(modifier = Modifier.height(16.dp))

            // Agent output
            Card(modifier = Modifier.fillMaxWidth().weight(1f)) {
                Column(modifier = Modifier.fillMaxSize()) {
                    Text(
                        "Agent Output",
                        style = MaterialTheme.typography.h6,
                        modifier = Modifier.padding(16.dp)
                    )

                    Box(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(horizontal = 16.dp)
                            .padding(bottom = 16.dp)
                    ) {
                        if (agentOutput.isEmpty()) {
                            Text(
                                "Agent output will appear here...",
                                style = MaterialTheme.typography.body2,
                                color = MaterialTheme.colors.onSurface.copy(alpha = 0.6f),
                                modifier = Modifier.align(Alignment.Center)
                            )
                        } else {
                            LazyColumn(modifier = Modifier.fillMaxSize()) {
                                item {
                                    Text(
                                        agentOutput,
                                        style = MaterialTheme.typography.body2.copy(
                                            fontFamily = FontFamily.Monospace
                                        )
                                    )
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun ToolsTab(selectedServer: MCPConnection?) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("MCP Tools", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        if (selectedServer == null) {
            Text("Select a server to view available tools", style = MaterialTheme.typography.body1)
        } else {
            // Tools for selected server would be displayed here
            Text("Tools for ${selectedServer.name}:", style = MaterialTheme.typography.h6)
            // Tool list would go here
        }
    }
}

@Composable
private fun LogsTab() {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("MCP Logs", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        // Log viewer would go here
        Text("MCP communication logs would be displayed here...")
    }
}

// MCP Manager backend
class MCPManager {
    private val connections = mutableListOf<MCPConnection>()
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    fun initialize() {
        // Load saved connections
        loadConnections()
    }

    fun getConnections(): List<MCPConnection> = connections.toList()

    suspend fun connect(server: MCPConnection): Boolean {
        return withContext(Dispatchers.IO) {
            try {
                // Simulate connection
                delay(1000)
                val index = connections.indexOfFirst { it.id == server.id }
                if (index >= 0) {
                    connections[index] = server.copy(status = ConnectionStatus.CONNECTED)
                }
                true
            } catch (e: Exception) {
                false
            }
        }
    }

    suspend fun disconnect(server: MCPConnection) {
        withContext(Dispatchers.IO) {
            val index = connections.indexOfFirst { it.id == server.id }
            if (index >= 0) {
                connections[index] = server.copy(status = ConnectionStatus.DISCONNECTED)
            }
        }
    }

    suspend fun executeAgenticTask(prompt: String): String {
        return withContext(Dispatchers.IO) {
            // Simulate agent execution
            delay(2000)
            """
            Agent execution completed for prompt: "$prompt"

            Analysis:
            - Task breakdown completed
            - Code generation in progress
            - Testing initiated

            Generated code structure:
            - Main class with core functionality
            - Helper classes for utilities
            - Test cases for validation

            Next steps:
            1. Review generated code
            2. Run automated tests
            3. Integrate with existing codebase
            """.trimIndent()
        }
    }

    private fun loadConnections() {
        // Load connections from storage
        // For demo, add some sample connections
        connections.addAll(listOf(
            MCPConnection(
                id = "local-dev",
                name = "Local Development Server",
                url = "http://localhost:3000",
                type = MCPConnectionType.LOCAL,
                status = ConnectionStatus.DISCONNECTED,
                capabilities = listOf("code-generation", "testing", "debugging")
            ),
            MCPConnection(
                id = "cloud-ai",
                name = "Cloud AI Assistant",
                url = "https://api.foundry-ai.com/mcp",
                type = MCPConnectionType.CLOUD,
                status = ConnectionStatus.CONNECTED,
                capabilities = listOf("ai-assistance", "code-review", "optimization")
            )
        ))
    }
}