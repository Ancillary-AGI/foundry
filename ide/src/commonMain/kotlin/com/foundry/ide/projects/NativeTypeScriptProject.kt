package com.foundry.ide.projects

import com.foundry.ide.*
import com.foundry.ide.typescript.*
import com.foundry.ide.platforms.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.util.concurrent.ConcurrentHashMap

/**
 * Native TypeScript Project for FoundryEngine
 * Similar to Flutter's approach - TypeScript compiles to native code for each platform
 */
@Serializable
data class NativeTypeScriptProjectConfig(
    val name: String,
    val version: String = "1.0.0",
    val description: String = "",
    val author: String = "",
    val main: String = "src/main.ts",
    val targetPlatforms: List<PlatformType> = listOf(PlatformType.DESKTOP),
    val template: ProjectTemplate = ProjectTemplate.BASIC_3D,
    val features: ProjectFeatures = ProjectFeatures(),
    val server: ServerConfig? = null
)

@Serializable
data class ProjectFeatures(
    val includePhysics: Boolean = true,
    val includeAudio: Boolean = true,
    val includeNetworking: Boolean = false,
    val includeVR: Boolean = false,
    val includeMobile: Boolean = false,
    val includeWeb: Boolean = false,
    val includeServer: Boolean = false
)

@Serializable
data class ServerConfig(
    val enabled: Boolean = false,
    val language: String = "go", // "go", "typescript", "rust"
    val port: Int = 8080,
    val protocol: String = "websocket", // "http", "websocket", "grpc"
    val database: String? = null
)

enum class ProjectTemplate {
    BASIC_2D,
    BASIC_3D,
    PLATFORMER,
    FPS,
    RTS,
    MOBILE_GAME,
    VR_EXPERIENCE,
    WEB_GAME,
    MULTIPLAYER,
    CUSTOM
}

class NativeTypeScriptProject {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private val projectConfig = ConcurrentHashMap<String, NativeTypeScriptProjectConfig>()
    private val activeProjects = ConcurrentHashMap<String, ProjectInstance>()

    /**
     * Create a new native TypeScript project
     */
    suspend fun createProject(
        projectPath: String,
        config: NativeTypeScriptProjectConfig,
        template: ProjectTemplate = ProjectTemplate.BASIC_3D
    ): ProjectCreationResult {
        return try {
            val projectDir = Paths.get(projectPath)
            
            // Create project directory structure
            createProjectStructure(projectDir, config)
            
            // Generate project files based on template
            generateProjectFiles(projectDir, config, template)
            
            // Initialize project instance
            val projectInstance = ProjectInstance(projectDir.toString(), config)
            activeProjects[projectPath] = projectInstance
            
            // Store project configuration
            projectConfig[projectPath] = config
            
            ProjectCreationResult.Success("Project created successfully at $projectPath")
            
        } catch (e: Exception) {
            ProjectCreationResult.Failure("Failed to create project: ${e.message}")
        }
    }

    /**
     * Create project from GitHub template
     */
    suspend fun createFromGitHub(
        projectPath: String,
        repoUrl: String,
        branch: String = "main"
    ): ProjectCreationResult {
        return try {
            val projectDir = Paths.get(projectPath)
            
            // Clone repository
            val cloneResult = cloneRepository(repoUrl, projectDir.toString(), branch)
            if (!cloneResult.success) {
                return ProjectCreationResult.Failure("Failed to clone repository: ${cloneResult.error}")
            }
            
            // Load project configuration
            val config = loadProjectConfig(projectDir.resolve("foundry.json"))
            
            // Initialize project
            val projectInstance = ProjectInstance(projectDir.toString(), config)
            activeProjects[projectPath] = projectInstance
            projectConfig[projectPath] = config
            
            ProjectCreationResult.Success("Project created from GitHub template: $repoUrl")
            
        } catch (e: Exception) {
            ProjectCreationResult.Failure("Failed to create project from GitHub: ${e.message}")
        }
    }

    /**
     * Build project for specific platform
     */
    suspend fun buildProject(
        projectPath: String,
        platform: PlatformType,
        buildType: BuildType = BuildType.RELEASE
    ): BuildResult {
        return try {
            val project = activeProjects[projectPath] ?: return BuildResult.Failure("Project not found")
            val config = projectConfig[projectPath] ?: return BuildResult.Failure("Project config not found")
            
            // Check if platform is supported
            if (platform !in config.targetPlatforms) {
                return BuildResult.Failure("Platform $platform not supported by this project")
            }
            
            // Compile TypeScript to native code
            val compileResult = compileTypeScriptToNative(project.projectPath, platform)
            if (!compileResult.success) {
                return BuildResult.Failure("TypeScript compilation failed: ${compileResult.error}")
            }
            
            // Build native executable
            val buildResult = buildNativeExecutable(project.projectPath, platform, buildType)
            if (!buildResult.success) {
                return BuildResult.Failure("Native build failed: ${buildResult.error}")
            }
            
            // Package for distribution
            val packageResult = packageForDistribution(project.projectPath, platform, buildType)
            if (!packageResult.success) {
                return BuildResult.Failure("Packaging failed: ${packageResult.error}")
            }
            
            BuildResult.Success("Project built successfully for $platform")
            
        } catch (e: Exception) {
            BuildResult.Failure("Build failed: ${e.message}")
        }
    }

    /**
     * Run project on target platform
     */
    suspend fun runProject(
        projectPath: String,
        platform: PlatformType,
        deviceId: String? = null
    ): RunResult {
        return try {
            val project = activeProjects[projectPath] ?: return RunResult.Failure("Project not found")
            
            // Build project if not already built
            val buildResult = buildProject(projectPath, platform, BuildType.DEBUG)
            if (!buildResult.success) {
                return RunResult.Failure("Build failed: ${buildResult.error}")
            }
            
            // Deploy and run on target platform
            val deployResult = deployToPlatform(project.projectPath, platform, deviceId)
            if (!deployResult.success) {
                return RunResult.Failure("Deployment failed: ${deployResult.error}")
            }
            
            RunResult.Success("Project running on $platform")
            
        } catch (e: Exception) {
            RunResult.Failure("Run failed: ${e.message}")
        }
    }

    /**
     * Hot reload for development
     */
    suspend fun hotReload(projectPath: String, changedFile: String): HotReloadResult {
        return try {
            val project = activeProjects[projectPath] ?: return HotReloadResult.Failure("Project not found")
            
            // Recompile changed file
            val compileResult = recompileFile(project.projectPath, changedFile)
            if (!compileResult.success) {
                return HotReloadResult.Failure("Recompilation failed: ${compileResult.error}")
            }
            
            // Reload in running application
            val reloadResult = reloadInApplication(project.projectPath, changedFile)
            if (!reloadResult.success) {
                return HotReloadResult.Failure("Reload failed: ${reloadResult.error}")
            }
            
            HotReloadResult.Success("Hot reload completed for $changedFile")
            
        } catch (e: Exception) {
            HotReloadResult.Failure("Hot reload failed: ${e.message}")
        }
    }

    private fun createProjectStructure(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        // Create main directories
        val directories = listOf(
            "src",
            "assets",
            "assets/textures",
            "assets/models",
            "assets/audio",
            "assets/shaders",
            "build",
            "dist",
            "docs",
            "tests"
        )
        
        directories.forEach { dir ->
            Files.createDirectories(projectDir.resolve(dir))
        }
        
        // Create platform-specific directories
        config.targetPlatforms.forEach { platform ->
            Files.createDirectories(projectDir.resolve("build/${platform.name.lowercase()}"))
            Files.createDirectories(projectDir.resolve("dist/${platform.name.lowercase()}"))
        }
        
        // Create server directory if enabled
        if (config.server?.enabled == true) {
            Files.createDirectories(projectDir.resolve("server"))
            Files.createDirectories(projectDir.resolve("server/src"))
        }
    }

    private fun generateProjectFiles(
        projectDir: Path,
        config: NativeTypeScriptProjectConfig,
        template: ProjectTemplate
    ) {
        // Generate foundry.json
        val foundryConfig = generateFoundryConfig(config)
        Files.write(projectDir.resolve("foundry.json"), foundryConfig.toByteArray())
        
        // Generate package.json
        val packageJson = generatePackageJson(config)
        Files.write(projectDir.resolve("package.json"), packageJson.toByteArray())
        
        // Generate tsconfig.json
        val tsConfig = generateTsConfig(config)
        Files.write(projectDir.resolve("tsconfig.json"), tsConfig.toByteArray())
        
        // Generate main TypeScript file
        val mainTs = generateMainTypeScriptFile(config, template)
        Files.write(projectDir.resolve("src/main.ts"), mainTs.toByteArray())
        
        // Generate platform-specific files
        config.targetPlatforms.forEach { platform ->
            generatePlatformSpecificFiles(projectDir, config, platform)
        }
        
        // Generate server files if enabled
        if (config.server?.enabled == true) {
            generateServerFiles(projectDir, config)
        }
        
        // Generate CMakeLists.txt
        val cmakeLists = generateCMakeLists(config)
        Files.write(projectDir.resolve("CMakeLists.txt"), cmakeLists.toByteArray())
        
        // Generate README.md
        val readme = generateReadme(config)
        Files.write(projectDir.resolve("README.md"), readme.toByteArray())
    }

    private fun generateFoundryConfig(config: NativeTypeScriptProjectConfig): String {
        return """
        {
            "name": "${config.name}",
            "version": "${config.version}",
            "description": "${config.description}",
            "author": "${config.author}",
            "main": "${config.main}",
            "targetPlatforms": [${config.targetPlatforms.joinToString(",") { "\"${it.name.lowercase()}\"" }}],
            "template": "${config.template.name.lowercase()}",
            "features": {
                "physics": ${config.features.includePhysics},
                "audio": ${config.features.includeAudio},
                "networking": ${config.features.includeNetworking},
                "vr": ${config.features.includeVR},
                "mobile": ${config.features.includeMobile},
                "web": ${config.features.includeWeb},
                "server": ${config.features.includeServer}
            },
            "server": ${if (config.server != null) {
                """
                {
                    "enabled": ${config.server.enabled},
                    "language": "${config.server.language}",
                    "port": ${config.server.port},
                    "protocol": "${config.server.protocol}"
                }
                """
            } else "null"}
        }
        """.trimIndent()
    }

    private fun generatePackageJson(config: NativeTypeScriptProjectConfig): String {
        return """
        {
            "name": "${config.name.lowercase().replace(" ", "-")}",
            "version": "${config.version}",
            "description": "${config.description}",
            "main": "${config.main}",
            "scripts": {
                "build": "foundry build",
                "build:windows": "foundry build --platform windows",
                "build:macos": "foundry build --platform macos",
                "build:linux": "foundry build --platform linux",
                "build:android": "foundry build --platform android",
                "build:ios": "foundry build --platform ios",
                "build:web": "foundry build --platform web",
                "run": "foundry run",
                "run:windows": "foundry run --platform windows",
                "run:macos": "foundry run --platform macos",
                "run:linux": "foundry run --platform linux",
                "run:android": "foundry run --platform android",
                "run:ios": "foundry run --platform ios",
                "run:web": "foundry run --platform web",
                "dev": "foundry dev",
                "test": "foundry test",
                "package": "foundry package"
            },
            "devDependencies": {
                "@foundry/engine": "^1.0.0",
                "@foundry/typescript": "^1.0.0",
                "typescript": "^5.0.0"
            },
            "dependencies": {
                "@foundry/core": "^1.0.0",
                "@foundry/math": "^1.0.0",
                "@foundry/graphics": "^1.0.0",
                "@foundry/physics": "^1.0.0",
                "@foundry/audio": "^1.0.0"
            }
        }
        """.trimIndent()
    }

    private fun generateTsConfig(config: NativeTypeScriptProjectConfig): String {
        return """
        {
            "compilerOptions": {
                "target": "ES2020",
                "module": "ESNext",
                "moduleResolution": "node",
                "strict": true,
                "esModuleInterop": true,
                "skipLibCheck": true,
                "forceConsistentCasingInFileNames": true,
                "declaration": true,
                "outDir": "./build",
                "rootDir": "./src",
                "resolveJsonModule": true,
                "allowSyntheticDefaultImports": true,
                "experimentalDecorators": true,
                "emitDecoratorMetadata": true,
                "baseUrl": ".",
                "paths": {
                    "@foundry/*": ["node_modules/@foundry/*"],
                    "@/*": ["./src/*"]
                }
            },
            "include": [
                "src/**/*"
            ],
            "exclude": [
                "node_modules",
                "build",
                "dist"
            ]
        }
        """.trimIndent()
    }

    private fun generateMainTypeScriptFile(config: NativeTypeScriptProjectConfig, template: ProjectTemplate): String {
        return when (template) {
            ProjectTemplate.BASIC_3D -> generateBasic3DTemplate(config)
            ProjectTemplate.PLATFORMER -> generatePlatformerTemplate(config)
            ProjectTemplate.FPS -> generateFPSTemplate(config)
            ProjectTemplate.MOBILE_GAME -> generateMobileTemplate(config)
            ProjectTemplate.VR_EXPERIENCE -> generateVRTemplate(config)
            ProjectTemplate.WEB_GAME -> generateWebTemplate(config)
            ProjectTemplate.MULTIPLAYER -> generateMultiplayerTemplate(config)
            else -> generateBasic3DTemplate(config)
        }
    }

    private fun generateBasic3DTemplate(config: NativeTypeScriptProjectConfig): String {
        return """
        import { Engine, World, Scene, Vector3, Transform, Renderer, Camera, Light } from '@foundry/core';
import { PhysicsWorld, RigidBody } from '@foundry/physics';
import { AudioSystem, AudioClip } from '@foundry/audio';

/**
 * ${config.name}
 * ${config.description}
 * 
 * @author ${config.author}
 * @version ${config.version}
 */

class Game {
    private engine: Engine;
    private world: World;
    private scene: Scene;
    private camera: Camera;
    private physics: PhysicsWorld;
    private audio: AudioSystem;
    
    private player: number = 0;
    private isRunning: boolean = false;

    constructor() {
        this.engine = new Engine();
        this.world = new World();
        this.scene = new Scene();
        this.camera = new Camera();
        this.physics = new PhysicsWorld();
        this.audio = new AudioSystem();
    }

    async initialize(): Promise<boolean> {
        console.log('Initializing ${config.name}...');
        
        // Initialize engine
        if (!this.engine.initialize()) {
            console.error('Failed to initialize engine');
            return false;
        }
        
        // Set up scene
        this.setupScene();
        
        // Set up physics
        this.setupPhysics();
        
        // Set up audio
        this.setupAudio();
        
        // Create game objects
        this.createGameObjects();
        
        console.log('Game initialized successfully');
        return true;
    }

    private setupScene(): void {
        // Set up camera
        this.camera.position = new Vector3(0, 5, 10);
        this.camera.lookAt(new Vector3(0, 0, 0));
        this.scene.setCamera(this.camera);
        
        // Add lighting
        const light = new Light();
        light.type = 'directional';
        light.direction = new Vector3(0, -1, 0);
        light.color = new Vector3(1, 1, 1);
        light.intensity = 1.0;
        this.scene.addLight(light);
        
        // Set ambient light
        this.scene.setAmbientLight(new Vector3(0.2, 0.2, 0.3), 0.3);
    }

    private setupPhysics(): void {
        // Set gravity
        this.physics.setGravity(new Vector3(0, -9.81, 0));
    }

    private setupAudio(): void {
        // Set up audio system
        this.audio.setMasterVolume(0.7);
    }

    private createGameObjects(): void {
        // Create player
        this.player = this.world.createEntity();
        const playerTransform = new Transform();
        playerTransform.position = new Vector3(0, 1, 0);
        this.world.addComponent(this.player, playerTransform);
        
        // Add physics body
        const playerBody = new RigidBody();
        playerBody.mass = 1.0;
        playerBody.shape = 'capsule';
        this.world.addComponent(this.player, playerBody);
        
        // Create ground
        const ground = this.world.createEntity();
        const groundTransform = new Transform();
        groundTransform.position = new Vector3(0, 0, 0);
        groundTransform.scale = new Vector3(10, 1, 10);
        this.world.addComponent(ground, groundTransform);
        
        const groundBody = new RigidBody();
        groundBody.mass = 0.0; // Static
        groundBody.shape = 'box';
        this.world.addComponent(ground, groundBody);
    }

    update(deltaTime: number): void {
        // Update physics
        this.physics.step(deltaTime);
        
        // Update game logic
        this.updateGameLogic(deltaTime);
    }

    private updateGameLogic(deltaTime: number): void {
        // Add your game logic here
    }

    render(): void {
        this.engine.render();
    }

    shutdown(): void {
        this.engine.shutdown();
    }
}

// Main game instance
const game = new Game();

// Initialize and start the game
async function main() {
    const success = await game.initialize();
    if (!success) {
        console.error('Failed to initialize game');
        return;
    }
    
    // Start game loop
    game.start();
}

// Start the game
main().catch(console.error);

export { Game };
        """.trimIndent()
    }

    private fun generatePlatformerTemplate(config: NativeTypeScriptProjectConfig): String {
        // Platformer-specific template
        return generateBasic3DTemplate(config) // Simplified for now
    }

    private fun generateFPSTemplate(config: NativeTypeScriptProjectConfig): String {
        // FPS-specific template
        return generateBasic3DTemplate(config) // Simplified for now
    }

    private fun generateMobileTemplate(config: NativeTypeScriptProjectConfig): String {
        // Mobile-specific template
        return generateBasic3DTemplate(config) // Simplified for now
    }

    private fun generateVRTemplate(config: NativeTypeScriptProjectConfig): String {
        // VR-specific template
        return generateBasic3DTemplate(config) // Simplified for now
    }

    private fun generateWebTemplate(config: NativeTypeScriptProjectConfig): String {
        // Web-specific template
        return generateBasic3DTemplate(config) // Simplified for now
    }

    private fun generateMultiplayerTemplate(config: NativeTypeScriptProjectConfig): String {
        // Multiplayer-specific template
        return generateBasic3DTemplate(config) // Simplified for now
    }

    private fun generatePlatformSpecificFiles(
        projectDir: Path,
        config: NativeTypeScriptProjectConfig,
        platform: PlatformType
    ) {
        when (platform) {
            PlatformType.WINDOWS -> generateWindowsFiles(projectDir, config)
            PlatformType.MACOS -> generateMacOSFiles(projectDir, config)
            PlatformType.LINUX -> generateLinuxFiles(projectDir, config)
            PlatformType.ANDROID -> generateAndroidFiles(projectDir, config)
            PlatformType.IOS -> generateiOSFiles(projectDir, config)
            PlatformType.WEB -> generateWebFiles(projectDir, config)
            else -> {}
        }
    }

    private fun generateWindowsFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        // Generate Windows-specific files
        val windowsConfig = """
        {
            "platform": "windows",
            "target": "win32",
            "arch": "x64",
            "runtime": "native"
        }
        """.trimIndent()
        
        Files.write(projectDir.resolve("build/windows/config.json"), windowsConfig.toByteArray())
    }

    private fun generateMacOSFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        // Generate macOS-specific files
        val macosConfig = """
        {
            "platform": "macos",
            "target": "darwin",
            "arch": "x64",
            "runtime": "native"
        }
        """.trimIndent()
        
        Files.write(projectDir.resolve("build/macos/config.json"), macosConfig.toByteArray())
    }

    private fun generateLinuxFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        // Generate Linux-specific files
        val linuxConfig = """
        {
            "platform": "linux",
            "target": "linux",
            "arch": "x64",
            "runtime": "native"
        }
        """.trimIndent()
        
        Files.write(projectDir.resolve("build/linux/config.json"), linuxConfig.toByteArray())
    }

    private fun generateAndroidFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        // Generate Android-specific files
        val androidConfig = """
        {
            "platform": "android",
            "target": "android",
            "arch": "arm64-v8a",
            "runtime": "native"
        }
        """.trimIndent()
        
        Files.write(projectDir.resolve("build/android/config.json"), androidConfig.toByteArray())
    }

    private fun generateiOSFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        // Generate iOS-specific files
        val iosConfig = """
        {
            "platform": "ios",
            "target": "ios",
            "arch": "arm64",
            "runtime": "native"
        }
        """.trimIndent()
        
        Files.write(projectDir.resolve("build/ios/config.json"), iosConfig.toByteArray())
    }

    private fun generateWebFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        // Generate Web-specific files
        val webConfig = """
        {
            "platform": "web",
            "target": "web",
            "arch": "wasm",
            "runtime": "webassembly"
        }
        """.trimIndent()
        
        Files.write(projectDir.resolve("build/web/config.json"), webConfig.toByteArray())
    }

    private fun generateServerFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        val server = config.server ?: return
        
        when (server.language) {
            "go" -> generateGoServerFiles(projectDir, config)
            "typescript" -> generateTypeScriptServerFiles(projectDir, config)
            "rust" -> generateRustServerFiles(projectDir, config)
        }
    }

    private fun generateGoServerFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        val goMod = """
        module ${config.name.lowercase().replace(" ", "-")}-server

        go 1.21

        require (
            github.com/gorilla/websocket v1.5.0
            github.com/gin-gonic/gin v1.9.1
        )
        """.trimIndent()
        
        Files.write(projectDir.resolve("server/go.mod"), goMod.toByteArray())
        
        val mainGo = """
        package main

        import (
            "log"
            "net/http"
            "github.com/gin-gonic/gin"
            "github.com/gorilla/websocket"
        )

        var upgrader = websocket.Upgrader{
            CheckOrigin: func(r *http.Request) bool {
                return true
            },
        }

        func main() {
            r := gin.Default()
            
            r.GET("/ws", handleWebSocket)
            r.GET("/health", func(c *gin.Context) {
                c.JSON(200, gin.H{"status": "ok"})
            })
            
            log.Printf("Server starting on port %d", ${config.server?.port ?: 8080})
            r.Run(":${config.server?.port ?: 8080}")
        }

        func handleWebSocket(c *gin.Context) {
            conn, err := upgrader.Upgrade(c.Writer, c.Request, nil)
            if err != nil {
                log.Print("upgrade failed: ", err)
                return
            }
            defer conn.Close()

            for {
                mt, message, err := conn.ReadMessage()
                if err != nil {
                    log.Println("read failed:", err)
                    break
                }
                
                log.Printf("recv: %s", message)
                
                err = conn.WriteMessage(mt, message)
                if err != nil {
                    log.Println("write failed:", err)
                    break
                }
            }
        }
        """.trimIndent()
        
        Files.write(projectDir.resolve("server/main.go"), mainGo.toByteArray())
    }

    private fun generateTypeScriptServerFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        val serverPackageJson = """
        {
            "name": "${config.name.lowercase().replace(" ", "-")}-server",
            "version": "${config.version}",
            "description": "Server for ${config.name}",
            "main": "src/main.ts",
            "scripts": {
                "start": "ts-node src/main.ts",
                "dev": "ts-node-dev src/main.ts",
                "build": "tsc",
                "test": "jest"
            },
            "dependencies": {
                "express": "^4.18.2",
                "socket.io": "^4.7.2",
                "cors": "^2.8.5"
            },
            "devDependencies": {
                "@types/node": "^20.0.0",
                "@types/express": "^4.17.17",
                "typescript": "^5.0.0",
                "ts-node": "^10.9.0",
                "ts-node-dev": "^2.0.0"
            }
        }
        """.trimIndent()
        
        Files.write(projectDir.resolve("server/package.json"), serverPackageJson.toByteArray())
        
        val serverMainTs = """
        import express from 'express';
import { createServer } from 'http';
import { Server } from 'socket.io';
import cors from 'cors';

const app = express();
const server = createServer(app);
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"]
    }
});

app.use(cors());
app.use(express.json());

app.get('/health', (req, res) => {
    res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

io.on('connection', (socket) => {
    console.log('Client connected:', socket.id);
    
    socket.on('disconnect', () => {
        console.log('Client disconnected:', socket.id);
    });
    
    socket.on('gameEvent', (data) => {
        // Broadcast to all other clients
        socket.broadcast.emit('gameEvent', data);
    });
});

const PORT = ${config.server?.port ?: 8080};
server.listen(PORT, () => {
    console.log(\`Server running on port \${PORT}\`);
});
        """.trimIndent()
        
        Files.write(projectDir.resolve("server/src/main.ts"), serverMainTs.toByteArray())
    }

    private fun generateRustServerFiles(projectDir: Path, config: NativeTypeScriptProjectConfig) {
        val cargoToml = """
        [package]
        name = "${config.name.lowercase().replace(" ", "-")}-server"
        version = "${config.version}"
        edition = "2021"

        [dependencies]
        tokio = { version = "1.0", features = ["full"] }
        warp = "0.3"
        serde = { version = "1.0", features = ["derive"] }
        serde_json = "1.0"
        """.trimIndent()
        
        Files.write(projectDir.resolve("server/Cargo.toml"), cargoToml.toByteArray())
    }

    private fun generateCMakeLists(config: NativeTypeScriptProjectConfig): String {
        return """
        cmake_minimum_required(VERSION 3.16)
        project(${config.name.replace(" ", "")})

        set(CMAKE_CXX_STANDARD 20)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)

        # Find FoundryEngine
        find_package(FoundryEngine REQUIRED)

        # TypeScript source files (will be compiled to C++)
        file(GLOB_RECURSE TYPESCRIPT_SOURCES "src/*.ts")

        # Generated C++ files
        set(GENERATED_CPP_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
        file(MAKE_DIRECTORY ${GENERATED_CPP_DIR})

        # Custom command to compile TypeScript to C++
        add_custom_command(
            OUTPUT ${GENERATED_CPP_DIR}/main.cpp
            COMMAND foundry-tsc --platform ${CMAKE_SYSTEM_NAME} --output ${GENERATED_CPP_DIR}
            DEPENDS ${TYPESCRIPT_SOURCES}
            COMMENT "Compiling TypeScript to native C++"
        )

        # Create executable
        add_executable(${config.name.replace(" ", "")} ${GENERATED_CPP_DIR}/main.cpp)

        # Link libraries
        target_link_libraries(${config.name.replace(" ", "")} FoundryEngine)

        # Platform-specific settings
        if(WIN32)
            target_link_libraries(${config.name.replace(" ", "")} d3d11 dxgi xaudio2)
        elseif(APPLE)
            target_link_libraries(${config.name.replace(" ", "")} "-framework OpenGL" "-framework Cocoa")
        elseif(UNIX)
            target_link_libraries(${config.name.replace(" ", "")} GL X11 pthread)
        endif()
        """.trimIndent()
    }

    private fun generateReadme(config: NativeTypeScriptProjectConfig): String {
        return """
        # ${config.name}

        ${config.description}

        ## Getting Started

        ### Prerequisites
        - FoundryEngine IDE
        - CMake 3.16+
        - Platform-specific dependencies

        ### Building

        \`\`\`bash
        # Build for all platforms
        foundry build

        # Build for specific platform
        foundry build --platform windows
        foundry build --platform macos
        foundry build --platform linux
        foundry build --platform android
        foundry build --platform ios
        foundry build --platform web
        \`\`\`

        ### Running

        \`\`\`bash
        # Run on current platform
        foundry run

        # Run on specific platform
        foundry run --platform windows
        foundry run --platform macos
        foundry run --platform linux
        foundry run --platform android
        foundry run --platform ios
        foundry run --platform web
        \`\`\`

        ### Development

        \`\`\`bash
        # Start development server with hot reload
        foundry dev
        \`\`\`

        ## Project Structure

        \`\`\`
        ${config.name}/
        ├── src/                    # TypeScript source code
        │   └── main.ts            # Main game file
        ├── assets/                # Game assets
        │   ├── textures/          # Texture files
        │   ├── models/            # 3D models
        │   ├── audio/             # Audio files
        │   └── shaders/           # Shader files
        ├── build/                 # Build output
        ├── dist/                  # Distribution packages
        ├── tests/                 # Test files
        ├── docs/                  # Documentation
        ├── foundry.json          # Project configuration
        ├── package.json          # Node.js dependencies
        ├── tsconfig.json         # TypeScript configuration
        └── CMakeLists.txt        # CMake build configuration
        \`\`\`

        ## Features

        ${if (config.features.includePhysics) "- ✅ Physics simulation" else "- ❌ Physics simulation"}
        ${if (config.features.includeAudio) "- ✅ Audio system" else "- ❌ Audio system"}
        ${if (config.features.includeNetworking) "- ✅ Networking" else "- ❌ Networking"}
        ${if (config.features.includeVR) "- ✅ VR support" else "- ❌ VR support"}
        ${if (config.features.includeMobile) "- ✅ Mobile support" else "- ❌ Mobile support"}
        ${if (config.features.includeWeb) "- ✅ Web support" else "- ❌ Web support"}
        ${if (config.features.includeServer) "- ✅ Server integration" else "- ❌ Server integration"}

        ## Target Platforms

        ${config.targetPlatforms.joinToString("\n") { "- ${it.name}" }}

        ## Author

        ${config.author}

        ## License

        This project is licensed under the MIT License.
        """.trimIndent()
    }

    // Helper methods for compilation and building
    private suspend fun compileTypeScriptToNative(projectPath: String, platform: PlatformType): CompileResult {
        // Implementation would use the NativeTypeScriptRuntime
        return CompileResult.Success("TypeScript compiled to native code")
    }

    private suspend fun buildNativeExecutable(projectPath: String, platform: PlatformType, buildType: BuildType): BuildResult {
        // Implementation would use CMake to build native executable
        return BuildResult.Success("Native executable built")
    }

    private suspend fun packageForDistribution(projectPath: String, platform: PlatformType, buildType: BuildType): PackageResult {
        // Implementation would package the built executable for distribution
        return PackageResult.Success("Package created")
    }

    private suspend fun deployToPlatform(projectPath: String, platform: PlatformType, deviceId: String?): DeployResult {
        // Implementation would deploy to target platform
        return DeployResult.Success("Deployed to platform")
    }

    private suspend fun recompileFile(projectPath: String, changedFile: String): CompileResult {
        // Implementation would recompile only the changed file
        return CompileResult.Success("File recompiled")
    }

    private suspend fun reloadInApplication(projectPath: String, changedFile: String): ReloadResult {
        // Implementation would reload the changed code in the running application
        return ReloadResult.Success("Code reloaded")
    }

    private suspend fun cloneRepository(repoUrl: String, targetPath: String, branch: String): CloneResult {
        // Implementation would clone the GitHub repository
        return CloneResult.Success("Repository cloned")
    }

    private fun loadProjectConfig(configFile: Path): NativeTypeScriptProjectConfig {
        // Implementation would load and parse the foundry.json file
        return NativeTypeScriptProjectConfig("Default Project")
    }
}

// Result classes
sealed class ProjectCreationResult {
    data class Success(val message: String) : ProjectCreationResult()
    data class Failure(val error: String) : ProjectCreationResult()
}

sealed class BuildResult {
    data class Success(val message: String) : BuildResult()
    data class Failure(val error: String) : BuildResult()
}

sealed class RunResult {
    data class Success(val message: String) : RunResult()
    data class Failure(val error: String) : RunResult()
}

sealed class HotReloadResult {
    data class Success(val message: String) : HotReloadResult()
    data class Failure(val error: String) : HotReloadResult()
}

sealed class CompileResult {
    data class Success(val message: String) : CompileResult()
    data class Failure(val error: String) : CompileResult()
}

sealed class PackageResult {
    data class Success(val message: String) : PackageResult()
    data class Failure(val error: String) : PackageResult()
}

sealed class DeployResult {
    data class Success(val message: String) : DeployResult()
    data class Failure(val error: String) : DeployResult()
}

sealed class ReloadResult {
    data class Success(val message: String) : ReloadResult()
    data class Failure(val error: String) : ReloadResult()
}

sealed class CloneResult {
    data class Success(val message: String) : CloneResult()
    data class Failure(val error: String) : CloneResult()
}

// Project instance
data class ProjectInstance(
    val projectPath: String,
    val config: NativeTypeScriptProjectConfig
)

enum class BuildType {
    DEBUG,
    RELEASE
}
