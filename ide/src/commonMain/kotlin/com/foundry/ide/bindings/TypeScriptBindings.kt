package com.foundry.ide.bindings

import com.foundry.ide.*
import com.foundry.ide.editors.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import java.io.File
import java.nio.file.Files
import java.nio.file.Path

/**
 * TypeScript to C++ bindings for Foundry Engine APIs
 * Provides native interface for TypeScript code to call engine functions
 */
@Serializable
data class TypeScriptBinding(
    val name: String,
    val typescriptSignature: String,
    val cppImplementation: String,
    val returnType: String,
    val parameters: List<Parameter>
)

@Serializable
data class Parameter(
    val name: String,
    val type: String,
    val typescriptType: String,
    val cppType: String
)

class TypeScriptBindings {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private val bindings = mutableMapOf<String, TypeScriptBinding>()
    private val generatedFiles = mutableSetOf<String>()

    init {
        initializeCoreBindings()
    }

    /**
     * Initialize core engine API bindings
     */
    private fun initializeCoreBindings() {
        // Math bindings
        addBinding(TypeScriptBinding(
            name = "Vector3.add",
            typescriptSignature = "add(other: Vector3): Vector3",
            cppImplementation = "Vector3_add",
            returnType = "Vector3",
            parameters = listOf(
                Parameter("this", "Vector3", "Vector3", "Vector3"),
                Parameter("other", "Vector3", "Vector3", "Vector3")
            )
        ))

        addBinding(TypeScriptBinding(
            name = "Vector3.subtract",
            typescriptSignature = "subtract(other: Vector3): Vector3",
            cppImplementation = "Vector3_subtract",
            returnType = "Vector3",
            parameters = listOf(
                Parameter("this", "Vector3", "Vector3", "Vector3"),
                Parameter("other", "Vector3", "Vector3", "Vector3")
            )
        ))

        // Engine core bindings
        addBinding(TypeScriptBinding(
            name = "FoundryEngine.createScene",
            typescriptSignature = "createScene(name: string): Scene",
            cppImplementation = "FoundryEngine_createScene",
            returnType = "Scene",
            parameters = listOf(
                Parameter("this", "FoundryEngine", "FoundryEngine", "FoundryEngine*"),
                Parameter("name", "string", "string", "const char*")
            )
        ))

        addBinding(TypeScriptBinding(
            name = "Scene.createEntity",
            typescriptSignature = "createEntity(name: string): Entity",
            cppImplementation = "Scene_createEntity",
            returnType = "Entity",
            parameters = listOf(
                Parameter("this", "Scene", "Scene", "Scene*"),
                Parameter("name", "string", "string", "const char*")
            )
        ))

        // Component bindings
        addBinding(TypeScriptBinding(
            name = "Entity.addComponent",
            typescriptSignature = "addComponent(type: string, config?: any): Component",
            cppImplementation = "Entity_addComponent",
            returnType = "Component",
            parameters = listOf(
                Parameter("this", "Entity", "Entity", "Entity*"),
                Parameter("type", "string", "string", "const char*"),
                Parameter("config", "any", "any", "void*")
            )
        ))

        // Rendering bindings
        addBinding(TypeScriptBinding(
            name = "FoundryEngine.render",
            typescriptSignature = "render(scene: Scene, camera: Entity): void",
            cppImplementation = "FoundryEngine_render",
            returnType = "void",
            parameters = listOf(
                Parameter("this", "FoundryEngine", "FoundryEngine", "FoundryEngine*"),
                Parameter("scene", "Scene", "Scene", "Scene*"),
                Parameter("camera", "Entity", "Entity", "Entity*")
            )
        ))

        // Input bindings
        addBinding(TypeScriptBinding(
            name = "Input.getKey",
            typescriptSignature = "getKey(keyCode: string): boolean",
            cppImplementation = "Input_getKey",
            returnType = "boolean",
            parameters = listOf(
                Parameter("keyCode", "string", "string", "const char*")
            )
        ))

        // Physics bindings
        addBinding(TypeScriptBinding(
            name = "RigidBody.applyForce",
            typescriptSignature = "applyForce(force: Vector3): void",
            cppImplementation = "RigidBody_applyForce",
            returnType = "void",
            parameters = listOf(
                Parameter("this", "RigidBody", "RigidBody", "RigidBody*"),
                Parameter("force", "Vector3", "Vector3", "Vector3")
            )
        ))

        // Audio bindings
        addBinding(TypeScriptBinding(
            name = "AudioSource.play",
            typescriptSignature = "play(): void",
            cppImplementation = "AudioSource_play",
            returnType = "void",
            parameters = listOf(
                Parameter("this", "AudioSource", "AudioSource", "AudioSource*")
            )
        ))

        // File system bindings
        addBinding(TypeScriptBinding(
            name = "FileSystem.readFile",
            typescriptSignature = "readFile(path: string): Promise<string>",
            cppImplementation = "FileSystem_readFile",
            returnType = "Promise<string>",
            parameters = listOf(
                Parameter("path", "string", "string", "const char*")
            )
        ))
    }

    /**
     * Add a new binding
     */
    fun addBinding(binding: TypeScriptBinding) {
        bindings[binding.name] = binding
    }

    /**
     * Get binding by name
     */
    fun getBinding(name: String): TypeScriptBinding? {
        return bindings[name]
    }

    /**
     * Get all bindings
     */
    fun getAllBindings(): List<TypeScriptBinding> {
        return bindings.values.toList()
    }

    /**
     * Generate TypeScript declaration file
     */
    fun generateTypeScriptDeclarations(outputPath: String) {
        val content = buildString {
            appendLine("// Auto-generated TypeScript declarations for Foundry Engine")
            appendLine("// Do not edit manually - generated by TypeScriptBindings.kt")
            appendLine()
            appendLine("declare global {")

            // Generate interface declarations
            generateInterfaceDeclarations(this)

            // Generate global function declarations
            generateGlobalDeclarations(this)

            appendLine("}")
            appendLine()
            appendLine("export {};")
        }

        File(outputPath).writeText(content)
        generatedFiles.add(outputPath)
    }

    /**
     * Generate C++ binding implementations
     */
    fun generateCppBindings(outputPath: String) {
        val headerContent = buildString {
            appendLine("// Auto-generated C++ bindings for TypeScript")
            appendLine("// Do not edit manually - generated by TypeScriptBindings.kt")
            appendLine("#pragma once")
            appendLine("#include \"TypeScriptRuntime.h\"")
            appendLine("#include \"FoundryEngine.h\"")
            appendLine()
            appendLine("extern \"C\" {")

            bindings.values.forEach { binding ->
                generateCppFunctionDeclaration(this, binding)
            }

            appendLine("}")
        }

        val implContent = buildString {
            appendLine("// Auto-generated C++ binding implementations")
            appendLine("// Do not edit manually - generated by TypeScriptBindings.kt")
            appendLine("#include \"TypeScriptBindings.h\"")
            appendLine("#include \"TypeScriptRuntime.h\"")
            appendLine()
            appendLine("extern \"C\" {")

            bindings.values.forEach { binding ->
                generateCppFunctionImplementation(this, binding)
            }

            appendLine("}")
        }

        File("$outputPath.h").writeText(headerContent)
        File("$outputPath.cpp").writeText(implContent)
        generatedFiles.add("$outputPath.h")
        generatedFiles.add("$outputPath.cpp")
    }

    /**
     * Generate JavaScript runtime bridge
     */
    fun generateJavaScriptBridge(outputPath: String) {
        val content = buildString {
            appendLine("// Auto-generated JavaScript bridge for TypeScript bindings")
            appendLine("// Do not edit manually - generated by TypeScriptBindings.kt")
            appendLine()
            appendLine("// Load WebAssembly module")
            appendLine("let wasmModule = null;")
            appendLine()
            appendLine("export async function initializeBindings(wasmPath) {")
            appendLine("    const response = await fetch(wasmPath);")
            appendLine("    const buffer = await response.arrayBuffer();")
            appendLine("    const result = await WebAssembly.instantiate(buffer, {")
            appendLine("        env: {")
            appendLine("            memory: new WebAssembly.Memory({ initial: 256 }),")
            appendLine("            table: new WebAssembly.Table({ initial: 0, element: 'anyfunc' })")
            appendLine("        }")
            appendLine("    });")
            appendLine("    ")
            appendLine("    wasmModule = result.instance;")
            appendLine("    return wasmModule;")
            appendLine("}")
            appendLine()
            appendLine("// Bridge functions")

            bindings.values.forEach { binding ->
                generateJavaScriptBridgeFunction(this, binding)
            }
        }

        File(outputPath).writeText(content)
        generatedFiles.add(outputPath)
    }

    /**
     * Compile TypeScript project with bindings
     */
    suspend fun compileTypeScriptProject(projectRoot: String): CompilationResult {
        return try {
            // Generate binding files
            val bindingsDir = Path.of(projectRoot, "generated", "bindings")
            Files.createDirectories(bindingsDir)

            generateTypeScriptDeclarations(bindingsDir.resolve("engine.d.ts").toString())
            generateCppBindings(bindingsDir.resolve("TypeScriptBindings").toString())
            generateJavaScriptBridge(bindingsDir.resolve("bridge.js").toString())

            // Run TypeScript compiler
            val process = ProcessBuilder("tsc", "--project", projectRoot)
                .directory(File(projectRoot))
                .redirectErrorStream(true)
                .start()

            val output = process.inputStream.bufferedReader().readText()
            val exitCode = process.waitFor()

            CompilationResult(
                success = exitCode == 0,
                diagnostics = parseCompilerOutput(output),
                output = output
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
     * Clean generated binding files
     */
    fun cleanGeneratedFiles() {
        generatedFiles.forEach { filePath ->
            try {
                File(filePath).delete()
            } catch (e: Exception) {
                println("Failed to delete generated file: $filePath")
            }
        }
        generatedFiles.clear()
    }

    // Private helper methods

    private fun generateInterfaceDeclarations(builder: StringBuilder) {
        // Vector3 interface
        builder.appendLine("    interface Vector3 {")
        builder.appendLine("        x: number;")
        builder.appendLine("        y: number;")
        builder.appendLine("        z: number;")
        builder.appendLine("        add(other: Vector3): Vector3;")
        builder.appendLine("        subtract(other: Vector3): Vector3;")
        builder.appendLine("        multiply(scalar: number): Vector3;")
        builder.appendLine("        dot(other: Vector3): number;")
        builder.appendLine("        cross(other: Vector3): Vector3;")
        builder.appendLine("        length(): number;")
        builder.appendLine("        normalized(): Vector3;")
        builder.appendLine("    }")

        // FoundryEngine interface
        builder.appendLine("    interface FoundryEngine {")
        builder.appendLine("        createScene(name: string): Scene;")
        builder.appendLine("        render(scene: Scene, camera: Entity): void;")
        builder.appendLine("        deltaTime: number;")
        builder.appendLine("        time: number;")
        builder.appendLine("    }")

        // Scene interface
        builder.appendLine("    interface Scene {")
        builder.appendLine("        createEntity(name: string): Entity;")
        builder.appendLine("        name: string;")
        builder.appendLine("    }")

        // Entity interface
        builder.appendLine("    interface Entity {")
        builder.appendLine("        addComponent(type: string, config?: any): Component;")
        builder.appendLine("        getComponent(type: string): Component | null;")
        builder.appendLine("        name: string;")
        builder.appendLine("        id: string;")
        builder.appendLine("    }")

        // Component interface
        builder.appendLine("    interface Component {")
        builder.appendLine("        entity: Entity;")
        builder.appendLine("        enabled: boolean;")
        builder.appendLine("    }")

        // Input interface
        builder.appendLine("    interface Input {")
        builder.appendLine("        getKey(keyCode: string): boolean;")
        builder.appendLine("        getMouseButton(button: number): boolean;")
        builder.appendLine("        getMousePosition(): Vector2;")
        builder.appendLine("    }")

        // FileSystem interface
        builder.appendLine("    interface FileSystem {")
        builder.appendLine("        readFile(path: string): Promise<string>;")
        builder.appendLine("        writeFile(path: string, content: string): Promise<void>;")
        builder.appendLine("    }")
    }

    private fun generateGlobalDeclarations(builder: StringBuilder) {
        builder.appendLine("    const FoundryEngine: {")
        builder.appendLine("        new(config: any): FoundryEngine;")
        builder.appendLine("    };")
        builder.appendLine()
        builder.appendLine("    const Vector3: {")
        builder.appendLine("        new(x: number, y: number, z: number): Vector3;")
        builder.appendLine("    };")
        builder.appendLine()
        builder.appendLine("    const Input: Input;")
        builder.appendLine("    const FileSystem: FileSystem;")
    }

    private fun generateCppFunctionDeclaration(builder: StringBuilder, binding: TypeScriptBinding) {
        val params = binding.parameters.joinToString(", ") { param ->
            "${param.cppType} ${param.name}"
        }
        builder.appendLine("    ${binding.returnType} ${binding.cppImplementation}($params);")
    }

    private fun generateCppFunctionImplementation(builder: StringBuilder, binding: TypeScriptBinding) {
        builder.appendLine("    ${binding.returnType} ${binding.cppImplementation}(${binding.parameters.joinToString(", ") { "${it.cppType} ${it.name}" }}) {")
        
        // Generate actual implementation based on function name
        when (binding.name) {
            "initializeEngine" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        return engine.initialize();")
            }
            "shutdownEngine" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        engine.shutdown();")
            }
            "updateEngine" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        engine.update(deltaTime);")
            }
            "renderFrame" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        engine.render();")
            }
            "getDeltaTime" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        return engine.getDeltaTime();")
            }
            "getTotalTime" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        return engine.getTotalTime();")
            }
            "getFrameCount" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        return engine.getFrameCount();")
            }
            "setTargetFPS" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        engine.setTargetFPS(fps);")
            }
            "pauseEngine" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        engine.pause();")
            }
            "resumeEngine" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        engine.resume();")
            }
            "quitEngine" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        engine.quit();")
            }
            "createEntity" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        World* world = engine.getWorld();")
                builder.appendLine("        return world->createEntity();")
            }
            "destroyEntity" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        World* world = engine.getWorld();")
                builder.appendLine("        world->destroyEntity(entityId);")
            }
            "addComponent" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        World* world = engine.getWorld();")
                builder.appendLine("        world->addComponent<TransformComponent>(entityId, *component);")
            }
            "getComponent" -> {
                builder.appendLine("        Engine& engine = Engine::getInstance();")
                builder.appendLine("        World* world = engine.getWorld();")
                builder.appendLine("        return world->getComponent<TransformComponent>(entityId);")
            }
            "Vector3_constructor" -> {
                builder.appendLine("        return new Vector3(x, y, z);")
            }
            "Vector3_add" -> {
                builder.appendLine("        return new Vector3(*this + other);")
            }
            "Vector3_subtract" -> {
                builder.appendLine("        return new Vector3(*this - other);")
            }
            "Vector3_multiply" -> {
                builder.appendLine("        return new Vector3(*this * scalar);")
            }
            "Vector3_divide" -> {
                builder.appendLine("        return new Vector3(*this / scalar);")
            }
            "Vector3_length" -> {
                builder.appendLine("        return this->length();")
            }
            "Vector3_normalize" -> {
                builder.appendLine("        return new Vector3(this->normalize());")
            }
            "Vector3_dot" -> {
                builder.appendLine("        return this->dot(other);")
            }
            "Vector3_cross" -> {
                builder.appendLine("        return new Vector3(this->cross(other));")
            }
            "Matrix4_identity" -> {
                builder.appendLine("        this->identity();")
                builder.appendLine("        return *this;")
            }
            "Matrix4_multiply" -> {
                builder.appendLine("        return new Matrix4(*this * other);")
            }
            "Matrix4_transpose" -> {
                builder.appendLine("        return new Matrix4(this->transpose());")
            }
            "Matrix4_inverse" -> {
                builder.appendLine("        return new Matrix4(this->inverse());")
            }
            "Quaternion_normalize" -> {
                builder.appendLine("        return new Quaternion(this->normalize());")
            }
            "Quaternion_multiply" -> {
                builder.appendLine("        return new Quaternion(*this * other);")
            }
            else -> {
                // Default implementation for unknown functions
                if (binding.returnType != "void") {
                    builder.appendLine("        // Auto-generated implementation for ${binding.name}")
                    builder.appendLine("        return ${binding.returnType}();")
                } else {
                    builder.appendLine("        // Auto-generated implementation for ${binding.name}")
                }
            }
        }
        
        builder.appendLine("    }")
        builder.appendLine()
    }

    private fun generateJavaScriptBridgeFunction(builder: StringBuilder, binding: TypeScriptBinding) {
        val functionName = binding.cppImplementation
        builder.appendLine("export function $functionName(${binding.parameters.joinToString(", ") { it.name }}) {")
        builder.appendLine("    if (!wasmModule) {")
        builder.appendLine("        throw new Error('WASM module not initialized. Call initializeBindings() first.');")
        builder.appendLine("    }")
        builder.appendLine("    ")
        builder.appendLine("    // Call WASM function")
        builder.appendLine("    return wasmModule.exports.$functionName(${binding.parameters.joinToString(", ") { it.name }});")
        builder.appendLine("}")
        builder.appendLine()
    }

    private fun parseCompilerOutput(output: String): List<TypeScriptDiagnostic> {
        val diagnostics = mutableListOf<TypeScriptDiagnostic>()
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

    fun shutdown() {
        scope.cancel()
        cleanGeneratedFiles()
    }
}

// Import required types
import com.foundry.ide.languages.*

// Global bindings instance
val typescriptBindings = TypeScriptBindings()