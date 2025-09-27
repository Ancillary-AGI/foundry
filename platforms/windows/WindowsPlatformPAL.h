#ifndef FOUNDRYENGINE_WINDOWS_PLATFORM_PAL_H
#define FOUNDRYENGINE_WINDOWS_PLATFORM_PAL_H

#include "../../include/GameEngine/platform/PlatformInterface.h"
#include <windows.h>
#include <windowsx.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <xaudio2.h>
#include <xinput.h>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace FoundryEngine {

// Forward declarations
class WindowsGraphicsContext;
class WindowsAudioContext;
class WindowsInputContext;
class WindowsNetworkContext;
class WindowsStorageContext;
class WindowsPlatformServices;
class WindowsWindowManager;
class WindowsEventSystem;

// ========== WINDOWS PLATFORM PAL IMPLEMENTATION ==========
class WindowsPlatformPAL : public PlatformInterface {
public:
    WindowsPlatformPAL();
    ~WindowsPlatformPAL() override;

    // PlatformInterface implementation
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    PlatformType getPlatformType() const override { return PlatformType::WINDOWS; }
    PlatformCapabilities getCapabilities() const override { return capabilities_; }
    PlatformConfig getConfig() const override { return config_; }
    std::string getPlatformName() const override { return "Windows"; }
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
    void* getNativeHandle() const override { return hInstance_; }
    void* getNativeDisplay() const override { return nullptr; }
    void* getNativeWindow() const override { return hwnd_; }

    // Windows-specific methods
    void setInstanceHandle(HINSTANCE hInstance);
    void setWindowHandle(HWND hwnd);
    void setCommandShow(int nCmdShow);
    void setIconHandle(HICON hIcon);

    // Windows API access
    HINSTANCE getInstanceHandle() const { return hInstance_; }
    HWND getWindowHandle() const { return hwnd_; }
    int getCommandShow() const { return nCmdShow_; }
    HICON getIconHandle() const { return hIcon_; }

    // Windows-specific features
    void enableHighDPIAwareness();
    void setProcessDPIAwareness();
    void enableDarkMode();
    void setWindowIcon(const std::string& iconPath);
    void setTaskbarIcon(const std::string& iconPath);
    void addTrayIcon(const std::string& iconPath, const std::string& tooltip);
    void removeTrayIcon();

private:
    // Windows-specific members
    HINSTANCE hInstance_;
    HWND hwnd_;
    int nCmdShow_;
    HICON hIcon_;
    NOTIFYICONDATA trayIconData_;
    bool highDPIAware_;
    bool darkModeEnabled_;

    // Context implementations
    WindowsGraphicsContext* graphicsContext_;
    WindowsAudioContext* audioContext_;
    WindowsInputContext* inputContext_;
    WindowsNetworkContext* networkContext_;
    WindowsStorageContext* storageContext_;

    // Service implementations
    WindowsPlatformServices* platformServices_;
    WindowsWindowManager* windowManager_;
    WindowsEventSystem* eventSystem_;

    // Platform state
    std::atomic<bool> initialized_;
    std::atomic<bool> appActive_;
    std::atomic<bool> windowFocused_;

    // Threading
    std::thread messageThread_;
    std::mutex platformMutex_;

    // Windows system information
    std::string deviceId_;
    std::string deviceModel_;
    std::string osVersion_;
    std::string locale_;
    OSVERSIONINFOEX osVersionInfo_;

    // Performance monitoring
    mutable float cpuUsage_;
    mutable float memoryUsage_;
    mutable size_t totalMemory_;
    mutable size_t availableMemory_;
    mutable float batteryLevel_;
    mutable bool batteryCharging_;

    // Initialize platform capabilities
    void initializeCapabilities();

    // Initialize Windows contexts
    void initializeGraphicsContext();
    void initializeAudioContext();
    void initializeInputContext();
    void initializeNetworkContext();
    void initializeStorageContext();

    // Initialize Windows services
    void initializePlatformServices();
    void initializeWindowManager();
    void initializeEventSystem();

    // Windows system queries
    void querySystemInformation();
    void queryMemoryInformation();
    void queryBatteryInformation();
    void queryDisplayInformation();

    // Windows-specific utilities
    std::string getWindowsDeviceId() const;
    std::string getWindowsDeviceModel() const;
    std::string getWindowsOSVersion() const;
    std::string getWindowsLocale() const;

    // Memory management
    void updateMemoryStats();
    void updateCPUStats();
    void updateBatteryStats();

    // Windows-specific features
    void setWindowsOrientation(int orientation);
    void setWindowsFullscreen(bool fullscreen);
    void setWindowsKeepScreenOn(bool keepOn);
    void performWindowsVibration(int durationMs);
    void showWindowsToast(const std::string& message);

    // Windows message handling
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void processWindowsMessages();
    void handleWindowsEvents();

    // Thread management
    void startMessageLoop();
    void stopMessageLoop();
    void messageLoop();
};

// ========== WINDOWS GRAPHICS CONTEXT ==========
class WindowsGraphicsContext : public GraphicsContext {
public:
    WindowsGraphicsContext(WindowsPlatformPAL* platform);
    ~WindowsGraphicsContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void swapBuffers() override;
    void makeCurrent() override;
    void setSwapInterval(int interval) override;

    GraphicsAPI getGraphicsAPI() const override { return GraphicsAPI::DIRECTX12; }
    int getMajorVersion() const override { return 12; }
    int getMinorVersion() const override { return 0; }
    std::string getVendor() const override;
    std::string getRenderer() const override;

    int getFramebufferWidth() const override;
    int getFramebufferHeight() const override;
    float getDisplayScale() const override;

    void* getNativeContext() const override { return device_; }
    void* getNativeDisplay() const override { return nullptr; }

    // DirectX 12-specific methods
    ID3D12Device* getD3D12Device() const { return device_; }
    ID3D12CommandQueue* getCommandQueue() const { return commandQueue_; }
    IDXGISwapChain3* getSwapChain() const { return swapChain_; }
    ID3D12DescriptorHeap* getRTVHeap() const { return rtvHeap_; }
    ID3D12DescriptorHeap* getDSVHeap() const { return dsvHeap_; }
    ID3D12DescriptorHeap* getSRVHeap() const { return srvHeap_; }

    UINT getRTVDescriptorSize() const { return rtvDescriptorSize_; }
    UINT getDSVDescriptorSize() const { return dsvDescriptorSize_; }
    UINT getSRVDescriptorSize() const { return srvDescriptorSize_; }

private:
    WindowsPlatformPAL* platform_;
    HWND hwnd_;

    // DirectX 12 objects
    ID3D12Device* device_;
    ID3D12CommandQueue* commandQueue_;
    IDXGISwapChain3* swapChain_;
    ID3D12DescriptorHeap* rtvHeap_;
    ID3D12DescriptorHeap* dsvHeap_;
    ID3D12DescriptorHeap* srvHeap_;
    ID3D12Resource* renderTargets_[2];
    ID3D12CommandAllocator* commandAllocator_;
    ID3D12GraphicsCommandList* commandList_;
    ID3D12Fence* fence_;
    HANDLE fenceEvent_;
    UINT frameIndex_;
    UINT rtvDescriptorSize_;
    UINT dsvDescriptorSize_;
    UINT srvDescriptorSize_;

    bool initializeDirectX12();
    void createDevice();
    void createCommandQueue();
    void createSwapChain();
    void createDescriptorHeaps();
    void createFrameResources();
    void createSyncObjects();
    void cleanupDirectX12();
};

// ========== WINDOWS AUDIO CONTEXT ==========
class WindowsAudioContext : public AudioContext {
public:
    WindowsAudioContext(WindowsPlatformPAL* platform);
    ~WindowsAudioContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void suspend() override;
    void resume() override;

    AudioAPI getAudioAPI() const override { return AudioAPI::WASAPI; }
    int getSampleRate() const override { return sampleRate_; }
    int getChannels() const override { return channels_; }
    int getBufferSize() const override { return bufferSize_; }

    float getMasterVolume() const override { return masterVolume_; }
    void setMasterVolume(float volume) override;

    void* getNativeContext() const override { return xaudio2_; }

    // XAudio2-specific methods
    IXAudio2* getXAudio2() const { return xaudio2_; }
    IXAudio2MasteringVoice* getMasteringVoice() const { return masteringVoice_; }
    IXAudio2SubmixVoice* getSubmixVoice() const { return submixVoice_; }

    void createSourceVoice(IXAudio2SourceVoice** sourceVoice, const WAVEFORMATEX* format);
    void createSubmixVoice(IXAudio2SubmixVoice** submixVoice, const WAVEFORMATEX* format);

private:
    WindowsPlatformPAL* platform_;
    int sampleRate_;
    int channels_;
    int bufferSize_;
    float masterVolume_;

    // XAudio2 objects
    IXAudio2* xaudio2_;
    IXAudio2MasteringVoice* masteringVoice_;
    IXAudio2SubmixVoice* submixVoice_;

    bool initializeXAudio2();
    void shutdownXAudio2();
    void createMasteringVoice();
    void createSubmixVoice();
    void setupAudioProcessing();
};

// ========== WINDOWS INPUT CONTEXT ==========
class WindowsInputContext : public InputContext {
public:
    WindowsInputContext(WindowsPlatformPAL* platform);
    ~WindowsInputContext() override;

    bool initialize() override;
    void shutdown() override;
    void update() override;

    bool isKeyPressed(int keyCode) const override;
    bool isMouseButtonPressed(int button) const override;
    void getMousePosition(float& x, float& y) const override;
    void getTouchPosition(int touchId, float& x, float& y) const override;

    int getTouchCount() const override;
    bool isTouchSupported() const override { return false; } // Windows doesn't have touch by default
    bool isGamepadSupported() const override { return true; }
    int getGamepadCount() const override;

    void setMousePosition(float x, float y) override;
    void showCursor(bool show) override;
    void captureCursor(bool capture) override;

    // Windows-specific input methods
    void handleKeyboardMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void handleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void handleGamepadInput();
    void handleRawInput(RAWINPUT* rawInput);

    // XInput gamepad support
    void updateGamepadState(int gamepadIndex);
    bool isGamepadButtonPressed(int gamepadIndex, WORD button) const;
    float getGamepadAxis(int gamepadIndex, WORD axis) const;

private:
    WindowsPlatformPAL* platform_;
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> mouseStates_;
    float mouseX_, mouseY_;
    bool cursorVisible_;
    bool cursorCaptured_;

    // Gamepad states
    XINPUT_STATE gamepadStates_[XUSER_MAX_COUNT];
    bool gamepadConnected_[XUSER_MAX_COUNT];

    void updateKeyboardState();
    void updateMouseState();
    void updateGamepadStates();
    void processRawInputData();
};

// ========== WINDOWS NETWORK CONTEXT ==========
class WindowsNetworkContext : public NetworkContext {
public:
    WindowsNetworkContext(WindowsPlatformPAL* platform);
    ~WindowsNetworkContext() override;

    bool initialize() override;
    void shutdown() override;
    void update() override;

    NetworkAPI getNetworkAPI() const override { return NetworkAPI::WINSOCK; }
    bool isNetworkAvailable() const override;
    std::string getNetworkType() const override;
    int getSignalStrength() const override;

    bool connect(const std::string& host, int port) override;
    void disconnect() override;
    bool isConnected() const override;

    int send(const void* data, size_t size) override;
    int receive(void* buffer, size_t size) override;

    void* getNativeSocket() const override { return (void*)socket_; }

    // Winsock-specific methods
    SOCKET getSocket() const { return socket_; }
    bool initializeWinsock();
    void shutdownWinsock();
    void updateNetworkStatus();

private:
    WindowsPlatformPAL* platform_;
    SOCKET socket_;
    bool connected_;
    std::string currentNetworkType_;
    int signalStrength_;
    WSADATA wsaData_;

    bool initializeNetworking();
    void shutdownNetworking();
    void detectNetworkChanges();
    void getNetworkAdapterInfo();
};

// ========== WINDOWS STORAGE CONTEXT ==========
class WindowsStorageContext : public StorageContext {
public:
    WindowsStorageContext(WindowsPlatformPAL* platform);
    ~WindowsStorageContext() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;

    StorageAPI getStorageAPI() const override { return StorageAPI::WINDOWS_STORAGE; }
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

    // Windows-specific storage methods
    void setBasePath(const std::string& path);
    void setDocumentsPath(const std::string& path);
    void setCachePath(const std::string& path);
    void setTempPath(const std::string& path);

private:
    WindowsPlatformPAL* platform_;
    std::string basePath_;
    std::string documentsPath_;
    std::string cachePath_;
    std::string tempPath_;

    bool initializePaths();
    std::string resolvePath(const std::string& path) const;
    std::string getKnownFolderPath(REFKNOWNFOLDERID folderId) const;
    bool hasWritePermission(const std::string& path) const;
};

// ========== WINDOWS PLATFORM SERVICES ==========
class WindowsPlatformServices : public PlatformServices {
public:
    WindowsPlatformServices(WindowsPlatformPAL* platform);
    ~WindowsPlatformServices() override;

    bool initialize() override;
    void shutdown() override;

    // In-App Purchase (not supported on Windows desktop)
    bool isIAPSupported() const override { return false; }
    bool purchaseProduct(const std::string& productId) override { return false; }
    bool restorePurchases() override { return false; }
    std::vector<std::string> getProducts() const override { return {}; }

    // Achievements (not supported on Windows desktop)
    bool isAchievementsSupported() const override { return false; }
    bool unlockAchievement(const std::string& achievementId) override { return false; }
    bool incrementAchievement(const std::string& achievementId, int increment) override { return false; }
    std::vector<std::string> getUnlockedAchievements() const override { return {}; }

    // Leaderboards (not supported on Windows desktop)
    bool isLeaderboardsSupported() const override { return false; }
    bool submitScore(const std::string& leaderboardId, int score) override { return false; }
    bool showLeaderboard(const std::string& leaderboardId) override { return false; }
    std::vector<std::pair<std::string, int>> getLeaderboardScores(const std::string& leaderboardId) const override { return {}; }

    // Cloud Save (using OneDrive or custom solution)
    bool isCloudSaveSupported() const override { return true; }
    bool saveToCloud(const std::string& key, const std::vector<uint8_t>& data) override;
    bool loadFromCloud(const std::string& key, std::vector<uint8_t>& data) override;
    bool deleteFromCloud(const std::string& key) override;

    // Push Notifications (using Windows Notification Platform)
    bool isPushNotificationsSupported() const override { return true; }
    bool registerForPushNotifications() override;
    bool unregisterForPushNotifications() override;
    void scheduleNotification(const std::string& title, const std::string& message, int delaySeconds) override;

private:
    WindowsPlatformPAL* platform_;

    bool initializeCloudStorage();
    void shutdownCloudStorage();
    bool initializeNotifications();
    void shutdownNotifications();
};

// ========== WINDOWS WINDOW MANAGER ==========
class WindowsWindowManager : public WindowManager {
public:
    WindowsWindowManager(WindowsPlatformPAL* platform);
    ~WindowsWindowManager() override;

    bool initialize(const PlatformConfig& config) override;
    void shutdown() override;
    void update() override;

    void* getNativeWindow() const override { return hwnd_; }
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

    // Windows-specific window methods
    void setIcon(HICON hIcon);
    void setCursor(HCURSOR hCursor);
    void setStyle(DWORD style);
    void setExtendedStyle(DWORD exStyle);
    void enableDragAndDrop();
    void disableDragAndDrop();

private:
    WindowsPlatformPAL* platform_;
    HWND hwnd_;
    int width_;
    int height_;
    float scale_;
    bool fullscreen_;
    bool minimized_;
    bool maximized_;
    bool visible_;
    bool focused_;
    bool resizable_;

    void updateWindowProperties();
    void handleWindowResize(int width, int height);
    void handleWindowMove(int x, int y);
    void handleFocusChange(bool focused);
    void handleVisibilityChange(bool visible);
};

// ========== WINDOWS EVENT SYSTEM ==========
class WindowsEventSystem : public EventSystem {
public:
    WindowsEventSystem(WindowsPlatformPAL* platform);
    ~WindowsEventSystem() override;

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

    // Windows-specific event methods
    void handleWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void handleKeyboardMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void handleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void handleDeviceMessage(UINT msg, WPARAM wParam, LPARAM lParam);

private:
    WindowsPlatformPAL* platform_;
    std::unordered_map<PlatformEventType, std::vector<PlatformEventCallback>> callbacks_;
    std::queue<PlatformEvent> eventQueue_;
    std::unordered_map<PlatformEventType, bool> enabledEvents_;
    std::mutex eventMutex_;

    void queueEvent(const PlatformEvent& event);
    void dispatchEvent(const PlatformEvent& event);
    void processWindowsMessages();
    void handleSystemEvents();
    void handleInputEvents();
    void handleDeviceEvents();
};

// ========== WINDOWS REGISTRY HELPER ==========
class WindowsRegistry {
public:
    static bool readString(HKEY rootKey, const std::string& subKey, const std::string& valueName, std::string& value);
    static bool writeString(HKEY rootKey, const std::string& subKey, const std::string& valueName, const std::string& value);
    static bool readDWord(HKEY rootKey, const std::string& subKey, const std::string& valueName, DWORD& value);
    static bool writeDWord(HKEY rootKey, const std::string& subKey, const std::string& valueName, DWORD value);
    static bool keyExists(HKEY rootKey, const std::string& subKey);
    static bool createKey(HKEY rootKey, const std::string& subKey);
    static bool deleteKey(HKEY rootKey, const std::string& subKey);
    static bool deleteValue(HKEY rootKey, const std::string& subKey, const std::string& valueName);
};

// ========== WINDOWS PERFORMANCE COUNTER ==========
class WindowsPerformanceCounter {
public:
    static bool initialize();
    static void shutdown();
    static float getCPUUsage();
    static size_t getMemoryUsage();
    static size_t getAvailableMemory();
    static size_t getTotalMemory();

private:
    static bool initialized_;
    static ULARGE_INTEGER lastCPU_;
    static ULARGE_INTEGER lastSysCPU_;
    static ULARGE_INTEGER lastUserCPU_;
    static int numProcessors_;
    static HANDLE self_;
};

// ========== WINDOWS BATTERY MONITOR ==========
class WindowsBatteryMonitor {
public:
    static bool initialize();
    static void shutdown();
    static float getBatteryLevel();
    static bool isBatteryCharging();
    static int getBatteryLifeTime();
    static int getBatteryFullLifeTime();

private:
    static bool initialized_;
    static SYSTEM_POWER_STATUS powerStatus_;
};

// ========== WINDOWS NETWORK MONITOR ==========
class WindowsNetworkMonitor {
public:
    static bool initialize();
    static void shutdown();
    static bool isNetworkAvailable();
    static std::string getNetworkType();
    static int getSignalStrength();

private:
    static bool initialized_;
    static std::vector<IP_ADAPTER_INFO> adapterInfo_;
};

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_WINDOWS_PLATFORM_PAL_H
