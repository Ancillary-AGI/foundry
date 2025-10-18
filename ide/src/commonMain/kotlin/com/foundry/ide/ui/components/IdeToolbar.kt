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
import com.foundry.ide.ui.IdeView

/**
 * Professional IDE Toolbar with quick access actions
 */
@Composable
fun IdeToolbar(
    currentView: IdeView,
    onViewChange: (IdeView) -> Unit,
    isProjectRunning: Boolean,
    onStopProject: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Color(0xFF3C3C3C))
            .padding(horizontal = 16.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        // Quick Action Buttons
        IconButton(
            onClick = { /* New File */ },
            modifier = Modifier.tooltip("New File (Ctrl+N)")
        ) {
            Icon(Icons.Default.Add, "New File", tint = Color.White)
        }

        IconButton(
            onClick = { /* Open File */ },
            modifier = Modifier.tooltip("Open File (Ctrl+O)")
        ) {
            Icon(Icons.Default.FolderOpen, "Open File", tint = Color.White)
        }

        IconButton(
            onClick = { /* Save */ },
            modifier = Modifier.tooltip("Save (Ctrl+S)")
        ) {
            Icon(Icons.Default.Save, "Save", tint = Color.White)
        }

        VerticalDivider()

        // View Toggle Buttons
        ToggleButton(
            selected = currentView == IdeView.EDITOR,
            onClick = { onViewChange(IdeView.EDITOR) },
            tooltip = "Code Editor"
        ) {
            Icon(Icons.Default.Code, "Code Editor")
        }

        ToggleButton(
            selected = currentView == IdeView.WORLD_EDITOR,
            onClick = { onViewChange(IdeView.WORLD_EDITOR) },
            tooltip = "World Editor"
        ) {
            Icon(Icons.Default.Public, "World Editor")
        }

        ToggleButton(
            selected = currentView == IdeView.ASSET_BROWSER,
            onClick = { onViewChange(IdeView.ASSET_BROWSER) },
            tooltip = "Asset Browser"
        ) {
            Icon(Icons.Default.Image, "Asset Browser")
        }

        ToggleButton(
            selected = currentView == IdeView.CONSOLE,
            onClick = { onViewChange(IdeView.CONSOLE) },
            tooltip = "Console"
        ) {
            Icon(Icons.Default.Terminal, "Console")
        }

        VerticalDivider()

        // Build/Run Buttons
        IconButton(
            onClick = { /* Build */ },
            modifier = Modifier.tooltip("Build Project (F7)")
        ) {
            Icon(Icons.Default.Build, "Build", tint = Color(0xFF4CAF50))
        }

        if (isProjectRunning) {
            IconButton(
                onClick = onStopProject,
                modifier = Modifier.tooltip("Stop Project (Shift+F5)")
            ) {
                Icon(Icons.Default.Stop, "Stop", tint = Color(0xFFF44336))
            }
        } else {
            IconButton(
                onClick = { /* Run */ },
                modifier = Modifier.tooltip("Run Project (F5)")
            ) {
                Icon(Icons.Default.PlayArrow, "Run", tint = Color(0xFF4CAF50))
            }
        }

        IconButton(
            onClick = { /* Debug */ },
            modifier = Modifier.tooltip("Debug Project (F5)")
        ) {
            Icon(Icons.Default.BugReport, "Debug", tint = Color(0xFFFF9800))
        }

        VerticalDivider()

        // Tool Buttons
        IconButton(
            onClick = { /* Undo */ },
            modifier = Modifier.tooltip("Undo (Ctrl+Z)")
        ) {
            Icon(Icons.Default.Undo, "Undo", tint = Color.White)
        }

        IconButton(
            onClick = { /* Redo */ },
            modifier = Modifier.tooltip("Redo (Ctrl+Y)")
        ) {
            Icon(Icons.Default.Redo, "Redo", tint = Color.White)
        }

        VerticalDivider()

        // Search
        OutlinedTextField(
            value = "",
            onValueChange = { /* Handle search */ },
            placeholder = { Text("Search...", color = Color.Gray) },
            modifier = Modifier.width(200.dp),
            textStyle = LocalTextStyle.current.copy(color = Color.White),
            colors = TextFieldDefaults.outlinedTextFieldColors(
                backgroundColor = Color(0xFF2A2A2A),
                focusedBorderColor = Color(0xFF007ACC),
                unfocusedBorderColor = Color(0xFF555555),
                cursorColor = Color.White
            ),
            leadingIcon = {
                Icon(Icons.Default.Search, "Search", tint = Color.Gray)
            },
            singleLine = true
        )

        Spacer(modifier = Modifier.weight(1f))

        // Right side tools
        IconButton(
            onClick = { /* Git */ },
            modifier = Modifier.tooltip("Git")
        ) {
            Icon(Icons.Default.CallSplit, "Git", tint = Color(0xFFF4511E))
        }

        IconButton(
            onClick = { /* Extensions */ },
            modifier = Modifier.tooltip("Extensions")
        ) {
            Icon(Icons.Default.Extension, "Extensions", tint = Color(0xFF9C27B0))
        }

        IconButton(
            onClick = { /* Settings */ },
            modifier = Modifier.tooltip("Settings")
        ) {
            Icon(Icons.Default.Settings, "Settings", tint = Color.White)
        }
    }
}

/**
 * Toggle button for toolbar
 */
@Composable
private fun ToggleButton(
    selected: Boolean,
    onClick: () -> Unit,
    tooltip: String,
    content: @Composable () -> Unit
) {
    IconButton(
        onClick = onClick,
        modifier = Modifier.tooltip(tooltip)
    ) {
        val tint = if (selected) Color(0xFF007ACC) else Color.White
        CompositionLocalProvider(LocalContentColor provides tint) {
            content()
        }
    }
}

/**
 * Vertical divider for toolbar
 */
@Composable
private fun VerticalDivider() {
    Box(
        modifier = Modifier
            .width(1.dp)
            .height(24.dp)
            .background(Color(0xFF555555))
    )
}

/**
 * Tooltip modifier extension
 */
fun Modifier.tooltip(text: String): Modifier = this.then(
    Modifier.hoverable(remember { mutableStateOf(false) })
)