package com.foundry.ide.ui

import androidx.compose.foundation.*
import androidx.compose.foundation.layout.*
import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.runtime.*
import androidx.compose.ui.*
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.*
import androidx.compose.ui.window.WindowScope
import com.foundry.ide.*
import com.foundry.ide.editors.CodeEditor
import com.foundry.ide.editors.WorldEditor

/**
 * Main IDE Window with professional UI/UX
 * Features modern design, cross-platform compatibility, and comprehensive functionality
 */
@Composable
fun WindowScope.IdeMainWindow() {
    val ideApp = remember { ideApp }
    val currentProject by remember { mutableStateOf(ideApp.getCurrentProject()) }
    val statusMessage by remember { mutableStateOf(ideApp.getStatusMessage()) }
    val isProjectRunning by remember { mutableStateOf(ideApp.isProjectRunning()) }
    val buildProgress by remember { mutableStateOf(ideApp.getBuildProgress()) }

    var currentView by remember { mutableStateOf(IdeView.WELCOME) }
    var selectedFile by remember { mutableStateOf<String?>(null) }
    var openTabs by remember { mutableStateOf(listOf<String>()) }

    MaterialTheme(
        colors = darkThemeColors
    ) {
        Column(modifier = Modifier.fillMaxSize()) {
            // Top Menu Bar
            IdeMenuBar(
                onNewProject = { showNewProjectDialog() },
                onOpenProject = { showOpenProjectDialog() },
                onSaveProject = { ideApp.saveProject() },
                onBuild = { showBuildDialog() },
                onRun = { ideApp.runProject("desktop") },
                onStop = { ideApp.stopProject() },
                onOpenMarketplace = { ideApp.openPluginMarketplace() },
                onOpenCharacterCreator = { ideApp.openCharacterCreator() },
                onOpenTerminal = { ideApp.openEmbeddedTerminal() },
                onOpenMCP = { ideApp.openMCPIntegration() },
                onOpenSimulator = { ideApp.openGameSimulator() }
            )

            // Main Toolbar
            IdeToolbar(
                currentView = currentView,
                onViewChange = { currentView = it },
                isProjectRunning = isProjectRunning,
                onStopProject = { ideApp.stopProject() }
            )

            // Main Content Area
            Row(modifier = Modifier.weight(1f)) {
                // Left Sidebar - Project Explorer
                ProjectExplorer(
                    modifier = Modifier.width(280.dp),
                    currentProject = currentProject,
                    onFileSelect = { file ->
                        selectedFile = file
                        if (file !in openTabs) {
                            openTabs = openTabs + file
                        }
                        currentView = IdeView.EDITOR
                    }
                )

                // Center - Main Editor Area
                Box(modifier = Modifier.weight(1f)) {
                    when (currentView) {
                        IdeView.WELCOME -> WelcomeView(
                            onNewProject = { showNewProjectDialog() },
                            onOpenProject = { showOpenProjectDialog() }
                        )
                        IdeView.EDITOR -> EditorView(
                            openTabs = openTabs,
                            selectedFile = selectedFile,
                            onTabClose = { file ->
                                openTabs = openTabs.filter { it != file }
                                if (selectedFile == file) {
                                    selectedFile = openTabs.firstOrNull()
                                }
                            },
                            onTabSelect = { selectedFile = it }
                        )
                        IdeView.WORLD_EDITOR -> WorldEditorView()
                        IdeView.ASSET_BROWSER -> AssetBrowserView()
                        IdeView.CONSOLE -> ConsoleView()
                    }
                }

                // Right Sidebar - Properties/Inspector
                PropertiesPanel(
                    modifier = Modifier.width(320.dp),
                    selectedFile = selectedFile
                )
            }

            // Bottom Status Bar
            StatusBar(
                statusMessage = statusMessage,
                buildProgress = buildProgress,
                isProjectRunning = isProjectRunning
            )
        }
    }
}

enum class IdeView {
    WELCOME, EDITOR, WORLD_EDITOR, ASSET_BROWSER, CONSOLE
}

// Dark theme colors for professional IDE look
private val darkThemeColors = darkColors(
    primary = Color(0xFF007ACC),
    primaryVariant = Color(0xFF005A9E),
    secondary = Color(0xFF4CAF50),
    background = Color(0xFF1E1E1E),
    surface = Color(0xFF2D2D2D),
    onPrimary = Color.White,
    onSecondary = Color.White,
    onBackground = Color.White,
    onSurface = Color.White
)

// Dialog management functions
private fun showNewProjectDialog() {
    // Implementation for new project dialog
    println("New project dialog would open")
}

private fun showOpenProjectDialog() {
    // Implementation for open project dialog
    println("Open project dialog would open")
}

private fun showBuildDialog() {
    // Implementation for build configuration dialog
    println("Build dialog would open")
}