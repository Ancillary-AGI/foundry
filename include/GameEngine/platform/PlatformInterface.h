#ifndef FOUNDRYENGINE_PLATFORM_INTERFACE_H
#define FOUNDRYENGINE_PLATFORM_INTERFACE_H

#include "../core/System.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>

namespace FoundryEngine {

// Forward declarations
class GraphicsContext;
class AudioContext;
class InputContext;
class NetworkContext;
class StorageContext;
class PlatformServices;
class WindowManager;
class EventSystem;

// Platform types
enum class PlatformType {
    UNKNOWN,
    ANDROID,
    WINDOWS,
    MACOS,
    IOS,
    LINUX,
    WEB,
    CONSOLE
};

// Platform capabilities
struct PlatformCapabilities {
    PlatformType type;
    std::string name;
    std::string version;
    std::string architecture;

    // Hardware capabilities
    bool supportsVulkan;
    bool supportsDirectX;
    bool supportsMetal;
    bool supportsOpenGL;
    bool supportsWebGL;

    // Audio capabilities
    bool supportsSpatialAudio;
    bool supportsLowLatencyAudio;
    bool supportsHardwareAcceleration;

    // Input capabilities
    bool supportsTouch;
    bool supportsStylus;
    bool supportsGamepad;
    bool supportsKeyboard;
    bool supportsMouse;

    // Network capabilities
    bool supportsWebRTC;
    bool supportsWebSocket;
    bool supportsQUIC;

    // Storage capabilities
    bool supportsCloudSave;
    bool supportsExternalStorage;
    bool supportsEncryptedStorage;

    // Platform services
    bool supportsIAP;
    bool supportsAchievements;
    bool supportsLeaderboards;
    bool supportsPushNotifications;

    // Advanced features
    bool supportsThermalManagement;
    bool supportsBackgroundTasks;
    bool supportsGestureRecognition;
    bool supportsAccessibility;

    // Performance features
    int maxTextureSize;
    int maxRenderTargets;
    int maxComputeUnits;
    int maxMemoryMB;
    int maxThreadCount;

    // Display capabilities
    int maxDisplayWidth;
    int maxDisplayHeight;
    int maxRefreshRate;
    bool supportsHDR;
    bool supportsMultipleDisplays;
};

// Platform configuration
struct PlatformConfig {
    std::string appName;
    std::string appVersion;
    std::string bundleId;
    std::string dataPath;
    std::string cachePath;
    std::string tempPath;

    // Window configuration
    int windowWidth;
    int windowHeight;
    bool fullscreen;
    bool resizable;
    bool vsync;

    // Graphics configuration
    int graphicsAPI; // 0=Vulkan, 1=DirectX, 2=Metal, 3=OpenGL
    int msaaSamples;
    bool enableHDR;
    bool enableRayTracing;

    // Audio configuration
    int audioSampleRate;
    int audioChannels;
    int audioBufferSize;
    bool enableSpatialAudio;

    // Performance configuration
    int targetFPS;
    int maxFrameTime;
    bool enableOptimizations;
    bool enableMultithreading;

    // Platform-specific settings
    std::unordered_map<std::string, std::string> platformSettings;
};

// Platform initialization parameters
struct PlatformInitParams {
    PlatformConfig config;
    /**
     * @brief Platform-specific window handle.
     * @note void* is used for cross-platform abstraction. Prefer type-safe wrappers if possible.
     */
    void* nativeWindowHandle;
    /**
     * @brief Platform-specific app handle.
     * @note void* is used for cross-platform abstraction. Prefer type-safe wrappers if possible.
     */
    void* nativeAppHandle;
    std::unordered_map<std::string, std::string> customParams;
};

// Platform event types
enum class PlatformEventType {
    WINDOW_CREATED,
    WINDOW_DESTROYED,
    WINDOW_RESIZED,
    WINDOW_FOCUSED,
    WINDOW_MINIMIZED,
    WINDOW_MAXIMIZED,
    WINDOW_RESTORED,

    APP_STARTED,
    APP_PAUSED,
    APP_RESUMED,
    APP_TERMINATED,
    APP_BACKGROUND,
    APP_FOREGROUND,

    DEVICE_ORIENTATION_CHANGED,
    DEVICE_LOW_MEMORY,
    DEVICE_THERMAL_WARNING,
    DEVICE_BATTERY_LOW,
    DEVICE_STORAGE_LOW,

    NETWORK_CONNECTED,
    NETWORK_DISCONNECTED,
    NETWORK_TYPE_CHANGED,

    INPUT_DEVICE_ADDED,
    INPUT_DEVICE_REMOVED,
    INPUT_GAMEPAD_CONNECTED,
    INPUT_GAMEPAD_DISCONNECTED,

    CUSTOM_EVENT
};

// Platform event data
struct PlatformEvent {
    PlatformEventType type;
    std::chrono::steady_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> data;
    /**
     * @brief Platform-specific event data.
     * @note void* is used for cross-platform abstraction. Prefer type-safe wrappers if possible.
     */
    void* platformData;
};

// Platform event callback
using PlatformEventCallback = std::function<void(const PlatformEvent&)>;

// Graphics API enumeration
enum class GraphicsAPI {
    VULKAN,
    DIRECTX12,
    DIRECTX11,
    METAL,
    OPENGL,
    OPENGL_ES,
    WEBGL,
    CUSTOM
};

// Audio API enumeration
enum class AudioAPI {
    AAUDIO,
    OPENSL_ES,
    COREAUDIO,
    WASAPI,
    ALSA,
    WEB_AUDIO,
    CUSTOM
};

// Network API enumeration
enum class NetworkAPI {
    NATIVE_SOCKETS,
    WINSOCK,
    POSIX_SOCKETS,
    WEBRTC,
    WEBSOCKET,
    CUSTOM
};

// Storage API enumeration
enum class StorageAPI {
    NATIVE_FILESYSTEM,
    ANDROID_STORAGE,
    WINDOWS_STORAGE,
    POSIX_FILESYSTEM,
    WEB_STORAGE,
    CUSTOM
};

// ========== PLATFORM INTERFACE ==========
// Base class for all platform implementations
class PlatformInterface : public System {
public:
    PlatformInterface();
    virtual ~PlatformInterface();

    // System interface
    virtual void initialize() override = 0;
    virtual void update(float dt) override = 0;
    virtual void shutdown() override = 0;

    // Platform information
    virtual PlatformType getPlatformType() const = 0;
    virtual PlatformCapabilities getCapabilities() const = 0;
    virtual PlatformConfig getConfig() const = 0;
    virtual std::string getPlatformName() const = 0;
    virtual std::string getPlatformVersion() const = 0;

    // Context management
    virtual GraphicsContext* getGraphicsContext() const = 0;
    virtual AudioContext* getAudioContext() const = 0;
    virtual InputContext* getInputContext() const = 0;
    virtual NetworkContext* getNetworkContext() const = 0;
    virtual StorageContext* getStorageContext() const = 0;

    // Platform services
    virtual PlatformServices* getPlatformServices() const = 0;
    virtual WindowManager* getWindowManager() const = 0;
    virtual EventSystem* getEventSystem() const = 0;

    // Lifecycle management
    virtual void onAppStart() = 0;
    virtual void onAppPause() = 0;
    virtual void onAppResume() = 0;
    virtual void onAppTerminate() = 0;
    virtual void onAppBackground() = 0;
    virtual void onAppForeground() = 0;

    // Event handling
    virtual void registerEventCallback(PlatformEventType type, PlatformEventCallback callback) = 0;
    virtual void unregisterEventCallback(PlatformEventType type, PlatformEventCallback callback) = 0;
    virtual void sendEvent(const PlatformEvent& event) = 0;

    // Platform-specific features
    virtual void setOrientation(int orientation) = 0;
    virtual void setFullscreen(bool fullscreen) = 0;
    virtual void setKeepScreenOn(bool keepOn) = 0;
    virtual void vibrate(int durationMs) = 0;
    virtual void showToast(const std::string& message) = 0;

    // Memory management
    virtual size_t getTotalMemory() const = 0;
    virtual size_t getAvailableMemory() const = 0;
    virtual size_t getUsedMemory() const = 0;
    virtual void garbageCollect() = 0;

    // Performance monitoring
    virtual float getCPUUsage() const = 0;
    virtual float getMemoryUsage() const = 0;
    virtual float getBatteryLevel() const = 0;
    virtual bool isBatteryCharging() const = 0;

    // Platform utilities
    virtual std::string getDeviceId() const = 0;
    virtual std::string getDeviceModel() const = 0;
    virtual std::string getOSVersion() const = 0;
    virtual std::string getLocale() const = 0;
    virtual long long getCurrentTimeMs() const = 0;

    // Platform-specific extensions
    virtual void* getNativeHandle() const = 0;
    virtual void* getNativeDisplay() const = 0;
    virtual void* getNativeWindow() const = 0;

protected:
    PlatformType platformType_;
    PlatformCapabilities capabilities_;
    PlatformConfig config_;
    PlatformInitParams initParams_;

    // Context pointers (to be implemented by derived classes)
    GraphicsContext* graphicsContext_;
    AudioContext* audioContext_;
    InputContext* inputContext_;
    NetworkContext* networkContext_;
    StorageContext* storageContext_;

    // Service pointers
    PlatformServices* platformServices_;
    WindowManager* windowManager_;
    EventSystem* eventSystem_;

    // Event callbacks
    std::unordered_map<PlatformEventType, std::vector<PlatformEventCallback>> eventCallbacks_;
};

// ========== GRAPHICS CONTEXT INTERFACE ==========
class GraphicsContext {
public:
    virtual ~GraphicsContext() = default;

    virtual bool initialize(const PlatformConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual void swapBuffers() = 0;
    virtual void makeCurrent() = 0;
    virtual void setSwapInterval(int interval) = 0;

    virtual GraphicsAPI getGraphicsAPI() const = 0;
    virtual int getMajorVersion() const = 0;
    virtual int getMinorVersion() const = 0;
    virtual std::string getVendor() const = 0;
    virtual std::string getRenderer() const = 0;

    virtual int getFramebufferWidth() const = 0;
    virtual int getFramebufferHeight() const = 0;
    virtual float getDisplayScale() const = 0;

    virtual void* getNativeContext() const = 0;
    virtual void* getNativeDisplay() const = 0;
};

// ========== AUDIO CONTEXT INTERFACE ==========
class AudioContext {
public:
    virtual ~AudioContext() = default;

    virtual bool initialize(const PlatformConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;

    virtual AudioAPI getAudioAPI() const = 0;
    virtual int getSampleRate() const = 0;
    virtual int getChannels() const = 0;
    virtual int getBufferSize() const = 0;

    virtual float getMasterVolume() const = 0;
    virtual void setMasterVolume(float volume) = 0;

    virtual void* getNativeContext() const = 0;
};

// ========== INPUT CONTEXT INTERFACE ==========
class InputContext {
public:
    virtual ~InputContext() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

    virtual bool isKeyPressed(int keyCode) const = 0;
    virtual bool isMouseButtonPressed(int button) const = 0;
    virtual void getMousePosition(float& x, float& y) const = 0;
    virtual void getTouchPosition(int touchId, float& x, float& y) const = 0;

    virtual int getTouchCount() const = 0;
    virtual bool isTouchSupported() const = 0;
    virtual bool isGamepadSupported() const = 0;
    virtual int getGamepadCount() const = 0;

    virtual void setMousePosition(float x, float y) = 0;
    virtual void showCursor(bool show) = 0;
    virtual void captureCursor(bool capture) = 0;
};

// ========== NETWORK CONTEXT INTERFACE ==========
class NetworkContext {
public:
    virtual ~NetworkContext() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

    virtual NetworkAPI getNetworkAPI() const = 0;
    virtual bool isNetworkAvailable() const = 0;
    virtual std::string getNetworkType() const = 0;
    virtual int getSignalStrength() const = 0;

    virtual bool connect(const std::string& host, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual int send(const void* data, size_t size) = 0;
    virtual int receive(void* buffer, size_t size) = 0;

    virtual void* getNativeSocket() const = 0;
};

// ========== STORAGE CONTEXT INTERFACE ==========
class StorageContext {
public:
    virtual ~StorageContext() = default;

    virtual bool initialize(const PlatformConfig& config) = 0;
    virtual void shutdown() = 0;

    virtual StorageAPI getStorageAPI() const = 0;
    virtual std::string getBasePath() const = 0;
    virtual std::string getDocumentsPath() const = 0;
    virtual std::string getCachePath() const = 0;
    virtual std::string getTempPath() const = 0;

    virtual bool fileExists(const std::string& path) const = 0;
    virtual size_t getFileSize(const std::string& path) const = 0;
    virtual bool readFile(const std::string& path, std::vector<uint8_t>& data) const = 0;
    virtual bool writeFile(const std::string& path, const std::vector<uint8_t>& data) = 0;
    virtual bool deleteFile(const std::string& path) = 0;

    virtual bool createDirectory(const std::string& path) = 0;
    virtual bool deleteDirectory(const std::string& path) = 0;
    virtual std::vector<std::string> listDirectory(const std::string& path) const = 0;

    virtual bool isWritable(const std::string& path) const = 0;
    virtual bool isReadable(const std::string& path) const = 0;
    virtual uint64_t getFreeSpace(const std::string& path) const = 0;
    virtual uint64_t getTotalSpace(const std::string& path) const = 0;
};

// ========== PLATFORM SERVICES INTERFACE ==========
class PlatformServices {
public:
    virtual ~PlatformServices() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    // In-App Purchase
    virtual bool isIAPSupported() const = 0;
    virtual bool purchaseProduct(const std::string& productId) = 0;
    virtual bool restorePurchases() = 0;
    virtual std::vector<std::string> getProducts() const = 0;

    // Achievements
    virtual bool isAchievementsSupported() const = 0;
    virtual bool unlockAchievement(const std::string& achievementId) = 0;
    virtual bool incrementAchievement(const std::string& achievementId, int increment) = 0;
    virtual std::vector<std::string> getUnlockedAchievements() const = 0;

    // Leaderboards
    virtual bool isLeaderboardsSupported() const = 0;
    virtual bool submitScore(const std::string& leaderboardId, int score) = 0;
    virtual bool showLeaderboard(const std::string& leaderboardId) = 0;
    virtual std::vector<std::pair<std::string, int>> getLeaderboardScores(const std::string& leaderboardId) const = 0;

    // Cloud Save
    virtual bool isCloudSaveSupported() const = 0;
    virtual bool saveToCloud(const std::string& key, const std::vector<uint8_t>& data) = 0;
    virtual bool loadFromCloud(const std::string& key, std::vector<uint8_t>& data) = 0;
    virtual bool deleteFromCloud(const std::string& key) = 0;

    // Push Notifications
    virtual bool isPushNotificationsSupported() const = 0;
    virtual bool registerForPushNotifications() = 0;
    virtual bool unregisterForPushNotifications() = 0;
    virtual void scheduleNotification(const std::string& title, const std::string& message, int delaySeconds) = 0;
};

// ========== WINDOW MANAGER INTERFACE ==========
class WindowManager {
public:
    virtual ~WindowManager() = default;

    virtual bool initialize(const PlatformConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

    virtual void* getNativeWindow() const = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual float getScale() const = 0;

    virtual void setTitle(const std::string& title) = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void setPosition(int x, int y) = 0;
    virtual void setFullscreen(bool fullscreen) = 0;
    virtual void setResizable(bool resizable) = 0;
    virtual void setVSync(bool vsync) = 0;

    virtual bool isFullscreen() const = 0;
    virtual bool isMinimized() const = 0;
    virtual bool isMaximized() const = 0;
    virtual bool isVisible() const = 0;
    virtual bool isFocused() const = 0;

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void restore() = 0;
    virtual void focus() = 0;
};

// ========== EVENT SYSTEM INTERFACE ==========
class EventSystem {
public:
    virtual ~EventSystem() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

    virtual void registerCallback(PlatformEventType type, PlatformEventCallback callback) = 0;
    virtual void unregisterCallback(PlatformEventType type, PlatformEventCallback callback) = 0;
    virtual void sendEvent(const PlatformEvent& event) = 0;

    virtual void processEvents() = 0;
    virtual bool hasPendingEvents() const = 0;
    virtual void flushEvents() = 0;

    virtual void enableEventType(PlatformEventType type) = 0;
    virtual void disableEventType(PlatformEventType type) = 0;
    virtual bool isEventTypeEnabled(PlatformEventType type) const = 0;
};

// ========== PLATFORM FACTORY ==========
// Factory for creating platform instances
class PlatformFactory {
public:
    static std::unique_ptr<PlatformInterface> createPlatform(PlatformType type);
    static PlatformType detectPlatform();
    static std::vector<PlatformType> getSupportedPlatforms();
    static std::string getPlatformName(PlatformType type);
    static bool isPlatformSupported(PlatformType type);

private:
    static std::unordered_map<PlatformType, std::function<std::unique_ptr<PlatformInterface>()>> platformCreators_;
};

// ========== PLATFORM REGISTRY ==========
// Registry for platform implementations
class PlatformRegistry {
public:
    static void registerPlatform(PlatformType type, std::function<std::unique_ptr<PlatformInterface>()> creator);
    static void unregisterPlatform(PlatformType type);
    static bool isPlatformRegistered(PlatformType type);
    static std::unique_ptr<PlatformInterface> createPlatform(PlatformType type);

private:
    static std::unordered_map<PlatformType, std::function<std::unique_ptr<PlatformInterface>()>>& getRegistry();
};

// Platform registration helper macros
#define REGISTER_PLATFORM(PlatformClass, PlatformType) \
    static bool PlatformClass##Registered = []() { \
        PlatformRegistry::registerPlatform(PlatformType, []() -> std::unique_ptr<PlatformInterface> { \
            return std::make_unique<PlatformClass>(); \
        }); \
        return true; \
    }();

#define IMPLEMENT_PLATFORM_INTERFACE(PlatformClass) \
    class PlatformClass : public PlatformInterface { \
    public: \
        PlatformClass(); \
        ~PlatformClass() override; \
        \
        void initialize() override; \
        void update(float dt) override; \
        void shutdown() override; \
        \
        PlatformType getPlatformType() const override; \
        PlatformCapabilities getCapabilities() const override; \
        PlatformConfig getConfig() const override; \
        std::string getPlatformName() const override; \
        std::string getPlatformVersion() const override; \
        \
        GraphicsContext* getGraphicsContext() const override; \
        AudioContext* getAudioContext() const override; \
        InputContext* getInputContext() const override; \
        NetworkContext* getNetworkContext() const override; \
        StorageContext* getStorageContext() const override; \
        \
        PlatformServices* getPlatformServices() const override; \
        WindowManager* getWindowManager() const override; \
        EventSystem* getEventSystem() const override; \
        \
        void onAppStart() override; \
        void onAppPause() override; \
        void onAppResume() override; \
        void onAppTerminate() override; \
        void onAppBackground() override; \
        void onAppForeground() override; \
        \
        void registerEventCallback(PlatformEventType type, PlatformEventCallback callback) override; \
        void unregisterEventCallback(PlatformEventType type, PlatformEventCallback callback) override; \
        void sendEvent(const PlatformEvent& event) override; \
        \
        void setOrientation(int orientation) override; \
        void setFullscreen(bool fullscreen) override; \
        void setKeepScreenOn(bool keepOn) override; \
        void vibrate(int durationMs) override; \
        void showToast(const std::string& message) override; \
        \
        size_t getTotalMemory() const override; \
        size_t getAvailableMemory() const override; \
        size_t getUsedMemory() const override; \
        void garbageCollect() override; \
        \
        float getCPUUsage() const override; \
        float getMemoryUsage() const override; \
        float getBatteryLevel() const override; \
        bool isBatteryCharging() const override; \
        \
        std::string getDeviceId() const override; \
        std::string getDeviceModel() const override; \
        std::string getOSVersion() const override; \
        std::string getLocale() const override; \
        long long getCurrentTimeMs() const override; \
        \
        void* getNativeHandle() const override; \
        void* getNativeDisplay() const override; \
        void* getNativeWindow() const override; \
    };

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_PLATFORM_INTERFACE_H
