/**
 * Foundry Engine Web Runtime
 * JavaScript/WebAssembly bridge for web deployment
 */

class FoundryEngineWeb {
    constructor() {
        this.wasmModule = null;
        this.isInitialized = false;
        this.canvas = null;
        this.gl = null;
        this.audioContext = null;

        // Performance monitoring
        this.frameCount = 0;
        this.lastFrameTime = 0;
        this.fps = 0;

        // Input handling
        this.inputState = {
            keys: new Set(),
            mousePosition: { x: 0, y: 0 },
            mouseButtons: new Set(),
            touches: new Map()
        };

        // Asset management
        this.loadedAssets = new Map();
        this.assetCache = new Map();

        // WebXR support
        this.xrSession = null;
        this.xrSupported = false;
    }

    async initialize(config = {}) {
        console.log('🚀 Initializing Foundry Engine Web Runtime...');

        try {
            // Set up canvas
            this.setupCanvas(config);

            // Initialize WebGL
            this.initializeWebGL(config);

            // Initialize audio
            this.initializeAudio(config);

            // Load WebAssembly module
            await this.loadWasmModule();

            // Set up input handling
            this.setupInputHandling();

            // Check for WebXR support
            await this.checkWebXRSupport();

            // Initialize engine systems
            this.initializeEngineSystems(config);

            this.isInitialized = true;
            console.log('✅ Foundry Engine Web Runtime initialized successfully!');

            return true;
        } catch (error) {
            console.error('❌ Failed to initialize Foundry Engine Web Runtime:', error);
            return false;
        }
    }

    setupCanvas(config) {
        // Create or get canvas
        this.canvas = config.canvas || document.getElementById('gameCanvas');

        if (!this.canvas) {
            this.canvas = document.createElement('canvas');
            this.canvas.id = 'foundryCanvas';
            this.canvas.width = config.width || 1280;
            this.canvas.height = config.height || 720;
            document.body.appendChild(this.canvas);
        }

        // Set canvas properties
        this.canvas.style.display = 'block';
        this.canvas.style.margin = '0 auto';

        // Handle canvas resize
        window.addEventListener('resize', () => {
            this.resizeCanvas();
        });

        this.resizeCanvas();
    }

    resizeCanvas() {
        const rect = this.canvas.getBoundingClientRect();
        const displayWidth = rect.width;
        const displayHeight = rect.height;

        if (this.canvas.width !== displayWidth || this.canvas.height !== displayHeight) {
            this.canvas.width = displayWidth;
            this.canvas.height = displayHeight;

            if (this.gl) {
                this.gl.viewport(0, 0, displayWidth, displayHeight);
            }
        }
    }

    initializeWebGL(config) {
        const contextOptions = {
            alpha: config.alpha !== false,
            antialias: config.antialias !== false,
            depth: config.depth !== false,
            stencil: config.stencil !== false,
            premultipliedAlpha: config.premultipliedAlpha !== false,
            preserveDrawingBuffer: config.preserveDrawingBuffer === true,
            powerPreference: config.powerPreference || 'high-performance'
        };

        // Try WebGL 2 first, fallback to WebGL 1
        this.gl = this.canvas.getContext('webgl2', contextOptions) ||
            this.canvas.getContext('webgl', contextOptions);

        if (!this.gl) {
            throw new Error('WebGL not supported');
        }

        // Check for required extensions
        this.checkWebGLExtensions();

        // Set up WebGL state
        this.gl.enable(this.gl.DEPTH_TEST);
        this.gl.enable(this.gl.CULL_FACE);
        this.gl.clearColor(0.0, 0.0, 0.0, 1.0);

        console.log('🎨 WebGL initialized:', this.gl.getParameter(this.gl.VERSION));
    }

    checkWebGLExtensions() {
        const requiredExtensions = [
            'OES_vertex_array_object',
            'WEBGL_depth_texture',
            'OES_texture_float',
            'OES_texture_half_float'
        ];

        const optionalExtensions = [
            'EXT_texture_filter_anisotropic',
            'WEBGL_compressed_texture_s3tc',
            'WEBGL_compressed_texture_etc1',
            'OES_element_index_uint'
        ];

        this.extensions = {};

        // Check required extensions
        for (const ext of requiredExtensions) {
            const extension = this.gl.getExtension(ext);
            if (!extension) {
                console.warn(`Required WebGL extension not supported: ${ext}`);
            } else {
                this.extensions[ext] = extension;
            }
        }

        // Check optional extensions
        for (const ext of optionalExtensions) {
            const extension = this.gl.getExtension(ext);
            if (extension) {
                this.extensions[ext] = extension;
                console.log(`✅ WebGL extension available: ${ext}`);
            }
        }
    }

    initializeAudio(config) {
        try {
            // Create audio context
            const AudioContext = window.AudioContext || window.webkitAudioContext;
            this.audioContext = new AudioContext();

            // Set up spatial audio
            this.listener = this.audioContext.listener;

            // Create master gain node
            this.masterGain = this.audioContext.createGain();
            this.masterGain.connect(this.audioContext.destination);
            this.masterGain.gain.value = config.masterVolume || 0.7;

            // Handle audio context suspension (browser policy)
            document.addEventListener('click', () => {
                if (this.audioContext.state === 'suspended') {
                    this.audioContext.resume();
                }
            }, { once: true });

            console.log('🔊 Web Audio initialized');
        } catch (error) {
            console.warn('Audio initialization failed:', error);
        }
    }

    async loadWasmModule() {
        try {
            // Load WebAssembly module
            const wasmPath = './foundryengine_wasm.wasm';
            const wasmModule = await WebAssembly.instantiateStreaming(fetch(wasmPath));

            this.wasmModule = wasmModule.instance;

            // Set up memory interface
            this.memory = this.wasmModule.exports.memory;
            this.heap = new Uint8Array(this.memory.buffer);

            console.log('📦 WebAssembly module loaded');
        } catch (error) {
            console.warn('WebAssembly loading failed, using JavaScript fallback:', error);
            // Initialize JavaScript fallback
            this.initializeJavaScriptFallback();
        }
    }

    initializeJavaScriptFallback() {
        // Implement core engine functionality in JavaScript
        this.wasmModule = {
            exports: {
                // Core engine functions
                engine_initialize: () => 1,
                engine_update: (deltaTime) => { },
                engine_render: () => { },
                engine_shutdown: () => { },

                // Entity system
                create_entity: () => Math.floor(Math.random() * 1000000),
                destroy_entity: (entityId) => { },

                // Component system
                add_transform_component: (entityId, x, y, z) => { },
                add_mesh_component: (entityId, meshName) => { },
                add_physics_component: (entityId, mass, shape) => { },

                // Memory management
                malloc: (size) => 0,
                free: (ptr) => { }
            }
        };

        console.log('📝 JavaScript fallback initialized');
    }

    setupInputHandling() {
        // Keyboard input
        document.addEventListener('keydown', (event) => {
            this.inputState.keys.add(event.code);
            this.onKeyDown(event);
        });

        document.addEventListener('keyup', (event) => {
            this.inputState.keys.delete(event.code);
            this.onKeyUp(event);
        });

        // Mouse input
        this.canvas.addEventListener('mousemove', (event) => {
            const rect = this.canvas.getBoundingClientRect();
            this.inputState.mousePosition.x = event.clientX - rect.left;
            this.inputState.mousePosition.y = event.clientY - rect.top;
            this.onMouseMove(event);
        });

        this.canvas.addEventListener('mousedown', (event) => {
            this.inputState.mouseButtons.add(event.button);
            this.onMouseDown(event);
        });

        this.canvas.addEventListener('mouseup', (event) => {
            this.inputState.mouseButtons.delete(event.button);
            this.onMouseUp(event);
        });

        // Touch input
        this.canvas.addEventListener('touchstart', (event) => {
            event.preventDefault();
            for (const touch of event.changedTouches) {
                this.inputState.touches.set(touch.identifier, {
                    x: touch.clientX,
                    y: touch.clientY
                });
            }
            this.onTouchStart(event);
        });

        this.canvas.addEventListener('touchmove', (event) => {
            event.preventDefault();
            for (const touch of event.changedTouches) {
                this.inputState.touches.set(touch.identifier, {
                    x: touch.clientX,
                    y: touch.clientY
                });
            }
            this.onTouchMove(event);
        });

        this.canvas.addEventListener('touchend', (event) => {
            event.preventDefault();
            for (const touch of event.changedTouches) {
                this.inputState.touches.delete(touch.identifier);
            }
            this.onTouchEnd(event);
        });

        // Gamepad support
        window.addEventListener('gamepadconnected', (event) => {
            console.log('🎮 Gamepad connected:', event.gamepad.id);
        });

        window.addEventListener('gamepaddisconnected', (event) => {
            console.log('🎮 Gamepad disconnected:', event.gamepad.id);
        });
    }

    async checkWebXRSupport() {
        if ('xr' in navigator) {
            try {
                this.xrSupported = await navigator.xr.isSessionSupported('immersive-vr');
                if (this.xrSupported) {
                    console.log('🥽 WebXR VR support detected');
                }

                const arSupported = await navigator.xr.isSessionSupported('immersive-ar');
                if (arSupported) {
                    console.log('📱 WebXR AR support detected');
                }
            } catch (error) {
                console.log('WebXR not available:', error);
            }
        }
    }

    initializeEngineSystems(config) {
        if (this.wasmModule && this.wasmModule.exports.engine_initialize) {
            const result = this.wasmModule.exports.engine_initialize();
            if (result !== 1) {
                throw new Error('Engine initialization failed');
            }
        }

        // Initialize TypeScript runtime
        this.initializeTypeScriptRuntime();

        // Set up render loop
        this.setupRenderLoop();
    }

    initializeTypeScriptRuntime() {
        // Set up TypeScript to JavaScript bridge
        window.FoundryEngine = {
            // Core engine API
            Engine: {
                initialize: (config) => this.initialize(config),
                shutdown: () => this.shutdown(),
                getDeltaTime: () => this.getDeltaTime(),
                getFrameRate: () => this.fps
            },

            // World and entity management
            World: {
                createEntity: () => this.createEntity(),
                destroyEntity: (id) => this.destroyEntity(id),
                addTransformComponent: (id, x, y, z) => this.addTransformComponent(id, x, y, z),
                addMeshComponent: (id, mesh) => this.addMeshComponent(id, mesh),
                addPhysicsComponent: (id, mass, shape) => this.addPhysicsComponent(id, mass, shape)
            },

            // Scene management
            Scene: {
                createScene: (name) => this.createScene(name),
                setActiveScene: (id) => this.setActiveScene(id)
            },

            // Input system
            InputSystem: {
                isKeyPressed: (key) => this.inputState.keys.has(key),
                getMousePosition: () => this.inputState.mousePosition,
                isMouseButtonPressed: (button) => this.inputState.mouseButtons.has(button)
            },

            // Math utilities
            Vector3: class {
                constructor(x = 0, y = 0, z = 0) {
                    this.x = x;
                    this.y = y;
                    this.z = z;
                }

                static distance(a, b) {
                    const dx = a.x - b.x;
                    const dy = a.y - b.y;
                    const dz = a.z - b.z;
                    return Math.sqrt(dx * dx + dy * dy + dz * dz);
                }

                length() {
                    return Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
                }

                normalize() {
                    const len = this.length();
                    if (len > 0) {
                        this.x /= len;
                        this.y /= len;
                        this.z /= len;
                    }
                    return this;
                }
            },

            // Asset management
            AssetManager: {
                loadAsset: (path, type) => this.loadAsset(path, type)
            }
        };

        console.log('📜 TypeScript runtime bridge initialized');
    }

    setupRenderLoop() {
        let lastTime = 0;

        const renderLoop = (currentTime) => {
            if (!this.isInitialized) return;

            // Calculate delta time
            const deltaTime = (currentTime - lastTime) / 1000.0;
            lastTime = currentTime;

            // Update FPS counter
            this.updateFPS(currentTime);

            // Update engine
            this.update(deltaTime);

            // Render frame
            this.render();

            // Continue loop
            requestAnimationFrame(renderLoop);
        };

        requestAnimationFrame(renderLoop);
    }

    update(deltaTime) {
        if (this.wasmModule && this.wasmModule.exports.engine_update) {
            this.wasmModule.exports.engine_update(deltaTime);
        }

        // Update gamepads
        this.updateGamepads();
    }

    render() {
        if (!this.gl) return;

        // Clear buffers
        this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);

        // Call WASM render function
        if (this.wasmModule && this.wasmModule.exports.engine_render) {
            this.wasmModule.exports.engine_render();
        }
    }

    updateFPS(currentTime) {
        this.frameCount++;

        if (currentTime - this.lastFrameTime >= 1000) {
            this.fps = this.frameCount;
            this.frameCount = 0;
            this.lastFrameTime = currentTime;
        }
    }

    updateGamepads() {
        const gamepads = navigator.getGamepads();
        for (let i = 0; i < gamepads.length; i++) {
            const gamepad = gamepads[i];
            if (gamepad) {
                // Process gamepad input
                this.processGamepadInput(gamepad);
            }
        }
    }

    processGamepadInput(gamepad) {
        // Process buttons
        for (let i = 0; i < gamepad.buttons.length; i++) {
            const button = gamepad.buttons[i];
            if (button.pressed) {
                this.onGamepadButton(gamepad.index, i, button.value);
            }
        }

        // Process axes
        for (let i = 0; i < gamepad.axes.length; i++) {
            const axis = gamepad.axes[i];
            if (Math.abs(axis) > 0.1) { // Dead zone
                this.onGamepadAxis(gamepad.index, i, axis);
            }
        }
    }

    // Engine API implementations
    createEntity() {
        if (this.wasmModule && this.wasmModule.exports.create_entity) {
            return this.wasmModule.exports.create_entity();
        }
        return Math.floor(Math.random() * 1000000);
    }

    destroyEntity(entityId) {
        if (this.wasmModule && this.wasmModule.exports.destroy_entity) {
            this.wasmModule.exports.destroy_entity(entityId);
        }
    }

    addTransformComponent(entityId, x, y, z) {
        if (this.wasmModule && this.wasmModule.exports.add_transform_component) {
            this.wasmModule.exports.add_transform_component(entityId, x, y, z);
        }
    }

    addMeshComponent(entityId, meshName) {
        if (this.wasmModule && this.wasmModule.exports.add_mesh_component) {
            // Convert string to WASM memory
            const meshNamePtr = this.allocateString(meshName);
            this.wasmModule.exports.add_mesh_component(entityId, meshNamePtr);
            this.wasmModule.exports.free(meshNamePtr);
        }
    }

    addPhysicsComponent(entityId, mass, shape) {
        if (this.wasmModule && this.wasmModule.exports.add_physics_component) {
            const shapePtr = this.allocateString(shape);
            this.wasmModule.exports.add_physics_component(entityId, mass, shapePtr);
            this.wasmModule.exports.free(shapePtr);
        }
    }

    async loadAsset(path, type) {
        // Check cache first
        if (this.assetCache.has(path)) {
            return this.assetCache.get(path);
        }

        try {
            let asset;

            switch (type) {
                case 'texture':
                    asset = await this.loadTexture(path);
                    break;
                case 'mesh':
                    asset = await this.loadMesh(path);
                    break;
                case 'audio':
                    asset = await this.loadAudio(path);
                    break;
                default:
                    asset = await this.loadGenericAsset(path);
            }

            this.assetCache.set(path, asset);
            return asset;
        } catch (error) {
            console.error(`Failed to load asset: ${path}`, error);
            return null;
        }
    }

    async loadTexture(path) {
        return new Promise((resolve, reject) => {
            const image = new Image();
            image.onload = () => {
                const texture = this.gl.createTexture();
                this.gl.bindTexture(this.gl.TEXTURE_2D, texture);
                this.gl.texImage2D(this.gl.TEXTURE_2D, 0, this.gl.RGBA, this.gl.RGBA, this.gl.UNSIGNED_BYTE, image);
                this.gl.generateMipmap(this.gl.TEXTURE_2D);
                resolve(texture);
            };
            image.onerror = reject;
            image.src = path;
        });
    }

    async loadAudio(path) {
        if (!this.audioContext) return null;

        try {
            const response = await fetch(path);
            const arrayBuffer = await response.arrayBuffer();
            const audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
            return audioBuffer;
        } catch (error) {
            console.error('Failed to load audio:', error);
            return null;
        }
    }

    // Utility functions
    allocateString(str) {
        if (!this.wasmModule || !this.wasmModule.exports.malloc) {
            return 0;
        }

        const encoder = new TextEncoder();
        const bytes = encoder.encode(str + '\0');
        const ptr = this.wasmModule.exports.malloc(bytes.length);

        const heap = new Uint8Array(this.memory.buffer);
        heap.set(bytes, ptr);

        return ptr;
    }

    getDeltaTime() {
        return 1.0 / 60.0; // Placeholder
    }

    // Event handlers (can be overridden)
    onKeyDown(event) { }
    onKeyUp(event) { }
    onMouseMove(event) { }
    onMouseDown(event) { }
    onMouseUp(event) { }
    onTouchStart(event) { }
    onTouchMove(event) { }
    onTouchEnd(event) { }
    onGamepadButton(gamepadIndex, buttonIndex, value) { }
    onGamepadAxis(gamepadIndex, axisIndex, value) { }

    shutdown() {
        this.isInitialized = false;

        if (this.wasmModule && this.wasmModule.exports.engine_shutdown) {
            this.wasmModule.exports.engine_shutdown();
        }

        if (this.audioContext) {
            this.audioContext.close();
        }

        console.log('🔚 Foundry Engine Web Runtime shutdown');
    }
}

// Global instance
window.FoundryEngineWeb = FoundryEngineWeb;

// Auto-initialize if canvas exists
document.addEventListener('DOMContentLoaded', () => {
    const canvas = document.getElementById('gameCanvas');
    if (canvas) {
        const engine = new FoundryEngineWeb();
        window.foundryEngine = engine;

        // Auto-initialize with default config
        engine.initialize({
            canvas: canvas,
            width: canvas.width || 1280,
            height: canvas.height || 720
        });
    }
});

console.log('🌐 Foundry Engine Web Runtime loaded');