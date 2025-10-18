// TypeScript definitions for Foundry Engine APIs
// This file provides type-safe access to native engine functions

// Math Types
export interface Vector2 {
    x: number;
    y: number;

    add(other: Vector2): Vector2;
    subtract(other: Vector2): Vector2;
    multiply(scalar: number): Vector2;
    divide(scalar: number): Vector2;
    dot(other: Vector2): number;
    cross(other: Vector2): number;
    length(): number;
    normalized(): Vector2;
    distance(other: Vector2): number;
}

export interface Vector3 {
    x: number;
    y: number;
    z: number;

    add(other: Vector3): Vector3;
    subtract(other: Vector3): Vector3;
    multiply(scalar: number): Vector3;
    divide(scalar: number): Vector3;
    dot(other: Vector3): number;
    cross(other: Vector3): Vector3;
    length(): number;
    normalized(): Vector3;
    distance(other: Vector3): number;
    lerp(other: Vector3, t: number): Vector3;
}

export interface Quaternion {
    x: number;
    y: number;
    z: number;
    w: number;

    static identity(): Quaternion;
    static fromEulerAngles(x: number, y: number, z: number): Quaternion;

    multiply(other: Quaternion): Quaternion;
    conjugate(): Quaternion;
    inverse(): Quaternion;
    normalize(): Quaternion;
}

// Engine Core
export interface EngineConfig {
    canvas?: HTMLCanvasElement;
    width?: number;
    height?: number;
    antialias?: boolean;
    vsync?: boolean;
    webglVersion?: number;
    powerPreference?: 'default' | 'high-performance' | 'low-power';
}

export interface FoundryEngine {
    deltaTime: number;
    time: number;
    frameCount: number;

    constructor(config: EngineConfig);

    createScene(name: string): Scene;
    destroyScene(scene: Scene): void;
    getScene(name: string): Scene | null;

    render(scene: Scene, camera: Entity): void;
    update(): void;

    resize(width: number, height: number): void;
    destroy(): void;

    // Event system
    on(event: string, callback: Function): void;
    off(event: string, callback: Function): void;
    emit(event: string, ...args: any[]): void;
}

// Scene Management
export interface Scene {
    name: string;
    entities: Entity[];
    isActive: boolean;

    createEntity(name: string): Entity;
    destroyEntity(entity: Entity): void;
    findEntity(name: string): Entity | null;
    getEntitiesByTag(tag: string): Entity[];

    addSystem(system: System): void;
    removeSystem(system: System): void;
    getSystem<T extends System>(type: new() => T): T | null;
}

// Entity Component System
export interface Entity {
    id: string;
    name: string;
    tag: string;
    isActive: boolean;
    transform: TransformComponent;

    addComponent(type: string, config?: any): Component;
    removeComponent(type: string): void;
    getComponent(type: string): Component | null;
    hasComponent(type: string): boolean;

    addChild(child: Entity): void;
    removeChild(child: Entity): void;
    getChildren(): Entity[];
    getParent(): Entity | null;

    destroy(): void;
}

export interface Component {
    entity: Entity;
    enabled: boolean;

    initialize?(): void;
    update?(deltaTime: number): void;
    destroy?(): void;
}

export interface System {
    priority: number;
    enabled: boolean;

    initialize?(): void;
    update?(deltaTime: number): void;
    destroy?(): void;
}

// Core Components
export interface TransformComponent extends Component {
    position: Vector3;
    rotation: Quaternion;
    scale: Vector3;

    translate(translation: Vector3): void;
    rotate(rotation: Quaternion): void;
    lookAt(target: Vector3, up?: Vector3): void;

    getWorldPosition(): Vector3;
    getWorldRotation(): Quaternion;
    getWorldScale(): Vector3;

    getForward(): Vector3;
    getRight(): Vector3;
    getUp(): Vector3;
}

export interface CameraComponent extends Component {
    fov: number;
    near: number;
    far: number;
    aspectRatio: number;
    clearColor: [number, number, number, number];

    screenToWorld(screenPoint: Vector2): Vector3;
    worldToScreen(worldPoint: Vector3): Vector2;

    setPerspective(fov: number, aspect: number, near: number, far: number): void;
    setOrthographic(left: number, right: number, bottom: number, top: number, near: number, far: number): void;
}

export interface MeshRendererComponent extends Component {
    geometry: string; // 'cube', 'sphere', 'plane', 'custom'
    material: Material;
    castShadows: boolean;
    receiveShadows: boolean;

    setMaterial(material: Material): void;
    setGeometry(geometry: string, customMesh?: Mesh): void;
}

export interface Material {
    color: [number, number, number, number];
    metallic: number;
    roughness: number;
    emissive: [number, number, number];
    texture?: Texture;
    normalMap?: Texture;
    metallicMap?: Texture;
    roughnessMap?: Texture;
    aoMap?: Texture;
    shader?: Shader;
}

export interface Texture {
    width: number;
    height: number;
    format: string;
    data: ArrayBuffer | HTMLImageElement;

    static fromImage(image: HTMLImageElement): Texture;
    static fromUrl(url: string): Promise<Texture>;
}

export interface Shader {
    vertexSource: string;
    fragmentSource: string;
    uniforms: { [key: string]: any };

    setUniform(name: string, value: any): void;
}

export interface Mesh {
    vertices: Float32Array;
    indices: Uint16Array;
    normals?: Float32Array;
    uvs?: Float32Array;
    colors?: Float32Array;

    static createCube(): Mesh;
    static createSphere(radius: number, segments: number): Mesh;
    static createPlane(): Mesh;
}

// Physics Components
export interface RigidBodyComponent extends Component {
    mass: number;
    drag: number;
    angularDrag: number;
    useGravity: boolean;
    isKinematic: boolean;
    velocity: Vector3;
    angularVelocity: Vector3;

    applyForce(force: Vector3): void;
    applyImpulse(impulse: Vector3): void;
    applyTorque(torque: Vector3): void;
    applyTorqueImpulse(torqueImpulse: Vector3): void;

    movePosition(position: Vector3): void;
    moveRotation(rotation: Quaternion): void;
}

export interface ColliderComponent extends Component {
    isTrigger: boolean;
    material: PhysicsMaterial;

    onCollisionEnter?(other: ColliderComponent): void;
    onCollisionStay?(other: ColliderComponent): void;
    onCollisionExit?(other: ColliderComponent): void;

    onTriggerEnter?(other: ColliderComponent): void;
    onTriggerStay?(other: ColliderComponent): void;
    onTriggerExit?(other: ColliderComponent): void;
}

export interface BoxColliderComponent extends ColliderComponent {
    size: Vector3;
    center: Vector3;
}

export interface SphereColliderComponent extends ColliderComponent {
    radius: number;
    center: Vector3;
}

export interface CapsuleColliderComponent extends ColliderComponent {
    radius: number;
    height: number;
    center: Vector3;
}

export interface MeshColliderComponent extends ColliderComponent {
    mesh: Mesh;
    convex: boolean;
}

export interface PhysicsMaterial {
    friction: number;
    restitution: number;
    frictionCombine: 'average' | 'minimum' | 'maximum' | 'multiply';
    restitutionCombine: 'average' | 'minimum' | 'maximum' | 'multiply';
}

// Lighting Components
export interface LightComponent extends Component {
    color: [number, number, number];
    intensity: number;
    castShadows: boolean;
    shadowMapSize: number;
}

export interface DirectionalLightComponent extends LightComponent {
    direction: Vector3;
}

export interface PointLightComponent extends LightComponent {
    range: number;
    attenuation: [number, number, number]; // constant, linear, quadratic
}

export interface SpotLightComponent extends PointLightComponent {
    angle: number;
    penumbra: number;
}

export interface AmbientLightComponent extends LightComponent {
    // Ambient lights don't have additional properties beyond base LightComponent
}

// Audio Components
export interface AudioSourceComponent extends Component {
    clip: AudioClip;
    volume: number;
    pitch: number;
    loop: boolean;
    spatialBlend: number; // 0 = 2D, 1 = 3D
    minDistance: number;
    maxDistance: number;

    play(): void;
    pause(): void;
    stop(): void;
    isPlaying(): boolean;
}

export interface AudioListenerComponent extends Component {
    volume: number;
}

// Asset Management
export interface AudioClip {
    length: number;
    channels: number;
    sampleRate: number;

    static fromUrl(url: string): Promise<AudioClip>;
    static fromArrayBuffer(buffer: ArrayBuffer): AudioClip;
}

// Input System
export interface Input {
    static getKey(keyCode: string): boolean;
    static getKeyDown(keyCode: string): boolean;
    static getKeyUp(keyCode: string): boolean;

    static getMouseButton(button: number): boolean;
    static getMouseButtonDown(button: number): boolean;
    static getMouseButtonUp(button: number): boolean;

    static getMousePosition(): Vector2;
    static getMouseDelta(): Vector2;

    static getAxis(axisName: string): number;

    // Touch input (mobile)
    static getTouchCount(): number;
    static getTouch(index: number): TouchInfo;
}

export interface TouchInfo {
    fingerId: number;
    position: Vector2;
    deltaPosition: Vector2;
    phase: 'began' | 'moved' | 'stationary' | 'ended' | 'canceled';
}

// Networking (calls Go backend)
export interface NetworkManager {
    static connect(url: string): Promise<void>;
    static disconnect(): void;
    static isConnected(): boolean;

    static send(data: any): void;
    static onMessage(callback: (data: any) => void): void;

    // HTTP requests to Go backend
    static get(endpoint: string, params?: any): Promise<any>;
    static post(endpoint: string, data: any): Promise<any>;
    static put(endpoint: string, data: any): Promise<any>;
    static delete(endpoint: string, data: any): Promise<any>;
}

// File System (limited web access)
export interface FileSystem {
    static readFile(path: string): Promise<string>;
    static writeFile(path: string, content: string): Promise<void>;
    static exists(path: string): Promise<boolean>;
    static listDirectory(path: string): Promise<string[]>;
    static createDirectory(path: string): Promise<void>;
    static delete(path: string): Promise<void>;
}

// Utility functions
export interface Utils {
    static random(): number;
    static randomRange(min: number, max: number): number;
    static clamp(value: number, min: number, max: number): number;
    static lerp(a: number, b: number, t: number): number;
    static degToRad(degrees: number): number;
    static radToDeg(radians: number): number;
}

// Declare global Foundry namespace
declare global {
    const FoundryEngine: {
        new(config: EngineConfig): FoundryEngine;
    };

    const Vector2: {
        new(x: number, y: number): Vector2;
    };

    const Vector3: {
        new(x: number, y: number, z: number): Vector3;
    };

    const Quaternion: {
        identity(): Quaternion;
        fromEulerAngles(x: number, y: number, z: number): Quaternion;
        new(x: number, y: number, z: number, w: number): Quaternion;
    };

    const Input: typeof Input;
    const NetworkManager: typeof NetworkManager;
    const FileSystem: typeof FileSystem;
    const Utils: typeof Utils;
}