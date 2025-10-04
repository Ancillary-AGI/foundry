/**
 * Windows Input Implementation
 * Windows Input Management System with Keyboard, Mouse, and Gamepad Support
 */

#include "WindowsPlatform.h"
#include <windows.h>
#include <xinput.h>

// Link required XInput library
#pragma comment(lib, "xinput.lib")

// ========== WINDOWS INPUT ==========
class WindowsInput : public PlatformInputManager {
private:
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> mouseButtons_;
    POINT mousePosition_;
    std::vector<XINPUT_STATE> gamepadStates_;
    std::vector<std::function<void(const InputEvent&)>> listeners_;
    HWND hwnd_;

public:
    WindowsInput(HWND hwnd) : hwnd_(hwnd) {
        gamepadStates_.resize(4); // XInput supports up to 4 controllers
    }

    ~WindowsInput() {
        // Clear vibration on exit
        for (int i = 0; i < 4; i++) {
            XINPUT_VIBRATION vibration = {0, 0};
            XInputSetState(i, &vibration);
        }
    }

    void update() {
        // Update gamepad states
        for (int i = 0; i < 4; i++) {
            DWORD result = XInputGetState(i, &gamepadStates_[i]);
            if (result != ERROR_SUCCESS) {
                // Controller not connected, clear state
                ZeroMemory(&gamepadStates_[i], sizeof(XINPUT_STATE));
            }
        }
    }

    std::unordered_map<int, bool> getKeyboardState() override { return keyStates_; }

    MouseState getMouseState() override {
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        ScreenToClient(hwnd_, &cursorPos);

        return {
            cursorPos.x,
            cursorPos.y,
            mouseButtons_
        };
    }

    std::vector<TouchPoint> getTouchState() override {
        return {}; // Windows doesn't have touch by default
    }

    GamepadState getGamepadState(int index) override {
        if (index < 0 || index >= 4) {
            return {false, "", {}, {}};
        }

        XINPUT_STATE state = gamepadStates_[index];
        if (XInputGetState(index, &state) != ERROR_SUCCESS) {
            return {false, "", {}, {}};
        }

        std::vector<ButtonState> buttons;
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0, state.Gamepad.bLeftTrigger / 255.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0, state.Gamepad.bRightTrigger / 255.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0, 0.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0, 0.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0, state.Gamepad.bLeftTrigger / 255.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0, state.Gamepad.bRightTrigger / 255.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0, 0.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0, 0.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0, 0.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0, 0.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0, 0.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0, 0.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0, 0.0f});
        buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0, 0.0f});

        std::vector<float> axes = {
            state.Gamepad.sThumbLX / 32767.0f,
            state.Gamepad.sThumbLY / 32767.0f,
            state.Gamepad.sThumbRX / 32767.0f,
            state.Gamepad.sThumbRY / 32767.0f,
            state.Gamepad.bLeftTrigger / 255.0f,
            state.Gamepad.bRightTrigger / 255.0f
        };

        return {true, "XInput Controller", buttons, axes};
    }

    std::vector<GamepadState> getConnectedGamepads() override {
        std::vector<GamepadState> connected;
        for (int i = 0; i < 4; i++) {
            GamepadState state = getGamepadState(i);
            if (state.connected) {
                connected.push_back(state);
            }
        }
        return connected;
    }

    int getGamepadCount() override {
        return 4; // XInput always supports up to 4 controllers
    }

    bool isGamepadConnected(int index) override {
        if (index < 0 || index >= 4) return false;
        XINPUT_STATE state;
        return XInputGetState(index, &state) == ERROR_SUCCESS;
    }

    std::string getGamepadName(int index) override {
        if (!isGamepadConnected(index)) return "";
        return "XInput Controller";
    }

    bool setGamepadVibration(int index, float leftMotor, float rightMotor, float duration) override {
        if (index < 0 || index >= 4) return false;

        XINPUT_VIBRATION vibration;
        vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
        vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);

        DWORD result = XInputSetState(index, &vibration);
        if (result == ERROR_SUCCESS && duration > 0) {
            // Stop vibration after duration
            std::thread([index, duration]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(duration * 1000)));
                XINPUT_VIBRATION stopVibration = {0, 0};
                XInputSetState(index, &stopVibration);
            }).detach();
        }

        return result == ERROR_SUCCESS;
    }

    void addEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override {
        listeners_.push_back(listener);
    }

    void removeEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override {
        // Remove listener implementation
    }

    // Windows message handling
    void handleKeyMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        bool pressed = (message == WM_KEYDOWN);
        keyStates_[static_cast<int>(wParam)] = pressed;

        InputEvent event = { "keyboard", pressed ? "press" : "release" };
        event.key = static_cast<int>(wParam);
        event.timestamp = GetTickCount64();

        for (auto& listener : listeners_) {
            listener(event);
        }
    }

    void handleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        int button = -1;
        bool pressed = false;

        switch (message) {
        case WM_LBUTTONDOWN: button = 0; pressed = true; break;
        case WM_LBUTTONUP: button = 0; pressed = false; break;
        case WM_RBUTTONDOWN: button = 2; pressed = true; break;
        case WM_RBUTTONUP: button = 2; pressed = false; break;
        case WM_MBUTTONDOWN: button = 1; pressed = true; break;
        case WM_MBUTTONUP: button = 1; pressed = false; break;
        }

        if (button >= 0) {
            mouseButtons_[button] = pressed;

            InputEvent event = { "mouse", pressed ? "press" : "release" };
            event.button = button;
            event.position = { LOWORD(lParam), HIWORD(lParam) };
            event.timestamp = GetTickCount64();

            for (auto& listener : listeners_) {
                listener(event);
            }
        }
    }
};
