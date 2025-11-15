/**
 * @file InputSystem.h
 * @brief Cross-platform input management system
 */

#pragma once

#include "../core/System.h"
#include <unordered_map>
#include <vector>
#include <functional>

namespace FoundryEngine {

// Forward declarations
class Gamepad;

/**
 * @enum KeyCode
 * @brief Standardized key codes across platforms
 */
enum class KeyCode {
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Space, Enter, Tab, Escape, Backspace, Delete, Insert,
    Left, Right, Up, Down,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift, RightShift, LeftControl, RightControl, LeftAlt, RightAlt,
    MouseLeft, MouseRight, MouseMiddle, MouseX1, MouseX2,
    UNKNOWN
};

/**
 * @class InputManager
 * @brief Cross-platform input system manager
 */
class InputManager : public System {
public:
    InputManager() = default;
    virtual ~InputManager() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;

    // Keyboard input
    virtual bool getKey(KeyCode key) const = 0;
    virtual bool getKeyDown(KeyCode key) const = 0;
    virtual bool getKeyUp(KeyCode key) const = 0;

    // Mouse input
    virtual bool getMouseButton(int button) const = 0;
    virtual bool getMouseButtonDown(int button) const = 0;
    virtual bool getMouseButtonUp(int button) const = 0;
    virtual void getMousePosition(int& x, int& y) const = 0;
    virtual void getMouseDelta(int& dx, int& dy) const = 0;

    // Gamepad input (for multiple gamepads)
    virtual int getGamepadCount() const = 0;
    virtual bool isGamepadConnected(int gamepadIndex) const = 0;
    virtual float getGamepadAxis(int gamepadIndex, int axis) const = 0;
    virtual bool getGamepadButton(int gamepadIndex, int button) const = 0;
    virtual bool getGamepadButtonDown(int gamepadIndex, int button) const = 0;
    virtual bool getGamepadButtonUp(int gamepadIndex, int button) const = 0;

    // Axis input (unified input system)
    virtual float getAxis(const std::string& axisName) const = 0;

    // Touch input (for mobile)
    virtual int getTouchCount() const = 0;
    virtual void getTouchPosition(int touchIndex, int& x, int& y) const = 0;

    // Text input
    virtual std::string getTextInput() const = 0;
    virtual void clearTextInput() = 0;

    // Input configuration
    virtual void setAxisMapping(const std::string& axisName,
                               KeyCode positiveKey, KeyCode negativeKey) = 0;
    virtual void setButtonMapping(const std::string& buttonName, KeyCode key) = 0;

    // Input callbacks
    virtual void setKeyCallback(std::function<void(KeyCode, bool)> callback) = 0;
    virtual void setMouseCallback(std::function<void(int, int, int, int)> callback) = 0;
    virtual void setGamepadCallback(std::function<void(int, int, int, bool)> callback) = 0;
};

/**
 * @class DefaultInputManager
 * @brief Default cross-platform input manager implementation
 */
class DefaultInputManager : public InputManager {
public:
    bool initialize() override { return true; }
    void shutdown() override {}
    void update(float deltaTime) override {}

    bool getKey(KeyCode key) const override { return false; }
    bool getKeyDown(KeyCode key) const override { return false; }
    bool getKeyUp(KeyCode key) const override { return false; }

    bool getMouseButton(int button) const override { return false; }
    bool getMouseButtonDown(int button) const override { return false; }
    bool getMouseButtonUp(int button) const override { return false; }
    void getMousePosition(int& x, int& y) const override { x = 0; y = 0; }
    void getMouseDelta(int& dx, int& dy) const override { dx = 0; dy = 0; }

    int getGamepadCount() const override { return 0; }
    bool isGamepadConnected(int gamepadIndex) const override { return false; }
    float getGamepadAxis(int gamepadIndex, int axis) const override { return 0.0f; }
    bool getGamepadButton(int gamepadIndex, int button) const override { return false; }
    bool getGamepadButtonDown(int gamepadIndex, int button) const override { return false; }
    bool getGamepadButtonUp(int gamepadIndex, int button) const override { return false; }

    float getAxis(const std::string& axisName) const override { return 0.0f; }

    int getTouchCount() const override { return 0; }
    void getTouchPosition(int touchIndex, int& x, int& y) const override { x = 0; y = 0; }

    std::string getTextInput() const override { return ""; }
    void clearTextInput() override {}

    void setAxisMapping(const std::string& axisName,
                       KeyCode positiveKey, KeyCode negativeKey) override {}
    void setButtonMapping(const std::string& buttonName, KeyCode key) override {}

    void setKeyCallback(std::function<void(KeyCode, bool)> callback) override {}
    void setMouseCallback(std::function<void(int, int, int, int)> callback) override {}
    void setGamepadCallback(std::function<void(int, int, int, bool)> callback) override {}
};

} // namespace FoundryEngine
