/**
 * Android Platform Implementation
 * Native Android platform using C++ with JNI bridge to Java
 */

#include "AndroidPlatform.h"
#include "../../core/Platform.h"
#include "../../core/System.h"
#include "../../math/Vector3.h"
#include "../../math/Quaternion.h"
#include "../../math/Matrix4.h"

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <jni.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <functional>

namespace FoundryEngine {

// Logging
#define LOG_TAG "GameEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// JNI references
static JavaVM* gJavaVM = nullptr;
static jobject gActivity = nullptr;
static jclass gActivityClass = nullptr;

// Forward declarations
class AndroidGraphics;
class AndroidAudio;
class AndroidInput;
class AndroidFileSystem;

class AndroidPlatform : public Platform {
private:
    std::unique_ptr<AndroidGraphics> graphics_;
    std::unique_ptr<AndroidAudio> audio_;
    std::unique_ptr<AndroidInput> input_;
    std::unique_ptr<AndroidFileSystem> fileSystem_;
    std::unique_ptr<AndroidTimer> timer_;
    std::unique_ptr<AndroidRandom> random_;

    PlatformCapabilities capabilities_;
    int windowWidth_ = 1080;
    int windowHeight_ = 1920;

public:
    AndroidPlatform();
    ~AndroidPlatform();

    // Platform interface implementation
    PlatformCapabilities getCapabilities() override { return capabilities_; }
    std::unique_ptr<PlatformCanvas> createCanvas(int width, int height) override;
    PlatformInputManager* getInputManager() override { return input_.get(); }
    PlatformFileSystem* getFileSystem() override { return fileSystem_.get(); }
    PlatformNetworking* getNetworking() override;
    PlatformAudio* getAudio() override { return audio_.get(); }
    PlatformGraphics* getGraphics() override { return graphics_.get(); }
    PlatformTimer* getTimer() override { return timer_.get(); }
    PlatformRandom* getRandom() override { return random_.get(); }

    // Android-specific methods
    void setJavaVM(JavaVM* vm) { gJavaVM = vm; }
    void setActivity(jobject activity) { gActivity = activity; }
    void onSurfaceCreated(ANativeWindow* window);
    void onSurfaceChanged(int width, int height);
    void onSurfaceDestroyed();

private:
    void detectCapabilities();
    void initializeJNI();
};

// ========== ANDROID GRAPHICS ==========
class AndroidGraphics : public PlatformGraphics {
private:
    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLConfig config_ = nullptr;
    ANativeWindow* window_ = nullptr;

public:
    AndroidGraphics();
    ~AndroidGraphics();

    bool initialize(ANativeWindow* window);
    void shutdown();

    // PlatformGraphics interface
    std::unique_ptr<PlatformGraphicsContext> createContext() override;
    PlatformCapabilities getCapabilities() override;

    // Android-specific
    void swapBuffers();
    void makeCurrent();
    void setWindow(ANativeWindow* window);
};

class AndroidGLContext : public PlatformGraphicsContext {
private:
    AndroidGraphics* graphics_;

public:
    AndroidGLContext(AndroidGraphics* graphics) : graphics_(graphics) {}

    // WebGL-style interface for compatibility
    void viewport(int x, int y, int width, int height) override {
        glViewport(x, y, width, height);
    }

    void clear(unsigned int mask) override {
        glClear(mask);
    }

    void clearColor(float r, float g, float b, float a) override {
        glClearColor(r, g, b, a);
    }

    void enable(unsigned int cap) override {
        glEnable(cap);
    }

    void disable(unsigned int cap) override {
        glDisable(cap);
    }

    // ... implement all other WebGL methods
    void cullFace(unsigned int mode) override { glCullFace(mode); }
    void depthFunc(unsigned int func) override { glDepthFunc(func); }
    void blendFunc(unsigned int sfactor, unsigned int dfactor) override { glBlendFunc(sfactor, dfactor); }

    unsigned int createBuffer() override {
        unsigned int buffer;
        glGenBuffers(1, &buffer);
        return buffer;
    }

    void bindBuffer(unsigned int target, unsigned int buffer) override {
        glBindBuffer(target, buffer);
    }

    void bufferData(unsigned int target, const void* data, size_t size, unsigned int usage) override {
        glBufferData(target, size, data, usage);
    }

    void deleteBuffer(unsigned int buffer) override {
        glDeleteBuffers(1, &buffer);
    }

    // Shader and program methods
    unsigned int createShader(unsigned int type) override {
        return glCreateShader(type);
    }

    void shaderSource(unsigned int shader, const std::string& source) override {
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
    }

    void compileShader(unsigned int shader) override {
        glCompileShader(shader);
    }

    int getShaderParameter(unsigned int shader, unsigned int pname) override {
        int result;
        glGetShaderiv(shader, pname, &result);
        return result;
    }

    std::string getShaderInfoLog(unsigned int shader) override {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        return std::string(infoLog);
    }

    void deleteShader(unsigned int shader) override {
        glDeleteShader(shader);
    }

    unsigned int createProgram() override {
        return glCreateProgram();
    }

    void attachShader(unsigned int program, unsigned int shader) override {
        glAttachShader(program, shader);
    }

    void linkProgram(unsigned int program) override {
        glLinkProgram(program);
    }

    int getProgramParameter(unsigned int program, unsigned int pname) override {
        int result;
        glGetProgramiv(program, pname, &result);
        return result;
    }

    std::string getProgramInfoLog(unsigned int program) override {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        return std::string(infoLog);
    }

    void useProgram(unsigned int program) override {
        glUseProgram(program);
    }

    void deleteProgram(unsigned int program) override {
        glDeleteProgram(program);
    }

    int getAttribLocation(unsigned int program, const std::string& name) override {
        return glGetAttribLocation(program, name.c_str());
    }

    int getUniformLocation(unsigned int program, const std::string& name) override {
        return glGetUniformLocation(program, name.c_str());
    }

    // ... implement remaining methods
};

// ========== ANDROID AUDIO ==========
class AndroidAudio : public PlatformAudio {
private:
    JNIEnv* env_ = nullptr;
    jobject audioManager_ = nullptr;

public:
    AndroidAudio();
    ~AndroidAudio();

    std::unique_ptr<PlatformAudioContext> createContext() override;

    void resume() override;
    void suspend() override;
};

class AndroidAudioContext : public PlatformAudioContext {
private:
    JNIEnv* env_ = nullptr;

public:
    AndroidAudioContext(JNIEnv* env) : env_(env) {}

    std::unique_ptr<PlatformAudioBuffer> createBuffer(unsigned int channels, unsigned int length, float sampleRate) override;
    std::unique_ptr<PlatformAudioBufferSource> createBufferSource() override;
    std::unique_ptr<PlatformGainNode> createGain() override;
    PlatformAudioDestination* getDestination() override;

    float getCurrentTime() override;
    float getSampleRate() override;
};

// ========== ANDROID INPUT ==========
class AndroidInput : public PlatformInputManager {
private:
    JNIEnv* env_ = nullptr;
    std::vector<GamepadState> gamepadStates_;
    std::unordered_map<int, bool> keyStates_;
    std::vector<TouchPoint> touchPoints_;
    std::vector<std::function<void(const InputEvent&)>> listeners_;

public:
    AndroidInput(JNIEnv* env) : env_(env) {}
    ~AndroidInput() {}

    std::unordered_map<int, bool> getKeyboardState() override { return keyStates_; }
    MouseState getMouseState() override { return {0, 0, {}}; }
    std::vector<TouchPoint> getTouchState() override { return touchPoints_; }
    GamepadState getGamepadState(int index) override;
    std::vector<GamepadState> getConnectedGamepads() override;
    int getGamepadCount() override;
    bool isGamepadConnected(int index) override;
    std::string getGamepadName(int index) override;
    bool setGamepadVibration(int index, float leftMotor, float rightMotor, float duration) override;

    void addEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override;
    void removeEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override;

    // Android-specific input handling
    void onTouchEvent(int action, float x, float y, int pointerId);
    void onKeyEvent(int keyCode, int action);
    void updateGamepadStates();
    void onGamepadConnected(int deviceId);
    void onGamepadDisconnected(int deviceId);
};

// ========== ANDROID FILE SYSTEM ==========
class AndroidFileSystem : public PlatformFileSystem {
private:
    JNIEnv* env_ = nullptr;
    std::string internalPath_;
    std::string externalPath_;

public:
    AndroidFileSystem(JNIEnv* env);

    std::vector<uint8_t> readFile(const std::string& path) override;
    void writeFile(const std::string& path, const std::vector<uint8_t>& data) override;
    void deleteFile(const std::string& path) override;
    std::vector<std::string> listFiles(const std::string& directory) override;
    void createDirectory(const std::string& path) override;
    bool exists(const std::string& path) override;
};

// ========== ANDROID TIMER ==========
class AndroidTimer : public PlatformTimer {
private:
    std::chrono::steady_clock::time_point startTime_;

public:
    AndroidTimer() : startTime_(std::chrono::steady_clock::now()) {}

    double now() override {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_).count();
    }

    int setTimeout(std::function<void()> callback, int delay) override;
    void clearTimeout(int id) override;
    int setInterval(std::function<void()> callback, int delay) override;
    void clearInterval(int id) override;
    int requestAnimationFrame(std::function<void(double)> callback) override;
    void cancelAnimationFrame(int id) override;
};

// ========== ANDROID RANDOM ==========
class AndroidRandom : public PlatformRandom {
public:
    double random() override {
        return static_cast<double>(rand()) / RAND_MAX;
    }

    int randomInt(int min, int max) override {
        return min + (rand() % (max - min + 1));
    }

    double randomFloat(double min, double max) override {
        return min + (random() * (max - min));
    }

    void seed(unsigned int seed) override {
        srand(seed);
    }
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {
    JNIEXPORT void JNICALL Java_com_example_gameengine_GameEngine_nativeOnCreate(JNIEnv* env, jobject thiz, jobject activity) {
        LOGI("Native onCreate called");

        // Store Java VM and activity references
        env->GetJavaVM(&gJavaVM);
        gActivity = env->NewGlobalRef(activity);

        // Get activity class
        jclass activityClass = env->GetObjectClass(activity);
        gActivityClass = (jclass)env->NewGlobalRef(activityClass);
        env->DeleteLocalRef(activityClass);
    }

    JNIEXPORT void JNICALL Java_com_example_gameengine_GameEngine_nativeOnDestroy(JNIEnv* env, jobject thiz) {
        LOGI("Native onDestroy called");

        // Clean up JNI references
        if (gActivity) {
            env->DeleteGlobalRef(gActivity);
            gActivity = nullptr;
        }
        if (gActivityClass) {
            env->DeleteGlobalRef(gActivityClass);
            gActivityClass = nullptr;
        }
        gJavaVM = nullptr;
    }

    JNIEXPORT void JNICALL Java_com_example_gameengine_GameEngine_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz, jobject surface) {
        LOGI("Native onSurfaceCreated called");

        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        // Platform instance would handle this
    }

    JNIEXPORT void JNICALL Java_com_example_gameengine_GameEngine_nativeOnSurfaceChanged(JNIEnv* env, jobject thiz, jint width, jint height) {
        LOGI("Native onSurfaceChanged: %dx%d", width, height);
        // Platform instance would handle this
    }

    JNIEXPORT void JNICALL Java_com_example_gameengine_GameEngine_nativeOnTouchEvent(JNIEnv* env, jobject thiz, jint action, jfloat x, jfloat y, jint pointerId) {
        LOGI("Native onTouchEvent: action=%d, x=%.1f, y=%.1f, pointerId=%d", action, x, y, pointerId);
        // Platform instance would handle this
    }

    JNIEXPORT void JNICALL Java_com_example_gameengine_GameEngine_nativeOnKeyEvent(JNIEnv* env, jobject thiz, jint keyCode, jint action) {
        LOGI("Native onKeyEvent: keyCode=%d, action=%d", keyCode, action);
        // Platform instance would handle this
    }

    JNIEXPORT void JNICALL Java_com_example_gameengine_GameEngine_nativeOnGamepadConnected(JNIEnv* env, jobject thiz, jint deviceId) {
        LOGI("Native onGamepadConnected: deviceId=%d", deviceId);
        // Platform instance would handle this
    }

    JNIEXPORT void JNICALL Java_com_example_gameengine_GameEngine_nativeOnGamepadDisconnected(JNIEnv* env, jobject thiz, jint deviceId) {
        LOGI("Native onGamepadDisconnected: deviceId=%d", deviceId);
        // Platform instance would handle this
    }
}

// ========== ANDROID INPUT IMPLEMENTATIONS ==========
GamepadState AndroidInput::getGamepadState(int index) {
    if (index < 0 || index >= static_cast<int>(gamepadStates_.size())) {
        return {false, "", {}, {}};
    }
    return gamepadStates_[index];
}

std::vector<GamepadState> AndroidInput::getConnectedGamepads() {
    std::vector<GamepadState> connected;
    for (const auto& gamepad : gamepadStates_) {
        if (gamepad.connected) {
            connected.push_back(gamepad);
        }
    }
    return connected;
}

int AndroidInput::getGamepadCount() {
    return 4; // Android typically supports up to 4 controllers
}

bool AndroidInput::isGamepadConnected(int index) {
    if (index < 0 || index >= static_cast<int>(gamepadStates_.size())) return false;
    return gamepadStates_[index].connected;
}

std::string AndroidInput::getGamepadName(int index) {
    if (!isGamepadConnected(index)) return "";
    return gamepadStates_[index].id;
}

bool AndroidInput::setGamepadVibration(int index, float leftMotor, float rightMotor, float duration) {
    if (!env_ || index < 0 || index >= static_cast<int>(gamepadStates_.size())) return false;

    // Call Java method to set vibration
    jclass inputManagerClass = env_->FindClass("android/hardware/input/InputManager");
    if (!inputManagerClass) return false;

    // Get vibrate method
    jmethodID vibrateMethod = env_->GetMethodID(inputManagerClass, "vibrate",
        "(IJLandroid/os/VibrationEffect;)V");

    if (vibrateMethod) {
        // Create vibration effect
        jclass vibrationEffectClass = env_->FindClass("android/os/VibrationEffect");
        jmethodID createOneShotMethod = env_->GetStaticMethodID(vibrationEffectClass,
            "createOneShot", "(JI)Landroid/os/VibrationEffect;");

        if (createOneShotMethod) {
            // Use stronger motor value for vibration
            int vibrationAmplitude = static_cast<int>((leftMotor + rightMotor) / 2.0f * 255.0f);
            long vibrationDuration = static_cast<long>(duration * 1000.0f);

            jobject vibrationEffect = env_->CallStaticObjectMethod(vibrationEffectClass,
                createOneShotMethod, vibrationDuration, vibrationAmplitude);

            if (vibrationEffect) {
                // Call vibrate on input device
                env_->CallVoidMethod(inputManager_, vibrateMethod,
                    index, vibrationDuration, vibrationEffect);
                env_->DeleteLocalRef(vibrationEffect);
                return true;
            }
        }
        env_->DeleteLocalRef(vibrationEffectClass);
    }
    env_->DeleteLocalRef(inputManagerClass);

    return false;
}

void AndroidInput::addEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) {
    listeners_.push_back(listener);
}

void AndroidInput::removeEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) {
    // Remove listener implementation
}

void AndroidInput::onTouchEvent(int action, float x, float y, int pointerId) {
    TouchPoint touchPoint = {x, y, pointerId};
    touchPoints_.push_back(touchPoint);

    InputEvent event = {"touch", "move"};
    event.position = {x, y};
    event.timestamp = 0; // Would need proper timestamp

    for (auto& listener : listeners_) {
        listener(event);
    }
}

void AndroidInput::onKeyEvent(int keyCode, int action) {
    bool pressed = (action == 0); // ACTION_DOWN
    keyStates_[keyCode] = pressed;

    InputEvent event = {"keyboard", pressed ? "press" : "release"};
    event.key = keyCode;
    event.timestamp = 0; // Would need proper timestamp

    for (auto& listener : listeners_) {
        listener(event);
    }
}

void AndroidInput::updateGamepadStates() {
    // Update gamepad states through JNI calls to Android InputManager
    if (!env_) return;

    // This would typically poll the Android InputManager for gamepad states
    // For now, we'll maintain the current state
}

void AndroidInput::onGamepadConnected(int deviceId) {
    if (deviceId >= 0 && deviceId < 4) {
        if (static_cast<size_t>(deviceId) >= gamepadStates_.size()) {
            gamepadStates_.resize(deviceId + 1);
        }

        gamepadStates_[deviceId] = {true, "Android Gamepad", {}, {}};
        LOGI("Gamepad connected: %d", deviceId);
    }
}

void AndroidInput::onGamepadDisconnected(int deviceId) {
    if (deviceId >= 0 && deviceId < static_cast<int>(gamepadStates_.size())) {
        gamepadStates_[deviceId] = {false, "", {}, {}};
        LOGI("Gamepad disconnected: %d", deviceId);
    }
}

// ========== ANDROID PLATFORM IMPLEMENTATIONS ==========
AndroidPlatform::AndroidPlatform() {
    detectCapabilities();
    initializeJNI();

    graphics_ = std::make_unique<AndroidGraphics>();
    audio_ = std::make_unique<AndroidAudio>();
    input_ = std::make_unique<AndroidInput>(nullptr); // Will be set later
    fileSystem_ = std::make_unique<AndroidFileSystem>(nullptr); // Will be set later
    timer_ = std::make_unique<AndroidTimer>();
    random_ = std::make_unique<AndroidRandom>();
}

AndroidPlatform::~AndroidPlatform() {
    // Cleanup handled by unique_ptr
}

std::unique_ptr<PlatformCanvas> AndroidPlatform::createCanvas(int width, int height) {
    return std::make_unique<AndroidCanvas>(width, height);
}

PlatformNetworking* AndroidPlatform::getNetworking() {
    static AndroidNetworking networking;
    return &networking;
}

void AndroidPlatform::detectCapabilities() {
    capabilities_.supportsWebGL = true;
    capabilities_.supportsWebGL2 = true;
    capabilities_.supportsComputeShaders = false; // Android OpenGL ES 3.1+
    capabilities_.supportsRayTracing = false;
    capabilities_.maxTextureSize = 4096;
    capabilities_.maxRenderbufferSize = 4096;
    capabilities_.maxViewportDims = {4096, 4096};
    capabilities_.maxVertexTextureImageUnits = 16;
    capabilities_.maxTextureImageUnits = 16;
    capabilities_.maxFragmentUniformVectors = 224;
    capabilities_.maxVertexUniformVectors = 256;
    capabilities_.maxVaryingVectors = 15;
    capabilities_.maxVertexAttribs = 16;
    capabilities_.maxCombinedTextureImageUnits = 32;
    capabilities_.maxCubeMapTextureSize = 4096;
    capabilities_.maxRenderbufferSize = 4096;
    capabilities_.maxTextureImageUnits = 16;
    capabilities_.maxTextureSize = 4096;
    capabilities_.maxVaryingVectors = 15;
    capabilities_.maxVertexTextureImageUnits = 16;
    capabilities_.maxCombinedTextureImageUnits = 32;
    capabilities_.maxCubeMapTextureSize = 4096;
    capabilities_.maxFragmentUniformVectors = 224;
    capabilities_.maxVertexUniformVectors = 256;
    capabilities_.maxVertexAttribs = 16;
}

void AndroidPlatform::initializeJNI() {
    // JNI initialization would happen in the Java activity
    LOGI("Android platform initialized");
}

void AndroidPlatform::onSurfaceCreated(ANativeWindow* window) {
    LOGI("Surface created");
    graphics_->initialize(window);
}

void AndroidPlatform::onSurfaceChanged(int width, int height) {
    LOGI("Surface changed: %dx%d", width, height);
    windowWidth_ = width;
    windowHeight_ = height;
}

void AndroidPlatform::onSurfaceDestroyed() {
    LOGI("Surface destroyed");
    graphics_->shutdown();
}

// ========== ANDROID GRAPHICS IMPLEMENTATIONS ==========
AndroidGraphics::AndroidGraphics() = default;

AndroidGraphics::~AndroidGraphics() {
    shutdown();
}

bool AndroidGraphics::initialize(ANativeWindow* window) {
    window_ = window;

    // Initialize EGL
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }

    if (!eglInitialize(display_, nullptr, nullptr)) {
        LOGE("Failed to initialize EGL");
        return false;
    }

    // Choose configuration
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };

    EGLint numConfigs;
    if (!eglChooseConfig(display_, attribs, &config_, 1, &numConfigs)) {
        LOGE("Failed to choose EGL config");
        return false;
    }

    // Create surface
    surface_ = eglCreateWindowSurface(display_, config_, window_, nullptr);
    if (surface_ == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return false;
    }

    // Create context
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return false;
    }

    // Make context current
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        LOGE("Failed to make EGL context current");
        return false;
    }

    LOGI("Android graphics initialized successfully");
    return true;
}

void AndroidGraphics::shutdown() {
    if (context_ != EGL_NO_CONTEXT) {
        eglDestroyContext(display_, context_);
        context_ = EGL_NO_CONTEXT;
    }

    if (surface_ != EGL_NO_SURFACE) {
        eglDestroySurface(display_, surface_);
        surface_ = EGL_NO_SURFACE;
    }

    if (display_ != EGL_NO_DISPLAY) {
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

std::unique_ptr<PlatformGraphicsContext> AndroidGraphics::createContext() {
    return std::make_unique<AndroidGLContext>(this);
}

PlatformCapabilities AndroidGraphics::getCapabilities() {
    PlatformCapabilities caps;
    caps.supportsWebGL = true;
    caps.supportsWebGL2 = true;
    caps.supportsComputeShaders = false;
    caps.supportsRayTracing = false;
    caps.maxTextureSize = 4096;
    caps.maxRenderbufferSize = 4096;
    caps.maxViewportDims = {4096, 4096};
    caps.maxVertexTextureImageUnits = 16;
    caps.maxTextureImageUnits = 16;
    caps.maxFragmentUniformVectors = 224;
    caps.maxVertexUniformVectors = 256;
    caps.maxVaryingVectors = 15;
    caps.maxVertexAttribs = 16;
    caps.maxCombinedTextureImageUnits = 32;
    caps.maxCubeMapTextureSize = 4096;
    caps.maxRenderbufferSize = 4096;
    caps.maxTextureImageUnits = 16;
    caps.maxTextureSize = 4096;
    caps.maxVaryingVectors = 15;
    caps.maxVertexTextureImageUnits = 16;
    caps.maxCombinedTextureImageUnits = 32;
    caps.maxCubeMapTextureSize = 4096;
    caps.maxFragmentUniformVectors = 224;
    caps.maxVertexUniformVectors = 256;
    caps.maxVertexAttribs = 16;
    return caps;
}

void AndroidGraphics::swapBuffers() {
    eglSwapBuffers(display_, surface_);
}

void AndroidGraphics::makeCurrent() {
    eglMakeCurrent(display_, surface_, surface_, context_);
}

void AndroidGraphics::setWindow(ANativeWindow* window) {
    window_ = window;
}

// ========== ANDROID AUDIO IMPLEMENTATIONS ==========
AndroidAudio::AndroidAudio() = default;

AndroidAudio::~AndroidAudio() = default;

std::unique_ptr<PlatformAudioContext> AndroidAudio::createContext() {
    return std::make_unique<AndroidAudioContext>(nullptr); // Will be set later
}

void AndroidAudio::resume() {
    LOGI("Audio resumed");
}

void AndroidAudio::suspend() {
    LOGI("Audio suspended");
}

std::unique_ptr<PlatformAudioBuffer> AndroidAudioContext::createBuffer(unsigned int channels, unsigned int length, float sampleRate) {
    // Implementation would create OpenSL ES buffer
    return nullptr;
}

std::unique_ptr<PlatformAudioBufferSource> AndroidAudioContext::createBufferSource() {
    // Implementation would create OpenSL ES buffer source
    return nullptr;
}

std::unique_ptr<PlatformGainNode> AndroidAudioContext::createGain() {
    // Implementation would create OpenSL ES gain node
    return nullptr;
}

PlatformAudioDestination* AndroidAudioContext::getDestination() {
    // Implementation would return OpenSL ES destination
    return nullptr;
}

float AndroidAudioContext::getCurrentTime() {
    return 0.0f; // Would need proper implementation
}

float AndroidAudioContext::getSampleRate() {
    return 44100.0f; // Standard sample rate
}

// ========== ANDROID FILE SYSTEM IMPLEMENTATIONS ==========
AndroidFileSystem::AndroidFileSystem(JNIEnv* env) : env_(env) {
    // Get internal and external paths
    internalPath_ = "/data/data/com.foundryengine.game/files/";
    externalPath_ = "/sdcard/Android/data/com.foundryengine.game/files/";
}

std::vector<uint8_t> AndroidFileSystem::readFile(const std::string& path) {
    if (!env_) return {};

    // Use JNI to read file from Android assets or internal storage
    jclass fileUtilsClass = env_->FindClass("com/foundryengine/game/FileUtils");
    if (!fileUtilsClass) return {};

    jmethodID readFileMethod = env_->GetStaticMethodID(fileUtilsClass, "readFile", "(Ljava/lang/String;)[B");
    if (!readFileMethod) return {};

    jstring pathStr = env_->NewStringUTF(path.c_str());
    jbyteArray result = (jbyteArray)env_->CallStaticObjectMethod(fileUtilsClass, readFileMethod, pathStr);

    if (result) {
        jsize length = env_->GetArrayLength(result);
        std::vector<uint8_t> data(length);
        env_->GetByteArrayRegion(result, 0, length, reinterpret_cast<jbyte*>(data.data()));
        env_->DeleteLocalRef(result);
        env_->DeleteLocalRef(pathStr);
        env_->DeleteLocalRef(fileUtilsClass);
        return data;
    }

    env_->DeleteLocalRef(pathStr);
    env_->DeleteLocalRef(fileUtilsClass);
    return {};
}

void AndroidFileSystem::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    if (!env_) return;

    // Use JNI to write file to Android internal storage
    jclass fileUtilsClass = env_->FindClass("com/foundryengine/game/FileUtils");
    if (!fileUtilsClass) return;

    jmethodID writeFileMethod = env_->GetStaticMethodID(fileUtilsClass, "writeFile", "(Ljava/lang/String;[B)V");
    if (!writeFileMethod) return;

    jstring pathStr = env_->NewStringUTF(path.c_str());
    jbyteArray dataArray = env_->NewByteArray(data.size());
    env_->SetByteArrayRegion(dataArray, 0, data.size(), reinterpret_cast<const jbyte*>(data.data()));

    env_->CallStaticVoidMethod(fileUtilsClass, writeFileMethod, pathStr, dataArray);

    env_->DeleteLocalRef(dataArray);
    env_->DeleteLocalRef(pathStr);
    env_->DeleteLocalRef(fileUtilsClass);
}

void AndroidFileSystem::deleteFile(const std::string& path) {
    if (!env_) return;

    // Use JNI to delete file
    jclass fileUtilsClass = env_->FindClass("com/foundryengine/game/FileUtils");
    if (!fileUtilsClass) return;

    jmethodID deleteFileMethod = env_->GetStaticMethodID(fileUtilsClass, "deleteFile", "(Ljava/lang/String;)V");
    if (!deleteFileMethod) return;

    jstring pathStr = env_->NewStringUTF(path.c_str());
    env_->CallStaticVoidMethod(fileUtilsClass, deleteFileMethod, pathStr);

    env_->DeleteLocalRef(pathStr);
    env_->DeleteLocalRef(fileUtilsClass);
}

std::vector<std::string> AndroidFileSystem::listFiles(const std::string& directory) {
    if (!env_) return {};

    // Use JNI to list files
    jclass fileUtilsClass = env_->FindClass("com/foundryengine/game/FileUtils");
    if (!fileUtilsClass) return {};

    jmethodID listFilesMethod = env_->GetStaticMethodID(fileUtilsClass, "listFiles", "(Ljava/lang/String;)[Ljava/lang/String;");
    if (!listFilesMethod) return {};

    jstring dirStr = env_->NewStringUTF(directory.c_str());
    jobjectArray result = (jobjectArray)env_->CallStaticObjectMethod(fileUtilsClass, listFilesMethod, dirStr);

    std::vector<std::string> files;
    if (result) {
        jsize length = env_->GetArrayLength(result);
        for (jsize i = 0; i < length; ++i) {
            jstring fileStr = (jstring)env_->GetObjectArrayElement(result, i);
            const char* fileChars = env_->GetStringUTFChars(fileStr, nullptr);
            files.push_back(fileChars);
            env_->ReleaseStringUTFChars(fileStr, fileChars);
            env_->DeleteLocalRef(fileStr);
        }
        env_->DeleteLocalRef(result);
    }

    env_->DeleteLocalRef(dirStr);
    env_->DeleteLocalRef(fileUtilsClass);
    return files;
}

void AndroidFileSystem::createDirectory(const std::string& path) {
    if (!env_) return;

    // Use JNI to create directory
    jclass fileUtilsClass = env_->FindClass("com/foundryengine/game/FileUtils");
    if (!fileUtilsClass) return;

    jmethodID createDirMethod = env_->GetStaticMethodID(fileUtilsClass, "createDirectory", "(Ljava/lang/String;)V");
    if (!createDirMethod) return;

    jstring pathStr = env_->NewStringUTF(path.c_str());
    env_->CallStaticVoidMethod(fileUtilsClass, createDirMethod, pathStr);

    env_->DeleteLocalRef(pathStr);
    env_->DeleteLocalRef(fileUtilsClass);
}

bool AndroidFileSystem::exists(const std::string& path) {
    if (!env_) return false;

    // Use JNI to check if file exists
    jclass fileUtilsClass = env_->FindClass("com/foundryengine/game/FileUtils");
    if (!fileUtilsClass) return false;

    jmethodID existsMethod = env_->GetStaticMethodID(fileUtilsClass, "exists", "(Ljava/lang/String;)Z");
    if (!existsMethod) return false;

    jstring pathStr = env_->NewStringUTF(path.c_str());
    jboolean result = env_->CallStaticBooleanMethod(fileUtilsClass, existsMethod, pathStr);

    env_->DeleteLocalRef(pathStr);
    env_->DeleteLocalRef(fileUtilsClass);
    return result;
}

// ========== ANDROID TIMER IMPLEMENTATIONS ==========
int AndroidTimer::setTimeout(std::function<void()> callback, int delay) {
    // Implementation would use Android Handler
    return 0;
}

void AndroidTimer::clearTimeout(int id) {
    // Implementation would cancel Android Handler
}

int AndroidTimer::setInterval(std::function<void()> callback, int delay) {
    // Implementation would use Android Handler with repetition
    return 0;
}

void AndroidTimer::clearInterval(int id) {
    // Implementation would cancel Android Handler
}

int AndroidTimer::requestAnimationFrame(std::function<void(double)> callback) {
    // Implementation would use Choreographer for frame callbacks
    return 0;
}

void AndroidTimer::cancelAnimationFrame(int id) {
    // Implementation would cancel Choreographer callback
}

// ========== ANDROID NETWORKING IMPLEMENTATIONS ==========
class AndroidNetworking : public PlatformNetworking {
public:
    std::unique_ptr<PlatformWebSocket> connect(const std::string& url) override {
        // Implementation would use Android WebSocket libraries
        return nullptr;
    }

    std::vector<uint8_t> httpGet(const std::string& url) override {
        // Implementation would use Android HTTP libraries
        return {};
    }

    std::vector<uint8_t> httpPost(const std::string& url, const std::vector<uint8_t>& data) override {
        // Implementation would use Android HTTP libraries
        return {};
    }
};

// ========== ANDROID CANVAS IMPLEMENTATIONS ==========
PlatformGraphicsContext* AndroidCanvas::getContext(const std::string& contextType) {
    // Return Android GL context
    return nullptr;
}

} // namespace FoundryEngine
