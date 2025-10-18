package com.foundry.ide.ui.components

import androidx.compose.foundation.*
import androidx.compose.foundation.layout.*
import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.runtime.*
import androidx.compose.ui.*
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.*

/**
 * Professional IDE Menu Bar with all essential functions
 */
@Composable
fun IdeMenuBar(
    onNewProject: () -> Unit,
    onOpenProject: () -> Unit,
    onSaveProject: () -> Unit,
    onBuild: () -> Unit,
    onRun: () -> Unit,
    onStop: () -> Unit,
    onOpenMarketplace: () -> Unit,
    onOpenCharacterCreator: () -> Unit,
    onOpenTerminal: () -> Unit,
    onOpenMCP: () -> Unit,
    onOpenSimulator: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Color(0xFF2D2D2D))
            .padding(horizontal = 8.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        // File Menu
        DropdownMenuButton(
            text = "File",
            items = listOf(
                MenuItem("New Project", Icons.Default.Add, onNewProject),
                MenuItem("Open Project", Icons.Default.FolderOpen, onOpenProject),
                MenuItem("Save Project", Icons.Default.Save, onSaveProject),
                null, // Separator
                MenuItem("Import Asset", Icons.Default.FileUpload, {}),
                MenuItem("Export Project", Icons.Default.FileDownload, {}),
                null, // Separator
                MenuItem("Exit", Icons.Default.ExitToApp, {})
            )
        )

        Spacer(modifier = Modifier.width(16.dp))

        // Edit Menu
        DropdownMenuButton(
            text = "Edit",
            items = listOf(
                MenuItem("Undo", Icons.Default.Undo, {}),
                MenuItem("Redo", Icons.Default.Redo, {}),
                null, // Separator
                MenuItem("Cut", Icons.Default.ContentCut, {}),
                MenuItem("Copy", Icons.Default.ContentCopy, {}),
                MenuItem("Paste", Icons.Default.ContentPaste, {}),
                null, // Separator
                MenuItem("Find", Icons.Default.Search, {}),
                MenuItem("Replace", Icons.Default.FindReplace, {})
            )
        )

        Spacer(modifier = Modifier.width(16.dp))

        // View Menu
        DropdownMenuButton(
            text = "View",
            items = listOf(
                MenuItem("World Editor", Icons.Default.Public, {}),
                MenuItem("Code Editor", Icons.Default.Code, {}),
                MenuItem("Asset Browser", Icons.Default.Image, {}),
                MenuItem("Console", Icons.Default.Terminal, {}),
                null, // Separator
                MenuItem("Properties", Icons.Default.Settings, {}),
                MenuItem("Toolbox", Icons.Default.Build, {})
            )
        )

        Spacer(modifier = Modifier.width(16.dp))

        // Build Menu
        DropdownMenuButton(
            text = "Build",
            items = listOf(
                MenuItem("Build Web", Icons.Default.Web, { onBuild() }),
                MenuItem("Build Desktop", Icons.Default.Computer, { onBuild() }),
                MenuItem("Build Mobile", Icons.Default.PhoneAndroid, { onBuild() }),
                null, // Separator
                MenuItem("Clean", Icons.Default.Clear, {}),
                MenuItem("Rebuild All", Icons.Default.Refresh, {})
            )
        )

        Spacer(modifier = Modifier.width(16.dp))

        // Tools Menu
        DropdownMenuButton(
            text = "Tools",
            items = listOf(
                MenuItem("Plugin Marketplace", Icons.Default.ShoppingCart, onOpenMarketplace),
                MenuItem("Character Creator", Icons.Default.Person, onOpenCharacterCreator),
                MenuItem("Terminal", Icons.Default.Terminal, onOpenTerminal),
                MenuItem("MCP Integration", Icons.Default.Hub, onOpenMCP),
                MenuItem("Game Simulator", Icons.Default.Games, onOpenSimulator),
                null, // Separator
                MenuItem("Profiler", Icons.Default.Timeline, {}),
                MenuItem("Benchmarking", Icons.Default.Speed, {}),
                null, // Separator
                MenuItem("Settings", Icons.Default.Settings, {})
            )
        )

        Spacer(modifier = Modifier.width(16.dp))

        // Run Menu
        DropdownMenuButton(
            text = "Run",
            items = listOf(
                MenuItem("Run Project", Icons.Default.PlayArrow, onRun, enabled = true),
                MenuItem("Debug Project", Icons.Default.BugReport, {}, enabled = false),
                MenuItem("Stop", Icons.Default.Stop, onStop, enabled = true),
                null, // Separator
                MenuItem("Run Tests", Icons.Default.Science, {}, enabled = false)
            )
        )

        // Spacer to push help menu to the right
        Spacer(modifier = Modifier.weight(1f))

        // Help Menu
        DropdownMenuButton(
            text = "Help",
            items = listOf(
                MenuItem("Documentation", Icons.Default.MenuBook, {}),
                MenuItem("Tutorials", Icons.Default.School, {}),
                MenuItem("API Reference", Icons.Default.Code, {}),
                null, // Separator
                MenuItem("About Foundry IDE", Icons.Default.Info, {})
            )
        )
    }
}

/**
 * Dropdown menu button component
 */
@Composable
private fun DropdownMenuButton(
    text: String,
    items: List<MenuItem?>
) {
    var expanded by remember { mutableStateOf(false) }

    Box {
        TextButton(
            onClick = { expanded = true },
            colors = ButtonDefaults.textButtonColors(
                contentColor = Color.White
            )
        ) {
            Text(text, style = MaterialTheme.typography.button)
            Spacer(modifier = Modifier.width(4.dp))
            Icon(
                Icons.Default.ArrowDropDown,
                "Dropdown",
                modifier = Modifier.size(16.dp)
            )
        }

        DropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false }
        ) {
            items.forEach { item ->
                if (item == null) {
                    Divider()
                } else {
                    DropdownMenuItem(
                        onClick = {
                            if (item.enabled) {
                                item.onClick()
                                expanded = false
                            }
                        },
                        enabled = item.enabled
                    ) {
                        if (item.icon != null) {
                            Icon(
                                item.icon,
                                item.text,
                                modifier = Modifier.size(18.dp)
                            )
                            Spacer(modifier = Modifier.width(8.dp))
                        }
                        Text(item.text)
                    }
                }
            }
        }
    }
}

/**
 * Menu item data class
 */
data class MenuItem(
    val text: String,
    val icon: androidx.compose.ui.graphics.vector.ImageVector? = null,
    val onClick: () -> Unit,
    val enabled: Boolean = true
)