/**
 * Linux Platform Implementation
 * Native Linux platform using SDL2 for cross-platform support
 */

#include "LinuxPlatform.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_gamecontroller.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <cstring>

// ========== LINUX CANVAS ==========
LinuxCanvas::LinuxCanvas(int width, int height, SDL_Window* window)
    : window_(window), width_(width), height_(height), glContext_(nullptr) {
    // Create OpenGL context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    glContext_ = SDL_GL_CreateContext(window_);
    if (glContext_) {
        SDL_GL_MakeCurrent(window_, glContext_);
    }
}

LinuxCanvas::~LinuxCanvas() {
    if (glContext_) {
        SDL_GL_DeleteContext(glContext_);
    }
}

PlatformGraphicsContext* LinuxCanvas::getContext(const std::string& contextType) {
    if (contextType == "opengl" && glContext_) {
        return new LinuxGLContext(glContext_);
    }
    return nullptr;
}

void LinuxCanvas::swapBuffers() {
    SDL_GL_SwapWindow(window_);
}

// ========== LINUX GRAPHICS ==========
LinuxGraphics::LinuxGraphics() : glContext_(nullptr) {}

LinuxGraphics::~LinuxGraphics() {
    if (glContext_) {
        SDL_GL_DeleteContext(glContext_);
    }
}

PlatformCapabilities LinuxGraphics::getCapabilities() {
    PlatformCapabilities caps = {};
    caps.hasOpenGL = true;
    caps.hasVulkan = false; // Could check for Vulkan support

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);

    caps.renderer = renderer ? (const char*)renderer : "";
    caps.vendor = vendor ? (const char*)vendor : "";
    caps.version = version ? (const char*)version : "";

    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    caps.maxTextureSize = maxTextureSize;

    return caps;
}

std::unique_ptr<PlatformGraphicsContext> LinuxGraphics::createContext() {
    return std::unique_ptr<PlatformGraphicsContext>(new LinuxGLContext(glContext_));
}

void LinuxGraphics::makeCurrent(SDL_Window* window) {
    if (glContext_) {
        SDL_GL_MakeCurrent(window, glContext_);
    }
}

// ========== LINUX AUDIO ==========
LinuxAudio::LinuxAudio() : initialized_(false) {}

LinuxAudio::~LinuxAudio() {
    shutdown();
}

bool LinuxAudio::initialize() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL audio initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer initialization failed: " << Mix_GetError() << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}

void LinuxAudio::shutdown() {
    if (initialized_) {
        Mix_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        initialized_ = false;
    }
}

std::unique_ptr<PlatformAudioContext> LinuxAudio::createContext() {
    return std::unique_ptr<PlatformAudioContext>(new LinuxAudioContext());
}

void LinuxAudio::resume() {
    Mix_ResumeMusic();
    Mix_Resume(-1);
}

void LinuxAudio::suspend() {
    Mix_PauseMusic();
    Mix_Pause(-1);
}

// ========== LINUX INPUT ==========
LinuxInput::LinuxInput() : mouseX_(0), mouseY_(0) {
    // Initialize gamepad states
    gamepadStates_.resize(4); // Support up to 4 controllers
}

LinuxInput::~LinuxInput() {
    // Close all game controllers
    for (size_t i = 0; i < gamepadStates_.size(); ++i) {
        if (gamepadStates_[i].controller) {
            SDL_GameControllerClose(gamepadStates_[i].controller);
        }
    }
}

void LinuxInput::update() {
    // Update gamepad states
    for (size_t i = 0; i < gamepadStates_.size(); ++i) {
        auto& state = gamepadStates_[i];
        if (state.controller) {
            // Update button states
            state.buttons.clear();
            state.axes.clear();

            // Standard gamepad buttons
            state.buttons.push_back({SDL_GameControllerGetButton(state.controller, SDL_CONTROLLER_BUTTON_A) != 0, 0.0f});
            state.buttons.push_back({SDL_GameControllerGetButton(state.controller, SDL_CONTROLLER_BUTTON_B) != 0, 0.0f});
            state.buttons.push_back({SDL_GameControllerGetButton(state.controller, SDL_CONTROLLER_BUTTON_X) != 0, 0.0f});
            state.buttons.push_back({SDL_GameControllerGetButton(state.controller, SDL_CONTROLLER_BUTTON_Y) != 0, 0.0f});
            state.buttons.push_back({SDL_GameControllerGetButton(state.controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) != 0, 0.0f});
            state.buttons.push_back({SDL_GameControllerGetButton(state.controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) != 0, 0.0f});
            state.buttons.push_back({SDL_GameControllerGetButton(state.controller, SDL_CONTROLLER_BUTTON_START) != 0, 0.0f});
            state.buttons.push_back({SDL_GameControllerGetButton(state.controller, SDL_CONTROLLER_BUTTON_BACK) != 0, 0.0f});

            // Analog sticks
            state.axes.push_back(SDL_GameControllerGetAxis(state.controller, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f);
            state.axes.push_back(SDL_GameControllerGetAxis(state.controller, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f);
            state.axes.push_back(SDL_GameControllerGetAxis(state.controller, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.0f);
            state.axes.push_back(SDL_GameControllerGetAxis(state.controller, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f);
            state.axes.push_back(SDL_GameControllerGetAxis(state.controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0f);
            state.axes.push_back(SDL_GameControllerGetAxis(state.controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0f);
        }
    }
}

MouseState LinuxInput::getMouseState() {
    return {static_cast<float>(mouseX_), static_cast<float>(mouseY_), mouseButtons_};
}

GamepadState LinuxInput::getGamepadState(int index) {
    if (index < 0 || index >= static_cast<int>(gamepadStates_.size())) {
        return {false, "", {}, {}};
    }
    return gamepadStates_[index];
}

std::vector<GamepadState> LinuxInput::getConnectedGamepads() {
    std::vector<GamepadState> connected;
    for (const auto& state : gamepadStates_) {
        if (state.connected) {
            connected.push_back(state);
        }
    }
    return connected;
}

int LinuxInput::getGamepadCount() {
    return 4; // SDL supports up to 4 controllers by default
}

bool LinuxInput::isGamepadConnected(int index) {
    if (index < 0 || index >= static_cast<int>(gamepadStates_.size())) return false;
    return gamepadStates_[index].connected;
}

std::string LinuxInput::getGamepadName(int index) {
    if (!isGamepadConnected(index)) return "";
    return gamepadStates_[index].id;
}

bool LinuxInput::setGamepadVibration(int index, float leftMotor, float rightMotor, float duration) {
    if (index < 0 || index >= static_cast<int>(gamepadStates_.size())) return false;

    auto& state = gamepadStates_[index];
    if (!state.controller) return false;

    // Convert float values to SDL rumble values (0-65535)
    Uint16 leftRumble = static_cast<Uint16>(leftMotor * 65535.0f);
    Uint16 rightRumble = static_cast<Uint16>(rightMotor * 65535.0f);

    if (SDL_GameControllerRumble(state.controller, leftRumble, rightRumble,
                                static_cast<Uint32>(duration * 1000.0f)) != 0) {
        return false;
    }

    // Stop vibration after duration if specified
    if (duration > 0) {
        std::thread([this, index, duration]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(duration * 1000)));
            if (index >= 0 && index < static_cast<int>(gamepadStates_.size()) && gamepadStates_[index].controller) {
                SDL_GameControllerRumble(gamepadStates_[index].controller, 0, 0, 0);
            }
        }).detach();
    }

    return true;
}

void LinuxInput::addEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) {
    listeners_.push_back(listener);
}

void LinuxInput::removeEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) {
    // Remove listener implementation
}

void LinuxInput::handleSDLEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        keyStates_[event.key.keysym.sym] = (event.key.state == SDL_PRESSED);
        break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        mouseButtons_[event.button.button] = (event.button.state == SDL_PRESSED);
        break;

    case SDL_MOUSEMOTION:
        mouseX_ = event.motion.x;
        mouseY_ = event.motion.y;
        break;

    case SDL_CONTROLLERDEVICEADDED:
        handleControllerAdded(event.cdevice.which);
        break;

    case SDL_CONTROLLERDEVICEREMOVED:
        handleControllerRemoved(event.cdevice.which);
        break;
        
    default:
        break;
    }
}

void LinuxInput::handleControllerAdded(int deviceIndex) {
    SDL_GameController* controller = SDL_GameControllerOpen(deviceIndex);
    if (controller) {
        // Find available slot
        for (size_t i = 0; i < gamepadStates_.size(); ++i) {
            if (!gamepadStates_[i].connected) {
                gamepadStates_[i].controller = controller;
                gamepadStates_[i].connected = true;
                gamepadStates_[i].id = SDL_GameControllerName(controller);
                break;
            }
        }
    }
}

void LinuxInput::handleControllerRemoved(int instanceId) {
    for (auto& state : gamepadStates_) {
        if (state.controller && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(state.controller)) == instanceId) {
            SDL_GameControllerClose(state.controller);
            state.controller = nullptr;
            state.connected = false;
            state.id = "";
            state.buttons.clear();
            state.axes.clear();
            break;
        }
    }
}

// ========== LINUX FILE SYSTEM ==========
LinuxFileSystem::LinuxFileSystem() {
    appDataPath_ = getAppDataPath();
    documentsPath_ = getDocumentsPath();
}

std::vector<uint8_t> LinuxFileSystem::readFile(const std::string& path) {
    // Validate path to prevent directory traversal
    if (path.find("..") != std::string::npos || path.find("//") != std::string::npos) {
        return {};
    }
    
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    return buffer;
}

void LinuxFileSystem::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    // Validate path to prevent directory traversal
    if (path.find("..") != std::string::npos || path.find("//") != std::string::npos) {
        return;
    }
    
    std::ofstream file(path, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
}

void LinuxFileSystem::deleteFile(const std::string& path) {
    unlink(path.c_str());
}

std::vector<std::string> LinuxFileSystem::listFiles(const std::string& directory) {
    std::vector<std::string> files;
    DIR* dir = opendir(directory.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                files.push_back(entry->d_name);
            }
        }
        closedir(dir);
    }
    return files;
}

void LinuxFileSystem::createDirectory(const std::string& path) {
    mkdir(path.c_str(), 0755);
}

bool LinuxFileSystem::exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string LinuxFileSystem::getAppDataPath() {
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.local/share/gameengine";
    }
    return "/tmp";
}

std::string LinuxFileSystem::getDocumentsPath() {
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Documents";
    }
    return "/tmp";
}

// ========== LINUX TIMER ==========
LinuxTimer::LinuxTimer() : startTime_(SDL_GetPerformanceCounter()) {}

double LinuxTimer::now() {
    Uint64 now = SDL_GetPerformanceCounter();
    return static_cast<double>(now - startTime_) / SDL_GetPerformanceFrequency() * 1000.0;
}

int LinuxTimer::setTimeout(std::function<void()> callback, int delay) {
    // SDL doesn't have built-in timer functions, would need custom implementation
    return 0;
}

void LinuxTimer::clearTimeout(int id) {
    // Implementation needed
}

int LinuxTimer::setInterval(std::function<void()> callback, int delay) {
    // Implementation needed
    return 0;
}

void LinuxTimer::clearInterval(int id) {
    // Implementation needed
}

int LinuxTimer::requestAnimationFrame(std::function<void(double)> callback) {
    // Linux doesn't have requestAnimationFrame, use timer
    return 0;
}

void LinuxTimer::cancelAnimationFrame(int id) {
    // Implementation needed
}

// ========== LINUX RANDOM ==========
double LinuxRandom::random() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
}

int LinuxRandom::randomInt(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

double LinuxRandom::randomFloat(double min, double max) {
    return min + (random() * (max - min));
}

void LinuxRandom::seed(unsigned int seed) {
    generator_.seed(seed);
}

private:
    std::mt19937 generator_;

// ========== LINUX APPLICATION ==========
LinuxApplication::LinuxApplication() : running_(false) {
    graphics_ = std::make_unique<LinuxGraphics>();
    audio_ = std::make_unique<LinuxAudio>();
    input_ = std::make_unique<LinuxInput>();
    fileSystem_ = std::make_unique<LinuxFileSystem>();
    timer_ = std::make_unique<LinuxTimer>();
    random_ = std::make_unique<LinuxRandom>();
}

LinuxApplication::~LinuxApplication() {
    shutdown();
}

bool LinuxApplication::initialize(int width, int height, const std::string& title) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create window
    window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window_) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create OpenGL context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize audio
    if (!audio_->initialize()) {
        std::cerr << "Audio initialization failed" << std::endl;
        return false;
    }

    // Make context current for graphics
    graphics_->makeCurrent(window_);

    return true;
}

void LinuxApplication::run() {
    running_ = true;
    Uint64 lastTime = SDL_GetPerformanceCounter();
    double targetFrameTime = 1000.0 / 60.0; // 60 FPS

    while (running_) {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        double deltaTime = static_cast<double>(currentTime - lastTime) / SDL_GetPerformanceFrequency() * 1000.0;
        lastTime = currentTime;

        processEvents();
        update(static_cast<float>(deltaTime));
        render();

        SDL_GL_SwapWindow(window_);

        // Frame rate limiting
        double frameTime = static_cast<double>(SDL_GetPerformanceCounter() - currentTime) / SDL_GetPerformanceFrequency() * 1000.0;
        if (frameTime < targetFrameTime) {
            SDL_Delay(static_cast<Uint32>(targetFrameTime - frameTime));
        }
    }
}

void LinuxApplication::shutdown() {
    running_ = false;

    if (glContext_) {
        SDL_GL_DeleteContext(glContext_);
        glContext_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    audio_->shutdown();
    SDL_Quit();
}

void LinuxApplication::update(float deltaTime) {
    input_->update();
    // Engine update would go here
}

void LinuxApplication::render() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Engine render would go here
}

void LinuxApplication::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            running_ = false;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEMOTION:
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
            input_->handleSDLEvent(event);
            break;
        default:
            break;
        }
    }
}

// ========== MAIN ENTRY POINT ==========
int main(int argc, char* argv[]) {
    LinuxApplication app;

    if (!app.initialize(1280, 720, "Game Engine")) {
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}
