#include "../../include/GameEngine/systems/InputSystem.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>

namespace FoundryEngine {

// Input Action Implementation
class InputActionImpl : public InputAction {
private:
    std::vector<KeyCode> keyBindings_;
    std::vector<MouseButton> mouseBindings_;
    std::vector<std::pair<int, GamepadButton>> gamepadBindings_;
    std::vector<std::pair<int, GamepadAxis>> gamepadAxisBindings_;

    bool pressed_ = false;
    bool wasPressed_ = false;
    bool wasReleased_ = false;
    float value_ = 0.0f;
    Vector2 vector2Value_{0, 0};

public:
    bool isPressed() const override { return pressed_; }
    bool wasPressed() const override { return wasPressed_; }
    bool wasReleased() const override { return wasReleased_; }
    float getValue() const override { return value_; }
    Vector2 getVector2() const override { return vector2Value_; }

    void update(bool pressed, float value = 0.0f, const Vector2& vector2 = Vector2{0, 0}) {
        wasPressed_ = !pressed_ && pressed;
        wasReleased_ = pressed_ && !pressed;
        pressed_ = pressed;
        value_ = value;
        vector2Value_ = vector2;
    }

    // Accessors for DefaultInputManagerImpl
    const std::vector<KeyCode>& getKeyBindings() const { return keyBindings_; }
    const std::vector<MouseButton>& getMouseBindings() const { return mouseBindings_; }
    const std::vector<std::pair<int, GamepadButton>>& getGamepadBindings() const { return gamepadBindings_; }
    const std::vector<std::pair<int, GamepadAxis>>& getGamepadAxisBindings() const { return gamepadAxisBindings_; }
};

// Input Binding Implementation
class InputBindingImpl : public InputBinding {
private:
    std::vector<KeyCode> keyBindings_;
    std::vector<MouseButton> mouseBindings_;
    std::vector<std::pair<int, GamepadButton>> gamepadBindings_;
    std::vector<std::pair<int, GamepadAxis>> gamepadAxisBindings_;

public:
    void addKeyBinding(KeyCode key) override {
        if (std::find(keyBindings_.begin(), keyBindings_.end(), key) == keyBindings_.end()) {
            keyBindings_.push_back(key);
        }
    }

    void addMouseBinding(MouseButton button) override {
        if (std::find(mouseBindings_.begin(), mouseBindings_.end(), button) == mouseBindings_.end()) {
            mouseBindings_.push_back(button);
        }
    }

    void addGamepadBinding(int gamepadId, GamepadButton button) override {
        auto binding = std::make_pair(gamepadId, button);
        if (std::find(gamepadBindings_.begin(), gamepadBindings_.end(), binding) == gamepadBindings_.end()) {
            gamepadBindings_.push_back(binding);
        }
    }

    void addGamepadAxisBinding(int gamepadId, GamepadAxis axis) override {
        auto binding = std::make_pair(gamepadId, axis);
        if (std::find(gamepadAxisBindings_.begin(), gamepadAxisBindings_.end(), binding) == gamepadAxisBindings_.end()) {
            gamepadAxisBindings_.push_back(binding);
        }
    }

    void removeKeyBinding(KeyCode key) override {
        keyBindings_.erase(std::remove(keyBindings_.begin(), keyBindings_.end(), key), keyBindings_.end());
    }

    void removeMouseBinding(MouseButton button) override {
        mouseBindings_.erase(std::remove(mouseBindings_.begin(), mouseBindings_.end(), button), mouseBindings_.end());
    }

    void removeGamepadBinding(int gamepadId, GamepadButton button) override {
        auto binding = std::make_pair(gamepadId, button);
        gamepadBindings_.erase(std::remove(gamepadBindings_.begin(), gamepadBindings_.end(), binding), gamepadBindings_.end());
    }

    void removeGamepadAxisBinding(int gamepadId, GamepadAxis axis) override {
        auto binding = std::make_pair(gamepadId, axis);
        gamepadAxisBindings_.erase(std::remove(gamepadAxisBindings_.begin(), gamepadAxisBindings_.end(), binding), gamepadAxisBindings_.end());
    }

    void clearBindings() override {
        keyBindings_.clear();
        mouseBindings_.clear();
        gamepadBindings_.clear();
        gamepadAxisBindings_.clear();
    }

    const std::vector<KeyCode>& getKeyBindings() const { return keyBindings_; }
    const std::vector<MouseButton>& getMouseBindings() const { return mouseBindings_; }
    const std::vector<std::pair<int, GamepadButton>>& getGamepadBindings() const { return gamepadBindings_; }
    const std::vector<std::pair<int, GamepadAxis>>& getGamepadAxisBindings() const { return gamepadAxisBindings_; }
};

// Default Input Manager Implementation
class DefaultInputManagerImpl : public SystemImplBase<DefaultInputManagerImpl> {
private:
    // Keyboard state
    std::unordered_map<KeyCode, bool> keyStates_;
    std::unordered_map<KeyCode, bool> keyStatesPrevious_;
    std::vector<KeyCode> pressedKeys_;

    // Mouse state
    std::unordered_map<MouseButton, bool> mouseStates_;
    std::unordered_map<MouseButton, bool> mouseStatesPrevious_;
    Vector2 mousePosition_{0, 0};
    Vector2 mouseDelta_{0, 0};
    float mouseWheel_ = 0.0f;
    bool mouseVisible_ = true;
    bool mouseLocked_ = false;

    // Touch state
    std::vector<TouchPoint> touchPoints_;
    bool touchSupported_ = false;

    // Gamepad state
    std::unordered_map<int, GamepadState> gamepadStates_;
    int gamepadCount_ = 0;

    // Input actions and bindings
    std::unordered_map<std::string, std::unique_ptr<InputActionImpl>> actions_;
    std::unordered_map<std::string, std::unique_ptr<InputBindingImpl>> bindings_;

    // Input mapping
    std::string currentInputMap_ = "default";
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> inputMaps_;

    // Text input
    bool textInputActive_ = false;
    std::string textInput_;

    // Recording and playback
    bool recording_ = false;
    bool playingBack_ = false;
    std::string recordingFilename_;
    std::string playbackFilename_;

    // Callbacks
    std::function<void(KeyCode, bool)> keyCallback_;
    std::function<void(MouseButton, bool, Vector2)> mouseButtonCallback_;
    std::function<void(Vector2, Vector2)> mouseMoveCallback_;
    std::function<void(float)> mouseWheelCallback_;
    std::function<void(const TouchPoint&)> touchCallback_;
    std::function<void(int, bool)> gamepadConnectedCallback_;
    std::function<void(const std::string&)> textInputCallback_;

    // Platform-specific
    void* windowHandle_ = nullptr;

    friend class SystemImplBase<DefaultInputManagerImpl>;

    bool onInitialize() override {
        std::cout << "Default Input Manager initialized" << std::endl;

        // Initialize default input map
        inputMaps_["default"] = {
            {"MoveForward", "W"},
            {"MoveBackward", "S"},
            {"MoveLeft", "A"},
            {"MoveRight", "D"},
            {"Jump", "Space"},
            {"Fire", "MouseLeft"}
        };

        return true;
    }

    void onShutdown() override {
        actions_.clear();
        bindings_.clear();
        std::cout << "Default Input Manager shutdown" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Update previous states
        keyStatesPrevious_ = keyStates_;
        mouseStatesPrevious_ = mouseStates_;

        // Update pressed keys
        pressedKeys_.clear();
        for (const auto& pair : keyStates_) {
            if (pair.second) {
                pressedKeys_.push_back(pair.first);
            }
        }

        // Update input actions
        for (auto& actionPair : actions_) {
            auto& action = actionPair.second;
            auto& binding = bindings_[actionPair.first];

            if (binding) {
                bool pressed = false;
                float value = 0.0f;
                Vector2 vector2{0, 0};

                // Check keyboard bindings
                for (KeyCode key : binding->getKeyBindings()) {
                    if (isKeyPressed(key)) {
                        pressed = true;
                        break;
                    }
                }

                // Check mouse bindings
                if (!pressed) {
                    for (MouseButton button : binding->getMouseBindings()) {
                        if (isMouseButtonPressed(button)) {
                            pressed = true;
                            break;
                        }
                    }
                }

                // Check gamepad bindings
                if (!pressed) {
                    for (const auto& gamepadBinding : binding->getGamepadBindings()) {
                        if (isGamepadButtonPressed(gamepadBinding.first, gamepadBinding.second)) {
                            pressed = true;
                            break;
                        }
                    }
                }

                // Check gamepad axis bindings
                for (const auto& axisBinding : binding->getGamepadAxisBindings()) {
                    float axisValue = getGamepadAxis(axisBinding.first, axisBinding.second);
                    if (std::abs(axisValue) > 0.1f) {
                        pressed = true;
                        value = axisValue;
                        if (axisBinding.second == GamepadAxis::LeftX || axisBinding.second == GamepadAxis::RightX) {
                            vector2.x = axisValue;
                        } else if (axisBinding.second == GamepadAxis::LeftY || axisBinding.second == GamepadAxis::RightY) {
                            vector2.y = axisValue;
                        }
                    }
                }

                action->update(pressed, value, vector2);
            }
        }
    }

public:
    DefaultInputManagerImpl() : SystemImplBase("DefaultInputManager") {}

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Input Stats - Keys: %zu pressed, Actions: %zu, Gamepads: %d",
                 pressedKeys_.size(), actions_.size(), gamepadCount_);
        return std::string(buffer);
    }

    // Keyboard input
    bool isKeyPressed(KeyCode key) const {
        auto it = keyStates_.find(key);
        return (it != keyStates_.end()) ? it->second : false;
    }

    bool wasKeyPressed(KeyCode key) const {
        bool current = isKeyPressed(key);
        auto it = keyStatesPrevious_.find(key);
        bool previous = (it != keyStatesPrevious_.end()) ? it->second : false;
        return current && !previous;
    }

    bool wasKeyReleased(KeyCode key) const {
        bool current = isKeyPressed(key);
        auto it = keyStatesPrevious_.find(key);
        bool previous = (it != keyStatesPrevious_.end()) ? it->second : false;
        return !current && previous;
    }

    std::vector<KeyCode> getPressedKeys() const { return pressedKeys_; }

    // Mouse input
    bool isMouseButtonPressed(MouseButton button) const {
        auto it = mouseStates_.find(button);
        return (it != mouseStates_.end()) ? it->second : false;
    }

    bool wasMouseButtonPressed(MouseButton button) const {
        bool current = isMouseButtonPressed(button);
        auto it = mouseStatesPrevious_.find(button);
        bool previous = (it != mouseStatesPrevious_.end()) ? it->second : false;
        return current && !previous;
    }

    bool wasMouseButtonReleased(MouseButton button) const {
        bool current = isMouseButtonPressed(button);
        auto it = mouseStatesPrevious_.find(button);
        bool previous = (it != mouseStatesPrevious_.end()) ? it->second : false;
        return !current && previous;
    }

    Vector2 getMousePosition() const { return mousePosition_; }
    Vector2 getMouseDelta() const { return mouseDelta_; }
    float getMouseWheel() const { return mouseWheel_; }

    void setMousePosition(const Vector2& position) { mousePosition_ = position; }
    void setMouseVisible(bool visible) { mouseVisible_ = visible; }
    bool isMouseVisible() const { return mouseVisible_; }
    void setMouseLocked(bool locked) { mouseLocked_ = locked; }
    bool isMouseLocked() const { return mouseLocked_; }

    // Touch input
    std::vector<TouchPoint> getTouchPoints() const { return touchPoints_; }
    TouchPoint getTouchPoint(int id) const {
        for (const auto& point : touchPoints_) {
            if (point.id == id) return point;
        }
        return TouchPoint{id, Vector2{0, 0}, Vector2{0, 0}, 0.0f, false};
    }
    int getTouchCount() const { return touchPoints_.size(); }
    bool isTouchSupported() const { return touchSupported_; }

    // Gamepad input
    int getGamepadCount() const { return gamepadCount_; }
    bool isGamepadConnected(int gamepadId) const {
        auto it = gamepadStates_.find(gamepadId);
        return (it != gamepadStates_.end()) ? it->second.connected : false;
    }

    GamepadState getGamepadState(int gamepadId) const {
        auto it = gamepadStates_.find(gamepadId);
        return (it != gamepadStates_.end()) ? it->second : GamepadState{false, "", {}, {}, 0.0f, 0.0f, Vector2{0, 0}, Vector2{0, 0}, false, false};
    }

    bool isGamepadButtonPressed(int gamepadId, GamepadButton button) const {
        auto it = gamepadStates_.find(gamepadId);
        if (it != gamepadStates_.end() && it->second.connected) {
            int buttonIndex = static_cast<int>(button);
            return (buttonIndex < it->second.buttons.size()) ? it->second.buttons[buttonIndex] : false;
        }
        return false;
    }

    bool wasGamepadButtonPressed(int gamepadId, GamepadButton button) const {
        // Would need to track previous gamepad states
        return false;
    }

    bool wasGamepadButtonReleased(int gamepadId, GamepadButton button) const {
        // Would need to track previous gamepad states
        return false;
    }

    float getGamepadAxis(int gamepadId, GamepadAxis axis) const {
        auto it = gamepadStates_.find(gamepadId);
        if (it != gamepadStates_.end() && it->second.connected) {
            int axisIndex = static_cast<int>(axis);
            return (axisIndex < it->second.axes.size()) ? it->second.axes[axisIndex] : 0.0f;
        }
        return 0.0f;
    }

    void setGamepadVibration(int gamepadId, float leftMotor, float rightMotor, float duration) {
        // Implement gamepad vibration
    }

    std::string getGamepadName(int gamepadId) const {
        auto it = gamepadStates_.find(gamepadId);
        return (it != gamepadStates_.end()) ? it->second.name : "";
    }

    // Input actions and bindings
    InputAction* createAction(const std::string& name) {
        auto action = std::make_unique<InputActionImpl>();
        InputAction* actionPtr = action.get();
        actions_[name] = std::move(action);
        return actionPtr;
    }

    void destroyAction(const std::string& name) {
        actions_.erase(name);
        bindings_.erase(name);
    }

    InputAction* getAction(const std::string& name) {
        auto it = actions_.find(name);
        return (it != actions_.end()) ? it->second.get() : nullptr;
    }

    InputBinding* getBinding(const std::string& actionName) {
        auto it = bindings_.find(actionName);
        if (it == bindings_.end()) {
            bindings_[actionName] = std::make_unique<InputBindingImpl>();
        }
        return bindings_[actionName].get();
    }

    // Input mapping
    void loadInputMap(const std::string& path) {
        // Load input map from file
    }

    void saveInputMap(const std::string& path) {
        // Save input map to file
    }

    void setInputMap(const std::string& mapName) {
        if (inputMaps_.find(mapName) != inputMaps_.end()) {
            currentInputMap_ = mapName;
        }
    }

    std::string getCurrentInputMap() const { return currentInputMap_; }

    std::vector<std::string> getAvailableInputMaps() const {
        std::vector<std::string> maps;
        for (const auto& pair : inputMaps_) {
            maps.push_back(pair.first);
        }
        return maps;
    }

    // Text input
    void startTextInput() { textInputActive_ = true; }
    void stopTextInput() { textInputActive_ = false; }
    bool isTextInputActive() const { return textInputActive_; }
    std::string getTextInput() const { return textInput_; }
    void clearTextInput() { textInput_.clear(); }

    // Callbacks
    void setKeyCallback(std::function<void(KeyCode, bool)> callback) { keyCallback_ = callback; }
    void setMouseButtonCallback(std::function<void(MouseButton, bool, Vector2)> callback) { mouseButtonCallback_ = callback; }
    void setMouseMoveCallback(std::function<void(Vector2, Vector2)> callback) { mouseMoveCallback_ = callback; }
    void setMouseWheelCallback(std::function<void(float)> callback) { mouseWheelCallback_ = callback; }
    void setTouchCallback(std::function<void(const TouchPoint&)> callback) { touchCallback_ = callback; }
    void setGamepadConnectedCallback(std::function<void(int, bool)> callback) { gamepadConnectedCallback_ = callback; }
    void setTextInputCallback(std::function<void(const std::string&)> callback) { textInputCallback_ = callback; }

    // Input recording and playback
    void startRecording(const std::string& filename) {
        recording_ = true;
        recordingFilename_ = filename;
    }

    void stopRecording() { recording_ = false; }
    bool isRecording() const { return recording_; }

    void startPlayback(const std::string& filename) {
        playingBack_ = true;
        playbackFilename_ = filename;
    }

    void stopPlayback() { playingBack_ = false; }
    bool isPlayingBack() const { return playingBack_; }

    // Platform-specific
    void handlePlatformEvent(void* event) {
        // Handle platform-specific input events
    }

    void setWindowHandle(void* windowHandle) { windowHandle_ = windowHandle; }

    // Internal methods for updating input state
    void setKeyState(KeyCode key, bool pressed) {
        keyStates_[key] = pressed;
        if (keyCallback_) {
            keyCallback_(key, pressed);
        }
    }

    void setMouseButtonState(MouseButton button, bool pressed) {
        mouseStates_[button] = pressed;
        if (mouseButtonCallback_) {
            mouseButtonCallback_(button, pressed, mousePosition_);
        }
    }

    void setMousePosition(const Vector2& position) {
        Vector2 oldPosition = mousePosition_;
        mousePosition_ = position;
        mouseDelta_ = position - oldPosition;
        if (mouseMoveCallback_) {
            mouseMoveCallback_(position, mouseDelta_);
        }
    }

    void setMouseWheel(float wheel) {
        mouseWheel_ = wheel;
        if (mouseWheelCallback_) {
            mouseWheelCallback_(wheel);
        }
    }
};

// Default Input Manager
DefaultInputManager::DefaultInputManager() : impl_(std::make_unique<DefaultInputManagerImpl>()) {}
DefaultInputManager::~DefaultInputManager() = default;

bool DefaultInputManager::initialize() { return impl_->initialize(); }
void DefaultInputManager::shutdown() { impl_->shutdown(); }
void DefaultInputManager::update(float deltaTime) { impl_->update(deltaTime); }
bool DefaultInputManager::isKeyPressed(KeyCode key) const { return impl_->isKeyPressed(key); }
bool DefaultInputManager::wasKeyPressed(KeyCode key) const { return impl_->wasKeyPressed(key); }
bool DefaultInputManager::wasKeyReleased(KeyCode key) const { return impl_->wasKeyReleased(key); }
std::vector<KeyCode> DefaultInputManager::getPressedKeys() const { return impl_->getPressedKeys(); }
bool DefaultInputManager::isMouseButtonPressed(MouseButton button) const { return impl_->isMouseButtonPressed(button); }
bool DefaultInputManager::wasMouseButtonPressed(MouseButton button) const { return impl_->wasMouseButtonPressed(button); }
bool DefaultInputManager::wasMouseButtonReleased(MouseButton button) const { return impl_->wasMouseButtonReleased(button); }
Vector2 DefaultInputManager::getMousePosition() const { return impl_->getMousePosition(); }
Vector2 DefaultInputManager::getMouseDelta() const { return impl_->getMouseDelta(); }
float DefaultInputManager::getMouseWheel() const { return impl_->getMouseWheel(); }
void DefaultInputManager::setMousePosition(const Vector2& position) { impl_->setMousePosition(position); }
void DefaultInputManager::setMouseVisible(bool visible) { impl_->setMouseVisible(visible); }
bool DefaultInputManager::isMouseVisible() const { return impl_->isMouseVisible(); }
void DefaultInputManager::setMouseLocked(bool locked) { impl_->setMouseLocked(locked); }
bool DefaultInputManager::isMouseLocked() const { return impl_->isMouseLocked(); }
std::vector<TouchPoint> DefaultInputManager::getTouchPoints() const { return impl_->getTouchPoints(); }
TouchPoint DefaultInputManager::getTouchPoint(int id) const { return impl_->getTouchPoint(id); }
int DefaultInputManager::getTouchCount() const { return impl_->getTouchCount(); }
bool DefaultInputManager::isTouchSupported() const { return impl_->isTouchSupported(); }
int DefaultInputManager::getGamepadCount() const { return impl_->getGamepadCount(); }
bool DefaultInputManager::isGamepadConnected(int gamepadId) const { return impl_->isGamepadConnected(gamepadId); }
GamepadState DefaultInputManager::getGamepadState(int gamepadId) const { return impl_->getGamepadState(gamepadId); }
bool DefaultInputManager::isGamepadButtonPressed(int gamepadId, GamepadButton button) const { return impl_->isGamepadButtonPressed(gamepadId, button); }
bool DefaultInputManager::wasGamepadButtonPressed(int gamepadId, GamepadButton button) const { return impl_->wasGamepadButtonPressed(gamepadId, button); }
bool DefaultInputManager::wasGamepadButtonReleased(int gamepadId, GamepadButton button) const { return impl_->wasGamepadButtonReleased(gamepadId, button); }
float DefaultInputManager::getGamepadAxis(int gamepadId, GamepadAxis axis) const { return impl_->getGamepadAxis(gamepadId, axis); }
void DefaultInputManager::setGamepadVibration(int gamepadId, float leftMotor, float rightMotor, float duration) { impl_->setGamepadVibration(gamepadId, leftMotor, rightMotor, duration); }
std::string DefaultInputManager::getGamepadName(int gamepadId) const { return impl_->getGamepadName(gamepadId); }
InputAction* DefaultInputManager::createAction(const std::string& name) { return impl_->createAction(name); }
void DefaultInputManager::destroyAction(const std::string& name) { impl_->destroyAction(name); }
InputAction* DefaultInputManager::getAction(const std::string& name) { return impl_->getAction(name); }
InputBinding* DefaultInputManager::getBinding(const std::string& actionName) { return impl_->getBinding(actionName); }
void DefaultInputManager::loadInputMap(const std::string& path) { impl_->loadInputMap(path); }
void DefaultInputManager::saveInputMap(const std::string& path) { impl_->saveInputMap(path); }
void DefaultInputManager::setInputMap(const std::string& mapName) { impl_->setInputMap(mapName); }
std::string DefaultInputManager::getCurrentInputMap() const { return impl_->getCurrentInputMap(); }
std::vector<std::string> DefaultInputManager::getAvailableInputMaps() const { return impl_->getAvailableInputMaps(); }
void DefaultInputManager::startTextInput() { impl_->startTextInput(); }
void DefaultInputManager::stopTextInput() { impl_->stopTextInput(); }
bool DefaultInputManager::isTextInputActive() const { return impl_->isTextInputActive(); }
std::string DefaultInputManager::getTextInput() const { return impl_->getTextInput(); }
void DefaultInputManager::clearTextInput() { impl_->clearTextInput(); }
void DefaultInputManager::setKeyCallback(std::function<void(KeyCode, bool)> callback) { impl_->setKeyCallback(callback); }
void DefaultInputManager::setMouseButtonCallback(std::function<void(MouseButton, bool, Vector2)> callback) { impl_->setMouseButtonCallback(callback); }
void DefaultInputManager::setMouseMoveCallback(std::function<void(Vector2, Vector2)> callback) { impl_->setMouseMoveCallback(callback); }
void DefaultInputManager::setMouseWheelCallback(std::function<void(float)> callback) { impl_->setMouseWheelCallback(callback); }
void DefaultInputManager::setTouchCallback(std::function<void(const TouchPoint&)> callback) { impl_->setTouchCallback(callback); }
void DefaultInputManager::setGamepadConnectedCallback(std::function<void(int, bool)> callback) { impl_->setGamepadConnectedCallback(callback); }
void DefaultInputManager::setTextInputCallback(std::function<void(const std::string&)> callback) { impl_->setTextInputCallback(callback); }
void DefaultInputManager::startRecording(const std::string& filename) { impl_->startRecording(filename); }
void DefaultInputManager::stopRecording() { impl_->stopRecording(); }
bool DefaultInputManager::isRecording() const { return impl_->isRecording(); }
void DefaultInputManager::startPlayback(const std::string& filename) { impl_->startPlayback(filename); }
void DefaultInputManager::stopPlayback() { impl_->stopPlayback(); }
bool DefaultInputManager::isPlayingBack() const { return impl_->isPlayingBack(); }
void DefaultInputManager::handlePlatformEvent(void* event) { impl_->handlePlatformEvent(event); }
void DefaultInputManager::setWindowHandle(void* windowHandle) { impl_->setWindowHandle(windowHandle); }

} // namespace FoundryEngine
