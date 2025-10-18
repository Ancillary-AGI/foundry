package com.foundry.ide.ui.components

import androidx.compose.foundation.*
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.*
import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.runtime.*
import androidx.compose.ui.*
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.*
import com.foundry.ide.ProjectInfo

/**
 * Professional Project Explorer with file tree navigation
 */
@Composable
fun ProjectExplorer(
    modifier: Modifier = Modifier,
    currentProject: ProjectInfo?,
    onFileSelect: (String) -> Unit
) {
    Card(
        modifier = modifier.fillMaxHeight(),
        elevation = 4.dp,
        backgroundColor = Color(0xFF252526)
    ) {
        Column(modifier = Modifier.fillMaxSize()) {
            // Header
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color(0xFF3C3C3C))
                    .padding(12.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(
                    Icons.Default.Folder,
                    "Project",
                    tint = Color(0xFF4CAF50),
                    modifier = Modifier.size(20.dp)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    text = currentProject?.name ?: "No Project",
                    style = MaterialTheme.typography.h6,
                    color = Color.White,
                    maxLines = 1,
                    modifier = Modifier.weight(1f)
                )
                IconButton(onClick = { /* Refresh */ }) {
                    Icon(
                        Icons.Default.Refresh,
                        "Refresh",
                        tint = Color.White,
                        modifier = Modifier.size(16.dp)
                    )
                }
            }

            // Project content
            if (currentProject != null) {
                ProjectTree(
                    project = currentProject,
                    onFileSelect = onFileSelect,
                    modifier = Modifier.weight(1f)
                )
            } else {
                // No project loaded
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .padding(16.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Icon(
                            Icons.Default.FolderOpen,
                            "No Project",
                            tint = Color.Gray,
                            modifier = Modifier.size(48.dp)
                        )
                        Spacer(modifier = Modifier.height(16.dp))
                        Text(
                            "No project loaded",
                            style = MaterialTheme.typography.body2,
                            color = Color.Gray
                        )
                        Spacer(modifier = Modifier.height(8.dp))
                        Text(
                            "Create or open a project to get started",
                            style = MaterialTheme.typography.caption,
                            color = Color.Gray,
                            textAlign = androidx.compose.ui.text.style.TextAlign.Center
                        )
                    }
                }
            }
        }
    }
}

/**
 * Project file tree component
 */
@Composable
private fun ProjectTree(
    project: ProjectInfo,
    onFileSelect: (String) -> Unit,
    modifier: Modifier = Modifier
) {
    val expandedFolders = remember { mutableStateMapOf<String, Boolean>() }

    LazyColumn(modifier = modifier) {
        // Root project folder
        item {
            ProjectTreeItem(
                name = project.name,
                icon = Icons.Default.Folder,
                iconTint = Color(0xFF4CAF50),
                isFolder = true,
                isExpanded = expandedFolders[project.name] ?: true,
                onClick = {
                    expandedFolders[project.name] = !(expandedFolders[project.name] ?: true)
                },
                level = 0
            )
        }

        // Project contents (when expanded)
        if (expandedFolders[project.name] == true) {
            // Scenes folder
            item {
                ProjectTreeItem(
                    name = "scenes",
                    icon = Icons.Default.Folder,
                    iconTint = Color(0xFFFF9800),
                    isFolder = true,
                    isExpanded = expandedFolders["scenes"] ?: false,
                    onClick = {
                        expandedFolders["scenes"] = !(expandedFolders["scenes"] ?: false)
                    },
                    level = 1
                )
            }

            // Scripts folder
            item {
                ProjectTreeItem(
                    name = "scripts",
                    icon = Icons.Default.Folder,
                    iconTint = Color(0xFF2196F3),
                    isFolder = true,
                    isExpanded = expandedFolders["scripts"] ?: false,
                    onClick = {
                        expandedFolders["scripts"] = !(expandedFolders["scripts"] ?: false)
                    },
                    level = 1
                )
            }

            // Assets folder
            item {
                ProjectTreeItem(
                    name = "assets",
                    icon = Icons.Default.Folder,
                    iconTint = Color(0xFF9C27B0),
                    isFolder = true,
                    isExpanded = expandedFolders["assets"] ?: false,
                    onClick = {
                        expandedFolders["assets"] = !(expandedFolders["assets"] ?: false)
                    },
                    level = 1
                )
            }

            // Sample files
            val sampleFiles = listOf(
                "main.scene" to Icons.Default.Description,
                "GameManager.kt" to Icons.Default.Code,
                "PlayerController.kt" to Icons.Default.Code,
                "background.png" to Icons.Default.Image,
                "soundtrack.mp3" to Icons.Default.VolumeUp
            )

            sampleFiles.forEach { (fileName, icon) ->
                item {
                    ProjectTreeItem(
                        name = fileName,
                        icon = icon,
                        iconTint = getFileIconTint(fileName),
                        isFolder = false,
                        isExpanded = false,
                        onClick = { onFileSelect(fileName) },
                        level = 1
                    )
                }
            }
        }
    }
}

/**
 * Individual tree item component
 */
@Composable
private fun ProjectTreeItem(
    name: String,
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    iconTint: Color,
    isFolder: Boolean,
    isExpanded: Boolean,
    onClick: () -> Unit,
    level: Int
) {
    val indent = level * 20

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clickable(onClick = onClick)
            .padding(start = indent.dp, end = 8.dp, top = 4.dp, bottom = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        if (isFolder) {
            Icon(
                if (isExpanded) Icons.Default.ExpandMore else Icons.Default.ChevronRight,
                "Expand",
                tint = Color.White,
                modifier = Modifier.size(16.dp)
            )
            Spacer(modifier = Modifier.width(4.dp))
        } else {
            Spacer(modifier = Modifier.width(20.dp))
        }

        Icon(
            icon,
            name,
            tint = iconTint,
            modifier = Modifier.size(16.dp)
        )

        Spacer(modifier = Modifier.width(8.dp))

        Text(
            text = name,
            style = MaterialTheme.typography.body2,
            color = Color.White,
            maxLines = 1,
            modifier = Modifier.weight(1f)
        )
    }
}

/**
 * Get appropriate icon tint based on file extension
 */
private fun getFileIconTint(fileName: String): Color {
    return when {
        fileName.endsWith(".kt") || fileName.endsWith(".java") -> Color(0xFF2196F3)
        fileName.endsWith(".cpp") || fileName.endsWith(".h") -> Color(0xFF4CAF50)
        fileName.endsWith(".scene") -> Color(0xFFFF9800)
        fileName.endsWith(".png") || fileName.endsWith(".jpg") -> Color(0xFF9C27B0)
        fileName.endsWith(".mp3") || fileName.endsWith(".wav") -> Color(0xFFFF5722)
        else -> Color.White
    }
}