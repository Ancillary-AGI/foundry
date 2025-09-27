//
//  iOSPlatform.swift
//  GameEngine iOS Platform
//
//  Swift wrapper for iOS platform implementation
//

import Foundation
import UIKit
import Metal
import MetalKit
import GameController
import AVFoundation

// MARK: - Platform Capabilities
struct iOSPlatformCapabilities {
    var hasMetal: Bool
    var hasOpenGL: Bool
    var hasVulkan: Bool
    var maxTextureSize: Int
    var renderer: String
    var vendor: String
    var version: String
}

// MARK: - Input Types
struct iOSTouchPoint {
    var x: Float
    var y: Float
    var id: Int
}

struct iOSButtonState {
    var pressed: Bool
    var value: Float
}

struct iOSGamepadState {
    var connected: Bool
    var id: String
    var buttons: [iOSButtonState]
    var axes: [Float]
}

struct iOSMouseState {
    var x: Float
    var y: Float
    var buttons: [Bool]
}

// MARK: - iOS Platform Implementation
class iOSPlatform {
    private var engine: UnsafeMutableRawPointer?
    private var capabilities: iOSPlatformCapabilities
    private var view: UIView?
    private var metalDevice: MTLDevice?
    private var commandQueue: MTLCommandQueue?

    // Input state
    private var keyboardState: [Int: Bool] = [:]
    private var mouseState = iOSMouseState(x: 0, y: 0, buttons: [])
    private var touchState: [iOSTouchPoint] = []
    private var gamepadStates: [Int: iOSGamepadState] = [:]

    // MARK: - Initialization
    init(view: UIView) {
        self.view = view
        self.capabilities = detectCapabilities()

        // Initialize Metal if available
        if capabilities.hasMetal {
            metalDevice = MTLCreateSystemDefaultDevice()
            commandQueue = metalDevice?.makeCommandQueue()
        }

        // Setup input handling
        setupInputHandling()

        // Create engine instance
        engine = GameEngineCreate()
    }

    deinit {
        if let engine = engine {
            GameEngineDestroy(engine)
        }
    }

    // MARK: - Platform Interface
    func initialize() -> Bool {
        guard let engine = engine else { return false }

        var cCapabilities = PlatformCapabilities(
            hasMetal: capabilities.hasMetal,
            hasOpenGL: capabilities.hasOpenGL,
            hasVulkan: capabilities.hasVulkan,
            maxTextureSize: Int32(capabilities.maxTextureSize),
            renderer: capabilities.renderer,
            vendor: capabilities.vendor,
            version: capabilities.version
        )

        return GameEngineInitialize(engine, cCapabilities)
    }

    func start() {
        guard let engine = engine else { return }
        GameEngineStart(engine)
    }

    func stop() {
        guard let engine = engine else { return }
        GameEngineStop(engine)
    }

    func update(deltaTime: TimeInterval) {
        guard let engine = engine else { return }

        // Update input state
        updateInputState()

        GameEngineUpdate(engine, deltaTime)
    }

    func render() {
        guard let engine = engine else { return }
        GameEngineRender(engine)
    }

    // MARK: - Entity Management
    func createEntity(name: String) -> UInt32 {
        guard let engine = engine else { return 0 }
        return GameEngineCreateEntity(engine, name)
    }

    func destroyEntity(entityId: UInt32) {
        guard let engine = engine else { return }
        GameEngineDestroyEntity(engine, entityId)
    }

    // MARK: - Component Management
    func addTransformComponent(entityId: UInt32, x: Float, y: Float, z: Float) {
        guard let engine = engine else { return }
        GameEngineAddTransformComponent(engine, entityId, x, y, z)
    }

    func addRenderComponent(entityId: UInt32, meshData: Data, materialData: Data) {
        guard let engine = engine else { return }
        meshData.withUnsafeBytes { meshPtr in
            materialData.withUnsafeBytes { materialPtr in
                GameEngineAddRenderComponent(engine, entityId,
                    meshPtr.baseAddress?.assumingMemoryBound(to: Int8.self),
                    Int32(meshData.count),
                    materialPtr.baseAddress?.assumingMemoryBound(to: Int8.self),
                    Int32(materialData.count))
            }
        }
    }

    func addNetworkComponent(entityId: UInt32) {
        guard let engine = engine else { return }
        GameEngineAddNetworkComponent(engine, entityId)
    }

    // MARK: - Networking
    func startServer(address: String, port: Int, maxClients: Int) {
        guard let engine = engine else { return }
        GameEngineStartServer(engine, address, Int32(port), Int32(maxClients))
    }

    func startClient(address: String, port: Int) {
        guard let engine = engine else { return }
        GameEngineStartClient(engine, address, Int32(port))
    }

    func stopNetworking() {
        guard let engine = engine else { return }
        GameEngineStopNetworking(engine)
    }

    func isNetworkConnected() -> Bool {
        guard let engine = engine else { return false }
        return GameEngineIsNetworkConnected(engine)
    }

    // MARK: - Scene Management
    func createScene(name: String) -> UInt32 {
        guard let engine = engine else { return 0 }
        return GameEngineCreateScene(engine, name)
    }

    func setCurrentScene(sceneId: UInt32) {
        guard let engine = engine else { return }
        GameEngineSetCurrentScene(engine, sceneId)
    }

    func addEntityToScene(sceneId: UInt32, entityId: UInt32) {
        guard let engine = engine else { return }
        GameEngineAddEntityToScene(engine, sceneId, entityId)
    }

    // MARK: - Resource Management
    func loadTexture(path: String) -> UInt32 {
        guard let engine = engine else { return 0 }
        return GameEngineLoadTexture(engine, path)
    }

    func loadMesh(path: String) -> UInt32 {
        guard let engine = engine else { return 0 }
        return GameEngineLoadMesh(engine, path)
    }

    func loadShader(vertexPath: String, fragmentPath: String) -> UInt32 {
        guard let engine = engine else { return 0 }
        return GameEngineLoadShader(engine, vertexPath, fragmentPath)
    }

    // MARK: - Audio
    func playSound(soundId: UInt32, volume: Float, loop: Bool) {
        guard let engine = engine else { return }
        GameEnginePlaySound(engine, soundId, volume, loop)
    }

    func stopSound(soundId: UInt32) {
        guard let engine = engine else { return }
        GameEngineStopSound(engine, soundId)
    }

    // MARK: - Physics
    func setGravity(x: Float, y: Float, z: Float) {
        guard let engine = engine else { return }
        GameEngineSetGravity(engine, x, y, z)
    }

    func createRigidBody(entityId: UInt32, mass: Float) -> UInt32 {
        guard let engine = engine else { return 0 }
        return GameEngineCreateRigidBody(engine, entityId, mass)
    }

    func applyForce(bodyId: UInt32, x: Float, y: Float, z: Float) {
        guard let engine = engine else { return }
        GameEngineApplyForce(engine, bodyId, x, y, z)
    }

    // MARK: - AI
    func createPath(startX: Float, startY: Float, endX: Float, endY: Float) -> UInt32 {
        guard let engine = engine else { return 0 }
        return GameEngineCreatePath(engine, startX, startY, endX, endY)
    }

    func updateAI(deltaTime: TimeInterval) {
        guard let engine = engine else { return }
        GameEngineUpdateAI(engine, deltaTime)
    }

    // MARK: - Performance Monitoring
    func getFPS() -> Double {
        guard let engine = engine else { return 0.0 }
        return GameEngineGetFPS(engine)
    }

    func getFrameTime() -> Double {
        guard let engine = engine else { return 0.0 }
        return GameEngineGetFrameTime(engine)
    }

    // MARK: - Private Methods
    private func detectCapabilities() -> iOSPlatformCapabilities {
        let device = UIDevice.current
        let screen = UIScreen.main

        return iOSPlatformCapabilities(
            hasMetal: MTLCreateSystemDefaultDevice() != nil,
            hasOpenGL: true, // iOS supports OpenGL ES
            hasVulkan: false, // iOS doesn't support Vulkan
            maxTextureSize: 4096, // Conservative estimate
            renderer: "Apple GPU",
            vendor: "Apple Inc.",
            version: device.systemVersion
        )
    }

    private func setupInputHandling() {
        // Setup keyboard handling
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(keyboardWillShow),
            name: UIResponder.keyboardWillShowNotification,
            object: nil
        )

        NotificationCenter.default.addObserver(
            self,
            selector: #selector(keyboardWillHide),
            name: UIResponder.keyboardWillHideNotification,
            object: nil
        )

        // Setup game controller handling
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(controllerDidConnect),
            name: .GCControllerDidConnect,
            object: nil
        )

        NotificationCenter.default.addObserver(
            self,
            selector: #selector(controllerDidDisconnect),
            name: .GCControllerDidDisconnect,
            object: nil
        )

        // Discover existing controllers
        GCController.startWirelessControllerDiscovery(completionHandler: nil)
    }

    private func updateInputState() {
        guard let engine = engine else { return }

        // Update keyboard state
        for (keyCode, pressed) in keyboardState {
            GameEngineSetKeyboardState(engine, Int32(keyCode), pressed)
        }

        // Update mouse state (if applicable)
        let mouseButtons = mouseState.buttons.map { $0 }
        GameEngineSetMouseState(engine, mouseState.x, mouseState.y,
                               UnsafeMutablePointer(mutating: mouseButtons),
                               Int32(mouseButtons.count))

        // Update touch state
        let cTouches = touchState.map { touch in
            TouchPoint(x: touch.x, y: touch.y, id: Int32(touch.id))
        }
        GameEngineSetTouchState(engine,
                               UnsafeMutablePointer(mutating: cTouches),
                               Int32(cTouches.count))

        // Update gamepad states
        for (index, gamepadState) in gamepadStates {
            let cButtons = gamepadState.buttons.map { button in
                ButtonState(pressed: button.pressed, value: button.value)
            }
            let cAxes = gamepadState.axes

            var cGamepadState = GamepadState(
                connected: gamepadState.connected,
                id: gamepadState.id,
                buttons: UnsafeMutablePointer(mutating: cButtons),
                buttonCount: Int32(cButtons.count),
                axes: UnsafeMutablePointer(mutating: cAxes),
                axisCount: Int32(cAxes.count)
            )

            GameEngineSetGamepadState(engine, Int32(index), cGamepadState)
        }
    }

    // MARK: - Notification Handlers
    @objc private func keyboardWillShow(notification: Notification) {
        // Handle keyboard show
    }

    @objc private func keyboardWillHide(notification: Notification) {
        // Handle keyboard hide
    }

    @objc private func controllerDidConnect(notification: Notification) {
        guard let controller = notification.object as? GCController else { return }

        let index = gamepadStates.count
        gamepadStates[index] = iOSGamepadState(
            connected: true,
            id: controller.vendorName ?? "Unknown Controller",
            buttons: [], // Will be populated during updates
            axes: []     // Will be populated during updates
        )

        // Setup controller input handling
        setupControllerInput(controller: controller, index: index)
    }

    @objc private func controllerDidDisconnect(notification: Notification) {
        guard let controller = notification.object as? GCController else { return }

        // Find and remove the disconnected controller
        for (index, state) in gamepadStates {
            if state.id == (controller.vendorName ?? "Unknown Controller") {
                gamepadStates.removeValue(forKey: index)
                break
            }
        }
    }

    private func setupControllerInput(controller: GCController, index: Int) {
        // Setup button handlers
        controller.extendedGamepad?.buttonA.pressedChangedHandler = { [weak self] button, value, pressed in
            self?.updateGamepadButton(index: index, buttonIndex: 0, pressed: pressed, value: value)
        }

        controller.extendedGamepad?.buttonB.pressedChangedHandler = { [weak self] button, value, pressed in
            self?.updateGamepadButton(index: index, buttonIndex: 1, pressed: pressed, value: value)
        }

        controller.extendedGamepad?.buttonX.pressedChangedHandler = { [weak self] button, value, pressed in
            self?.updateGamepadButton(index: index, buttonIndex: 2, pressed: pressed, value: value)
        }

        controller.extendedGamepad?.buttonY.pressedChangedHandler = { [weak self] button, value, pressed in
            self?.updateGamepadButton(index: index, buttonIndex: 3, pressed: pressed, value: value)
        }

        // Setup analog stick handlers
        controller.extendedGamepad?.leftThumbstick.valueChangedHandler = { [weak self] thumbstick, xValue, yValue in
            self?.updateGamepadAxis(index: index, axisIndex: 0, value: xValue)
            self?.updateGamepadAxis(index: index, axisIndex: 1, value: yValue)
        }

        controller.extendedGamepad?.rightThumbstick.valueChangedHandler = { [weak self] thumbstick, xValue, yValue in
            self?.updateGamepadAxis(index: index, axisIndex: 2, value: xValue)
            self?.updateGamepadAxis(index: index, axisIndex: 3, value: yValue)
        }
    }

    private func updateGamepadButton(index: Int, buttonIndex: Int, pressed: Bool, value: Float) {
        guard var gamepadState = gamepadStates[index] else { return }

        if buttonIndex >= gamepadState.buttons.count {
            gamepadState.buttons.append(contentsOf: repeatElement(iOSButtonState(pressed: false, value: 0.0),
                                                                count: buttonIndex - gamepadState.buttons.count + 1))
        }

        gamepadState.buttons[buttonIndex] = iOSButtonState(pressed: pressed, value: value)
        gamepadStates[index] = gamepadState
    }

    private func updateGamepadAxis(index: Int, axisIndex: Int, value: Float) {
        guard var gamepadState = gamepadStates[index] else { return }

        if axisIndex >= gamepadState.axes.count {
            gamepadState.axes.append(contentsOf: repeatElement(0.0,
                                                             count: axisIndex - gamepadState.axes.count + 1))
        }

        gamepadState.axes[axisIndex] = value
        gamepadStates[index] = gamepadState
    }

    // MARK: - Metal Rendering
    func createMetalView() -> MTKView? {
        guard let device = metalDevice, let view = view else { return nil }

        let metalView = MTKView(frame: view.bounds, device: device)
        metalView.clearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
        metalView.colorPixelFormat = .bgra8Unorm
        metalView.depthStencilPixelFormat = .depth32Float

        return metalView
    }
}
