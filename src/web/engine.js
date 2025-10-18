/**
 * FoundryEngine JavaScript Runtime Bridge
 * Provides high-level JavaScript API for the WebAssembly engine
 */

class FoundryEngineRuntime {
    constructor() {
        this.wasmModule = null;
        this.isInitialized = false;
        this.config = null;
        this.eventCallbacks = new Map();
        this.performanceMetrics = {
            frameTime: 0,
            fps: 0,
            memoryUsage: 0,
            drawCalls: 0,
            triangles: 0,
            gpuMemory: 0
        };
        this.lastFrameTime = 0;
        this.frameCount = 0;
        this.animationFrameId = null;
    }

    /**
     * Initialize the engine with WebAssembly module
     */
    async initialize(config = {}) {
        try {
            // Default configuration
            this.config = {
                windowWidth: 800,
                windowHeight: 600,
                title: "Foundry Game",
                fullscreen: false,
                vsync: true,
                antialiasing: 4,
                targetFPS: 60,
                enablePhysics: true,
                enableAudio: true,
                enableNetworking: false,
                enableScripting: true,
                ...config
            };

            // Load WebAssembly module
            await this.loadWasmModule();

            // Initialize engine
            const success = this.wasmModule.Engine.initialize();
            if (!success) {
                throw new Error('Failed to initialize engine');
            }

            this.isInitialized = true;
            this.lastFrameTime = performance.now();

            // Start main loop
            this.startMainLoop();

            console.log('FoundryEngine initialized successfully');
            return true;
        } catch (error) {
            console.error('Failed to initialize FoundryEngine:', error);
            return false;
        }
    }

    /**
     * Load WebAssembly module
     */
    async loadWasmModule() {
        return new Promise((resolve, reject) => {
            // Try to load from different possible locations
            const possiblePaths = [
                'foundryengine_wasm.js',
                'build/wasm/foundryengine_wasm.js',
                'assets/wasm/foundryengine_wasm.js'
            ];

            let loaded = false;
            for (const path of possiblePaths) {
                try {
                    const script = document.createElement('script');
                    script.src = path;
                    script.onload = () => {
                        if (!loaded) {
                            loaded = true;
                            this.wasmModule = window.FoundryEngineWASM || window.FoundryEngine;
                            if (this.wasmModule) {
                                resolve();
                            } else {
                                reject(new Error(`WASM module not found at ${path}`));
                            }
                        }
                    };
                    script.onerror = () => {
                        if (!loaded && path === possiblePaths[possiblePaths.length - 1]) {
                            reject(new Error(`Failed to load WASM module from any path`));
                        }
                    };
                    document.head.appendChild(script);
                    break;
                } catch (error) {
                    if (path === possiblePaths[possiblePaths.length - 1]) {
                        reject(error);
                    }
                }
            }
        });
    }

    /**
     * Start the main game loop
     */
    startMainLoop() {
        if (!this.isInitialized) return;

        const gameLoop = (currentTime) => {
            if (!this.isInitialized) return;

            const deltaTime = (currentTime - this.lastFrameTime) / 1000.0;
            this.lastFrameTime = currentTime;

            // Update performance metrics
            this.updatePerformanceMetrics(deltaTime);

            // Update engine
            this.wasmModule.Engine.update(deltaTime);

            // Render frame
            this.wasmModule.Engine.render();

            // Emit frame event
            this.emit('frame', { deltaTime, frameCount: this.frameCount++ });

            // Continue loop
            this.animationFrameId = requestAnimationFrame(gameLoop);
        };

        this.animationFrameId = requestAnimationFrame(gameLoop);
    }

    /**
     * Stop the main game loop
     */
    stopMainLoop() {
        if (this.animationFrameId) {
            cancelAnimationFrame(this.animationFrameId);
            this.animationFrameId = null;
        }
    }

    /**
     * Update performance metrics
     */
    updatePerformanceMetrics(deltaTime) {
        this.performanceMetrics.frameTime = deltaTime * 1000; // Convert to milliseconds
        this.performanceMetrics.fps = 1.0 / deltaTime;
        
        // Get memory usage from WASM module if available
        if (this.wasmModule && this.wasmModule.Memory) {
            this.performanceMetrics.memoryUsage = this.wasmModule.Memory.getManagedObjectCount();
        }
    }

    /**
     * Shutdown the engine
     */
    shutdown() {
        this.stopMainLoop();
        
        if (this.isInitialized && this.wasmModule) {
            this.wasmModule.Engine.shutdown();
            this.isInitialized = false;
        }

        this.eventCallbacks.clear();
        console.log('FoundryEngine shutdown');
    }

    /**
     * Event system
     */
    on(eventType, callback) {
        if (!this.eventCallbacks.has(eventType)) {
            this.eventCallbacks.set(eventType, []);
        }
        this.eventCallbacks.get(eventType).push(callback);
    }

    off(eventType, callback) {
        if (this.eventCallbacks.has(eventType)) {
            const callbacks = this.eventCallbacks.get(eventType);
            const index = callbacks.indexOf(callback);
            if (index > -1) {
                callbacks.splice(index, 1);
            }
        }
    }

    emit(eventType, data) {
        if (this.eventCallbacks.has(eventType)) {
            this.eventCallbacks.get(eventType).forEach(callback => {
                try {
                    callback(data);
                } catch (error) {
                    console.error(`Error in event callback for ${eventType}:`, error);
                }
            });
        }
    }

    /**
     * World and Entity management
     */
    createEntity() {
        if (!this.isInitialized) return 0;
        return this.wasmModule.World.createEntity();
    }

    destroyEntity(entityId) {
        if (!this.isInitialized) return;
        this.wasmModule.World.destroyEntity(entityId);
    }

    hasComponent(entityId, componentType) {
        if (!this.isInitialized) return false;
        return this.wasmModule.World.hasComponent(entityId, componentType);
    }

    addTransformComponent(entityId, x = 0, y = 0, z = 0) {
        if (!this.isInitialized) return 0;
        return this.wasmModule.World.addTransformComponent(entityId, x, y, z);
    }

    updateTransformComponent(componentId, x, y, z) {
        if (!this.isInitialized) return;
        this.wasmModule.World.updateTransformComponent(componentId, x, y, z);
    }

    removeComponent(entityId, componentType) {
        if (!this.isInitialized) return;
        this.wasmModule.World.removeComponent(entityId, componentType);
    }

    /**
     * Scene management
     */
    createScene(name) {
        if (!this.isInitialized) return 0;
        return this.wasmModule.Scene.createScene(name);
    }

    setActiveScene(sceneId) {
        if (!this.isInitialized) return;
        this.wasmModule.Scene.setActiveScene(sceneId);
    }

    addEntityToScene(sceneId, entityId) {
        if (!this.isInitialized) return;
        this.wasmModule.Scene.addEntityToScene(sceneId, entityId);
    }

    removeEntityFromScene(sceneId, entityId) {
        if (!this.isInitialized) return;
        this.wasmModule.Scene.removeEntityFromScene(sceneId, entityId);
    }

    /**
     * Memory management
     */
    releaseObject(objectId) {
        if (!this.isInitialized) return;
        this.wasmModule.Memory.releaseObject(objectId);
    }

    getManagedObjectCount() {
        if (!this.isInitialized) return 0;
        return this.wasmModule.Memory.getManagedObjectCount();
    }

    /**
     * Utility functions
     */
    getDeltaTime() {
        if (!this.isInitialized) return 0;
        return this.wasmModule.Engine.getDeltaTime();
    }

    getFrameCount() {
        if (!this.isInitialized) return 0;
        return this.wasmModule.Engine.getFrameCount();
    }

    isRunning() {
        if (!this.isInitialized) return false;
        return this.wasmModule.Engine.isRunning();
    }

    getPerformanceMetrics() {
        return { ...this.performanceMetrics };
    }

    /**
     * Math utilities
     */
    createVector3(x = 0, y = 0, z = 0) {
        if (!this.isInitialized) return null;
        return new this.wasmModule.Vector3(x, y, z);
    }

    createMatrix4() {
        if (!this.isInitialized) return null;
        return new this.wasmModule.Matrix4();
    }

    /**
     * Error handling
     */
    handleError(error) {
        console.error('FoundryEngine Error:', error);
        this.emit('error', error);
    }
}

// Create global instance
window.FoundryEngineRuntime = FoundryEngineRuntime;

// Export for module systems
if (typeof module !== 'undefined' && module.exports) {
    module.exports = FoundryEngineRuntime;
}

// Auto-initialize if script is loaded with data attributes
document.addEventListener('DOMContentLoaded', () => {
    const script = document.querySelector('script[data-foundry-auto-init]');
    if (script) {
        const config = script.dataset.foundryConfig ? JSON.parse(script.dataset.foundryConfig) : {};
        const engine = new FoundryEngineRuntime();
        engine.initialize(config).then(success => {
            if (success) {
                window.foundryEngine = engine;
                console.log('FoundryEngine auto-initialized');
            }
        });
    }
});
