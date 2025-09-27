#ifndef FOUNDRYENGINE_ANDROID_PLATFORM_PAL_H
#define FOUNDRYENGINE_ANDROID_PLATFORM_PAL_H

#include "../../include/GameEngine/platform/PlatformInterface.h"
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace FoundryEngine {

// Forward declarations
class AndroidGraphicsContext;
class AndroidAudioContext;
class AndroidInputContext;
class AndroidNetworkContext;
class AndroidStorageContext;
class AndroidPlatformServices;
class AndroidWindowManager;
class AndroidEventSystem;

// ========== ANDROID PLATFORM PAL IMPLEMENTATION ==========
class AndroidPlatformPAL : public PlatformInterface {
public:
    AndroidPlatformPAL();
    ~AndroidPlatformPAL() override;

    // PlatformInterface implementation
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    PlatformType getPlatformType() const override { return PlatformType::ANDROID; }
    PlatformCapabilities getCapabilities() const override { return capabilities_; }
    PlatformConfig getConfig() const override { return config_; }
    std::string getPlatformName() const override { return "Android"; }
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
    void* getNativeHandle() const override { return nativeActivity_; }
    void* getNativeDisplay() const override { return nullptr; }
    void* getNativeWindow() const override { return nativeWindow_; }

    // Android-specific methods
    void setNativeActivity(ANativeActivity* activity);
    void setNativeWindow(ANativeWindow* window);
    void setAssetManager(AAssetManager* assetManager);
    void setJavaVM(JavaVM* javaVM);

    // JNI environment access
    JNIEnv* getJNIEnvironment() const { return jniEnv_; }
    jobject getApplicationContext() const { return applicationContext_; }

    // Advanced Android features
    void enableImmersiveMode(bool enable);
    void setNavigationBarColor(int color);
    void setStatusBarColor(int color);
    void requestPermissions(const std::vector<std::string>& permissions);
    bool hasPermission(const std::string& permission) const;

private:
    // Android-specific members
    ANativeActivity* nativeActivity_;
    ANativeWindow* nativeWindow_;
    AAssetManager* assetManager_;
    JavaVM* javaVM_;
    JNIEnv* jniEnv_;
    jobject applicationContext_;

    // Context implementations
    AndroidGraphicsContext* graphicsContext_;
    AndroidAudioContext* audioContext_;
    AndroidInputContext* inputContext_;
    AndroidNetworkContext* networkContext_;
    AndroidStorageContext* storageContext_;

    // Service implementations
    AndroidPlatformServices* platformServices_;
    AndroidWindowManager* windowManager_;
    AndroidEventSystem* eventSystem_;

    // Platform state
    std::atomic<bool> initialized_;
    std::atomic<bool> appActive_;
    std::atomic<bool> windowFocused_;
    std::atomic<bool> immersiveMode_;

    // Threading
    std::thread lifecycleThread_;
    std::mutex platformMutex_;

    // Android system information
    std::string deviceId_;
    std::string deviceModel_;
    std::string osVersion_;
    std::string locale_;
    int apiLevel_;

    // Performance monitoring
    mutable float cpuUsage_;
    mutable float memoryUsage_;
    mutable size_t totalMemory_;
    mutable size_t availableMemory_;
    mutable float batteryLevel_;
    mutable bool batteryCharging_;

    // Initialize platform capabilities
    void initializeCapabilities();

    // Initialize Android contexts
    void initializeGraphicsContext();
    void initializeAudioContext();
    void initializeInputContext();
    void initializeNetworkContext();
    void initializeStorageContext();

    // Initialize Android services
    void initializePlatformServices();
    void initializeWindowManager();
    void initializeEventSystem();

    // Android system queries
    void querySystemInformation();
    void queryMemoryInformation();
    void queryBatteryInformation();
    void queryDisplayInformation();

    // JNI helper methods
    void attachToJavaThread();
    void detachFromJavaThread();
    void initializeJNIReferences();

    // Event processing
    void processAndroidEvents();
    void handleAndroidLifecycleEvents();
    void handleAndroidInputEvents();
    void handleAndroidSensorEvents();

    // Platform-specific utilities
    std::string getAndroidDeviceId() const;
    std::string getAndroidDeviceModel() const;
    std::string getAndroidOSVersion() const;
    std::string getAndroidLocale() const;
    int getAndroidAPILevel() const;

    // Memory management
    void updateMemoryStats();
    void updateCPUStats();
    void updateBatteryStats();

    // Android permissions
    void requestAndroidPermissions();
    bool checkAndroidPermission(const std::string& permission) const;
    void handlePermissionResults();

    // Android-specific features
    void setAndroidOrientation(int orientation);
    void setAndroidFullscreen(bool fullscreen);
    void setAndroidKeepScreenOn(bool keepOn);
    void performAndroidVibration(int durationMs);
    void showAndroidToast(const std::string& message);

    // Thread management
    void startLifecycleThread();
    void stopLifecycleThread();
    void lifecycleThreadLoop();
};

// ========== ANDROID GRAPHICS CONTEXT ==========
class AndroidGraphicsContext : public GraphicsContext {
public:
    AndroidGraphicsContext(AndroidPlatformPAL* platform);
    ~AndroidGraphicsContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void swapBuffers() override;
    void makeCurrent() override;
    void setSwapInterval(int interval) override;

    GraphicsAPI getGraphicsAPI() const override { return GraphicsAPI::VULKAN; }
    int getMajorVersion() const override { return 1; }
    int getMinorVersion() const override { return 3; }
    std::string getVendor() const override;
    std::string getRenderer() const override;

    int getFramebufferWidth() const override;
    int getFramebufferHeight() const override;
    float getDisplayScale() const override;

    void* getNativeContext() const override { return nullptr; }
    void* getNativeDisplay() const override { return nullptr; }

private:
    AndroidPlatformPAL* platform_;
    ANativeWindow* nativeWindow_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLConfig config_;

    bool initializeEGL();
    bool createEGLSurface();
    bool createEGLContext();
    void destroyEGLSurface();
    void destroyEGLContext();
    void terminateEGL();
};

// ========== ANDROID AUDIO CONTEXT ==========
class AndroidAudioContext : public AudioContext {
public:
    AndroidAudioContext(AndroidPlatformPAL* platform);
    ~AndroidAudioContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void suspend() override;
    void resume() override;

    AudioAPI getAudioAPI() const override { return AudioAPI::AAUDIO; }
    int getSampleRate() const override { return sampleRate_; }
    int getChannels() const override { return channels_; }
    int getBufferSize() const override { return bufferSize_; }

    float getMasterVolume() const override { return masterVolume_; }
    void setMasterVolume(float volume) override;

    void* getNativeContext() const override { return nullptr; }

private:
    AndroidPlatformPAL* platform_;
    int sampleRate_;
    int channels_;
    int bufferSize_;
    float masterVolume_;

    bool initializeAAudio();
    void shutdownAAudio();
    void updateStreamConfiguration();
};

// ========== ANDROID INPUT CONTEXT ==========
class AndroidInputContext : public InputContext {
public:
    AndroidInputContext(AndroidPlatformPAL* platform);
    ~AndroidInputContext() override;

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

private:
    AndroidPlatformPAL* platform_;
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> mouseStates_;
    std::unordered_map<int, std::pair<float, float>> touchPositions_;
    float mouseX_, mouseY_;
    bool cursorVisible_;
    bool cursorCaptured_;

    void processKeyEvent(int keyCode, bool pressed);
    void processTouchEvent(int touchId, float x, float y, bool pressed);
    void processMotionEvent(float x, float y);
    void updateInputStates();
};

// ========== ANDROID NETWORK CONTEXT ==========
class AndroidNetworkContext : public NetworkContext {
public:
    AndroidNetworkContext(AndroidPlatformPAL* platform);
    ~AndroidNetworkContext() override;

    bool initialize() override;
    void shutdown() override;
    void update() override;

    NetworkAPI getNetworkAPI() const override { return NetworkAPI::NATIVE_SOCKETS; }
    bool isNetworkAvailable() const override;
    std::string getNetworkType() const override;
    int getSignalStrength() const override;

    bool connect(const std::string& host, int port) override;
    void disconnect() override;
    bool isConnected() const override;

    int send(const void* data, size_t size) override;
    int receive(void* buffer, size_t size) override;

    void* getNativeSocket() const override { return nullptr; }

private:
    AndroidPlatformPAL* platform_;
    int socket_;
    bool connected_;
    std::string currentNetworkType_;
    int signalStrength_;

    bool initializeNetworking();
    void shutdownNetworking();
    void updateNetworkStatus();
    void detectNetworkChanges();
};

// ========== ANDROID STORAGE CONTEXT ==========
class AndroidStorageContext : public StorageContext {
public:
    AndroidStorageContext(AndroidPlatformPAL* platform);
    ~AndroidStorageContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;

    StorageAPI getStorageAPI() const override { return StorageAPI::ANDROID_STORAGE; }
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

private:
    AndroidPlatformPAL* platform_;
    std::string basePath_;
    std::string documentsPath_;
    std::string cachePath_;
    std::string tempPath_;

    bool initializePaths();
    std::string resolvePath(const std::string& path) const;
    bool hasStoragePermission() const;
    void requestStoragePermission();
};

// ========== ANDROID PLATFORM SERVICES ==========
class AndroidPlatformServices : public PlatformServices {
public:
    AndroidPlatformServices(AndroidPlatformPAL* platform);
    ~AndroidPlatformServices() override;

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
    AndroidPlatformPAL* platform_;

    bool initializeGooglePlayServices();
    void shutdownGooglePlayServices();
    bool checkGooglePlayServicesAvailability();
};

// ========== ANDROID WINDOW MANAGER ==========
class AndroidWindowManager : public WindowManager {
public:
    AndroidWindowManager(AndroidPlatformPAL* platform);
    ~AndroidWindowManager() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void update() override;

    void* getNativeWindow() const override { return nativeWindow_; }
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
    bool isMinimized() const override { return false; } // Android doesn't minimize
    bool isMaximized() const override { return false; } // Android doesn't maximize
    bool isVisible() const override { return visible_; }
    bool isFocused() const override { return focused_; }

    void show() override;
    void hide() override;
    void minimize() override;
    void maximize() override;
    void restore() override;
    void focus() override;

private:
    AndroidPlatformPAL* platform_;
    ANativeWindow* nativeWindow_;
    int width_;
    int height_;
    float scale_;
    bool fullscreen_;
    bool visible_;
    bool focused_;

    void updateWindowProperties();
    void handleSurfaceChanged(int width, int height);
    void handleSurfaceRedrawNeeded();
    void handleSurfaceDestroyed();
};

// ========== ANDROID EVENT SYSTEM ==========
class AndroidEventSystem : public EventSystem {
public:
    AndroidEventSystem(AndroidPlatformPAL* platform);
    ~AndroidEventSystem() override;

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

private:
    AndroidPlatformPAL* platform_;
    std::unordered_map<PlatformEventType, std::vector<PlatformEventCallback>> callbacks_;
    std::queue<PlatformEvent> eventQueue_;
    std::unordered_map<PlatformEventType, bool> enabledEvents_;
    std::mutex eventMutex_;

    void processAndroidAppEvents();
    void processAndroidInputEvents();
    void processAndroidSensorEvents();
    void processAndroidSystemEvents();

    void queueEvent(const PlatformEvent& event);
    void dispatchEvent(const PlatformEvent& event);
    void handleAppCommand(int32_t cmd);
    void handleInputEvent(AInputEvent* event);
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // Platform lifecycle callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onAppStart(JNIEnv* env, jobject thiz);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onAppPause(JNIEnv* env, jobject thiz);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onAppResume(JNIEnv* env, jobject thiz);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onAppStop(JNIEnv* env, jobject thiz);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onAppDestroy(JNIEnv* env, jobject thiz);

    // Surface lifecycle callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onSurfaceCreated(JNIEnv* env, jobject thiz);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onSurfaceChanged(JNIEnv* env, jobject thiz, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onSurfaceRedrawNeeded(JNIEnv* env, jobject thiz);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onSurfaceDestroyed(JNIEnv* env, jobject thiz);

    // Input event callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onTouchEvent(JNIEnv* env, jobject thiz, int action, float x, float y, int pointerId);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onKeyEvent(JNIEnv* env, jobject thiz, int keyCode, int action);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onMotionEvent(JNIEnv* env, jobject thiz, float x, float y);

    // Sensor event callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onAccelerometerChanged(JNIEnv* env, jobject thiz, float x, float y, float z);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onGyroscopeChanged(JNIEnv* env, jobject thiz, float x, float y, float z);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onMagnetometerChanged(JNIEnv* env, jobject thiz, float x, float y, float z);

    // System event callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onLowMemory(JNIEnv* env, jobject thiz);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onBatteryLow(JNIEnv* env, jobject thiz);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onBatteryStatus(JNIEnv* env, jobject thiz, int level, jboolean charging);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onNetworkChanged(JNIEnv* env, jobject thiz, jstring networkType, int signalStrength);

    // Permission callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onPermissionGranted(JNIEnv* env, jobject thiz, jstring permission);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onPermissionDenied(JNIEnv* env, jobject thiz, jstring permission);

    // Platform service callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onIAPResult(JNIEnv* env, jobject thiz, jstring productId, jboolean success);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onAchievementUnlocked(JNIEnv* env, jobject thiz, jstring achievementId);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onLeaderboardScore(JNIEnv* env, jobject thiz, jstring leaderboardId, int score);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onCloudSaveResult(JNIEnv* env, jobject thiz, jstring key, jboolean success);
    JNIEXPORT void JNICALL Java_com_foundryengine_android_FoundryActivity_onPushNotificationReceived(JNIEnv* env, jobject thiz, jstring title, jstring message);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_ANDROID_PLATFORM_PAL_H
