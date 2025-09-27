#ifndef FOUNDRYENGINE_STYLUS_MANAGER_H
#define FOUNDRYENGINE_STYLUS_MANAGER_H

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
class StylusManager;
class PressureProcessor;
class TiltProcessor;
class PalmRejectionManager;
class StylusCalibrationManager;
class GestureRecognizer;

// Stylus types
enum class StylusType {
    NONE,           // No stylus
    BASIC,          // Basic stylus without pressure
    PRESSURE,       // Pressure-sensitive stylus
    TILT,           // Tilt-aware stylus
    ERASER,         // Stylus with eraser functionality
    BUTTONS,        // Stylus with buttons
    ADVANCED        // Full-featured stylus with all capabilities
};

// Stylus button types
enum class StylusButton {
    PRIMARY,        // Primary button (usually barrel button)
    SECONDARY,      // Secondary button
    ERASER,         // Eraser button
    CUSTOM_1,       // Custom button 1
    CUSTOM_2        // Custom button 2
};

// Pressure sensitivity levels
enum class PressureLevel {
    LIGHT = 0,      // Light pressure (0-25%)
    MEDIUM = 1,     // Medium pressure (25-75%)
    HEAVY = 2,      // Heavy pressure (75-100%)
    CUSTOM = 3      // Custom pressure mapping
};

// Tilt direction
enum class TiltDirection {
    NORTH,          // North (up)
    NORTH_EAST,     // Northeast
    EAST,           // East (right)
    SOUTH_EAST,     // Southeast
    SOUTH,          // South (down)
    SOUTH_WEST,     // Southwest
    WEST,           // West (left)
    NORTH_WEST      // Northwest
};

// Stylus gesture types
enum class StylusGesture {
    TAP,            // Single tap
    DOUBLE_TAP,     // Double tap
    LONG_PRESS,     // Long press
    DRAG,           // Drag gesture
    FLICK,          // Quick flick
    CIRCLE,         // Circular gesture
    SCRIBBLE,       // Scribble gesture
    ERASE,          // Erase gesture
    CUSTOM          // Custom gesture
};

// Stylus input modes
enum class StylusMode {
    DRAWING,        // Drawing/painting mode
    ERASING,        // Erasing mode
    SELECTION,      // Selection mode
    NAVIGATION,     // Navigation mode
    TEXT_INPUT,     // Text input mode
    CUSTOM          // Custom mode
};

// Stylus event types
enum class StylusEventType {
    STYLUS_DOWN,    // Stylus touched down
    STYLUS_MOVE,    // Stylus moved
    STYLUS_UP,      // Stylus lifted up
    PRESSURE_CHANGE,// Pressure changed
    TILT_CHANGE,    // Tilt changed
    BUTTON_DOWN,    // Button pressed
    BUTTON_UP,      // Button released
    ERASER_DOWN,    // Eraser touched down
    ERASER_MOVE,    // Eraser moved
    ERASER_UP,      // Eraser lifted up
    PALM_TOUCH,     // Palm detected
    PALM_RELEASE,   // Palm released
    GESTURE_START,  // Gesture started
    GESTURE_END,    // Gesture ended
    GESTURE_RECOGNIZED // Gesture recognized
};

// Stylus pressure data
struct StylusPressure {
    float current;          // Current pressure (0.0 - 1.0)
    float min;              // Minimum pressure threshold
    float max;              // Maximum pressure threshold
    PressureLevel level;    // Current pressure level
    bool isValid;           // Whether pressure data is valid
    float normalized;       // Normalized pressure (0.0 - 1.0)
    float raw;              // Raw pressure value from hardware
};

// Stylus tilt data
struct StylusTilt {
    float x;                // X-axis tilt (-90 to +90 degrees)
    float y;                // Y-axis tilt (-90 to +90 degrees)
    float angle;            // Combined tilt angle
    TiltDirection direction;// Primary tilt direction
    bool isValid;           // Whether tilt data is valid
    float altitude;         // Altitude angle from surface
    float azimuth;          // Azimuth angle around surface normal
};

// Stylus button state
struct StylusButtonState {
    StylusButton button;    // Button type
    bool isPressed;         // Whether button is pressed
    float pressure;         // Button pressure (if supported)
    int clickCount;         // Number of clicks
    std::chrono::steady_clock::time_point lastPressTime;
    std::chrono::steady_clock::time_point lastReleaseTime;
};

// Stylus position data
struct StylusPosition {
    float x;                // X coordinate
    float y;                // Y coordinate
    float z;                // Z coordinate (if 3D input supported)
    float screenX;          // Screen X coordinate
    float screenY;          // Screen Y coordinate
    bool isValid;           // Whether position data is valid
    std::chrono::steady_clock::time_point timestamp;
    float velocityX;        // X velocity
    float velocityY;        // Y velocity
    float accelerationX;    // X acceleration
    float accelerationY;    // Y acceleration
};

// Stylus event data
struct StylusEvent {
    StylusEventType type;   // Event type
    StylusPosition position;// Position data
    StylusPressure pressure;// Pressure data
    StylusTilt tilt;        // Tilt data
    std::vector<StylusButtonState> buttons; // Button states
    StylusMode mode;        // Current stylus mode
    bool isEraser;          // Whether using eraser
    bool isPalm;            // Whether palm touch detected
    std::string gestureName;// Gesture name (if gesture event)
    float confidence;       // Event confidence (0.0 - 1.0)
    std::unordered_map<std::string, float> customData; // Custom event data
};

// Stylus capabilities
struct StylusCapabilities {
    bool hasPressure;       // Supports pressure sensitivity
    bool hasTilt;           // Supports tilt detection
    bool hasEraser;         // Has eraser functionality
    bool hasButtons;        // Has physical buttons
    bool hasHover;          // Supports hover detection
    bool hasPalmRejection;  // Supports palm rejection
    bool hasGestureRecognition; // Supports gesture recognition
    int maxPressureLevels;  // Maximum pressure levels
    int buttonCount;        // Number of buttons
    float pressureResolution; // Pressure resolution
    float tiltResolution;   // Tilt resolution in degrees
    std::vector<StylusGesture> supportedGestures; // Supported gestures
};

// Stylus calibration data
struct StylusCalibration {
    float pressureMin;      // Minimum pressure calibration
    float pressureMax;      // Maximum pressure calibration
    float tiltOffsetX;      // X tilt offset
    float tiltOffsetY;      // Y tilt offset
    float positionOffsetX;  // X position offset
    float positionOffsetY;  // Y position offset
    float pressureCurve[10]; // Pressure curve mapping
    bool isCalibrated;      // Whether stylus is calibrated
    std::chrono::steady_clock::time_point lastCalibration;
};

// Stylus gesture configuration
struct GestureConfig {
    StylusGesture gesture;  // Gesture type
    float minDuration;      // Minimum gesture duration (ms)
    float maxDuration;      // Maximum gesture duration (ms)
    float minDistance;      // Minimum gesture distance
    float maxDistance;      // Maximum gesture distance
    float tolerance;        // Gesture recognition tolerance
    bool requirePressure;   // Whether pressure is required
    float minPressure;      // Minimum pressure for gesture
    bool enabled;           // Whether gesture is enabled
};

// Stylus drawing parameters
struct DrawingParams {
    float brushSize;        // Base brush size
    float pressureMultiplier; // Pressure size multiplier
    float tiltMultiplier;   // Tilt size multiplier
    float opacity;          // Base opacity
    float pressureOpacity;  // Pressure opacity multiplier
    float tiltOpacity;      // Tilt opacity multiplier
    float smoothing;        // Stroke smoothing factor
    float stabilization;    // Stroke stabilization
    bool usePressure;       // Use pressure for size
    bool useTilt;           // Use tilt for rotation
    bool useVelocity;       // Use velocity for effects
    std::string brushType;  // Brush type name
};

// Stylus settings
struct StylusSettings {
    bool enabled;           // Whether stylus support is enabled
    StylusMode defaultMode; // Default stylus mode
    float pressureSensitivity; // Global pressure sensitivity (0.1 - 5.0)
    float tiltSensitivity;  // Global tilt sensitivity (0.1 - 5.0)
    bool palmRejectionEnabled; // Enable palm rejection
    bool gestureRecognitionEnabled; // Enable gesture recognition
    bool hapticFeedbackEnabled; // Enable haptic feedback
    bool visualFeedbackEnabled; // Enable visual feedback
    float doubleTapThreshold; // Double tap time threshold (ms)
    float longPressThreshold; // Long press time threshold (ms)
    float flickThreshold;   // Flick velocity threshold
    bool enableSmoothing;   // Enable stroke smoothing
    bool enableStabilization; // Enable stroke stabilization
    int predictionSamples;  // Number of prediction samples
    float predictionStrength; // Prediction strength (0.0 - 1.0)
};

// Callback types
using StylusEventCallback = std::function<void(const StylusEvent&)>;
using StylusGestureCallback = std::function<void(StylusGesture, const StylusEvent&)>;
using StylusButtonCallback = std::function<void(StylusButton, bool)>;
using StylusCalibrationCallback = std::function<void(bool)>;
using StylusCapabilitiesCallback = std::function<void(const StylusCapabilities&)>;

// ========== STYLUS MANAGER ==========
class StylusManager : public System {
private:
    static StylusManager* instance_;

    PressureProcessor* pressureProcessor_;
    TiltProcessor* tiltProcessor_;
    PalmRejectionManager* palmRejectionManager_;
    StylusCalibrationManager* calibrationManager_;
    GestureRecognizer* gestureRecognizer_;

    // JNI environment
    JNIEnv* env_;
    jobject context_;

    // Stylus state
    std::atomic<bool> initialized_;
    std::atomic<bool> stylusPresent_;
    std::atomic<bool> stylusActive_;
    StylusType stylusType_;
    StylusCapabilities capabilities_;
    StylusCalibration calibration_;
    StylusSettings settings_;

    // Current state
    StylusEvent currentEvent_;
    StylusPosition lastPosition_;
    StylusPressure lastPressure_;
    StylusTilt lastTilt_;
    std::vector<StylusButtonState> buttonStates_;
    StylusMode currentMode_;

    // Event system
    std::unordered_map<std::string, StylusEventCallback> eventCallbacks_;
    std::unordered_map<std::string, StylusGestureCallback> gestureCallbacks_;
    std::unordered_map<std::string, StylusButtonCallback> buttonCallbacks_;
    std::unordered_map<std::string, StylusCalibrationCallback> calibrationCallbacks_;
    std::unordered_map<std::string, StylusCapabilitiesCallback> capabilitiesCallbacks_;

    // Event queue
    std::queue<StylusEvent> eventQueue_;
    std::mutex eventMutex_;
    std::condition_variable eventCondition_;

    // Drawing state
    DrawingParams drawingParams_;
    bool isDrawing_;
    std::vector<StylusPosition> strokePoints_;
    std::chrono::steady_clock::time_point strokeStartTime_;

    // Service management
    std::atomic<bool> serviceRunning_;
    std::thread eventThread_;
    std::thread gestureThread_;
    std::thread calibrationThread_;

    // Settings
    bool autoCalibration_;
    bool adaptiveSensitivity_;
    float sensitivityUpdateInterval_;

public:
    StylusManager();
    ~StylusManager();

    static StylusManager* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // JNI setup
    void setJNIEnvironment(JNIEnv* env, jobject context);

    // Stylus detection and capabilities
    bool isStylusAvailable() const { return stylusPresent_; }
    bool isStylusActive() const { return stylusActive_; }
    StylusType getStylusType() const { return stylusType_; }
    StylusCapabilities getStylusCapabilities() const { return capabilities_; }
    bool hasPressureSupport() const { return capabilities_.hasPressure; }
    bool hasTiltSupport() const { return capabilities_.hasTilt; }
    bool hasEraserSupport() const { return capabilities_.hasEraser; }
    bool hasPalmRejection() const { return capabilities_.hasPalmRejection; }

    // Stylus calibration
    void startCalibration();
    void stopCalibration();
    bool isCalibrating() const;
    void resetCalibration();
    void saveCalibration();
    void loadCalibration();
    StylusCalibration getCalibration() const { return calibration_; }
    bool isCalibrated() const { return calibration_.isCalibrated; }

    // Stylus settings
    void setSettings(const StylusSettings& settings);
    StylusSettings getSettings() const { return settings_; }
    void setPressureSensitivity(float sensitivity);
    void setTiltSensitivity(float sensitivity);
    void setPalmRejectionEnabled(bool enabled);
    void setGestureRecognitionEnabled(bool enabled);
    void setHapticFeedbackEnabled(bool enabled);
    void setVisualFeedbackEnabled(bool enabled);

    // Mode management
    void setStylusMode(StylusMode mode);
    StylusMode getStylusMode() const { return currentMode_; }
    void setDrawingMode();
    void setErasingMode();
    void setSelectionMode();
    void setNavigationMode();
    void setTextInputMode();

    // Drawing parameters
    void setDrawingParams(const DrawingParams& params);
    DrawingParams getDrawingParams() const { return drawingParams_; }
    void setBrushSize(float size);
    void setPressureMultiplier(float multiplier);
    void setTiltMultiplier(float multiplier);
    void setOpacity(float opacity);
    void setSmoothing(float smoothing);
    void setStabilization(float stabilization);

    // Event handling
    void processStylusEvent(const StylusEvent& event);
    StylusEvent getCurrentStylusEvent() const { return currentEvent_; }
    bool hasActiveStroke() const { return isDrawing_; }
    std::vector<StylusPosition> getCurrentStroke() const { return strokePoints_; }

    // Gesture management
    void enableGesture(StylusGesture gesture, bool enable);
    void setGestureConfig(StylusGesture gesture, const GestureConfig& config);
    GestureConfig getGestureConfig(StylusGesture gesture) const;
    std::vector<StylusGesture> getSupportedGestures() const;
    bool isGestureEnabled(StylusGesture gesture) const;

    // Button management
    bool isButtonPressed(StylusButton button) const;
    int getButtonClickCount(StylusButton button) const;
    std::vector<StylusButtonState> getButtonStates() const { return buttonStates_; }

    // Advanced features
    void enableAdaptiveSensitivity(bool enable);
    void setPredictionSamples(int samples);
    void setPredictionStrength(float strength);
    void enableStrokeSmoothing(bool enable);
    void enableStrokeStabilization(bool enable);

    // Pressure processing
    float getNormalizedPressure() const;
    PressureLevel getPressureLevel() const;
    float getPressureSensitivity() const { return settings_.pressureSensitivity; }
    void setPressureCurve(const float curve[10]);
    float getPressureAtLevel(PressureLevel level) const;

    // Tilt processing
    float getTiltAngle() const;
    TiltDirection getTiltDirection() const;
    float getTiltSensitivity() const { return settings_.tiltSensitivity; }
    void setTiltOffset(float offsetX, float offsetY);
    std::pair<float, float> getTiltOffset() const;

    // Palm rejection
    void enablePalmRejection(bool enable);
    bool isPalmRejectionEnabled() const { return settings_.palmRejectionEnabled; }
    void setPalmRejectionSensitivity(float sensitivity);
    float getPalmRejectionSensitivity() const;

    // Haptic feedback
    void triggerHapticFeedback(float intensity = 1.0f, int duration = 50);
    void setHapticPattern(const std::vector<int>& pattern);
    std::vector<int> getHapticPattern() const;

    // Visual feedback
    void enableVisualFeedback(bool enable);
    bool isVisualFeedbackEnabled() const { return settings_.visualFeedbackEnabled; }
    void setVisualFeedbackColor(int color);
    int getVisualFeedbackColor() const;
    void setVisualFeedbackSize(float size);
    float getVisualFeedbackSize() const;

    // Callback management
    void registerStylusEventCallback(const std::string& id, StylusEventCallback callback);
    void unregisterStylusEventCallback(const std::string& id);
    void registerStylusGestureCallback(const std::string& id, StylusGestureCallback callback);
    void unregisterStylusGestureCallback(const std::string& id);
    void registerStylusButtonCallback(const std::string& id, StylusButtonCallback callback);
    void unregisterStylusButtonCallback(const std::string& id);
    void registerStylusCalibrationCallback(const std::string& id, StylusCalibrationCallback callback);
    void unregisterStylusCalibrationCallback(const std::string& id);
    void registerStylusCapabilitiesCallback(const std::string& id, StylusCapabilitiesCallback callback);
    void unregisterStylusCapabilitiesCallback(const std::string& id);

    // Utility functions
    bool isStylusSupported() const;
    std::string getStylusStatus() const;
    void resetStylusState();
    void testStylusFunctionality();

    // Advanced input
    void enableHoverMode(bool enable);
    bool isHoverModeEnabled() const;
    void setHoverDistance(float distance);
    float getHoverDistance() const;

    // Custom gestures
    void defineCustomGesture(const std::string& name, const std::vector<StylusPosition>& points);
    void removeCustomGesture(const std::string& name);
    std::vector<std::string> getCustomGestures() const;
    bool recognizeCustomGesture(const std::string& name, const std::vector<StylusPosition>& points);

    // Data export/import
    bool exportStylusData(const std::string& filename);
    bool importStylusData(const std::string& filename);
    void clearStylusData();

private:
    void initializeDefaults();
    void detectStylusCapabilities();
    void startServiceThreads();
    void stopServiceThreads();
    void eventThreadLoop();
    void gestureThreadLoop();
    void calibrationThreadLoop();

    // JNI helper methods
    void detectStylusJNI();
    void getStylusCapabilitiesJNI();
    void processStylusInputJNI();
    void calibrateStylusJNI();
    void setHapticFeedbackJNI(float intensity, int duration);

    // Event processing
    void onStylusEvent(const StylusEvent& event);
    void onStylusGesture(StylusGesture gesture, const StylusEvent& event);
    void onStylusButton(StylusButton button, bool pressed);
    void onCalibrationComplete(bool success);
    void onCapabilitiesDetected(const StylusCapabilities& capabilities);

    // Drawing and stroke processing
    void startStroke(const StylusEvent& event);
    void updateStroke(const StylusEvent& event);
    void endStroke(const StylusEvent& event);
    void processStrokePoints();
    void applySmoothing();
    void applyStabilization();

    // Pressure processing
    void updatePressure(const StylusPressure& pressure);
    void normalizePressure();
    void applyPressureCurve();
    void detectPressureLevel();

    // Tilt processing
    void updateTilt(const StylusTilt& tilt);
    void normalizeTilt();
    void calculateTiltDirection();
    void applyTiltOffset();

    // Palm rejection
    void processPalmRejection();
    void detectPalmTouch();
    void rejectPalmInput();

    // Gesture recognition
    void processGestures();
    void recognizeGestures();
    void validateGesture(StylusGesture gesture);

    // Calibration
    void performCalibration();
    void validateCalibration();
    void applyCalibration();
    void updateAdaptiveSensitivity();

    // Settings management
    void saveSettingsToStorage();
    void loadSettingsFromStorage();
    void applySettings();

    // Utility functions
    bool isValidStylusEvent(const StylusEvent& event);
    bool isValidPressure(const StylusPressure& pressure);
    bool isValidTilt(const StylusTilt& tilt);
    float calculateDistance(const StylusPosition& p1, const StylusPosition& p2);
    float calculateAngle(const StylusPosition& p1, const StylusPosition& p2);
    void updateButtonStates(const std::vector<StylusButtonState>& buttons);
    StylusButtonState getButtonState(StylusButton button) const;
};

// ========== PRESSURE PROCESSOR ==========
class PressureProcessor {
private:
    StylusManager* manager_;

    // Pressure state
    float currentPressure_;
    float normalizedPressure_;
    PressureLevel currentLevel_;
    float sensitivity_;
    float curve_[10];

    // Pressure history
    std::vector<float> pressureHistory_;
    std::vector<PressureLevel> levelHistory_;
    std::mutex pressureMutex_;

public:
    PressureProcessor(StylusManager* manager);
    ~PressureProcessor();

    bool initialize();
    void shutdown();

    // Pressure processing
    void processPressure(const StylusPressure& pressure);
    float getCurrentPressure() const { return currentPressure_; }
    float getNormalizedPressure() const { return normalizedPressure_; }
    PressureLevel getCurrentLevel() const { return currentLevel_; }

    // Settings
    void setSensitivity(float sensitivity);
    float getSensitivity() const { return sensitivity_; }
    void setPressureCurve(const float curve[10]);
    float* getPressureCurve() { return curve_; }

    // Advanced processing
    void applySmoothing(float factor);
    void applyDeadzone(float min, float max);
    void enablePressurePrediction(bool enable);
    void setPredictionStrength(float strength);

private:
    void normalizePressure();
    void applyPressureCurve();
    void detectPressureLevel();
    void updatePressureHistory();
    void predictPressure();
};

// ========== TILT PROCESSOR ==========
class TiltProcessor {
private:
    StylusManager* manager_;

    // Tilt state
    float tiltX_;
    float tiltY_;
    float tiltAngle_;
    TiltDirection direction_;
    float sensitivity_;
    float offsetX_;
    float offsetY_;

    // Tilt history
    std::vector<float> tiltHistoryX_;
    std::vector<float> tiltHistoryY_;
    std::vector<TiltDirection> directionHistory_;
    std::mutex tiltMutex_;

public:
    TiltProcessor(StylusManager* manager);
    ~TiltProcessor();

    bool initialize();
    void shutdown();

    // Tilt processing
    void processTilt(const StylusTilt& tilt);
    float getTiltX() const { return tiltX_; }
    float getTiltY() const { return tiltY_; }
    float getTiltAngle() const { return tiltAngle_; }
    TiltDirection getDirection() const { return direction_; }

    // Settings
    void setSensitivity(float sensitivity);
    float getSensitivity() const { return sensitivity_; }
    void setOffset(float offsetX, float offsetY);
    std::pair<float, float> getOffset() const { return {offsetX_, offsetY_}; }

    // Advanced processing
    void applySmoothing(float factor);
    void enableTiltPrediction(bool enable);
    void setPredictionStrength(float strength);
    void calibrateTilt();

private:
    void normalizeTilt();
    void calculateTiltDirection();
    void updateTiltHistory();
    void predictTilt();
};

// ========== PALM REJECTION MANAGER ==========
class PalmRejectionManager {
private:
    StylusManager* manager_;

    // Palm rejection state
    std::atomic<bool> enabled_;
    std::atomic<bool> palmDetected_;
    float sensitivity_;
    std::chrono::steady_clock::time_point lastPalmTime_;

    // Palm detection data
    std::vector<StylusPosition> palmPoints_;
    std::vector<StylusPosition> stylusPoints_;
    std::mutex palmMutex_;

    // Rejection settings
    float palmSizeThreshold_;
    float stylusPalmDistance_;
    float rejectionTimeout_;

public:
    PalmRejectionManager(StylusManager* manager);
    ~PalmRejectionManager();

    bool initialize();
    void shutdown();

    // Palm rejection control
    void enable(bool enable);
    bool isEnabled() const { return enabled_; }
    bool isPalmDetected() const { return palmDetected_; }

    // Settings
    void setSensitivity(float sensitivity);
    float getSensitivity() const { return sensitivity_; }
    void setPalmSizeThreshold(float threshold);
    void setStylusPalmDistance(float distance);
    void setRejectionTimeout(float timeout);

    // Palm detection
    void processTouchInput(const StylusEvent& event);
    void detectPalm();
    void rejectPalmInput();
    void clearPalmData();

private:
    bool isPalmTouch(const StylusEvent& event);
    float calculatePalmSize(const std::vector<StylusPosition>& points);
    float calculateDistanceToStylus(const StylusPosition& palmPoint);
    void updatePalmHistory(const StylusEvent& event);
    void validatePalmDetection();
};

// ========== STYLUS CALIBRATION MANAGER ==========
class StylusCalibrationManager {
private:
    StylusManager* manager_;

    // Calibration state
    std::atomic<bool> calibrating_;
    StylusCalibration calibration_;
    std::vector<StylusPressure> pressureSamples_;
    std::vector<StylusTilt> tiltSamples_;
    std::vector<StylusPosition> positionSamples_;
    std::mutex calibrationMutex_;

    // Calibration settings
    int requiredSamples_;
    float calibrationTimeout_;
    bool autoCalibration_;

public:
    StylusCalibrationManager(StylusManager* manager);
    ~StylusCalibrationManager();

    bool initialize();
    void shutdown();

    // Calibration control
    void startCalibration();
    void stopCalibration();
    bool isCalibrating() const { return calibrating_; }
    float getCalibrationProgress() const;

    // Calibration data
    void addPressureSample(const StylusPressure& pressure);
    void addTiltSample(const StylusTilt& tilt);
    void addPositionSample(const StylusPosition& position);
    StylusCalibration getCalibration() const { return calibration_; }

    // Settings
    void setRequiredSamples(int samples);
    void setCalibrationTimeout(float timeout);
    void setAutoCalibration(bool enable);
    void resetCalibration();

private:
    void processCalibrationData();
    void calculatePressureCalibration();
    void calculateTiltCalibration();
    void calculatePositionCalibration();
    void validateCalibration();
    void applyCalibration();
    void saveCalibrationToStorage();
    void loadCalibrationFromStorage();
};

// ========== GESTURE RECOGNIZER ==========
class GestureRecognizer {
private:
    StylusManager* manager_;

    // Gesture state
    std::atomic<bool> enabled_;
    std::unordered_map<StylusGesture, GestureConfig> gestureConfigs_;
    std::vector<StylusPosition> currentGesture_;
    StylusGesture currentGestureType_;
    std::chrono::steady_clock::time_point gestureStartTime_;
    std::mutex gestureMutex_;

    // Custom gestures
    std::unordered_map<std::string, std::vector<StylusPosition>> customGestures_;

public:
    GestureRecognizer(StylusManager* manager);
    ~GestureRecognizer();

    bool initialize();
    void shutdown();

    // Gesture recognition
    void enable(bool enable);
    bool isEnabled() const { return enabled_; }
    void processGesture(const StylusEvent& event);
    StylusGesture recognizeGesture(const std::vector<StylusPosition>& points);
    float getGestureConfidence(StylusGesture gesture) const;

    // Gesture configuration
    void setGestureConfig(StylusGesture gesture, const GestureConfig& config);
    GestureConfig getGestureConfig(StylusGesture gesture) const;
    void enableGesture(StylusGesture gesture, bool enable);
    bool isGestureEnabled(StylusGesture gesture) const;

    // Custom gestures
    void defineCustomGesture(const std::string& name, const std::vector<StylusPosition>& points);
    void removeCustomGesture(const std::string& name);
    bool recognizeCustomGesture(const std::string& name, const std::vector<StylusPosition>& points);
    std::vector<std::string> getCustomGestures() const;

private:
    void startGesture(const StylusEvent& event);
    void updateGesture(const StylusEvent& event);
    void endGesture(const StylusEvent& event);
    void validateGesture();
    bool matchGesture(StylusGesture gesture, const std::vector<StylusPosition>& points);
    float calculateGestureSimilarity(const std::vector<StylusPosition>& points1,
                                   const std::vector<StylusPosition>& points2);
    void normalizeGesture(std::vector<StylusPosition>& points);
    void resampleGesture(std::vector<StylusPosition>& points, int targetPoints);
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // Stylus detection callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onStylusDetected(
        JNIEnv* env, jobject thiz, jstring stylusType, jboolean hasPressure, jboolean hasTilt);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onStylusRemoved(
        JNIEnv* env, jobject thiz);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onStylusCapabilitiesChanged(
        JNIEnv* env, jobject thiz, jstring capabilitiesJson);

    // Stylus input callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onStylusDown(
        JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat pressure, jfloat tiltX, jfloat tiltY);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onStylusMove(
        JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat pressure, jfloat tiltX, jfloat tiltY);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onStylusUp(
        JNIEnv* env, jobject thiz, jfloat x, jfloat y);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onPressureChanged(
        JNIEnv* env, jobject thiz, jfloat pressure);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onTiltChanged(
        JNIEnv* env, jobject thiz, jfloat tiltX, jfloat tiltY);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onButtonPressed(
        JNIEnv* env, jobject thiz, jint button, jfloat pressure);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onButtonReleased(
        JNIEnv* env, jobject thiz, jint button);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onEraserDown(
        JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat pressure);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onEraserMove(
        JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat pressure);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onEraserUp(
        JNIEnv* env, jobject thiz, jfloat x, jfloat y);

    // Palm rejection callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onPalmDetected(
        JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat size);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onPalmReleased(
        JNIEnv* env, jobject thiz);

    // Gesture callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onGestureRecognized(
        JNIEnv* env, jobject thiz, jstring gestureName, jfloat confidence);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onGestureStarted(
        JNIEnv* env, jobject thiz, jstring gestureName);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onGestureEnded(
        JNIEnv* env, jobject thiz, jstring gestureName);

    // Calibration callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onCalibrationStarted(
        JNIEnv* env, jobject thiz);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onCalibrationProgress(
        JNIEnv* env, jobject thiz, jfloat progress);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onCalibrationCompleted(
        JNIEnv* env, jobject thiz, jboolean success);

    // Haptic feedback callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_StylusManager_onHapticFeedbackCompleted(
        JNIEnv* env, jobject thiz);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_STYLUS_MANAGER_H
