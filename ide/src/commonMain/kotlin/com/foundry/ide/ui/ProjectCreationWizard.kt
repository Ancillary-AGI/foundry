package com.foundry.ide.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.selection.selectableGroup
import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.foundry.ide.projects.*
import kotlinx.coroutines.launch

/**
 * Project Creation Wizard for FoundryEngine
 * Provides a step-by-step interface for creating new TypeScript projects
 */
@Composable
fun ProjectCreationWizard(
    onProjectCreated: (String) -> Unit,
    onCancel: () -> Unit
) {
    var currentStep by remember { mutableStateOf(0) }
    var projectConfig by remember { mutableStateOf(NativeTypeScriptProjectConfig("")) }
    var selectedTemplate by remember { mutableStateOf(ProjectTemplate.BASIC_3D) }
    var projectPath by remember { mutableStateOf("") }
    var isLoading by remember { mutableStateOf(false) }
    var errorMessage by remember { mutableStateOf("") }
    
    val scope = rememberCoroutineScope()
    val projectManager = remember { NativeTypeScriptProject() }

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(16.dp),
        elevation = 8.dp
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(24.dp)
        ) {
            // Header
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "Create New FoundryEngine Project",
                    fontSize = 24.sp,
                    fontWeight = FontWeight.Bold,
                    color = MaterialTheme.colors.primary
                )
                
                IconButton(onClick = onCancel) {
                    Icon(Icons.Default.Close, contentDescription = "Close")
                }
            }
            
            Spacer(modifier = Modifier.height(16.dp))
            
            // Progress indicator
            LinearProgressIndicator(
                progress = (currentStep + 1) / 4f,
                modifier = Modifier.fillMaxWidth()
            )
            
            Spacer(modifier = Modifier.height(24.dp))
            
            // Step content
            when (currentStep) {
                0 -> ProjectBasicInfoStep(
                    projectConfig = projectConfig,
                    onConfigChanged = { projectConfig = it }
                )
                1 -> ProjectTemplateStep(
                    selectedTemplate = selectedTemplate,
                    onTemplateSelected = { selectedTemplate = it }
                )
                2 -> ProjectPlatformStep(
                    projectConfig = projectConfig,
                    onConfigChanged = { projectConfig = it }
                )
                3 -> ProjectPathStep(
                    projectPath = projectPath,
                    onPathChanged = { projectPath = it }
                )
            }
            
            Spacer(modifier = Modifier.height(24.dp))
            
            // Error message
            if (errorMessage.isNotEmpty()) {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    backgroundColor = MaterialTheme.colors.error.copy(alpha = 0.1f)
                ) {
                    Row(
                        modifier = Modifier.padding(16.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Icon(
                            Icons.Default.Error,
                            contentDescription = "Error",
                            tint = MaterialTheme.colors.error
                        )
                        Spacer(modifier = Modifier.width(8.dp))
                        Text(
                            text = errorMessage,
                            color = MaterialTheme.colors.error
                        )
                    }
                }
                Spacer(modifier = Modifier.height(16.dp))
            }
            
            // Navigation buttons
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Button(
                    onClick = { 
                        if (currentStep > 0) {
                            currentStep--
                            errorMessage = ""
                        } else {
                            onCancel()
                        }
                    },
                    enabled = !isLoading
                ) {
                    Text(if (currentStep == 0) "Cancel" else "Back")
                }
                
                Button(
                    onClick = {
                        when (currentStep) {
                            0, 1, 2 -> {
                                if (validateStep(currentStep, projectConfig, selectedTemplate)) {
                                    currentStep++
                                    errorMessage = ""
                                } else {
                                    errorMessage = getValidationError(currentStep)
                                }
                            }
                            3 -> {
                                scope.launch {
                                    isLoading = true
                                    errorMessage = ""
                                    
                                    try {
                                        val result = projectManager.createProject(
                                            projectPath,
                                            projectConfig,
                                            selectedTemplate
                                        )
                                        
                                        when (result) {
                                            is ProjectCreationResult.Success -> {
                                                onProjectCreated(projectPath)
                                            }
                                            is ProjectCreationResult.Failure -> {
                                                errorMessage = result.error
                                            }
                                        }
                                    } catch (e: Exception) {
                                        errorMessage = "Failed to create project: ${e.message}"
                                    } finally {
                                        isLoading = false
                                    }
                                }
                            }
                        }
                    },
                    enabled = !isLoading && canProceed(currentStep, projectConfig, projectPath)
                ) {
                    if (isLoading) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(16.dp),
                            strokeWidth = 2.dp
                        )
                        Spacer(modifier = Modifier.width(8.dp))
                    }
                    Text(if (currentStep == 3) "Create Project" else "Next")
                }
            }
        }
    }
}

@Composable
private fun ProjectBasicInfoStep(
    projectConfig: NativeTypeScriptProjectConfig,
    onConfigChanged: (NativeTypeScriptProjectConfig) -> Unit
) {
    var name by remember { mutableStateOf(projectConfig.name) }
    var description by remember { mutableStateOf(projectConfig.description) }
    var author by remember { mutableStateOf(projectConfig.author) }
    var version by remember { mutableStateOf(projectConfig.version) }
    
    Column(
        modifier = Modifier.fillMaxWidth()
    ) {
        Text(
            text = "Project Information",
            fontSize = 20.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        OutlinedTextField(
            value = name,
            onValueChange = { 
                name = it
                onConfigChanged(projectConfig.copy(name = it))
            },
            label = { Text("Project Name") },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true
        )
        
        Spacer(modifier = Modifier.height(16.dp))
        
        OutlinedTextField(
            value = description,
            onValueChange = { 
                description = it
                onConfigChanged(projectConfig.copy(description = it))
            },
            label = { Text("Description") },
            modifier = Modifier.fillMaxWidth(),
            maxLines = 3
        )
        
        Spacer(modifier = Modifier.height(16.dp))
        
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            OutlinedTextField(
                value = author,
                onValueChange = { 
                    author = it
                    onConfigChanged(projectConfig.copy(author = it))
                },
                label = { Text("Author") },
                modifier = Modifier.weight(1f),
                singleLine = true
            )
            
            OutlinedTextField(
                value = version,
                onValueChange = { 
                    version = it
                    onConfigChanged(projectConfig.copy(version = it))
                },
                label = { Text("Version") },
                modifier = Modifier.weight(1f),
                singleLine = true
            )
        }
    }
}

@Composable
private fun ProjectTemplateStep(
    selectedTemplate: ProjectTemplate,
    onTemplateSelected: (ProjectTemplate) -> Unit
) {
    Column(
        modifier = Modifier.fillMaxWidth()
    ) {
        Text(
            text = "Choose Template",
            fontSize = 20.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        Text(
            text = "Select a template to get started quickly with your game project.",
            color = MaterialTheme.colors.onSurface.copy(alpha = 0.7f),
            modifier = Modifier.padding(bottom = 24.dp)
        )
        
        LazyColumn(
            modifier = Modifier.fillMaxWidth(),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            items(ProjectTemplate.values()) { template ->
                TemplateCard(
                    template = template,
                    isSelected = template == selectedTemplate,
                    onClick = { onTemplateSelected(template) }
                )
            }
        }
    }
}

@Composable
private fun TemplateCard(
    template: ProjectTemplate,
    isSelected: Boolean,
    onClick: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .selectable(
                selected = isSelected,
                onClick = onClick
            ),
        elevation = if (isSelected) 4.dp else 1.dp,
        backgroundColor = if (isSelected) 
            MaterialTheme.colors.primary.copy(alpha = 0.1f) 
        else 
            MaterialTheme.colors.surface
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            RadioButton(
                selected = isSelected,
                onClick = onClick
            )
            
            Spacer(modifier = Modifier.width(16.dp))
            
            Column(
                modifier = Modifier.weight(1f)
            ) {
                Text(
                    text = getTemplateName(template),
                    fontWeight = FontWeight.Medium,
                    fontSize = 16.sp
                )
                
                Text(
                    text = getTemplateDescription(template),
                    color = MaterialTheme.colors.onSurface.copy(alpha = 0.7f),
                    fontSize = 14.sp
                )
            }
            
            Icon(
                getTemplateIcon(template),
                contentDescription = null,
                tint = if (isSelected) MaterialTheme.colors.primary else MaterialTheme.colors.onSurface.copy(alpha = 0.5f)
            )
        }
    }
}

@Composable
private fun ProjectPlatformStep(
    projectConfig: NativeTypeScriptProjectConfig,
    onConfigChanged: (NativeTypeScriptProjectConfig) -> Unit
) {
    var selectedPlatforms by remember { mutableStateOf(projectConfig.targetPlatforms.toSet()) }
    
    Column(
        modifier = Modifier.fillMaxWidth()
    ) {
        Text(
            text = "Target Platforms",
            fontSize = 20.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        Text(
            text = "Select the platforms you want to target. You can always add more platforms later.",
            color = MaterialTheme.colors.onSurface.copy(alpha = 0.7f),
            modifier = Modifier.padding(bottom = 24.dp)
        )
        
        Column(
            modifier = Modifier.selectableGroup()
        ) {
            PlatformType.values().forEach { platform ->
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .selectable(
                            selected = platform in selectedPlatforms,
                            onClick = {
                                selectedPlatforms = if (platform in selectedPlatforms) {
                                    selectedPlatforms - platform
                                } else {
                                    selectedPlatforms + platform
                                }
                                onConfigChanged(projectConfig.copy(targetPlatforms = selectedPlatforms.toList()))
                            }
                        )
                        .padding(vertical = 8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Checkbox(
                        checked = platform in selectedPlatforms,
                        onCheckedChange = {
                            selectedPlatforms = if (platform in selectedPlatforms) {
                                selectedPlatforms - platform
                            } else {
                                selectedPlatforms + platform
                            }
                            onConfigChanged(projectConfig.copy(targetPlatforms = selectedPlatforms.toList()))
                        }
                    )
                    
                    Spacer(modifier = Modifier.width(16.dp))
                    
                    Icon(
                        getPlatformIcon(platform),
                        contentDescription = null,
                        modifier = Modifier.size(24.dp)
                    )
                    
                    Spacer(modifier = Modifier.width(16.dp))
                    
                    Column {
                        Text(
                            text = getPlatformName(platform),
                            fontWeight = FontWeight.Medium
                        )
                        Text(
                            text = getPlatformDescription(platform),
                            color = MaterialTheme.colors.onSurface.copy(alpha = 0.7f),
                            fontSize = 12.sp
                        )
                    }
                }
            }
        }
        
        Spacer(modifier = Modifier.height(24.dp))
        
        // Features section
        Text(
            text = "Features",
            fontSize = 18.sp,
            fontWeight = FontWeight.Medium,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        Column {
            FeatureToggle(
                title = "Physics Simulation",
                description = "Enable physics simulation with Bullet Physics",
                checked = projectConfig.features.includePhysics,
                onCheckedChange = { 
                    onConfigChanged(projectConfig.copy(
                        features = projectConfig.features.copy(includePhysics = it)
                    ))
                }
            )
            
            FeatureToggle(
                title = "Audio System",
                description = "Enable 3D spatial audio system",
                checked = projectConfig.features.includeAudio,
                onCheckedChange = { 
                    onConfigChanged(projectConfig.copy(
                        features = projectConfig.features.copy(includeAudio = it)
                    ))
                }
            )
            
            FeatureToggle(
                title = "Networking",
                description = "Enable multiplayer networking support",
                checked = projectConfig.features.includeNetworking,
                onCheckedChange = { 
                    onConfigChanged(projectConfig.copy(
                        features = projectConfig.features.copy(includeNetworking = it)
                    ))
                }
            )
            
            FeatureToggle(
                title = "Server Integration",
                description = "Include server-side code and APIs",
                checked = projectConfig.features.includeServer,
                onCheckedChange = { 
                    onConfigChanged(projectConfig.copy(
                        features = projectConfig.features.copy(includeServer = it)
                    ))
                }
            )
        }
    }
}

@Composable
private fun FeatureToggle(
    title: String,
    description: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Checkbox(
            checked = checked,
            onCheckedChange = onCheckedChange
        )
        
        Spacer(modifier = Modifier.width(16.dp))
        
        Column {
            Text(
                text = title,
                fontWeight = FontWeight.Medium
            )
            Text(
                text = description,
                color = MaterialTheme.colors.onSurface.copy(alpha = 0.7f),
                fontSize = 12.sp
            )
        }
    }
}

@Composable
private fun ProjectPathStep(
    projectPath: String,
    onPathChanged: (String) -> Unit
) {
    Column(
        modifier = Modifier.fillMaxWidth()
    ) {
        Text(
            text = "Project Location",
            fontSize = 20.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        Text(
            text = "Choose where to create your project. The project will be created in a new folder at this location.",
            color = MaterialTheme.colors.onSurface.copy(alpha = 0.7f),
            modifier = Modifier.padding(bottom = 24.dp)
        )
        
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            OutlinedTextField(
                value = projectPath,
                onValueChange = onPathChanged,
                label = { Text("Project Path") },
                modifier = Modifier.weight(1f),
                singleLine = true
            )
            
            Spacer(modifier = Modifier.width(8.dp))
            
            Button(
                onClick = { /* TODO: Open folder picker */ }
            ) {
                Text("Browse")
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // GitHub template option
        Card(
            modifier = Modifier.fillMaxWidth(),
            backgroundColor = MaterialTheme.colors.surface
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Or create from GitHub template",
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                
                Text(
                    text = "Clone an existing project template from GitHub",
                    color = MaterialTheme.colors.onSurface.copy(alpha = 0.7f),
                    fontSize = 14.sp,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                OutlinedTextField(
                    value = "",
                    onValueChange = { /* TODO: Handle GitHub URL input */ },
                    label = { Text("GitHub Repository URL") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true
                )
            }
        }
    }
}

// Helper functions
private fun validateStep(step: Int, config: NativeTypeScriptProjectConfig, template: ProjectTemplate): Boolean {
    return when (step) {
        0 -> config.name.isNotBlank() && config.author.isNotBlank()
        1 -> true // Template is always valid
        2 -> config.targetPlatforms.isNotEmpty()
        3 -> true // Path validation is handled separately
        else -> false
    }
}

private fun canProceed(step: Int, config: NativeTypeScriptProjectConfig, projectPath: String): Boolean {
    return when (step) {
        0 -> config.name.isNotBlank() && config.author.isNotBlank()
        1 -> true
        2 -> config.targetPlatforms.isNotEmpty()
        3 -> projectPath.isNotBlank()
        else -> false
    }
}

private fun getValidationError(step: Int): String {
    return when (step) {
        0 -> "Please fill in all required fields"
        1 -> "Please select a template"
        2 -> "Please select at least one target platform"
        3 -> "Please specify a project path"
        else -> "Unknown error"
    }
}

private fun getTemplateName(template: ProjectTemplate): String {
    return when (template) {
        ProjectTemplate.BASIC_2D -> "2D Game"
        ProjectTemplate.BASIC_3D -> "3D Game"
        ProjectTemplate.PLATFORMER -> "Platformer"
        ProjectTemplate.FPS -> "First-Person Shooter"
        ProjectTemplate.RTS -> "Real-Time Strategy"
        ProjectTemplate.MOBILE_GAME -> "Mobile Game"
        ProjectTemplate.VR_EXPERIENCE -> "VR Experience"
        ProjectTemplate.WEB_GAME -> "Web Game"
        ProjectTemplate.MULTIPLAYER -> "Multiplayer Game"
        ProjectTemplate.CUSTOM -> "Custom Template"
    }
}

private fun getTemplateDescription(template: ProjectTemplate): String {
    return when (template) {
        ProjectTemplate.BASIC_2D -> "A simple 2D game with basic physics and rendering"
        ProjectTemplate.BASIC_3D -> "A 3D game with camera, lighting, and basic objects"
        ProjectTemplate.PLATFORMER -> "A 2D platformer with character controller and levels"
        ProjectTemplate.FPS -> "A first-person shooter with weapons and enemies"
        ProjectTemplate.RTS -> "A real-time strategy game with units and buildings"
        ProjectTemplate.MOBILE_GAME -> "A mobile-optimized game with touch controls"
        ProjectTemplate.VR_EXPERIENCE -> "A VR experience with hand tracking and spatial audio"
        ProjectTemplate.WEB_GAME -> "A web-based game with HTML5 canvas"
        ProjectTemplate.MULTIPLAYER -> "A multiplayer game with networking and server code"
        ProjectTemplate.CUSTOM -> "Start with a minimal template and build your own"
    }
}

@Composable
private fun getTemplateIcon(template: ProjectTemplate) = when (template) {
    ProjectTemplate.BASIC_2D -> Icons.Default.Games
    ProjectTemplate.BASIC_3D -> Icons.Default.ViewInAr
    ProjectTemplate.PLATFORMER -> Icons.Default.Jumping
    ProjectTemplate.FPS -> Icons.Default.SportsEsports
    ProjectTemplate.RTS -> Icons.Default.MilitaryTech
    ProjectTemplate.MOBILE_GAME -> Icons.Default.PhoneAndroid
    ProjectTemplate.VR_EXPERIENCE -> Icons.Default.Vr
    ProjectTemplate.WEB_GAME -> Icons.Default.Web
    ProjectTemplate.MULTIPLAYER -> Icons.Default.Group
    ProjectTemplate.CUSTOM -> Icons.Default.Build
}

private fun getPlatformName(platform: PlatformType): String {
    return when (platform) {
        PlatformType.WINDOWS -> "Windows"
        PlatformType.MACOS -> "macOS"
        PlatformType.LINUX -> "Linux"
        PlatformType.ANDROID -> "Android"
        PlatformType.IOS -> "iOS"
        PlatformType.WEB -> "Web"
        PlatformType.DESKTOP -> "Desktop"
    }
}

private fun getPlatformDescription(platform: PlatformType): String {
    return when (platform) {
        PlatformType.WINDOWS -> "Windows 10/11 with DirectX 11"
        PlatformType.MACOS -> "macOS 10.15+ with Metal"
        PlatformType.LINUX -> "Linux with OpenGL"
        PlatformType.ANDROID -> "Android 7.0+ with OpenGL ES"
        PlatformType.IOS -> "iOS 13.0+ with Metal"
        PlatformType.WEB -> "Web browsers with WebGL 2.0"
        PlatformType.DESKTOP -> "Cross-platform desktop"
    }
}

@Composable
private fun getPlatformIcon(platform: PlatformType) = when (platform) {
    PlatformType.WINDOWS -> Icons.Default.Computer
    PlatformType.MACOS -> Icons.Default.Apple
    PlatformType.LINUX -> Icons.Default.Terminal
    PlatformType.ANDROID -> Icons.Default.PhoneAndroid
    PlatformType.IOS -> Icons.Default.PhoneIphone
    PlatformType.WEB -> Icons.Default.Web
    PlatformType.DESKTOP -> Icons.Default.DesktopWindows
}
