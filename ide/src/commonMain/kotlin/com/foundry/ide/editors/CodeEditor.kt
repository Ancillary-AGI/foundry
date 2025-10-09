package com.foundry.ide.editors

import com.foundry.ide.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths

/**
 * Code editor integration for Foundry IDE
 * Provides syntax highlighting, IntelliSense, and debugging features
 */
@Serializable
data class EditorDocument(
    val path: String,
    val content: String,
    val language: String,
    val isDirty: Boolean = false,
    val cursorPosition: CursorPosition = CursorPosition(0, 0),
    val selection: TextSelection? = null,
    val breakpoints: List<Int> = emptyList()
)

@Serializable
data class CursorPosition(
    val line: Int,
    val column: Int
)

@Serializable
data class TextSelection(
    val startLine: Int,
    val startColumn: Int,
    val endLine: Int,
    val endColumn: Int
)

@Serializable
data class EditorSettings(
    val fontSize: Int = 14,
    val tabSize: Int = 4,
    val insertSpaces: Boolean = true,
    val wordWrap: Boolean = false,
    val minimap: Boolean = true,
    val lineNumbers: Boolean = true,
    val folding: Boolean = true,
    val autoSave: Boolean = true,
    val formatOnSave: Boolean = true
)

class CodeEditor {
    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())
    private val openDocuments = mutableMapOf<String, EditorDocument>()
    private val documentWatchers = mutableMapOf<String, Job>()
    private val languageMap = mapOf(
        "kt" to "kotlin",
        "java" to "java",
        "cpp" to "cpp",
        "cc" to "cpp",
        "cxx" to "cpp",
        "h" to "c",
        "hpp" to "cpp",
        "glsl" to "glsl",
        "vert" to "glsl",
        "frag" to "glsl",
        "json" to "json",
        "xml" to "xml",
        "md" to "markdown"
    )

    private var settings = EditorSettings()

    /**
     * Open document for editing
     */
    fun openDocument(filePath: String): EditorDocument? {
        return try {
            // Validate file path to prevent path traversal
            val normalizedPath = Paths.get(filePath).normalize().toAbsolutePath()
            if (!isValidFilePath(normalizedPath.toString())) {
                return null
            }
            
            val file = normalizedPath.toFile()
            if (!file.exists() || !isAllowedFileExtension(file.extension)) {
                return null
            }

            val content = Files.readString(file.toPath())
            val language = detectLanguage(file)

            val document = EditorDocument(
                path = filePath,
                content = content,
                language = language,
                breakpoints = loadBreakpoints(filePath)
            )

            openDocuments[filePath] = document

            // Start watching file for external changes
            startWatchingDocument(filePath)

            document
        } catch (e: Exception) {
            println("Failed to open document: ${e.message}")
            null
        }
    }

    /**
     * Close document
     */
    fun closeDocument(filePath: String): Boolean {
        return try {
            // Stop watching
            stopWatchingDocument(filePath)

            // Save if dirty
            val document = openDocuments[filePath]
            if (document?.isDirty == true) {
                saveDocument(filePath)
            }

            openDocuments.remove(filePath)
            true
        } catch (e: Exception) {
            println("Failed to close document: ${e.message}")
            false
        }
    }

    /**
     * Update document content
     */
    fun updateDocument(filePath: String, content: String, cursorPosition: CursorPosition? = null): Boolean {
        return try {
            val document = openDocuments[filePath] ?: return false

            val updatedDocument = document.copy(
                content = content,
                isDirty = document.content != content,
                cursorPosition = cursorPosition ?: document.cursorPosition
            )

            openDocuments[filePath] = updatedDocument
            true
        } catch (e: Exception) {
            println("Failed to update document: ${e.message}")
            false
        }
    }

    /**
     * Save document
     */
    fun saveDocument(filePath: String): Boolean {
        return try {
            val document = openDocuments[filePath] ?: return false
            
            // Validate file path to prevent path traversal
            val normalizedPath = Paths.get(filePath).normalize().toAbsolutePath()
            if (!isValidFilePath(normalizedPath.toString())) {
                return false
            }

            Files.writeString(normalizedPath, document.content)

            val updatedDocument = document.copy(isDirty = false)
            openDocuments[filePath] = updatedDocument

            // Save breakpoints
            saveBreakpoints(filePath, document.breakpoints)

            true
        } catch (e: Exception) {
            println("Failed to save document: ${e.message}")
            false
        }
    }

    /**
     * Save all open documents
     */
    fun saveAllDocuments(): Boolean {
        return try {
            openDocuments.keys.forEach { filePath ->
                saveDocument(filePath)
            }
            true
        } catch (e: Exception) {
            println("Failed to save all documents: ${e.message}")
            false
        }
    }

    /**
     * Get document content
     */
    fun getDocument(filePath: String): EditorDocument? {
        return openDocuments[filePath]
    }

    /**
     * Get all open documents
     */
    fun getOpenDocuments(): List<EditorDocument> {
        return openDocuments.values.toList()
    }

    /**
     * Add breakpoint
     */
    fun addBreakpoint(filePath: String, line: Int): Boolean {
        return try {
            val document = openDocuments[filePath] ?: return false

            if (line in document.breakpoints) return true

            val updatedBreakpoints = document.breakpoints + line
            val updatedDocument = document.copy(breakpoints = updatedBreakpoints)
            openDocuments[filePath] = updatedDocument

            // Save breakpoints
            saveBreakpoints(filePath, updatedBreakpoints)

            true
        } catch (e: Exception) {
            println("Failed to add breakpoint: ${e.message}")
            false
        }
    }

    /**
     * Remove breakpoint
     */
    fun removeBreakpoint(filePath: String, line: Int): Boolean {
        return try {
            val document = openDocuments[filePath] ?: return false

            if (line !in document.breakpoints) return true

            val updatedBreakpoints = document.breakpoints - line
            val updatedDocument = document.copy(breakpoints = updatedBreakpoints)
            openDocuments[filePath] = updatedDocument

            // Save breakpoints
            saveBreakpoints(filePath, updatedBreakpoints)

            true
        } catch (e: Exception) {
            println("Failed to remove breakpoint: ${e.message}")
            false
        }
    }

    /**
     * Toggle breakpoint
     */
    fun toggleBreakpoint(filePath: String, line: Int): Boolean {
        val document = openDocuments[filePath] ?: return false

        return if (line in document.breakpoints) {
            removeBreakpoint(filePath, line)
        } else {
            addBreakpoint(filePath, line)
        }
    }

    /**
     * Format document
     */
    fun formatDocument(filePath: String): Boolean {
        return try {
            val document = openDocuments[filePath] ?: return false

            val formattedContent = formatCode(document.content, document.language)
            updateDocument(filePath, formattedContent)

            true
        } catch (e: Exception) {
            println("Failed to format document: ${e.message}")
            false
        }
    }

    /**
     * Find and replace in document
     */
    fun findReplace(
        filePath: String,
        findText: String,
        replaceText: String,
        caseSensitive: Boolean = false,
        regex: Boolean = false
    ): Int {
        return try {
            val document = openDocuments[filePath] ?: return 0

            val content = document.content
            val searchText = if (caseSensitive) findText else findText.lowercase()
            val searchContent = if (caseSensitive) content else content.lowercase()

            var replacements = 0
            val newContent = if (regex) {
                // Simple regex replacement (in a real implementation, use proper regex)
                content.replace(findText, replaceText)
            } else {
                // Simple string replacement
                var result = content
                var index = searchContent.indexOf(searchText)
                while (index != -1) {
                    result = result.replaceRange(index, index + findText.length, replaceText)
                    replacements++
                    index = searchContent.indexOf(searchText, index + replaceText.length)
                }
                result
            }

            if (replacements > 0) {
                updateDocument(filePath, newContent)
            }

            replacements
        } catch (e: Exception) {
            println("Failed to find and replace: ${e.message}")
            0
        }
    }

    /**
     * Get editor settings
     */
    fun getSettings(): EditorSettings {
        return settings
    }

    /**
     * Update editor settings
     */
    fun updateSettings(newSettings: EditorSettings) {
        settings = newSettings
        saveSettings()
    }

    /**
     * Detect language from file extension
     */
    private fun detectLanguage(file: File): String {
        val extension = file.extension.lowercase()
        return languageMap[extension] ?: "plaintext"
    }

    /**
     * Format code based on language
     */
    private fun formatCode(content: String, language: String): String {
        return when (language) {
            "kotlin", "java" -> formatKotlinJava(content)
            "cpp", "c" -> formatCpp(content)
            "json" -> formatJson(content)
            else -> content
        }
    }

    /**
     * Format Kotlin/Java code
     */
    private fun formatKotlinJava(content: String): String {
        // Simple formatting (in a real implementation, use kotlin formatter)
        return content.split("\n").map { line ->
            line.trimStart()
        }.joinToString("\n")
    }

    /**
     * Format C++ code
     */
    private fun formatCpp(content: String): String {
        // Simple formatting (in a real implementation, use clang-format)
        return content.split("\n").map { line ->
            line.trimStart()
        }.joinToString("\n")
    }

    /**
     * Format JSON
     */
    private fun formatJson(content: String): String {
        return try {
            val jsonElement = kotlinx.serialization.json.Json.parseToJsonElement(content)
            kotlinx.serialization.json.Json.encodeToString(
                kotlinx.serialization.json.JsonElement.serializer(),
                jsonElement
            )
        } catch (e: Exception) {
            content // Return original if formatting fails
        }
    }

    /**
     * Start watching document for external changes
     */
    private fun startWatchingDocument(filePath: String) {
        val watcherJob = scope.launch {
            val file = File(filePath)
            var lastModified = file.lastModified()

            while (isActive) {
                try {
                    delay(1000) // Check every second

                    if (file.lastModified() > lastModified) {
                        lastModified = file.lastModified()

                        // Reload document if it wasn't modified internally
                        val document = openDocuments[filePath]
                        if (document != null && !document.isDirty) {
                            val content = Files.readString(file.toPath())
                            val updatedDocument = document.copy(content = content)
                            openDocuments[filePath] = updatedDocument
                        }
                    }
                } catch (e: Exception) {
                    break
                }
            }
        }

        documentWatchers[filePath] = watcherJob
    }

    /**
     * Stop watching document
     */
    private fun stopWatchingDocument(filePath: String) {
        documentWatchers[filePath]?.cancel()
        documentWatchers.remove(filePath)
    }

    /**
     * Load breakpoints from file
     */
    private fun loadBreakpoints(filePath: String): List<Int> {
        return try {
            val breakpointsFile = getBreakpointsFile(filePath)
            if (!breakpointsFile.exists()) return emptyList()

            breakpointsFile.readLines().mapNotNull { line ->
                line.trim().toIntOrNull()
            }
        } catch (e: Exception) {
            emptyList()
        }
    }

    /**
     * Save breakpoints to file
     */
    private fun saveBreakpoints(filePath: String, breakpoints: List<Int>) {
        try {
            val breakpointsFile = getBreakpointsFile(filePath)
            breakpointsFile.parentFile.mkdirs()

            val content = breakpoints.joinToString("\n")
            breakpointsFile.writeText(content)
        } catch (e: Exception) {
            println("Failed to save breakpoints: ${e.message}")
        }
    }

    /**
     * Get breakpoints file path
     */
    private fun getBreakpointsFile(filePath: String): File {
        val normalizedPath = Paths.get(filePath).normalize().toAbsolutePath()
        val file = normalizedPath.toFile()
        val breakpointsDir = File(file.parentFile, ".foundry")
        return File(breakpointsDir, "${file.name}.breakpoints")
    }

    /**
     * Save editor settings
     */
    private fun saveSettings() {
        try {
            val settingsDir = File(System.getProperty("user.home"), ".foundry/ide")
            settingsDir.mkdirs()

            val settingsFile = settingsDir.resolve("editor_settings.json")
            val settingsJson = kotlinx.serialization.json.Json.encodeToString(
                EditorSettings.serializer(),
                settings
            )
            settingsFile.writeText(settingsJson)
        } catch (e: Exception) {
            println("Failed to save editor settings: ${e.message}")
        }
    }

    /**
     * Load editor settings
     */
    fun loadSettings(): EditorSettings {
        return try {
            val settingsFile = File(System.getProperty("user.home"), ".foundry/ide/editor_settings.json")
            if (!settingsFile.exists()) return EditorSettings()

            val settingsJson = settingsFile.readText()
            kotlinx.serialization.json.Json.decodeFromString(
                EditorSettings.serializer(),
                settingsJson
            )
        } catch (e: Exception) {
            println("Failed to load editor settings: ${e.message}")
            EditorSettings()
        }
    }

    /**
     * Get syntax errors for document
     */
    fun getSyntaxErrors(filePath: String): List<SyntaxError> {
        val document = openDocuments[filePath] ?: return emptyList()

        return when (document.language) {
            "kotlin" -> checkKotlinSyntax(document.content)
            "cpp", "c" -> checkCppSyntax(document.content)
            "json" -> checkJsonSyntax(document.content)
            else -> emptyList()
        }
    }

    /**
     * Check Kotlin syntax
     */
    private fun checkKotlinSyntax(content: String): List<SyntaxError> {
        val errors = mutableListOf<SyntaxError>()

        // Simple syntax checking (in a real implementation, use Kotlin compiler)
        val lines = content.lines()

        lines.forEachIndexed { index, line ->
            // Check for unmatched braces
            val openBraces = line.count { it == '{' }
            val closeBraces = line.count { it == '}' }

            if (openBraces > closeBraces) {
                errors.add(SyntaxError(
                    line = index + 1,
                    column = line.length + 1,
                    message = "Unmatched opening brace",
                    severity = "error"
                ))
            }

            // Check for missing semicolons in some contexts
            if (line.contains("class ") && !line.endsWith("{")) {
                errors.add(SyntaxError(
                    line = index + 1,
                    column = line.length + 1,
                    message = "Missing opening brace for class definition",
                    severity = "error"
                ))
            }
        }

        return errors
    }

    /**
     * Check C++ syntax
     */
    private fun checkCppSyntax(content: String): List<SyntaxError> {
        val errors = mutableListOf<SyntaxError>()

        // Simple syntax checking (in a real implementation, use clang)
        val lines = content.lines()

        lines.forEachIndexed { index, line ->
            // Check for unmatched braces
            val openBraces = line.count { it == '{' }
            val closeBraces = line.count { it == '}' }

            if (openBraces > closeBraces) {
                errors.add(SyntaxError(
                    line = index + 1,
                    column = line.length + 1,
                    message = "Unmatched opening brace",
                    severity = "error"
                ))
            }
        }

        return errors
    }

    /**
     * Check JSON syntax
     */
    private fun checkJsonSyntax(content: String): List<SyntaxError> {
        return try {
            kotlinx.serialization.json.Json.parseToJsonElement(content)
            emptyList()
        } catch (e: Exception) {
            listOf(SyntaxError(
                line = 1,
                column = 1,
                message = "Invalid JSON: ${e.message}",
                severity = "error"
            ))
        }
    }

    /**
     * Get code completion suggestions
     */
    fun getCompletions(filePath: String, line: Int, column: Int): List<CompletionItem> {
        val document = openDocuments[filePath] ?: return emptyList()

        return when (document.language) {
            "kotlin" -> getKotlinCompletions(document.content, line, column)
            "cpp" -> getCppCompletions(document.content, line, column)
            else -> emptyList()
        }
    }

    /**
     * Get Kotlin code completions
     */
    private fun getKotlinCompletions(content: String, line: Int, column: Int): List<CompletionItem> {
        val completions = mutableListOf<CompletionItem>()

        // Common Kotlin keywords
        val keywords = listOf(
            "fun", "val", "var", "class", "interface", "object", "package", "import",
            "if", "else", "when", "for", "while", "return", "break", "continue",
            "try", "catch", "finally", "throw", "suspend", "coroutine"
        )

        keywords.forEach { keyword ->
            completions.add(CompletionItem(
                label = keyword,
                kind = CompletionKind.KEYWORD,
                detail = "Kotlin keyword"
            ))
        }

        // Common types
        val types = listOf(
            "String", "Int", "Float", "Double", "Boolean", "List", "Map", "Set",
            "Array", "MutableList", "MutableMap", "MutableSet"
        )

        types.forEach { type ->
            completions.add(CompletionItem(
                label = type,
                kind = CompletionKind.TYPE,
                detail = "Kotlin type"
            ))
        }

        return completions
    }

    /**
     * Get C++ code completions
     */
    private fun getCppCompletions(content: String, line: Int, column: Int): List<CompletionItem> {
        val completions = mutableListOf<CompletionItem>()

        // Common C++ keywords
        val keywords = listOf(
            "class", "struct", "enum", "namespace", "public", "private", "protected",
            "virtual", "override", "const", "static", "extern", "inline", "template"
        )

        keywords.forEach { keyword ->
            completions.add(CompletionItem(
                label = keyword,
                kind = CompletionKind.KEYWORD,
                detail = "C++ keyword"
            ))
        }

        return completions
    }

    /**
     * Validate file path to prevent path traversal
     */
    private fun isValidFilePath(filePath: String): Boolean {
        val normalizedPath = Paths.get(filePath).normalize().toAbsolutePath().toString()
        return !normalizedPath.contains("..") && 
               !normalizedPath.startsWith("/etc") &&
               !normalizedPath.startsWith("/proc") &&
               !normalizedPath.startsWith("/sys")
    }
    
    /**
     * Check if file extension is allowed
     */
    private fun isAllowedFileExtension(extension: String): Boolean {
        val allowedExtensions = setOf(
            "kt", "java", "cpp", "cc", "cxx", "h", "hpp", "c",
            "glsl", "vert", "frag", "json", "xml", "md", "txt",
            "gradle", "kts", "properties", "yml", "yaml"
        )
        return extension.lowercase() in allowedExtensions
    }

    /**
     * Shutdown editor
     */
    fun shutdown() {
        // Stop all watchers
        documentWatchers.values.forEach { it.cancel() }
        documentWatchers.clear()

        // Save all documents
        saveAllDocuments()

        openDocuments.clear()
    }
}

/**
 * Syntax error data class
 */
@Serializable
data class SyntaxError(
    val line: Int,
    val column: Int,
    val message: String,
    val severity: String
)

/**
 * Code completion item
 */
@Serializable
data class CompletionItem(
    val label: String,
    val kind: CompletionKind,
    val detail: String,
    val documentation: String? = null,
    val insertText: String? = null
)

enum class CompletionKind {
    KEYWORD, TYPE, FUNCTION, VARIABLE, PROPERTY, METHOD, CLASS, INTERFACE
}
