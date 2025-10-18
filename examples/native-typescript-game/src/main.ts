import { Engine, World, Scene, Vector3, Transform, Renderer, Camera, Light } from '@foundry/core';
import { PhysicsWorld, RigidBody } from '@foundry/physics';
import { AudioSystem, AudioClip } from '@foundry/audio';

/**
 * Native TypeScript Game Example
 * This TypeScript code will be compiled to native C++ for each platform
 * Similar to how Flutter compiles Dart to native code
 */

class NativeTypeScriptGame {
    private engine: Engine;
    private world: World;
    private scene: Scene;
    private camera: Camera;
    private physics: PhysicsWorld;
    private audio: AudioSystem;
    
    private player: number = 0;
    private enemies: number[] = [];
    private isRunning: boolean = false;
    private score: number = 0;
    private gameTime: number = 0;

    constructor() {
        this.engine = new Engine();
        this.world = new World();
        this.scene = new Scene();
        this.camera = new Camera();
        this.physics = new PhysicsWorld();
        this.audio = new AudioSystem();
    }

    async initialize(): Promise<boolean> {
        console.log('Initializing Native TypeScript Game...');
        
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
        this.camera.position = new Vector3(0, 10, 20);
        this.camera.lookAt(new Vector3(0, 0, 0));
        this.camera.fov = 60;
        this.camera.nearPlane = 0.1;
        this.camera.farPlane = 1000;
        this.scene.setCamera(this.camera);
        
        // Add directional light
        const sunLight = new Light();
        sunLight.type = 'directional';
        sunLight.direction = new Vector3(0, -1, 0);
        sunLight.color = new Vector3(1, 0.95, 0.8);
        sunLight.intensity = 1.0;
        this.scene.addLight(sunLight);
        
        // Add ambient light
        this.scene.setAmbientLight(new Vector3(0.2, 0.2, 0.3), 0.3);
        
        // Add point light for atmosphere
        const pointLight = new Light();
        pointLight.type = 'point';
        pointLight.position = new Vector3(5, 5, 5);
        pointLight.color = new Vector3(1, 0.5, 0.2);
        pointLight.intensity = 0.8;
        pointLight.range = 20;
        this.scene.addLight(pointLight);
    }

    private setupPhysics(): void {
        // Set gravity
        this.physics.setGravity(new Vector3(0, -9.81, 0));
        
        // Set physics world properties
        this.physics.setTimeStep(1.0 / 60.0);
        this.physics.setMaxSubSteps(3);
    }

    private setupAudio(): void {
        // Set up audio system
        this.audio.setMasterVolume(0.7);
        this.audio.setListenerPosition(this.camera.position);
        this.audio.setListenerOrientation(this.camera.forward, this.camera.up);
    }

    private createGameObjects(): void {
        // Create player
        this.player = this.world.createEntity();
        const playerTransform = new Transform();
        playerTransform.position = new Vector3(0, 2, 0);
        playerTransform.scale = new Vector3(1, 2, 1);
        this.world.addComponent(this.player, playerTransform);
        
        // Add physics body for player
        const playerBody = new RigidBody();
        playerBody.mass = 1.0;
        playerBody.shape = 'capsule';
        playerBody.friction = 0.7;
        playerBody.restitution = 0.1;
        this.world.addComponent(this.player, playerBody);
        
        // Create ground
        const ground = this.world.createEntity();
        const groundTransform = new Transform();
        groundTransform.position = new Vector3(0, 0, 0);
        groundTransform.scale = new Vector3(50, 1, 50);
        this.world.addComponent(ground, groundTransform);
        
        const groundBody = new RigidBody();
        groundBody.mass = 0.0; // Static
        groundBody.shape = 'box';
        groundBody.friction = 0.8;
        this.world.addComponent(ground, groundBody);
        
        // Create some obstacles
        this.createObstacles();
        
        // Create enemies
        this.createEnemies();
    }

    private createObstacles(): void {
        const obstaclePositions = [
            new Vector3(5, 1, 5),
            new Vector3(-5, 1, 5),
            new Vector3(5, 1, -5),
            new Vector3(-5, 1, -5),
            new Vector3(0, 1, 10),
            new Vector3(10, 1, 0),
            new Vector3(-10, 1, 0),
            new Vector3(0, 1, -10)
        ];
        
        for (const pos of obstaclePositions) {
            const obstacle = this.world.createEntity();
            const obstacleTransform = new Transform();
            obstacleTransform.position = pos;
            obstacleTransform.scale = new Vector3(2, 2, 2);
            this.world.addComponent(obstacle, obstacleTransform);
            
            const obstacleBody = new RigidBody();
            obstacleBody.mass = 0.0; // Static
            obstacleBody.shape = 'box';
            obstacleBody.friction = 0.6;
            this.world.addComponent(obstacle, obstacleBody);
        }
    }

    private createEnemies(): void {
        for (let i = 0; i < 5; i++) {
            const enemy = this.world.createEntity();
            const enemyTransform = new Transform();
            enemyTransform.position = new Vector3(
                Math.random() * 20 - 10,
                1,
                Math.random() * 20 - 10
            );
            enemyTransform.scale = new Vector3(1, 1, 1);
            this.world.addComponent(enemy, enemyTransform);
            
            const enemyBody = new RigidBody();
            enemyBody.mass = 0.5;
            enemyBody.shape = 'sphere';
            enemyBody.friction = 0.5;
            enemyBody.restitution = 0.3;
            this.world.addComponent(enemy, enemyBody);
            
            this.enemies.push(enemy);
        }
    }

    update(deltaTime: number): void {
        this.gameTime += deltaTime;
        
        // Update physics
        this.physics.step(deltaTime);
        
        // Update game logic
        this.updatePlayer(deltaTime);
        this.updateEnemies(deltaTime);
        this.updateCamera(deltaTime);
        this.updateAudio(deltaTime);
        
        // Check for collisions
        this.checkCollisions();
        
        // Update score
        this.updateScore(deltaTime);
    }

    private updatePlayer(deltaTime: number): void {
        const playerTransform = this.world.getComponent<Transform>(this.player);
        if (!playerTransform) return;
        
        // Simple player movement (in a real game, this would be input-driven)
        const moveSpeed = 5.0;
        const jumpForce = 10.0;
        
        // Move player in a circle
        const time = this.gameTime;
        const radius = 8.0;
        playerTransform.position.x = Math.cos(time * 0.5) * radius;
        playerTransform.position.z = Math.sin(time * 0.5) * radius;
        
        // Jump occasionally
        if (Math.sin(time * 2) > 0.8) {
            const playerBody = this.world.getComponent<RigidBody>(this.player);
            if (playerBody) {
                playerBody.applyImpulse(new Vector3(0, jumpForce, 0));
            }
        }
    }

    private updateEnemies(deltaTime: number): void {
        for (const enemyId of this.enemies) {
            const enemyTransform = this.world.getComponent<Transform>(enemyId);
            if (!enemyTransform) continue;
            
            // Move enemies towards player
            const playerTransform = this.world.getComponent<Transform>(this.player);
            if (!playerTransform) continue;
            
            const direction = Vector3.subtract(playerTransform.position, enemyTransform.position);
            direction.normalize();
            
            const enemyBody = this.world.getComponent<RigidBody>(enemyId);
            if (enemyBody) {
                enemyBody.applyForce(Vector3.multiply(direction, 2.0));
            }
        }
    }

    private updateCamera(deltaTime: number): void {
        const playerTransform = this.world.getComponent<Transform>(this.player);
        if (!playerTransform) return;
        
        // Follow player with camera
        const targetPosition = Vector3.add(playerTransform.position, new Vector3(0, 8, 12));
        this.camera.position = Vector3.lerp(this.camera.position, targetPosition, deltaTime * 2.0);
        
        // Look at player
        const lookAtTarget = Vector3.add(playerTransform.position, new Vector3(0, 2, 0));
        this.camera.lookAt(lookAtTarget);
    }

    private updateAudio(deltaTime: number): void {
        // Update audio listener position
        this.audio.setListenerPosition(this.camera.position);
        this.audio.setListenerOrientation(this.camera.forward, this.camera.up);
        
        // Play ambient sounds
        if (Math.random() < 0.01) { // 1% chance per frame
            // this.audio.playSound("ambient_wind.wav", this.camera.position, 0.3);
        }
    }

    private checkCollisions(): void {
        // Check player-enemy collisions
        const playerTransform = this.world.getComponent<Transform>(this.player);
        if (!playerTransform) return;
        
        for (let i = this.enemies.length - 1; i >= 0; i--) {
            const enemyId = this.enemies[i];
            const enemyTransform = this.world.getComponent<Transform>(enemyId);
            if (!enemyTransform) continue;
            
            const distance = Vector3.distance(playerTransform.position, enemyTransform.position);
            if (distance < 2.0) {
                // Collision detected
                this.score += 10;
                console.log(`Enemy destroyed! Score: ${this.score}`);
                
                // Remove enemy
                this.world.destroyEntity(enemyId);
                this.enemies.splice(i, 1);
                
                // Play sound effect
                // this.audio.playSound("enemy_destroyed.wav", playerTransform.position, 0.8);
                
                // Spawn new enemy
                this.spawnNewEnemy();
            }
        }
    }

    private spawnNewEnemy(): void {
        const enemy = this.world.createEntity();
        const enemyTransform = new Transform();
        enemyTransform.position = new Vector3(
            Math.random() * 40 - 20,
            1,
            Math.random() * 40 - 20
        );
        enemyTransform.scale = new Vector3(1, 1, 1);
        this.world.addComponent(enemy, enemyTransform);
        
        const enemyBody = new RigidBody();
        enemyBody.mass = 0.5;
        enemyBody.shape = 'sphere';
        enemyBody.friction = 0.5;
        enemyBody.restitution = 0.3;
        this.world.addComponent(enemy, enemyBody);
        
        this.enemies.push(enemy);
    }

    private updateScore(deltaTime: number): void {
        // Update score based on survival time
        this.score += deltaTime * 0.1;
        
        // Display score every 5 seconds
        if (Math.floor(this.gameTime) % 5 === 0 && Math.floor(this.gameTime * 10) % 10 === 0) {
            console.log(`Score: ${Math.floor(this.score)} | Enemies: ${this.enemies.length} | Time: ${Math.floor(this.gameTime)}s`);
        }
    }

    render(): void {
        this.engine.render();
    }

    start(): void {
        this.isRunning = true;
        console.log('Game started!');
    }

    stop(): void {
        this.isRunning = false;
        console.log('Game stopped!');
    }

    shutdown(): void {
        this.stop();
        this.engine.shutdown();
        console.log('Game shutdown complete');
    }

    // Getters for game state
    getScore(): number {
        return this.score;
    }

    getGameTime(): number {
        return this.gameTime;
    }

    getEnemyCount(): number {
        return this.enemies.length;
    }

    isGameRunning(): boolean {
        return this.isRunning;
    }
}

// Main game instance
const game = new NativeTypeScriptGame();

// Initialize and start the game
async function main() {
    console.log('Starting Native TypeScript Game...');
    
    const success = await game.initialize();
    if (!success) {
        console.error('Failed to initialize game');
        return;
    }
    
    // Start game loop
    game.start();
    
    // Game loop (this would be handled by the engine in a real implementation)
    let lastTime = 0;
    function gameLoop(currentTime: number) {
        const deltaTime = (currentTime - lastTime) / 1000.0;
        lastTime = currentTime;
        
        if (game.isGameRunning()) {
            game.update(deltaTime);
            game.render();
            
            // Continue game loop
            requestAnimationFrame(gameLoop);
        } else {
            game.shutdown();
        }
    }
    
    // Start the game loop
    requestAnimationFrame(gameLoop);
}

// Start the game
main().catch(console.error);

// Export for potential use by other modules
export { NativeTypeScriptGame };
