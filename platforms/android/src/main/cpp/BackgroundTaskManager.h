#ifndef FOUNDRYENGINE_BACKGROUND_TASK_MANAGER_H
#define FOUNDRYENGINE_BACKGROUND_TASK_MANAGER_H

#include "../../core/System.h"
#include <jni.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <chrono>

namespace FoundryEngine {

// Forward declarations
class BackgroundTaskManager;
class DownloadManager;
class UploadManager;
class TaskScheduler;
class NetworkManager;
class BatteryManager;
class ConnectivityManager;

// Task types
enum class TaskType {
    DOWNLOAD,           // File download
    UPLOAD,             // File upload
    SYNC,               // Data synchronization
    BACKUP,             // Data backup
    CLEANUP,            // Cleanup operations
    MAINTENANCE,        // Maintenance tasks
    ANALYTICS,          // Analytics upload
    UPDATE,             // App updates
    CUSTOM              // Custom task
};

// Task priority levels
enum class TaskPriority {
    LOW,                // Low priority (background)
    NORMAL,             // Normal priority
    HIGH,               // High priority
    CRITICAL            // Critical priority (immediate)
};

// Task execution state
enum class TaskState {
    PENDING,            // Task is waiting to be executed
    RUNNING,            // Task is currently running
    PAUSED,             // Task is paused
    COMPLETED,          // Task completed successfully
    FAILED,             // Task failed
    CANCELLED,          // Task was cancelled
    RETRYING            // Task is retrying after failure
};

// Network condition requirements
enum class NetworkRequirement {
    NONE,               // No network required
    ANY,                // Any network connection
    WIFI,               // WiFi connection required
    MOBILE,             // Mobile data allowed
    UNMETERED,          // Unmetered connection only
    METERED             // Metered connection allowed
};

// Battery condition requirements
enum class BatteryRequirement {
    NONE,               // No battery requirement
    ANY,                // Any battery level
    CHARGING,           // Device must be charging
    NOT_LOW,            // Battery not low
    ABOVE_20,           // Battery above 20%
    ABOVE_50            // Battery above 50%
};

// Task execution constraints
struct TaskConstraints {
    NetworkRequirement networkRequirement; // Network requirement
    BatteryRequirement batteryRequirement; // Battery requirement
    bool requiresCharging;      // Requires device charging
    bool requiresIdle;          // Requires device idle
    bool requiresWifi;          // Requires WiFi connection
    int minBatteryLevel;        // Minimum battery level (0-100)
    int maxExecutionTime;       // Maximum execution time in seconds
    std::vector<std::string> requiredFeatures; // Required device features
};

// Download task configuration
struct DownloadConfig {
    std::string url;            // Download URL
    std::string destinationPath; // Local destination path
    std::string tempPath;       // Temporary file path
    size_t chunkSize;           // Download chunk size
    int maxRetries;             // Maximum retry attempts
    int timeoutSeconds;         // Request timeout
    bool resumeSupported;       // Whether resume is supported
    bool verifyIntegrity;       // Verify file integrity
    std::string expectedHash;   // Expected file hash
    std::unordered_map<std::string, std::string> headers; // HTTP headers
};

// Upload task configuration
struct UploadConfig {
    std::string url;            // Upload URL
    std::string filePath;       // Local file path
    std::string uploadName;     // Upload field name
    int maxRetries;             // Maximum retry attempts
    int timeoutSeconds;         // Request timeout
    bool compress;              // Compress before upload
    std::string contentType;    // Content type
    std::unordered_map<std::string, std::string> headers; // HTTP headers
    std::unordered_map<std::string, std::string> formData; // Form data
};

// Task progress information
struct TaskProgress {
    size_t bytesTransferred;    // Bytes transferred so far
    size_t totalBytes;          // Total bytes to transfer
    float progress;             // Progress percentage (0.0 - 1.0)
    float speed;                // Transfer speed in bytes/second
    int etaSeconds;             // Estimated time remaining
    std::string currentFile;    // Currently processing file
    std::string statusMessage;  // Current status message
    std::chrono::steady_clock::time_point lastUpdate;
};

// Task result information
struct TaskResult {
    bool success;               // Whether task completed successfully
    std::string errorMessage;   // Error message if failed
    int errorCode;              // Error code
    size_t bytesTransferred;    // Total bytes transferred
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    std::chrono::duration<double> duration; // Task duration
    std::unordered_map<std::string, std::string> metadata; // Task metadata
};

// Background task definition
struct BackgroundTask {
    std::string id;             // Unique task identifier
    TaskType type;              // Task type
    TaskPriority priority;      // Task priority
    TaskState state;            // Current state
    TaskConstraints constraints; // Execution constraints
    std::chrono::steady_clock::time_point createdTime;
    std::chrono::steady_clock::time_point scheduledTime;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    int retryCount;             // Number of retries attempted
    int maxRetries;             // Maximum allowed retries
    bool persistent;            // Whether task survives app restart
    bool requiresNetwork;       // Whether task requires network
    std::string groupId;        // Task group identifier
    std::string description;    // Human-readable description

    // Task-specific data
    union {
        DownloadConfig downloadConfig;
        UploadConfig uploadConfig;
    } config;

    TaskProgress progress;      // Current progress
    TaskResult result;          // Final result
    std::unordered_map<std::string, std::string> metadata; // Additional metadata
};

// Task group for batch operations
struct TaskGroup {
    std::string id;             // Group identifier
    std::string name;           // Group name
    std::vector<std::string> taskIds; // Task IDs in group
    bool parallelExecution;     // Whether tasks can run in parallel
    int maxConcurrentTasks;     // Maximum concurrent tasks
    TaskState state;            // Group state
    std::chrono::steady_clock::time_point createdTime;
    std::chrono::steady_clock::time_point completedTime;
    std::unordered_map<std::string, std::string> metadata;
};

// Network status information
struct NetworkStatus {
    bool isConnected;           // Whether network is connected
    bool isWifi;                // Whether connected to WiFi
    bool isMobile;              // Whether connected to mobile data
    bool isMetered;             // Whether connection is metered
    int signalStrength;         // Signal strength (0-100)
    long bandwidthDown;         // Download bandwidth in bps
    long bandwidthUp;           // Upload bandwidth in bps
    std::string networkType;    // Network type (WiFi, LTE, etc.)
    std::chrono::steady_clock::time_point lastUpdate;
};

// Battery status information
struct BatteryStatus {
    int level;                  // Battery level (0-100)
    bool isCharging;            // Whether device is charging
    bool isLow;                 // Whether battery is low
    int temperature;            // Battery temperature in Celsius
    float voltage;              // Battery voltage
    std::string status;         // Charging status
    std::chrono::steady_clock::time_point lastUpdate;
};

// Device status information
struct DeviceStatus {
    bool isIdle;                // Whether device is idle
    bool screenOn;              // Whether screen is on
    bool powerSaveMode;         // Whether in power save mode
    int thermalStatus;          // Thermal status level
    NetworkStatus network;      // Network status
    BatteryStatus battery;      // Battery status
    std::chrono::steady_clock::time_point lastUpdate;
};

// Task scheduling configuration
struct SchedulingConfig {
    bool enableScheduling;      // Enable task scheduling
    int maxConcurrentTasks;     // Maximum concurrent tasks
    int maxTasksPerHour;        // Rate limiting
    bool respectBattery;        // Respect battery constraints
    bool respectNetwork;        // Respect network constraints
    bool respectThermal;        // Respect thermal constraints
    int retryDelaySeconds;      // Delay between retries
    int maxRetryDelaySeconds;   // Maximum retry delay
    bool exponentialBackoff;    // Use exponential backoff
    std::vector<std::string> preferredNetworkTypes; // Preferred networks
};

// Task manager settings
struct TaskManagerSettings {
    bool enabled;               // Whether background tasks are enabled
    bool autoStart;             // Auto-start on app launch
    bool persistentTasks;       // Persist tasks across app restarts
    int maxActiveTasks;         // Maximum active tasks
    int maxPendingTasks;        // Maximum pending tasks
    int maxTaskHistory;         // Maximum task history to keep
    bool enableNotifications;   // Enable task notifications
    bool enableAnalytics;       // Enable task analytics
    bool enableCompression;     // Enable data compression
    int defaultTimeout;         // Default task timeout
    std::string storagePath;    // Task storage path
    SchedulingConfig scheduling; // Scheduling configuration
};

// Callback types
using TaskProgressCallback = std::function<void(const std::string&, const TaskProgress&)>;
using TaskCompletedCallback = std::function<void(const std::string&, const TaskResult&)>;
using TaskFailedCallback = std::function<void(const std::string&, const std::string&)>;
using TaskStateChangedCallback = std::function<void(const std::string&, TaskState)>;
using NetworkStatusCallback = std::function<void(const NetworkStatus&)>;
using BatteryStatusCallback = std::function<void(const BatteryStatus&)>;

// ========== BACKGROUND TASK MANAGER ==========
class BackgroundTaskManager : public System {
private:
    static BackgroundTaskManager* instance_;

    DownloadManager* downloadManager_;
    UploadManager* uploadManager_;
    TaskScheduler* taskScheduler_;
    NetworkManager* networkManager_;
    BatteryManager* batteryManager_;
    ConnectivityManager* connectivityManager_;

    // JNI environment
    JNIEnv* env_;
    jobject context_;

    // Task management state
    std::atomic<bool> initialized_;
    std::atomic<bool> taskManagementActive_;
    TaskManagerSettings settings_;
    std::unordered_map<std::string, BackgroundTask> activeTasks_;
    std::unordered_map<std::string, TaskGroup> taskGroups_;
    std::vector<BackgroundTask> taskHistory_;
    std::queue<std::string> pendingTasks_;
    std::mutex taskMutex_;

    // Device monitoring
    DeviceStatus deviceStatus_;
    std::atomic<bool> monitoringActive_;
    std::chrono::steady_clock::time_point lastDeviceCheck_;

    // Event system
    std::unordered_map<std::string, TaskProgressCallback> progressCallbacks_;
    std::unordered_map<std::string, TaskCompletedCallback> completedCallbacks_;
    std::unordered_map<std::string, TaskFailedCallback> failedCallbacks_;
    std::unordered_map<std::string, TaskStateChangedCallback> stateChangedCallbacks_;
    std::unordered_map<std::string, NetworkStatusCallback> networkCallbacks_;
    std::unordered_map<std::string, BatteryStatusCallback> batteryCallbacks_;

    // Service management
    std::atomic<bool> serviceRunning_;
    std::thread taskThread_;
    std::thread monitorThread_;
    std::thread schedulerThread_;

    // Settings
    bool adaptiveScheduling_;
    int consecutiveFailures_;

public:
    BackgroundTaskManager();
    ~BackgroundTaskManager();

    static BackgroundTaskManager* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // JNI setup
    void setJNIEnvironment(JNIEnv* env, jobject context);

    // Task management settings
    void setSettings(const TaskManagerSettings& settings);
    TaskManagerSettings getSettings() const { return settings_; }
    void setMaxActiveTasks(int maxTasks);
    void setMaxPendingTasks(int maxTasks);
    void enablePersistence(bool enable);
    void enableNotifications(bool enable);
    void enableAnalytics(bool enable);

    // Task creation and management
    std::string createDownloadTask(const DownloadConfig& config, TaskPriority priority = TaskPriority::NORMAL);
    std::string createUploadTask(const UploadConfig& config, TaskPriority priority = TaskPriority::NORMAL);
    std::string createSyncTask(const std::string& dataId, TaskPriority priority = TaskPriority::NORMAL);
    std::string createBackupTask(const std::string& dataPath, TaskPriority priority = TaskPriority::NORMAL);
    std::string createCustomTask(TaskType type, const std::unordered_map<std::string, std::string>& params,
                               TaskPriority priority = TaskPriority::NORMAL);

    // Task control
    bool startTask(const std::string& taskId);
    bool pauseTask(const std::string& taskId);
    bool resumeTask(const std::string& taskId);
    bool cancelTask(const std::string& taskId);
    bool retryTask(const std::string& taskId);
    bool removeTask(const std::string& taskId);

    // Task information
    BackgroundTask getTask(const std::string& taskId) const;
    std::vector<BackgroundTask> getActiveTasks() const;
    std::vector<BackgroundTask> getPendingTasks() const;
    std::vector<BackgroundTask> getCompletedTasks() const;
    std::vector<BackgroundTask> getFailedTasks() const;
    TaskState getTaskState(const std::string& taskId) const;
    TaskProgress getTaskProgress(const std::string& taskId) const;

    // Task groups
    std::string createTaskGroup(const std::string& name, bool parallelExecution = false);
    bool addTaskToGroup(const std::string& groupId, const std::string& taskId);
    bool removeTaskFromGroup(const std::string& groupId, const std::string& taskId);
    bool startTaskGroup(const std::string& groupId);
    bool cancelTaskGroup(const std::string& groupId);
    TaskGroup getTaskGroup(const std::string& groupId) const;
    std::vector<TaskGroup> getAllTaskGroups() const;

    // Scheduling and constraints
    void setTaskConstraints(const std::string& taskId, const TaskConstraints& constraints);
    void setTaskPriority(const std::string& taskId, TaskPriority priority);
    void setTaskNetworkRequirement(const std::string& taskId, NetworkRequirement requirement);
    void setTaskBatteryRequirement(const std::string& taskId, BatteryRequirement requirement);
    bool canExecuteTask(const std::string& taskId) const;
    bool canExecuteTask(const BackgroundTask& task) const;

    // Device status monitoring
    DeviceStatus getDeviceStatus() const { return deviceStatus_; }
    NetworkStatus getNetworkStatus() const { return deviceStatus_.network; }
    BatteryStatus getBatteryStatus() const { return deviceStatus_.battery; }
    bool isDeviceIdle() const { return deviceStatus_.isIdle; }
    bool isNetworkAvailable() const { return deviceStatus_.network.isConnected; }
    bool isBatteryCharging() const { return deviceStatus_.battery.isCharging; }
    int getBatteryLevel() const { return deviceStatus_.battery.level; }

    // Advanced features
    void enableAdaptiveScheduling(bool enable);
    void setSchedulingConfig(const SchedulingConfig& config);
    void enableTaskPersistence(bool enable);
    void setRetryPolicy(int maxRetries, int retryDelay);
    void enableCompression(bool enable);
    void setBandwidthLimits(long downloadLimit, long uploadLimit);

    // Task history and analytics
    std::vector<BackgroundTask> getTaskHistory() const { return taskHistory_; }
    void clearTaskHistory();
    int getTaskCount(TaskState state) const;
    std::vector<std::string> getRecentTaskIds(int count) const;
    std::unordered_map<TaskType, int> getTaskTypeStatistics() const;

    // Callback management
    void registerTaskProgressCallback(const std::string& id, TaskProgressCallback callback);
    void unregisterTaskProgressCallback(const std::string& id);
    void registerTaskCompletedCallback(const std::string& id, TaskCompletedCallback callback);
    void unregisterTaskCompletedCallback(const std::string& id);
    void registerTaskFailedCallback(const std::string& id, TaskFailedCallback callback);
    void unregisterTaskFailedCallback(const std::string& id);
    void registerTaskStateChangedCallback(const std::string& id, TaskStateChangedCallback callback);
    void unregisterTaskStateChangedCallback(const std::string& id);
    void registerNetworkStatusCallback(const std::string& id, NetworkStatusCallback callback);
    void unregisterNetworkStatusCallback(const std::string& id);
    void registerBatteryStatusCallback(const std::string& id, BatteryStatusCallback callback);
    void unregisterBatteryStatusCallback(const std::string& id);

    // Utility functions
    bool isTaskManagementActive() const { return taskManagementActive_; }
    std::string getTaskStatusString() const;
    std::string getDeviceStatusString() const;
    void resetAllTasks();
    void testTaskSystem();

    // Emergency controls
    void pauseAllTasks();
    void resumeAllTasks();
    void cancelAllTasks();
    bool isSystemBusy() const;

    // Performance optimization
    void setMaxProcessingTime(float maxTime);
    void enableParallelProcessing(bool enable);
    void setThreadCount(int threads);

private:
    void initializeDefaults();
    void detectDeviceCapabilities();
    void startServiceThreads();
    void stopServiceThreads();
    void taskThreadLoop();
    void monitorThreadLoop();
    void schedulerThreadLoop();

    // JNI helper methods
    void initializeJNI();
    void updateDeviceStatusJNI();
    void executeTaskJNI(const std::string& taskId);
    void cancelTaskJNI(const std::string& taskId);

    // Task processing
    void onTaskProgress(const std::string& taskId, const TaskProgress& progress);
    void onTaskCompleted(const std::string& taskId, const TaskResult& result);
    void onTaskFailed(const std::string& taskId, const std::string& error);
    void onTaskStateChanged(const std::string& taskId, TaskState state);
    void onNetworkStatusChanged(const NetworkStatus& status);
    void onBatteryStatusChanged(const BatteryStatus& status);

    // Task execution
    bool executeTask(BackgroundTask& task);
    bool executeDownloadTask(BackgroundTask& task);
    bool executeUploadTask(BackgroundTask& task);
    bool executeSyncTask(BackgroundTask& task);
    bool executeBackupTask(BackgroundTask& task);
    bool executeCustomTask(BackgroundTask& task);

    // Task scheduling
    void scheduleTask(const std::string& taskId);
    void scheduleTaskGroup(const std::string& groupId);
    void processTaskQueue();
    void updateTaskStates();
    void handleTaskTimeouts();
    void retryFailedTasks();

    // Device monitoring
    void updateDeviceStatus();
    void checkNetworkConditions();
    void checkBatteryConditions();
    void checkThermalConditions();
    bool areConstraintsSatisfied(const TaskConstraints& constraints) const;

    // Utility functions
    std::string generateTaskId() const;
    bool isValidTaskId(const std::string& taskId) const;
    void validateTask(const BackgroundTask& task) const;
    void updateTaskProgress(const std::string& taskId, const TaskProgress& progress);
    void updateTaskHistory(const BackgroundTask& task);
    void cleanupOldTasks();
    void persistTaskData();
    void loadPersistedTasks();
    void applySchedulingConfig();
    void calculateOptimalExecution();
};

// ========== DOWNLOAD MANAGER ==========
class DownloadManager {
private:
    BackgroundTaskManager* manager_;

    // Download state
    std::unordered_map<std::string, size_t> downloadProgress_;
    std::unordered_map<std::string, std::string> tempFiles_;
    std::mutex downloadMutex_;

    // Download settings
    size_t defaultChunkSize_;
    int defaultTimeout_;
    int maxConcurrentDownloads_;
    bool resumeEnabled_;
    bool integrityCheckEnabled_;

public:
    DownloadManager(BackgroundTaskManager* manager);
    ~DownloadManager();

    bool initialize();
    void shutdown();

    // Download control
    bool startDownload(const std::string& taskId, const DownloadConfig& config);
    bool pauseDownload(const std::string& taskId);
    bool resumeDownload(const std::string& taskId);
    bool cancelDownload(const std::string& taskId);
    TaskProgress getDownloadProgress(const std::string& taskId) const;

    // Download settings
    void setChunkSize(size_t chunkSize);
    void setTimeout(int timeout);
    void setMaxConcurrentDownloads(int maxDownloads);
    void enableResume(bool enable);
    void enableIntegrityCheck(bool enable);

    // Advanced download features
    bool supportsResume(const std::string& url) const;
    size_t getFileSize(const std::string& url) const;
    bool verifyFileIntegrity(const std::string& filePath, const std::string& expectedHash) const;
    void optimizeDownload(const std::string& url, DownloadConfig& config);

private:
    bool downloadFile(const std::string& taskId, const DownloadConfig& config);
    bool downloadChunk(const std::string& url, size_t offset, size_t size, const std::string& tempPath);
    bool mergeChunks(const std::string& tempPath, const std::string& destPath, size_t totalSize);
    void updateProgress(const std::string& taskId, size_t bytesTransferred, size_t totalBytes);
    std::string calculateFileHash(const std::string& filePath) const;
};

// ========== UPLOAD MANAGER ==========
class UploadManager {
private:
    BackgroundTaskManager* manager_;

    // Upload state
    std::unordered_map<std::string, size_t> uploadProgress_;
    std::mutex uploadMutex_;

    // Upload settings
    int defaultTimeout_;
    int maxConcurrentUploads_;
    bool compressionEnabled_;
    size_t maxFileSize_;

public:
    UploadManager(BackgroundTaskManager* manager);
    ~UploadManager();

    bool initialize();
    void shutdown();

    // Upload control
    bool startUpload(const std::string& taskId, const UploadConfig& config);
    bool pauseUpload(const std::string& taskId);
    bool resumeUpload(const std::string& taskId);
    bool cancelUpload(const std::string& taskId);
    TaskProgress getUploadProgress(const std::string& taskId) const;

    // Upload settings
    void setTimeout(int timeout);
    void setMaxConcurrentUploads(int maxUploads);
    void enableCompression(bool enable);
    void setMaxFileSize(size_t maxSize);

    // Advanced upload features
    bool compressFile(const std::string& inputPath, const std::string& outputPath);
    size_t getCompressedSize(const std::string& filePath) const;
    bool prepareMultipartData(const UploadConfig& config, std::string& multipartData);

private:
    bool uploadFile(const std::string& taskId, const UploadConfig& config);
    bool uploadChunk(const std::string& url, const std::string& data, const std::string& contentType);
    void updateProgress(const std::string& taskId, size_t bytesTransferred, size_t totalBytes);
    std::string createMultipartBoundary() const;
};

// ========== TASK SCHEDULER ==========
class TaskScheduler {
private:
    BackgroundTaskManager* manager_;

    // Scheduling state
    std::priority_queue<std::string> scheduledTasks_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> taskSchedules_;
    std::mutex schedulerMutex_;

    // Scheduling settings
    SchedulingConfig config_;
    int maxConcurrentTasks_;
    bool adaptiveScheduling_;

public:
    TaskScheduler(BackgroundTaskManager* manager);
    ~TaskScheduler();

    bool initialize();
    void shutdown();

    // Task scheduling
    void scheduleTask(const std::string& taskId, const std::chrono::steady_clock::time_point& scheduleTime);
    void scheduleTaskDelayed(const std::string& taskId, int delaySeconds);
    void scheduleTaskRecurring(const std::string& taskId, int intervalSeconds);
    void unscheduleTask(const std::string& taskId);
    void processScheduledTasks();

    // Scheduling settings
    void setSchedulingConfig(const SchedulingConfig& config);
    void setMaxConcurrentTasks(int maxTasks);
    void enableAdaptiveScheduling(bool enable);

    // Advanced scheduling
    bool canScheduleTask(const std::string& taskId) const;
    std::vector<std::string> getScheduledTasks() const;
    std::chrono::steady_clock::time_point getNextScheduledTime() const;
    void optimizeSchedule();

private:
    void checkTaskConstraints(const std::string& taskId);
    bool areResourcesAvailable(const std::string& taskId) const;
    void adjustScheduleForConstraints();
    void handleScheduleConflicts();
};

// ========== NETWORK MANAGER ==========
class NetworkManager {
private:
    BackgroundTaskManager* manager_;

    // Network state
    NetworkStatus currentStatus_;
    std::unordered_map<std::string, long> bandwidthHistory_;
    std::mutex networkMutex_;

    // Network settings
    long maxBandwidthDown_;
    long maxBandwidthUp_;
    bool bandwidthLimiting_;
    int connectionTimeout_;

public:
    NetworkManager(BackgroundTaskManager* manager);
    ~NetworkManager();

    bool initialize();
    void shutdown();

    // Network monitoring
    void updateNetworkStatus();
    NetworkStatus getNetworkStatus() const { return currentStatus_; }
    bool isNetworkAvailable() const { return currentStatus_.isConnected; }
    bool isWifiAvailable() const { return currentStatus_.isWifi; }
    long getAvailableBandwidth() const;

    // Network control
    void setBandwidthLimits(long downloadLimit, long uploadLimit);
    void enableBandwidthLimiting(bool enable);
    void setConnectionTimeout(int timeout);

    // Advanced networking
    bool testConnection(const std::string& url) const;
    long measureBandwidth(const std::string& url) const;
    std::vector<std::string> getAvailableNetworks() const;
    bool switchToNetwork(const std::string& networkType);

private:
    void detectNetworkChanges();
    void updateBandwidthMeasurements();
    void calculateOptimalBandwidth();
    bool isNetworkSuitable(const NetworkRequirement& requirement) const;
};

// ========== BATTERY MANAGER ==========
class BatteryManager {
private:
    BackgroundTaskManager* manager_;

    // Battery state
    BatteryStatus currentStatus_;
    std::vector<int> batteryHistory_;
    std::mutex batteryMutex_;

    // Battery settings
    int lowBatteryThreshold_;
    int criticalBatteryThreshold_;
    bool powerSaveMode_;

public:
    BatteryManager(BackgroundTaskManager* manager);
    ~BatteryManager();

    bool initialize();
    void shutdown();

    // Battery monitoring
    void updateBatteryStatus();
    BatteryStatus getBatteryStatus() const { return currentStatus_; }
    int getBatteryLevel() const { return currentStatus_.level; }
    bool isBatteryCharging() const { return currentStatus_.isCharging; }
    bool isBatteryLow() const { return currentStatus_.isLow; }

    // Battery settings
    void setLowBatteryThreshold(int threshold);
    void setCriticalBatteryThreshold(int threshold);
    void enablePowerSaveMode(bool enable);

    // Advanced battery management
    int getEstimatedTimeRemaining() const;
    float getBatteryTemperature() const;
    bool isBatteryOptimizationEnabled() const;
    void optimizeForBatteryLife();

private:
    void detectBatteryChanges();
    void updateBatteryHistory();
    void predictBatteryDrain();
    bool shouldThrottleTasks() const;
};

// ========== CONNECTIVITY MANAGER ==========
class ConnectivityManager {
private:
    BackgroundTaskManager* manager_;

    // Connectivity state
    DeviceStatus deviceStatus_;
    std::unordered_map<std::string, bool> featureAvailability_;
    std::mutex connectivityMutex_;

    // Connectivity settings
    bool autoSwitchNetworks_;
    int networkSwitchDelay_;
    std::vector<std::string> preferredNetworks_;

public:
    ConnectivityManager(BackgroundTaskManager* manager);
    ~ConnectivityManager();

    bool initialize();
    void shutdown();

    // Connectivity monitoring
    void updateConnectivityStatus();
    DeviceStatus getDeviceStatus() const { return deviceStatus_; }
    bool isDeviceIdle() const { return deviceStatus_.isIdle; }
    bool isScreenOn() const { return deviceStatus_.screenOn; }

    // Connectivity control
    void setAutoSwitchNetworks(bool enable);
    void setNetworkSwitchDelay(int delay);
    void addPreferredNetwork(const std::string& networkType);
    void removePreferredNetwork(const std::string& networkType);

    // Advanced connectivity
    bool isFeatureAvailable(const std::string& feature) const;
    std::vector<std::string> getAvailableFeatures() const;
    bool requestFeature(const std::string& feature);
    void releaseFeature(const std::string& feature);

private:
    void detectConnectivityChanges();
    void updateFeatureAvailability();
    void handleNetworkTransitions();
    void optimizeConnectivity();
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // Task management callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onTaskProgress(
        JNIEnv* env, jobject thiz, jstring taskId, jlong bytesTransferred, jlong totalBytes, jfloat progress);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onTaskCompleted(
        JNIEnv* env, jobject thiz, jstring taskId, jboolean success, jstring resultJson);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onTaskFailed(
        JNIEnv* env, jobject thiz, jstring taskId, jstring errorMessage, jint errorCode);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onTaskStateChanged(
        JNIEnv* env, jobject thiz, jstring taskId, jstring state);

    // Device status callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onNetworkStatusChanged(
        JNIEnv* env, jobject thiz, jboolean isConnected, jboolean isWifi, jboolean isMetered, jint signalStrength);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onBatteryStatusChanged(
        JNIEnv* env, jobject thiz, jint level, jboolean isCharging, jboolean isLow, jfloat temperature);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onDeviceStatusChanged(
        JNIEnv* env, jobject thiz, jboolean isIdle, jboolean screenOn, jboolean powerSaveMode);

    // Task scheduling callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onTaskScheduled(
        JNIEnv* env, jobject thiz, jstring taskId, jlong scheduledTime);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onTaskGroupCompleted(
        JNIEnv* env, jobject thiz, jstring groupId, jboolean success);

    // Download/Upload callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onDownloadProgress(
        JNIEnv* env, jobject thiz, jstring taskId, jlong bytesDownloaded, jlong totalBytes, jfloat speed);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onUploadProgress(
        JNIEnv* env, jobject thiz, jstring taskId, jlong bytesUploaded, jlong totalBytes, jfloat speed);

    // System events
    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onAppBackgrounded(
        JNIEnv* env, jobject thiz);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onAppForegrounded(
        JNIEnv* env, jobject thiz);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onMemoryWarning(
        JNIEnv* env, jobject thiz);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_BackgroundTaskManager_onThermalWarning(
        JNIEnv* env, jobject thiz, jstring warning);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_BACKGROUND_TASK_MANAGER_H
