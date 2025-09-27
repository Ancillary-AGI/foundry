/**
 * Linux Platform Header
 * Native Linux platform interface definitions using SDL
 */

#ifndef LINUX_PLATFORM_H
#define LINUX_PLATFORM_H

#include "../../core/Platform.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_gamecontroller.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

// Platform interface implementations
class LinuxCanvas : public PlatformCanvas {
private:
    SDL_Window* window_;
    SDL_GLContext glContext_;
    int width_, height_;

public:
    LinuxCanvas(int width, int height, SDL_Window* window);
    ~LinuxCanvas();

    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    void setWidth(int width) override { width_ = width; }
    void setHeight(int height) override { height_ = height; }

    PlatformGraphicsContext* getContext(const std::string& contextType) override;
    void addEventListener(const std::string& event, std::function<void(const InputEvent&)> listener) override {}
    void removeEventListener(const std::string& event, std::function<void(const InputEvent&)> listener) override {}

    void swapBuffers();
    SDL_Window* getWindow() { return window_; }
};

class LinuxGraphics : public PlatformGraphics {
private:
    SDL_GLContext glContext_;

public:
    LinuxGraphics();
    ~LinuxGraphics();

    PlatformCapabilities getCapabilities() override;
    std::unique_ptr<PlatformGraphicsContext> createContext() override;

    void makeCurrent(SDL_Window* window);
};

class LinuxAudio : public PlatformAudio {
private:
    bool initialized_ = false;

public:
    LinuxAudio();
    ~LinuxAudio();

    bool initialize();
    void shutdown();

    std::unique_ptr<PlatformAudioContext> createContext() override;
    void resume() override;
    void suspend() override;
};

class LinuxInput : public PlatformInputManager {
private:
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> mouseButtons_;
    int mouseX_, mouseY_;
    std::vector<TouchPoint> touchPoints_;
    std::vector<GamepadState> gamepadStates_;
    std::vector<std::function<void(const InputEvent&)>> listeners_;

public:
    LinuxInput();
    ~LinuxInput();

    void update();

    std::unordered_map<int, bool> getKeyboardState() override { return keyStates_; }
    MouseState getMouseState() override;
    std::vector<TouchPoint> getTouchState() override { return touchPoints_; }
    GamepadState getGamepadState(int index) override;
    std::vector<GamepadState> getConnectedGamepads() override;
    int getGamepadCount() override;
    bool isGamepadConnected(int index) override;
    std::string getGamepadName(int index) override;
    bool setGamepadVibration(int index, float leftMotor, float rightMotor, float duration) override;

    void addEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override;
    void removeEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override;

    // SDL event handling
    void handleSDLEvent(const SDL_Event& event);
    void handleControllerAdded(int deviceIndex);
    void handleControllerRemoved(int instanceId);
};

class LinuxFileSystem : public PlatformFileSystem {
private:
    std::string appDataPath_;
    std::string documentsPath_;

public:
    LinuxFileSystem();

    std::vector<uint8_t> readFile(const std::string& path) override;
    void writeFile(const std::string& path, const std::vector<uint8_t>& data) override;
    void deleteFile(const std::string& path) override;
    std::vector<std::string> listFiles(const std::string& directory) override;
    void createDirectory(const std::string& path) override;
    bool exists(const std::string& path) override;

private:
    std::string getAppDataPath();
    std::string getDocumentsPath();
};

class LinuxNetworking : public PlatformNetworking {
public:
    std::unique_ptr<PlatformWebSocket> connect(const std::string& url) override;
    std::vector<uint8_t> httpGet(const std::string& url) override;
    std::vector<uint8_t> httpPost(const std::string& url, const std::vector<uint8_t>& data) override;
};

class LinuxTimer : public PlatformTimer {
private:
    Uint64 startTime_;

public:
    LinuxTimer();

    double now() override;
    int setTimeout(std::function<void()> callback, int delay) override;
    void clearTimeout(int id) override;
    int setInterval(std::function<void()> callback, int delay) override;
    void clearInterval(int id) override;
    int requestAnimationFrame(std::function<void(double)> callback) override;
    void cancelAnimationFrame(int id) override;
};

class LinuxRandom : public PlatformRandom {
public:
    double random() override;
    int randomInt(int min, int max) override;
    double randomFloat(double min, double max) override;
    void seed(unsigned int seed) override;
};

// Linux Application class
class LinuxApplication {
private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext glContext_ = nullptr;
    std::unique_ptr<GameEngine> engine_;
    bool running_ = false;

    std::unique_ptr<LinuxGraphics> graphics_;
    std::unique_ptr<LinuxAudio> audio_;
    std::unique_ptr<LinuxInput> input_;
    std::unique_ptr<LinuxFileSystem> fileSystem_;
    std::unique_ptr<LinuxTimer> timer_;
    std::unique_ptr<LinuxRandom> random_;

public:
    LinuxApplication();
    ~LinuxApplication();

    bool initialize(int width, int height, const std::string& title);
    void run();
    void shutdown();

private:
    void update(float deltaTime);
    void render();
    void processEvents();
};

#endif // LINUX_PLATFORM_H
