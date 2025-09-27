#include "WebPlatformPAL.h"
#include "GameEngine/networking/UDPNetworking.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/webrtc.h>
#include <emscripten/webaudio.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>

namespace FoundryEngine {

// Static registry for platform creators
static std::unordered_map<PlatformType, std::function<std::unique_ptr<PlatformInterface>()>> platformCreators;
static std::mutex platformRegistryMutex;

// ========== WEB PLATFORM PAL IMPLEMENTATION ==========
WebPlatformPAL::WebPlatformPAL()
    : PlatformInterface(), canvasId_("gameCanvas"), webGLContext_(0), pointerLocked_(false),
      fullscreen_(false), initialized_(false), appActive_(true), windowFocused_(true) {
}

WebPlatformPAL::~WebPlatformPAL() {
    shutdown();
}

void WebPlatformPAL::initialize() {
    if (initialized_) return;

    // Initialize platform capabilities
    initializeCapabilities();

    // Query system information
    querySystemInformation();

    // Initialize Web contexts
    initializeGraphicsContext();
    initializeAudioContext();
    initializeInputContext();
    initializeNetworkContext();
    initializeStorageContext();

    // Initialize Web services
    initializePlatformServices();
    initializeWindowManager();
    initializeEventSystem();

    // Start main loop
    startMainLoop();

    initialized_ = true;
}

void WebPlatformPAL::update(float dt) {
    if (!initialized_) return;

    // Update performance stats
    updateMemoryStats();
    updateCPUStats();

    // Update contexts
    if (graphicsContext_) graphicsContext_->update(dt);
    if (audioContext_) audioContext_->update(dt);
    if (inputContext_) inputContext_->update();
    if (networkContext_) networkContext_->update();
    if (storageContext_) storageContext_->update();
    if (platformServices_) platformServices_->update();
    if (windowManager_) windowManager_->update();
    if (eventSystem_) eventSystem_->update();
}

void WebPlatformPAL::shutdown() {
    if (!initialized_) return;

    // Stop main loop
    stopMainLoop();

    // Shutdown contexts
    if (graphicsContext_) {
        graphicsContext_->shutdown();
        delete graphicsContext_;
        graphicsContext_ = nullptr;
    }

    if (audioContext_) {
        audioContext_->shutdown();
        delete audioContext_;
        audioContext_ = nullptr;
    }

    if (inputContext_) {
        inputContext_->shutdown();
        delete inputContext_;
        inputContext_ = nullptr;
    }

    if (networkContext_) {
        networkContext_->shutdown();
        delete networkContext_;
        networkContext_ = nullptr;
    }

    if (storageContext_) {
        storageContext_->shutdown();
        delete storageContext_;
        storageContext_ = nullptr;
    }

    if (platformServices_) {
        platformServices_->shutdown();
        delete platformServices_;
        platformServices_ = nullptr;
    }

    if (windowManager_) {
        windowManager_->shutdown();
        delete windowManager_;
        windowManager_ = nullptr;
    }

    if (eventSystem_) {
        eventSystem_->shutdown();
        delete eventSystem_;
        eventSystem_ = nullptr;
    }

    initialized_ = false;
}

std::string WebPlatformPAL::getPlatformVersion() const {
    return "1.0.0";
}

void WebPlatformPAL::onAppStart() {
    appActive_ = true;
    sendEvent(PlatformEvent{PlatformEventType::APP_STARTED, std::chrono::steady_clock::now(), {}, nullptr});
}

void WebPlatformPAL::onAppPause() {
    appActive_ = false;
    sendEvent(PlatformEvent{PlatformEventType::APP_PAUSED, std::chrono::steady_clock::now(), {}, nullptr});
}

void WebPlatformPAL::onAppResume() {
    appActive_ = true;
    sendEvent(PlatformEvent{PlatformEventType::APP_RESUMED, std::chrono::steady_clock::now(), {}, nullptr});
}

void WebPlatformPAL::onAppTerminate() {
    sendEvent(PlatformEvent{PlatformEventType::APP_TERMINATED, std::chrono::steady_clock::now(), {}, nullptr});
}

void WebPlatformPAL::onAppBackground() {
    sendEvent(PlatformEvent{PlatformEventType::APP_BACKGROUND, std::chrono::steady_clock::now(), {}, nullptr});
}

void WebPlatformPAL::onAppForeground() {
    sendEvent(PlatformEvent{PlatformEventType::APP_FOREGROUND, std::chrono::steady_clock::now(), {}, nullptr});
}

void WebPlatformPAL::registerEventCallback(PlatformEventType type, PlatformEventCallback callback) {
    std::lock_guard<std::mutex> lock(platformMutex_);
    eventCallbacks_[type].push_back(callback);
}

void WebPlatformPAL::unregisterEventCallback(PlatformEventType type, PlatformEventCallback callback) {
    std::lock_guard<std::mutex> lock(platformMutex_);
    auto& callbacks = eventCallbacks_[type];
    callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback), callbacks.end());
}

void WebPlatformPAL::sendEvent(const PlatformEvent& event) {
    std::lock_guard<std::mutex> lock(platformMutex_);
    auto it = eventCallbacks_.find(event.type);
    if (it != eventCallbacks_.end()) {
        for (const auto& callback : it->second) {
            callback(event);
        }
    }
}

void WebPlatformPAL::setOrientation(int orientation) {
    setWebOrientation(orientation);
}

void WebPlatformPAL::setFullscreen(bool fullscreen) {
    setWebFullscreen(fullscreen);
}

void WebPlatformPAL::setKeepScreenOn(bool keepOn) {
    // Web doesn't have keep screen on functionality
}

void WebPlatformPAL::vibrate(int durationMs) {
    performWebVibration(durationMs);
}

void WebPlatformPAL::showToast(const std::string& message) {
    showWebToast(message);
}

size_t WebPlatformPAL::getTotalMemory() const {
    return totalMemory_;
}

size_t WebPlatformPAL::getAvailableMemory() const {
    return availableMemory_;
}

size_t WebPlatformPAL::getUsedMemory() const {
    return totalMemory_ - availableMemory_;
}

void WebPlatformPAL::garbageCollect() {
    emscripten_run_script("if (typeof gc !== 'undefined') gc();");
}

float WebPlatformPAL::getCPUUsage() const {
    return cpuUsage_;
}

float WebPlatformPAL::getMemoryUsage() const {
    return memoryUsage_;
}

float WebPlatformPAL::getBatteryLevel() const {
    return 1.0f; // Web doesn't have battery API
}

bool WebPlatformPAL::isBatteryCharging() const {
    return true; // Web doesn't have battery API
}

std::string WebPlatformPAL::getDeviceId() const {
    return deviceId_;
}

std::string WebPlatformPAL::getDeviceModel() const {
    return deviceModel_;
}

std::string WebPlatformPAL::getOSVersion() const {
    return osVersion_;
}

std::string WebPlatformPAL::getLocale() const {
    return locale_;
}

long long WebPlatformPAL::getCurrentTimeMs() const {
    return getCurrentTimeMs();
}

void WebPlatformPAL::setCanvasId(const std::string& canvasId) {
    canvasId_ = canvasId;
}

void WebPlatformPAL::setDocumentTitle(const std::string& title) {
    emscripten_run_script(("document.title = '" + title + "';").c_str());
}

void WebPlatformPAL::openURL(const std::string& url) {
    emscripten_run_script(("window.open('" + url + "', '_blank');").c_str());
}

void WebPlatformPAL::setStatusText(const std::string& text) {
    emscripten_run_script(("window.status = '" + text + "';").c_str());
}

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE WebPlatformPAL::getWebGLContext() const {
    return webGLContext_;
}

int WebPlatformPAL::getCanvasWidth() const {
    int width = 0;
    emscripten_run_script_int("return Module.canvas ? Module.canvas.width : 0;", &width);
    return width;
}

int WebPlatformPAL::getCanvasHeight() const {
    int height = 0;
    emscripten_run_script_int("return Module.canvas ? Module.canvas.height : 0;", &height);
    return height;
}

float WebPlatformPAL::getDevicePixelRatio() const {
    return emscripten_get_device_pixel_ratio();
}

void WebPlatformPAL::enablePointerLock() {
    emscripten_request_pointerlock(canvasId_.c_str(), false);
}

void WebPlatformPAL::disablePointerLock() {
    emscripten_exit_pointerlock();
}

bool WebPlatformPAL::isPointerLocked() const {
    return pointerLocked_;
}

void WebPlatformPAL::requestFullscreen() {
    EmscriptenFullscreenStrategy strategy = {
        .scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
        .canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF,
        .filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT
    };
    emscripten_request_fullscreen_strategy(canvasId_.c_str(), true, &strategy);
}

void WebPlatformPAL::exitFullscreen() {
    emscripten_exit_fullscreen();
}

bool WebPlatformPAL::isFullscreen() const {
    return fullscreen_;
}

void WebPlatformPAL::initializeCapabilities() {
    capabilities_.type = PlatformType::WEB;
    capabilities_.name = "Web";
    capabilities_.version = "1.0.0";
    capabilities_.architecture = "JavaScript";

    // Web capabilities
    capabilities_.supportsWebGL = true;
    capabilities_.supportsSpatialAudio = true;
    capabilities_.supportsLowLatencyAudio = false;
    capabilities_.supportsTouch = true;
    capabilities_.supportsKeyboard = true;
    capabilities_.supportsMouse = true;
    capabilities_.supportsGamepad = true;
    capabilities_.supportsWebRTC = true;
    capabilities_.supportsWebSocket = true;
    capabilities_.supportsCloudSave = true;
    capabilities_.supportsPushNotifications = true;
    capabilities_.supportsGestureRecognition = true;
    capabilities_.supportsAccessibility = true;

    // Web limitations
    capabilities_.supportsVulkan = false;
    capabilities_.supportsDirectX = false;
    capabilities_.supportsMetal = false;
    capabilities_.supportsOpenGL = false;
    capabilities_.supportsOpenGL_ES = false;
    capabilities_.supportsIAP = false;
    capabilities_.supportsAchievements = false;
    capabilities_.supportsLeaderboards = false;
    capabilities_.supportsThermalManagement = false;
    capabilities_.supportsBackgroundTasks = false;

    // Web performance characteristics
    capabilities_.maxTextureSize = 4096;
    capabilities_.maxRenderTargets = 8;
    capabilities_.maxComputeUnits = 256;
    capabilities_.maxMemoryMB = 4096;
    capabilities_.maxThreadCount = 4;
    capabilities_.maxDisplayWidth = 3840;
    capabilities_.maxDisplayHeight = 2160;
    capabilities_.maxRefreshRate = 60;
    capabilities_.supportsHDR = false;
    capabilities_.supportsMultipleDisplays = false;
}

void WebPlatformPAL::initializeGraphicsContext() {
    graphicsContext_ = new WebGraphicsContext(this);
    if (!graphicsContext_->initialize(config_)) {
        // Handle initialization failure
    }
}

void WebPlatformPAL::initializeAudioContext() {
    audioContext_ = new WebAudioContext(this);
    if (!audioContext_->initialize(config_)) {
        // Handle initialization failure
    }
}

void WebPlatformPAL::initializeInputContext() {
    inputContext_ = new WebInputContext(this);
    if (!inputContext_->initialize()) {
        // Handle initialization failure
    }
}

void WebPlatformPAL::initializeNetworkContext() {
    networkContext_ = new WebNetworkContext(this);
    if (!networkContext_->initialize()) {
        // Handle initialization failure
    }
}

void WebPlatformPAL::initializeStorageContext() {
    storageContext_ = new WebStorageContext(this);
    if (!storageContext_->initialize(config_)) {
        // Handle initialization failure
    }
}

void WebPlatformPAL::initializePlatformServices() {
    platformServices_ = new WebPlatformServices(this);
    if (!platformServices_->initialize()) {
        // Handle initialization failure
    }
}

void WebPlatformPAL::initializeWindowManager() {
    windowManager_ = new WebWindowManager(this);
    if (!windowManager_->initialize(config_)) {
        // Handle initialization failure
    }
}

void WebPlatformPAL::initializeEventSystem() {
    eventSystem_ = new WebEventSystem(this);
    if (!eventSystem_->initialize()) {
        // Handle initialization failure
    }
}

void WebPlatformPAL::querySystemInformation() {
    // Query Web system information
    deviceId_ = getWebDeviceId();
    deviceModel_ = getWebDeviceModel();
    osVersion_ = getWebOSVersion();
    locale_ = getWebLocale();

    // Query memory information
    queryMemoryInformation();

    // Query display information
    queryDisplayInformation();
}

void WebPlatformPAL::queryMemoryInformation() {
    // Web memory information
    totalMemory_ = 1024 * 1024 * 1024; // 1GB default
    availableMemory_ = 512 * 1024 * 1024; // 512MB default
    memoryUsage_ = 0.5f; // 50% default
}

void WebPlatformPAL::queryDisplayInformation() {
    // Web display information is handled by the browser
}

std::string WebPlatformPAL::getWebDeviceId() const {
    char buffer[256] = {0};
    emscripten_run_script_string("try { return navigator.userAgent; } catch(e) { return 'Unknown'; }", buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string WebPlatformPAL::getWebDeviceModel() const {
    char buffer[256] = {0};
    emscripten_run_script_string("try { return navigator.platform; } catch(e) { return 'Web'; }", buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string WebPlatformPAL::getWebOSVersion() const {
    char buffer[256] = {0};
    emscripten_run_script_string("try { return navigator.userAgent; } catch(e) { return 'Web/1.0'; }", buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string WebPlatformPAL::getWebLocale() const {
    char buffer[256] = {0};
    emscripten_run_script_string("try { return navigator.language; } catch(e) { return 'en-US'; }", buffer, sizeof(buffer));
    return std::string(buffer);
}

void WebPlatformPAL::updateMemoryStats() {
    // Update Web memory statistics
    memoryUsage_ = 0.5f; // Placeholder
}

void WebPlatformPAL::updateCPUStats() {
    // Update Web CPU statistics
    cpuUsage_ = 0.3f; // Placeholder
}

void WebPlatformPAL::setWebOrientation(int orientation) {
    // Web orientation handling
    switch (orientation) {
        case 0: // Portrait
            emscripten_run_script("screen.orientation.lock('portrait');");
            break;
        case 1: // Landscape
            emscripten_run_script("screen.orientation.lock('landscape');");
            break;
        default:
            emscripten_run_script("screen.orientation.unlock();");
            break;
    }
}

void WebPlatformPAL::setWebFullscreen(bool fullscreen) {
    if (fullscreen) {
        requestFullscreen();
    } else {
        exitFullscreen();
    }
}

void WebPlatformPAL::performWebVibration(int durationMs) {
    std::string script = "try { navigator.vibrate(" + std::to_string(durationMs) + "); } catch(e) {}";
    emscripten_run_script(script.c_str());
}

void WebPlatformPAL::showWebToast(const std::string& message) {
    std::string script = "try { console.log('Toast: " + message + "'); } catch(e) {}";
    emscripten_run_script(script.c_str());
}

void WebPlatformPAL::onAnimationFrame(double time) {
    // Animation frame callback
}

void WebPlatformPAL::onVisibilityChange(int visibilityState) {
    // Visibility change callback
}

void WebPlatformPAL::onFullscreenChange(int isFullscreen) {
    // Fullscreen change callback
}

void WebPlatformPAL::onPointerLockChange(int isLocked) {
    // Pointer lock change callback
}

void WebPlatformPAL::onContextLost() {
    // WebGL context lost callback
}

void WebPlatformPAL::onContextRestored() {
    // WebGL context restored callback
}

void WebPlatformPAL::startMainLoop() {
    // Start Emscripten main loop
    emscripten_set_main_loop([]() {
        // Main loop implementation
    }, 0, 1);
}

void WebPlatformPAL::stopMainLoop() {
    // Stop Emscripten main loop
    emscripten_cancel_main_loop();
}

void WebPlatformPAL::mainLoop() {
    // Main loop implementation
}

// ========== WEB GRAPHICS CONTEXT ==========
WebGraphicsContext::WebGraphicsContext(WebPlatformPAL* platform)
    : GraphicsContext(), platform_(platform), webGLContext_(0), webGL2Supported_(false) {
}

WebGraphicsContext::~WebGraphicsContext() {
    shutdown();
}

bool WebGraphicsContext::initialize(const PlatformConfig& config) {
    if (!initializeWebGL()) {
        return false;
    }
    return true;
}

void WebGraphicsContext::shutdown() {
    if (webGLContext_) {
        emscripten_webgl_destroy_context(webGLContext_);
        webGLContext_ = 0;
    }
}

void WebGraphicsContext::swapBuffers() {
    // WebGL swap buffers is automatic
}

void WebGraphicsContext::makeCurrent() {
    emscripten_webgl_make_context_current(webGLContext_);
}

void WebGraphicsContext::setSwapInterval(int interval) {
    // WebGL doesn't support swap interval
}

GraphicsAPI WebGraphicsContext::getGraphicsAPI() const {
    return GraphicsAPI::WEBGL;
}

int WebGraphicsContext::getMajorVersion() const {
    return webGL2Supported_ ? 2 : 1;
}

int WebGraphicsContext::getMinorVersion() const {
    return 0;
}

std::string WebGraphicsContext::getVendor() const {
    char buffer[256] = {0};
    emscripten_webgl_get_parameter_v(webGLContext_, GL_VENDOR, buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string WebGraphicsContext::getRenderer() const {
    char buffer[256] = {0};
    emscripten_webgl_get_parameter_v(webGLContext_, GL_RENDERER, buffer, sizeof(buffer));
    return std::string(buffer);
}

int WebGraphicsContext::getFramebufferWidth() const {
    int width = 0;
    emscripten_webgl_get_parameter_i(webGLContext_, GL_VIEWPORT, &width);
    return width;
}

int WebGraphicsContext::getFramebufferHeight() const {
    int height = 0;
    emscripten_webgl_get_parameter_i(webGLContext_, GL_VIEWPORT, &height);
    return height;
}

float WebGraphicsContext::getDisplayScale() const {
    return emscripten_get_device_pixel_ratio();
}

void* WebGraphicsContext::getNativeContext() const {
    return nullptr;
}

void* WebGraphicsContext::getNativeDisplay() const {
    return nullptr;
}

WebGLRenderingContext* WebGraphicsContext::getWebGL1Context() const {
    return webGL2Supported_ ? nullptr : reinterpret_cast<WebGLRenderingContext*>(webGLContext_);
}

WebGL2RenderingContext* WebGraphicsContext::getWebGL2Context() const {
    return webGL2Supported_ ? reinterpret_cast<WebGL2RenderingContext*>(webGLContext_) : nullptr;
}

bool WebGraphicsContext::isWebGL2Supported() const {
    return webGL2Supported_;
}

void WebGraphicsContext::setContextAttributes(const EmscriptenWebGLContextAttributes& attrs) {
    // Set context attributes
}

bool WebGraphicsContext::initializeWebGL() {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);

    attrs.alpha = true;
    attrs.depth = true;
    attrs.stencil = true;
    attrs.antialias = true;
    attrs.premultipliedAlpha = true;
    attrs.preserveDrawingBuffer = false;
    attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    attrs.failIfMajorPerformanceCaveat = false;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.enableExtensionsByDefault = true;

    webGLContext_ = emscripten_webgl_create_context("canvas", &attrs);
    if (webGLContext_) {
        webGL2Supported_ = true;
    } else {
        // Fallback to WebGL 1.0
        attrs.majorVersion = 1;
        attrs.minorVersion = 0;
        webGLContext_ = emscripten_webgl_create_context("canvas", &attrs);
        webGL2Supported_ = false;
    }

    if (!webGLContext_) {
        return false;
    }

    makeCurrent();
    setupWebGLContext();
    return true;
}

void WebGraphicsContext::setupWebGLContext() {
    // Setup WebGL context
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void WebGraphicsContext::handleContextLoss() {
    // Handle WebGL context loss
}

void WebGraphicsContext::handleContextRestore() {
    // Handle WebGL context restore
    setupWebGLContext();
}

// ========== WEB AUDIO CONTEXT ==========
WebAudioContext::WebAudioContext(WebPlatformPAL* platform)
    : AudioContext(), platform_(platform), sampleRate_(44100), channels_(2), bufferSize_(512), masterVolume_(1.0f) {
}

WebAudioContext::~WebAudioContext() {
    shutdown();
}

bool WebAudioContext::initialize(const PlatformConfig& config) {
    if (!initializeWebAudio()) {
        return false;
    }
    return true;
}

void WebAudioContext::shutdown() {
    // Shutdown Web Audio
}

void WebAudioContext::suspend() {
    // Suspend Web Audio
}

void WebAudioContext::resume() {
    // Resume Web Audio
}

AudioAPI WebAudioContext::getAudioAPI() const {
    return AudioAPI::WEB_AUDIO;
}

int WebAudioContext::getSampleRate() const {
    return sampleRate_;
}

int WebAudioContext::getChannels() const {
    return channels_;
}

int WebAudioContext::getBufferSize() const {
    return bufferSize_;
}

float WebAudioContext::getMasterVolume() const {
    return masterVolume_;
}

void WebAudioContext::setMasterVolume(float volume) {
    masterVolume_ = std::max(0.0f, std::min(1.0f, volume));
}

void* WebAudioContext::getNativeContext() const {
    return nullptr;
}

void WebAudioContext::createAudioWorklet(const std::string& workletName, const std::string& workletCode) {
    // Create Web Audio worklet
}

void WebAudioContext::loadAudioBuffer(const std::string& url, const std::string& bufferName) {
    // Load audio buffer from URL
}

void WebAudioContext::playBuffer(const std::string& bufferName, bool loop) {
    // Play audio buffer
}

void WebAudioContext::stopBuffer(const std::string& bufferName) {
    // Stop audio buffer
}

void WebAudioContext::setBufferVolume(const std::string& bufferName, float volume) {
    // Set buffer volume
}

bool WebAudioContext::initializeWebAudio() {
    // Initialize Web Audio API
    return true;
}

void WebAudioContext::shutdownWebAudio() {
    // Shutdown Web Audio API
}

void WebAudioContext::createAudioContext() {
    // Create Web Audio context
}

void WebAudioContext::setupAudioNodes() {
    // Setup Web Audio nodes
}

// ========== WEB INPUT CONTEXT ==========
WebInputContext::WebInputContext(WebPlatformPAL* platform)
    : InputContext(), platform_(platform), mouseX_(0), mouseY_(0), cursorVisible_(true), pointerLocked_(false) {
}

WebInputContext::~WebInputContext() {
    shutdown();
}

bool WebInputContext::initialize() {
    // Initialize Web input
    return true;
}

void WebInputContext::shutdown() {
    // Shutdown Web input
}

void WebInputContext::update() {
    updateKeyboardState();
    updateMouseState();
    updateTouchState();
    updateGamepadState();
}

bool WebInputContext::isKeyPressed(int keyCode) const {
    auto it = keyStates_.find(keyCode);
    return it != keyStates_.end() && it->second;
}

bool WebInputContext::isMouseButtonPressed(int button) const {
    auto it = mouseStates_.find(button);
    return it != mouseStates_.end() && it->second;
}

void WebInputContext::getMousePosition(float& x, float& y) const {
    x = mouseX_;
    y = mouseY_;
}

void WebInputContext::getTouchPosition(int touchId, float& x, float& y) const {
    auto it = touchPositions_.find(touchId);
    if (it != touchPositions_.end()) {
        x = it->second.first;
        y = it->second.second;
    } else {
        x = y = 0.0f;
    }
}

int WebInputContext::getTouchCount() const {
    return touchPositions_.size();
}

bool WebInputContext::isTouchSupported() const {
    return true;
}

bool WebInputContext::isGamepadSupported() const {
    return true;
}

int WebInputContext::getGamepadCount() const {
    return 0; // Placeholder
}

void WebInputContext::setMousePosition(float x, float y) {
    mouseX_ = x;
    mouseY_ = y;
}

void WebInputContext::showCursor(bool show) {
    cursorVisible_ = show;
}

void WebInputContext::captureCursor(bool capture) {
    if (capture) {
        platform_->enablePointerLock();
    } else {
        platform_->disablePointerLock();
    }
}

void WebInputContext::handleKeyboardEvent(int keyCode, bool pressed, bool repeat) {
    keyStates_[keyCode] = pressed;
}

void WebInputContext::handleMouseEvent(int button, bool pressed, float x, float y) {
    mouseStates_[button] = pressed;
    mouseX_ = x;
    mouseY_ = y;
}

void WebInputContext::handleTouchEvent(int touchId, float x, float y, bool pressed) {
    if (pressed) {
        touchPositions_[touchId] = std::make_pair(x, y);
    } else {
        touchPositions_.erase(touchId);
    }
}

void WebInputContext::handleWheelEvent(float deltaX, float deltaY) {
    wheelX_ = deltaX;
    wheelY_ = deltaY;
}

void WebInputContext::handleGamepadConnected(int gamepadId) {
    // Handle gamepad connected
}

void WebInputContext::handleGamepadDisconnected(int gamepadId) {
    // Handle gamepad disconnected
}

void WebInputContext::updateKeyboardState() {
    // Update keyboard state
}

void WebInputContext::updateMouseState() {
    // Update mouse state
}

void WebInputContext::updateTouchState() {
    // Update touch state
}

void WebInputContext::updateGamepadState() {
    // Update gamepad state
}

// ========== WEB NETWORK CONTEXT ==========
WebNetworkContext::WebNetworkContext(WebPlatformPAL* platform)
    : NetworkContext(), platform_(platform), networking_(nullptr), connection_(nullptr), connected_(false), signalStrength_(100) {
}

WebNetworkContext::~WebNetworkContext() {
    shutdown();
}

bool WebNetworkContext::initialize() {
    // Create UDP networking instance for Web platform
    networking_ = FoundryEngine::createUDPNetworking();
    if (!networking_) {
        return false;
    }

    if (!networking_->initialize()) {
        return false;
    }

    // Initialize WebRTC for peer-to-peer support
    if (!initializeWebRTC()) {
        return false;
    }

    return true;
}

void WebNetworkContext::shutdown() {
    // Disconnect any active connections
    disconnect();

    // Shutdown UDP networking
    if (networking_) {
        networking_->shutdown();
        FoundryEngine::destroyUDPNetworking(networking_);
        networking_ = nullptr;
    }

    // Shutdown WebRTC
    shutdownWebRTC();
}

void WebNetworkContext::update() {
    // Update UDP networking
    if (networking_) {
        networking_->update(0.016f); // 60 FPS
    }

    // Update network status
    updateNetworkStatus();
}

NetworkAPI WebNetworkContext::getNetworkAPI() const {
    return NetworkAPI::UDP; // Primary API is UDP
}

bool WebNetworkContext::isNetworkAvailable() const {
    return true; // Web always has network access
}

std::string WebNetworkContext::getNetworkType() const {
    return currentNetworkType_;
}

int WebNetworkContext::getSignalStrength() const {
    return signalStrength_;
}

bool WebNetworkContext::connect(const std::string& host, int port) {
    if (!networking_) return false;

    // Create UDP connection
    connection_ = networking_->createConnection();
    if (!connection_) {
        return false;
    }

    // Set up connection callbacks
    connection_->setConnectCallback([this]() {
        connected_ = true;
        onConnectionEstablished();
    });

    connection_->setDisconnectCallback([this]() {
        connected_ = false;
        onConnectionLost();
    });

    connection_->setPacketCallback([this](const FoundryEngine::UDPPacket& packet) {
        onPacketReceived(packet);
    });

    connection_->setErrorCallback([this](const std::string& error) {
        onNetworkError(error);
    });

    // Attempt connection
    if (!connection_->connect(host, static_cast<uint16_t>(port))) {
        return false;
    }

    return true;
}

void WebNetworkContext::disconnect() {
    if (connection_) {
        connection_->disconnect();
        connection_.reset();
        connected_ = false;
    }
}

bool WebNetworkContext::isConnected() const {
    return connected_;
}

int WebNetworkContext::send(const void* data, size_t size) {
    if (!connection_ || !connected_) {
        return -1;
    }

    // Create UDP packet
    FoundryEngine::UDPPacket packet;
    packet.type = FoundryEngine::UDPPacketType::Custom;
    packet.timestamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    // Copy data
    packet.payload.assign(static_cast<const uint8_t*>(data),
                         static_cast<const uint8_t*>(data) + size);

    // Send unreliably by default (can be made reliable by caller)
    return connection_->sendPacket(packet, false) ? size : -1;
}

int WebNetworkContext::receive(void* buffer, size_t size) {
    // UDP networking handles packets asynchronously via callbacks
    // This method is not used in the UDP networking model
    return 0;
}

void* WebNetworkContext::getNativeSocket() const {
    return nullptr;
}

void WebNetworkContext::createPeerConnection() {
    // Create WebRTC peer connection for P2P
}

void WebNetworkContext::addIceCandidate(const std::string& candidate) {
    // Add ICE candidate for WebRTC
}

void WebNetworkContext::createDataChannel(const std::string& channelName) {
    // Create WebRTC data channel
}

void WebNetworkContext::sendDataChannelMessage(const std::string& channelName, const std::string& message) {
    // Send WebRTC data channel message
}

void WebNetworkContext::closePeerConnection() {
    // Close WebRTC peer connection
}

bool WebNetworkContext::initializeWebRTC() {
    // Initialize WebRTC for P2P support
    return true;
}

void WebNetworkContext::shutdownWebRTC() {
    // Shutdown WebRTC
}

void WebNetworkContext::updateNetworkStatus() {
    // Update network status and signal strength
    currentNetworkType_ = "WebRTC/UDP";

    // Web has good connectivity by default
    signalStrength_ = 100;
}

void WebNetworkContext::handleConnectionStateChange() {
    // Handle WebRTC connection state change
}

void WebNetworkContext::handleIceGatheringStateChange() {
    // Handle ICE gathering state change
}

void WebNetworkContext::handleSignalingStateChange() {
    // Handle signaling state change
}

// UDP Networking callbacks
void WebNetworkContext::onConnectionEstablished() {
    connected_ = true;
    // Notify platform about connection
    platform_->sendEvent(PlatformEvent{PlatformEventType::NETWORK_CONNECTED,
                                      std::chrono::steady_clock::now(), {}, nullptr});
}

void WebNetworkContext::onConnectionLost() {
    connected_ = false;
    // Notify platform about disconnection
    platform_->sendEvent(PlatformEvent{PlatformEventType::NETWORK_DISCONNECTED,
                                      std::chrono::steady_clock::now(), {}, nullptr});
}

void WebNetworkContext::onPacketReceived(const FoundryEngine::UDPPacket& packet) {
    // Handle incoming UDP packets
    // This is where game-specific packet handling would occur
    switch (packet.type) {
    case FoundryEngine::UDPPacketType::PlayerState:
        // Handle player state update
        break;
    case FoundryEngine::UDPPacketType::WorldState:
        // Handle world state update
        break;
    case FoundryEngine::UDPPacketType::Chat:
        // Handle chat message
        break;
    default:
        // Handle custom packets
        break;
    }
}

void WebNetworkContext::onNetworkError(const std::string& error) {
    // Handle network errors
    platform_->sendEvent(PlatformEvent{PlatformEventType::NETWORK_ERROR,
                                      std::chrono::steady_clock::now(), {}, nullptr});
}

// ========== WEB STORAGE CONTEXT ==========
WebStorageContext::WebStorageContext(WebPlatformPAL* platform)
    : StorageContext(), platform_(platform), persistentStorage_(false), storageQuota_(0) {
}

WebStorageContext::~WebStorageContext() {
    shutdown();
}

bool WebStorageContext::initialize(const PlatformConfig& config) {
    if (!initializeWebStorage()) {
        return false;
    }
    return true;
}

void WebStorageContext::shutdown() {
    shutdownWebStorage();
}

StorageAPI WebStorageContext::getStorageAPI() const {
    return StorageAPI::WEB_STORAGE;
}

std::string WebStorageContext::getBasePath() const {
    return "/";
}

std::string WebStorageContext::getDocumentsPath() const {
    return "/persistent";
}

std::string WebStorageContext::getCachePath() const {
    return "/session";
}

std::string WebStorageContext::getTempPath() const {
    return "/temp";
}

bool WebStorageContext::fileExists(const std::string& path) const {
    // Check if file exists in Web storage
    return false;
}

size_t WebStorageContext::getFileSize(const std::string& path) const {
    // Get file size from Web storage
    return 0;
}

bool WebStorageContext::readFile(const std::string& path, std::vector<uint8_t>& data) const {
    // Read file from Web storage
    return false;
}

bool WebStorageContext::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    // Write file to Web storage
    return false;
}

bool WebStorageContext::deleteFile(const std::string& path) {
    // Delete file from Web storage
    return false;
}

bool WebStorageContext::createDirectory(const std::string& path) {
    // Create directory in Web storage
    return false;
}

bool WebStorageContext::deleteDirectory(const std::string& path) {
    // Delete directory from Web storage
    return false;
}

std::vector<std::string> WebStorageContext::listDirectory(const std::string& path) const {
    // List directory contents in Web storage
    return {};
}

bool WebStorageContext::isWritable(const std::string& path) const {
    return true;
}

bool WebStorageContext::isReadable(const std::string& path) const {
    return true;
}

uint64_t WebStorageContext::getFreeSpace(const std::string& path) const {
    return 0;
}

uint64_t WebStorageContext::getTotalSpace(const std::string& path) const {
    return 0;
}

void WebStorageContext::setPersistentStorage(bool persistent) {
    persistentStorage_ = persistent;
}

void WebStorageContext::requestStorageQuota(int64_t bytes) {
    storageQuota_ = bytes;
}

void WebStorageContext::clearStorage() {
    // Clear Web storage
}

void WebStorageContext::syncStorage() {
    // Sync Web storage
}

bool WebStorageContext::initializeWebStorage() {
    // Initialize Web storage
    return true;
}

void WebStorageContext::shutdownWebStorage() {
    // Shutdown Web storage
}

std::string WebStorageContext::resolveStoragePath(const std::string& path) const {
    return path;
}

bool WebStorageContext::checkStorageQuota(int64_t requiredBytes) const {
    return true;
}

// ========== WEB PLATFORM SERVICES ==========
WebPlatformServices::WebPlatformServices(WebPlatformPAL* platform)
    : PlatformServices(), platform_(platform) {
}

WebPlatformServices::~WebPlatformServices() {
    shutdown();
}

bool WebPlatformServices::initialize() {
    if (!initializeIndexedDB()) {
        return false;
    }
    if (!initializeServiceWorker()) {
        return false;
    }
    return true;
}

void WebPlatformServices::shutdown() {
    shutdownIndexedDB();
    shutdownServiceWorker();
}

bool WebPlatformServices::isIAPSupported() const {
    return false;
}

bool WebPlatformServices::purchaseProduct(const std::string& productId) {
    return false;
}

bool WebPlatformServices::restorePurchases() {
    return false;
}

std::vector<std::string> WebPlatformServices::getProducts() const {
    return {};
}

bool WebPlatformServices::isAchievementsSupported() const {
    return false;
}

bool WebPlatformServices::unlockAchievement(const std::string& achievementId) {
    return false;
}

bool WebPlatformServices::incrementAchievement(const std::string& achievementId, int increment) {
    return false;
}

std::vector<std::string> WebPlatformServices::getUnlockedAchievements() const {
    return {};
}

bool WebPlatformServices::isLeaderboardsSupported() const {
    return false;
}

bool WebPlatformServices::submitScore(const std::string& leaderboardId, int score) {
    return false;
}

bool WebPlatformServices::showLeaderboard(const std::string& leaderboardId) {
    return false;
}

std::vector<std::pair<std::string, int>> WebPlatformServices::getLeaderboardScores(const std::string& leaderboardId) const {
    return {};
}

bool WebPlatformServices::isCloudSaveSupported() const {
    return true;
}

bool WebPlatformServices::saveToCloud(const std::string& key, const std::vector<uint8_t>& data) {
    // Save to IndexedDB
    return true;
}

bool WebPlatformServices::loadFromCloud(const std::string& key, std::vector<uint8_t>& data) {
    // Load from IndexedDB
    return false;
}

bool WebPlatformServices::deleteFromCloud(const std::string& key) {
    // Delete from IndexedDB
    return false;
}

bool WebPlatformServices::isPushNotificationsSupported() const {
    return true;
}

bool WebPlatformServices::registerForPushNotifications() {
    // Register for push notifications
    return true;
}

bool WebPlatformServices::unregisterForPushNotifications() {
    // Unregister for push notifications
    return false;
}

void WebPlatformServices::scheduleNotification(const std::string& title, const std::string& message, int delaySeconds) {
    // Schedule notification
}

bool WebPlatformServices::initializeIndexedDB() {
    // Initialize IndexedDB
    return true;
}

void WebPlatformServices::shutdownIndexedDB() {
    // Shutdown IndexedDB
}

bool WebPlatformServices::initializeServiceWorker() {
    // Initialize Service Worker
    return true;
}

void WebPlatformServices::shutdownServiceWorker() {
    // Shutdown Service Worker
}

// ========== WEB WINDOW MANAGER ==========
WebWindowManager::WebWindowManager(WebPlatformPAL* platform)
    : WindowManager(), platform_(platform), width_(800), height_(600), scale_(1.0f),
      fullscreen_(false), minimized_(false), maximized_(false), visible_(true), focused_(true), resizable_(true) {
}

WebWindowManager::~WebWindowManager() {
    shutdown();
}

bool WebWindowManager::initialize(const PlatformConfig& config) {
    width_ = config.windowWidth;
    height_ = config.windowHeight;
    fullscreen_ = config.fullscreen;
    resizable_ = config.resizable;
    return true;
}

void WebWindowManager::shutdown() {
    // Shutdown window manager
}

void WebWindowManager::update() {
    updateCanvasSize();
    updateViewport();
}

void* WebWindowManager::getNativeWindow() const {
    return nullptr;
}

int WebWindowManager::getWidth() const {
    return width_;
}

int WebWindowManager::getHeight() const {
    return height_;
}

float WebWindowManager::getScale() const {
    return scale_;
}

void WebWindowManager::setTitle(const std::string& title) {
    platform_->setDocumentTitle(title);
}

void WebWindowManager::setSize(int width, int height) {
    width_ = width;
    height_ = height;
    updateCanvasSize();
}

void WebWindowManager::setPosition(int x, int y) {
    // Web windows don't have position
}

void WebWindowManager::setFullscreen(bool fullscreen) {
    fullscreen_ = fullscreen;
    platform_->setFullscreen(fullscreen);
}

void WebWindowManager::setResizable(bool resizable) {
    resizable_ = resizable;
}

void WebWindowManager::setVSync(bool vsync) {
    // Web doesn't support VSync control
}

bool WebWindowManager::isFullscreen() const {
    return fullscreen_;
}

bool WebWindowManager::isMinimized() const {
    return minimized_;
}

bool WebWindowManager::isMaximized() const {
    return maximized_;
}

bool WebWindowManager::isVisible() const {
    return visible_;
}

bool WebWindowManager::isFocused() const {
    return focused_;
}

void WebWindowManager::show() {
    visible_ = true;
}

void WebWindowManager::hide() {
    visible_ = false;
}

void WebWindowManager::minimize() {
    minimized_ = true;
}

void WebWindowManager::maximize() {
    maximized_ = true;
}

void WebWindowManager::restore() {
    minimized_ = false;
    maximized_ = false;
}

void WebWindowManager::focus() {
    focused_ = true;
}

void WebWindowManager::setCanvasSize(int width, int height) {
    width_ = width;
    height_ = height;
    updateCanvasSize();
}

void WebWindowManager::setViewportSize(int width, int height) {
    width_ = width;
    height_ = height;
    updateViewport();
}

void WebWindowManager::handleResize(int width, int height) {
    width_ = width;
    height_ = height;
    updateCanvasSize();
    updateViewport();
}

void WebWindowManager::handleOrientationChange() {
    // Handle orientation change
}

void WebWindowManager::updateCanvasSize() {
    // Update canvas size
}

void WebWindowManager::updateViewport() {
    // Update viewport
}

void WebWindowManager::handleFullscreenChange() {
    // Handle fullscreen change
}

// ========== WEB EVENT SYSTEM ==========
WebEventSystem::WebEventSystem(WebPlatformPAL* platform)
    : EventSystem(), platform_(platform) {
}

WebEventSystem::~WebEventSystem() {
    shutdown();
}

bool WebEventSystem::initialize() {
    // Initialize Web event system
    return true;
}

void WebEventSystem::shutdown() {
    // Shutdown Web event system
}

void WebEventSystem::update() {
    processEvents();
}

void WebEventSystem::registerCallback(PlatformEventType type, PlatformEventCallback callback) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    callbacks_[type].push_back(callback);
}

void WebEventSystem::unregisterCallback(PlatformEventType type, PlatformEventCallback callback) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    auto& callbacks = callbacks_[type];
    callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback), callbacks.end());
}

void WebEventSystem::sendEvent(const PlatformEvent& event) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    auto it = callbacks_.find(event.type);
    if (it != callbacks_.end()) {
        for (const auto& callback : it->second) {
            callback(event);
        }
    }
}

void WebEventSystem::processEvents() {
    processWebEvents();
    handleWebInputEvents();
    handleWebSystemEvents();
}

bool WebEventSystem::hasPendingEvents() const {
    std::lock_guard<std::mutex> lock(eventMutex_);
    return !eventQueue_.empty();
}

void WebEventSystem::flushEvents() {
    std::lock_guard<std::mutex> lock(eventMutex_);
    while (!eventQueue_.empty()) {
        eventQueue_.pop();
    }
}

void WebEventSystem::enableEventType(PlatformEventType type) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    enabledEvents_[type] = true;
}

void WebEventSystem::disableEventType(PlatformEventType type) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    enabledEvents_[type] = false;
}

bool WebEventSystem::isEventTypeEnabled(PlatformEventType type) const {
    std::lock_guard<std::mutex> lock(eventMutex_);
    auto it = enabledEvents_.find(type);
    return it != enabledEvents_.end() && it->second;
}

void WebEventSystem::handleKeyEvent(int keyCode, bool pressed, bool repeat) {
    // Handle key event
}

void WebEventSystem::handleMouseEvent(int button, bool pressed, float x, float y) {
    // Handle mouse event
}

void WebEventSystem::handleTouchEvent(int touchId, float x, float y, bool pressed) {
    // Handle touch event
}

void WebEventSystem::handleWheelEvent(float deltaX, float deltaY) {
    // Handle wheel event
}

void WebEventSystem::handleFocusEvent(bool focused) {
    // Handle focus event
}

void WebEventSystem::handleVisibilityEvent(bool visible) {
    // Handle visibility event
}

void WebEventSystem::handleFullscreenEvent(bool fullscreen) {
    // Handle fullscreen event
}

void WebEventSystem::queueEvent(const PlatformEvent& event) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    eventQueue_.push(event);
}

void WebEventSystem::dispatchEvent(const PlatformEvent& event) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    auto it = callbacks_.find(event.type);
    if (it != callbacks_.end()) {
        for (const auto& callback : it->second) {
            callback(event);
        }
    }
}

void WebEventSystem::processWebEvents() {
    // Process Web events
}

void WebEventSystem::handleWebInputEvents() {
    // Handle Web input events
}

void WebEventSystem::handleWebSystemEvents() {
    // Handle Web system events
}

// ========== EMSCRIPTEN CALLBACKS ==========
extern "C" {

void onAnimationFrame(double time) {
    // Animation frame callback
}

EM_BOOL onKeyEvent(int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData) {
    // Key event callback
    return EM_TRUE;
}

EM_BOOL onMouseEvent(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData) {
    // Mouse event callback
    return EM_TRUE;
}

EM_BOOL onTouchEvent(int eventType, const EmscriptenTouchEvent* touchEvent, void* userData) {
    // Touch event callback
    return EM_TRUE;
}

EM_BOOL onWheelEvent(int eventType, const EmscriptenWheelEvent* wheelEvent, void* userData) {
    // Wheel event callback
    return EM_TRUE;
}

EM_BOOL onFocusEvent(int eventType, const EmscriptenFocusEvent* focusEvent, void* userData) {
    // Focus event callback
    return EM_TRUE;
}

EM_BOOL onResizeEvent(int eventType, const EmscriptenUiEvent* uiEvent, void* userData) {
    // Resize event callback
    return EM_TRUE;
}

EM_BOOL onFullscreenChange(int eventType, const EmscriptenFullscreenChangeEvent* fullscreenEvent, void* userData) {
    // Fullscreen change callback
    return EM_TRUE;
}

void onWebRTCDataChannelOpen(int eventType, const EmscriptenWebRTCDataChannelEvent* event, void* userData) {
    // WebRTC data channel open callback
}

void onWebRTCDataChannelClose(int eventType, const EmscriptenWebRTCDataChannelEvent* event, void* userData) {
    // WebRTC data channel close callback
}

void onWebRTCDataChannelMessage(int eventType, const EmscriptenWebRTCDataChannelEvent* event, void* userData) {
    // WebRTC data channel message callback
}

void onWebRTCIceCandidate(int eventType, const EmscriptenWebRTCIceCandidateEvent* event, void* userData) {
    // WebRTC ICE candidate callback
}

void onWebRTCConnectionStateChange(int eventType, const EmscriptenWebRTCConnectionStateChangeEvent* event, void* userData) {
    // WebRTC connection state change callback
}

void onWebAudioLoad(const char* bufferName) {
    // Web Audio load callback
}

void onWebAudioError(const char* bufferName, const char* error) {
    // Web Audio error callback
}

void onStorageQuotaExceeded(const char* storageType) {
    // Storage quota exceeded callback
}

void onStorageReady(const char* storageType) {
    // Storage ready callback
}

void onServiceWorkerMessage(int eventType, const EmscriptenServiceWorkerMessageEvent* event, void* userData) {
    // Service worker message callback
}

void onPushNotification(const char* title, const char* message) {
    // Push notification callback
}

}

} // namespace FoundryEngine
