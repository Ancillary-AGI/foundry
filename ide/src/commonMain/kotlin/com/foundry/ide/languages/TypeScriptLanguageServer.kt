package com.foundry.ide.languages

import com.foundry.ide.*
import com.foundry.ide.editors.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import kotlin.io.path.exists

/**
 * TypeScript Language Server integration for Foundry IDE
 * Provides TypeScript language services including IntelliSense, diagnostics, and refactoring
 */
@Serializable
data class TypeScriptConfig(
    val target: String = "ES2020",
    val module: String = "ESNext",
    val strict: Boolean = true,
    val esModuleInterop: Boolean = true,
    val skipLibCheck: Boolean = true,
    val forceConsistentCasingInFileNames: Boolean = true,
    val moduleResolution: String = "node",
    val allowSyntheticDefaultImports: Boolean = true,
    val resolveJsonModule: Boolean = true,
    val isolatedModules: Boolean = true,
    val noEmit: Boolean = false,
    val declaration: Boolean = true,
    val declarationMap: Boolean = true,
    val sourceMap: Boolean = true,
    val removeComments: Boolean = false,
    val noImplicitAny: Boolean = true,
    val strictNullChecks: Boolean = true,
    val strictFunctionTypes: Boolean = true,
    val noImplicitReturns: Boolean = true,
    val noFallthroughCasesInSwitch: Boolean = true,
    val noUncheckedIndexedAccess: Boolean = true,
    val exactOptionalPropertyTypes: Boolean = true,
    val noImplicitOverride: Boolean = true,
    val allowUnusedLabels: Boolean = false,
    val allowUnreachableCode: Boolean = false,
    val experimentalDecorators: Boolean = true,
    val emitDecoratorMetadata: Boolean = true
)

@Serializable
data class TypeScriptDiagnostic(
    val file: String,
    val line: Int,
    val column: Int,
    val severity: DiagnosticSeverity,
    val message: String,
    val code: String? = null,
    val source: String = "typescript"
)

enum class DiagnosticSeverity {
    ERROR, WARNING, INFO, HINT
}

@Serializable
data class TypeScriptCompletionItem(
    val label: String,
    val kind: CompletionItemKind,
    val detail: String? = null,
    val documentation: String? = null,
    val insertText: String? = null,
    val sortText: String? = null,
    val filterText: String? = null,
    val preselect: Boolean = false,
    val commitCharacters: List<String>? = null
)

enum class CompletionItemKind {
    TEXT, METHOD, FUNCTION, CONSTRUCTOR, FIELD, VARIABLE, CLASS, INTERFACE,
    MODULE, PROPERTY, UNIT, VALUE, ENUM, KEYWORD, SNIPPET, COLOR, FILE,
    REFERENCE, FOLDER, ENUMMEMBER, CONSTANT, STRUCT, EVENT, OPERATOR, TYPEPARAMETER
}

class TypeScriptLanguageServer {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var serverProcess: Process? = null
    private val diagnosticCallbacks = mutableListOf<(List<TypeScriptDiagnostic>) -> Unit>()

    private val config = TypeScriptConfig()
    private val openFiles = mutableMapOf<String, String>()

    /**
     * Initialize the TypeScript language server
     */
    suspend fun initialize(projectRoot: String): Boolean {
        return try {
            // Check if TypeScript is available
            if (!isTypeScriptAvailable()) {
                println("TypeScript compiler not found. Please install TypeScript globally: npm install -g typescript")
                return false
            }

            // Start TypeScript language service
            startLanguageService(projectRoot)

            // Load tsconfig.json if it exists
            loadTypeScriptConfig(projectRoot)

            true
        } catch (e: Exception) {
            println("Failed to initialize TypeScript language server: ${e.message}")
            false
        }
    }

    /**
     * Open a TypeScript file for editing
     */
    suspend fun openFile(filePath: String, content: String) {
        try {
            openFiles[filePath] = content

            // Notify language server about opened file
            notifyFileOpened(filePath, content)

            // Trigger initial diagnostics
            updateDiagnostics(filePath)
        } catch (e: Exception) {
            println("Failed to open TypeScript file: ${e.message}")
        }
    }

    /**
     * Update content of an open TypeScript file
     */
    suspend fun updateFile(filePath: String, content: String) {
        try {
            openFiles[filePath] = content

            // Notify language server about file change
            notifyFileChanged(filePath, content)

            // Update diagnostics
            updateDiagnostics(filePath)
        } catch (e: Exception) {
            println("Failed to update TypeScript file: ${e.message}")
        }
    }

    /**
     * Close a TypeScript file
     */
    suspend fun closeFile(filePath: String) {
        try {
            openFiles.remove(filePath)

            // Notify language server about closed file
            notifyFileClosed(filePath)
        } catch (e: Exception) {
            println("Failed to close TypeScript file: ${e.message}")
        }
    }

    /**
     * Get completion suggestions at position
     */
    suspend fun getCompletions(filePath: String, line: Int, column: Int): List<TypeScriptCompletionItem> {
        return try {
            val content = openFiles[filePath] ?: return emptyList()

            // Get completions from TypeScript language service
            getCompletionsFromService(filePath, line, column, content)
        } catch (e: Exception) {
            println("Failed to get TypeScript completions: ${e.message}")
            emptyList()
        }
    }

    /**
     * Get diagnostics for a file
     */
    suspend fun getDiagnostics(filePath: String): List<TypeScriptDiagnostic> {
        return try {
            val content = openFiles[filePath] ?: return emptyList()

            // Get diagnostics from TypeScript compiler
            getDiagnosticsFromCompiler(filePath, content)
        } catch (e: Exception) {
            println("Failed to get TypeScript diagnostics: ${e.message}")
            emptyList()
        }
    }

    /**
     * Get definition location for symbol at position
     */
    suspend fun getDefinition(filePath: String, line: Int, column: Int): Location? {
        return try {
            val content = openFiles[filePath] ?: return null

            // Find definition using TypeScript language service
            findDefinition(filePath, line, column, content)
        } catch (e: Exception) {
            println("Failed to get TypeScript definition: ${e.message}")
            null
        }
    }

    /**
     * Get references for symbol at position
     */
    suspend fun getReferences(filePath: String, line: Int, column: Int): List<Location> {
        return try {
            val content = openFiles[filePath] ?: return emptyList()

            // Find references using TypeScript language service
            findReferences(filePath, line, column, content)
        } catch (e: Exception) {
            println("Failed to get TypeScript references: ${e.message}")
            emptyList()
        }
    }

    /**
     * Rename symbol at position
     */
    suspend fun renameSymbol(filePath: String, line: Int, column: Int, newName: String): Map<String, TextEdit> {
        return try {
            val content = openFiles[filePath] ?: return emptyMap()

            // Perform rename refactoring
            performRename(filePath, line, column, newName, content)
        } catch (e: Exception) {
            println("Failed to rename TypeScript symbol: ${e.message}")
            emptyMap()
        }
    }

    /**
     * Format document or range
     */
    suspend fun formatDocument(filePath: String, range: Range? = null): List<TextEdit> {
        return try {
            val content = openFiles[filePath] ?: return emptyList()

            // Format using TypeScript formatter
            formatTypeScriptCode(filePath, content, range)
        } catch (e: Exception) {
            println("Failed to format TypeScript document: ${e.message}")
            emptyList()
        }
    }

    /**
     * Compile TypeScript project
     */
    suspend fun compileProject(projectRoot: String): CompilationResult {
        return try {
            val result = runTypeScriptCompiler(projectRoot)
            CompilationResult(
                success = result.exitCode == 0,
                diagnostics = parseCompilerOutput(result.output),
                output = result.output
            )
        } catch (e: Exception) {
            CompilationResult(
                success = false,
                diagnostics = listOf(TypeScriptDiagnostic(
                    file = "",
                    line = 0,
                    column = 0,
                    severity = DiagnosticSeverity.ERROR,
                    message = "Compilation failed: ${e.message}"
                )),
                output = e.message ?: ""
            )
        }
    }

    /**
     * Shutdown the language server
     */
    fun shutdown() {
        try {
            serverProcess?.destroy()
            serverProcess = null
            scope.cancel()
        } catch (e: Exception) {
            println("Failed to shutdown TypeScript language server: ${e.message}")
        }
    }

    // Private implementation methods

    private fun isTypeScriptAvailable(): Boolean {
        return try {
            val process = ProcessBuilder("tsc", "--version")
                .redirectErrorStream(true)
                .start()

            val result = process.waitFor()
            result == 0
        } catch (e: Exception) {
            false
        }
    }

    private fun startLanguageService(projectRoot: String) {
        // In a real implementation, this would start the TypeScript language service
        // For now, we'll use direct compiler calls
    }

    private fun loadTypeScriptConfig(projectRoot: String) {
        try {
            val tsconfigPath = Path.of(projectRoot, "tsconfig.json")
            if (tsconfigPath.exists()) {
                val content = Files.readString(tsconfigPath)
                // Parse and apply tsconfig.json settings
                // This would integrate with the config system
            }
        } catch (e: Exception) {
            println("Failed to load tsconfig.json: ${e.message}")
        }
    }

    private suspend fun notifyFileOpened(filePath: String, content: String) {
        // Notify TypeScript language service about opened file
    }

    private suspend fun notifyFileChanged(filePath: String, content: String) {
        // Notify TypeScript language service about file changes
    }

    private suspend fun notifyFileClosed(filePath: String) {
        // Notify TypeScript language service about closed file
    }

    private suspend fun updateDiagnostics(filePath: String) {
        val diagnostics = getDiagnostics(filePath)
        diagnosticCallbacks.forEach { callback ->
            try {
                callback(diagnostics)
            } catch (e: Exception) {
                println("Error in diagnostic callback: ${e.message}")
            }
        }
    }

    private suspend fun getCompletionsFromService(
        filePath: String,
        line: Int,
        column: Int,
        content: String
    ): List<TypeScriptCompletionItem> {
        // This would call the TypeScript language service for completions
        // For now, return basic keyword completions
        return listOf(
            TypeScriptCompletionItem("function", CompletionItemKind.KEYWORD, "function keyword"),
            TypeScriptCompletionItem("class", CompletionItemKind.KEYWORD, "class keyword"),
            TypeScriptCompletionItem("interface", CompletionItemKind.KEYWORD, "interface keyword"),
            TypeScriptCompletionItem("const", CompletionItemKind.KEYWORD, "const keyword"),
            TypeScriptCompletionItem("let", CompletionItemKind.KEYWORD, "let keyword"),
            TypeScriptCompletionItem("console", CompletionItemKind.VARIABLE, "console object")
        )
    }

    private suspend fun getDiagnosticsFromCompiler(filePath: String, content: String): List<TypeScriptDiagnostic> {
        return try {
            // Create temporary file for compilation
            val tempFile = File.createTempFile("typescript_check_", ".ts")
            tempFile.writeText(content)

            // Run TypeScript compiler in check mode
            val process = ProcessBuilder("tsc", "--noEmit", "--strict", tempFile.absolutePath)
                .redirectErrorStream(true)
                .start()

            val output = process.inputStream.bufferedReader().readText()
            val exitCode = process.waitFor()

            // Clean up temp file
            tempFile.delete()

            if (exitCode == 0) {
                emptyList()
            } else {
                // Parse TypeScript compiler output
                parseCompilerOutput(output)
            }
        } catch (e: Exception) {
            listOf(TypeScriptDiagnostic(
                file = filePath,
                line = 0,
                column = 0,
                severity = DiagnosticSeverity.ERROR,
                message = "Failed to check TypeScript: ${e.message}"
            ))
        }
    }

    private suspend fun findDefinition(
        filePath: String,
        line: Int,
        column: Int,
        content: String
    ): Location? {
        // This would use TypeScript language service to find definitions
        // For now, return null
        return null
    }

    private suspend fun findReferences(
        filePath: String,
        line: Int,
        column: Int,
        content: String
    ): List<Location> {
        // This would use TypeScript language service to find references
        // For now, return empty list
        return emptyList()
    }

    private suspend fun performRename(
        filePath: String,
        line: Int,
        column: Int,
        newName: String,
        content: String
    ): Map<String, TextEdit> {
        // This would use TypeScript language service for rename refactoring
        // For now, return empty map
        return emptyMap()
    }

    private suspend fun formatTypeScriptCode(
        filePath: String,
        content: String,
        range: Range?
    ): List<TextEdit> {
        // This would use a TypeScript formatter
        // For now, return empty list
        return emptyList()
    }

    private suspend fun runTypeScriptCompiler(projectRoot: String): CompilerResult {
        return try {
            val process = ProcessBuilder("tsc", "--project", projectRoot)
                .directory(File(projectRoot))
                .redirectErrorStream(true)
                .start()

            val output = process.inputStream.bufferedReader().readText()
            val exitCode = process.waitFor()

            CompilerResult(exitCode, output)
        } catch (e: Exception) {
            CompilerResult(-1, "Error: ${e.message}")
        }
    }

    private fun parseCompilerOutput(output: String): List<TypeScriptDiagnostic> {
        val diagnostics = mutableListOf<TypeScriptDiagnostic>()

        // Parse TypeScript compiler output format
        // Example: file.ts(10,5): error TS1234: Some error message
        val regex = Regex("(.+)\\((\\d+),(\\d+)\\): (error|warning) TS(\\d+): (.+)")
        output.lines().forEach { line ->
            val match = regex.find(line)
            if (match != null) {
                val (file, lineNum, colNum, type, code, message) = match.destructured
                diagnostics.add(TypeScriptDiagnostic(
                    file = file,
                    line = lineNum.toInt(),
                    column = colNum.toInt(),
                    severity = if (type == "error") DiagnosticSeverity.ERROR else DiagnosticSeverity.WARNING,
                    message = message,
                    code = code
                ))
            }
        }

        return diagnostics
    }

    // Callback registration
    fun onDiagnosticsChanged(callback: (List<TypeScriptDiagnostic>) -> Unit) {
        diagnosticCallbacks.add(callback)
    }

    fun removeDiagnosticsCallback(callback: (List<TypeScriptDiagnostic>) -> Unit) {
        diagnosticCallbacks.remove(callback)
    }
}

// Supporting data classes
@Serializable
data class Location(
    val file: String,
    val line: Int,
    val column: Int
)

@Serializable
data class Range(
    val start: Location,
    val end: Location
)

@Serializable
data class TextEdit(
    val range: Range,
    val newText: String
)

data class CompilerResult(
    val exitCode: Int,
    val output: String
)

@Serializable
data class CompilationResult(
    val success: Boolean,
    val diagnostics: List<TypeScriptDiagnostic>,
    val output: String
)

// Global TypeScript language server instance
val typescriptLanguageServer = TypeScriptLanguageServer()