# 🚀 Getting Started with Foundry Engine

Welcome to Foundry Engine - the world's most advanced TypeScript-first game development platform! This guide will help you create your first game in minutes.

## 📋 Prerequisites

Before you begin, ensure you have:

- **C++20** compatible compiler (MSVC 2022, GCC 11+, Clang 13+)
- **CMake 3.20+**
- **Vulkan SDK** (for advanced graphics)
- **Node.js 18+** (for TypeScript tooling)
- **Git** (for version control)

## 🛠️ Installation

### Option 1: Quick Install (Recommended)

```bash
# Install Foundry CLI
npm install -g @foundry/cli

# Create new project
foundry create my-game --template basic-3d

# Navigate to project
cd my-game

# Build and run
foundry dev
```

### Option 2: Build from Source

```bash
# Clone repository
git clone https://github.com/foundryengine/foundry.git
cd foundry

# Build engine
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Install
cmake --install .
```

## 🎮 Your First Game

Let's create a simple 3D game with TypeScript:

### 1. Create Project Structure

```
my-game/
├── src/
│   └── main.ts
├── assets/
│   ├── models/
│   ├── textures/
│   └── audio/
├── foundry.json
└── package.json
```

### 2. Configure Your Game

Create `foundry.json`:

```json
{
    "name": "My First Game",
    "version": "1.0.0",
    "main": "src/main.ts",
    "targetPlatforms": ["windows", "web"],
    "features": {
        "physics": true,
        "audio": true,
        "networking": false,
        "vr": false
    }
}
```

### 3. Write Your Game Code

Create `src/main.ts`:

```typescript
import { 
    Engine, 
    World, 
    Scene, 
    Vector3, 
    InputSystem,
    PhysicsSystem 
} from '@foundry/engine';

class MyFirstGame {
    private engine = new Engine();
    private world = new World();
    private scene = new Scene();
    private player: number = 0;

    async initialize(): Promise<boolean> {
        // Initialize engine
        const success = await this.engine.initialize({
            title: "My First Game",
            width: 1280,
            height: 720,
            enablePhysics: true,
            enableRayTracing: true
        });

        if (!success) return false;

        // Create scene
        const sceneId = this.scene.createScene('MainScene');
        this.scene.setActiveScene(sceneId);

        // Create player
        this.player = this.world.createEntity();
        this.world.addTransformComponent(this.player, 0, 2, 0);
        this.world.addMeshComponent(this.player, 'player_capsule');
        this.world.addPhysicsComponent(this.player, 70.0, 'capsule');

        // Create ground
        const ground = this.world.createEntity();
        this.world.addTransformComponent(ground, 0, 0, 0);
        this.world.addMeshComponent(ground, 'ground_plane');
        this.world.addPhysicsComponent(ground, 0.0, 'box'); // Static

        // Add lighting
        const sun = this.world.createEntity();
        this.world.addTransformComponent(sun, 0, 10, 0);
        this.world.addLightComponent(sun, {
            type: 'directional',
            color: new Vector3(1, 0.95, 0.8),
            intensity: 2.0,
            castShadows: true
        });

        console.log('🎮 Game initialized!');
        return true;
    }

    update(deltaTime: number): void {
        // Handle input
        this.handleInput(deltaTime);
        
        // Update physics
        PhysicsSystem.step(deltaTime);
    }

    private handleInput(deltaTime: number): void {
        const moveSpeed = 5.0;
        const force = new Vector3(0, 0, 0);

        if (InputSystem.isKeyPressed('KeyW')) {
            force.z = -moveSpeed;
        }
        if (InputSystem.isKeyPressed('KeyS')) {
            force.z = moveSpeed;
        }
        if (InputSystem.isKeyPressed('KeyA')) {
            force.x = -moveSpeed;
        }
        if (InputSystem.isKeyPressed('KeyD')) {
            force.x = moveSpeed;
        }
        if (InputSystem.isKeyPressed('Space')) {
            force.y = 10.0; // Jump
        }

        if (force.length() > 0) {
            PhysicsSystem.applyForce(this.player, force);
        }
    }

    start(): void {
        let lastTime = 0;
        
        const gameLoop = (currentTime: number) => {
            const deltaTime = (currentTime - lastTime) / 1000;
            lastTime = currentTime;

            this.update(deltaTime);
            this.engine.render();
            
            requestAnimationFrame(gameLoop);
        };

        requestAnimationFrame(gameLoop);
    }
}

// Start the game
async function main() {
    const game = new MyFirstGame();
    
    if (await game.initialize()) {
        game.start();
    } else {
        console.error('Failed to initialize game');
    }
}

main();
```

### 4. Run Your Game

```bash
# Development mode with hot reload
foundry dev

# Build for production
foundry build --platform web

# Build for multiple platforms
foundry build --platform windows,web,android
```

## 🎯 Key Concepts

### Entity Component System (ECS)

Foundry uses an advanced ECS architecture:

```typescript
// Create entity
const entity = world.createEntity();

// Add components
world.addTransformComponent(entity, x, y, z);
world.addMeshComponent(entity, 'model_name');
world.addPhysicsComponent(entity, mass, shape);

// Custom components
world.addComponent(entity, 'Health', { current: 100, max: 100 });
world.addComponent(entity, 'Inventory', { items: [] });
```

### TypeScript Runtime

Foundry's native TypeScript runtime provides:

- **JIT Compilation**: TypeScript compiles to optimized native code
- **Hot Reload**: Instant code updates without restart
- **Zero-Copy Bindings**: Direct memory access between TS and C++

```typescript
// Hot reload automatically updates this function
function updateGameLogic(deltaTime: number) {
    // Your code here - changes apply instantly!
}
```

### Advanced Graphics

```typescript
// Enable ray tracing
engine.enableRayTracing(true);

// Volumetric lighting
const volumetricLight = world.createEntity();
world.addVolumetricLightComponent(volumetricLight, {
    density: 0.1,
    scattering: 0.5,
    color: new Vector3(1, 0.8, 0.6)
});

// NeRF rendering for photorealistic scenes
const nerfScene = world.createEntity();
world.addNeRFComponent(nerfScene, 'captured_scene.nerf');
```

### Physics Simulation

```typescript
// Fluid simulation
const fluid = world.createEntity();
world.addFluidComponent(fluid, {
    particleCount: 10000,
    viscosity: 0.1,
    density: 1000
});

// Cloth physics
const cloth = world.createEntity();
world.addClothComponent(cloth, {
    width: 10,
    height: 10,
    stiffness: 0.8,
    damping: 0.1
});
```

## 🎨 Asset Pipeline

### Importing Assets

```typescript
// Automatic optimization and format conversion
const model = await AssetManager.loadAsset('character.fbx', 'mesh');
const texture = await AssetManager.loadAsset('diffuse.png', 'texture');
const audio = await AssetManager.loadAsset('music.mp3', 'audio');

// Streaming assets for large worlds
const terrain = await AssetManager.streamAsset('terrain_chunk_0_0.mesh');
```

### Procedural Generation

```typescript
import { ProceduralGenerator } from '@foundry/procedural';

// Generate terrain
const terrain = ProceduralGenerator.generateTerrain({
    size: 1000,
    height: 100,
    octaves: 6,
    persistence: 0.5
});

// Generate textures
const texture = ProceduralGenerator.generateTexture({
    type: 'noise',
    size: 512,
    frequency: 0.1
});
```

## 🤖 AI Integration

### Neural Networks

```typescript
import { NeuralNetwork } from '@foundry/ai';

// Create and train network
const network = new NeuralNetwork();
network.addLayer({ neurons: 32, activation: 'relu' });
network.addLayer({ neurons: 16, activation: 'relu' });
network.addLayer({ neurons: 4, activation: 'softmax' });

network.compile({ learningRate: 0.001, optimizer: 'adam' });
network.train(trainingData, labels);

// Use for game AI
const decision = network.predict(gameState);
```

### Behavior Trees

```typescript
import { BehaviorTree } from '@foundry/ai';

const npcBehavior = new BehaviorTree();
const rootNode = npcBehavior.addNode('sequence', 'root');
const patrolNode = npcBehavior.addNode('action', 'patrol');
const attackNode = npcBehavior.addNode('action', 'attack');

npcBehavior.addChild(rootNode, patrolNode);
npcBehavior.addChild(rootNode, attackNode);
```

## 🌐 Multiplayer

### Basic Networking

```typescript
import { NetworkSystem } from '@foundry/networking';

// Host a game
const network = new NetworkSystem();
await network.initialize({ mode: 'host', port: 7777 });

// Register RPCs
network.registerRPC('player_move', (playerId, position) => {
    updatePlayerPosition(playerId, position);
});

// Send data
network.callRPC('player_move', playerId, newPosition);
```

### Client Prediction

```typescript
// Enable prediction for smooth gameplay
network.enablePrediction(true);
network.setRollbackBuffer(60); // 1 second at 60fps

// Prediction automatically handles:
// - Input prediction
// - State rollback
// - Server reconciliation
```

## 📱 Mobile Development

### Adaptive Quality

```typescript
import { MobileSystem, AdaptiveRenderer } from '@foundry/mobile';

const mobile = new MobileSystem();
const renderer = new AdaptiveRenderer();

// Automatic quality scaling based on performance
renderer.enableAdaptiveScaling(true);
renderer.setPerformanceTarget({ targetFPS: 60 });

// Battery optimization
mobile.enableBatteryOptimization(true);
mobile.setPowerMode('adaptive');
```

## 🥽 VR/AR Development

### VR Setup

```typescript
import { VRSystem } from '@foundry/vr';

const vr = new VRSystem();
await vr.initialize({
    platform: 'OpenXR', // Works with all VR headsets
    trackingSpace: 'RoomScale',
    handTracking: 'Mixed'
});

// Handle VR input
const controllers = vr.getControllers();
const leftHand = vr.getLeftHand();
const rightHand = vr.getRightHand();
```

## 🔧 Debugging & Profiling

### Built-in Tools

```typescript
import { Profiler, Debugger } from '@foundry/debug';

// Performance profiling
Profiler.startProfiling();
// ... game code ...
const metrics = Profiler.getMetrics();
console.log(`Frame time: ${metrics.frameTime}ms`);

// Memory debugging
const memoryUsage = Profiler.getMemoryUsage();
console.log(`Memory: ${memoryUsage.current}MB`);

// Visual debugging
Debugger.drawLine(start, end, color);
Debugger.drawSphere(center, radius, color);
```

## 📚 Next Steps

Now that you have your first game running, explore these advanced topics:

1. **[Advanced Graphics](docs/graphics.md)** - Ray tracing, volumetric rendering, NeRF
2. **[Physics Simulation](docs/physics.md)** - Fluids, cloth, soft bodies
3. **[AI & Machine Learning](docs/ai.md)** - Neural networks, behavior trees
4. **[Multiplayer Networking](docs/networking.md)** - Prediction, anti-cheat
5. **[VR/AR Development](docs/vr-ar.md)** - Immersive experiences
6. **[Mobile Optimization](docs/mobile.md)** - Performance and battery life
7. **[Asset Pipeline](docs/assets.md)** - Optimization and streaming

## 🆘 Getting Help

- 📖 **Documentation**: [docs.foundryengine.com](https://docs.foundryengine.com)
- 💬 **Discord**: [discord.gg/foundryengine](https://discord.gg/foundryengine)
- 🐛 **Issues**: [GitHub Issues](https://github.com/foundryengine/foundry/issues)
- 📧 **Email**: support@foundryengine.com

Welcome to the future of game development! 🚀