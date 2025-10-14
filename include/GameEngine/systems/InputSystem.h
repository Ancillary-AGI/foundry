#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include "../math/Vector2.h"

namespace FoundryEngine {

enum class KeyCode {
    Unknown = 0,
    A = 4, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num1 = 30, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    Return = 40, Escape, Backspace, Tab, Space,
    F1 = 58, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Left = 80, Down, Right, Up,
    LeftShift = 225, LeftCtrl = 224, LeftAlt = 226,
    RightShift = 229, RightCtrl = 228, RightAlt = 230
};

enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    X1 = 3,
    X2 = 4
};

enum class GamepadButton {
    A = 0, B, X, Y,
    LeftBumper, RightBumper,
    Back, Start, Guide,
    LeftStick, RightStick,
    DPadUp, DPadDown, DPadLeft, DPadRight
};

enum class GamepadAxis {
    LeftX = 0, LeftY,
    RightX, RightY,
    LeftTrigger, RightTrigger
};

struct TouchPoint {
    int id;
    Vector2 position;
    Vector2 deltaPosition;
    float pressure;
    bool active;
};

struct GamepadState {
    bool connected;
    std::string name;
    std::vector<bool> buttons;
    std::vector<float> axes;
    float leftTrigger;
    float rightTrigger;
    Vector2 leftStick;
    Vector2 rightStick;
    bool leftStickPressed;
    bool rightStickPressed;
};

class InputAction {
public:
    virtual ~InputAction() = default;
    virtual bool isPressed() const = 0;
    virtual bool wasPressed() const = 0;
    virtual bool wasReleased() const = 0;
    virtual float getValue() const = 0;
    virtual Vector2 getVector2() const = 0;
};

class InputBinding {
public:
    virtual ~InputBinding() = default;
    virtual void addKeyBinding(KeyCode key) = 0;
    virtual void addMouseBinding(MouseButton button) = 0;
    virtual void addGamepadBinding(int gamepadId, GamepadButton button) = 0;
    virtual void addGamepadAxisBinding(int gamepadId, GamepadAxis axis) = 0;
    virtual void removeKeyBinding(KeyCode key) = 0;
    virtual void removeMouseBinding(MouseButton button) = 0;
    virtual void removeGamepadBinding(int gamepadId, GamepadButton button) = 0;
    virtual void removeGamepadAxisBinding(int gamepadId, GamepadAxis axis) = 0;
    virtual void clearBindings() = 0;
};

class InputManager {
public:
    virtual ~InputManager() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;
    
    // Keyboard input
    virtual bool isKeyPressed(KeyCode key) const = 0;
    virtual bool wasKeyPressed(KeyCode key) const = 0;
    virtual bool wasKeyReleased(KeyCode key) const = 0;
    virtual std::vector<KeyCode> getPressedKeys() const = 0;
    
    // Mouse input
    virtual bool isMouseButtonPressed(MouseButton button) const = 0;
    virtual bool wasMouseButtonPressed(MouseButton button) const = 0;
    virtual bool wasMouseButtonReleased(MouseButton button) const = 0;
    virtual Vector2 getMousePosition() const = 0;
    virtual Vector2 getMouseDelta() const = 0;
    virtual float getMouseWheel() const = 0;
    virtual void setMousePosition(const Vector2& position) = 0;
    virtual void setMouseVisible(bool visible) = 0;
    virtual bool isMouseVisible() const = 0;
    virtual void setMouseLocked(bool locked) = 0;
    virtual bool isMouseLocked() const = 0;
    
    // Touch input
    virtual std::vector<TouchPoint> getTouchPoints() const = 0;
    virtual TouchPoint getTouchPoint(int id) const = 0;
    virtual int getTouchCount() const = 0;
    virtual bool isTouchSupported() const = 0;
    
    // Gamepad input
    virtual int getGamepadCount() const = 0;
    virtual bool isGamepadConnected(int gamepadId) const = 0;
    virtual GamepadState getGamepadState(int gamepadId) const = 0;
    virtual bool isGamepadButtonPressed(int gamepadId, GamepadButton button) const = 0;
    virtual bool wasGamepadButtonPressed(int gamepadId, GamepadButton button) const = 0;
    virtual bool wasGamepadButtonReleased(int gamepadId, GamepadButton button) const = 0;
    virtual float getGamepadAxis(int gamepadId, GamepadAxis axis) const = 0;
    virtual void setGamepadVibration(int gamepadId, float leftMotor, float rightMotor, float duration = 0.0f) = 0;
    virtual std::string getGamepadName(int gamepadId) const = 0;
    
    // Input actions and bindings
    virtual InputAction* createAction(const std::string& name) = 0;
    virtual void destroyAction(const std::string& name) = 0;
    virtual InputAction* getAction(const std::string& name) = 0;
    virtual InputBinding* getBinding(const std::string& actionName) = 0;
    
    // Input mapping
    virtual void loadInputMap(const std::string& path) = 0;
    virtual void saveInputMap(const std::string& path) = 0;
    virtual void setInputMap(const std::string& mapName) = 0;
    virtual std::string getCurrentInputMap() const = 0;
    virtual std::vector<std::string> getAvailableInputMaps() const = 0;
    
    // Text input
    virtual void startTextInput() = 0;
    virtual void stopTextInput() = 0;
    virtual bool isTextInputActive() const = 0;
    virtual std::string getTextInput() const = 0;
    virtual void clearTextInput() = 0;
    
    // Callbacks
    virtual void setKeyCallback(std::function<void(KeyCode, bool)> callback) = 0;
    virtual void setMouseButtonCallback(std::function<void(MouseButton, bool, Vector2)> callback) = 0;
    virtual void setMouseMoveCallback(std::function<void(Vector2, Vector2)> callback) = 0;
    virtual void setMouseWheelCallback(std::function<void(float)> callback) = 0;
    virtual void setTouchCallback(std::function<void(const TouchPoint&)> callback) = 0;
    virtual void setGamepadConnectedCallback(std::function<void(int, bool)> callback) = 0;
    virtual void setTextInputCallback(std::function<void(const std::string&)> callback) = 0;
    
    // Input recording and playback
    virtual void startRecording(const std::string& filename) = 0;
    virtual void stopRecording() = 0;
    virtual bool isRecording() const = 0;
    virtual void startPlayback(const std::string& filename) = 0;
    virtual void stopPlayback() = 0;
    virtual bool isPlayingBack() const = 0;
    
    // Platform-specific
    virtual void handlePlatformEvent(void* event) = 0;
    virtual void setWindowHandle(void* windowHandle) = 0;
};

class DefaultInputManager : public InputManager {
public:
    bool initialize() override;
    void shutdown() override;
    void update() override;
    bool isKeyPressed(KeyCode key) const override;
    bool wasKeyPressed(KeyCode key) const override;
    bool wasKeyReleased(KeyCode key) const override;
    std::vector<KeyCode> getPressedKeys() const override;
    bool isMouseButtonPressed(MouseButton button) const override;
    bool wasMouseButtonPressed(MouseButton button) const override;
    bool wasMouseButtonReleased(MouseButton button) const override;
    Vector2 getMousePosition() const override;
    Vector2 getMouseDelta() const override;
    float getMouseWheel() const override;
    void setMousePosition(const Vector2& position) override;
    void setMouseVisible(bool visible) override;
    bool isMouseVisible() const override;
    void setMouseLocked(bool locked) override;
    bool isMouseLocked() const override;
    std::vector<TouchPoint> getTouchPoints() const override;
    TouchPoint getTouchPoint(int id) const override;
    int getTouchCount() const override;
    bool isTouchSupported() const override;
    int getGamepadCount() const override;
    bool isGamepadConnected(int gamepadId) const override;
    GamepadState getGamepadState(int gamepadId) const override;
    bool isGamepadButtonPressed(int gamepadId, GamepadButton button) const override;
    bool wasGamepadButtonPressed(int gamepadId, GamepadButton button) const override;
    bool wasGamepadButtonReleased(int gamepadId, GamepadButton button) const override;
    float getGamepadAxis(int gamepadId, GamepadAxis axis) const override;
    void setGamepadVibration(int gamepadId, float leftMotor, float rightMotor, float duration) override;
    std::string getGamepadName(int gamepadId) const override;
    InputAction* createAction(const std::string& name) override;
    void destroyAction(const std::string& name) override;
    InputAction* getAction(const std::string& name) override;
    InputBinding* getBinding(const std::string& actionName) override;
    void loadInputMap(const std::string& path) override;
    void saveInputMap(const std::string& path) override;
    void setInputMap(const std::string& mapName) override;
    std::string getCurrentInputMap() const override;
    std::vector<std::string> getAvailableInputMaps() const override;
    void startTextInput() override;
    void stopTextInput() override;
    bool isTextInputActive() const override;
    std::string getTextInput() const override;
    void clearTextInput() override;
    void setKeyCallback(std::function<void(KeyCode, bool)> callback) override;
    void setMouseButtonCallback(std::function<void(MouseButton, bool, Vector2)> callback) override;
    void setMouseMoveCallback(std::function<void(Vector2, Vector2)> callback) override;
    void setMouseWheelCallback(std::function<void(float)> callback) override;
    void setTouchCallback(std::function<void(const TouchPoint&)> callback) override;
    void setGamepadConnectedCallback(std::function<void(int, bool)> callback) override;
    void setTextInputCallback(std::function<void(const std::string&)> callback) override;
    void startRecording(const std::string& filename) override;
    void stopRecording() override;
    bool isRecording() const override;
    void startPlayback(const std::string& filename) override;
    void stopPlayback() override;
    bool isPlayingBack() const override;
    void handlePlatformEvent(void* event) override;
    void setWindowHandle(void* windowHandle) override;
};

} // namespace FoundryEngine
