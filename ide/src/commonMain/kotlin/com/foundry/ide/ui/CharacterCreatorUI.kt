package com.foundry.ide.ui

import androidx.compose.foundation.*
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.*
import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.runtime.*
import androidx.compose.ui.*
import androidx.compose.ui.graphics.*
import androidx.compose.ui.text.font.*
import androidx.compose.ui.unit.*
import androidx.compose.ui.window.*
import com.foundry.ide.*

/**
 * Character Creator UI for Foundry IDE
 * Complete character creation and editing interface
 */
@Composable
fun CharacterCreatorWindow(onClose: () -> Unit) {
    var currentTab by remember { mutableStateOf(CharacterTab.APPEARANCE) }
    var selectedBodyPart by remember { mutableStateOf<BodyPart?>(null) }
    var characterName by remember { mutableStateOf("New Character") }
    var characterData by remember { mutableStateOf(CharacterData()) }
    var selectedTool by remember { mutableStateOf<CharacterTool>(CharacterTool.SELECT) }
    var symmetryEnabled by remember { mutableStateOf(true) }
    var snapToGrid by remember { mutableStateOf(false) }
    var showWireframe by remember { mutableStateOf(false) }
    var viewportMode by remember { mutableStateOf(ViewportMode.PERSPECTIVE) }

    Window(
        onCloseRequest = onClose,
        title = "Character Creator - $characterName",
        state = rememberWindowState(width = 1400.dp, height = 900.dp)
    ) {
        MaterialTheme {
            Row(modifier = Modifier.fillMaxSize()) {
                // Left Panel - Character Preview & Controls
                Column(
                    modifier = Modifier
                        .width(300.dp)
                        .fillMaxHeight()
                        .padding(16.dp)
                ) {
                    CharacterPreviewPanel(characterData)
                    Spacer(modifier = Modifier.height(16.dp))
                    CharacterControlsPanel(
                        characterName = characterName,
                        onNameChange = { characterName = it },
                        onSave = { saveCharacter() },
                        onLoad = { loadCharacter() },
                        onExport = { exportCharacter() }
                    )
                }

                // Center - 3D Preview
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxHeight()
                        .background(Color.DarkGray.copy(alpha = 0.1f))
                ) {
                    Character3DPreview(characterData, selectedBodyPart)
                }

                // Right Panel - Editing Tools
                Column(
                    modifier = Modifier
                        .width(400.dp)
                        .fillMaxHeight()
                        .padding(16.dp)
                ) {
                    // Advanced Toolbar
                    CharacterToolbar(
                        selectedTool = selectedTool,
                        onToolSelect = { selectedTool = it },
                        symmetryEnabled = symmetryEnabled,
                        onSymmetryToggle = { symmetryEnabled = it },
                        snapToGrid = snapToGrid,
                        onSnapToggle = { snapToGrid = it },
                        showWireframe = showWireframe,
                        onWireframeToggle = { showWireframe = it },
                        viewportMode = viewportMode,
                        onViewportModeChange = { viewportMode = it }
                    )

                    Spacer(modifier = Modifier.height(16.dp))

                    CharacterTabs(currentTab, onTabChange = { currentTab = it })
                    Spacer(modifier = Modifier.height(16.dp))

                    when (currentTab) {
                        CharacterTab.APPEARANCE -> AppearancePanel(
                            characterData = characterData,
                            onDataChange = { characterData = it },
                            onBodyPartSelect = { selectedBodyPart = it },
                            selectedTool = selectedTool
                        )
                        CharacterTab.ANIMATION -> AnimationPanel(characterData)
                        CharacterTab.PHYSICS -> PhysicsPanel(characterData)
                        CharacterTab.AI -> AIPanel(characterData)
                        CharacterTab.AUDIO -> AudioPanel(characterData)
                    }
                }
            }
        }
    }

    fun saveCharacter() {
        // Save character to project
        println("Saving character: $characterName")
    }

    fun loadCharacter() {
        // Load character from file
        println("Loading character")
    }

    fun exportCharacter() {
        // Export character for use in game
        println("Exporting character: $characterName")
    }
}

enum class CharacterTab {
    APPEARANCE, ANIMATION, PHYSICS, AI, AUDIO
}

enum class CharacterTool {
    SELECT, MOVE, ROTATE, SCALE, EXTRUDE, LOOP_CUT, KNIFE, BRUSH
}

enum class ViewportMode {
    PERSPECTIVE, ORTHOGRAPHIC_FRONT, ORTHOGRAPHIC_SIDE, ORTHOGRAPHIC_TOP
}

enum class BodyPart {
    HEAD, TORSO, LEFT_ARM, RIGHT_ARM, LEFT_LEG, RIGHT_LEG, HANDS, FEET
}

data class CharacterData(
    val name: String = "Character",
    val appearance: AppearanceData = AppearanceData(),
    val animations: List<AnimationData> = emptyList(),
    val physics: PhysicsData = PhysicsData(),
    val ai: AIData = AIData(),
    val audio: AudioData = AudioData()
)

data class AppearanceData(
    val skinColor: Color = Color(0xFFDEB887),
    val hairColor: Color = Color(0xFF8B4513),
    val eyeColor: Color = Color(0xFF000080),
    val bodyType: BodyType = BodyType.MEDIUM,
    val height: Float = 1.75f,
    val weight: Float = 70f,
    val age: Int = 25,
    val gender: Gender = Gender.MALE,
    val clothing: Map<String, ClothingItem> = emptyMap(),
    val accessories: List<AccessoryItem> = emptyList()
)

enum class BodyType { SLIM, MEDIUM, MUSCULAR, HEAVY }
enum class Gender { MALE, FEMALE, OTHER }

data class ClothingItem(
    val id: String,
    val name: String,
    val category: String,
    val color: Color = Color.White,
    val texture: String? = null
)

data class AccessoryItem(
    val id: String,
    val name: String,
    val type: String,
    val position: BodyPart
)

data class AnimationData(
    val id: String,
    val name: String,
    val type: AnimationType,
    val duration: Float,
    val loop: Boolean = false,
    val blendTime: Float = 0.2f
)

enum class AnimationType { IDLE, WALK, RUN, JUMP, ATTACK, DEFEND, DIE }

data class PhysicsData(
    val mass: Float = 70f,
    val drag: Float = 0.1f,
    val useGravity: Boolean = true,
    val colliderType: ColliderType = ColliderType.CAPSULE,
    val colliderSize: Vector3 = Vector3(0.5f, 1.8f, 0.5f),
    val jointConstraints: List<JointConstraint> = emptyList()
)

enum class ColliderType { BOX, SPHERE, CAPSULE, MESH }

data class JointConstraint(
    val joint: String,
    val minAngle: Float,
    val maxAngle: Float,
    val stiffness: Float = 1f
)

data class AIData(
    val behaviorTree: String? = null,
    val navigationMesh: String? = null,
    val perceptionRange: Float = 10f,
    val movementSpeed: Float = 3.5f,
    val intelligence: Int = 50,
    val aggression: Int = 30,
    val fear: Int = 20
)

data class AudioData(
    val footsteps: List<String> = emptyList(),
    val voiceLines: Map<String, String> = emptyMap(),
    val combatSounds: Map<String, String> = emptyMap(),
    val ambientSounds: List<String> = emptyList(),
    val voicePitch: Float = 1f,
    val voiceVolume: Float = 1f
)

@Composable
private fun CharacterPreviewPanel(characterData: CharacterData) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text("Character Preview", style = MaterialTheme.typography.h6)

            Spacer(modifier = Modifier.height(16.dp))

            // Character stats
            Row(modifier = Modifier.fillMaxWidth()) {
                Column(modifier = Modifier.weight(1f)) {
                    Text("Height: ${"%.2f".format(characterData.appearance.height)}m")
                    Text("Weight: ${characterData.appearance.weight}kg")
                    Text("Age: ${characterData.appearance.age}")
                }
                Column(modifier = Modifier.weight(1f)) {
                    Text("Body Type: ${characterData.appearance.bodyType}")
                    Text("Gender: ${characterData.appearance.gender}")
                    Text("Mass: ${characterData.physics.mass}kg")
                }
            }

            Spacer(modifier = Modifier.height(16.dp))

            // Color swatches
            Text("Colors:", style = MaterialTheme.typography.subtitle1)
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceEvenly) {
                ColorSwatch("Skin", characterData.appearance.skinColor)
                ColorSwatch("Hair", characterData.appearance.hairColor)
                ColorSwatch("Eyes", characterData.appearance.eyeColor)
            }
        }
    }
}

@Composable
private fun ColorSwatch(label: String, color: Color) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Box(
            modifier = Modifier
                .size(32.dp)
                .background(color, shape = MaterialTheme.shapes.small)
                .border(1.dp, Color.Black, MaterialTheme.shapes.small)
        )
        Text(label, style = MaterialTheme.typography.caption)
    }
}

@Composable
private fun CharacterControlsPanel(
    characterName: String,
    onNameChange: (String) -> Unit,
    onSave: () -> Unit,
    onLoad: () -> Unit,
    onExport: () -> Unit
) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text("Character Controls", style = MaterialTheme.typography.h6)

            Spacer(modifier = Modifier.height(16.dp))

            OutlinedTextField(
                value = characterName,
                onValueChange = onNameChange,
                label = { Text("Character Name") },
                modifier = Modifier.fillMaxWidth()
            )

            Spacer(modifier = Modifier.height(16.dp))

            Row(modifier = Modifier.fillMaxWidth()) {
                Button(onClick = onSave, modifier = Modifier.weight(1f)) {
                    Icon(Icons.Default.Save, "Save")
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Save")
                }
                Spacer(modifier = Modifier.width(8.dp))
                OutlinedButton(onClick = onLoad, modifier = Modifier.weight(1f)) {
                    Icon(Icons.Default.FolderOpen, "Load")
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Load")
                }
            }

            Spacer(modifier = Modifier.height(8.dp))

            Button(onClick = onExport, modifier = Modifier.fillMaxWidth()) {
                Icon(Icons.Default.Share, "Export")
                Spacer(modifier = Modifier.width(8.dp))
                Text("Export to Game")
            }
        }
    }
}

@Composable
private fun Character3DPreview(characterData: CharacterData, selectedBodyPart: BodyPart?) {
    Box(
        modifier = Modifier.fillMaxSize(),
        contentAlignment = Alignment.Center
    ) {
        // 3D Preview placeholder
        Card(
            modifier = Modifier.size(300.dp),
            elevation = 8.dp
        ) {
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text("3D Character Preview", style = MaterialTheme.typography.h6)
                    Spacer(modifier = Modifier.height(16.dp))
                    Text("Character model would render here", style = MaterialTheme.typography.caption)
                    Spacer(modifier = Modifier.height(16.dp))

                    // Body part selection
                    Text("Selected: ${selectedBodyPart?.name ?: "None"}", style = MaterialTheme.typography.caption)
                }
            }
        }

        // Advanced control buttons
        Column(
            modifier = Modifier
                .align(Alignment.BottomStart)
                .padding(16.dp)
        ) {
            // Viewport controls
            Row {
                IconButton(onClick = { /* Rotate left */ }) {
                    Icon(Icons.Default.RotateLeft, "Rotate Left")
                }
                IconButton(onClick = { /* Zoom in */ }) {
                    Icon(Icons.Default.ZoomIn, "Zoom In")
                }
                IconButton(onClick = { /* Reset view */ }) {
                    Icon(Icons.Default.Refresh, "Reset View")
                }
                IconButton(onClick = { /* Zoom out */ }) {
                    Icon(Icons.Default.ZoomOut, "Zoom Out")
                }
                IconButton(onClick = { /* Rotate right */ }) {
                    Icon(Icons.Default.RotateRight, "Rotate Right")
                }
            }
    
            Spacer(modifier = Modifier.height(8.dp))
    
            // Advanced controls
            Row {
                IconButton(onClick = { /* Orthographic view */ }) {
                    Icon(Icons.Default.GridView, "Orthographic")
                }
                IconButton(onClick = { /* Show/hide grid */ }) {
                    Icon(Icons.Default.GridOn, "Toggle Grid")
                }
                IconButton(onClick = { /* Camera settings */ }) {
                    Icon(Icons.Default.Camera, "Camera Settings")
                }
                IconButton(onClick = { /* Render settings */ }) {
                    Icon(Icons.Default.Settings, "Render Settings")
                }
            }
        }
    
        // Tool gizmos overlay
        if (selectedBodyPart != null) {
            Box(
                modifier = Modifier
                    .align(Alignment.TopEnd)
                    .padding(16.dp)
            ) {
                ToolGizmoOverlay(selectedTool = selectedTool)
            }
        }
    }
}

@Composable
private fun CharacterTabs(currentTab: CharacterTab, onTabChange: (CharacterTab) -> Unit) {
    TabRow(selectedTabIndex = currentTab.ordinal) {
        CharacterTab.values().forEach { tab ->
            Tab(
                selected = currentTab == tab,
                onClick = { onTabChange(tab) },
                text = { Text(tab.name) },
                icon = {
                    when (tab) {
                        CharacterTab.APPEARANCE -> Icon(Icons.Default.Person, tab.name)
                        CharacterTab.ANIMATION -> Icon(Icons.Default.PlayArrow, tab.name)
                        CharacterTab.PHYSICS -> Icon(Icons.Default.Build, tab.name)
                        CharacterTab.AI -> Icon(Icons.Default.Psychology, tab.name)
                        CharacterTab.AUDIO -> Icon(Icons.Default.VolumeUp, tab.name)
                    }
                }
            )
        }
    }
}

@Composable
private fun CharacterToolbar(
    selectedTool: CharacterTool,
    onToolSelect: (CharacterTool) -> Unit,
    symmetryEnabled: Boolean,
    onSymmetryToggle: (Boolean) -> Unit,
    snapToGrid: Boolean,
    onSnapToggle: (Boolean) -> Unit,
    showWireframe: Boolean,
    onWireframeToggle: (Boolean) -> Unit,
    viewportMode: ViewportMode,
    onViewportModeChange: (ViewportMode) -> Unit
) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(8.dp)) {
            Text("Tools", style = MaterialTheme.typography.subtitle1)

            // Tool selection
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceEvenly
            ) {
                CharacterTool.values().forEach { tool ->
                    IconButton(
                        onClick = { onToolSelect(tool) },
                        modifier = Modifier.background(
                            if (selectedTool == tool) Color.LightGray else Color.Transparent,
                            shape = MaterialTheme.shapes.small
                        )
                    ) {
                        Icon(
                            when (tool) {
                                CharacterTool.SELECT -> Icons.Default.TouchApp
                                CharacterTool.MOVE -> Icons.Default.OpenWith
                                CharacterTool.ROTATE -> Icons.Default.RotateRight
                                CharacterTool.SCALE -> Icons.Default.Transform
                                CharacterTool.EXTRUDE -> Icons.Default.ArrowUpward
                                CharacterTool.LOOP_CUT -> Icons.Default.ContentCut
                                CharacterTool.KNIFE -> Icons.Default.CallSplit
                                CharacterTool.BRUSH -> Icons.Default.Brush
                            },
                            tool.name
                        )
                    }
                }
            }

            // Options
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceEvenly
            ) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = symmetryEnabled, onCheckedChange = onSymmetryToggle)
                    Text("Symmetry", style = MaterialTheme.typography.caption)
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = snapToGrid, onCheckedChange = onSnapToggle)
                    Text("Snap", style = MaterialTheme.typography.caption)
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = showWireframe, onCheckedChange = onWireframeToggle)
                    Text("Wireframe", style = MaterialTheme.typography.caption)
                }
            }

            // Viewport mode
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.Center
            ) {
                ViewportMode.values().forEach { mode ->
                    OutlinedButton(
                        onClick = { onViewportModeChange(mode) },
                        modifier = Modifier.padding(horizontal = 2.dp),
                        colors = ButtonDefaults.outlinedButtonColors(
                            backgroundColor = if (viewportMode == mode) Color.LightGray else Color.Transparent
                        )
                    ) {
                        Text(mode.name.replace("_", " "), style = MaterialTheme.typography.caption)
                    }
                }
            }
        }
    }
}

@Composable
private fun ToolGizmoOverlay(selectedTool: CharacterTool) {
    Card(
        modifier = Modifier.size(120.dp),
        elevation = 8.dp
    ) {
        Box(
            modifier = Modifier.fillMaxSize(),
            contentAlignment = Alignment.Center
        ) {
            when (selectedTool) {
                CharacterTool.MOVE -> {
                    // Move gizmo (RGB axes)
                    Canvas(modifier = Modifier.fillMaxSize()) {
                        val center = Offset(size.width / 2, size.height / 2)
                        val length = 30f

                        // X axis (red)
                        drawLine(Color.Red, center, center + Offset(length, 0f), strokeWidth = 3f)
                        // Y axis (green)
                        drawLine(Color.Green, center, center + Offset(0f, -length), strokeWidth = 3f)
                        // Z axis (blue)
                        drawLine(Color.Blue, center, center + Offset(length * 0.7f, length * 0.7f), strokeWidth = 3f)
                    }
                }
                CharacterTool.ROTATE -> {
                    // Rotate gizmo (circles)
                    Canvas(modifier = Modifier.fillMaxSize()) {
                        val center = Offset(size.width / 2, size.height / 2)
                        val radius = 25f

                        // X rotation (red)
                        drawCircle(Color.Red, radius, center, style = Stroke(width = 2f))
                        // Y rotation (green)
                        drawCircle(Color.Green, radius * 0.8f, center, style = Stroke(width = 2f))
                        // Z rotation (blue)
                        drawCircle(Color.Blue, radius * 0.6f, center, style = Stroke(width = 2f))
                    }
                }
                CharacterTool.SCALE -> {
                    // Scale gizmo (cubes)
                    Canvas(modifier = Modifier.fillMaxSize()) {
                        val center = Offset(size.width / 2, size.height / 2)
                        val size = 8f

                        // X handle (red)
                        drawRect(Color.Red, topLeft = center + Offset(20f, -size/2), size = Size(size, size))
                        // Y handle (green)
                        drawRect(Color.Green, topLeft = center + Offset(-size/2, -25f), size = Size(size, size))
                        // Z handle (blue)
                        drawRect(Color.Blue, topLeft = center + Offset(15f, 15f), size = Size(size, size))
                    }
                }
                else -> {
                    Text("Tool: ${selectedTool.name}", style = MaterialTheme.typography.caption)
                }
            }
        }
    }
}

@Composable
private fun AppearancePanel(
    characterData: CharacterData,
    onDataChange: (CharacterData) -> Unit,
    onBodyPartSelect: (BodyPart) -> Unit,
    selectedTool: CharacterTool
) {
    Column(modifier = Modifier.fillMaxSize()) {
        Text("Appearance Editor", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(16.dp))

        // Advanced modeling tools
        when (selectedTool) {
            CharacterTool.EXTRUDE -> ExtrudeToolPanel(characterData, onDataChange)
            CharacterTool.LOOP_CUT -> LoopCutToolPanel(characterData, onDataChange)
            CharacterTool.KNIFE -> KnifeToolPanel(characterData, onDataChange)
            CharacterTool.BRUSH -> BrushToolPanel(characterData, onDataChange)
            else -> {
                // Body parts selection
                Text("Body Parts:", style = MaterialTheme.typography.subtitle1)
                LazyVerticalGrid(
                    columns = GridCells.Fixed(2),
                    modifier = Modifier.height(200.dp)
                ) {
                    items(BodyPart.values()) { bodyPart ->
                        Card(
                            modifier = Modifier
                                .padding(4.dp)
                                .clickable { onBodyPartSelect(bodyPart) },
                            elevation = 4.dp
                        ) {
                            Box(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .height(60.dp),
                                contentAlignment = Alignment.Center
                            ) {
                                Text(bodyPart.name.replace("_", " "))
                            }
                        }
                    }
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Basic attributes
        Text("Basic Attributes:", style = MaterialTheme.typography.subtitle1)
        Spacer(modifier = Modifier.height(8.dp))

        Row(modifier = Modifier.fillMaxWidth()) {
            OutlinedTextField(
                value = characterData.appearance.height.toString(),
                onValueChange = {
                    val height = it.toFloatOrNull() ?: characterData.appearance.height
                    onDataChange(characterData.copy(
                        appearance = characterData.appearance.copy(height = height)
                    ))
                },
                label = { Text("Height (m)") },
                modifier = Modifier.weight(1f)
            )
            Spacer(modifier = Modifier.width(8.dp))
            OutlinedTextField(
                value = characterData.appearance.weight.toString(),
                onValueChange = {
                    val weight = it.toFloatOrNull() ?: characterData.appearance.weight
                    onDataChange(characterData.copy(
                        appearance = characterData.appearance.copy(weight = weight)
                    ))
                },
                label = { Text("Weight (kg)") },
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(8.dp))

        Row(modifier = Modifier.fillMaxWidth()) {
            OutlinedTextField(
                value = characterData.appearance.age.toString(),
                onValueChange = {
                    val age = it.toIntOrNull() ?: characterData.appearance.age
                    onDataChange(characterData.copy(
                        appearance = characterData.appearance.copy(age = age)
                    ))
                },
                label = { Text("Age") },
                modifier = Modifier.weight(1f)
            )
            Spacer(modifier = Modifier.width(8.dp))
            // Gender selection would go here
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Color pickers
        Text("Colors:", style = MaterialTheme.typography.subtitle1)
        // Color picker components would go here
    }
}

@Composable
private fun AnimationPanel(characterData: CharacterData) {
    Column(modifier = Modifier.fillMaxSize()) {
        Text("Animation Editor", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(16.dp))

        // Animation list
        Text("Available Animations:", style = MaterialTheme.typography.subtitle1)
        LazyColumn(modifier = Modifier.weight(1f)) {
            items(characterData.animations) { animation ->
                Card(modifier = Modifier.padding(4.dp)) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(8.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text(animation.name, fontWeight = FontWeight.Bold)
                            Text("${animation.type} - ${animation.duration}s", style = MaterialTheme.typography.caption)
                        }
                        Row {
                            IconButton(onClick = { /* Play animation */ }) {
                                Icon(Icons.Default.PlayArrow, "Play")
                            }
                            IconButton(onClick = { /* Edit animation */ }) {
                                Icon(Icons.Default.Edit, "Edit")
                            }
                        }
                    }
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Button(onClick = { /* Add new animation */ }, modifier = Modifier.fillMaxWidth()) {
            Icon(Icons.Default.Add, "Add")
            Spacer(modifier = Modifier.width(8.dp))
            Text("Add Animation")
        }
    }
}

@Composable
private fun PhysicsPanel(characterData: CharacterData) {
    Column(modifier = Modifier.fillMaxSize()) {
        Text("Physics Settings", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(16.dp))

        // Physics properties
        Text("Physics Properties:", style = MaterialTheme.typography.subtitle1)
        Spacer(modifier = Modifier.height(8.dp))

        OutlinedTextField(
            value = characterData.physics.mass.toString(),
            onValueChange = { /* Update mass */ },
            label = { Text("Mass (kg)") },
            modifier = Modifier.fillMaxWidth()
        )

        Spacer(modifier = Modifier.height(8.dp))

        Row(modifier = Modifier.fillMaxWidth()) {
            OutlinedTextField(
                value = characterData.physics.drag.toString(),
                onValueChange = { /* Update drag */ },
                label = { Text("Drag") },
                modifier = Modifier.weight(1f)
            )
            Spacer(modifier = Modifier.width(8.dp))
            // Gravity toggle would go here
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Collider settings
        Text("Collider Settings:", style = MaterialTheme.typography.subtitle1)
        // Collider configuration would go here
    }
}

@Composable
private fun AIPanel(characterData: CharacterData) {
    Column(modifier = Modifier.fillMaxSize()) {
        Text("AI Configuration", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(16.dp))

        // AI properties
        Text("AI Properties:", style = MaterialTheme.typography.subtitle1)
        Spacer(modifier = Modifier.height(8.dp))

        OutlinedTextField(
            value = characterData.ai.perceptionRange.toString(),
            onValueChange = { /* Update perception range */ },
            label = { Text("Perception Range") },
            modifier = Modifier.fillMaxWidth()
        )

        Spacer(modifier = Modifier.height(8.dp))

        OutlinedTextField(
            value = characterData.ai.movementSpeed.toString(),
            onValueChange = { /* Update movement speed */ },
            label = { Text("Movement Speed") },
            modifier = Modifier.fillMaxWidth()
        )

        Spacer(modifier = Modifier.height(16.dp))

        // Behavior tree
        Text("Behavior Tree:", style = MaterialTheme.typography.subtitle1)
        // Behavior tree editor would go here
    }
}

@Composable
private fun AudioPanel(characterData: CharacterData) {
    Column(modifier = Modifier.fillMaxSize()) {
        Text("Audio Configuration", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(16.dp))

        // Audio properties
        Text("Voice Settings:", style = MaterialTheme.typography.subtitle1)
        Spacer(modifier = Modifier.height(8.dp))

        Row(modifier = Modifier.fillMaxWidth()) {
            OutlinedTextField(
                value = characterData.audio.voicePitch.toString(),
                onValueChange = { /* Update voice pitch */ },
                label = { Text("Voice Pitch") },
                modifier = Modifier.weight(1f)
            )
            Spacer(modifier = Modifier.width(8.dp))
            OutlinedTextField(
                value = characterData.audio.voiceVolume.toString(),
                onValueChange = { /* Update voice volume */ },
                label = { Text("Voice Volume") },
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Sound categories
        Text("Sound Categories:", style = MaterialTheme.typography.subtitle1)
        // Sound assignment interface would go here
    }
}

@Composable
private fun ExtrudeToolPanel(characterData: CharacterData, onDataChange: (CharacterData) -> Unit) {
    Column {
        Text("Extrude Tool", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(8.dp))

        var extrudeAmount by remember { mutableStateOf(0.1f) }
        var extrudeDirection by remember { mutableStateOf(ExtrudeDirection.NORMAL) }

        Slider(
            value = extrudeAmount,
            onValueChange = { extrudeAmount = it },
            valueRange = 0f..2f,
            modifier = Modifier.fillMaxWidth()
        )
        Text("Amount: ${"%.2f".format(extrudeAmount)}", style = MaterialTheme.typography.caption)

        Spacer(modifier = Modifier.height(8.dp))

        Text("Direction:", style = MaterialTheme.typography.subtitle2)
        ExtrudeDirection.values().forEach { direction ->
            Row(verticalAlignment = Alignment.CenterVertically) {
                RadioButton(
                    selected = extrudeDirection == direction,
                    onClick = { extrudeDirection = direction }
                )
                Text(direction.name, style = MaterialTheme.typography.body2)
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Button(onClick = { /* Apply extrusion */ }, modifier = Modifier.fillMaxWidth()) {
            Text("Apply Extrusion")
        }
    }
}

@Composable
private fun LoopCutToolPanel(characterData: CharacterData, onDataChange: (CharacterData) -> Unit) {
    Column {
        Text("Loop Cut Tool", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(8.dp))

        var numberOfCuts by remember { mutableStateOf(1) }
        var smooth by remember { mutableStateOf(true) }

        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("Number of cuts:", style = MaterialTheme.typography.body2)
            Spacer(modifier = Modifier.width(8.dp))
            OutlinedTextField(
                value = numberOfCuts.toString(),
                onValueChange = { numberOfCuts = it.toIntOrNull() ?: 1 },
                modifier = Modifier.width(60.dp)
            )
        }

        Spacer(modifier = Modifier.height(8.dp))

        Row(verticalAlignment = Alignment.CenterVertically) {
            Checkbox(checked = smooth, onCheckedChange = { smooth = it })
            Text("Smooth", style = MaterialTheme.typography.body2)
        }

        Spacer(modifier = Modifier.height(16.dp))

        Button(onClick = { /* Apply loop cut */ }, modifier = Modifier.fillMaxWidth()) {
            Text("Apply Loop Cut")
        }
    }
}

@Composable
private fun KnifeToolPanel(characterData: CharacterData, onDataChange: (CharacterData) -> Unit) {
    Column {
        Text("Knife Tool", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(8.dp))

        var angleSnap by remember { mutableStateOf(false) }
        var constrainToAxis by remember { mutableStateOf(false) }

        Row(verticalAlignment = Alignment.CenterVertically) {
            Checkbox(checked = angleSnap, onCheckedChange = { angleSnap = it })
            Text("Angle Snap", style = MaterialTheme.typography.body2)
        }

        Row(verticalAlignment = Alignment.CenterVertically) {
            Checkbox(checked = constrainToAxis, onCheckedChange = { constrainToAxis = it })
            Text("Constrain to Axis", style = MaterialTheme.typography.body2)
        }

        Spacer(modifier = Modifier.height(16.dp))

        Text("Click and drag to cut geometry", style = MaterialTheme.typography.caption)
    }
}

@Composable
private fun BrushToolPanel(characterData: CharacterData, onDataChange: (CharacterData) -> Unit) {
    Column {
        Text("Sculpt Brush", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(8.dp))

        var brushSize by remember { mutableStateOf(0.5f) }
        var brushStrength by remember { mutableStateOf(0.5f) }
        var brushMode by remember { mutableStateOf(BrushMode.ADD) }

        Text("Size:", style = MaterialTheme.typography.subtitle2)
        Slider(
            value = brushSize,
            onValueChange = { brushSize = it },
            valueRange = 0.1f..2f,
            modifier = Modifier.fillMaxWidth()
        )

        Text("Strength:", style = MaterialTheme.typography.subtitle2)
        Slider(
            value = brushStrength,
            onValueChange = { brushStrength = it },
            valueRange = 0.1f..1f,
            modifier = Modifier.fillMaxWidth()
        )

        Spacer(modifier = Modifier.height(8.dp))

        Text("Mode:", style = MaterialTheme.typography.subtitle2)
        BrushMode.values().forEach { mode ->
            Row(verticalAlignment = Alignment.CenterVertically) {
                RadioButton(
                    selected = brushMode == mode,
                    onClick = { brushMode = mode }
                )
                Text(mode.name, style = MaterialTheme.typography.body2)
            }
        }
    }
}

enum class ExtrudeDirection {
    NORMAL, X_AXIS, Y_AXIS, Z_AXIS
}

enum class BrushMode {
    ADD, SUBTRACT, SMOOTH, GRAB
}