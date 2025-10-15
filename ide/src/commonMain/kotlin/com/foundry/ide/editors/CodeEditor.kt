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
     * Get syntax errors and linting issues for document
     */
    fun getSyntaxErrors(filePath: String): List<SyntaxError> {
        val document = openDocuments[filePath] ?: return emptyList()

        val errors = mutableListOf<SyntaxError>()

        // Basic syntax checking
        errors.addAll(when (document.language) {
            "kotlin" -> checkKotlinSyntax(document.content)
            "cpp", "c" -> checkCppSyntax(document.content)
            "java" -> checkJavaSyntax(document.content)
            "json" -> checkJsonSyntax(document.content)
            "glsl" -> checkGLSLSyntax(document.content)
            else -> emptyList()
        })

        // Advanced linting
        errors.addAll(performLinting(document))

        return errors
    }

    /**
     * Perform advanced linting on the document
     */
    private fun performLinting(document: EditorDocument): List<SyntaxError> {
        val errors = mutableListOf<SyntaxError>()
        val lines = document.content.lines()

        lines.forEachIndexed { index, line ->
            val lineNumber = index + 1

            // Check for common issues
            when (document.language) {
                "kotlin" -> {
                    // Check for unused imports (simplified)
                    if (line.startsWith("import ") && line.contains(".*")) {
                        errors.add(SyntaxError(
                            line = lineNumber,
                            column = 1,
                            message = "Wildcard import detected - consider importing specific classes",
                            severity = "warning"
                        ))
                    }

                    // Check for long lines
                    if (line.length > 120) {
                        errors.add(SyntaxError(
                            line = lineNumber,
                            column = 121,
                            message = "Line too long (max 120 characters)",
                            severity = "warning"
                        ))
                    }

                    // Check for TODO comments
                    if (line.contains("TODO") || line.contains("FIXME")) {
                        errors.add(SyntaxError(
                            line = lineNumber,
                            column = line.indexOf("TODO") + 1,
                            message = "TODO/FIXME comment found",
                            severity = "info"
                        ))
                    }
                }
                "cpp" -> {
                    // Check for missing includes
                    if (line.startsWith("#include") && !line.contains("<") && !line.contains("\"")) {
                        errors.add(SyntaxError(
                            line = lineNumber,
                            column = 1,
                            message = "Include statement may be malformed",
                            severity = "warning"
                        ))
                    }

                    // Check for long lines
                    if (line.length > 100) {
                        errors.add(SyntaxError(
                            line = lineNumber,
                            column = 101,
                            message = "Line too long (max 100 characters)",
                            severity = "warning"
                        ))
                    }
                }
                "java" -> {
                    // Check for missing semicolons
                    if (line.trim().matches(Regex(".*\\w+.*")) &&
                        !line.trim().endsWith(";") &&
                        !line.trim().endsWith("{") &&
                        !line.trim().endsWith("}") &&
                        !line.trim().endsWith(",") &&
                        !line.contains("class ") &&
                        !line.contains("interface ") &&
                        !line.contains("if ") &&
                        !line.contains("for ") &&
                        !line.contains("while ")) {
                        errors.add(SyntaxError(
                            line = lineNumber,
                            column = line.length + 1,
                            message = "Missing semicolon",
                            severity = "error"
                        ))
                    }
                }
            }
        }

        return errors
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
     * Check Java syntax
     */
    private fun checkJavaSyntax(content: String): List<SyntaxError> {
        val errors = mutableListOf<SyntaxError>()
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

            // Check for missing class declaration
            if (line.contains("public static void main") && !content.contains("class ")) {
                errors.add(SyntaxError(
                    line = index + 1,
                    column = 1,
                    message = "Main method found but no class declaration",
                    severity = "error"
                ))
            }
        }

        return errors
    }

    /**
     * Check GLSL syntax
     */
    private fun checkGLSLSyntax(content: String): List<SyntaxError> {
        val errors = mutableListOf<SyntaxError>()
        val lines = content.lines()

        lines.forEachIndexed { index, line ->
            // Check for missing precision qualifiers in fragment shaders
            if (content.contains("#version") &&
                content.contains("fragment") &&
                line.contains("float") &&
                !line.contains("highp") &&
                !line.contains("mediump") &&
                !line.contains("lowp") &&
                !line.contains("precision")) {
                errors.add(SyntaxError(
                    line = index + 1,
                    column = 1,
                    message = "Missing precision qualifier for float in fragment shader",
                    severity = "warning"
                ))
            }

            // Check for void main function
            if (line.contains("void main") && !line.contains("(")) {
                errors.add(SyntaxError(
                    line = index + 1,
                    column = line.indexOf("void main") + 1,
                    message = "Main function declaration incomplete",
                    severity = "error"
                ))
            }
        }

        return errors
    }

    /**
     * Get code completion suggestions with enhanced context awareness
     */
    fun getCompletions(filePath: String, line: Int, column: Int): List<CompletionItem> {
        val document = openDocuments[filePath] ?: return emptyList()

        val completions = mutableListOf<CompletionItem>()

        // Get context-aware completions based on current line content
        val lines = document.content.lines()
        if (line < lines.size) {
            val currentLine = lines[line]
            val prefix = currentLine.substring(0, minOf(column, currentLine.length))

            // Add context-aware completions
            completions.addAll(getContextCompletions(document.language, prefix, document.content))
        }

        // Add language-specific completions
        completions.addAll(when (document.language) {
            "kotlin" -> getKotlinCompletions(document.content, line, column)
            "cpp" -> getCppCompletions(document.content, line, column)
            "java" -> getJavaCompletions(document.content, line, column)
            "glsl" -> getGLSLCompletions(document.content, line, column)
            else -> emptyList()
        })

        return completions.distinctBy { it.label }
    }

    /**
     * Get context-aware completions based on current typing
     */
    private fun getContextCompletions(language: String, prefix: String, fullContent: String): List<CompletionItem> {
        val completions = mutableListOf<CompletionItem>()

        // Extract variables, functions, and classes from current file
        when (language) {
            "kotlin" -> {
                // Find variable declarations
                Regex("val\\s+(\\w+)").findAll(fullContent).forEach { match ->
                    val varName = match.groupValues[1]
                    completions.add(CompletionItem(
                        label = varName,
                        kind = CompletionKind.VARIABLE,
                        detail = "Variable"
                    ))
                }

                // Find function declarations
                Regex("fun\\s+(\\w+)").findAll(fullContent).forEach { match ->
                    val funcName = match.groupValues[1]
                    completions.add(CompletionItem(
                        label = funcName,
                        kind = CompletionKind.FUNCTION,
                        detail = "Function"
                    ))
                }

                // Find class declarations
                Regex("class\\s+(\\w+)").findAll(fullContent).forEach { match ->
                    val className = match.groupValues[1]
                    completions.add(CompletionItem(
                        label = className,
                        kind = CompletionKind.CLASS,
                        detail = "Class"
                    ))
                }
            }
            "cpp" -> {
                // Find variable declarations
                Regex("(?:int|float|double|char|String|bool)\\s+(\\w+)").findAll(fullContent).forEach { match ->
                    val varName = match.groupValues[1]
                    completions.add(CompletionItem(
                        label = varName,
                        kind = CompletionKind.VARIABLE,
                        detail = "Variable"
                    ))
                }

                // Find function declarations
                Regex("(?:int|float|double|char|String|bool|void)\\s+(\\w+)\\s*\\(").findAll(fullContent).forEach { match ->
                    val funcName = match.groupValues[1]
                    completions.add(CompletionItem(
                        label = funcName,
                        kind = CompletionKind.FUNCTION,
                        detail = "Function"
                    ))
                }
            }
        }

        return completions
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
            "virtual", "override", "const", "static", "extern", "inline", "template",
            "typename", "using", "auto", "nullptr", "constexpr", "noexcept",
            "thread_local", "mutable", "volatile", "explicit", "friend"
        )

        keywords.forEach { keyword ->
            completions.add(CompletionItem(
                label = keyword,
                kind = CompletionKind.KEYWORD,
                detail = "C++ keyword"
            ))
        }

        // Common C++ types
        val types = listOf(
            "int", "float", "double", "char", "bool", "void", "size_t", "string",
            "vector", "map", "set", "array", "unique_ptr", "shared_ptr", "weak_ptr"
        )

        types.forEach { type ->
            completions.add(CompletionItem(
                label = type,
                kind = CompletionKind.TYPE,
                detail = "C++ type"
            ))
        }

        return completions
    }

    /**
     * Get Java code completions
     */
    private fun getJavaCompletions(content: String, line: Int, column: Int): List<CompletionItem> {
        val completions = mutableListOf<CompletionItem>()

        // Common Java keywords
        val keywords = listOf(
            "class", "interface", "enum", "public", "private", "protected", "static",
            "final", "abstract", "synchronized", "volatile", "transient", "native",
            "strictfp", "default", "package", "import", "extends", "implements"
        )

        keywords.forEach { keyword ->
            completions.add(CompletionItem(
                label = keyword,
                kind = CompletionKind.KEYWORD,
                detail = "Java keyword"
            ))
        }

        // Common Java types
        val types = listOf(
            "String", "Integer", "Double", "Boolean", "Object", "List", "Map", "Set",
            "ArrayList", "HashMap", "HashSet", "Optional", "Stream", "Collectors"
        )

        types.forEach { type ->
            completions.add(CompletionItem(
                label = type,
                kind = CompletionKind.TYPE,
                detail = "Java type"
            ))
        }

        return completions
    }

    /**
     * Get GLSL code completions
     */
    private fun getGLSLCompletions(content: String, line: Int, column: Int): List<CompletionItem> {
        val completions = mutableListOf<CompletionItem>()

        // GLSL keywords
        val keywords = listOf(
            "uniform", "attribute", "varying", "in", "out", "layout", "precision",
            "highp", "mediump", "lowp", "const", "flat", "smooth"
        )

        keywords.forEach { keyword ->
            completions.add(CompletionItem(
                label = keyword,
                kind = CompletionKind.KEYWORD,
                detail = "GLSL keyword"
            ))
        }

        // GLSL types
        val types = listOf(
            "vec2", "vec3", "vec4", "mat2", "mat3", "mat4", "sampler2D", "samplerCube",
            "float", "int", "bool", "void"
        )

        types.forEach { type ->
            completions.add(CompletionItem(
                label = type,
                kind = CompletionKind.TYPE,
                detail = "GLSL type"
            ))
        }

        // GLSL built-in functions
        val functions = listOf(
            "texture", "textureLod", "mix", "clamp", "normalize", "dot", "cross",
            "length", "distance", "sin", "cos", "tan", "pow", "exp", "log"
        )

        functions.forEach { func ->
            completions.add(CompletionItem(
                label = func,
                kind = CompletionKind.FUNCTION,
                detail = "GLSL function"
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
