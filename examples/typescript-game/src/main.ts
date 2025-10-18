/**
 * FoundryEngine TypeScript Game Example
 * Demonstrates the complete engine API usage
 */

import { 
    Engine, 
    World, 
    Scene, 
    Vector3, 
    Matrix4, 
    Memory,
    EventSystem,
    AssetManager,
    InputSystem,
    AudioSystem,
    RenderSystem,
    PhysicsSystem,
    NetworkSystem,
    Utils,
    Profiler
} from 'foundry-engine';

class Game {
    private engine: typeof Engine;
    private world: typeof World;
    private scene: typeof Scene;
    private isRunning: boolean = false;
    private entities: number[] = [];
    private sceneId: number = 0;
    
    // Game state
    private playerEntity: number = 0;
    private camera: any = null;
    private lights: any[] = [];
    private audioClips: Map<string, any> = new Map();
    
    // Performance monitoring
    private frameCount: number = 0;
    private lastFPSUpdate: number = 0;
    private currentFPS: number = 0;

    constructor() {
        this.engine = Engine;
        this.world = World;
        this.scene = Scene;
    }

    /**
     * Initialize the game
     */
    async initialize(): Promise<boolean> {
        try {
            console.log('Initializing FoundryEngine TypeScript Game...');
            
            // Initialize engine
            const success = await this.engine.initialize({
                windowWidth: 1280,
                windowHeight: 720,
                title: "FoundryEngine TypeScript Game",
                fullscreen: false,
                vsync: true,
                antialiasing: 4,
                targetFPS: 60,
                enablePhysics: true,
                enableAudio: true,
                enableNetworking: false,
                enableScripting: true
            });

            if (!success) {
                throw new Error('Failed to initialize engine');
            }

            // Set up event system
            this.setupEventSystem();
            
            // Load assets
            await this.loadAssets();
            
            // Create scene
            this.createScene();
            
            // Create game objects
            this.createGameObjects();
            
            // Set up rendering
            this.setupRendering();
            
            // Set up physics
            this.setupPhysics();
            
            // Set up audio
            this.setupAudio();
            
            // Set up input
            this.setupInput();
            
            console.log('Game initialized successfully');
            return true;
            
        } catch (error) {
            console.error('Failed to initialize game:', error);
            return false;
        }
    }

    /**
     * Set up event system
     */
    private setupEventSystem(): void {
        EventSystem.on('frame', (event) => {
            this.updateFPS();
        });

        EventSystem.on('error', (error) => {
            console.error('Engine error:', error);
        });
    }

    /**
     * Load game assets
     */
    private async loadAssets(): Promise<void> {
        console.log('Loading assets...');
        
        try {
            // Load textures
            const playerTexture = await AssetManager.loadAsset('assets/textures/player.png', 'texture');
            const groundTexture = await AssetManager.loadAsset('assets/textures/ground.jpg', 'texture');
            const skyboxTexture = await AssetManager.loadAsset('assets/textures/skybox.hdr', 'texture');
            
            // Load models
            const playerModel = await AssetManager.loadAsset('assets/models/player.gltf', 'mesh');
            const groundModel = await AssetManager.loadAsset('assets/models/ground.obj', 'mesh');
            
            // Load audio
            const backgroundMusic = await AssetManager.loadAsset('assets/audio/background.mp3', 'audio');
            const jumpSound = await AssetManager.loadAsset('assets/audio/jump.wav', 'audio');
            const collectSound = await AssetManager.loadAsset('assets/audio/collect.wav', 'audio');
            
            this.audioClips.set('background', backgroundMusic);
            this.audioClips.set('jump', jumpSound);
            this.audioClips.set('collect', collectSound);
            
            console.log('Assets loaded successfully');
            
        } catch (error) {
            console.warn('Some assets failed to load:', error);
        }
    }

    /**
     * Create the main scene
     */
    private createScene(): void {
        this.sceneId = this.scene.createScene('MainScene');
        this.scene.setActiveScene(this.sceneId);
        
        console.log('Scene created with ID:', this.sceneId);
    }

    /**
     * Create game objects
     */
    private createGameObjects(): void {
        // Create player
        this.playerEntity = this.world.createEntity();
        this.world.addTransformComponent(this.playerEntity, 0, 1, 0);
        this.scene.addEntityToScene(this.sceneId, this.playerEntity);
        
        // Create ground
        const groundEntity = this.world.createEntity();
        this.world.addTransformComponent(groundEntity, 0, 0, 0);
        this.scene.addEntityToScene(this.sceneId, groundEntity);
        
        // Create collectibles
        for (let i = 0; i < 10; i++) {
            const collectibleEntity = this.world.createEntity();
            const x = Utils.random(-10, 10);
            const z = Utils.random(-10, 10);
            this.world.addTransformComponent(collectibleEntity, x, 0.5, z);
            this.scene.addEntityToScene(this.sceneId, collectibleEntity);
            this.entities.push(collectibleEntity);
        }
        
        console.log('Game objects created');
    }

    /**
     * Set up rendering
     */
    private setupRendering(): void {
        // Set up camera
        this.camera = {
            position: new Vector3(0, 5, 10),
            rotation: new Vector3(-15, 0, 0),
            fov: 60,
            near: 0.1,
            far: 1000,
            projection: Matrix4.identity(),
            view: Matrix4.identity()
        };
        
        RenderSystem.setCamera(this.camera);
        
        // Set up lighting
        const directionalLight = {
            type: 'directional' as const,
            position: new Vector3(0, 10, 0),
            direction: new Vector3(0, -1, 0),
            color: new Vector3(1, 1, 1),
            intensity: 1.0,
            range: 0,
            spotAngle: 0
        };
        
        RenderSystem.addLight(directionalLight);
        this.lights.push(directionalLight);
        
        // Set ambient light
        RenderSystem.setAmbientLight(new Vector3(0.2, 0.2, 0.3), 0.3);
        
        // Set skybox
        RenderSystem.setSkybox('skybox');
        
        // Set fog
        RenderSystem.setFog(new Vector3(0.5, 0.5, 0.6), 0.01, 50, 200);
        
        console.log('Rendering setup completed');
    }

    /**
     * Set up physics
     */
    private setupPhysics(): void {
        // Set gravity
        PhysicsSystem.setGravity(new Vector3(0, -9.81, 0));
        
        // Add physics bodies
        PhysicsSystem.addRigidBody(this.playerEntity, 1.0, 'capsule');
        
        // Add ground physics
        const groundEntity = this.entities[0]; // Assuming first entity is ground
        PhysicsSystem.addRigidBody(groundEntity, 0.0, 'box'); // Static ground
        
        console.log('Physics setup completed');
    }

    /**
     * Set up audio
     */
    private setupAudio(): void {
        // Set listener position
        AudioSystem.setListenerPosition(new Vector3(0, 0, 0));
        AudioSystem.setListenerOrientation(new Vector3(0, 0, -1), new Vector3(0, 1, 0));
        
        // Set master volume
        AudioSystem.setMasterVolume(0.7);
        
        // Play background music
        const backgroundMusic = this.audioClips.get('background');
        if (backgroundMusic) {
            AudioSystem.playSound('background', 0.5, 1.0);
        }
        
        console.log('Audio setup completed');
    }

    /**
     * Set up input handling
     */
    private setupInput(): void {
        // Input will be handled in the update loop
        console.log('Input setup completed');
    }

    /**
     * Update FPS counter
     */
    private updateFPS(): void {
        this.frameCount++;
        const now = performance.now();
        
        if (now - this.lastFPSUpdate >= 1000) {
            this.currentFPS = this.frameCount;
            this.frameCount = 0;
            this.lastFPSUpdate = now;
            
            // Update FPS display
            const fpsElement = document.getElementById('fps');
            if (fpsElement) {
                fpsElement.textContent = `FPS: ${this.currentFPS}`;
            }
        }
    }

    /**
     * Handle input
     */
    private handleInput(): void {
        const inputState = InputSystem.getInputState();
        
        // Player movement
        const moveSpeed = 5.0;
        const deltaTime = this.engine.getDeltaTime();
        
        if (InputSystem.isKeyPressed('KeyW') || InputSystem.isKeyPressed('ArrowUp')) {
            // Move forward
            const force = new Vector3(0, 0, -moveSpeed);
            PhysicsSystem.applyForce(this.playerEntity, force);
        }
        
        if (InputSystem.isKeyPressed('KeyS') || InputSystem.isKeyPressed('ArrowDown')) {
            // Move backward
            const force = new Vector3(0, 0, moveSpeed);
            PhysicsSystem.applyForce(this.playerEntity, force);
        }
        
        if (InputSystem.isKeyPressed('KeyA') || InputSystem.isKeyPressed('ArrowLeft')) {
            // Move left
            const force = new Vector3(-moveSpeed, 0, 0);
            PhysicsSystem.applyForce(this.playerEntity, force);
        }
        
        if (InputSystem.isKeyPressed('KeyD') || InputSystem.isKeyPressed('ArrowRight')) {
            // Move right
            const force = new Vector3(moveSpeed, 0, 0);
            PhysicsSystem.applyForce(this.playerEntity, force);
        }
        
        // Jump
        if (InputSystem.isKeyPressed('Space')) {
            const jumpForce = new Vector3(0, 10, 0);
            PhysicsSystem.applyImpulse(this.playerEntity, jumpForce);
            
            // Play jump sound
            const jumpSound = this.audioClips.get('jump');
            if (jumpSound) {
                AudioSystem.playSound('jump', 0.8, 1.0);
            }
        }
        
        // Mouse look
        const mousePos = InputSystem.getMousePosition();
        if (mousePos.x !== 0 || mousePos.y !== 0) {
            // Update camera rotation based on mouse movement
            const sensitivity = 0.1;
            this.camera.rotation.y += mousePos.x * sensitivity;
            this.camera.rotation.x += mousePos.y * sensitivity;
            
            // Clamp vertical rotation
            this.camera.rotation.x = Utils.clamp(this.camera.rotation.x, -90, 90);
        }
    }

    /**
     * Update game logic
     */
    private update(): void {
        // Handle input
        this.handleInput();
        
        // Update camera to follow player
        this.updateCamera();
        
        // Check for collectible collection
        this.checkCollectibles();
        
        // Update performance metrics
        this.updatePerformanceMetrics();
    }

    /**
     * Update camera to follow player
     */
    private updateCamera(): void {
        // Get player position (simplified - would get from transform component)
        const playerPos = new Vector3(0, 2, 0); // Placeholder
        
        // Update camera position
        this.camera.position.x = playerPos.x;
        this.camera.position.y = playerPos.y + 5;
        this.camera.position.z = playerPos.z + 10;
        
        // Update camera matrices
        this.camera.projection = Matrix4.identity(); // Would calculate proper projection
        this.camera.view = Matrix4.identity(); // Would calculate proper view matrix
        
        RenderSystem.setCamera(this.camera);
    }

    /**
     * Check for collectible collection
     */
    private checkCollectibles(): void {
        const playerPos = new Vector3(0, 1, 0); // Placeholder player position
        
        for (let i = this.entities.length - 1; i >= 0; i--) {
            const entityId = this.entities[i];
            const collectiblePos = new Vector3(0, 0.5, 0); // Placeholder collectible position
            
            const distance = Utils.distance(playerPos, collectiblePos);
            
            if (distance < 1.0) {
                // Collect the item
                this.world.destroyEntity(entityId);
                this.entities.splice(i, 1);
                
                // Play collect sound
                const collectSound = this.audioClips.get('collect');
                if (collectSound) {
                    AudioSystem.playSound('collect', 0.6, 1.2);
                }
                
                console.log('Collectible collected! Remaining:', this.entities.length);
            }
        }
    }

    /**
     * Update performance metrics
     */
    private updatePerformanceMetrics(): void {
        const metrics = Profiler.getMetrics();
        
        // Update performance display
        const metricsElement = document.getElementById('metrics');
        if (metricsElement) {
            metricsElement.innerHTML = `
                <div>Frame Time: ${metrics.frameTime.toFixed(2)}ms</div>
                <div>FPS: ${metrics.fps.toFixed(1)}</div>
                <div>Memory: ${(metrics.memoryUsage / 1024 / 1024).toFixed(2)}MB</div>
                <div>Draw Calls: ${metrics.drawCalls}</div>
                <div>Triangles: ${metrics.triangles}</div>
            `;
        }
    }

    /**
     * Start the game loop
     */
    public start(): void {
        if (this.isRunning) {
            return;
        }
        
        this.isRunning = true;
        console.log('Starting game loop...');
        
        // Set up game loop
        const gameLoop = () => {
            if (!this.isRunning) {
                return;
            }
            
            // Update game
            this.update();
            
            // Continue loop
            requestAnimationFrame(gameLoop);
        };
        
        // Start the loop
        requestAnimationFrame(gameLoop);
    }

    /**
     * Stop the game
     */
    public stop(): void {
        this.isRunning = false;
        console.log('Game stopped');
    }

    /**
     * Shutdown the game
     */
    public shutdown(): void {
        this.stop();
        
        // Clean up entities
        for (const entityId of this.entities) {
            this.world.destroyEntity(entityId);
        }
        this.entities = [];
        
        // Shutdown engine
        this.engine.shutdown();
        
        console.log('Game shutdown completed');
    }
}

// Initialize and start the game
async function main() {
    const game = new Game();
    
    // Initialize the game
    const success = await game.initialize();
    if (!success) {
        console.error('Failed to initialize game');
        return;
    }
    
    // Start the game
    game.start();
    
    // Handle window close
    window.addEventListener('beforeunload', () => {
        game.shutdown();
    });
    
    // Handle visibility change
    document.addEventListener('visibilitychange', () => {
        if (document.hidden) {
            game.stop();
        } else {
            game.start();
        }
    });
}

// Start the game when the page loads
window.addEventListener('load', main);

// Export for potential module usage
export { Game };
