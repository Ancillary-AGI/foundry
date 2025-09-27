#ifndef FOUNDRYENGINE_MAC_IOS_PLATFORM_PAL_H
#define FOUNDRYENGINE_MAC_IOS_PLATFORM_PAL_H

#include "../../include/GameEngine/platform/PlatformInterface.h"
#include <TargetConditionals.h>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>

#if TARGET_OS_IPHONE
    #include <UIKit/UIKit.h>
    #include <Metal/Metal.h>
    #include <MetalKit/MetalKit.h>
    #include <AVFoundation/AVFoundation.h>
    #include <GameKit/GameKit.h>
    #include <CoreMotion/CoreMotion.h>
    #define PLATFORM_IOS 1
    #define PLATFORM_MACOS 0
#elif TARGET_OS_MAC
    #include <Cocoa/Cocoa.h>
    #include <Metal/Metal.h>
    #include <MetalKit/MetalKit.h>
    #include <AVFoundation/AVFoundation.h>
    #include <GameKit/GameKit.h>
    #include <CoreFoundation/CoreFoundation.h>
    #define PLATFORM_IOS 0
    #define PLATFORM_MACOS 1
#endif

namespace FoundryEngine {

// Forward declarations
class MaciOSGraphicsContext;
class MaciOSAudioContext;
class MaciOSInputContext;
class MaciOSNetworkContext;
class MaciOSStorageContext;
class MaciOSPlatformServices;
class MaciOSWindowManager;
class MaciOSEventSystem;

// ========== MAC/IOS PLATFORM PAL IMPLEMENTATION ==========
class MaciOSPlatformPAL : public PlatformInterface {
public:
    MaciOSPlatformPAL();
    ~MaciOSPlatformPAL() override;

    // PlatformInterface implementation
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    PlatformType getPlatformType() const override {
        return PLATFORM_IOS ? PlatformType::IOS : PlatformType::MACOS;
    }
    PlatformCapabilities getCapabilities() const override { return capabilities_; }
    PlatformConfig getConfig() const override { return config_; }
    std::string getPlatformName() const override {
        return PLATFORM_IOS ? "iOS" : "macOS";
    }
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
    void* getNativeHandle() const override { return PLATFORM_IOS ? (__bridge void*)viewController_ : (__bridge void*)window_; }
    void* getNativeDisplay() const override { return nullptr; }
    void* getNativeWindow() const override { return PLATFORM_IOS ? (__bridge void*)view_ : (__bridge void*)window_; }

    // macOS/iOS-specific methods
    #if PLATFORM_IOS
        void setViewController(UIViewController* viewController);
        void setView(UIView* view);
        void setMotionManager(CMMotionManager* motionManager);
        void setLocationManager(CLLocationManager* locationManager);
    #elif PLATFORM_MACOS
        void setWindow(NSWindow* window);
        void setView(NSView* view);
        void setDelegate(id<NSApplicationDelegate> delegate);
    #endif

    // Apple-specific API access
    #if PLATFORM_IOS
        UIViewController* getViewController() const { return viewController_; }
        UIView* getView() const { return view_; }
        CMMotionManager* getMotionManager() const { return motionManager_; }
        CLLocationManager* getLocationManager() const { return locationManager_; }
    #elif PLATFORM_MACOS
        NSWindow* getWindow() const { return window_; }
        NSView* getView() const { return view_; }
        id<NSApplicationDelegate> getDelegate() const { return delegate_; }
    #endif

    // Apple-specific features
    void enableHaptics();
    void disableHaptics();
    void setHapticFeedbackStyle(int style);
    void triggerHapticFeedback(int type);
    void enableCoreMotion();
    void disableCoreMotion();
    void enableLocationServices();
    void disableLocationServices();

private:
    // Apple-specific members
    #if PLATFORM_IOS
        UIViewController* viewController_;
        UIView* view_;
        CMMotionManager* motionManager_;
        CLLocationManager* locationManager_;
        UIImpactFeedbackGenerator* impactGenerator_;
        UINotificationFeedbackGenerator* notificationGenerator_;
        UISelectionFeedbackGenerator* selectionGenerator_;
    #elif PLATFORM_MACOS
        NSWindow* window_;
        NSView* view_;
        id<NSApplicationDelegate> delegate_;
        NSHapticFeedbackManager* hapticManager_;
    #endif

    // Context implementations
    MaciOSGraphicsContext* graphicsContext_;
    MaciOSAudioContext* audioContext_;
    MaciOSInputContext* inputContext_;
    MaciOSNetworkContext* networkContext_;
    MaciOSStorageContext* storageContext_;

    // Service implementations
    MaciOSPlatformServices* platformServices_;
    MaciOSWindowManager* windowManager_;
    MaciOSEventSystem* eventSystem_;

    // Platform state
    std::atomic<bool> initialized_;
    std::atomic<bool> appActive_;
    std::atomic<bool> windowFocused_;

    // Threading
    std::thread mainLoopThread_;
    std::mutex platformMutex_;

    // Apple system information
    std::string deviceId_;
    std::string deviceModel_;
    std::string osVersion_;
    std::string locale_;
    std::string systemVersion_;

    // Performance monitoring
    mutable float cpuUsage_;
    mutable float memoryUsage_;
    mutable size_t totalMemory_;
    mutable size_t availableMemory_;
    mutable float batteryLevel_;
    mutable bool batteryCharging_;

    // Initialize platform capabilities
    void initializeCapabilities();

    // Initialize Apple contexts
    void initializeGraphicsContext();
    void initializeAudioContext();
    void initializeInputContext();
    void initializeNetworkContext();
    void initializeStorageContext();

    // Initialize Apple services
    void initializePlatformServices();
    void initializeWindowManager();
    void initializeEventSystem();

    // Apple system queries
    void querySystemInformation();
    void queryMemoryInformation();
    void queryBatteryInformation();
    void queryDisplayInformation();

    // Apple-specific utilities
    std::string getAppleDeviceId() const;
    std::string getAppleDeviceModel() const;
    std::string getAppleOSVersion() const;
    std::string getAppleLocale() const;

    // Memory management
    void updateMemoryStats();
    void updateCPUStats();
    void updateBatteryStats();

    // Apple-specific features
    void setAppleOrientation(int orientation);
    void setAppleFullscreen(bool fullscreen);
    void setAppleKeepScreenOn(bool keepOn);
    void performAppleVibration(int durationMs);
    void showAppleToast(const std::string& message);

    // Apple event handling
    #if PLATFORM_IOS
        static void onAppDidBecomeActive();
        static void onAppWillResignActive();
        static void onAppDidEnterBackground();
        static void onAppWillEnterForeground();
        static void onAppWillTerminate();
        static void onDeviceOrientationDidChange();
    #elif PLATFORM_MACOS
        static void onApplicationDidBecomeActive();
        static void onApplicationWillResignActive();
        static void onApplicationDidHide();
        static void onApplicationDidUnhide();
        static void onApplicationWillTerminate();
        static void onWindowDidResize();
        static void onWindowDidMiniaturize();
        static void onWindowDidDeminiaturize();
    #endif

    // Thread management
    void startMainLoop();
    void stopMainLoop();
    void mainLoop();
};

// ========== MAC/IOS GRAPHICS CONTEXT ==========
class MaciOSGraphicsContext : public GraphicsContext {
public:
    MaciOSGraphicsContext(MaciOSPlatformPAL* platform);
    ~MaciOSGraphicsContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void swapBuffers() override;
    void makeCurrent() override;
    void setSwapInterval(int interval) override;

    GraphicsAPI getGraphicsAPI() const override { return GraphicsAPI::METAL; }
    int getMajorVersion() const override { return 3; }
    int getMinorVersion() const override { return 0; }
    std::string getVendor() const override;
    std::string getRenderer() const override;

    int getFramebufferWidth() const override;
    int getFramebufferHeight() const override;
    float getDisplayScale() const override;

    void* getNativeContext() const override { return device_; }
    void* getNativeDisplay() const override { return nullptr; }

    // Metal-specific methods
    id<MTLDevice> getMetalDevice() const { return device_; }
    id<MTLCommandQueue> getCommandQueue() const { return commandQueue_; }
    id<MTLCommandBuffer> getCurrentCommandBuffer() const { return commandBuffer_; }
    id<MTLRenderCommandEncoder> getCurrentRenderEncoder() const { return renderEncoder_; }

    MTLRenderPassDescriptor* getCurrentRenderPassDescriptor() const { return renderPassDescriptor_; }
    CAMetalLayer* getMetalLayer() const { return metalLayer_; }

    void createCommandBuffer();
    void createRenderEncoder(MTLRenderPassDescriptor* descriptor);
    void endEncoding();

private:
    MaciOSPlatformPAL* platform_;
    #if PLATFORM_IOS
        CAMetalLayer* metalLayer_;
    #elif PLATFORM_MACOS
        CAMetalLayer* metalLayer_;
    #endif

    // Metal objects
    id<MTLDevice> device_;
    id<MTLCommandQueue> commandQueue_;
    id<MTLCommandBuffer> commandBuffer_;
    id<MTLRenderCommandEncoder> renderEncoder_;
    MTLRenderPassDescriptor* renderPassDescriptor_;

    bool initializeMetal();
    void createMetalLayer();
    void createDevice();
    void createCommandQueue();
    void setupRenderPassDescriptor();
    void cleanupMetal();
};

// ========== MAC/IOS AUDIO CONTEXT ==========
class MaciOSAudioContext : public AudioContext {
public:
    MaciOSAudioContext(MaciOSPlatformPAL* platform);
    ~MaciOSAudioContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void suspend() override;
    void resume() override;

    AudioAPI getAudioAPI() const override { return AudioAPI::COREAUDIO; }
    int getSampleRate() const override { return sampleRate_; }
    int getChannels() const override { return channels_; }
    int getBufferSize() const override { return bufferSize_; }

    float getMasterVolume() const override { return masterVolume_; }
    void setMasterVolume(float volume) override;

    void* getNativeContext() const override { return audioEngine_; }

    // AVAudioEngine-specific methods
    AVAudioEngine* getAudioEngine() const { return audioEngine_; }
    AVAudioPlayerNode* getPlayerNode() const { return playerNode_; }
    AVAudioMixerNode* getMixerNode() const { return mixerNode_; }

    void loadAudioFile(const std::string& filePath, const std::string& bufferName);
    void playBuffer(const std::string& bufferName, bool loop = false);
    void stopBuffer(const std::string& bufferName);
    void setBufferVolume(const std::string& bufferName, float volume);

private:
    MaciOSPlatformPAL* platform_;
    int sampleRate_;
    int channels_;
    int bufferSize_;
    float masterVolume_;

    // AVAudioEngine objects
    AVAudioEngine* audioEngine_;
    AVAudioPlayerNode* playerNode_;
    AVAudioMixerNode* mixerNode_;
    AVAudioEnvironmentNode* environmentNode_;
    std::unordered_map<std::string, AVAudioPCMBuffer*> audioBuffers_;

    bool initializeAVAudioEngine();
    void shutdownAVAudioEngine();
    void createAudioNodes();
    void setupAudioProcessing();
    void loadAudioBufferFromFile(const std::string& filePath, AVAudioPCMBuffer** buffer);
};

// ========== MAC/IOS INPUT CONTEXT ==========
class MaciOSInputContext : public InputContext {
public:
    MaciOSInputContext(MaciOSPlatformPAL* platform);
    ~MaciOSInputContext() override;

    bool initialize() override;
    void shutdown() override;
    void update() override;

    bool isKeyPressed(int keyCode) const override;
    bool isMouseButtonPressed(int button) const override;
    void getMousePosition(float& x, float& y) const override;
    void getTouchPosition(int touchId, float& x, float& y) const override;

    int getTouchCount() const override;
    bool isTouchSupported() const override { return PLATFORM_IOS; }
    bool isGamepadSupported() const override { return true; }
    int getGamepadCount() const override;

    void setMousePosition(float x, float y) override;
    void showCursor(bool show) override;
    void captureCursor(bool capture) override;

    // Apple-specific input methods
    #if PLATFORM_IOS
        void handleTouchEvent(UITouch* touch, UIEvent* event);
        void handleMotionEvent(CMDeviceMotion* motion);
        void handleAccelerometerEvent(CMAccelerometerData* accelerometer);
        void handleGyroscopeEvent(CMGyroscopeData* gyroscope);
        void handleMagnetometerEvent(CMMagnetometerData* magnetometer);
    #elif PLATFORM_MACOS
        void handleKeyEvent(NSEvent* event);
        void handleMouseEvent(NSEvent* event);
        void handleScrollEvent(NSEvent* event);
        void handleGestureEvent(NSGestureRecognizer* gesture);
    #endif

    // Game Controller support
    void handleGamepadConnected(GCController* controller);
    void handleGamepadDisconnected(GCController* controller);
    void updateGamepadState(GCController* controller);

private:
    MaciOSPlatformPAL* platform_;
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> mouseStates_;
    std::unordered_map<int, std::pair<float, float>> touchPositions_;
    std::unordered_map<GCController*, bool> gamepadStates_;
    float mouseX_, mouseY_;
    bool cursorVisible_;
    bool cursorCaptured_;

    #if PLATFORM_IOS
        CMMotionManager* motionManager_;
        NSArray<UITouch*>* activeTouches_;
    #elif PLATFORM_MACOS
        NSView* trackingView_;
    #endif

    void updateKeyboardState();
    void updateMouseState();
    void updateTouchState();
    void updateGamepadStates();
    void processMotionData();
};

// ========== MAC/IOS NETWORK CONTEXT ==========
class MaciOSNetworkContext : public NetworkContext {
public:
    MaciOSNetworkContext(MaciOSPlatformPAL* platform);
    ~MaciOSNetworkContext() override;

    bool initialize() override;
    void shutdown() override;
    void update() override;

    NetworkAPI getNetworkAPI() const override { return NetworkAPI::POSIX_SOCKETS; }
    bool isNetworkAvailable() const override;
    std::string getNetworkType() const override;
    int getSignalStrength() const override;

    bool connect(const std::string& host, int port) override;
    void disconnect() override;
    bool isConnected() const override;

    int send(const void* data, size_t size) override;
    int receive(void* buffer, size_t size) override;

    void* getNativeSocket() const override { return (void*)socket_; }

    // Network-specific methods
    int getSocket() const { return socket_; }
    bool initializeNetwork();
    void shutdownNetwork();
    void updateNetworkStatus();

private:
    MaciOSPlatformPAL* platform_;
    int socket_;
    bool connected_;
    std::string currentNetworkType_;
    int signalStrength_;

    bool initializeNetworking();
    void shutdownNetworking();
    void detectNetworkChanges();
    void getNetworkInterfaceInfo();
};

// ========== MAC/IOS STORAGE CONTEXT ==========
class MaciOSStorageContext : public StorageContext {
public:
    MaciOSStorageContext(MaciOSPlatformPAL* platform);
    ~MaciOSStorageContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;

    StorageAPI getStorageAPI() const override { return StorageAPI::POSIX_FILESYSTEM; }
    std::string getBasePath() const override { return basePath_; }
    std::string getDocumentsPath() const override { return documentsPath_; }
    std::string getCachePath() const override { return cachePath_; }
    std::string getTempPath() const override { return tempPath_; }

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

    // Apple-specific storage methods
    void setBasePath(const std::string& path);
    void setDocumentsPath(const std::string& path);
    void setCachePath(const std::string& path);
    void setTempPath(const std::string& path);

    #if PLATFORM_IOS
        void enableiCloudSync();
        void disableiCloudSync();
        void syncWithiCloud();
    #elif PLATFORM_MACOS
        void enableiCloudSync();
        void disableiCloudSync();
        void syncWithiCloud();
    #endif

private:
    MaciOSPlatformPAL* platform_;
    std::string basePath_;
    std::string documentsPath_;
    std::string cachePath_;
    std::string tempPath_;

    bool initializePaths();
    std::string resolvePath(const std::string& path) const;
    std::string getKnownFolderPath(const std::string& folderName) const;
    bool hasWritePermission(const std::string& path) const;
};

// ========== MAC/IOS PLATFORM SERVICES ==========
class MaciOSPlatformServices : public PlatformServices {
public:
    MaciOSPlatformServices(MaciOSPlatformPAL* platform);
    ~MaciOSPlatformServices() override;

    bool initialize() override;
    void shutdown() override;

    // In-App Purchase
    bool isIAPSupported() const override { return true; }
    bool purchaseProduct(const std::string& productId) override;
    bool restorePurchases() override;
    std::vector<std::string> getProducts() const override;

    // Achievements
    bool isAchievementsSupported() const override { return true; }
    bool unlockAchievement(const std::string& achievementId) override;
    bool incrementAchievement(const std::string& achievementId, int increment) override;
    std::vector<std::string> getUnlockedAchievements() const override;

    // Leaderboards
    bool isLeaderboardsSupported() const override { return true; }
    bool submitScore(const std::string& leaderboardId, int score) override;
    bool showLeaderboard(const std::string& leaderboardId) override;
    std::vector<std::pair<std::string, int>> getLeaderboardScores(const std::string& leaderboardId) const override;

    // Cloud Save
    bool isCloudSaveSupported() const override { return true; }
    bool saveToCloud(const std::string& key, const std::vector<uint8_t>& data) override;
    bool loadFromCloud(const std::string& key, std::vector<uint8_t>& data) override;
    bool deleteFromCloud(const std::string& key) override;

    // Push Notifications
    bool isPushNotificationsSupported() const override { return true; }
    bool registerForPushNotifications() override;
    bool unregisterForPushNotifications() override;
    void scheduleNotification(const std::string& title, const std::string& message, int delaySeconds) override;

private:
    MaciOSPlatformPAL* platform_;

    bool initializeGameKit();
    void shutdownGameKit();
    bool initializeCloudKit();
    void shutdownCloudKit();
    bool initializeUserNotifications();
    void shutdownUserNotifications();
};

// ========== MAC/IOS WINDOW MANAGER ==========
class MaciOSWindowManager : public WindowManager {
public:
    MaciOSWindowManager(MaciOSPlatformPAL* platform);
    ~MaciOSWindowManager() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void update() override;

    void* getNativeWindow() const override {
        return PLATFORM_IOS ? (__bridge void*)view_ : (__bridge void*)window_;
    }
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
    bool isMinimized() const override { return minimized_; }
    bool isMaximized() const override { return maximized_; }
    bool isVisible() const override { return visible_; }
    bool isFocused() const override { return focused_; }

    void show() override;
    void hide() override;
    void minimize() override;
    void maximize() override;
    void restore() override;
    void focus() override;

    // Apple-specific window methods
    #if PLATFORM_IOS
        void setView(UIView* view);
        void setViewController(UIViewController* viewController);
        void handleOrientationChange(UIInterfaceOrientation orientation);
        void handleStatusBarChange();
    #elif PLATFORM_MACOS
        void setWindow(NSWindow* window);
        void setView(NSView* view);
        void handleWindowResize();
        void handleWindowMove();
        void handleWindowFocus();
    #endif

private:
    MaciOSPlatformPAL* platform_;
    int width_;
    int height_;
    float scale_;
    bool fullscreen_;
    bool minimized_;
    bool maximized_;
    bool visible_;
    bool focused_;
    bool resizable_;

    #if PLATFORM_IOS
        UIView* view_;
        UIViewController* viewController_;
    #elif PLATFORM_MACOS
        NSWindow* window_;
        NSView* view_;
    #endif

    void updateWindowProperties();
    void handleWindowResize(int width, int height);
    void handleWindowMove(int x, int y);
    void handleFocusChange(bool focused);
    void handleVisibilityChange(bool visible);
};

// ========== MAC/IOS EVENT SYSTEM ==========
class MaciOSEventSystem : public EventSystem {
public:
    MaciOSEventSystem(MaciOSPlatformPAL* platform);
    ~MaciOSEventSystem() override;

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

    // Apple-specific event methods
    #if PLATFORM_IOS
        void handleTouchEvent(UITouch* touch, UIEvent* event);
        void handleMotionEvent(UIEvent* event);
        void handleApplicationEvent(UIApplication* application, NSNotification* notification);
    #elif PLATFORM_MACOS
        void handleKeyEvent(NSEvent* event);
        void handleMouseEvent(NSEvent* event);
        void handleApplicationEvent(NSApplication* application, NSNotification* notification);
        void handleWindowEvent(NSWindow* window, NSEvent* event);
    #endif

private:
    MaciOSPlatformPAL* platform_;
    std::unordered_map<PlatformEventType, std::vector<PlatformEventCallback>> callbacks_;
    std::queue<PlatformEvent> eventQueue_;
    std::unordered_map<PlatformEventType, bool> enabledEvents_;
    std::mutex eventMutex_;

    void queueEvent(const PlatformEvent& event);
    void dispatchEvent(const PlatformEvent& event);
    void processAppleEvents();
    void handleAppleInputEvents();
    void handleAppleSystemEvents();
    void handleAppleApplicationEvents();
};

// ========== APPLE NOTIFICATION MANAGER ==========
class AppleNotificationManager {
public:
    static bool initialize();
    static void shutdown();
    static bool requestPermission();
    static bool hasPermission();
    static void scheduleNotification(const std::string& title, const std::string& message, int delaySeconds);
    static void removeScheduledNotification(const std::string& identifier);
    static void removeAllScheduledNotifications();

private:
    static bool initialized_;
    static bool hasPermission_;
};

// ========== APPLE HAPTIC FEEDBACK ==========
class AppleHapticFeedback {
public:
    static bool initialize();
    static void shutdown();
    static void prepareHaptics();
    static void triggerImpact(int style);
    static void triggerNotification(int type);
    static void triggerSelection();

private:
    static bool initialized_;
    #if PLATFORM_IOS
        static UIImpactFeedbackGenerator* impactGenerator_;
        static UINotificationFeedbackGenerator* notificationGenerator_;
        static UISelectionFeedbackGenerator* selectionGenerator_;
    #elif PLATFORM_MACOS
        static NSHapticFeedbackManager* hapticManager_;
    #endif
};

// ========== APPLE MOTION MANAGER ==========
class AppleMotionManager {
public:
    static bool initialize();
    static void shutdown();
    static void startMotionUpdates();
    static void stopMotionUpdates();
    static void getAccelerometerData(float& x, float& y, float& z);
    static void getGyroscopeData(float& x, float& y, float& z);
    static void getMagnetometerData(float& x, float& y, float& z);

private:
    static bool initialized_;
    static CMMotionManager* motionManager_;
    static CMAccelerometerData* accelerometerData_;
    static CMGyroData* gyroData_;
    static CMMagnetometerData* magnetometerData_;
};

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_MAC_IOS_PLATFORM_PAL_H
