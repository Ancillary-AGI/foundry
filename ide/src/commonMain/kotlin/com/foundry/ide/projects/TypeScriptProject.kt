package com.foundry.ide.projects

import com.foundry.ide.*
import com.foundry.ide.bindings.*
import com.foundry.ide.languages.*
import com.foundry.ide.editors.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths

/**
 * TypeScript project type for Foundry IDE
 * Handles TypeScript game development projects
 */
@Serializable
data class TypeScriptProjectConfig(
    val name: String,
    val version: String = "1.0.0",
    val description: String = "",
    val author: String = "",
    val main: String = "src/main.ts",
    val target: String = "web", // "web", "desktop", "mobile"
    val typescript: TypeScriptConfig = TypeScriptConfig(),
    val build: BuildConfig = BuildConfig(),
    val assets: AssetConfig = AssetConfig(),
    val dependencies: Map<String, String> = emptyMap()
)

@Serializable
data class TypeScriptConfig(
    val target: String = "ES2020",
    val module: String = "ESNext",
    val strict: Boolean = true,
    val esModuleInterop: Boolean = true,
    val skipLibCheck: Boolean = true,
    val forceConsistentCasingInFileNames: Boolean = true,
    val moduleResolution: String = "node"
)

@Serializable
data class BuildConfig(
    val optimization: Int = 2,
    val sourceMap: Boolean = true,
    val minify: Boolean = true,
    val enableWasm: Boolean = true,
    val enableThreads: Boolean = false,
    val enableSIMD: Boolean = true,
    val outDir: String = "dist",
    val customFlags: List<String> = emptyList()
)

@Serializable
data class AssetConfig(
    val images: String = "assets/images/**/*.{png,jpg,jpeg,gif,svg,webp}",
    val sounds: String = "assets/sounds/**/*.{mp3,wav,ogg,m4a}",
    val models: String = "assets/models/**/*.{obj,fbx,gltf,glb}",
    val shaders: String = "assets/shaders/**/*.{vert,frag,glsl}",
    val data: String = "assets/data/**/*.{json,xml,yaml,yml}"
)

class TypeScriptProject(
    private val projectRoot: String,
    private val config: TypeScriptProjectConfig
) : Project {

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var languageServer: TypeScriptLanguageServer? = null
    private var bindings: TypeScriptBindings? = null
    private val openFiles = mutableMapOf<String, EditorDocument>()

    override val name: String = config.name
    override val type: String = "typescript"
    override val rootPath: String = projectRoot

    /**
     * Initialize the TypeScript project
     */
    override suspend fun initialize(): Boolean {
        return try {
            // Initialize language server
            languageServer = TypeScriptLanguageServer()
            val serverInitialized = languageServer?.initialize(projectRoot) ?: false
            if (!serverInitialized) {
                println("Failed to initialize TypeScript language server")
                return false
            }

            // Initialize bindings
            bindings = TypeScriptBindings()

            // Generate project structure if needed
            createProjectStructure()

            // Generate binding files
            generateBindingFiles()

            true
        } catch (e: Exception) {
            println("Failed to initialize TypeScript project: ${e.message}")
            false
        }
    }

    /**
     * Open a file in the project
     */
    override suspend fun openFile(filePath: String): EditorDocument? {
        return try {
            val fullPath = Paths.get(projectRoot, filePath).toString()
            val file = File(fullPath)

            if (!file.exists() || !file.canRead()) {
                return null
            }

            val content = Files.readString(file.toPath())
            val language = detectLanguage(filePath)

            val document = EditorDocument(
                path = filePath,
                content = content,
                language = language
            )

            openFiles[filePath] = document

            // Notify language server
            if (language == "typescript" || language == "javascript") {
                languageServer?.openFile(fullPath, content)
            }

            document
        } catch (e: Exception) {
            println("Failed to open file $filePath: ${e.message}")
            null
        }
    }

    /**
     * Save a file in the project
     */
    override suspend fun saveFile(filePath: String, content: String): Boolean {
        return try {
            val fullPath = Paths.get(projectRoot, filePath)
            Files.createDirectories(fullPath.parent)
            Files.writeString(fullPath, content)

            // Update open file
            openFiles[filePath]?.let { document ->
                val updated = document.copy(content = content, isDirty = false)
                openFiles[filePath] = updated

                // Notify language server
                if (document.language == "typescript" || document.language == "javascript") {
                    languageServer?.updateFile(fullPath.toString(), content)
                }
            }

            true
        } catch (e: Exception) {
            println("Failed to save file $filePath: ${e.message}")
            false
        }
    }

    /**
     * Close a file in the project
     */
    override suspend fun closeFile(filePath: String): Boolean {
        return try {
            openFiles.remove(filePath)

            // Notify language server
            val fullPath = Paths.get(projectRoot, filePath).toString()
            languageServer?.closeFile(fullPath)

            true
        } catch (e: Exception) {
            println("Failed to close file $filePath: ${e.message}")
            false
        }
    }

    /**
     * Build the project
     */
    override suspend fun build(target: String): BuildResult {
        return try {
            when (target) {
                "typescript" -> compileTypeScript()
                "wasm" -> buildWebAssembly()
                "native" -> buildNative()
                else -> BuildResult.Failure("Unknown build target: $target")
            }
        } catch (e: Exception) {
            BuildResult.Failure("Build failed: ${e.message}")
        }
    }

    /**
     * Run the project
     */
    override suspend fun run(target: String): Boolean {
        return try {
            when (target) {
                "web" -> runWeb()
                "desktop" -> runDesktop()
                "mobile" -> runMobile()
                else -> {
                    println("Unknown run target: $target")
                    false
                }
            }
        } catch (e: Exception) {
            println("Failed to run project: ${e.message}")
            false
        }
    }

    /**
     * Debug the project
     */
    override suspend fun debug(target: String): Boolean {
        return try {
            when (target) {
                "web" -> debugWeb()
                "desktop" -> debugDesktop()
                else -> {
                    println("Debugging not supported for target: $target")
                    false
                }
            }
        } catch (e: Exception) {
            println("Failed to debug project: ${e.message}")
            false
        }
    }

    /**
     * Get project diagnostics
     */
    override suspend fun getDiagnostics(): List<ProjectDiagnostic> {
        val diagnostics = mutableListOf<ProjectDiagnostic>()

        // Get TypeScript diagnostics
        languageServer?.let { server ->
            openFiles.values.forEach { document ->
                if (document.language == "typescript" || document.language == "javascript") {
                    val fileDiagnostics = server.getDiagnostics(Paths.get(projectRoot, document.path).toString())
                    diagnostics.addAll(fileDiagnostics.map { diagnostic ->
                        ProjectDiagnostic(
                            file = document.path,
                            line = diagnostic.line,
                            column = diagnostic.column,
                            severity = when (diagnostic.severity) {
                                DiagnosticSeverity.ERROR -> "error"
                                DiagnosticSeverity.WARNING -> "warning"
                                DiagnosticSeverity.INFO -> "info"
                                DiagnosticSeverity.HINT -> "hint"
                            },
                            message = diagnostic.message,
                            source = "typescript"
                        )
                    })
                }
            }
        }

        return diagnostics
    }

    /**
     * Get project completions
     */
    override suspend fun getCompletions(filePath: String, line: Int, column: Int): List<CompletionItem> {
        return try {
            languageServer?.getCompletions(
                Paths.get(projectRoot, filePath).toString(),
                line,
                column
            )?.map { completion ->
                CompletionItem(
                    label = completion.label,
                    kind = when (completion.kind) {
                        CompletionItemKind.CLASS -> CompletionKind.CLASS
                        CompletionItemKind.FUNCTION -> CompletionKind.FUNCTION
                        CompletionItemKind.METHOD -> CompletionKind.METHOD
                        CompletionItemKind.VARIABLE -> CompletionKind.VARIABLE
                        CompletionItemKind.PROPERTY -> CompletionKind.PROPERTY
                        else -> CompletionKind.TEXT
                    },
                    detail = completion.detail,
                    documentation = completion.documentation,
                    insertText = completion.insertText
                )
            } ?: emptyList()
        } catch (e: Exception) {
            println("Failed to get completions: ${e.message}")
            emptyList()
        }
    }

    /**
     * Shutdown the project
     */
    override fun shutdown() {
        try {
            // Close all open files
            openFiles.keys.toList().forEach { filePath ->
                scope.launch { closeFile(filePath) }
            }

            // Shutdown language server
            languageServer?.shutdown()

            // Clean up bindings
            bindings?.shutdown()

            scope.cancel()
        } catch (e: Exception) {
            println("Error during project shutdown: ${e.message}")
        }
    }

    // Private implementation methods

    private fun createProjectStructure() {
        val dirs = listOf(
            "src",
            "assets/images",
            "assets/sounds",
            "assets/models",
            "assets/shaders",
            "assets/data",
            "dist",
            "generated/bindings"
        )

        dirs.forEach { dir ->
            val path = Paths.get(projectRoot, dir)
            if (!Files.exists(path)) {
                Files.createDirectories(path)
            }
        }

        // Create default files if they don't exist
        createDefaultFiles()
    }

    private fun createDefaultFiles() {
        // Create tsconfig.json
        val tsconfigPath = Paths.get(projectRoot, "tsconfig.json")
        if (!Files.exists(tsconfigPath)) {
            val tsconfig = """
                {
                  "compilerOptions": {
                    "target": "${config.typescript.target}",
                    "module": "${config.typescript.module}",
                    "lib": ["${config.typescript.target}", "DOM"],
                    "outDir": "${config.build.outDir}",
                    "rootDir": "src",
                    "strict": ${config.typescript.strict},
                    "esModuleInterop": ${config.typescript.esModuleInterop},
                    "skipLibCheck": ${config.typescript.skipLibCheck},
                    "forceConsistentCasingInFileNames": ${config.typescript.forceConsistentCasingInFileNames},
                    "moduleResolution": "${config.typescript.moduleResolution}",
                    "allowSyntheticDefaultImports": true,
                    "resolveJsonModule": true,
                    "isolatedModules": true,
                    "noEmit": false,
                    "declaration": true,
                    "declarationMap": true,
                    "sourceMap": ${config.build.sourceMap},
                    "removeComments": false,
                    "noImplicitAny": true,
                    "strictNullChecks": true,
                    "strictFunctionTypes": true,
                    "noImplicitReturns": true,
                    "noFallthroughCasesInSwitch": true,
                    "noUncheckedIndexedAccess": true,
                    "exactOptionalPropertyTypes": true,
                    "noImplicitOverride": true,
                    "allowUnusedLabels": false,
                    "allowUnreachableCode": false
                  },
                  "include": [
                    "src/**/*"
                  ],
                  "exclude": [
                    "node_modules",
                    "${config.build.outDir}",
                    "**/*.test.ts",
                    "**/*.spec.ts"
                  ]
                }
            """.trimIndent()
            Files.writeString(tsconfigPath, tsconfig)
        }

        // Create main.ts if it doesn't exist
        val mainPath = Paths.get(projectRoot, config.main)
        if (!Files.exists(mainPath)) {
            val mainContent = """
                import { FoundryEngine, Scene, Entity, Vector3 } from './generated/bindings/engine';

                // Game class
                class Game {
                    private engine: FoundryEngine;
                    private scene: Scene;
                    private player: Entity;

                    constructor(canvas: HTMLCanvasElement) {
                        // Initialize Foundry Engine
                        this.engine = new FoundryEngine({
                            canvas: canvas,
                            width: 800,
                            height: 600
                        });

                        // Create scene
                        this.scene = this.engine.createScene('MainScene');

                        // Create player
                        this.player = this.scene.createEntity('Player');
                        this.player.addComponent('Transform', {
                            position: new Vector3(0, 0, 0)
                        });

                        console.log('Game initialized!');
                    }

                    update(deltaTime: number) {
                        // Game update logic here
                    }

                    render() {
                        // Find camera entity (you would create this)
                        const camera = this.scene.findEntity('Camera');
                        if (camera) {
                            this.engine.render(this.scene, camera);
                        }
                    }
                }

                // Initialize game when page loads
                document.addEventListener('DOMContentLoaded', () => {
                    const canvas = document.getElementById('game-canvas') as HTMLCanvasElement;
                    if (canvas) {
                        const game = new Game(canvas);

                        // Main game loop
                        function gameLoop() {
                            game.update(1/60); // 60 FPS
                            game.render();
                            requestAnimationFrame(gameLoop);
                        }

                        gameLoop();
                    }
                });
            """.trimIndent()
            Files.createDirectories(mainPath.parent)
            Files.writeString(mainPath, mainContent)
        }
    }

    private fun generateBindingFiles() {
        val bindingsDir = Paths.get(projectRoot, "generated", "bindings")
        Files.createDirectories(bindingsDir)

        // Generate TypeScript declarations
        bindings?.generateTypeScriptDeclarations(bindingsDir.resolve("engine.d.ts").toString())

        // Generate C++ bindings
        bindings?.generateCppBindings(bindingsDir.resolve("TypeScriptBindings").toString())

        // Generate JavaScript bridge
        bindings?.generateJavaScriptBridge(bindingsDir.resolve("bridge.js").toString())
    }

    private suspend fun compileTypeScript(): BuildResult {
        return try {
            val result = bindings?.compileTypeScriptProject(projectRoot)
                ?: return BuildResult.Failure("Bindings not initialized")

            if (result.success) {
                BuildResult.Success("TypeScript compilation successful")
            } else {
                BuildResult.Failure("TypeScript compilation failed: ${result.output}")
            }
        } catch (e: Exception) {
            BuildResult.Failure("TypeScript compilation error: ${e.message}")
        }
    }

    private suspend fun buildWebAssembly(): BuildResult {
        // Build WebAssembly module
        return BuildResult.Success("WebAssembly build not implemented yet")
    }

    private suspend fun buildNative(): BuildResult {
        // Build native executable
        return BuildResult.Success("Native build not implemented yet")
    }

    private suspend fun runWeb(): Boolean {
        // Open web browser with the game
        return try {
            val url = "file://${Paths.get(projectRoot, "index.html").toAbsolutePath()}"
            // In a real implementation, this would open the default browser
            println("Opening game in browser: $url")
            true
        } catch (e: Exception) {
            println("Failed to run web version: ${e.message}")
            false
        }
    }

    private suspend fun runDesktop(): Boolean {
        // Run desktop version
        return try {
            println("Desktop version not implemented yet")
            false
        } catch (e: Exception) {
            println("Failed to run desktop version: ${e.message}")
            false
        }
    }

    private suspend fun runMobile(): Boolean {
        // Run mobile version
        return try {
            println("Mobile version not implemented yet")
            false
        } catch (e: Exception) {
            println("Failed to run mobile version: ${e.message}")
            false
        }
    }

    private suspend fun debugWeb(): Boolean {
        // Start debugging session for web version
        return try {
            println("Web debugging not implemented yet")
            false
        } catch (e: Exception) {
            println("Failed to start web debugging: ${e.message}")
            false
        }
    }

    private suspend fun debugDesktop(): Boolean {
        // Start debugging session for desktop version
        return try {
            println("Desktop debugging not implemented yet")
            false
        } catch (e: Exception) {
            println("Failed to start desktop debugging: ${e.message}")
            false
        }
    }

    private fun detectLanguage(filePath: String): String {
        return when (File(filePath).extension.lowercase()) {
            "ts" -> "typescript"
            "js" -> "javascript"
            "json" -> "json"
            "md" -> "markdown"
            "html" -> "html"
            "css" -> "css"
            "glsl", "vert", "frag" -> "glsl"
            else -> "plaintext"
        }
    }
}

// Import required types
import com.foundry.ide.languages.*

// Project factory function
fun createTypeScriptProject(projectRoot: String, config: TypeScriptProjectConfig): TypeScriptProject {
    return TypeScriptProject(projectRoot, config)
}