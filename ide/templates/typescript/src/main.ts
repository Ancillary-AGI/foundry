import { FoundryEngine, Scene, Entity, Component } from '@foundry/engine';
import { Vector3, Quaternion } from '@foundry/math';
import { Renderer } from '@foundry/graphics';

// Game class - Main game logic
class Game {
    private engine: FoundryEngine;
    private scene: Scene;
    private player: Entity;
    private camera: Entity;

    constructor() {
        // Initialize Foundry Engine
        this.engine = new FoundryEngine({
            canvas: document.getElementById('game-canvas') as HTMLCanvasElement,
            width: 800,
            height: 600,
            antialias: true,
            vsync: true
        });

        // Create main scene
        this.scene = this.engine.createScene('MainScene');

        // Initialize game objects
        this.initializeGame();

        // Start game loop
        this.startGameLoop();
    }

    private initializeGame(): void {
        // Create camera
        this.camera = this.scene.createEntity('Camera');
        this.camera.addComponent('Transform', {
            position: new Vector3(0, 5, 10),
            rotation: Quaternion.identity(),
            scale: new Vector3(1, 1, 1)
        });
        this.camera.addComponent('Camera', {
            fov: 60,
            near: 0.1,
            far: 1000,
            clearColor: [0.1, 0.1, 0.1, 1.0]
        });

        // Create player
        this.player = this.scene.createEntity('Player');
        this.player.addComponent('Transform', {
            position: new Vector3(0, 0, 0),
            rotation: Quaternion.identity(),
            scale: new Vector3(1, 1, 1)
        });

        // Add visual component (cube)
        this.player.addComponent('MeshRenderer', {
            geometry: 'cube',
            material: {
                color: [0.8, 0.2, 0.2, 1.0],
                metallic: 0.0,
                roughness: 0.5
            }
        });

        // Add physics
        this.player.addComponent('RigidBody', {
            mass: 1.0,
            useGravity: true,
            isKinematic: false
        });

        // Add collider
        this.player.addComponent('BoxCollider', {
            size: new Vector3(1, 1, 1),
            center: new Vector3(0, 0, 0)
        });

        // Create ground
        const ground = this.scene.createEntity('Ground');
        ground.addComponent('Transform', {
            position: new Vector3(0, -2, 0),
            rotation: Quaternion.identity(),
            scale: new Vector3(10, 0.1, 10)
        });
        ground.addComponent('MeshRenderer', {
            geometry: 'cube',
            material: {
                color: [0.2, 0.8, 0.2, 1.0],
                metallic: 0.0,
                roughness: 0.8
            }
        });
        ground.addComponent('RigidBody', {
            mass: 0.0, // Static
            useGravity: false,
            isKinematic: true
        });
        ground.addComponent('BoxCollider', {
            size: new Vector3(10, 0.1, 10),
            center: new Vector3(0, 0, 0)
        });

        // Set up input handling
        this.setupInput();

        // Set up lighting
        this.setupLighting();
    }

    private setupInput(): void {
        // Keyboard input
        document.addEventListener('keydown', (event) => {
            const transform = this.player.getComponent('Transform');
            const speed = 5.0;

            switch (event.code) {
                case 'KeyW':
                case 'ArrowUp':
                    transform.position.z -= speed * this.engine.deltaTime;
                    break;
                case 'KeyS':
                case 'ArrowDown':
                    transform.position.z += speed * this.engine.deltaTime;
                    break;
                case 'KeyA':
                case 'ArrowLeft':
                    transform.position.x -= speed * this.engine.deltaTime;
                    break;
                case 'KeyD':
                case 'ArrowRight':
                    transform.position.x += speed * this.engine.deltaTime;
                    break;
                case 'Space':
                    // Jump
                    const rigidBody = this.player.getComponent('RigidBody');
                    if (rigidBody) {
                        rigidBody.applyImpulse(new Vector3(0, 10, 0));
                    }
                    break;
            }
        });

        // Mouse input for camera
        let isMouseDown = false;
        let lastMouseX = 0;
        let lastMouseY = 0;

        document.addEventListener('mousedown', (event) => {
            isMouseDown = true;
            lastMouseX = event.clientX;
            lastMouseY = event.clientY;
        });

        document.addEventListener('mouseup', () => {
            isMouseDown = false;
        });

        document.addEventListener('mousemove', (event) => {
            if (!isMouseDown) return;

            const deltaX = event.clientX - lastMouseX;
            const deltaY = event.clientY - lastMouseY;

            const cameraTransform = this.camera.getComponent('Transform');
            cameraTransform.rotation.y -= deltaX * 0.01;
            cameraTransform.rotation.x -= deltaY * 0.01;

            lastMouseX = event.clientX;
            lastMouseY = event.clientY;
        });
    }

    private setupLighting(): void {
        // Create directional light
        const directionalLight = this.scene.createEntity('DirectionalLight');
        directionalLight.addComponent('Transform', {
            position: new Vector3(0, 10, 0),
            rotation: Quaternion.fromEulerAngles(-45, 45, 0),
            scale: new Vector3(1, 1, 1)
        });
        directionalLight.addComponent('DirectionalLight', {
            color: [1.0, 1.0, 1.0],
            intensity: 1.0,
            castShadows: true
        });

        // Create ambient light
        const ambientLight = this.scene.createEntity('AmbientLight');
        ambientLight.addComponent('AmbientLight', {
            color: [0.2, 0.2, 0.2],
            intensity: 1.0
        });
    }

    private startGameLoop(): void {
        const gameLoop = () => {
            this.update();
            this.render();
            requestAnimationFrame(gameLoop);
        };

        gameLoop();
    }

    private update(): void {
        // Update camera to follow player
        const playerTransform = this.player.getComponent('Transform');
        const cameraTransform = this.camera.getComponent('Transform');

        // Smooth camera follow
        const targetPosition = playerTransform.position.add(new Vector3(0, 5, 10));
        cameraTransform.position = cameraTransform.position.lerp(targetPosition, 0.1);
    }

    private render(): void {
        // Render the scene
        this.engine.render(this.scene, this.camera);
    }

    public destroy(): void {
        this.engine.destroy();
    }
}

// Backend integration for multiplayer/server communication
class GameServer {
    private ws: WebSocket | null = null;
    private game: Game;

    constructor(game: Game) {
        this.game = game;
        this.connectToServer();
    }

    private connectToServer(): void {
        // Connect to Go backend server
        this.ws = new WebSocket('ws://localhost:8080/game');

        this.ws.onopen = () => {
            console.log('Connected to game server');
            this.sendMessage('join', { playerId: 'player1' });
        };

        this.ws.onmessage = (event) => {
            const message = JSON.parse(event.data);
            this.handleServerMessage(message);
        };

        this.ws.onclose = () => {
            console.log('Disconnected from game server');
            // Attempt reconnection
            setTimeout(() => this.connectToServer(), 5000);
        };

        this.ws.onerror = (error) => {
            console.error('WebSocket error:', error);
        };
    }

    private sendMessage(type: string, data: any): void {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify({
                type: type,
                data: data,
                timestamp: Date.now()
            }));
        }
    }

    private handleServerMessage(message: any): void {
        switch (message.type) {
            case 'player_joined':
                console.log('Player joined:', message.data.playerId);
                break;
            case 'player_moved':
                // Update other player positions
                this.updatePlayerPosition(message.data);
                break;
            case 'game_state':
                // Update game state from server
                this.updateGameState(message.data);
                break;
        }
    }

    private updatePlayerPosition(data: any): void {
        // Update position of other players
        console.log('Updating player position:', data);
    }

    private updateGameState(data: any): void {
        // Update game state from server
        console.log('Updating game state:', data);
    }

    public sendPlayerUpdate(position: any, rotation: any): void {
        this.sendMessage('player_update', {
            position: position,
            rotation: rotation
        });
    }
}

// Initialize the game when the page loads
document.addEventListener('DOMContentLoaded', () => {
    const game = new Game();
    const server = new GameServer(game);

    // Handle window resize
    window.addEventListener('resize', () => {
        const canvas = document.getElementById('game-canvas') as HTMLCanvasElement;
        if (canvas) {
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;
            game.engine.resize(window.innerWidth, window.innerHeight);
        }
    });

    // Cleanup on page unload
    window.addEventListener('beforeunload', () => {
        game.destroy();
    });
});

// Export for use in other modules
export { Game, GameServer };