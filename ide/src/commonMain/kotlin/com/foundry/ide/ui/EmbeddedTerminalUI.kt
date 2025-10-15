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
import androidx.compose.ui.text.*
import androidx.compose.ui.text.font.*
import androidx.compose.ui.unit.*
import androidx.compose.ui.window.*
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.*

/**
 * Embedded Terminal UI for Foundry IDE
 * Complete terminal emulator with command execution, history, and scripting support
 */
@Composable
fun EmbeddedTerminalWindow(onClose: () -> Unit) {
    var currentDirectory by remember { mutableStateOf(System.getProperty("user.dir") ?: "/") }
    var commandHistory by remember { mutableStateOf(listOf<String>()) }
    var commandIndex by remember { mutableStateOf(-1) }
    var terminalOutput by remember { mutableStateOf(listOf<TerminalLine>()) }
    var currentCommand by remember { mutableStateOf("") }
    var isRunning by remember { mutableStateOf(false) }
    var selectedTab by remember { mutableStateOf(TerminalTab.MAIN) }
    var terminalHistory by remember { mutableStateOf(mutableListOf<String>()) }
    var autoComplete by remember { mutableStateOf(true) }
    var terminalTheme by remember { mutableStateOf(TerminalTheme.DARK) }

    val scope = rememberCoroutineScope()
    val terminal = remember { EmbeddedTerminal() }

    // Initialize terminal
    LaunchedEffect(Unit) {
        terminal.initialize()
        addOutput("Foundry Terminal v1.0.0", TerminalLineType.SYSTEM)
        addOutput("Type 'help' for available commands", TerminalLineType.INFO)
        addOutput("$ ", TerminalLineType.PROMPT)
    }

    Window(
        onCloseRequest = onClose,
        title = "Embedded Terminal - $currentDirectory",
        state = rememberWindowState(width = 1000.dp, height = 600.dp)
    ) {
        MaterialTheme {
            Column(modifier = Modifier.fillMaxSize()) {
                // Toolbar
                TerminalToolbar(
                    currentDirectory = currentDirectory,
                    isRunning = isRunning,
                    onClear = { terminalOutput = emptyList() },
                    onStop = { terminal.stopCurrentCommand() }
                )

                // Tabs
                TerminalTabs(selectedTab, onTabChange = { selectedTab = it })

                // Terminal content
                when (selectedTab) {
                    TerminalTab.MAIN -> MainTerminalTab(
                        terminalOutput = terminalOutput,
                        currentCommand = currentCommand,
                        onCommandChange = { currentCommand = it },
                        onCommandExecute = { executeCommand(it) },
                        onHistoryUp = { navigateHistory(-1) },
                        onHistoryDown = { navigateHistory(1) },
                        autoComplete = autoComplete,
                        terminalTheme = terminalTheme
                    )
                    TerminalTab.SCRIPTING -> ScriptingTab()
                    TerminalTab.HISTORY -> HistoryTab(commandHistory)
                    TerminalTab.SETTINGS -> SettingsTab(
                        autoComplete = autoComplete,
                        onAutoCompleteChange = { autoComplete = it },
                        terminalTheme = terminalTheme,
                        onThemeChange = { terminalTheme = it }
                    )
                }
            }
        }
    }

    fun addOutput(text: String, type: TerminalLineType = TerminalLineType.OUTPUT) {
        terminalOutput = terminalOutput + TerminalLine(text, type)
    }

    fun executeCommand(command: String) {
        if (command.isBlank()) return

        // Add command to history
        commandHistory = commandHistory + command
        commandIndex = -1

        // Display command
        addOutput("$ $command", TerminalLineType.COMMAND)

        // Execute command
        scope.launch {
            isRunning = true
            try {
                val result = terminal.executeCommand(command, currentDirectory)
                result.output.forEach { line ->
                    addOutput(line, if (result.success) TerminalLineType.OUTPUT else TerminalLineType.ERROR)
                }

                if (result.newDirectory != null) {
                    currentDirectory = result.newDirectory
                }
            } catch (e: Exception) {
                addOutput("Error: ${e.message}", TerminalLineType.ERROR)
            } finally {
                isRunning = false
                addOutput("$ ", TerminalLineType.PROMPT)
            }
        }

        currentCommand = ""
    }

    fun navigateHistory(direction: Int) {
        if (commandHistory.isEmpty()) return

        commandIndex = (commandIndex + direction).coerceIn(-1, commandHistory.size - 1)
        currentCommand = if (commandIndex >= 0) commandHistory[commandIndex] else ""
    }
}

enum class TerminalTab {
    MAIN, SCRIPTING, HISTORY, SETTINGS
}

enum class TerminalTheme {
    DARK, LIGHT, SOLARIZED_DARK, SOLARIZED_LIGHT
}

enum class TerminalLineType {
    OUTPUT, ERROR, COMMAND, SYSTEM, INFO, PROMPT
}

data class TerminalLine(
    val text: String,
    val type: TerminalLineType,
    val timestamp: Long = System.currentTimeMillis()
)

@Composable
private fun TerminalToolbar(
    currentDirectory: String,
    isRunning: Boolean,
    onClear: () -> Unit,
    onStop: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        // Directory display
        Row(verticalAlignment = Alignment.CenterVertically) {
            Icon(Icons.Default.Folder, "Directory")
            Spacer(modifier = Modifier.width(8.dp))
            Text(
                text = currentDirectory,
                style = MaterialTheme.typography.body2,
                maxLines = 1,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.width(16.dp))

        // Status indicator
        if (isRunning) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                CircularProgressIndicator(modifier = Modifier.size(16.dp))
                Spacer(modifier = Modifier.width(8.dp))
                Text("Running...", style = MaterialTheme.typography.caption)
            }
        }

        Spacer(modifier = Modifier.width(16.dp))

        // Action buttons
        IconButton(onClick = onClear) {
            Icon(Icons.Default.Clear, "Clear")
        }

        if (isRunning) {
            IconButton(onClick = onStop) {
                Icon(Icons.Default.Stop, "Stop")
            }
        }
    }
}

@Composable
private fun TerminalTabs(selectedTab: TerminalTab, onTabChange: (TerminalTab) -> Unit) {
    TabRow(selectedTabIndex = selectedTab.ordinal) {
        TerminalTab.values().forEach { tab ->
            Tab(
                selected = selectedTab == tab,
                onClick = { onTabChange(tab) },
                text = { Text(tab.name) },
                icon = {
                    when (tab) {
                        TerminalTab.MAIN -> Icon(Icons.Default.Terminal, tab.name)
                        TerminalTab.SCRIPTING -> Icon(Icons.Default.Code, tab.name)
                        TerminalTab.HISTORY -> Icon(Icons.Default.History, tab.name)
                        TerminalTab.SETTINGS -> Icon(Icons.Default.Settings, tab.name)
                    }
                }
            )
        }
    }
}

@Composable
private fun MainTerminalTab(
    terminalOutput: List<TerminalLine>,
    currentCommand: String,
    onCommandChange: (String) -> Unit,
    onCommandExecute: (String) -> Unit,
    onHistoryUp: () -> Unit,
    onHistoryDown: () -> Unit,
    autoComplete: Boolean,
    terminalTheme: TerminalTheme
) {
    Column(modifier = Modifier.fillMaxSize()) {
        // Terminal output with theme
        val backgroundColor = when (terminalTheme) {
            TerminalTheme.DARK -> Color.Black
            TerminalTheme.LIGHT -> Color.White
            TerminalTheme.SOLARIZED_DARK -> Color(0xFF002B36)
            TerminalTheme.SOLARIZED_LIGHT -> Color(0xFFFDF6E3)
        }

        LazyColumn(
            modifier = Modifier
                .weight(1f)
                .fillMaxWidth()
                .background(backgroundColor)
                .padding(8.dp)
        ) {
            items(terminalOutput) { line ->
                TerminalLineView(line, terminalTheme)
            }
        }

        // Enhanced command input with autocomplete
        Column(modifier = Modifier.fillMaxWidth()) {
            // Autocomplete suggestions
            if (autoComplete && currentCommand.isNotBlank()) {
                val suggestions = getCommandSuggestions(currentCommand)
                if (suggestions.isNotEmpty()) {
                    LazyRow(
                        modifier = Modifier
                            .fillMaxWidth()
                            .background(Color.DarkGray.copy(alpha = 0.2f))
                            .padding(horizontal = 8.dp, vertical = 4.dp)
                    ) {
                        items(suggestions.take(5)) { suggestion ->
                            Chip(
                                onClick = { onCommandChange(suggestion) },
                                modifier = Modifier.padding(end = 4.dp)
                            ) {
                                Text(suggestion, style = MaterialTheme.typography.caption)
                            }
                        }
                    }
                }
            }

            // Command input
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(
                        when (terminalTheme) {
                            TerminalTheme.DARK -> Color.DarkGray.copy(alpha = 0.1f)
                            TerminalTheme.LIGHT -> Color.LightGray.copy(alpha = 0.1f)
                            TerminalTheme.SOLARIZED_DARK -> Color(0xFF073642)
                            TerminalTheme.SOLARIZED_LIGHT -> Color(0xFFEEE8D5)
                        }
                    )
                    .padding(8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    "$ ",
                    style = MaterialTheme.typography.body1.copy(
                        fontFamily = FontFamily.Monospace,
                        color = when (terminalTheme) {
                            TerminalTheme.DARK, TerminalTheme.SOLARIZED_DARK -> Color.Green
                            TerminalTheme.LIGHT, TerminalTheme.SOLARIZED_LIGHT -> Color(0xFF859900)
                        }
                    )
                )

                TextField(
                    value = currentCommand,
                    onValueChange = onCommandChange,
                    modifier = Modifier.weight(1f),
                    textStyle = TextStyle(
                        fontFamily = FontFamily.Monospace,
                        color = when (terminalTheme) {
                            TerminalTheme.DARK, TerminalTheme.SOLARIZED_DARK -> Color.White
                            TerminalTheme.LIGHT -> Color.Black
                            TerminalTheme.SOLARIZED_LIGHT -> Color(0xFF586E75)
                        }
                    ),
                    colors = TextFieldDefaults.textFieldColors(
                        backgroundColor = Color.Transparent,
                        focusedIndicatorColor = Color.Transparent,
                        unfocusedIndicatorColor = Color.Transparent
                    ),
                    keyboardActions = androidx.compose.foundation.text.KeyboardActions(
                        onEnter = { onCommandExecute(currentCommand) }
                    ),
                    keyboardOptions = androidx.compose.foundation.text.KeyboardOptions(
                        keyboardType = androidx.compose.ui.text.input.KeyboardType.Text,
                        imeAction = androidx.compose.ui.text.input.ImeAction.Send
                    )
                )

                Spacer(modifier = Modifier.width(8.dp))

                IconButton(onClick = { onCommandExecute(currentCommand) }) {
                    Icon(Icons.Default.Send, "Execute")
                }
            }
        }
    }
}

@Composable
private fun TerminalLineView(line: TerminalLine, theme: TerminalTheme = TerminalTheme.DARK) {
    val color = when (theme) {
        TerminalTheme.DARK -> when (line.type) {
            TerminalLineType.OUTPUT -> Color.White
            TerminalLineType.ERROR -> Color.Red
            TerminalLineType.COMMAND -> Color.Yellow
            TerminalLineType.SYSTEM -> Color.Cyan
            TerminalLineType.INFO -> Color.Green
            TerminalLineType.PROMPT -> Color.Green
        }
        TerminalTheme.LIGHT -> when (line.type) {
            TerminalLineType.OUTPUT -> Color.Black
            TerminalLineType.ERROR -> Color(0xFFD32F2F)
            TerminalLineType.COMMAND -> Color(0xFFF57C00)
            TerminalLineType.SYSTEM -> Color(0xFF1976D2)
            TerminalLineType.INFO -> Color(0xFF388E3C)
            TerminalLineType.PROMPT -> Color(0xFF388E3C)
        }
        TerminalTheme.SOLARIZED_DARK -> when (line.type) {
            TerminalLineType.OUTPUT -> Color(0xFF93A1A1)
            TerminalLineType.ERROR -> Color(0xFFDC322F)
            TerminalLineType.COMMAND -> Color(0xFFB58900)
            TerminalLineType.SYSTEM -> Color(0xFF268BD2)
            TerminalLineType.INFO -> Color(0xFF859900)
            TerminalLineType.PROMPT -> Color(0xFF859900)
        }
        TerminalTheme.SOLARIZED_LIGHT -> when (line.type) {
            TerminalLineType.OUTPUT -> Color(0xFF586E75)
            TerminalLineType.ERROR -> Color(0xFFDC322F)
            TerminalLineType.COMMAND -> Color(0xFFB58900)
            TerminalLineType.SYSTEM -> Color(0xFF268BD2)
            TerminalLineType.INFO -> Color(0xFF859900)
            TerminalLineType.PROMPT -> Color(0xFF859900)
        }
    }

    Text(
        text = line.text,
        style = MaterialTheme.typography.body2.copy(
            fontFamily = FontFamily.Monospace,
            color = color
        ),
        modifier = Modifier.fillMaxWidth()
    )
}

@Composable
private fun ScriptingTab() {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Terminal Scripting", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        // Script editor
        var scriptContent by remember { mutableStateOf("") }

        OutlinedTextField(
            value = scriptContent,
            onValueChange = { scriptContent = it },
            label = { Text("Script Content") },
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f),
            textStyle = TextStyle(fontFamily = FontFamily.Monospace)
        )

        Spacer(modifier = Modifier.height(16.dp))

        Row(modifier = Modifier.fillMaxWidth()) {
            Button(onClick = { /* Run script */ }) {
                Icon(Icons.Default.PlayArrow, "Run")
                Spacer(modifier = Modifier.width(8.dp))
                Text("Run Script")
            }

            Spacer(modifier = Modifier.width(8.dp))

            OutlinedButton(onClick = { /* Save script */ }) {
                Icon(Icons.Default.Save, "Save")
                Spacer(modifier = Modifier.width(8.dp))
                Text("Save Script")
            }

            Spacer(modifier = Modifier.width(8.dp))

            OutlinedButton(onClick = { /* Load script */ }) {
                Icon(Icons.Default.FolderOpen, "Load")
                Spacer(modifier = Modifier.width(8.dp))
                Text("Load Script")
            }
        }
    }
}

@Composable
private fun HistoryTab(commandHistory: List<String>) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Command History", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        LazyColumn(modifier = Modifier.fillMaxSize()) {
            items(commandHistory.asReversed()) { command ->
                Card(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(4.dp)
                        .clickable { /* Copy to clipboard */ }
                ) {
                    Row(
                        modifier = Modifier.padding(12.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Icon(
                            Icons.Default.History,
                            "Command",
                            modifier = Modifier.size(16.dp)
                        )
                        Spacer(modifier = Modifier.width(12.dp))
                        Text(
                            command,
                            style = MaterialTheme.typography.body2.copy(fontFamily = FontFamily.Monospace),
                            modifier = Modifier.weight(1f)
                        )
                        IconButton(onClick = { /* Execute command */ }) {
                            Icon(Icons.Default.Replay, "Execute")
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun SettingsTab(
    autoComplete: Boolean,
    onAutoCompleteChange: (Boolean) -> Unit,
    terminalTheme: TerminalTheme,
    onThemeChange: (TerminalTheme) -> Unit
) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Terminal Settings", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        // Autocomplete setting
        Row(verticalAlignment = Alignment.CenterVertically) {
            Checkbox(checked = autoComplete, onCheckedChange = onAutoCompleteChange)
            Spacer(modifier = Modifier.width(8.dp))
            Text("Enable autocomplete suggestions", style = MaterialTheme.typography.body1)
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Theme selection
        Text("Terminal Theme:", style = MaterialTheme.typography.subtitle1)
        Spacer(modifier = Modifier.height(8.dp))

        TerminalTheme.values().forEach { theme ->
            Row(verticalAlignment = Alignment.CenterVertically) {
                RadioButton(
                    selected = terminalTheme == theme,
                    onClick = { onThemeChange(theme) }
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(theme.name.replace("_", " "), style = MaterialTheme.typography.body1)
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Font size setting
        Text("Font Size:", style = MaterialTheme.typography.subtitle1)
        // Font size slider would go here

        Spacer(modifier = Modifier.height(16.dp))

        // Buffer size setting
        Text("Buffer Size:", style = MaterialTheme.typography.subtitle1)
        // Buffer size setting would go here
    }
}

// Terminal backend implementation
class EmbeddedTerminal {
    private var currentProcess: Process? = null
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    fun initialize() {
        // Initialize terminal environment
    }

    suspend fun executeCommand(command: String, workingDirectory: String): CommandResult {
        return withContext(Dispatchers.IO) {
            try {
                val parts = command.split(" ")
                val processBuilder = ProcessBuilder(parts)
                    .directory(java.io.File(workingDirectory))
                    .redirectErrorStream(true)

                currentProcess = processBuilder.start()

                val output = mutableListOf<String>()
                val reader = currentProcess!!.inputStream.bufferedReader()

                reader.useLines { lines ->
                    lines.forEach { line ->
                        output.add(line)
                    }
                }

                val exitCode = currentProcess!!.waitFor()
                currentProcess = null

                val newDirectory = if (command.startsWith("cd ")) {
                    val newDir = command.substringAfter("cd ").trim()
                    if (newDir.isNotBlank()) {
                        val targetDir = java.io.File(workingDirectory, newDir).canonicalPath
                        if (java.io.File(targetDir).exists()) targetDir else workingDirectory
                    } else workingDirectory
                } else workingDirectory

                CommandResult(
                    success = exitCode == 0,
                    output = output,
                    newDirectory = newDirectory
                )
            } catch (e: Exception) {
                CommandResult(
                    success = false,
                    output = listOf("Error: ${e.message}"),
                    newDirectory = workingDirectory
                )
            }
        }
    }

    fun stopCurrentCommand() {
        currentProcess?.destroyForcibly()
        currentProcess = null
    }

    fun dispose() {
        stopCurrentCommand()
        scope.cancel()
    }
}

data class CommandResult(
    val success: Boolean,
    val output: List<String>,
    val newDirectory: String? = null
)

// Command suggestions for autocomplete
private fun getCommandSuggestions(prefix: String): List<String> {
    val commonCommands = listOf(
        "ls", "cd", "pwd", "mkdir", "rmdir", "cp", "mv", "rm", "cat", "grep",
        "find", "chmod", "chown", "ps", "kill", "top", "df", "du", "free",
        "git status", "git add", "git commit", "git push", "git pull",
        "npm install", "npm run", "gradle build", "cmake", "make",
        "python", "java", "javac", "kotlin", "cargo", "go run"
    )

    return commonCommands.filter { it.startsWith(prefix, ignoreCase = true) }
}