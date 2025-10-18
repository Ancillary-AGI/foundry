package com.foundry.ide

import androidx.compose.desktop.ui.tooling.preview.Preview
import androidx.compose.foundation.*
import androidx.compose.foundation.layout.*
import androidx.compose.material.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.awt.ComposeWindow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.*
import kotlinx.coroutines.launch
import java.awt.FileDialog
import java.io.File
import javax.swing.JFileChooser
import javax.swing.JFrame

/**
 * Main entry point for the Foundry IDE Desktop Application
 */
fun main() {
    // Initialize the IDE application
    initializeIde()

    application {
        val windowState = rememberWindowState(
            width = 1400.dp,
            height = 900.dp,
            position = WindowPosition(Alignment.Center)
        )

        val ideState by remember { mutableStateOf(ideApp.getCurrentState()) }
        val scope = rememberCoroutineScope()

        // Listen for state changes
        DisposableEffect(Unit) {
            val listener: (IdeState) -> Unit = { newState ->
                // Update UI state on main thread
                scope.launch {
                    // Trigger recomposition
                }
            }
            ideApp.addStateListener(listener)

            onDispose {
                ideApp.shutdown()
            }
        }

        Window(
            onCloseRequest = ::exitApplication,
            state = windowState,
            title = "Foundry IDE - ${ideState.currentProject?.name ?: "No Project"}"
        ) {
            ComposeWindow()

            MaterialTheme(
                colors = darkColors(
                    background = Color(0xFF1E1E1E),
                    surface = Color(0xFF2D2D2D),
                    primary = Color(0xFF4A90E2),
                    onBackground = Color.White,
                    onSurface = Color.White
                )
            ) {
                Scaffold(
                    topBar = { IdeTopBar() },
                    bottomBar = { IdeStatusBar(ideState) }
                ) { padding ->
                    Box(modifier = Modifier.padding(padding)) {
                        when (ideState.activeView) {
                            IdeView.WELCOME -> WelcomeView()
                            IdeView.PROJECT_BROWSER -> ProjectBrowserView()
                            IdeView.WORLD_EDITOR -> WorldEditorView()
                            IdeView.CODE_EDITOR -> CodeEditorView()
                            IdeView.ASSET_BROWSER -> AssetBrowserView()
                            IdeView.COMPONENT_INSPECTOR -> ComponentInspectorView()
                            IdeView.SYSTEM_VIEWER -> SystemViewerView()
                            IdeView.BUILD_SETTINGS -> BuildSettingsView()
                            IdeView.DEBUG_VIEW -> DebugView()
                            IdeView.PROFILING_VIEW -> ProfilingView()
                        }
                    }
                }
            }
        }
    }
}

/**
 * Top toolbar for the IDE
 */
@Composable
fun IdeTopBar() {
    TopAppBar(
        title = { Text("Foundry IDE", fontWeight = FontWeight.Bold) },
        backgroundColor = MaterialTheme.colors.surface,
        actions = {
            // File menu
            DropdownMenuButton(
                text = "File",
                items = listOf(
                    "New Project" to { showNewProjectDialog() },
                    "Open Project" to { showOpenProjectDialog() },
                    "Save Project" to { ideApp.saveProject() },
                    "Import Asset" to { showImportAssetDialog() },
                    "Export Project" to { showExportProjectDialog() }
                )
            )

            // Edit menu
            DropdownMenuButton(
                text = "Edit",
                items = listOf(
                    "Undo" to { /* TODO */ },
                    "Redo" to { /* TODO */ },
                    "Preferences" to { showPreferencesDialog() }
                )
            )

            // View menu
            DropdownMenuButton(
                text = "View",
                items = listOf(
                    "World Editor" to { ideApp.switchView(IdeView.WORLD_EDITOR) },
                    "Code Editor" to { ideApp.switchView(IdeView.CODE_EDITOR) },
                    "Asset Browser" to { ideApp.switchView(IdeView.ASSET_BROWSER) },
                    "Component Inspector" to { ideApp.switchView(IdeView.COMPONENT_INSPECTOR) },
                    "Debug View" to { ideApp.switchView(IdeView.DEBUG_VIEW) }
                )
            )

            // Build menu
            DropdownMenuButton(
                text = "Build",
                items = listOf(
                    "Build Desktop" to { ideApp.buildProject("desktop") },
                    "Build Web" to { ideApp.buildProject("web") },
                    "Build Mobile" to { ideApp.buildProject("mobile") },
                    "Run Project" to { ideApp.runProject("desktop") },
                    "Stop Project" to { ideApp.stopProject() }
                )
            )

            // Tools menu
            DropdownMenuButton(
                text = "Tools",
                items = listOf(
                    "Extension Manager" to { showExtensionManagerDialog() },
                    "Profiling Tools" to { ideApp.switchView(IdeView.PROFILING_VIEW) },
                    "System Monitor" to { showSystemMonitorDialog() }
                )
            )

            // Help menu
            DropdownMenuButton(
                text = "Help",
                items = listOf(
                    "Documentation" to { openDocumentation() },
                    "About" to { showAboutDialog() }
                )
            )
        }
    )
}

/**
 * Status bar showing current project and build status
 */
@Composable
fun IdeStatusBar(state: IdeState) {
    BottomAppBar(
        backgroundColor = MaterialTheme.colors.background
    ) {
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Left side - project info
            Row(verticalAlignment = Alignment.CenterVertically) {
                state.currentProject?.let { project ->
                    Text(
                        text = "Project: ${project.name}",
                        color = MaterialTheme.colors.onBackground,
                        fontSize = 12.sp
                    )
                    Spacer(modifier = Modifier.width(16.dp))
                    Text(
                        text = "Entities: ${project.entities.size}",
                        color = MaterialTheme.colors.onBackground,
                        fontSize = 12.sp
                    )
                } ?: Text(
                    text = "No project loaded",
                    color = MaterialTheme.colors.onBackground.copy(alpha = 0.7f),
                    fontSize = 12.sp
                )
            }

            // Right side - build status and engine info
            Row(verticalAlignment = Alignment.CenterVertically) {
                when (state.buildStatus) {
                    BuildStatus.IDLE -> Icon(
                        painter = painterResource("icons/idle.png"),
                        contentDescription = "Idle",
                        tint = Color.Green,
                        modifier = Modifier.size(16.dp)
                    )
                    BuildStatus.BUILDING -> Icon(
                        painter = painterResource("icons/building.png"),
                        contentDescription = "Building",
                        tint = Color.Yellow,
                        modifier = Modifier.size(16.dp)
                    )
                    BuildStatus.SUCCESS -> Icon(
                        painter = painterResource("icons/success.png"),
                        contentDescription = "Success",
                        tint = Color.Green,
                        modifier = Modifier.size(16.dp)
                    )
                    BuildStatus.FAILED -> Icon(
                        painter = painterResource("icons/error.png"),
                        contentDescription = "Failed",
                        tint = Color.Red,
                        modifier = Modifier.size(16.dp)
                    )
                    BuildStatus.CANCELLING -> Icon(
                        painter = painterResource("icons/cancelling.png"),
                        contentDescription = "Cancelling",
                        tint = Color.Orange,
                        modifier = Modifier.size(16.dp)
                    )
                }

                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    text = state.buildStatus.name,
                    color = MaterialTheme.colors.onBackground.copy(alpha = 0.7f),
                    fontSize = 12.sp
                )
            }
        }
    }
}

/**
 * Welcome screen shown when no project is loaded
 */
@Composable
fun WelcomeView() {
    Column(
        modifier = Modifier.fillMaxSize().padding(32.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Text(
            text = "Welcome to Foundry IDE",
            fontSize = 32.sp,
            fontWeight = FontWeight.Bold,
            color = MaterialTheme.colors.onBackground
        )

        Spacer(modifier = Modifier.height(16.dp))

        Text(
            text = "Create powerful games and applications with the Foundry Game Engine",
            fontSize = 16.sp,
            color = MaterialTheme.colors.onBackground.copy(alpha = 0.7f)
        )

        Spacer(modifier = Modifier.height(48.dp))

        // Quick action buttons
        Row(
            horizontalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Button(
                onClick = { showNewProjectDialog() },
                modifier = Modifier.width(200.dp).height(60.dp)
            ) {
                Text("New Project", fontSize = 16.sp)
            }

            Button(
                onClick = { showOpenProjectDialog() },
                modifier = Modifier.width(200.dp).height(60.dp)
            ) {
                Text("Open Project", fontSize = 16.sp)
            }
        }

        Spacer(modifier = Modifier.height(32.dp))

        // Recent projects
        val recentProjects = ideApp.getCurrentState().recentProjects
        if (recentProjects.isNotEmpty()) {
            Text(
                text = "Recent Projects",
                fontSize = 14.sp,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colors.onBackground
            )

            Spacer(modifier = Modifier.height(8.dp))

            recentProjects.take(5).forEach { projectPath ->
                Button(
                    onClick = { ideApp.loadProject(projectPath) },
                    modifier = Modifier.fillMaxWidth(0.5f)
                ) {
                    Text(File(projectPath).name, fontSize = 12.sp)
                }
            }
        }
    }
}

/**
 * Generic dropdown menu button component
 */
@Composable
fun DropdownMenuButton(text: String, items: List<Pair<String, () -> Unit>>) {
    var expanded by remember { mutableStateOf(false) }

    Box {
        Button(onClick = { expanded = true }) {
            Text(text)
        }

        DropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false }
        ) {
            items.forEach { (label, action) ->
                DropdownMenuItem(onClick = {
                    action()
                    expanded = false
                }) {
                    Text(label)
                }
            }
        }
    }
}

// Placeholder composables for different views
@Composable fun ProjectBrowserView() { Text("Project Browser - Coming Soon") }
@Composable fun WorldEditorView() { Text("World Editor - Coming Soon") }
@Composable fun CodeEditorView() { Text("Code Editor - Coming Soon") }
@Composable fun AssetBrowserView() { Text("Asset Browser - Coming Soon") }
@Composable fun ComponentInspectorView() { Text("Component Inspector - Coming Soon") }
@Composable fun SystemViewerView() { 
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "System Viewer",
            fontSize = 24.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        // System Information
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = 4.dp
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "System Information",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                
                Text("OS: ${System.getProperty("os.name")}")
                Text("Architecture: ${System.getProperty("os.arch")}")
                Text("Java Version: ${System.getProperty("java.version")}")
                Text("Available Processors: ${Runtime.getRuntime().availableProcessors()}")
                Text("Total Memory: ${Runtime.getRuntime().totalMemory() / 1024 / 1024} MB")
                Text("Free Memory: ${Runtime.getRuntime().freeMemory() / 1024 / 1024} MB")
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Engine Systems
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = 4.dp
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Engine Systems",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                
                val systems = listOf(
                    "Rendering System" to "Active",
                    "Physics System" to "Active", 
                    "Audio System" to "Active",
                    "Input System" to "Active",
                    "Network System" to "Standby",
                    "Scripting System" to "Active"
                )
                
                systems.forEach { (system, status) ->
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text(system)
                        Text(
                            status,
                            color = if (status == "Active") Color.Green else Color.Orange
                        )
                    }
                }
            }
        }
    }
}

@Composable fun BuildSettingsView() { 
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "Build Settings",
            fontSize = 24.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        // Build Configuration
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = 4.dp
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Build Configuration",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                var buildType by remember { mutableStateOf("Release") }
                var optimization by remember { mutableStateOf("O3") }
                var debugInfo by remember { mutableStateOf(false) }
                var profiling by remember { mutableStateOf(false) }
                
                // Build Type
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text("Build Type:")
                    DropdownMenu(
                        expanded = false,
                        onDismissRequest = { }
                    ) {
                        DropdownMenuItem(onClick = { buildType = "Debug" }) {
                            Text("Debug")
                        }
                        DropdownMenuItem(onClick = { buildType = "Release" }) {
                            Text("Release")
                        }
                    }
                    Text(buildType)
                }
                
                Spacer(modifier = Modifier.height(8.dp))
                
                // Optimization Level
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text("Optimization:")
                    DropdownMenu(
                        expanded = false,
                        onDismissRequest = { }
                    ) {
                        DropdownMenuItem(onClick = { optimization = "O0" }) {
                            Text("O0 (None)")
                        }
                        DropdownMenuItem(onClick = { optimization = "O1" }) {
                            Text("O1 (Basic)")
                        }
                        DropdownMenuItem(onClick = { optimization = "O2" }) {
                            Text("O2 (Standard)")
                        }
                        DropdownMenuItem(onClick = { optimization = "O3" }) {
                            Text("O3 (Aggressive)")
                        }
                    }
                    Text(optimization)
                }
                
                Spacer(modifier = Modifier.height(8.dp))
                
                // Debug Info
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text("Debug Information:")
                    Checkbox(
                        checked = debugInfo,
                        onCheckedChange = { debugInfo = it }
                    )
                }
                
                Spacer(modifier = Modifier.height(8.dp))
                
                // Profiling
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text("Enable Profiling:")
                    Checkbox(
                        checked = profiling,
                        onCheckedChange = { profiling = it }
                    )
                }
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Platform Targets
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = 4.dp
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Target Platforms",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                val platforms = listOf("Windows", "macOS", "Linux", "Android", "iOS", "Web")
                platforms.forEach { platform ->
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(platform)
                        Checkbox(
                            checked = true,
                            onCheckedChange = { }
                        )
                    }
                }
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Build Actions
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = { /* Build project */ },
                modifier = Modifier.weight(1f)
            ) {
                Text("Build Project")
            }
            
            Button(
                onClick = { /* Clean build */ },
                modifier = Modifier.weight(1f)
            ) {
                Text("Clean Build")
            }
        }
    }
}

@Composable fun DebugView() { 
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "Debug View",
            fontSize = 24.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        // Debug Controls
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = 4.dp
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Debug Controls",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Button(
                        onClick = { /* Start debugging */ },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("Start Debug")
                    }
                    
                    Button(
                        onClick = { /* Pause debugging */ },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("Pause")
                    }
                    
                    Button(
                        onClick = { /* Stop debugging */ },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("Stop")
                    }
                }
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Breakpoints
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = 4.dp
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Breakpoints",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                val breakpoints = listOf(
                    "main.ts:25" to true,
                    "game.ts:42" to false,
                    "physics.ts:18" to true
                )
                
                breakpoints.forEach { (location, enabled) ->
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(location)
                        Checkbox(
                            checked = enabled,
                            onCheckedChange = { }
                        )
                    }
                }
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Call Stack
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = 4.dp
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Call Stack",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                val callStack = listOf(
                    "main() at main.ts:30",
                    "initializeGame() at game.ts:15",
                    "createPlayer() at player.ts:8"
                )
                
                callStack.forEach { frame ->
                    Text(
                        frame,
                        modifier = Modifier.padding(vertical = 2.dp)
                    )
                }
            }
        }
    }
}

@Composable fun ProfilingView() { 
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "Profiling View",
            fontSize = 24.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        // Performance Metrics
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = 4.dp
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Performance Metrics",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                val metrics = listOf(
                    "FPS" to "60.0",
                    "Frame Time" to "16.67 ms",
                    "CPU Usage" to "45.2%",
                    "Memory Usage" to "128.5 MB",
                    "GPU Usage" to "67.8%"
                )
                
                metrics.forEach { (metric, value) ->
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text(metric)
                        Text(
                            value,
                            fontWeight = FontWeight.Medium
                        )
                    }
                }
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Function Profiling
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = 4.dp
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Function Profiling",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                val functions = listOf(
                    "updatePhysics()" to "2.3 ms",
                    "renderFrame()" to "8.1 ms",
                    "updateAudio()" to "0.5 ms",
                    "processInput()" to "0.2 ms"
                )
                
                functions.forEach { (function, time) ->
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text(function)
                        Text(
                            time,
                            fontWeight = FontWeight.Medium
                        )
                    }
                }
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Profiling Controls
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = { /* Start profiling */ },
                modifier = Modifier.weight(1f)
            ) {
                Text("Start Profiling")
            }
            
            Button(
                onClick = { /* Stop profiling */ },
                modifier = Modifier.weight(1f)
            ) {
                Text("Stop Profiling")
            }
            
            Button(
                onClick = { /* Export profile */ },
                modifier = Modifier.weight(1f)
            ) {
                Text("Export Profile")
            }
        }
    }
}

// Dialog functions
fun showNewProjectDialog() { 
    // Implementation would show the project creation wizard
    println("Opening New Project Dialog...")
}

fun showOpenProjectDialog() { 
    // Implementation would show file browser for project selection
    println("Opening Project Browser...")
}

fun showImportAssetDialog() { 
    // Implementation would show asset import dialog
    println("Opening Asset Import Dialog...")
}

fun showExportProjectDialog() { 
    // Implementation would show project export options
    println("Opening Project Export Dialog...")
}

fun showPreferencesDialog() { 
    // Implementation would show IDE preferences
    println("Opening Preferences Dialog...")
}

fun showExtensionManagerDialog() { 
    // Implementation would show extension manager
    println("Opening Extension Manager...")
}

fun showSystemMonitorDialog() { 
    // Implementation would show system monitoring tools
    println("Opening System Monitor...")
}

fun openDocumentation() { 
    // Implementation would open documentation in browser
    println("Opening Documentation...")
}

fun showAboutDialog() { 
    // Implementation would show about dialog
    println("FoundryEngine IDE v1.0.0")
    println("Cross-platform game development environment")
    println("Built with Kotlin Multiplatform")
}
