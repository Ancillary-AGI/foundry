#include "ThermalManager.h"
#include <android/log.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <random>
#include <nlohmann/json.hpp> // For JSON parsing

#define LOG_TAG "ThermalManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace FoundryEngine {

// Static instance
ThermalManager* ThermalManager::instance_ = nullptr;

// ========== THERMAL MANAGER IMPLEMENTATION ==========
ThermalManager::ThermalManager() : temperatureMonitor_(nullptr), performanceGovernor_(nullptr),
                                 policyManager_(nullptr), qualityManager_(nullptr),
                                 env_(nullptr), context_(nullptr), initialized_(false),
                                 thermalManagementActive_(false), serviceRunning_(false),
                                 adaptivePolicies_(true), temperatureScale_(1.0f), consecutiveFailures_(0) {
    LOGI("ThermalManager constructor called");
}

ThermalManager::~ThermalManager() {
    shutdown();
    LOGI("ThermalManager destructor called");
}

ThermalManager* ThermalManager::getInstance() {
    if (!instance_) {
        instance_ = new ThermalManager();
    }
    return instance_;
}

void ThermalManager::initialize() {
    LOGI("Initializing Thermal Manager");

    if (initialized_) {
        LOGW("Thermal Manager already initialized");
        return;
    }

    // Initialize sub-managers
    temperatureMonitor_ = new TemperatureMonitor(this);
    performanceGovernor_ = new PerformanceGovernor(this);
    policyManager_ = new ThermalPolicyManager(this);
    qualityManager_ = new AdaptiveQualityManager(this);

    // Initialize defaults
    initializeDefaults();

    // Start service threads
    startServiceThreads();

    // Detect thermal capabilities
    detectThermalCapabilities();

    // Initialize sub-managers
    if (temperatureMonitor_->initialize()) {
        LOGI("Temperature Monitor initialized successfully");
    } else {
        LOGE("Failed to initialize Temperature Monitor");
    }

    if (performanceGovernor_->initialize()) {
        LOGI("Performance Governor initialized successfully");
    } else {
        LOGE("Failed to initialize Performance Governor");
    }

    if (policyManager_->initialize()) {
        LOGI("Thermal Policy Manager initialized successfully");
    } else {
        LOGE("Failed to initialize Thermal Policy Manager");
    }

    if (qualityManager_->initialize()) {
        LOGI("Adaptive Quality Manager initialized successfully");
    } else {
        LOGE("Failed to initialize Adaptive Quality Manager");
    }

    initialized_ = true;
    LOGI("Thermal Manager initialized successfully");
}

void ThermalManager::update(float dt) {
    // Update thermal monitoring
    if (temperatureMonitor_) {
        temperatureMonitor_->updateTemperatures();
    }

    // Update performance metrics
    updatePerformanceMetrics();

    // Evaluate thermal policies
    if (policyManager_) {
        policyManager_->evaluatePolicies(currentStatus_);
    }

    // Adapt performance and quality
    if (settings_.autoAdaptation) {
        adaptPerformanceLevel();
        adaptQualityLevel();
    }

    // Check cooldown periods
    checkCooldownPeriod();

    // Update adaptive policies
    if (adaptivePolicies_) {
        applyAdaptivePolicies();
    }
}

void ThermalManager::shutdown() {
    LOGI("Shutting down Thermal Manager");

    if (!initialized_) {
        return;
    }

    // Stop service threads
    stopServiceThreads();

    // Shutdown sub-managers
    if (temperatureMonitor_) {
        temperatureMonitor_->shutdown();
        delete temperatureMonitor_;
        temperatureMonitor_ = nullptr;
    }

    if (performanceGovernor_) {
        performanceGovernor_->shutdown();
        delete performanceGovernor_;
        performanceGovernor_ = nullptr;
    }

    if (policyManager_) {
        policyManager_->shutdown();
        delete policyManager_;
        policyManager_ = nullptr;
    }

    if (qualityManager_) {
        qualityManager_->shutdown();
        delete qualityManager_;
        qualityManager_ = nullptr;
    }

    // Clear callbacks
    statusCallbacks_.clear();
    actionCallbacks_.clear();
    performanceCallbacks_.clear();
    qualityCallbacks_.clear();
    alertCallbacks_.clear();

    // Clear thermal data
    currentStatus_ = ThermalStatusData();
    currentMetrics_ = PerformanceMetrics();
    mitigationHistory_.clear();
    actionCounts_.clear();

    initialized_ = false;
    LOGI("Thermal Manager shutdown complete");
}

void ThermalManager::setJNIEnvironment(JNIEnv* env, jobject context) {
    env_ = env;
    context_ = context;
    LOGI("JNI environment set for Thermal Manager");
}

void ThermalManager::detectThermalCapabilities() {
    LOGI("Detecting thermal capabilities");

    // In a real implementation, this would query the Android system for thermal capabilities
    // For now, simulate capability detection
    LOGI("Thermal capabilities detected");
}

void ThermalManager::setSettings(const ThermalSettings& settings) {
    settings_ = settings;
    validateSettings();
    LOGI("Thermal settings updated");
}

void ThermalManager::setMonitoringInterval(float interval) {
    settings_.monitoringInterval = interval;
    if (temperatureMonitor_) {
        temperatureMonitor_->setMonitoringInterval(interval);
    }
    LOGI("Monitoring interval set to: %.2f", interval);
}

void ThermalManager::setAdaptationInterval(float interval) {
    settings_.adaptationInterval = interval;
    LOGI("Adaptation interval set to: %.2f", interval);
}

void ThermalManager::setCooldownPeriod(float period) {
    settings_.cooldownPeriod = period;
    LOGI("Cooldown period set to: %.2f", period);
}

void ThermalManager::setCriticalTemperature(float temperature) {
    settings_.criticalTemperature = temperature;
    LOGI("Critical temperature set to: %.2f", temperature);
}

void ThermalManager::setEmergencyTemperature(float temperature) {
    settings_.emergencyTemperature = temperature;
    LOGI("Emergency temperature set to: %.2f", temperature);
}

void ThermalManager::enableAutoAdaptation(bool enable) {
    settings_.autoAdaptation = enable;
    LOGI("Auto adaptation %s", enable ? "enabled" : "disabled");
}

void ThermalManager::enableAggressiveThrottling(bool enable) {
    settings_.aggressiveThrottling = enable;
    LOGI("Aggressive throttling %s", enable ? "enabled" : "disabled");
}

void ThermalManager::enableLogging(bool enable) {
    settings_.enableLogging = enable;
    LOGI("Logging %s", enable ? "enabled" : "disabled");
}

void ThermalManager::enableNotifications(bool enable) {
    settings_.enableNotifications = enable;
    LOGI("Notifications %s", enable ? "enabled" : "disabled");
}

void ThermalManager::setPerformanceLevel(PerformanceLevel level) {
    currentPerformanceLevel_ = level;
    performanceSettings_.level = level;

    if (performanceGovernor_) {
        performanceGovernor_->setPerformanceLevel(level);
    }

    onPerformanceChanged(level);
    LOGI("Performance level set to: %d", static_cast<int>(level));
}

void ThermalManager::setTargetFps(int fps) {
    performanceSettings_.targetFps = fps;
    if (performanceGovernor_) {
        performanceGovernor_->setTargetFps(fps);
    }
    LOGI("Target FPS set to: %d", fps);
}

void ThermalManager::setMaxFrameTime(int frameTime) {
    performanceSettings_.maxFrameTime = frameTime;
    if (performanceGovernor_) {
        performanceGovernor_->setMaxFrameTime(frameTime);
    }
    LOGI("Max frame time set to: %d", frameTime);
}

void ThermalManager::enableVSync(bool enable) {
    performanceSettings_.enableVSync = enable;
    LOGI("VSync %s", enable ? "enabled" : "disabled");
}

void ThermalManager::setCpuFrequencyScale(float scale) {
    performanceSettings_.cpuFrequencyScale = std::max(0.1f, std::min(1.0f, scale));
    LOGI("CPU frequency scale set to: %.2f", performanceSettings_.cpuFrequencyScale);
}

void ThermalManager::setGpuFrequencyScale(float scale) {
    performanceSettings_.gpuFrequencyScale = std::max(0.1f, std::min(1.0f, scale));
    LOGI("GPU frequency scale set to: %.2f", performanceSettings_.gpuFrequencyScale);
}

void ThermalManager::setQualityLevel(QualityLevel level) {
    currentQualityLevel_ = level;
    qualitySettings_.level = level;

    if (qualityManager_) {
        qualityManager_->setQualityLevel(level);
    }

    onQualityChanged(level);
    LOGI("Quality level set to: %d", static_cast<int>(level));
}

void ThermalManager::setTextureQuality(int quality) {
    qualitySettings_.textureQuality = std::max(0, std::min(100, quality));
    if (qualityManager_) {
        qualityManager_->setTextureQuality(quality);
    }
    LOGI("Texture quality set to: %d", quality);
}

void ThermalManager::setShadowQuality(int quality) {
    qualitySettings_.shadowQuality = std::max(0, std::min(100, quality));
    if (qualityManager_) {
        qualityManager_->setShadowQuality(quality);
    }
    LOGI("Shadow quality set to: %d", quality);
}

void ThermalManager::setParticleQuality(int quality) {
    qualitySettings_.particleQuality = std::max(0, std::min(100, quality));
    if (qualityManager_) {
        qualityManager_->setParticleQuality(quality);
    }
    LOGI("Particle quality set to: %d", quality);
}

void ThermalManager::setLightingQuality(int quality) {
    qualitySettings_.lightingQuality = std::max(0, std::min(100, quality));
    if (qualityManager_) {
        qualityManager_->setLightingQuality(quality);
    }
    LOGI("Lighting quality set to: %d", quality);
}

void ThermalManager::setPostProcessQuality(int quality) {
    qualitySettings_.postProcessQuality = std::max(0, std::min(100, quality));
    if (qualityManager_) {
        qualityManager_->setPostProcessQuality(quality);
    }
    LOGI("Post-processing quality set to: %d", quality);
}

void ThermalManager::enableBloom(bool enable) {
    qualitySettings_.enableBloom = enable;
    if (qualityManager_) {
        qualityManager_->enableBloom(enable);
    }
    LOGI("Bloom %s", enable ? "enabled" : "disabled");
}

void ThermalManager::enableMotionBlur(bool enable) {
    qualitySettings_.enableMotionBlur = enable;
    if (qualityManager_) {
        qualityManager_->enableMotionBlur(enable);
    }
    LOGI("Motion blur %s", enable ? "enabled" : "disabled");
}

void ThermalManager::enableDepthOfField(bool enable) {
    qualitySettings_.enableDepthOfField = enable;
    if (qualityManager_) {
        qualityManager_->enableDepthOfField(enable);
    }
    LOGI("Depth of field %s", enable ? "enabled" : "disabled");
}

void ThermalManager::enableSSAO(bool enable) {
    qualitySettings_.enableSSAO = enable;
    if (qualityManager_) {
        qualityManager_->enableSSAO(enable);
    }
    LOGI("SSAO %s", enable ? "enabled" : "disabled");
}

void ThermalManager::addThermalPolicy(const ThermalPolicy& policy) {
    if (policyManager_) {
        policyManager_->addPolicy(policy);
    }
    LOGI("Thermal policy added: %s", policy.name.c_str());
}

void ThermalManager::removeThermalPolicy(const std::string& name) {
    if (policyManager_) {
        policyManager_->removePolicy(name);
    }
    LOGI("Thermal policy removed: %s", name.c_str());
}

void ThermalManager::enableThermalPolicy(const std::string& name, bool enable) {
    if (policyManager_) {
        policyManager_->enablePolicy(name, enable);
    }
    LOGI("Thermal policy %s %s", name.c_str(), enable ? "enabled" : "disabled");
}

bool ThermalManager::isThermalPolicyEnabled(const std::string& name) const {
    if (policyManager_) {
        return policyManager_->isPolicyEnabled(name);
    }
    return false;
}

std::vector<ThermalPolicy> ThermalManager::getThermalPolicies() const {
    if (policyManager_) {
        return policyManager_->getPolicies();
    }
    return std::vector<ThermalPolicy>();
}

void ThermalManager::clearThermalPolicies() {
    if (policyManager_) {
        // Clear policies would be implemented in policy manager
    }
    LOGI("Thermal policies cleared");
}

void ThermalManager::triggerThermalAction(ThermalAction action, const std::string& reason) {
    applyThermalAction(action, reason);
    LOGI("Thermal action triggered: %d, reason: %s", static_cast<int>(action), reason.c_str());
}

void ThermalManager::revertThermalAction(ThermalAction action) {
    // In a real implementation, this would revert the last action
    LOGI("Thermal action reverted: %d", static_cast<int>(action));
}

void ThermalManager::resetThermalState() {
    LOGI("Resetting thermal state");

    thermalManagementActive_ = false;
    currentStatus_ = ThermalStatusData();
    currentMetrics_ = PerformanceMetrics();
    mitigationHistory_.clear();
    actionCounts_.clear();
    consecutiveFailures_ = 0;

    LOGI("Thermal state reset");
}

int ThermalManager::getActionCount(ThermalAction action) const {
    auto it = actionCounts_.find(action);
    if (it != actionCounts_.end()) {
        return it->second;
    }
    return 0;
}

void ThermalManager::enableAdaptivePolicies(bool enable) {
    adaptivePolicies_ = enable;
    if (policyManager_) {
        policyManager_->enableAdaptivePolicies(enable);
    }
    LOGI("Adaptive policies %s", enable ? "enabled" : "disabled");
}

void ThermalManager::setTemperatureScale(float scale) {
    temperatureScale_ = std::max(0.5f, std::min(2.0f, scale));
    if (temperatureMonitor_) {
        temperatureMonitor_->setTemperatureScale(scale);
    }
    LOGI("Temperature scale set to: %.2f", temperatureScale_);
}

void ThermalManager::setMaxConsecutiveActions(int maxActions) {
    settings_.maxConsecutiveActions = maxActions;
    if (policyManager_) {
        policyManager_->setMaxConsecutiveActions(maxActions);
    }
    LOGI("Max consecutive actions set to: %d", maxActions);
}

void ThermalManager::enableThermalPrediction(bool enable) {
    if (temperatureMonitor_) {
        temperatureMonitor_->enablePrediction(enable);
    }
    LOGI("Thermal prediction %s", enable ? "enabled" : "disabled");
}

void ThermalManager::setPredictionWindow(int seconds) {
    if (temperatureMonitor_) {
        temperatureMonitor_->setPredictionSamples(seconds);
    }
    LOGI("Prediction window set to: %d seconds", seconds);
}

void ThermalManager::updatePerformanceMetrics(const PerformanceMetrics& metrics) {
    currentMetrics_ = metrics;

    if (performanceGovernor_) {
        performanceGovernor_->adaptToPerformanceMetrics(metrics);
    }

    if (qualityManager_) {
        qualityManager_->adaptToPerformanceLevel(currentPerformanceLevel_);
    }
}

void ThermalManager::forceThermalCheck() {
    if (temperatureMonitor_) {
        temperatureMonitor_->updateTemperatures();
    }
    LOGI("Forced thermal check completed");
}

bool ThermalManager::isThermalEmergency() const {
    return currentStatus_.status == ThermalStatus::EMERGENCY ||
           currentStatus_.status == ThermalStatus::SHUTDOWN;
}

bool ThermalManager::isPerformanceDegraded() const {
    return currentPerformanceLevel_ == PerformanceLevel::MINIMUM ||
           currentPerformanceLevel_ == PerformanceLevel::LOW;
}

void ThermalManager::registerThermalStatusCallback(const std::string& id, ThermalStatusCallback callback) {
    statusCallbacks_[id] = callback;
    LOGI("Thermal status callback registered: %s", id.c_str());
}

void ThermalManager::unregisterThermalStatusCallback(const std::string& id) {
    statusCallbacks_.erase(id);
    LOGI("Thermal status callback unregistered: %s", id.c_str());
}

void ThermalManager::registerThermalActionCallback(const std::string& id, ThermalActionCallback callback) {
    actionCallbacks_[id] = callback;
    LOGI("Thermal action callback registered: %s", id.c_str());
}

void ThermalManager::unregisterThermalActionCallback(const std::string& id) {
    actionCallbacks_.erase(id);
    LOGI("Thermal action callback unregistered: %s", id.c_str());
}

void ThermalManager::registerPerformanceChangedCallback(const std::string& id, PerformanceChangedCallback callback) {
    performanceCallbacks_[id] = callback;
    LOGI("Performance changed callback registered: %s", id.c_str());
}

void ThermalManager::unregisterPerformanceChangedCallback(const std::string& id) {
    performanceCallbacks_.erase(id);
    LOGI("Performance changed callback unregistered: %s", id.c_str());
}

void ThermalManager::registerQualityChangedCallback(const std::string& id, QualityChangedCallback callback) {
    qualityCallbacks_[id] = callback;
    LOGI("Quality changed callback registered: %s", id.c_str());
}

void ThermalManager::unregisterQualityChangedCallback(const std::string& id) {
    qualityCallbacks_.erase(id);
    LOGI("Quality changed callback unregistered: %s", id.c_str());
}

void ThermalManager::registerThermalAlertCallback(const std::string& id, ThermalAlertCallback callback) {
    alertCallbacks_[id] = callback;
    LOGI("Thermal alert callback registered: %s", id.c_str());
}

void ThermalManager::unregisterThermalAlertCallback(const std::string& id) {
    alertCallbacks_.erase(id);
    LOGI("Thermal alert callback unregistered: %s", id.c_str());
}

std::string ThermalManager::getThermalStatusString() const {
    std::stringstream status;
    status << "Thermal Status:\n";
    status << "Status: " << static_cast<int>(currentStatus_.status) << "\n";
    status << "CPU Temp: " << currentStatus_.cpuTemperature << "°C\n";
    status << "GPU Temp: " << currentStatus_.gpuTemperature << "°C\n";
    status << "Battery Temp: " << currentStatus_.batteryTemperature << "°C\n";
    status << "Skin Temp: " << currentStatus_.skinTemperature << "°C\n";
    status << "Throttling: " << (currentStatus_.isThrottling ? "YES" : "NO") << "\n";
    status << "Headroom: " << (currentStatus_.thermalHeadroom * 100.0f) << "%\n";
    return status.str();
}

std::string ThermalManager::getPerformanceStatusString() const {
    std::stringstream status;
    status << "Performance Status:\n";
    status << "Level: " << static_cast<int>(currentPerformanceLevel_) << "\n";
    status << "CPU Usage: " << currentMetrics_.cpuUsage << "%\n";
    status << "GPU Usage: " << currentMetrics_.gpuUsage << "%\n";
    status << "Memory Usage: " << currentMetrics_.memoryUsage << "%\n";
    status << "FPS: " << currentMetrics_.fps << "\n";
    status << "Target FPS: " << currentMetrics_.targetFps << "\n";
    status << "Frame Time: " << currentMetrics_.frameTime << "ms\n";
    status << "CPU Freq: " << currentMetrics_.cpuFrequency << "MHz\n";
    status << "GPU Freq: " << currentMetrics_.gpuFrequency << "MHz\n";
    return status.str();
}

void ThermalManager::resetAllSettings() {
    LOGI("Resetting all thermal settings");

    initializeDefaults();
    resetThermalState();

    if (performanceGovernor_) {
        performanceGovernor_->resetToDefaults();
    }

    if (qualityManager_) {
        // Reset quality settings would be implemented
    }

    LOGI("All thermal settings reset");
}

void ThermalManager::testThermalSystem() {
    LOGI("Testing thermal system");

    // Test temperature monitoring
    forceThermalCheck();

    // Test performance adaptation
    setPerformanceLevel(PerformanceLevel::HIGH);
    setQualityLevel(QualityLevel::HIGH);

    // Test policy evaluation
    if (policyManager_) {
        policyManager_->evaluatePolicies(currentStatus_);
    }

    LOGI("Thermal system test completed");
}

void ThermalManager::enableEmergencyMode() {
    LOGI("Enabling emergency mode");

    thermalManagementActive_ = true;
    setPerformanceLevel(PerformanceLevel::MINIMUM);
    setQualityLevel(QualityLevel::LOWEST);

    // Trigger emergency action
    triggerThermalAction(ThermalAction::PAUSE_GAME, "Emergency mode activated");

    onThermalAlert(ThermalStatus::EMERGENCY, "Emergency thermal mode activated");
}

void ThermalManager::disableEmergencyMode() {
    LOGI("Disabling emergency mode");

    thermalManagementActive_ = false;
    resetThermalState();

    onThermalAlert(ThermalStatus::MODERATE, "Emergency thermal mode deactivated");
}

bool ThermalManager::isEmergencyMode() const {
    return thermalManagementActive_ && currentPerformanceLevel_ == PerformanceLevel::MINIMUM;
}

void ThermalManager::setEmergencyAction(ThermalAction action) {
    // In a real implementation, this would set the emergency action
    LOGI("Emergency action set to: %d", static_cast<int>(action));
}

void ThermalManager::setMaxProcessingTime(float maxTime) {
    // In a real implementation, this would set max processing time
    LOGI("Max processing time set to: %.2f", maxTime);
}

void ThermalManager::enableParallelProcessing(bool enable) {
    // In a real implementation, this would enable parallel processing
    LOGI("Parallel processing %s", enable ? "enabled" : "disabled");
}

void ThermalManager::setThreadCount(int threads) {
    // In a real implementation, this would set thread count
    LOGI("Thread count set to: %d", threads);
}

void ThermalManager::initializeDefaults() {
    LOGI("Initializing thermal defaults");

    // Initialize default settings
    settings_.enabled = true;
    settings_.autoAdaptation = true;
    settings_.aggressiveThrottling = false;
    settings_.monitoringInterval = 1.0f;
    settings_.adaptationInterval = 2.0f;
    settings_.cooldownPeriod = 30.0f;
    settings_.enableLogging = true;
    settings_.enableNotifications = true;
    settings_.enableHapticFeedback = true;
    settings_.criticalTemperature = 80.0f;
    settings_.emergencyTemperature = 90.0f;
    settings_.maxConsecutiveActions = 3;

    // Initialize default performance settings
    performanceSettings_.level = PerformanceLevel::HIGH;
    performanceSettings_.targetFps = 60;
    performanceSettings_.maxFrameTime = 16;
    performanceSettings_.cpuFrequencyScale = 1.0f;
    performanceSettings_.gpuFrequencyScale = 1.0f;
    performanceSettings_.enableVSync = true;
    performanceSettings_.enableMultithreading = true;
    performanceSettings_.threadCount = 4;
    performanceSettings_.enableOptimizations = true;

    // Initialize default quality settings
    qualitySettings_.level = QualityLevel::HIGH;
    qualitySettings_.textureQuality = 80;
    qualitySettings_.shadowQuality = 70;
    qualitySettings_.particleQuality = 60;
    qualitySettings_.lightingQuality = 75;
    qualitySettings_.postProcessQuality = 70;
    qualitySettings_.geometryQuality = 80;
    qualitySettings_.enableBloom = true;
    qualitySettings_.enableMotionBlur = true;
    qualitySettings_.enableDepthOfField = false;
    qualitySettings_.enableSSAO = true;
    qualitySettings_.maxTextureSize = 2048;
    qualitySettings_.maxShadowMapSize = 1024;

    // Initialize current levels
    currentPerformanceLevel_ = PerformanceLevel::HIGH;
    currentQualityLevel_ = QualityLevel::HIGH;

    // Initialize thermal status
    currentStatus_.status = ThermalStatus::LIGHT;
    currentStatus_.thermalHeadroom = 1.0f;
    currentStatus_.isThrottling = false;

    // Initialize performance metrics
    currentMetrics_.cpuUsage = 0.0f;
    currentMetrics_.gpuUsage = 0.0f;
    currentMetrics_.memoryUsage = 0.0f;
    currentMetrics_.batteryLevel = 100.0f;
    currentMetrics_.fps = 60.0f;
    currentMetrics_.targetFps = 60.0f;
    currentMetrics_.frameTime = 16;
    currentMetrics_.cpuFrequency = 2000;
    currentMetrics_.gpuFrequency = 800;

    LOGI("Thermal defaults initialized");
}

void ThermalManager::startServiceThreads() {
    LOGI("Starting thermal service threads");

    serviceRunning_ = true;
    monitorThread_ = std::thread(&ThermalManager::monitorThreadLoop, this);
    adaptationThread_ = std::thread(&ThermalManager::adaptationThreadLoop, this);
    policyThread_ = std::thread(&ThermalManager::policyThreadLoop, this);

    LOGI("Thermal service threads started");
}

void ThermalManager::stopServiceThreads() {
    LOGI("Stopping thermal service threads");

    serviceRunning_ = false;

    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }

    if (adaptationThread_.joinable()) {
        adaptationThread_.join();
    }

    if (policyThread_.joinable()) {
        policyThread_.join();
    }

    LOGI("Thermal service threads stopped");
}

void ThermalManager::monitorThreadLoop() {
    LOGI("Thermal monitor thread started");

    while (serviceRunning_) {
        if (temperatureMonitor_) {
            temperatureMonitor_->updateTemperatures();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(settings_.monitoringInterval * 1000)));
    }

    LOGI("Thermal monitor thread ended");
}

void ThermalManager::adaptationThreadLoop() {
    LOGI("Thermal adaptation thread started");

    while (serviceRunning_) {
        if (settings_.autoAdaptation) {
            adaptPerformanceLevel();
            adaptQualityLevel();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(settings_.adaptationInterval * 1000)));
    }

    LOGI("Thermal adaptation thread ended");
}

void ThermalManager::policyThreadLoop() {
    LOGI("Thermal policy thread started");

    while (serviceRunning_) {
        if (policyManager_) {
            policyManager_->evaluatePolicies(currentStatus_);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    LOGI("Thermal policy thread ended");
}

void ThermalManager::onThermalStatusChanged(const ThermalStatusData& status) {
    currentStatus_ = status;

    // Call registered callbacks
    for (const auto& pair : statusCallbacks_) {
        pair.second(status);
    }

    LOGI("Thermal status changed: %d", static_cast<int>(status.status));
}

void ThermalManager::onThermalActionTriggered(ThermalAction action, const std::string& reason) {
    updateMitigationHistory(action, reason);

    // Call registered callbacks
    for (const auto& pair : actionCallbacks_) {
        pair.second(action, reason);
    }

    LOGI("Thermal action triggered: %d, reason: %s", static_cast<int>(action), reason.c_str());
}

void ThermalManager::onPerformanceChanged(PerformanceLevel level) {
    currentPerformanceLevel_ = level;

    // Call registered callbacks
    for (const auto& pair : performanceCallbacks_) {
        pair.second(level);
    }

    LOGI("Performance level changed: %d", static_cast<int>(level));
}

void ThermalManager::onQualityChanged(QualityLevel level) {
    currentQualityLevel_ = level;

    // Call registered callbacks
    for (const auto& pair : qualityCallbacks_) {
        pair.second(level);
    }

    LOGI("Quality level changed: %d", static_cast<int>(level));
}

void ThermalManager::onThermalAlert(ThermalStatus status, const std::string& message) {
    // Call registered callbacks
    for (const auto& pair : alertCallbacks_) {
        pair.second(status, message);
    }

    LOGI("Thermal alert: %d, message: %s", static_cast<int>(status), message.c_str());
}

void ThermalManager::evaluateThermalPolicies() {
    if (policyManager_) {
        policyManager_->evaluatePolicies(currentStatus_);
    }
}

void ThermalManager::applyThermalAction(ThermalAction action, const std::string& reason) {
    LOGI("Applying thermal action: %d, reason: %s", static_cast<int>(action), reason.c_str());

    // Update action count
    actionCounts_[action]++;

    // Apply the action based on type
    switch (action) {
        case ThermalAction::REDUCE_CPU:
            setCpuFrequencyScale(0.7f);
            break;
        case ThermalAction::REDUCE_GPU:
            setGpuFrequencyScale(0.7f);
            break;
        case ThermalAction::LIMIT_FPS:
            setTargetFps(30);
            break;
        case ThermalAction::REDUCE_QUALITY:
            setQualityLevel(QualityLevel::MEDIUM);
            break;
        case ThermalAction::DISABLE_FEATURES:
            // Disable non-essential features
            enableBloom(false);
            enableMotionBlur(false);
            break;
        case ThermalAction::PAUSE_GAME:
            // In a real implementation, this would pause the game
            LOGI("Game would be paused");
            break;
        case ThermalAction::SHUTDOWN_APP:
            // In a real implementation, this would shutdown the app
            LOGI("App would be shutdown");
            break;
        default:
            break;
    }

    // Record the mitigation
    MitigationEvent event;
    event.action = action;
    event.triggerStatus = currentStatus_.status;
    event.temperature = (currentStatus_.cpuTemperature + currentStatus_.gpuTemperature) / 2.0f;
    event.timestamp = std::chrono::steady_clock::now();
    event.reason = reason;
    event.wasAutomatic = true;

    updateMitigationHistory(action, reason);
    onThermalActionTriggered(action, reason);
}

void ThermalManager::revertLastAction() {
    // In a real implementation, this would revert the last thermal action
    LOGI("Reverting last thermal action");
}

void ThermalManager::updateMitigationHistory(ThermalAction action, const std::string& reason) {
    MitigationEvent event;
    event.action = action;
    event.triggerStatus = currentStatus_.status;
    event.temperature = (currentStatus_.cpuTemperature + currentStatus_.gpuTemperature) / 2.0f;
    event.timestamp = std::chrono::steady_clock::now();
    event.reason = reason;
    event.wasAutomatic = true;

    mitigationHistory_.push_back(event);

    // Limit history size
    if (mitigationHistory_.size() > 100) {
        mitigationHistory_.erase(mitigationHistory_.begin(), mitigationHistory_.begin() + 20);
    }
}

void ThermalManager::checkCooldownPeriod() {
    auto now = std::chrono::steady_clock::now();
    auto cooldownDuration = std::chrono::duration_cast<std::chrono::seconds>(now - lastCooldownTime_).count();

    if (cooldownDuration >= static_cast<long long>(settings_.cooldownPeriod)) {
        // Cooldown period has passed, can increase performance
        if (currentPerformanceLevel_ < PerformanceLevel::HIGH) {
            setPerformanceLevel(static_cast<PerformanceLevel>(static_cast<int>(currentPerformanceLevel_) + 1));
        }
        lastCooldownTime_ = now;
    }
}

void ThermalManager::adaptPerformanceLevel() {
    // In a real implementation, this would adapt performance based on thermal conditions
    LOGI("Adapting performance level");
}

void ThermalManager::adaptQualityLevel() {
    // In a real implementation, this would adapt quality based on thermal conditions
    LOGI("Adapting quality level");
}

void ThermalManager::calculateOptimalSettings() {
    // In a real implementation, this would calculate optimal settings
    LOGI("Calculating optimal settings");
}

void ThermalManager::applyPerformanceSettings() {
    // In a real implementation, this would apply performance settings
    LOGI("Applying performance settings");
}

void ThermalManager::applyQualitySettings() {
    // In a real implementation, this would apply quality settings
    LOGI("Applying quality settings");
}

bool ThermalManager::isValidThermalStatus(const ThermalStatusData& status) const {
    return status.cpuTemperature >= 0.0f && status.gpuTemperature >= 0.0f &&
           status.batteryTemperature >= 0.0f && status.skinTemperature >= 0.0f;
}

ThermalStatus ThermalManager::determineThermalStatus(float temperature) const {
    if (temperature >= settings_.emergencyTemperature) {
        return ThermalStatus::EMERGENCY;
    } else if (temperature >= settings_.criticalTemperature) {
        return ThermalStatus::CRITICAL;
    } else if (temperature >= 70.0f) {
        return ThermalStatus::SEVERE;
    } else if (temperature >= 60.0f) {
        return ThermalStatus::MODERATE;
    } else if (temperature >= 50.0f) {
        return ThermalStatus::LIGHT;
    } else {
        return ThermalStatus::NONE;
    }
}

float ThermalManager::calculateThermalHeadroom() const {
    float maxTemp = std::max({currentStatus_.cpuTemperature, currentStatus_.gpuTemperature,
                             currentStatus_.batteryTemperature, currentStatus_.skinTemperature});
    float criticalTemp = settings_.criticalTemperature;

    if (maxTemp >= criticalTemp) {
        return 0.0f;
    }

    return 1.0f - (maxTemp / criticalTemp);
}

void ThermalManager::updatePerformanceMetrics() {
    // In a real implementation, this would update performance metrics
    LOGI("Updating performance metrics");
}

void ThermalManager::validateSettings() {
    // In a real implementation, this would validate settings
    LOGI("Validating settings");
}

void ThermalManager::applyAdaptivePolicies() {
    // In a real implementation, this would apply adaptive policies
    LOGI("Applying adaptive policies");
}

void ThermalManager::predictThermalTrends() {
    // In a real implementation, this would predict thermal trends
    LOGI("Predicting thermal trends");
}

void ThermalManager::updateActionCounts() {
    // In a real implementation, this would update action counts
    LOGI("Updating action counts");
}

void ThermalManager::cleanupOldHistory() {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::hours(24); // Keep 24 hours of history

    mitigationHistory_.erase(
        std::remove_if(mitigationHistory_.begin(), mitigationHistory_.end(),
                      [cutoff](const MitigationEvent& event) {
                          return event.timestamp < cutoff;
                      }),
        mitigationHistory_.end());
}

// ========== TEMPERATURE MONITOR IMPLEMENTATION ==========
TemperatureMonitor::TemperatureMonitor(ThermalManager* manager) : manager_(manager), monitoringInterval_(1.0f),
                                                                 predictionEnabled_(true), predictionSamples_(5),
                                                                 temperatureScale_(1.0f) {
    LOGI("TemperatureMonitor constructor called");
}

TemperatureMonitor::~TemperatureMonitor() {
    shutdown();
    LOGI("TemperatureMonitor destructor called");
}

bool TemperatureMonitor::initialize() {
    LOGI("Initializing Temperature Monitor");
    return true;
}

void TemperatureMonitor::shutdown() {
    LOGI("Shutting down Temperature Monitor");

    std::lock_guard<std::mutex> lock(sensorMutex_);
    sensors_.clear();
    zoneSensors_.clear();
}

void TemperatureMonitor::updateTemperatures() {
    std::lock_guard<std::mutex> lock(sensorMutex_);

    readSensorData();
    updateZoneTemperatures();

    if (predictionEnabled_) {
        predictTemperatureTrends();
    }

    applyTemperatureSmoothing();
}

void TemperatureMonitor::readSensorData() {
    // In a real implementation, this would read from actual thermal sensors
    // For now, simulate sensor readings
    LOGI("Reading sensor data");
}

void TemperatureMonitor::updateZoneTemperatures() {
    // In a real implementation, this would update zone temperatures
    LOGI("Updating zone temperatures");
}

void TemperatureMonitor::predictTemperatureTrends() {
    // In a real implementation, this would predict temperature trends
    LOGI("Predicting temperature trends");
}

void TemperatureMonitor::applyTemperatureSmoothing() {
    // In a real implementation, this would apply temperature smoothing
    LOGI("Applying temperature smoothing");
}

void TemperatureMonitor::setMonitoringInterval(float interval) {
    monitoringInterval_ = interval;
    LOGI("Monitoring interval set to: %.2f", interval);
}

void TemperatureMonitor::setTemperatureScale(float scale) {
    temperatureScale_ = scale;
    LOGI("Temperature scale set to: %.2f", scale);
}

void TemperatureMonitor::enablePrediction(bool enable) {
    predictionEnabled_ = enable;
    LOGI("Temperature prediction %s", enable ? "enabled" : "disabled");
}

void TemperatureMonitor::setPredictionSamples(int samples) {
    predictionSamples_ = samples;
    LOGI("Prediction samples set to: %d", samples);
}

ThermalSensor TemperatureMonitor::getSensor(ThermalZone zone) const {
    std::lock_guard<std::mutex> lock(sensorMutex_);

    auto it = zoneSensors_.find(zone);
    if (it != zoneSensors_.end()) {
        return it->second;
    }

    return ThermalSensor();
}

float TemperatureMonitor::getTemperature(ThermalZone zone) const {
    auto sensor = getSensor(zone);
    return sensor.temperature;
}

bool TemperatureMonitor::isZoneAvailable(ThermalZone zone) const {
    std::lock_guard<std::mutex> lock(sensorMutex_);

    return zoneSensors_.find(zone) != zoneSensors_.end();
}

void TemperatureMonitor::addCustomSensor(const std::string& name, const std::string& path) {
    // In a real implementation, this would add a custom sensor
    LOGI("Custom sensor added: %s", name.c_str());
}

void TemperatureMonitor::removeCustomSensor(const std::string& name) {
    // In a real implementation, this would remove a custom sensor
    LOGI("Custom sensor removed: %s", name.c_str());
}

void TemperatureMonitor::calibrateSensors() {
    // In a real implementation, this would calibrate sensors
    LOGI("Calibrating sensors");
}

void TemperatureMonitor::validateSensorData() {
    // In a real implementation, this would validate sensor data
    LOGI("Validating sensor data");
}

bool TemperatureMonitor::isValidTemperature(float temperature, ThermalZone zone) const {
    return temperature >= 0.0f && temperature <= 150.0f; // Reasonable temperature range
}

// ========== PERFORMANCE GOVERNOR IMPLEMENTATION ==========
PerformanceGovernor::PerformanceGovernor(ThermalManager* manager) : manager_(manager), currentLevel_(PerformanceLevel::HIGH),
                                                                   autoAdaptation_(true), adaptationThreshold_(0.8f),
                                                                   minFrameTime_(8), maxFrameTime_(33) {
    LOGI("PerformanceGovernor constructor called");
}

PerformanceGovernor::~PerformanceGovernor() {
    shutdown();
    LOGI("PerformanceGovernor destructor called");
}

bool PerformanceGovernor::initialize() {
    LOGI("Initializing Performance Governor");
    return true;
}

void PerformanceGovernor::shutdown() {
    LOGI("Shutting down Performance Governor");

    std::lock_guard<std::mutex> lock(governorMutex_);
    levelSettings_.clear();
}

void PerformanceGovernor::setPerformanceLevel(PerformanceLevel level) {
    std::lock_guard<std::mutex> lock(governorMutex_);

    currentLevel_ = level;
    settings_.level = level;

    applyPerformanceSettings();
    LOGI("Performance level set to: %d", static_cast<int>(level));
}

void PerformanceGovernor::setTargetFps(int fps) {
    settings_.targetFps = fps;
    LOGI("Target FPS set to: %d", fps);
}

void PerformanceGovernor::setMaxFrameTime(int frameTime) {
    settings_.maxFrameTime = frameTime;
    LOGI("Max frame time set to: %d", frameTime);
}

void PerformanceGovernor::enableVSync(bool enable) {
    settings_.enableVSync = enable;
    LOGI("VSync %s", enable ? "enabled" : "disabled");
}

void PerformanceGovernor::setCpuFrequencyScale(float scale) {
    settings_.cpuFrequencyScale = std::max(0.1f, std::min(1.0f, scale));
    LOGI("CPU frequency scale set to: %.2f", settings_.cpuFrequencyScale);
}

void PerformanceGovernor::setGpuFrequencyScale(float scale) {
    settings_.gpuFrequencyScale = std::max(0.1f, std::min(1.0f, scale));
    LOGI("GPU frequency scale set to: %.2f", settings_.gpuFrequencyScale);
}

void PerformanceGovernor::adaptToThermalConditions(ThermalStatus status) {
    // In a real implementation, this would adapt to thermal conditions
    LOGI("Adapting to thermal conditions: %d", static_cast<int>(status));
}

void PerformanceGovernor::adaptToPerformanceMetrics(const PerformanceMetrics& metrics) {
    // In a real implementation, this would adapt to performance metrics
    LOGI("Adapting to performance metrics");
}

void PerformanceGovernor::optimizeForBatteryLevel(float batteryLevel) {
    // In a real implementation, this would optimize for battery level
    LOGI("Optimizing for battery level: %.1f%%", batteryLevel);
}

void PerformanceGovernor::optimizeForWorkload(int workload) {
    // In a real implementation, this would optimize for workload
    LOGI("Optimizing for workload: %d", workload);
}

void PerformanceGovernor::definePerformanceLevel(PerformanceLevel level, const PerformanceSettings& settings) {
    std::lock_guard<std::mutex> lock(governorMutex_);
    levelSettings_[level] = settings;
    LOGI("Performance level defined: %d", static_cast<int>(level));
}

PerformanceSettings PerformanceGovernor::getPerformanceSettings(PerformanceLevel level) const {
    std::lock_guard<std::mutex> lock(governorMutex_);

    auto it = levelSettings_.find(level);
    if (it != levelSettings_.end()) {
        return it->second;
    }

    return PerformanceSettings();
}

void PerformanceGovernor::resetToDefaults() {
    // In a real implementation, this would reset to defaults
    LOGI("Resetting to defaults");
}

void PerformanceGovernor::applyPerformanceSettings() {
    // In a real implementation, this would apply performance settings
    LOGI("Applying performance settings");
}

void PerformanceGovernor::calculateOptimalFrameRate() {
    // In a real implementation, this would calculate optimal frame rate
    LOGI("Calculating optimal frame rate");
}

void PerformanceGovernor::adjustFrequencyScaling() {
    // In a real implementation, this would adjust frequency scaling
    LOGI("Adjusting frequency scaling");
}

void PerformanceGovernor::validatePerformanceSettings() {
    // In a real implementation, this would validate performance settings
    LOGI("Validating performance settings");
}

bool PerformanceGovernor::isPerformanceLevelValid(PerformanceLevel level) const {
    return level >= PerformanceLevel::MINIMUM && level <= PerformanceLevel::MAXIMUM;
}

} // namespace FoundryEngine
