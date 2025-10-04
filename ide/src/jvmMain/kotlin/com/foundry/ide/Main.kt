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
@Composable fun SystemViewerView() { Text("System Viewer - Coming Soon") }
@Composable fun BuildSettingsView() { Text("Build Settings - Coming Soon") }
@Composable fun DebugView() { Text("Debug View - Coming Soon") }
@Composable fun ProfilingView() { Text("Profiling View - Coming Soon") }

// Dialog functions (placeholders)
fun showNewProjectDialog() { /* TODO */ }
fun showOpenProjectDialog() { /* TODO */ }
fun showImportAssetDialog() { /* TODO */ }
fun showExportProjectDialog() { /* TODO */ }
fun showPreferencesDialog() { /* TODO */ }
fun showExtensionManagerDialog() { /* TODO */ }
fun showSystemMonitorDialog() { /* TODO */ }
fun openDocumentation() { /* TODO */ }
fun showAboutDialog() { /* TODO */ }
