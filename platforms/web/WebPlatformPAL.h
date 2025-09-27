#ifndef FOUNDRYENGINE_WEB_PLATFORM_PAL_H
#define FOUNDRYENGINE_WEB_PLATFORM_PAL_H

#include "../../include/GameEngine/platform/PlatformInterface.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <webgl/webgl1.h>
#include <webgl/webgl2.h>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace FoundryEngine {

// Forward declarations
class WebGraphicsContext;
class WebAudioContext;
class WebInputContext;
class WebNetworkContext;
class WebStorageContext;
class WebPlatformServices;
class WebWindowManager;
class WebEventSystem;

// ========== WEB PLATFORM PAL IMPLEMENTATION ==========
class WebPlatformPAL : public PlatformInterface {
public:
    WebPlatformPAL();
    ~WebPlatformPAL() override;

    // PlatformInterface implementation
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    PlatformType getPlatformType() const override { return PlatformType::WEB; }
    PlatformCapabilities getCapabilities() const override { return capabilities_; }
    PlatformConfig getConfig() const override { return config_; }
    std::string getPlatformName() const override { return "Web"; }
    std::string getPlatformVersion() const override;

    // Context access
    GraphicsContext* getGraphicsContext() const override { return graphicsContext_; }
    AudioContext* getAudioContext() const override { return audioContext_; }
    InputContext* getInputContext() const override { return inputContext_; }
    NetworkContext* getNetworkContext() const override { return networkContext_; }
    StorageContext* getStorageContext() const override { return storageContext_; }

    // Service access
    PlatformServices* getPlatformServices() const override { return platformServices_; }
    WindowManager* getWindowManager() const override { return windowManager_; }
    EventSystem* getEventSystem() const override { return eventSystem_; }

    // Lifecycle management
    void onAppStart() override;
    void onAppPause() override;
    void onAppResume() override;
    void onAppTerminate() override;
    void onAppBackground() override;
    void onAppForeground() override;

    // Event handling
    void registerEventCallback(PlatformEventType type, PlatformEventCallback callback) override;
    void unregisterEventCallback(PlatformEventType type, PlatformEventCallback callback) override;
    void sendEvent(const PlatformEvent& event) override;

    // Platform-specific features
    void setOrientation(int orientation) override;
    void setFullscreen(bool fullscreen) override;
    void setKeepScreenOn(bool keepOn) override;
    void vibrate(int durationMs) override;
    void showToast(const std::string& message) override;

    // Memory management
    size_t getTotalMemory() const override;
    size_t getAvailableMemory() const override;
    size_t getUsedMemory() const override;
    void garbageCollect() override;

    // Performance monitoring
    float getCPUUsage() const override;
    float getMemoryUsage() const override;
    float getBatteryLevel() const override;
    bool isBatteryCharging() const override;

    // Platform utilities
    std::string getDeviceId() const override;
    std::string getDeviceModel() const override;
    std::string getOSVersion() const override;
    std::string getLocale() const override;
    long long getCurrentTimeMs() const override;

    // Platform-specific extensions
    void* getNativeHandle() const override { return nullptr; }
    void* getNativeDisplay() const override { return nullptr; }
    void* getNativeWindow() const override { return nullptr; }

    // Web-specific methods
    void setCanvasId(const std::string& canvasId);
    void setDocumentTitle(const std::string& title);
    void openURL(const std::string& url);
    void setStatusText(const std::string& text);

    // Web API access
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE getWebGLContext() const;
    int getCanvasWidth() const;
    int getCanvasHeight() const;
    float getDevicePixelRatio() const;

    // Web-specific features
    void enablePointerLock();
    void disablePointerLock();
    bool isPointerLocked() const;
    void requestFullscreen();
    void exitFullscreen();
    bool isFullscreen() const;

private:
    // Web-specific members
    std::string canvasId_;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webGLContext_;
    bool pointerLocked_;
    bool fullscreen_;

    // Context implementations
    WebGraphicsContext* graphicsContext_;
    WebAudioContext* audioContext_;
    WebInputContext* inputContext_;
    WebNetworkContext* networkContext_;
    WebStorageContext* storageContext_;

    // Service implementations
    WebPlatformServices* platformServices_;
    WebWindowManager* windowManager_;
    WebEventSystem* eventSystem_;

    // Platform state
    std::atomic<bool> initialized_;
    std::atomic<bool> appActive_;
    std::atomic<bool> windowFocused_;

    // Threading
    std::thread mainLoopThread_;
    std::mutex platformMutex_;

    // Web system information
    std::string deviceId_;
    std::string deviceModel_;
    std::string osVersion_;
    std::string locale_;
    std::string userAgent_;

    // Performance monitoring
    mutable float cpuUsage_;
    mutable float memoryUsage_;
    mutable size_t totalMemory_;
    mutable size_t availableMemory_;

    // Initialize platform capabilities
    void initializeCapabilities();

    // Initialize Web contexts
    void initializeGraphicsContext();
    void initializeAudioContext();
    void initializeInputContext();
    void initializeNetworkContext();
    void initializeStorageContext();

    // Initialize Web services
    void initializePlatformServices();
    void initializeWindowManager();
    void initializeEventSystem();

    // Web system queries
    void querySystemInformation();
    void queryMemoryInformation();
    void queryDisplayInformation();

    // Web-specific utilities
    std::string getWebDeviceId() const;
    std::string getWebDeviceModel() const;
    std::string getWebOSVersion() const;
    std::string getWebLocale() const;

    // Memory management
    void updateMemoryStats();
    void updateCPUStats();

    // Web-specific features
    void setWebOrientation(int orientation);
    void setWebFullscreen(bool fullscreen);
    void performWebVibration(int durationMs);
    void showWebToast(const std::string& message);

    // Emscripten callbacks
    static void onAnimationFrame(double time);
    static void onVisibilityChange(int visibilityState);
    static void onFullscreenChange(int isFullscreen);
    static void onPointerLockChange(int isLocked);
    static void onContextLost();
    static void onContextRestored();

    // Thread management
    void startMainLoop();
    void stopMainLoop();
    void mainLoop();
};

// ========== WEB GRAPHICS CONTEXT ==========
class WebGraphicsContext : public GraphicsContext {
public:
    WebGraphicsContext(WebPlatformPAL* platform);
    ~WebGraphicsContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void swapBuffers() override;
    void makeCurrent() override;
    void setSwapInterval(int interval) override;

    GraphicsAPI getGraphicsAPI() const override { return GraphicsAPI::WEBGL; }
    int getMajorVersion() const override { return 2; }
    int getMinorVersion() const override { return 0; }
    std::string getVendor() const override;
    std::string getRenderer() const override;

    int getFramebufferWidth() const override;
    int getFramebufferHeight() const override;
    float getDisplayScale() const override;

    void* getNativeContext() const override { return nullptr; }
    void* getNativeDisplay() const override { return nullptr; }

    // WebGL-specific methods
    WebGLRenderingContext* getWebGL1Context() const;
    WebGL2RenderingContext* getWebGL2Context() const;
    bool isWebGL2Supported() const;
    void setContextAttributes(const EmscriptenWebGLContextAttributes& attrs);

private:
    WebPlatformPAL* platform_;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webGLContext_;
    EmscriptenWebGLContextAttributes contextAttributes_;
    bool webGL2Supported_;

    bool initializeWebGL();
    void setupWebGLContext();
    void handleContextLoss();
    void handleContextRestore();
};

// ========== WEB AUDIO CONTEXT ==========
class WebAudioContext : public AudioContext {
public:
    WebAudioContext(WebPlatformPAL* platform);
    ~WebAudioContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void suspend() override;
    void resume() override;

    AudioAPI getAudioAPI() const override { return AudioAPI::WEB_AUDIO; }
    int getSampleRate() const override { return sampleRate_; }
    int getChannels() const override { return channels_; }
    int getBufferSize() const override { return bufferSize_; }

    float getMasterVolume() const override { return masterVolume_; }
    void setMasterVolume(float volume) override;

    void* getNativeContext() const override { return nullptr; }

    // Web Audio-specific methods
    void createAudioWorklet(const std::string& workletName, const std::string& workletCode);
    void loadAudioBuffer(const std::string& url, const std::string& bufferName);
    void playBuffer(const std::string& bufferName, bool loop = false);
    void stopBuffer(const std::string& bufferName);
    void setBufferVolume(const std::string& bufferName, float volume);

private:
    WebPlatformPAL* platform_;
    int sampleRate_;
    int channels_;
    int bufferSize_;
    float masterVolume_;

    bool initializeWebAudio();
    void shutdownWebAudio();
    void createAudioContext();
    void setupAudioNodes();
};

// ========== WEB INPUT CONTEXT ==========
class WebInputContext : public InputContext {
public:
    WebInputContext(WebPlatformPAL* platform);
    ~WebInputContext() override;

    bool initialize() override;
    void shutdown() override;
    void update() override;

    bool isKeyPressed(int keyCode) const override;
    bool isMouseButtonPressed(int button) const override;
    void getMousePosition(float& x, float& y) const override;
    void getTouchPosition(int touchId, float& x, float& y) const override;

    int getTouchCount() const override;
    bool isTouchSupported() const override { return true; }
    bool isGamepadSupported() const override { return true; }
    int getGamepadCount() const override;

    void setMousePosition(float x, float y) override;
    void showCursor(bool show) override;
    void captureCursor(bool capture) override;

    // Web-specific input methods
    void handleKeyboardEvent(int keyCode, bool pressed, bool repeat);
    void handleMouseEvent(int button, bool pressed, float x, float y);
    void handleTouchEvent(int touchId, float x, float y, bool pressed);
    void handleWheelEvent(float deltaX, float deltaY);
    void handleGamepadConnected(int gamepadId);
    void handleGamepadDisconnected(int gamepadId);

private:
    WebPlatformPAL* platform_;
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> mouseStates_;
    std::unordered_map<int, std::pair<float, float>> touchPositions_;
    std::unordered_map<int, bool> gamepadStates_;
    float mouseX_, mouseY_;
    float wheelX_, wheelY_;
    bool cursorVisible_;
    bool pointerLocked_;

    void updateKeyboardState();
    void updateMouseState();
    void updateTouchState();
    void updateGamepadState();
};

// ========== WEB NETWORK CONTEXT ==========
class WebNetworkContext : public NetworkContext {
public:
    WebNetworkContext(WebPlatformPAL* platform);
    ~WebNetworkContext() override;

    bool initialize() override;
    void shutdown() override;
    void update() override;

    NetworkAPI getNetworkAPI() const override { return NetworkAPI::WEBRTC; }
    bool isNetworkAvailable() const override;
    std::string getNetworkType() const override;
    int getSignalStrength() const override;

    bool connect(const std::string& host, int port) override;
    void disconnect() override;
    bool isConnected() const override;

    int send(const void* data, size_t size) override;
    int receive(void* buffer, size_t size) override;

    void* getNativeSocket() const override { return nullptr; }

    // WebRTC-specific methods
    void createPeerConnection();
    void addIceCandidate(const std::string& candidate);
    void createDataChannel(const std::string& channelName);
    void sendDataChannelMessage(const std::string& channelName, const std::string& message);
    void closePeerConnection();

private:
    WebPlatformPAL* platform_;
    bool connected_;
    std::string currentNetworkType_;
    int signalStrength_;

    bool initializeWebRTC();
    void shutdownWebRTC();
    void updateNetworkStatus();
    void handleConnectionStateChange();
    void handleIceGatheringStateChange();
    void handleSignalingStateChange();
};

// ========== WEB STORAGE CONTEXT ==========
class WebStorageContext : public StorageContext {
public:
    WebStorageContext(WebPlatformPAL* platform);
    ~WebStorageContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;

    StorageAPI getStorageAPI() const override { return StorageAPI::WEB_STORAGE; }
    std::string getBasePath() const override { return "/"; }
    std::string getDocumentsPath() const override { return "/persistent"; }
    std::string getCachePath() const override { return "/session"; }
    std::string getTempPath() const override { return "/temp"; }

    bool fileExists(const std::string& path) const override;
    size_t getFileSize(const std::string& path) const override;
    bool readFile(const std::string& path, std::vector<uint8_t>& data) const override;
    bool writeFile(const std::string& path, const std::vector<uint8_t>& data) override;
    bool deleteFile(const std::string& path) override;

    bool createDirectory(const std::string& path) override;
    bool deleteDirectory(const std::string& path) override;
    std::vector<std::string> listDirectory(const std::string& path) const override;

    bool isWritable(const std::string& path) const override;
    bool isReadable(const std::string& path) const override;
    uint64_t getFreeSpace(const std::string& path) const override;
    uint64_t getTotalSpace(const std::string& path) const override;

    // Web Storage-specific methods
    void setPersistentStorage(bool persistent);
    void requestStorageQuota(int64_t bytes);
    void clearStorage();
    void syncStorage();

private:
    WebPlatformPAL* platform_;
    bool persistentStorage_;
    int64_t storageQuota_;

    bool initializeWebStorage();
    void shutdownWebStorage();
    std::string resolveStoragePath(const std::string& path) const;
    bool checkStorageQuota(int64_t requiredBytes) const;
};

// ========== WEB PLATFORM SERVICES ==========
class WebPlatformServices : public PlatformServices {
public:
    WebPlatformServices(WebPlatformPAL* platform);
    ~WebPlatformServices() override;

    bool initialize() override;
    void shutdown() override;

    // In-App Purchase (not supported on web)
    bool isIAPSupported() const override { return false; }
    bool purchaseProduct(const std::string& productId) override { return false; }
    bool restorePurchases() override { return false; }
    std::vector<std::string> getProducts() const override { return {}; }

    // Achievements (not supported on web)
    bool isAchievementsSupported() const override { return false; }
    bool unlockAchievement(const std::string& achievementId) override { return false; }
    bool incrementAchievement(const std::string& achievementId, int increment) override { return false; }
    std::vector<std::string> getUnlockedAchievements() const override { return {}; }

    // Leaderboards (not supported on web)
    bool isLeaderboardsSupported() const override { return false; }
    bool submitScore(const std::string& leaderboardId, int score) override { return false; }
    bool showLeaderboard(const std::string& leaderboardId) override { return false; }
    std::vector<std::pair<std::string, int>> getLeaderboardScores(const std::string& leaderboardId) const override { return {}; }

    // Cloud Save (using IndexedDB)
    bool isCloudSaveSupported() const override { return true; }
    bool saveToCloud(const std::string& key, const std::vector<uint8_t>& data) override;
    bool loadFromCloud(const std::string& key, std::vector<uint8_t>& data) override;
    bool deleteFromCloud(const std::string& key) override;

    // Push Notifications (using Service Workers)
    bool isPushNotificationsSupported() const override { return true; }
    bool registerForPushNotifications() override;
    bool unregisterForPushNotifications() override;
    void scheduleNotification(const std::string& title, const std::string& message, int delaySeconds) override;

private:
    WebPlatformPAL* platform_;

    bool initializeIndexedDB();
    void shutdownIndexedDB();
    bool initializeServiceWorker();
    void shutdownServiceWorker();
};

// ========== WEB WINDOW MANAGER ==========
class WebWindowManager : public WindowManager {
public:
    WebWindowManager(WebPlatformPAL* platform);
    ~WebWindowManager() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void update() override;

    void* getNativeWindow() const override { return nullptr; }
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    float getScale() const override { return scale_; }

    void setTitle(const std::string& title) override;
    void setSize(int width, int height) override;
    void setPosition(int x, int y) override;
    void setFullscreen(bool fullscreen) override;
    void setResizable(bool resizable) override;
    void setVSync(bool vsync) override;

    bool isFullscreen() const override { return fullscreen_; }
    bool isMinimized() const override { return false; } // Web doesn't minimize
    bool isMaximized() const override { return false; } // Web doesn't maximize
    bool isVisible() const override { return visible_; }
    bool isFocused() const override { return focused_; }

    void show() override;
    void hide() override;
    void minimize() override;
    void maximize() override;
    void restore() override;
    void focus() override;

    // Web-specific window methods
    void setCanvasSize(int width, int height);
    void setViewportSize(int width, int height);
    void handleResize(int width, int height);
    void handleOrientationChange();

private:
    WebPlatformPAL* platform_;
    int width_;
    int height_;
    float scale_;
    bool fullscreen_;
    bool visible_;
    bool focused_;

    void updateCanvasSize();
    void updateViewport();
    void handleFullscreenChange();
};

// ========== WEB EVENT SYSTEM ==========
class WebEventSystem : public EventSystem {
public:
    WebEventSystem(WebPlatformPAL* platform);
    ~WebEventSystem() override;

    bool initialize() override;
    void shutdown() override;
    void update() override;

    void registerCallback(PlatformEventType type, PlatformEventCallback callback) override;
    void unregisterCallback(PlatformEventType type, PlatformEventCallback callback) override;
    void sendEvent(const PlatformEvent& event) override;

    void processEvents() override;
    bool hasPendingEvents() const override;
    void flushEvents() override;

    void enableEventType(PlatformEventType type) override;
    void disableEventType(PlatformEventType type) override;
    bool isEventTypeEnabled(PlatformEventType type) const override;

    // Web-specific event methods
    void handleKeyEvent(int keyCode, bool pressed, bool repeat);
    void handleMouseEvent(int button, bool pressed, float x, float y);
    void handleTouchEvent(int touchId, float x, float y, bool pressed);
    void handleWheelEvent(float deltaX, float deltaY);
    void handleFocusEvent(bool focused);
    void handleVisibilityEvent(bool visible);
    void handleFullscreenEvent(bool fullscreen);

private:
    WebPlatformPAL* platform_;
    std::unordered_map<PlatformEventType, std::vector<PlatformEventCallback>> callbacks_;
    std::queue<PlatformEvent> eventQueue_;
    std::unordered_map<PlatformEventType, bool> enabledEvents_;
    std::mutex eventMutex_;

    void queueEvent(const PlatformEvent& event);
    void dispatchEvent(const PlatformEvent& event);
    void processWebEvents();
    void handleWebInputEvents();
    void handleWebSystemEvents();
};

// ========== EMSCRIPTEN CALLBACKS ==========
extern "C" {

    // Animation frame callback
    void onAnimationFrame(double time);

    // Event callbacks
    EM_BOOL onKeyEvent(int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData);
    EM_BOOL onMouseEvent(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData);
    EM_BOOL onTouchEvent(int eventType, const EmscriptenTouchEvent* touchEvent, void* userData);
    EM_BOOL onWheelEvent(int eventType, const EmscriptenWheelEvent* wheelEvent, void* userData);
    EM_BOOL onFocusEvent(int eventType, const EmscriptenFocusEvent* focusEvent, void* userData);
    EM_BOOL onResizeEvent(int eventType, const EmscriptenUiEvent* uiEvent, void* userData);
    EM_BOOL onFullscreenChange(int eventType, const EmscriptenFullscreenChangeEvent* fullscreenEvent, void* userData);

    // WebRTC callbacks
    void onWebRTCDataChannelOpen(int eventType, const EmscriptenWebRTCDataChannelEvent* event, void* userData);
    void onWebRTCDataChannelClose(int eventType, const EmscriptenWebRTCDataChannelEvent* event, void* userData);
    void onWebRTCDataChannelMessage(int eventType, const EmscriptenWebRTCDataChannelEvent* event, void* userData);
    void onWebRTCIceCandidate(int eventType, const EmscriptenWebRTCIceCandidateEvent* event, void* userData);
    void onWebRTCConnectionStateChange(int eventType, const EmscriptenWebRTCConnectionStateChangeEvent* event, void* userData);

    // Web Audio callbacks
    void onWebAudioLoad(const char* bufferName);
    void onWebAudioError(const char* bufferName, const char* error);

    // Web Storage callbacks
    void onStorageQuotaExceeded(const char* storageType);
    void onStorageReady(const char* storageType);

    // Web Service Worker callbacks
    void onServiceWorkerMessage(int eventType, const EmscriptenServiceWorkerMessageEvent* event, void* userData);
    void onPushNotification(const char* title, const char* message);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_WEB_PLATFORM_PAL_H
