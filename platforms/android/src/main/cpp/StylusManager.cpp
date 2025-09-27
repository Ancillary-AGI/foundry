#include "StylusManager.h"
#include <android/log.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <random>
#include <nlohmann/json.hpp> // For JSON parsing

#define LOG_TAG "StylusManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace FoundryEngine {

// Static instance
StylusManager* StylusManager::instance_ = nullptr;

// ========== STYLUS MANAGER IMPLEMENTATION ==========
StylusManager::StylusManager() : pressureProcessor_(nullptr), tiltProcessor_(nullptr),
                               palmRejectionManager_(nullptr), calibrationManager_(nullptr),
                               gestureRecognizer_(nullptr), env_(nullptr), context_(nullptr),
                               initialized_(false), stylusPresent_(false), stylusActive_(false),
                               stylusType_(StylusType::NONE), currentMode_(StylusMode::DRAWING),
                               isDrawing_(false), serviceRunning_(false), autoCalibration_(true),
                               adaptiveSensitivity_(true), sensitivityUpdateInterval_(1.0f) {
    LOGI("StylusManager constructor called");
}

StylusManager::~StylusManager() {
    shutdown();
    LOGI("StylusManager destructor called");
}

StylusManager* StylusManager::getInstance() {
    if (!instance_) {
        instance_ = new StylusManager();
    }
    return instance_;
}

void StylusManager::initialize() {
    LOGI("Initializing Stylus Manager");

    if (initialized_) {
        LOGW("Stylus Manager already initialized");
        return;
    }

    // Initialize sub-managers
    pressureProcessor_ = new PressureProcessor(this);
    tiltProcessor_ = new TiltProcessor(this);
    palmRejectionManager_ = new PalmRejectionManager(this);
    calibrationManager_ = new StylusCalibrationManager(this);
    gestureRecognizer_ = new GestureRecognizer(this);

    // Initialize defaults
    initializeDefaults();

    // Start service threads
    startServiceThreads();

    // Detect stylus capabilities
    detectStylusCapabilities();

    // Initialize sub-managers
    if (pressureProcessor_->initialize()) {
        LOGI("Pressure Processor initialized successfully");
    } else {
        LOGE("Failed to initialize Pressure Processor");
    }

    if (tiltProcessor_->initialize()) {
        LOGI("Tilt Processor initialized successfully");
    } else {
        LOGE("Failed to initialize Tilt Processor");
    }

    if (palmRejectionManager_->initialize()) {
        LOGI("Palm Rejection Manager initialized successfully");
    } else {
        LOGE("Failed to initialize Palm Rejection Manager");
    }

    if (calibrationManager_->initialize()) {
        LOGI("Stylus Calibration Manager initialized successfully");
    } else {
        LOGE("Failed to initialize Stylus Calibration Manager");
    }

    if (gestureRecognizer_->initialize()) {
        LOGI("Gesture Recognizer initialized successfully");
    } else {
        LOGE("Failed to initialize Gesture Recognizer");
    }

    initialized_ = true;
    LOGI("Stylus Manager initialized successfully");
}

void StylusManager::update(float dt) {
    // Process event queue
    processStylusEvents();

    // Update gesture recognition
    if (gestureRecognizer_) {
        // Gesture processing happens in real-time
    }

    // Update calibration if needed
    if (autoCalibration_ && calibrationManager_) {
        // Auto-calibration happens periodically
    }

    // Update adaptive sensitivity
    if (adaptiveSensitivity_) {
        updateAdaptiveSensitivity();
    }

    // Process stroke data if drawing
    if (isDrawing_) {
        processStrokePoints();
    }
}

void StylusManager::shutdown() {
    LOGI("Shutting down Stylus Manager");

    if (!initialized_) {
        return;
    }

    // Stop service threads
    stopServiceThreads();

    // Shutdown sub-managers
    if (pressureProcessor_) {
        pressureProcessor_->shutdown();
        delete pressureProcessor_;
        pressureProcessor_ = nullptr;
    }

    if (tiltProcessor_) {
        tiltProcessor_->shutdown();
        delete tiltProcessor_;
        tiltProcessor_ = nullptr;
    }

    if (palmRejectionManager_) {
        palmRejectionManager_->shutdown();
        delete palmRejectionManager_;
        palmRejectionManager_ = nullptr;
    }

    if (calibrationManager_) {
        calibrationManager_->shutdown();
        delete calibrationManager_;
        calibrationManager_ = nullptr;
    }

    if (gestureRecognizer_) {
        gestureRecognizer_->shutdown();
        delete gestureRecognizer_;
        gestureRecognizer_ = nullptr;
    }

    // Clear callbacks
    eventCallbacks_.clear();
    gestureCallbacks_.clear();
    buttonCallbacks_.clear();
    calibrationCallbacks_.clear();
    capabilitiesCallbacks_.clear();

    // Clear event queue
    std::lock_guard<std::mutex> lock(eventMutex_);
    while (!eventQueue_.empty()) {
        eventQueue_.pop();
    }

    // Clear stroke data
    strokePoints_.clear();
    isDrawing_ = false;

    initialized_ = false;
    LOGI("Stylus Manager shutdown complete");
}

void StylusManager::setJNIEnvironment(JNIEnv* env, jobject context) {
    env_ = env;
    context_ = context;
    LOGI("JNI environment set for Stylus Manager");
}

void StylusManager::detectStylusCapabilities() {
    LOGI("Detecting stylus capabilities");

    // In a real implementation, this would query the Android system for stylus capabilities
    // For now, simulate capability detection
    capabilities_.hasPressure = true;
    capabilities_.hasTilt = true;
    capabilities_.hasEraser = true;
    capabilities_.hasButtons = true;
    capabilities_.hasHover = true;
    capabilities_.hasPalmRejection = true;
    capabilities_.hasGestureRecognition = true;
    capabilities_.maxPressureLevels = 2048;
    capabilities_.buttonCount = 2;
    capabilities_.pressureResolution = 0.001f;
    capabilities_.tiltResolution = 0.1f;
    capabilities_.supportedGestures = {
        StylusGesture::TAP,
        StylusGesture::DOUBLE_TAP,
        StylusGesture::LONG_PRESS,
        StylusGesture::DRAG,
        StylusGesture::FLICK,
        StylusGesture::CIRCLE
    };

    LOGI("Stylus capabilities detected");
    LOGI("Pressure: %s, Tilt: %s, Eraser: %s, Buttons: %s",
         capabilities_.hasPressure ? "YES" : "NO",
         capabilities_.hasTilt ? "YES" : "NO",
         capabilities_.hasEraser ? "YES" : "NO",
         capabilities_.hasButtons ? "YES" : "NO");
}

void StylusManager::startCalibration() {
    LOGI("Starting stylus calibration");

    if (calibrationManager_) {
        calibrationManager_->startCalibration();
    } else {
        LOGE("Calibration Manager not available");
    }
}

void StylusManager::stopCalibration() {
    LOGI("Stopping stylus calibration");

    if (calibrationManager_) {
        calibrationManager_->stopCalibration();
    } else {
        LOGE("Calibration Manager not available");
    }
}

bool StylusManager::isCalibrating() const {
    if (calibrationManager_) {
        return calibrationManager_->isCalibrating();
    }
    return false;
}

void StylusManager::resetCalibration() {
    LOGI("Resetting stylus calibration");

    if (calibrationManager_) {
        calibrationManager_->resetCalibration();
    } else {
        LOGE("Calibration Manager not available");
    }
}

void StylusManager::saveCalibration() {
    LOGI("Saving stylus calibration");

    if (calibrationManager_) {
        calibrationManager_->getCalibration(); // This would save to storage
    } else {
        LOGE("Calibration Manager not available");
    }
}

void StylusManager::loadCalibration() {
    LOGI("Loading stylus calibration");

    if (calibrationManager_) {
        // Load calibration from storage
    } else {
        LOGE("Calibration Manager not available");
    }
}

void StylusManager::setSettings(const StylusSettings& settings) {
    settings_ = settings;
    applySettings();
    LOGI("Stylus settings updated");
}

void StylusManager::setPressureSensitivity(float sensitivity) {
    settings_.pressureSensitivity = std::max(0.1f, std::min(5.0f, sensitivity));

    if (pressureProcessor_) {
        pressureProcessor_->setSensitivity(settings_.pressureSensitivity);
    }

    LOGI("Pressure sensitivity set to: %.2f", settings_.pressureSensitivity);
}

void StylusManager::setTiltSensitivity(float sensitivity) {
    settings_.tiltSensitivity = std::max(0.1f, std::min(5.0f, sensitivity));

    if (tiltProcessor_) {
        tiltProcessor_->setSensitivity(settings_.tiltSensitivity);
    }

    LOGI("Tilt sensitivity set to: %.2f", settings_.tiltSensitivity);
}

void StylusManager::setPalmRejectionEnabled(bool enabled) {
    settings_.palmRejectionEnabled = enabled;

    if (palmRejectionManager_) {
        palmRejectionManager_->enable(enabled);
    }

    LOGI("Palm rejection %s", enabled ? "enabled" : "disabled");
}

void StylusManager::setGestureRecognitionEnabled(bool enabled) {
    settings_.gestureRecognitionEnabled = enabled;

    if (gestureRecognizer_) {
        gestureRecognizer_->enable(enabled);
    }

    LOGI("Gesture recognition %s", enabled ? "enabled" : "disabled");
}

void StylusManager::setHapticFeedbackEnabled(bool enabled) {
    settings_.hapticFeedbackEnabled = enabled;
    LOGI("Haptic feedback %s", enabled ? "enabled" : "disabled");
}

void StylusManager::setVisualFeedbackEnabled(bool enabled) {
    settings_.visualFeedbackEnabled = enabled;
    LOGI("Visual feedback %s", enabled ? "enabled" : "disabled");
}

void StylusManager::setStylusMode(StylusMode mode) {
    currentMode_ = mode;
    LOGI("Stylus mode set to: %d", static_cast<int>(mode));
}

void StylusManager::setDrawingMode() {
    setStylusMode(StylusMode::DRAWING);
}

void StylusManager::setErasingMode() {
    setStylusMode(StylusMode::ERASING);
}

void StylusManager::setSelectionMode() {
    setStylusMode(StylusMode::SELECTION);
}

void StylusManager::setNavigationMode() {
    setStylusMode(StylusMode::NAVIGATION);
}

void StylusManager::setTextInputMode() {
    setStylusMode(StylusMode::TEXT_INPUT);
}

void StylusManager::setDrawingParams(const DrawingParams& params) {
    drawingParams_ = params;
    LOGI("Drawing parameters updated");
}

void StylusManager::setBrushSize(float size) {
    drawingParams_.brushSize = std::max(0.1f, size);
    LOGI("Brush size set to: %.2f", drawingParams_.brushSize);
}

void StylusManager::setPressureMultiplier(float multiplier) {
    drawingParams_.pressureMultiplier = std::max(0.0f, multiplier);
    LOGI("Pressure multiplier set to: %.2f", drawingParams_.pressureMultiplier);
}

void StylusManager::setTiltMultiplier(float multiplier) {
    drawingParams_.tiltMultiplier = std::max(0.0f, multiplier);
    LOGI("Tilt multiplier set to: %.2f", drawingParams_.tiltMultiplier);
}

void StylusManager::setOpacity(float opacity) {
    drawingParams_.opacity = std::max(0.0f, std::min(1.0f, opacity));
    LOGI("Opacity set to: %.2f", drawingParams_.opacity);
}

void StylusManager::setSmoothing(float smoothing) {
    drawingParams_.smoothing = std::max(0.0f, std::min(1.0f, smoothing));
    LOGI("Smoothing set to: %.2f", drawingParams_.smoothing);
}

void StylusManager::setStabilization(float stabilization) {
    drawingParams_.stabilization = std::max(0.0f, std::min(1.0f, stabilization));
    LOGI("Stabilization set to: %.2f", drawingParams_.stabilization);
}

void StylusManager::processStylusEvent(const StylusEvent& event) {
    // Add to event queue
    std::lock_guard<std::mutex> lock(eventMutex_);
    eventQueue_.push(event);
    eventCondition_.notify_one();
}

void StylusManager::processStylusEvents() {
    std::unique_lock<std::mutex> lock(eventMutex_);

    while (!eventQueue_.empty()) {
        StylusEvent event = eventQueue_.front();
        eventQueue_.pop();

        lock.unlock();

        // Process the event
        onStylusEvent(event);

        lock.lock();
    }
}

void StylusManager::enableGesture(StylusGesture gesture, bool enable) {
    if (gestureRecognizer_) {
        gestureRecognizer_->enableGesture(gesture, enable);
    }
    LOGI("Gesture %d %s", static_cast<int>(gesture), enable ? "enabled" : "disabled");
}

void StylusManager::setGestureConfig(StylusGesture gesture, const GestureConfig& config) {
    if (gestureRecognizer_) {
        gestureRecognizer_->setGestureConfig(gesture, config);
    }
    LOGI("Gesture config updated for gesture: %d", static_cast<int>(gesture));
}

GestureConfig StylusManager::getGestureConfig(StylusGesture gesture) const {
    if (gestureRecognizer_) {
        return gestureRecognizer_->getGestureConfig(gesture);
    }
    return GestureConfig();
}

std::vector<StylusGesture> StylusManager::getSupportedGestures() const {
    return capabilities_.supportedGestures;
}

bool StylusManager::isGestureEnabled(StylusGesture gesture) const {
    if (gestureRecognizer_) {
        return gestureRecognizer_->isGestureEnabled(gesture);
    }
    return false;
}

bool StylusManager::isButtonPressed(StylusButton button) const {
    for (const auto& state : buttonStates_) {
        if (state.button == button && state.isPressed) {
            return true;
        }
    }
    return false;
}

int StylusManager::getButtonClickCount(StylusButton button) const {
    for (const auto& state : buttonStates_) {
        if (state.button == button) {
            return state.clickCount;
        }
    }
    return 0;
}

void StylusManager::enableAdaptiveSensitivity(bool enable) {
    adaptiveSensitivity_ = enable;
    LOGI("Adaptive sensitivity %s", enable ? "enabled" : "disabled");
}

void StylusManager::setPredictionSamples(int samples) {
    settings_.predictionSamples = std::max(0, samples);
    LOGI("Prediction samples set to: %d", settings_.predictionSamples);
}

void StylusManager::setPredictionStrength(float strength) {
    settings_.predictionStrength = std::max(0.0f, std::min(1.0f, strength));
    LOGI("Prediction strength set to: %.2f", settings_.predictionStrength);
}

void StylusManager::enableStrokeSmoothing(bool enable) {
    settings_.enableSmoothing = enable;
    LOGI("Stroke smoothing %s", enable ? "enabled" : "disabled");
}

void StylusManager::enableStrokeStabilization(bool enable) {
    settings_.enableStabilization = enable;
    LOGI("Stroke stabilization %s", enable ? "enabled" : "disabled");
}

float StylusManager::getNormalizedPressure() const {
    if (pressureProcessor_) {
        return pressureProcessor_->getNormalizedPressure();
    }
    return 0.0f;
}

PressureLevel StylusManager::getPressureLevel() const {
    if (pressureProcessor_) {
        return pressureProcessor_->getCurrentLevel();
    }
    return PressureLevel::LIGHT;
}

void StylusManager::setPressureCurve(const float curve[10]) {
    if (pressureProcessor_) {
        pressureProcessor_->setPressureCurve(curve);
    }
    LOGI("Pressure curve updated");
}

float StylusManager::getPressureAtLevel(PressureLevel level) const {
    // In a real implementation, this would use the pressure curve
    switch (level) {
        case PressureLevel::LIGHT:
            return 0.25f;
        case PressureLevel::MEDIUM:
            return 0.5f;
        case PressureLevel::HEAVY:
            return 0.75f;
        case PressureLevel::CUSTOM:
            return 1.0f;
        default:
            return 0.5f;
    }
}

float StylusManager::getTiltAngle() const {
    if (tiltProcessor_) {
        return tiltProcessor_->getTiltAngle();
    }
    return 0.0f;
}

TiltDirection StylusManager::getTiltDirection() const {
    if (tiltProcessor_) {
        return tiltProcessor_->getDirection();
    }
    return TiltDirection::NORTH;
}

void StylusManager::setTiltOffset(float offsetX, float offsetY) {
    if (tiltProcessor_) {
        tiltProcessor_->setOffset(offsetX, offsetY);
    }
    LOGI("Tilt offset set to: %.2f, %.2f", offsetX, offsetY);
}

std::pair<float, float> StylusManager::getTiltOffset() const {
    if (tiltProcessor_) {
        return tiltProcessor_->getOffset();
    }
    return {0.0f, 0.0f};
}

void StylusManager::enablePalmRejection(bool enable) {
    setPalmRejectionEnabled(enable);
}

void StylusManager::setPalmRejectionSensitivity(float sensitivity) {
    if (palmRejectionManager_) {
        palmRejectionManager_->setSensitivity(sensitivity);
    }
    LOGI("Palm rejection sensitivity set to: %.2f", sensitivity);
}

float StylusManager::getPalmRejectionSensitivity() const {
    if (palmRejectionManager_) {
        return palmRejectionManager_->getSensitivity();
    }
    return 1.0f;
}

void StylusManager::triggerHapticFeedback(float intensity, int duration) {
    LOGI("Triggering haptic feedback: intensity=%.2f, duration=%d", intensity, duration);

    // In a real implementation, this would trigger haptic feedback
    LOGI("Haptic feedback triggered");
}

void StylusManager::setHapticPattern(const std::vector<int>& pattern) {
    // In a real implementation, this would set the haptic pattern
    LOGI("Haptic pattern updated");
}

std::vector<int> StylusManager::getHapticPattern() const {
    // In a real implementation, this would return the current haptic pattern
    return std::vector<int>();
}

void StylusManager::enableVisualFeedback(bool enable) {
    setVisualFeedbackEnabled(enable);
}

void StylusManager::setVisualFeedbackColor(int color) {
    // In a real implementation, this would set the visual feedback color
    LOGI("Visual feedback color set to: %d", color);
}

int StylusManager::getVisualFeedbackColor() const {
    // In a real implementation, this would return the visual feedback color
    return 0xFF0000FF; // Blue
}

void StylusManager::setVisualFeedbackSize(float size) {
    // In a real implementation, this would set the visual feedback size
    LOGI("Visual feedback size set to: %.2f", size);
}

float StylusManager::getVisualFeedbackSize() const {
    // In a real implementation, this would return the visual feedback size
    return 1.0f;
}

void StylusManager::registerStylusEventCallback(const std::string& id, StylusEventCallback callback) {
    eventCallbacks_[id] = callback;
    LOGI("Stylus event callback registered: %s", id.c_str());
}

void StylusManager::unregisterStylusEventCallback(const std::string& id) {
    eventCallbacks_.erase(id);
    LOGI("Stylus event callback unregistered: %s", id.c_str());
}

void StylusManager::registerStylusGestureCallback(const std::string& id, StylusGestureCallback callback) {
    gestureCallbacks_[id] = callback;
    LOGI("Stylus gesture callback registered: %s", id.c_str());
}

void StylusManager::unregisterStylusGestureCallback(const std::string& id) {
    gestureCallbacks_.erase(id);
    LOGI("Stylus gesture callback unregistered: %s", id.c_str());
}

void StylusManager::registerStylusButtonCallback(const std::string& id, StylusButtonCallback callback) {
    buttonCallbacks_[id] = callback;
    LOGI("Stylus button callback registered: %s", id.c_str());
}

void StylusManager::unregisterStylusButtonCallback(const std::string& id) {
    buttonCallbacks_.erase(id);
    LOGI("Stylus button callback unregistered: %s", id.c_str());
}

void StylusManager::registerStylusCalibrationCallback(const std::string& id, StylusCalibrationCallback callback) {
    calibrationCallbacks_[id] = callback;
    LOGI("Stylus calibration callback registered: %s", id.c_str());
}

void StylusManager::unregisterStylusCalibrationCallback(const std::string& id) {
    calibrationCallbacks_.erase(id);
    LOGI("Stylus calibration callback unregistered: %s", id.c_str());
}

void StylusManager::registerStylusCapabilitiesCallback(const std::string& id, StylusCapabilitiesCallback callback) {
    capabilitiesCallbacks_[id] = callback;
    LOGI("Stylus capabilities callback registered: %s", id.c_str());
}

void StylusManager::unregisterStylusCapabilitiesCallback(const std::string& id) {
    capabilitiesCallbacks_.erase(id);
    LOGI("Stylus capabilities callback unregistered: %s", id.c_str());
}

bool StylusManager::isStylusSupported() const {
    return stylusPresent_ && stylusType_ != StylusType::NONE;
}

std::string StylusManager::getStylusStatus() const {
    std::stringstream status;
    status << "Stylus Status:\n";
    status << "Present: " << (stylusPresent_ ? "YES" : "NO") << "\n";
    status << "Active: " << (stylusActive_ ? "YES" : "NO") << "\n";
    status << "Type: " << static_cast<int>(stylusType_) << "\n";
    status << "Mode: " << static_cast<int>(currentMode_) << "\n";
    status << "Pressure: " << (capabilities_.hasPressure ? "YES" : "NO") << "\n";
    status << "Tilt: " << (capabilities_.hasTilt ? "YES" : "NO") << "\n";
    status << "Eraser: " << (capabilities_.hasEraser ? "YES" : "NO") << "\n";
    status << "Palm Rejection: " << (capabilities_.hasPalmRejection ? "YES" : "NO") << "\n";
    status << "Calibrated: " << (calibration_.isCalibrated ? "YES" : "NO") << "\n";
    return status.str();
}

void StylusManager::resetStylusState() {
    LOGI("Resetting stylus state");

    stylusActive_ = false;
    currentEvent_ = StylusEvent();
    lastPosition_ = StylusPosition();
    lastPressure_ = StylusPressure();
    lastTilt_ = StylusTilt();
    buttonStates_.clear();
    strokePoints_.clear();
    isDrawing_ = false;

    LOGI("Stylus state reset");
}

void StylusManager::testStylusFunctionality() {
    LOGI("Testing stylus functionality");

    // Test basic functionality
    if (isStylusSupported()) {
        LOGI("Stylus is supported");

        if (hasPressureSupport()) {
            LOGI("Pressure support detected");
        }

        if (hasTiltSupport()) {
            LOGI("Tilt support detected");
        }

        if (hasEraserSupport()) {
            LOGI("Eraser support detected");
        }

        if (hasPalmRejection()) {
            LOGI("Palm rejection support detected");
        }
    } else {
        LOGI("No stylus support detected");
    }

    LOGI("Stylus functionality test completed");
}

void StylusManager::enableHoverMode(bool enable) {
    // In a real implementation, this would enable/disable hover mode
    LOGI("Hover mode %s", enable ? "enabled" : "disabled");
}

bool StylusManager::isHoverModeEnabled() const {
    // In a real implementation, this would return hover mode status
    return capabilities_.hasHover;
}

void StylusManager::setHoverDistance(float distance) {
    // In a real implementation, this would set hover distance
    LOGI("Hover distance set to: %.2f", distance);
}

float StylusManager::getHoverDistance() const {
    // In a real implementation, this would return hover distance
    return 10.0f; // 10mm
}

void StylusManager::defineCustomGesture(const std::string& name, const std::vector<StylusPosition>& points) {
    if (gestureRecognizer_) {
        gestureRecognizer_->defineCustomGesture(name, points);
    }
    LOGI("Custom gesture defined: %s", name.c_str());
}

void StylusManager::removeCustomGesture(const std::string& name) {
    if (gestureRecognizer_) {
        gestureRecognizer_->removeCustomGesture(name);
    }
    LOGI("Custom gesture removed: %s", name.c_str());
}

std::vector<std::string> StylusManager::getCustomGestures() const {
    if (gestureRecognizer_) {
        return gestureRecognizer_->getCustomGestures();
    }
    return std::vector<std::string>();
}

bool StylusManager::recognizeCustomGesture(const std::string& name, const std::vector<StylusPosition>& points) {
    if (gestureRecognizer_) {
        return gestureRecognizer_->recognizeCustomGesture(name, points);
    }
    return false;
}

bool StylusManager::exportStylusData(const std::string& filename) {
    LOGI("Exporting stylus data to: %s", filename.c_str());

    // In a real implementation, this would export stylus data
    LOGI("Stylus data exported: %s", filename.c_str());
    return true;
}

bool StylusManager::importStylusData(const std::string& filename) {
    LOGI("Importing stylus data from: %s", filename.c_str());

    // In a real implementation, this would import stylus data
    LOGI("Stylus data imported: %s", filename.c_str());
    return true;
}

void StylusManager::clearStylusData() {
    LOGI("Clearing stylus data");

    // Clear all data
    resetStylusState();
    if (calibrationManager_) {
        calibrationManager_->resetCalibration();
    }

    LOGI("Stylus data cleared");
}

void StylusManager::initializeDefaults() {
    LOGI("Initializing stylus defaults");

    // Initialize default settings
    settings_.enabled = true;
    settings_.defaultMode = StylusMode::DRAWING;
    settings_.pressureSensitivity = 1.0f;
    settings_.tiltSensitivity = 1.0f;
    settings_.palmRejectionEnabled = true;
    settings_.gestureRecognitionEnabled = true;
    settings_.hapticFeedbackEnabled = true;
    settings_.visualFeedbackEnabled = true;
    settings_.doubleTapThreshold = 300; // 300ms
    settings_.longPressThreshold = 1000; // 1000ms
    settings_.flickThreshold = 1000.0f; // 1000 units/second
    settings_.enableSmoothing = true;
    settings_.enableStabilization = true;
    settings_.predictionSamples = 5;
    settings_.predictionStrength = 0.5f;

    // Initialize default drawing parameters
    drawingParams_.brushSize = 10.0f;
    drawingParams_.pressureMultiplier = 2.0f;
    drawingParams_.tiltMultiplier = 1.5f;
    drawingParams_.opacity = 1.0f;
    drawingParams_.pressureOpacity = 1.0f;
    drawingParams_.tiltOpacity = 0.5f;
    drawingParams_.smoothing = 0.8f;
    drawingParams_.stabilization = 0.6f;
    drawingParams_.usePressure = true;
    drawingParams_.useTilt = true;
    drawingParams_.useVelocity = false;
    drawingParams_.brushType = "round";

    // Initialize default calibration
    calibration_.pressureMin = 0.0f;
    calibration_.pressureMax = 1.0f;
    calibration_.tiltOffsetX = 0.0f;
    calibration_.tiltOffsetY = 0.0f;
    calibration_.positionOffsetX = 0.0f;
    calibration_.positionOffsetY = 0.0f;
    calibration_.isCalibrated = false;

    // Initialize default pressure curve (linear)
    for (int i = 0; i < 10; i++) {
        calibration_.pressureCurve[i] = i / 9.0f;
    }

    LOGI("Stylus defaults initialized");
}

void StylusManager::startServiceThreads() {
    LOGI("Starting stylus service threads");

    serviceRunning_ = true;
    eventThread_ = std::thread(&StylusManager::eventThreadLoop, this);
    gestureThread_ = std::thread(&StylusManager::gestureThreadLoop, this);
    calibrationThread_ = std::thread(&StylusManager::calibrationThreadLoop, this);

    LOGI("Stylus service threads started");
}

void StylusManager::stopServiceThreads() {
    LOGI("Stopping stylus service threads");

    serviceRunning_ = false;
    eventCondition_.notify_all();

    if (eventThread_.joinable()) {
        eventThread_.join();
    }

    if (gestureThread_.joinable()) {
        gestureThread_.join();
    }

    if (calibrationThread_.joinable()) {
        calibrationThread_.join();
    }

    LOGI("Stylus service threads stopped");
}

void StylusManager::eventThreadLoop() {
    LOGI("Stylus event thread started");

    while (serviceRunning_) {
        std::unique_lock<std::mutex> lock(eventMutex_);

        eventCondition_.wait_for(lock, std::chrono::milliseconds(10));

        if (!eventQueue_.empty()) {
            processStylusEvents();
        }
    }

    LOGI("Stylus event thread ended");
}

void StylusManager::gestureThreadLoop() {
    LOGI("Stylus gesture thread started");

    while (serviceRunning_) {
        if (gestureRecognizer_) {
            // Process gestures periodically
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    LOGI("Stylus gesture thread ended");
}

void StylusManager::calibrationThreadLoop() {
    LOGI("Stylus calibration thread started");

    while (serviceRunning_) {
        if (autoCalibration_ && calibrationManager_) {
            // Perform auto-calibration checks
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    LOGI("Stylus calibration thread ended");
}

void StylusManager::onStylusEvent(const StylusEvent& event) {
    // Update current event
    currentEvent_ = event;

    // Update position, pressure, and tilt
    lastPosition_ = event.position;
    lastPressure_ = event.pressure;
    lastTilt_ = event.tilt;

    // Update button states
    updateButtonStates(event.buttons);

    // Process based on event type
    switch (event.type) {
        case StylusEventType::STYLUS_DOWN:
            startStroke(event);
            break;
        case StylusEventType::STYLUS_MOVE:
            updateStroke(event);
            break;
        case StylusEventType::STYLUS_UP:
            endStroke(event);
            break;
        case StylusEventType::PRESSURE_CHANGE:
            updatePressure(event.pressure);
            break;
        case StylusEventType::TILT_CHANGE:
            updateTilt(event.tilt);
            break;
        case StylusEventType::BUTTON_DOWN:
        case StylusEventType::BUTTON_UP:
            onStylusButton(StylusButton::PRIMARY, event.type == StylusEventType::BUTTON_DOWN);
            break;
        case StylusEventType::PALM_TOUCH:
            processPalmRejection();
            break;
        case StylusEventType::GESTURE_START:
        case StylusEventType::GESTURE_END:
        case StylusEventType::GESTURE_RECOGNIZED:
            onStylusGesture(StylusGesture::TAP, event);
            break;
        default:
            break;
    }

    // Call registered callbacks
    for (const auto& pair : eventCallbacks_) {
        pair.second(event);
    }
}

void StylusManager::onStylusGesture(StylusGesture gesture, const StylusEvent& event) {
    // Call registered callbacks
    for (const auto& pair : gestureCallbacks_) {
        pair.second(gesture, event);
    }
}

void StylusManager::onStylusButton(StylusButton button, bool pressed) {
    // Call registered callbacks
    for (const auto& pair : buttonCallbacks_) {
        pair.second(button, pressed);
    }
}

void StylusManager::onCalibrationComplete(bool success) {
    // Call registered callbacks
    for (const auto& pair : calibrationCallbacks_) {
        pair.second(success);
    }
}

void StylusManager::onCapabilitiesDetected(const StylusCapabilities& capabilities) {
    capabilities_ = capabilities;

    // Call registered callbacks
    for (const auto& pair : capabilitiesCallbacks_) {
        pair.second(capabilities);
    }
}

void StylusManager::startStroke(const StylusEvent& event) {
    LOGI("Starting stroke");

    isDrawing_ = true;
    strokePoints_.clear();
    strokeStartTime_ = std::chrono::steady_clock::now();

    // Add first point
    strokePoints_.push_back(event.position);

    // Trigger haptic feedback if enabled
    if (settings_.hapticFeedbackEnabled) {
        triggerHapticFeedback(0.5f, 30);
    }
}

void StylusManager::updateStroke(const StylusEvent& event) {
    if (!isDrawing_) return;

    // Add point to stroke
    strokePoints_.push_back(event.position);

    // Apply smoothing and stabilization if enabled
    if (settings_.enableSmoothing) {
        applySmoothing();
    }

    if (settings_.enableStabilization) {
        applyStabilization();
    }
}

void StylusManager::endStroke(const StylusEvent& event) {
    LOGI("Ending stroke");

    if (!isDrawing_) return;

    // Add final point
    strokePoints_.push_back(event.position);

    // Process final stroke
    processStrokePoints();

    isDrawing_ = false;
    strokePoints_.clear();

    // Trigger haptic feedback if enabled
    if (settings_.hapticFeedbackEnabled) {
        triggerHapticFeedback(0.3f, 20);
    }
}

void StylusManager::processStrokePoints() {
    // In a real implementation, this would process the stroke points
    // for drawing, gesture recognition, etc.
    LOGI("Processing stroke with %zu points", strokePoints_.size());
}

void StylusManager::applySmoothing() {
    // In a real implementation, this would apply smoothing to the stroke
    if (strokePoints_.size() < 3) return;

    // Simple moving average smoothing
    std::vector<StylusPosition> smoothed = strokePoints_;

    for (size_t i = 1; i < strokePoints_.size() - 1; i++) {
        smoothed[i].x = (strokePoints_[i-1].x + strokePoints_[i].x + strokePoints_[i+1].x) / 3.0f;
        smoothed[i].y = (strokePoints_[i-1].y + strokePoints_[i].y + strokePoints_[i+1].y) / 3.0f;
    }

    strokePoints_ = smoothed;
}

void StylusManager::applyStabilization() {
    // In a real implementation, this would apply stabilization to the stroke
    if (strokePoints_.size() < 2) return;

    // Simple stabilization by averaging recent points
    const int stabilizationWindow = 3;
    if (strokePoints_.size() >= stabilizationWindow) {
        size_t start = strokePoints_.size() - stabilizationWindow;
        float avgX = 0.0f, avgY = 0.0f;

        for (size_t i = start; i < strokePoints_.size(); i++) {
            avgX += strokePoints_[i].x;
            avgY += strokePoints_[i].y;
        }

        avgX /= stabilizationWindow;
        avgY /= stabilizationWindow;

        strokePoints_.back().x = avgX;
        strokePoints_.back().y = avgY;
    }
}

void StylusManager::updatePressure(const StylusPressure& pressure) {
    if (pressureProcessor_) {
        pressureProcessor_->processPressure(pressure);
    }
}

void StylusManager::normalizePressure() {
    // In a real implementation, this would normalize pressure values
    if (pressureProcessor_) {
        // Normalization is handled by the pressure processor
    }
}

void StylusManager::applyPressureCurve() {
    // In a real implementation, this would apply the pressure curve
    if (pressureProcessor_) {
        // Curve application is handled by the pressure processor
    }
}

void StylusManager::detectPressureLevel() {
    // In a real implementation, this would detect pressure levels
    if (pressureProcessor_) {
        // Level detection is handled by the pressure processor
    }
}

void StylusManager::updateTilt(const StylusTilt& tilt) {
    if (tiltProcessor_) {
        tiltProcessor_->processTilt(tilt);
    }
}

void StylusManager::normalizeTilt() {
    // In a real implementation, this would normalize tilt values
    if (tiltProcessor_) {
        // Normalization is handled by the tilt processor
    }
}

void StylusManager::calculateTiltDirection() {
    // In a real implementation, this would calculate tilt direction
    if (tiltProcessor_) {
        // Direction calculation is handled by the tilt processor
    }
}

void StylusManager::applyTiltOffset() {
    // In a real implementation, this would apply tilt offset
    if (tiltProcessor_) {
        // Offset application is handled by the tilt processor
    }
}

void StylusManager::processPalmRejection() {
    if (palmRejectionManager_) {
        // Palm rejection processing is handled by the palm rejection manager
    }
}

void StylusManager::detectPalmTouch() {
    // In a real implementation, this would detect palm touches
    if (palmRejectionManager_) {
        // Palm detection is handled by the palm rejection manager
    }
}

void StylusManager::rejectPalmInput() {
    // In a real implementation, this would reject palm input
    if (palmRejectionManager_) {
        // Palm rejection is handled by the palm rejection manager
    }
}

void StylusManager::processGestures() {
    if (gestureRecognizer_) {
        // Gesture processing is handled by the gesture recognizer
    }
}

void StylusManager::recognizeGestures() {
    if (gestureRecognizer_) {
        // Gesture recognition is handled by the gesture recognizer
    }
}

void StylusManager::validateGesture(StylusGesture gesture) {
    // In a real implementation, this would validate gestures
    LOGI("Validating gesture: %d", static_cast<int>(gesture));
}

void StylusManager::performCalibration() {
    if (calibrationManager_) {
        // Calibration is handled by the calibration manager
    }
}

void StylusManager::validateCalibration() {
    // In a real implementation, this would validate calibration
    LOGI("Validating calibration");
}

void StylusManager::applyCalibration() {
    // In a real implementation, this would apply calibration
    LOGI("Applying calibration");
}

void StylusManager::updateAdaptiveSensitivity() {
    // In a real implementation, this would update adaptive sensitivity
    LOGI("Updating adaptive sensitivity");
}

void StylusManager::saveSettingsToStorage() {
    // In a real implementation, this would save settings to storage
    LOGI("Saving settings to storage");
}

void StylusManager::loadSettingsFromStorage() {
    // In a real implementation, this would load settings from storage
    LOGI("Loading settings from storage");
}

void StylusManager::applySettings() {
    // Apply settings to sub-managers
    if (pressureProcessor_) {
        pressureProcessor_->setSensitivity(settings_.pressureSensitivity);
    }

    if (tiltProcessor_) {
        tiltProcessor_->setSensitivity(settings_.tiltSensitivity);
    }

    if (palmRejectionManager_) {
        palmRejectionManager_->enable(settings_.palmRejectionEnabled);
    }

    if (gestureRecognizer_) {
        gestureRecognizer_->enable(settings_.gestureRecognitionEnabled);
    }

    LOGI("Settings applied");
}

bool StylusManager::isValidStylusEvent(const StylusEvent& event) {
    return event.position.isValid && event.type != StylusEventType::STYLUS_UP;
}

bool StylusManager::isValidPressure(const StylusPressure& pressure) {
    return pressure.isValid && pressure.current >= 0.0f && pressure.current <= 1.0f;
}

bool StylusManager::isValidTilt(const StylusTilt& tilt) {
    return tilt.isValid && tilt.x >= -90.0f && tilt.x <= 90.0f &&
           tilt.y >= -90.0f && tilt.y <= 90.0f;
}

float StylusManager::calculateDistance(const StylusPosition& p1, const StylusPosition& p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return std::sqrt(dx * dx + dy * dy);
}

float StylusManager::calculateAngle(const StylusPosition& p1, const StylusPosition& p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return std::atan2(dy, dx) * 180.0f / M_PI;
}

void StylusManager::updateButtonStates(const std::vector<StylusButtonState>& buttons) {
    buttonStates_ = buttons;
}

StylusButtonState StylusManager::getButtonState(StylusButton button) const {
    for (const auto& state : buttonStates_) {
        if (state.button == button) {
            return state;
        }
    }
    return StylusButtonState();
}

// ========== PRESSURE PROCESSOR IMPLEMENTATION ==========
PressureProcessor::PressureProcessor(StylusManager* manager) : manager_(manager), currentPressure_(0.0f),
                                                              normalizedPressure_(0.0f), currentLevel_(PressureLevel::LIGHT),
                                                              sensitivity_(1.0f) {
    LOGI("PressureProcessor constructor called");

    // Initialize default pressure curve (linear)
    for (int i = 0; i < 10; i++) {
        curve_[i] = i / 9.0f;
    }
}

PressureProcessor::~PressureProcessor() {
    shutdown();
    LOGI("PressureProcessor destructor called");
}

bool PressureProcessor::initialize() {
    LOGI("Initializing Pressure Processor");
    return true;
}

void PressureProcessor::shutdown() {
    LOGI("Shutting down Pressure Processor");

    std::lock_guard<std::mutex> lock(pressureMutex_);
    pressureHistory_.clear();
    levelHistory_.clear();
}

void PressureProcessor::processPressure(const StylusPressure& pressure) {
    std::lock_guard<std::mutex> lock(pressureMutex_);

    currentPressure_ = pressure.current;
    normalizedPressure_ = currentPressure_ * sensitivity_;
    normalizedPressure_ = std::max(0.0f, std::min(1.0f, normalizedPressure_));

    // Apply pressure curve
    applyPressureCurve();

    // Detect pressure level
    detectPressureLevel();

    // Update history
    updatePressureHistory();
}

void PressureProcessor::setSensitivity(float sensitivity) {
    sensitivity_ = std::max(0.1f, std::min(5.0f, sensitivity));
    LOGI("Pressure sensitivity set to: %.2f", sensitivity_);
}

void PressureProcessor::setPressureCurve(const float curve[10]) {
    std::lock_guard<std::mutex> lock(pressureMutex_);

    for (int i = 0; i < 10; i++) {
        curve_[i] = curve[i];
    }

    LOGI("Pressure curve updated");
}

void PressureProcessor::applySmoothing(float factor) {
    // In a real implementation, this would apply smoothing
    LOGI("Applying pressure smoothing: %.2f", factor);
}

void PressureProcessor::applyDeadzone(float min, float max) {
    // In a real implementation, this would apply deadzone
    LOGI("Applying pressure deadzone: %.2f - %.2f", min, max);
}

void PressureProcessor::enablePressurePrediction(bool enable) {
    // In a real implementation, this would enable pressure prediction
    LOGI("Pressure prediction %s", enable ? "enabled" : "disabled");
}

void PressureProcessor::setPredictionStrength(float strength) {
    // In a real implementation, this would set prediction strength
    LOGI("Pressure prediction strength set to: %.2f", strength);
}

void PressureProcessor::normalizePressure() {
    // Normalization is already done in processPressure
}

void PressureProcessor::applyPressureCurve() {
    // Apply the pressure curve
    int index = static_cast<int>(normalizedPressure_ * 9.0f);
    index = std::max(0, std::min(9, index));

    float fraction = normalizedPressure_ * 9.0f - index;
    normalizedPressure_ = curve_[index] * (1.0f - fraction) + curve_[index + 1] * fraction;
}

void PressureProcessor::detectPressureLevel() {
    if (normalizedPressure_ < 0.25f) {
        currentLevel_ = PressureLevel::LIGHT;
    } else if (normalizedPressure_ < 0.75f) {
        currentLevel_ = PressureLevel::MEDIUM;
    } else {
        currentLevel_ = PressureLevel::HEAVY;
    }
}

void PressureProcessor::updatePressureHistory() {
    pressureHistory_.push_back(currentPressure_);
    levelHistory_.push_back(currentLevel_);

    // Limit history size
    if (pressureHistory_.size() > 100) {
        pressureHistory_.erase(pressureHistory_.begin());
        levelHistory_.erase(levelHistory_.begin());
    }
}

void PressureProcessor::predictPressure() {
    // In a real implementation, this would predict future pressure values
    LOGI("Predicting pressure");
}

// ========== TILT PROCESSOR IMPLEMENTATION ==========
TiltProcessor::TiltProcessor(StylusManager* manager) : manager_(manager), tiltX_(0.0f), tiltY_(0.0f),
                                                      tiltAngle_(0.0f), direction_(TiltDirection::NORTH),
                                                      sensitivity_(1.0f), offsetX_(0.0f), offsetY_(0.0f) {
    LOGI("TiltProcessor constructor called");
}

TiltProcessor::~TiltProcessor() {
    shutdown();
    LOGI("TiltProcessor destructor called");
}

bool TiltProcessor::initialize() {
    LOGI("Initializing Tilt Processor");
    return true;
}

void TiltProcessor::shutdown() {
    LOGI("Shutting down Tilt Processor");

    std::lock_guard<std::mutex> lock(tiltMutex_);
    tiltHistoryX_.clear();
    tiltHistoryY_.clear();
    directionHistory_.clear();
}

void TiltProcessor::processTilt(const StylusTilt& tilt) {
    std::lock_guard<std::mutex> lock(tiltMutex_);

    tiltX_ = tilt.x * sensitivity_ + offsetX_;
    tiltY_ = tilt.y * sensitivity_ + offsetY_;

    // Normalize tilt values
    normalizeTilt();

    // Calculate tilt angle
    tiltAngle_ = std::sqrt(tiltX_ * tiltX_ + tiltY_ * tiltY_);

    // Calculate tilt direction
    calculateTiltDirection();

    // Update history
    updateTiltHistory();
}

void TiltProcessor::setSensitivity(float sensitivity) {
    sensitivity_ = std::max(0.1f, std::min(5.0f, sensitivity));
    LOGI("Tilt sensitivity set to: %.2f", sensitivity_);
}

void TiltProcessor::setOffset(float offsetX, float offsetY) {
    offsetX_ = offsetX;
    offsetY_ = offsetY;
    LOGI("Tilt offset set to: %.2f, %.2f", offsetX_, offsetY_);
}

void TiltProcessor::applySmoothing(float factor) {
    // In a real implementation, this would apply smoothing
    LOGI("Applying tilt smoothing: %.2f", factor);
}

void TiltProcessor::enableTiltPrediction(bool enable) {
    // In a real implementation, this would enable tilt prediction
    LOGI("Tilt prediction %s", enable ? "enabled" : "disabled");
}

void TiltProcessor::setPredictionStrength(float strength) {
    // In a real implementation, this would set prediction strength
    LOGI("Tilt prediction strength set to: %.2f", strength);
}

void TiltProcessor::calibrateTilt() {
    // In a real implementation, this would calibrate tilt
    LOGI("Calibrating tilt");
}

void TiltProcessor::normalizeTilt() {
    // Normalize tilt values to valid ranges
    tiltX_ = std::max(-90.0f, std::min(90.0f, tiltX_));
    tiltY_ = std::max(-90.0f, std::min(90.0f, tiltY_));
}

void TiltProcessor::calculateTiltDirection() {
    // Calculate primary tilt direction based on X and Y components
    if (std::abs(tiltX_) > std::abs(tiltY_)) {
        if (tiltX_ > 0) {
            direction_ = TiltDirection::EAST;
        } else {
            direction_ = TiltDirection::WEST;
        }
    } else {
        if (tiltY_ > 0) {
            direction_ = TiltDirection::SOUTH;
        } else {
            direction_ = TiltDirection::NORTH;
        }
    }
}

void TiltProcessor::updateTiltHistory() {
    tiltHistoryX_.push_back(tiltX_);
    tiltHistoryY_.push_back(tiltY_);
    directionHistory_.push_back(direction_);

    // Limit history size
    if (tiltHistoryX_.size() > 100) {
        tiltHistoryX_.erase(tiltHistoryX_.begin());
        tiltHistoryY_.erase(tiltHistoryY_.begin());
        directionHistory_.erase(directionHistory_.begin());
    }
}

void TiltProcessor::predictTilt() {
    // In a real implementation, this would predict future tilt values
    LOGI("Predicting tilt");
}

// ========== PALM REJECTION MANAGER IMPLEMENTATION ==========
PalmRejectionManager::PalmRejectionManager(StylusManager* manager) : manager_(manager), enabled_(true),
                                                                    palmDetected_(false), sensitivity_(1.0f),
                                                                    palmSizeThreshold_(100.0f), stylusPalmDistance_(50.0f),
                                                                    rejectionTimeout_(1000) {
    LOGI("PalmRejectionManager constructor called");
}

PalmRejectionManager::~PalmRejectionManager() {
    shutdown();
    LOGI("PalmRejectionManager destructor called");
}

bool PalmRejectionManager::initialize() {
    LOGI("Initializing Palm Rejection Manager");
    return true;
}

void PalmRejectionManager::shutdown() {
    LOGI("Shutting down Palm Rejection Manager");

    std::lock_guard<std::mutex> lock(palmMutex_);
    palmPoints_.clear();
    stylusPoints_.clear();
}

void PalmRejectionManager::enable(bool enable) {
    enabled_ = enable;
    LOGI("Palm rejection %s", enabled_ ? "enabled" : "disabled");
}

void PalmRejectionManager::setSensitivity(float sensitivity) {
    sensitivity_ = std::max(0.1f, std::min(5.0f, sensitivity));
    LOGI("Palm rejection sensitivity set to: %.2f", sensitivity_);
}

void PalmRejectionManager::setPalmSizeThreshold(float threshold) {
    palmSizeThreshold_ = threshold;
    LOGI("Palm size threshold set to: %.2f", threshold);
}

void PalmRejectionManager::setStylusPalmDistance(float distance) {
    stylusPalmDistance_ = distance;
    LOGI("Stylus-palm distance set to: %.2f", distance);
}

void PalmRejectionManager::setRejectionTimeout(float timeout) {
    rejectionTimeout_ = timeout;
    LOGI("Rejection timeout set to: %.2f", timeout);
}

void PalmRejectionManager::processTouchInput(const StylusEvent& event) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(palmMutex_);

    if (event.isPalm) {
        palmPoints_.push_back(event.position);
        detectPalm();
    } else {
        stylusPoints_.push_back(event.position);
        updatePalmHistory(event);
    }
}

void PalmRejectionManager::detectPalm() {
    if (palmPoints_.empty()) return;

    float palmSize = calculatePalmSize(palmPoints_);

    if (palmSize > palmSizeThreshold_) {
        palmDetected_ = true;
        lastPalmTime_ = std::chrono::steady_clock::now();
        LOGI("Palm detected, size: %.2f", palmSize);
    }
}

void PalmRejectionManager::rejectPalmInput() {
    if (!palmDetected_) return;

    // In a real implementation, this would reject palm input
    LOGI("Rejecting palm input");

    // Clear palm points
    palmPoints_.clear();
}

void PalmRejectionManager::clearPalmData() {
    std::lock_guard<std::mutex> lock(palmMutex_);

    palmPoints_.clear();
    stylusPoints_.clear();
    palmDetected_ = false;

    LOGI("Palm data cleared");
}

bool PalmRejectionManager::isPalmTouch(const StylusEvent& event) {
    // In a real implementation, this would determine if a touch is from palm
    return event.isPalm;
}

float PalmRejectionManager::calculatePalmSize(const std::vector<StylusPosition>& points) {
    if (points.size() < 2) return 0.0f;

    // Calculate the bounding box size
    float minX = points[0].x, maxX = points[0].x;
    float minY = points[0].y, maxY = points[0].y;

    for (const auto& point : points) {
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }

    float width = maxX - minX;
    float height = maxY - minY;
    return width * height;
}

float PalmRejectionManager::calculateDistanceToStylus(const StylusPosition& palmPoint) {
    if (stylusPoints_.empty()) return std::numeric_limits<float>::max();

    float minDistance = std::numeric_limits<float>::max();

    for (const auto& stylusPoint : stylusPoints_) {
        float dx = palmPoint.x - stylusPoint.x;
        float dy = palmPoint.y - stylusPoint.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        minDistance = std::min(minDistance, distance);
    }

    return minDistance;
}

void PalmRejectionManager::updatePalmHistory(const StylusEvent& event) {
    // Update palm detection history
    if (event.isPalm) {
        palmPoints_.push_back(event.position);
    } else {
        stylusPoints_.push_back(event.position);
    }

    // Limit history size
    if (palmPoints_.size() > 50) {
        palmPoints_.erase(palmPoints_.begin(), palmPoints_.begin() + 10);
    }

    if (stylusPoints_.size() > 50) {
        stylusPoints_.erase(stylusPoints_.begin(), stylusPoints_.begin() + 10);
    }
}

void PalmRejectionManager::validatePalmDetection() {
    // In a real implementation, this would validate palm detection
    LOGI("Validating palm detection");
}

// ========== STYLUS CALIBRATION MANAGER IMPLEMENTATION ==========
StylusCalibrationManager::StylusCalibrationManager(StylusManager* manager) : manager_(manager), calibrating_(false),
                                                                          requiredSamples_(10), calibrationTimeout_(30.0f),
                                                                          autoCalibration_(true) {
    LOGI("StylusCalibrationManager constructor called");
}

StylusCalibrationManager::~StylusCalibrationManager() {
    shutdown();
    LOGI("StylusCalibrationManager destructor called");
}

bool StylusCalibrationManager::initialize() {
    LOGI("Initializing Stylus Calibration Manager");
    return true;
}

void StylusCalibrationManager::shutdown() {
    LOGI("Shutting down Stylus Calibration Manager");

    std::lock_guard<std::mutex> lock(calibrationMutex_);
    pressureSamples_.clear();
    tiltSamples_.clear();
    positionSamples_.clear();
}

void StylusCalibrationManager::startCalibration() {
    LOGI("Starting stylus calibration");

    calibrating_ = true;
    pressureSamples_.clear();
    tiltSamples_.clear();
    positionSamples_.clear();

    LOGI("Stylus calibration started");
}

void StylusCalibrationManager::stopCalibration() {
    LOGI("Stopping stylus calibration");

    calibrating_ = false;
    processCalibrationData();

    LOGI("Stylus calibration stopped");
}

float StylusCalibrationManager::getCalibrationProgress() const {
    if (!calibrating_) return 1.0f;

    int totalSamples = pressureSamples_.size() + tiltSamples_.size() + positionSamples_.size();
    int targetSamples = requiredSamples_ * 3; // 3 types of samples

    return std::min(1.0f, static_cast<float>(totalSamples) / targetSamples);
}

void StylusCalibrationManager::addPressureSample(const StylusPressure& pressure) {
    if (!calibrating_) return;

    std::lock_guard<std::mutex> lock(calibrationMutex_);
    pressureSamples_.push_back(pressure);

    if (pressureSamples_.size() >= requiredSamples_) {
        calculatePressureCalibration();
    }
}

void StylusCalibrationManager::addTiltSample(const StylusTilt& tilt) {
    if (!calibrating_) return;

    std::lock_guard<std::mutex> lock(calibrationMutex_);
    tiltSamples_.push_back(tilt);

    if (tiltSamples_.size() >= requiredSamples_) {
        calculateTiltCalibration();
    }
}

void StylusCalibrationManager::addPositionSample(const StylusPosition& position) {
    if (!calibrating_) return;

    std::lock_guard<std::mutex> lock(calibrationMutex_);
    positionSamples_.push_back(position);

    if (positionSamples_.size() >= requiredSamples_) {
        calculatePositionCalibration();
    }
}

void StylusCalibrationManager::setRequiredSamples(int samples) {
    requiredSamples_ = std::max(5, samples);
    LOGI("Required calibration samples set to: %d", requiredSamples_);
}

void StylusCalibrationManager::setCalibrationTimeout(float timeout) {
    calibrationTimeout_ = timeout;
    LOGI("Calibration timeout set to: %.2f", timeout);
}

void StylusCalibrationManager::setAutoCalibration(bool enable) {
    autoCalibration_ = enable;
    LOGI("Auto calibration %s", enable ? "enabled" : "disabled");
}

void StylusCalibrationManager::resetCalibration() {
    LOGI("Resetting stylus calibration");

    calibrating_ = false;
    pressureSamples_.clear();
    tiltSamples_.clear();
    positionSamples_.clear();

    calibration_.pressureMin = 0.0f;
    calibration_.pressureMax = 1.0f;
    calibration_.tiltOffsetX = 0.0f;
    calibration_.tiltOffsetY = 0.0f;
    calibration_.positionOffsetX = 0.0f;
    calibration_.positionOffsetY = 0.0f;
    calibration_.isCalibrated = false;

    LOGI("Stylus calibration reset");
}

void StylusCalibrationManager::processCalibrationData() {
    LOGI("Processing calibration data");

    if (pressureSamples_.empty() && tiltSamples_.empty() && positionSamples_.empty()) {
        LOGW("No calibration data to process");
        return;
    }

    calculatePressureCalibration();
    calculateTiltCalibration();
    calculatePositionCalibration();

    validateCalibration();
    applyCalibration();

    LOGI("Calibration data processed");
}

void StylusCalibrationManager::calculatePressureCalibration() {
    if (pressureSamples_.empty()) return;

    float minPressure = std::numeric_limits<float>::max();
    float maxPressure = std::numeric_limits<float>::min();

    for (const auto& sample : pressureSamples_) {
        minPressure = std::min(minPressure, sample.current);
        maxPressure = std::max(maxPressure, sample.current);
    }

    calibration_.pressureMin = minPressure;
    calibration_.pressureMax = maxPressure;

    LOGI("Pressure calibration calculated: %.3f - %.3f", minPressure, maxPressure);
}

void StylusCalibrationManager::calculateTiltCalibration() {
    if (tiltSamples_.empty()) return;

    float avgTiltX = 0.0f;
    float avgTiltY = 0.0f;

    for (const auto& sample : tiltSamples_) {
        avgTiltX += sample.x;
        avgTiltY += sample.y;
    }

    avgTiltX /= tiltSamples_.size();
    avgTiltY /= tiltSamples_.size();

    calibration_.tiltOffsetX = -avgTiltX;
    calibration_.tiltOffsetY = -avgTiltY;

    LOGI("Tilt calibration calculated: offset (%.3f, %.3f)", avgTiltX, avgTiltY);
}

void StylusCalibrationManager::calculatePositionCalibration() {
    if (positionSamples_.empty()) return;

    float avgPosX = 0.0f;
    float avgPosY = 0.0f;

    for (const auto& sample : positionSamples_) {
        avgPosX += sample.x;
        avgPosY += sample.y;
    }

    avgPosX /= positionSamples_.size();
    avgPosY /= positionSamples_.size();

    calibration_.positionOffsetX = -avgPosX;
    calibration_.positionOffsetY = -avgPosY;

    LOGI("Position calibration calculated: offset (%.3f, %.3f)", avgPosX, avgPosY);
}

void StylusCalibrationManager::validateCalibration() {
    // In a real implementation, this would validate calibration data
    LOGI("Validating calibration");

    calibration_.isCalibrated = true;
    calibration_.lastCalibration = std::chrono::steady_clock::now();
}

void StylusCalibrationManager::applyCalibration() {
    // In a real implementation, this would apply calibration to the stylus manager
    LOGI("Applying calibration");
}

void StylusCalibrationManager::saveCalibrationToStorage() {
    // In a real implementation, this would save calibration to storage
    LOGI("Saving calibration to storage");
}

void StylusCalibrationManager::loadCalibrationFromStorage() {
    // In a real implementation, this would load calibration from storage
    LOGI("Loading calibration from storage");
}

// ========== GESTURE RECOGNIZER IMPLEMENTATION ==========
GestureRecognizer::GestureRecognizer(StylusManager* manager) : manager_(manager), enabled_(true),
                                                              currentGestureType_(StylusGesture::TAP) {
    LOGI("GestureRecognizer constructor called");
}

GestureRecognizer::~GestureRecognizer() {
    shutdown();
    LOGI("GestureRecognizer destructor called");
}

bool GestureRecognizer::initialize() {
    LOGI("Initializing Gesture Recognizer");

    // Initialize default gesture configurations
    GestureConfig tapConfig;
    tapConfig.gesture = StylusGesture::TAP;
    tapConfig.minDuration = 0;
    tapConfig.maxDuration = 200;
    tapConfig.minDistance = 0;
    tapConfig.maxDistance = 10;
    tapConfig.tolerance = 5;
    tapConfig.requirePressure = false;
    tapConfig.minPressure = 0.1f;
    tapConfig.enabled = true;

    gestureConfigs_[StylusGesture::TAP] = tapConfig;

    GestureConfig doubleTapConfig;
    doubleTapConfig.gesture = StylusGesture::DOUBLE_TAP;
    doubleTapConfig.minDuration = 0;
    doubleTapConfig.maxDuration = 500;
    doubleTapConfig.minDistance = 0;
    doubleTapConfig.maxDistance = 20;
    doubleTapConfig.tolerance = 10;
    doubleTapConfig.requirePressure = false;
    doubleTapConfig.minPressure = 0.1f;
    doubleTapConfig.enabled = true;

    gestureConfigs_[StylusGesture::DOUBLE_TAP] = doubleTapConfig;

    return true;
}

void GestureRecognizer::shutdown() {
    LOGI("Shutting down Gesture Recognizer");

    std::lock_guard<std::mutex> lock(gestureMutex_);
    currentGesture_.clear();
    customGestures_.clear();
    gestureConfigs_.clear();
}

void GestureRecognizer::enable(bool enable) {
    enabled_ = enable;
    LOGI("Gesture recognition %s", enabled_ ? "enabled" : "disabled");
}

void GestureRecognizer::processGesture(const StylusEvent& event) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(gestureMutex_);

    switch (event.type) {
        case StylusEventType::STYLUS_DOWN:
            startGesture(event);
            break;
        case StylusEventType::STYLUS_MOVE:
            updateGesture(event);
            break;
        case StylusEventType::STYLUS_UP:
            endGesture(event);
            break;
        default:
            break;
    }
}

StylusGesture GestureRecognizer::recognizeGesture(const std::vector<StylusPosition>& points) {
    if (points.size() < 2) return StylusGesture::TAP;

    // Calculate gesture characteristics
    float duration = 0.0f; // Would be calculated from timestamps
    float distance = 0.0f;
    float avgPressure = 0.0f;

    for (size_t i = 1; i < points.size(); i++) {
        distance += manager_->calculateDistance(points[i-1], points[i]);
    }

    // Simple gesture recognition based on characteristics
    if (distance < 50.0f) {
        return StylusGesture::TAP;
    } else if (distance < 200.0f) {
        return StylusGesture::DRAG;
    } else {
        return StylusGesture::FLICK;
    }
}

float GestureRecognizer::getGestureConfidence(StylusGesture gesture) const {
    // In a real implementation, this would return confidence for the gesture
    return 0.8f;
}

void GestureRecognizer::setGestureConfig(StylusGesture gesture, const GestureConfig& config) {
    std::lock_guard<std::mutex> lock(gestureMutex_);
    gestureConfigs_[gesture] = config;
    LOGI("Gesture config updated for: %d", static_cast<int>(gesture));
}

GestureConfig GestureRecognizer::getGestureConfig(StylusGesture gesture) const {
    std::lock_guard<std::mutex> lock(gestureMutex_);

    auto it = gestureConfigs_.find(gesture);
    if (it != gestureConfigs_.end()) {
        return it->second;
    }

    return GestureConfig();
}

void GestureRecognizer::enableGesture(StylusGesture gesture, bool enable) {
    std::lock_guard<std::mutex> lock(gestureMutex_);

    auto it = gestureConfigs_.find(gesture);
    if (it != gestureConfigs_.end()) {
        it->second.enabled = enable;
        LOGI("Gesture %d %s", static_cast<int>(gesture), enable ? "enabled" : "disabled");
    }
}

bool GestureRecognizer::isGestureEnabled(StylusGesture gesture) const {
    std::lock_guard<std::mutex> lock(gestureMutex_);

    auto it = gestureConfigs_.find(gesture);
    if (it != gestureConfigs_.end()) {
        return it->second.enabled;
    }

    return false;
}

void GestureRecognizer::defineCustomGesture(const std::string& name, const std::vector<StylusPosition>& points) {
    std::lock_guard<std::mutex> lock(gestureMutex_);
    customGestures_[name] = points;
    LOGI("Custom gesture defined: %s", name.c_str());
}

void GestureRecognizer::removeCustomGesture(const std::string& name) {
    std::lock_guard<std::mutex> lock(gestureMutex_);
    customGestures_.erase(name);
    LOGI("Custom gesture removed: %s", name.c_str());
}

bool GestureRecognizer::recognizeCustomGesture(const std::string& name, const std::vector<StylusPosition>& points) {
    std::lock_guard<std::mutex> lock(gestureMutex_);

    auto it = customGestures_.find(name);
    if (it == customGestures_.end()) {
        return false;
    }

    // Compare gestures using similarity calculation
    float similarity = calculateGestureSimilarity(points, it->second);
    return similarity > 0.8f; // 80% similarity threshold
}

std::vector<std::string> GestureRecognizer::getCustomGestures() const {
    std::lock_guard<std::mutex> lock(gestureMutex_);

    std::vector<std::string> names;
    for (const auto& pair : customGestures_) {
        names.push_back(pair.first);
    }

    return names;
}

void GestureRecognizer::startGesture(const StylusEvent& event) {
    currentGesture_.clear();
    currentGesture_.push_back(event.position);
    gestureStartTime_ = std::chrono::steady_clock::now();
    currentGestureType_ = StylusGesture::TAP;
}

void GestureRecognizer::updateGesture(const StylusEvent& event) {
    if (currentGesture_.empty()) return;

    currentGesture_.push_back(event.position);
}

void GestureRecognizer::endGesture(const StylusEvent& event) {
    if (currentGesture_.empty()) return;

    currentGesture_.push_back(event.position);

    // Recognize the gesture
    StylusGesture recognized = recognizeGesture(currentGesture_);

    // Validate the gesture
    validateGesture();

    // Notify manager
    StylusEvent gestureEvent = event;
    gestureEvent.gestureName = "recognized_gesture";
    manager_->onStylusGesture(recognized, gestureEvent);

    currentGesture_.clear();
}

void GestureRecognizer::validateGesture() {
    // In a real implementation, this would validate the gesture
    LOGI("Validating gesture");
}

bool GestureRecognizer::matchGesture(StylusGesture gesture, const std::vector<StylusPosition>& points) {
    // In a real implementation, this would match the gesture
    return true;
}

float GestureRecognizer::calculateGestureSimilarity(const std::vector<StylusPosition>& points1,
                                                  const std::vector<StylusPosition>& points2) {
    if (points1.size() != points2.size()) return 0.0f;

    float totalDistance = 0.0f;
    for (size_t i = 0; i < points1.size(); i++) {
        totalDistance += manager_->calculateDistance(points1[i], points2[i]);
    }

    float avgDistance = totalDistance / points1.size();
    return std::max(0.0f, 1.0f - avgDistance / 100.0f); // Normalize to 0-100 range
}

void GestureRecognizer::normalizeGesture(std::vector<StylusPosition>& points) {
    // In a real implementation, this would normalize the gesture
    LOGI("Normalizing gesture");
}

void GestureRecognizer::resampleGesture(std::vector<StylusPosition>& points, int targetPoints) {
    // In a real implementation, this would resample the gesture
    LOGI("Resampling gesture to %d points", targetPoints);
}

} // namespace FoundryEngine
