/**
 * FoundryEngine TypeScript Declarations
 * Auto-generated from C++ WebAssembly bindings
 */

declare module "foundry-engine" {
    // Core math types
    export class Vector3 {
        x: number;
        y: number;
        z: number;
        
        constructor();
        constructor(x: number, y: number, z: number);
        
        add(other: Vector3): Vector3;
        subtract(other: Vector3): Vector3;
        multiply(scalar: number): Vector3;
        dot(other: Vector3): number;
        cross(other: Vector3): Vector3;
        length(): number;
        normalize(): Vector3;
    }

    export class Matrix4 {
        constructor();
        
        multiply(other: Matrix4): Matrix4;
        transformPoint(point: Vector3): Vector3;
        transformDirection(direction: Vector3): Vector3;
        
        static identity(): Matrix4;
        static translation(translation: Vector3): Matrix4;
        static rotation(axis: Vector3, angle: number): Matrix4;
        static scale(scale: Vector3): Matrix4;
    }

    // Engine core
    export class Engine {
        static initialize(): boolean;
        static shutdown(): void;
        static update(deltaTime: number): void;
        static render(): void;
        static getDeltaTime(): number;
        static getFrameCount(): number;
        static isRunning(): boolean;
    }

    // World and ECS
    export class World {
        static createEntity(): number;
        static destroyEntity(entityId: number): void;
        static hasComponent(entityId: number, componentType: string): boolean;
        static addTransformComponent(entityId: number, x: number, y: number, z: number): number;
        static updateTransformComponent(componentId: number, x: number, y: number, z: number): void;
        static removeComponent(entityId: number, componentType: string): void;
    }

    // Scene management
    export class Scene {
        static createScene(name: string): number;
        static setActiveScene(sceneId: number): void;
        static addEntityToScene(sceneId: number, entityId: number): void;
        static removeEntityFromScene(sceneId: number, entityId: number): void;
    }

    // Memory management
    export class Memory {
        static releaseObject(objectId: number): void;
        static getManagedObjectCount(): number;
    }

    // Component types
    export interface TransformComponent {
        position: Vector3;
        rotation: Vector3;
        scale: Vector3;
    }

    export interface RigidBodyComponent {
        mass: number;
        velocity: Vector3;
        angularVelocity: Vector3;
        isKinematic: boolean;
    }

    export interface RenderComponent {
        meshId: string;
        materialId: string;
        visible: boolean;
    }

    export interface AudioComponent {
        soundId: string;
        volume: number;
        pitch: number;
        loop: boolean;
        is3D: boolean;
    }

    // System types
    export interface System {
        update(deltaTime: number): void;
        initialize(): boolean;
        shutdown(): void;
    }

    // Event system
    export interface GameEvent {
        type: string;
        data: any;
        timestamp: number;
    }

    export class EventSystem {
        static emit(event: GameEvent): void;
        static on(eventType: string, callback: (event: GameEvent) => void): void;
        static off(eventType: string, callback: (event: GameEvent) => void): void;
    }

    // Asset management
    export interface Asset {
        id: string;
        type: string;
        path: string;
        loaded: boolean;
        size: number;
    }

    export class AssetManager {
        static loadAsset(path: string, type: string): Promise<Asset>;
        static unloadAsset(assetId: string): void;
        static getAsset(assetId: string): Asset | null;
        static preloadAssets(assetPaths: string[]): Promise<Asset[]>;
    }

    // Input system
    export interface InputState {
        keyboard: { [key: string]: boolean };
        mouse: {
            position: Vector3;
            buttons: { [button: number]: boolean };
            wheel: number;
        };
        gamepad: {
            connected: boolean;
            buttons: { [button: number]: boolean };
            axes: { [axis: number]: number };
        };
    }

    export class InputSystem {
        static getInputState(): InputState;
        static isKeyPressed(key: string): boolean;
        static isMouseButtonPressed(button: number): boolean;
        static getMousePosition(): Vector3;
        static isGamepadConnected(): boolean;
        static getGamepadAxis(axis: number): number;
        static isGamepadButtonPressed(button: number): boolean;
    }

    // Audio system
    export interface AudioClip {
        id: string;
        duration: number;
        volume: number;
        pitch: number;
    }

    export class AudioSystem {
        static playSound(soundId: string, volume?: number, pitch?: number): void;
        static stopSound(soundId: string): void;
        static pauseSound(soundId: string): void;
        static resumeSound(soundId: string): void;
        static setMasterVolume(volume: number): void;
        static setListenerPosition(position: Vector3): void;
        static setListenerOrientation(forward: Vector3, up: Vector3): void;
    }

    // Rendering system
    export interface Camera {
        position: Vector3;
        rotation: Vector3;
        fov: number;
        near: number;
        far: number;
        projection: Matrix4;
        view: Matrix4;
    }

    export interface Light {
        type: 'directional' | 'point' | 'spot';
        position: Vector3;
        direction: Vector3;
        color: Vector3;
        intensity: number;
        range: number;
        spotAngle: number;
    }

    export class RenderSystem {
        static setCamera(camera: Camera): void;
        static addLight(light: Light): void;
        static removeLight(lightId: string): void;
        static setAmbientLight(color: Vector3, intensity: number): void;
        static setSkybox(textureId: string): void;
        static setFog(color: Vector3, density: number, start: number, end: number): void;
    }

    // Physics system
    export interface PhysicsWorld {
        gravity: Vector3;
        timeStep: number;
        iterations: number;
    }

    export class PhysicsSystem {
        static setGravity(gravity: Vector3): void;
        static addRigidBody(entityId: number, mass: number, shape: string): void;
        static removeRigidBody(entityId: number): void;
        static applyForce(entityId: number, force: Vector3): void;
        static applyImpulse(entityId: number, impulse: Vector3): void;
        static setVelocity(entityId: number, velocity: Vector3): void;
        static getVelocity(entityId: number): Vector3;
        static raycast(origin: Vector3, direction: Vector3, maxDistance: number): RaycastHit | null;
    }

    export interface RaycastHit {
        entityId: number;
        point: Vector3;
        normal: Vector3;
        distance: number;
    }

    // Networking
    export interface NetworkMessage {
        type: string;
        data: any;
        reliable: boolean;
        timestamp: number;
    }

    export class NetworkSystem {
        static connect(host: string, port: number): Promise<boolean>;
        static disconnect(): void;
        static isConnected(): boolean;
        static sendMessage(message: NetworkMessage): void;
        static onMessage(callback: (message: NetworkMessage) => void): void;
        static getLatency(): number;
        static getPacketLoss(): number;
    }

    // Utility functions
    export class Utils {
        static lerp(a: number, b: number, t: number): number;
        static clamp(value: number, min: number, max: number): number;
        static degToRad(degrees: number): number;
        static radToDeg(radians: number): number;
        static random(min: number, max: number): number;
        static distance(a: Vector3, b: Vector3): number;
        static angle(a: Vector3, b: Vector3): number;
    }

    // Configuration
    export interface EngineConfig {
        windowWidth: number;
        windowHeight: number;
        title: string;
        fullscreen: boolean;
        vsync: boolean;
        antialiasing: number;
        targetFPS: number;
        enablePhysics: boolean;
        enableAudio: boolean;
        enableNetworking: boolean;
        enableScripting: boolean;
    }

    // Main initialization function
    export function initializeEngine(config?: Partial<EngineConfig>): Promise<boolean>;
    export function shutdownEngine(): void;
    export function getEngineVersion(): string;
    export function getPlatformInfo(): { name: string; version: string; architecture: string };

    // Error handling
    export class EngineError extends Error {
        code: number;
        component: string;
    }

    // Performance monitoring
    export interface PerformanceMetrics {
        frameTime: number;
        fps: number;
        memoryUsage: number;
        drawCalls: number;
        triangles: number;
        gpuMemory: number;
    }

    export class Profiler {
        static getMetrics(): PerformanceMetrics;
        static startProfile(name: string): void;
        static endProfile(name: string): number;
        static getProfileData(): { [name: string]: number };
        static reset(): void;
    }
}

// Global engine instance for convenience
declare global {
    const FoundryEngine: typeof import("foundry-engine");
}
