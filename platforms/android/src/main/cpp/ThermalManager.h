#ifndef FOUNDRYENGINE_THERMAL_MANAGER_H
#define FOUNDRYENGINE_THERMAL_MANAGER_H

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
class ThermalManager;
class TemperatureMonitor;
class PerformanceGovernor;
class ThermalPolicyManager;
class AdaptiveQualityManager;

// Thermal status levels
enum class ThermalStatus {
    NONE,           // No thermal data available
    LIGHT,          // Light thermal load
    MODERATE,       // Moderate thermal load
    SEVERE,         // Severe thermal load
    CRITICAL,       // Critical thermal load
    EMERGENCY,      // Emergency thermal load
    SHUTDOWN        // Device shutdown imminent
};

// Thermal mitigation actions
enum class ThermalAction {
    NONE,           // No action needed
    REDUCE_CPU,     // Reduce CPU frequency
    REDUCE_GPU,     // Reduce GPU frequency
    LIMIT_FPS,      // Limit frame rate
    REDUCE_QUALITY, // Reduce rendering quality
    DISABLE_FEATURES, // Disable non-essential features
    PAUSE_GAME,     // Pause game execution
    SHUTDOWN_APP    // Shutdown application
};

// Device thermal zones
enum class ThermalZone {
    CPU,            // CPU thermal zone
    GPU,            // GPU thermal zone
    BATTERY,        // Battery thermal zone
    SKIN,           // Device skin temperature
    AMBIENT,        // Ambient temperature
    CAMERA,         // Camera thermal zone
    MODEM,          // Modem thermal zone
    DISPLAY,        // Display thermal zone
    CUSTOM_1,       // Custom thermal zone 1
    CUSTOM_2        // Custom thermal zone 2
};

// Performance levels
enum class PerformanceLevel {
    MINIMUM,        // Minimum performance (thermal safety)
    LOW,            // Low performance
    MEDIUM,         // Medium performance
    HIGH,           // High performance
    MAXIMUM,        // Maximum performance
    CUSTOM          // Custom performance level
};

// Quality levels
enum class QualityLevel {
    LOWEST,         // Lowest quality
    LOW,            // Low quality
    MEDIUM,         // Medium quality
    HIGH,           // High quality
    HIGHEST,        // Highest quality
    ULTRA,          // Ultra quality
    CUSTOM          // Custom quality level
};

// Thermal sensor data
struct ThermalSensor {
    ThermalZone zone;       // Thermal zone
    float temperature;      // Temperature in Celsius
    float threshold;        // Temperature threshold
    bool isValid;           // Whether sensor data is valid
    std::string name;       // Sensor name
    std::string path;       // Sensor file path
    std::chrono::steady_clock::time_point lastUpdate;
};

// Thermal status data
struct ThermalStatusData {
    ThermalStatus status;   // Overall thermal status
    float cpuTemperature;   // CPU temperature
    float gpuTemperature;   // GPU temperature
    float batteryTemperature; // Battery temperature
    float skinTemperature;  // Skin temperature
    float ambientTemperature; // Ambient temperature
    std::unordered_map<ThermalZone, float> zoneTemperatures; // All zone temperatures
    std::chrono::steady_clock::time_point timestamp;
    bool isThrottling;      // Whether device is throttling
    float thermalHeadroom;  // Available thermal headroom (0.0 - 1.0)
};

// Performance metrics
struct PerformanceMetrics {
    float cpuUsage;         // CPU usage percentage (0.0 - 100.0)
    float gpuUsage;         // GPU usage percentage (0.0 - 100.0)
    float memoryUsage;      // Memory usage percentage (0.0 - 100.0)
    float batteryLevel;     // Battery level percentage (0.0 - 100.0)
    float fps;              // Current FPS
    float targetFps;        // Target FPS
    int frameTime;          // Frame time in milliseconds
    int cpuFrequency;       // Current CPU frequency in MHz
    int gpuFrequency;       // Current GPU frequency in MHz
    std::chrono::steady_clock::time_point timestamp;
};

// Thermal policy configuration
struct ThermalPolicy {
    ThermalStatus triggerStatus; // Status that triggers this policy
    ThermalAction action;       // Action to take
    float temperatureThreshold; // Temperature threshold
    float durationThreshold;    // How long status must persist
    bool enabled;               // Whether policy is enabled
    std::string name;           // Policy name
    int priority;               // Policy priority (higher = more important)
    std::unordered_map<std::string, float> parameters; // Policy parameters
};

// Quality adaptation settings
struct QualitySettings {
    QualityLevel level;         // Current quality level
    int textureQuality;         // Texture quality (0-100)
    int shadowQuality;          // Shadow quality (0-100)
    int particleQuality;        // Particle quality (0-100)
    int lightingQuality;        // Lighting quality (0-100)
    int postProcessQuality;     // Post-processing quality (0-100)
    int geometryQuality;        // Geometry quality (0-100)
    bool enableBloom;           // Enable bloom effects
    bool enableMotionBlur;      // Enable motion blur
    bool enableDepthOfField;    // Enable depth of field
    bool enableSSAO;            // Enable screen space ambient occlusion
    int maxTextureSize;         // Maximum texture size
    int maxShadowMapSize;       // Maximum shadow map size
};

// Performance adaptation settings
struct PerformanceSettings {
    PerformanceLevel level;     // Current performance level
    int targetFps;              // Target frame rate
    int maxFrameTime;           // Maximum frame time in ms
    float cpuFrequencyScale;    // CPU frequency scale (0.0 - 1.0)
    float gpuFrequencyScale;    // GPU frequency scale (0.0 - 1.0)
    bool enableVSync;           // Enable VSync
    bool enableMultithreading;  // Enable multithreading
    int threadCount;            // Number of threads
    bool enableOptimizations;   // Enable performance optimizations
    std::unordered_map<std::string, bool> featureFlags; // Feature enable/disable flags
};

// Thermal mitigation history
struct MitigationEvent {
    ThermalAction action;       // Action taken
    ThermalStatus triggerStatus; // Status that triggered action
    float temperature;          // Temperature when action was taken
    std::chrono::steady_clock::time_point timestamp;
    std::string reason;         // Reason for mitigation
    bool wasAutomatic;          // Whether action was automatic
};

// Thermal manager settings
struct ThermalSettings {
    bool enabled;               // Whether thermal management is enabled
    bool autoAdaptation;        // Enable automatic adaptation
    bool aggressiveThrottling;  // Use aggressive throttling
    float monitoringInterval;   // Temperature monitoring interval (seconds)
    float adaptationInterval;   // Adaptation check interval (seconds)
    float cooldownPeriod;       // Cooldown period before increasing performance
    bool enableLogging;         // Enable thermal logging
    bool enableNotifications;   // Enable thermal notifications
    bool enableHapticFeedback;  // Enable haptic feedback for thermal events
    float criticalTemperature;  // Critical temperature threshold
    float emergencyTemperature; // Emergency temperature threshold
    int maxConsecutiveActions;  // Maximum consecutive mitigation actions
    std::vector<ThermalPolicy> policies; // Thermal policies
};

// Callback types
using ThermalStatusCallback = std::function<void(const ThermalStatusData&)>;
using ThermalActionCallback = std::function<void(ThermalAction, const std::string&)>;
using PerformanceChangedCallback = std::function<void(PerformanceLevel)>;
using QualityChangedCallback = std::function<void(QualityLevel)>;
using ThermalAlertCallback = std::function<void(ThermalStatus, const std::string&)>;

// ========== THERMAL MANAGER ==========
class ThermalManager : public System {
private:
    static ThermalManager* instance_;

    TemperatureMonitor* temperatureMonitor_;
    PerformanceGovernor* performanceGovernor_;
    ThermalPolicyManager* policyManager_;
    AdaptiveQualityManager* qualityManager_;

    // JNI environment
    JNIEnv* env_;
    jobject context_;

    // Thermal state
    std::atomic<bool> initialized_;
    std::atomic<bool> thermalManagementActive_;
    ThermalSettings settings_;
    ThermalStatusData currentStatus_;
    PerformanceMetrics currentMetrics_;

    // Performance state
    PerformanceSettings performanceSettings_;
    QualitySettings qualitySettings_;
    PerformanceLevel currentPerformanceLevel_;
    QualityLevel currentQualityLevel_;

    // Mitigation state
    std::vector<MitigationEvent> mitigationHistory_;
    std::unordered_map<ThermalAction, int> actionCounts_;
    std::chrono::steady_clock::time_point lastActionTime_;
    std::chrono::steady_clock::time_point lastCooldownTime_;
    std::mutex thermalMutex_;

    // Event system
    std::unordered_map<std::string, ThermalStatusCallback> statusCallbacks_;
    std::unordered_map<std::string, ThermalActionCallback> actionCallbacks_;
    std::unordered_map<std::string, PerformanceChangedCallback> performanceCallbacks_;
    std::unordered_map<std::string, QualityChangedCallback> qualityCallbacks_;
    std::unordered_map<std::string, ThermalAlertCallback> alertCallbacks_;

    // Service management
    std::atomic<bool> serviceRunning_;
    std::thread monitorThread_;
    std::thread adaptationThread_;
    std::thread policyThread_;

    // Settings
    bool adaptivePolicies_;
    float temperatureScale_;
    int consecutiveFailures_;

public:
    ThermalManager();
    ~ThermalManager();

    static ThermalManager* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // JNI setup
    void setJNIEnvironment(JNIEnv* env, jobject context);

    // Thermal monitoring
    ThermalStatusData getCurrentThermalStatus() const { return currentStatus_; }
    float getCpuTemperature() const { return currentStatus_.cpuTemperature; }
    float getGpuTemperature() const { return currentStatus_.gpuTemperature; }
    float getBatteryTemperature() const { return currentStatus_.batteryTemperature; }
    float getSkinTemperature() const { return currentStatus_.skinTemperature; }
    ThermalStatus getThermalStatus() const { return currentStatus_.status; }
    bool isThrottling() const { return currentStatus_.isThrottling; }
    float getThermalHeadroom() const { return currentStatus_.thermalHeadroom; }

    // Performance monitoring
    PerformanceMetrics getCurrentPerformanceMetrics() const { return currentMetrics_; }
    float getCpuUsage() const { return currentMetrics_.cpuUsage; }
    float getGpuUsage() const { return currentMetrics_.gpuUsage; }
    float getMemoryUsage() const { return currentMetrics_.memoryUsage; }
    float getCurrentFps() const { return currentMetrics_.fps; }
    int getCpuFrequency() const { return currentMetrics_.cpuFrequency; }
    int getGpuFrequency() const { return currentMetrics_.gpuFrequency; }

    // Thermal settings
    void setSettings(const ThermalSettings& settings);
    ThermalSettings getSettings() const { return settings_; }
    void setMonitoringInterval(float interval);
    void setAdaptationInterval(float interval);
    void setCooldownPeriod(float period);
    void setCriticalTemperature(float temperature);
    void setEmergencyTemperature(float temperature);
    void enableAutoAdaptation(bool enable);
    void enableAggressiveThrottling(bool enable);
    void enableLogging(bool enable);
    void enableNotifications(bool enable);

    // Performance control
    void setPerformanceLevel(PerformanceLevel level);
    PerformanceLevel getPerformanceLevel() const { return currentPerformanceLevel_; }
    void setTargetFps(int fps);
    int getTargetFps() const { return performanceSettings_.targetFps; }
    void setMaxFrameTime(int frameTime);
    void enableVSync(bool enable);
    void setCpuFrequencyScale(float scale);
    void setGpuFrequencyScale(float scale);

    // Quality control
    void setQualityLevel(QualityLevel level);
    QualityLevel getQualityLevel() const { return currentQualityLevel_; }
    void setTextureQuality(int quality);
    void setShadowQuality(int quality);
    void setParticleQuality(int quality);
    void setLightingQuality(int quality);
    void setPostProcessQuality(int quality);
    void enableBloom(bool enable);
    void enableMotionBlur(bool enable);
    void enableDepthOfField(bool enable);
    void enableSSAO(bool enable);

    // Thermal policies
    void addThermalPolicy(const ThermalPolicy& policy);
    void removeThermalPolicy(const std::string& name);
    void enableThermalPolicy(const std::string& name, bool enable);
    bool isThermalPolicyEnabled(const std::string& name) const;
    std::vector<ThermalPolicy> getThermalPolicies() const;
    void clearThermalPolicies();

    // Mitigation control
    void triggerThermalAction(ThermalAction action, const std::string& reason = "");
    void revertThermalAction(ThermalAction action);
    void resetThermalState();
    std::vector<MitigationEvent> getMitigationHistory() const { return mitigationHistory_; }
    int getActionCount(ThermalAction action) const;

    // Advanced features
    void enableAdaptivePolicies(bool enable);
    void setTemperatureScale(float scale);
    void setMaxConsecutiveActions(int maxActions);
    void enableThermalPrediction(bool enable);
    void setPredictionWindow(int seconds);

    // Real-time adaptation
    void updatePerformanceMetrics(const PerformanceMetrics& metrics);
    void forceThermalCheck();
    bool isThermalEmergency() const;
    bool isPerformanceDegraded() const;

    // Callback management
    void registerThermalStatusCallback(const std::string& id, ThermalStatusCallback callback);
    void unregisterThermalStatusCallback(const std::string& id);
    void registerThermalActionCallback(const std::string& id, ThermalActionCallback callback);
    void unregisterThermalActionCallback(const std::string& id);
    void registerPerformanceChangedCallback(const std::string& id, PerformanceChangedCallback callback);
    void unregisterPerformanceChangedCallback(const std::string& id);
    void registerQualityChangedCallback(const std::string& id, QualityChangedCallback callback);
    void unregisterQualityChangedCallback(const std::string& id);
    void registerThermalAlertCallback(const std::string& id, ThermalAlertCallback callback);
    void unregisterThermalAlertCallback(const std::string& id);

    // Utility functions
    bool isThermalManagementActive() const { return thermalManagementActive_; }
    std::string getThermalStatusString() const;
    std::string getPerformanceStatusString() const;
    void resetAllSettings();
    void testThermalSystem();

    // Emergency controls
    void enableEmergencyMode();
    void disableEmergencyMode();
    bool isEmergencyMode() const;
    void setEmergencyAction(ThermalAction action);

    // Performance optimization
    void setMaxProcessingTime(float maxTime);
    void enableParallelProcessing(bool enable);
    void setThreadCount(int threads);

private:
    void initializeDefaults();
    void detectThermalCapabilities();
    void startServiceThreads();
    void stopServiceThreads();
    void monitorThreadLoop();
    void adaptationThreadLoop();
    void policyThreadLoop();

    // JNI helper methods
    void detectThermalZonesJNI();
    void readTemperatureSensorsJNI();
    void setCpuFrequencyJNI(int frequency);
    void setGpuFrequencyJNI(int frequency);
    void triggerThermalActionJNI(ThermalAction action);

    // Thermal processing
    void onThermalStatusChanged(const ThermalStatusData& status);
    void onThermalActionTriggered(ThermalAction action, const std::string& reason);
    void onPerformanceChanged(PerformanceLevel level);
    void onQualityChanged(QualityLevel level);
    void onThermalAlert(ThermalStatus status, const std::string& message);

    // Mitigation logic
    void evaluateThermalPolicies();
    void applyThermalAction(ThermalAction action, const std::string& reason);
    void revertLastAction();
    void updateMitigationHistory(ThermalAction action, const std::string& reason);
    void checkCooldownPeriod();

    // Performance adaptation
    void adaptPerformanceLevel();
    void adaptQualityLevel();
    void calculateOptimalSettings();
    void applyPerformanceSettings();
    void applyQualitySettings();

    // Utility functions
    bool isValidThermalStatus(const ThermalStatusData& status) const;
    ThermalStatus determineThermalStatus(float temperature) const;
    float calculateThermalHeadroom() const;
    void updatePerformanceMetrics();
    void validateSettings();
    void applyAdaptivePolicies();
    void predictThermalTrends();
    void updateActionCounts();
    void cleanupOldHistory();
};

// ========== TEMPERATURE MONITOR ==========
class TemperatureMonitor {
private:
    ThermalManager* manager_;

    // Temperature state
    std::vector<ThermalSensor> sensors_;
    std::unordered_map<ThermalZone, ThermalSensor> zoneSensors_;
    std::mutex sensorMutex_;

    // Monitoring settings
    float monitoringInterval_;
    bool predictionEnabled_;
    int predictionSamples_;
    float temperatureScale_;

public:
    TemperatureMonitor(ThermalManager* manager);
    ~TemperatureMonitor();

    bool initialize();
    void shutdown();

    // Temperature monitoring
    void updateTemperatures();
    std::vector<ThermalSensor> getAllSensors() const { return sensors_; }
    ThermalSensor getSensor(ThermalZone zone) const;
    float getTemperature(ThermalZone zone) const;
    bool isZoneAvailable(ThermalZone zone) const;

    // Settings
    void setMonitoringInterval(float interval);
    void setTemperatureScale(float scale);
    void enablePrediction(bool enable);
    void setPredictionSamples(int samples);

    // Advanced monitoring
    void addCustomSensor(const std::string& name, const std::string& path);
    void removeCustomSensor(const std::string& name);
    void calibrateSensors();
    void validateSensorData();

private:
    void readSensorData();
    void updateZoneTemperatures();
    void predictTemperatureTrends();
    bool isValidTemperature(float temperature, ThermalZone zone) const;
    void applyTemperatureSmoothing();
};

// ========== PERFORMANCE GOVERNOR ==========
class PerformanceGovernor {
private:
    ThermalManager* manager_;

    // Performance state
    PerformanceSettings settings_;
    PerformanceLevel currentLevel_;
    std::unordered_map<PerformanceLevel, PerformanceSettings> levelSettings_;
    std::mutex governorMutex_;

    // Governor settings
    bool autoAdaptation_;
    float adaptationThreshold_;
    int minFrameTime_;
    int maxFrameTime_;

public:
    PerformanceGovernor(ThermalManager* manager);
    ~PerformanceGovernor();

    bool initialize();
    void shutdown();

    // Performance control
    void setPerformanceLevel(PerformanceLevel level);
    PerformanceLevel getPerformanceLevel() const { return currentLevel_; }
    void setTargetFps(int fps);
    void setMaxFrameTime(int frameTime);
    void enableVSync(bool enable);
    void setCpuFrequencyScale(float scale);
    void setGpuFrequencyScale(float scale);

    // Performance adaptation
    void adaptToThermalConditions(ThermalStatus status);
    void adaptToPerformanceMetrics(const PerformanceMetrics& metrics);
    void optimizeForBatteryLevel(float batteryLevel);
    void optimizeForWorkload(int workload);

    // Settings management
    void definePerformanceLevel(PerformanceLevel level, const PerformanceSettings& settings);
    PerformanceSettings getPerformanceSettings(PerformanceLevel level) const;
    void resetToDefaults();

private:
    void applyPerformanceSettings();
    void calculateOptimalFrameRate();
    void adjustFrequencyScaling();
    void validatePerformanceSettings();
    bool isPerformanceLevelValid(PerformanceLevel level) const;
};

// ========== THERMAL POLICY MANAGER ==========
class ThermalPolicyManager {
private:
    ThermalManager* manager_;

    // Policy state
    std::vector<ThermalPolicy> policies_;
    std::unordered_map<std::string, ThermalPolicy> policyMap_;
    std::vector<MitigationEvent> activeMitigations_;
    std::mutex policyMutex_;

    // Policy settings
    bool adaptivePolicies_;
    int maxConsecutiveActions_;
    float policyTimeout_;

public:
    ThermalPolicyManager(ThermalManager* manager);
    ~ThermalPolicyManager();

    bool initialize();
    void shutdown();

    // Policy management
    void addPolicy(const ThermalPolicy& policy);
    void removePolicy(const std::string& name);
    void enablePolicy(const std::string& name, bool enable);
    bool isPolicyEnabled(const std::string& name) const;
    std::vector<ThermalPolicy> getPolicies() const { return policies_; }
    ThermalPolicy getPolicy(const std::string& name) const;

    // Policy evaluation
    void evaluatePolicies(const ThermalStatusData& status);
    std::vector<ThermalAction> getRecommendedActions(ThermalStatus status) const;
    bool shouldTriggerAction(const ThermalPolicy& policy, const ThermalStatusData& status) const;
    float calculatePolicyPriority(const ThermalPolicy& policy) const;

    // Mitigation tracking
    void recordMitigation(const MitigationEvent& event);
    void revertLastMitigation();
    std::vector<MitigationEvent> getActiveMitigations() const { return activeMitigations_; }

    // Advanced policy features
    void enableAdaptivePolicies(bool enable);
    void setMaxConsecutiveActions(int maxActions);
    void setPolicyTimeout(float timeout);
    void optimizePolicyOrder();

private:
    void sortPoliciesByPriority();
    void validatePolicy(const ThermalPolicy& policy) const;
    void applyPolicyDefaults(ThermalPolicy& policy);
    void cleanupExpiredMitigations();
    bool canExecuteAction(ThermalAction action) const;
};

// ========== ADAPTIVE QUALITY MANAGER ==========
class AdaptiveQualityManager {
private:
    ThermalManager* manager_;

    // Quality state
    QualitySettings settings_;
    QualityLevel currentLevel_;
    std::unordered_map<QualityLevel, QualitySettings> levelSettings_;
    std::mutex qualityMutex_;

    // Adaptation settings
    bool autoAdaptation_;
    float qualityScale_;
    int adaptationInterval_;
    std::vector<std::string> qualityFeatures_;

public:
    AdaptiveQualityManager(ThermalManager* manager);
    ~AdaptiveQualityManager();

    bool initialize();
    void shutdown();

    // Quality control
    void setQualityLevel(QualityLevel level);
    QualityLevel getQualityLevel() const { return currentLevel_; }
    void setTextureQuality(int quality);
    void setShadowQuality(int quality);
    void setParticleQuality(int quality);
    void setLightingQuality(int quality);
    void setPostProcessQuality(int quality);

    // Feature toggles
    void enableBloom(bool enable);
    void enableMotionBlur(bool enable);
    void enableDepthOfField(bool enable);
    void enableSSAO(bool enable);
    void setMaxTextureSize(int size);
    void setMaxShadowMapSize(int size);

    // Quality adaptation
    void adaptToThermalConditions(ThermalStatus status);
    void adaptToPerformanceLevel(PerformanceLevel performance);
    void adaptToAvailableMemory(float memoryMB);
    void optimizeForDeviceCapabilities();

    // Quality presets
    void defineQualityLevel(QualityLevel level, const QualitySettings& settings);
    QualitySettings getQualitySettings(QualityLevel level) const;
    void applyQualityPreset(const std::string& presetName);

    // Advanced features
    void enableAutoAdaptation(bool enable);
    void setQualityScale(float scale);
    void setAdaptationInterval(int seconds);
    void addQualityFeature(const std::string& feature);
    void removeQualityFeature(const std::string& feature);

private:
    void applyQualitySettings();
    void calculateOptimalQuality();
    void validateQualitySettings();
    bool isQualityLevelValid(QualityLevel level) const;
    void updateFeatureStates();
    void optimizeTextureMemory();
    void optimizeShaderComplexity();
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // Thermal monitoring callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onTemperatureChanged(
        JNIEnv* env, jobject thiz, jstring zone, jfloat temperature);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onThermalStatusChanged(
        JNIEnv* env, jobject thiz, jstring status, jfloat cpuTemp, jfloat gpuTemp, jfloat batteryTemp);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onThermalHeadroomChanged(
        JNIEnv* env, jobject thiz, jfloat headroom);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onThrottlingStateChanged(
        JNIEnv* env, jobject thiz, jboolean isThrottling);

    // Performance monitoring callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onPerformanceMetricsChanged(
        JNIEnv* env, jobject thiz, jfloat cpuUsage, jfloat gpuUsage, jfloat memoryUsage, jfloat fps);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onCpuFrequencyChanged(
        JNIEnv* env, jobject thiz, jint frequency);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onGpuFrequencyChanged(
        JNIEnv* env, jobject thiz, jint frequency);

    // Thermal action callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onThermalActionTriggered(
        JNIEnv* env, jobject thiz, jstring action, jstring reason);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onThermalActionReverted(
        JNIEnv* env, jobject thiz, jstring action);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onPerformanceLevelChanged(
        JNIEnv* env, jobject thiz, jstring level);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onQualityLevelChanged(
        JNIEnv* env, jobject thiz, jstring level);

    // Thermal alerts
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onThermalAlert(
        JNIEnv* env, jobject thiz, jstring status, jstring message);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onThermalEmergency(
        JNIEnv* env, jobject thiz, jstring message);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onThermalCooldown(
        JNIEnv* env, jobject thiz, jstring message);

    // Device capabilities
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onThermalCapabilitiesDetected(
        JNIEnv* env, jobject thiz, jstring capabilitiesJson);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ThermalManager_onDeviceThermalLimits(
        JNIEnv* env, jobject thiz, jfloat criticalTemp, jfloat emergencyTemp);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_THERMAL_MANAGER_H
