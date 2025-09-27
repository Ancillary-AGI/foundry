#ifndef FOUNDRYENGINE_GESTURE_MANAGER_H
#define FOUNDRYENGINE_GESTURE_MANAGER_H

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
class GestureManager;
class TouchProcessor;
class PinchProcessor;
class PanProcessor;
class SwipeProcessor;
class RotateProcessor;
class TapProcessor;
class LongPressProcessor;
class GestureCalibrationManager;

// Touch point data
struct TouchPoint {
    int id;                 // Unique touch ID
    float x;                // X coordinate
    float y;                // Y coordinate
    float pressure;         // Touch pressure (0.0 - 1.0)
    float size;             // Touch size/major axis
    std::chrono::steady_clock::time_point timestamp;
    bool isValid;           // Whether this touch point is valid
    float velocityX;        // X velocity
    float velocityY;        // Y velocity
    float accelerationX;    // X acceleration
    float accelerationY;    // Y acceleration
};

// Gesture types
enum class GestureType {
    TAP,                    // Single tap
    DOUBLE_TAP,             // Double tap
    TRIPLE_TAP,             // Triple tap
    LONG_PRESS,             // Long press
    PAN,                    // Pan/drag gesture
    PINCH,                  // Pinch gesture (zoom)
    SPREAD,                 // Spread gesture (zoom out)
    ROTATE,                 // Rotation gesture
    SWIPE_LEFT,             // Swipe left
    SWIPE_RIGHT,            // Swipe right
    SWIPE_UP,               // Swipe up
    SWIPE_DOWN,             // Swipe down
    TWO_FINGER_TAP,         // Two finger tap
    TWO_FINGER_PAN,         // Two finger pan
    TWO_FINGER_PINCH,       // Two finger pinch
    TWO_FINGER_ROTATE,      // Two finger rotate
    THREE_FINGER_SWIPE,     // Three finger swipe
    FOUR_FINGER_SWIPE,      // Four finger swipe
    FIVE_FINGER_PINCH,      // Five finger pinch
    EDGE_SWIPE_LEFT,        // Edge swipe left
    EDGE_SWIPE_RIGHT,       // Edge swipe right
    CUSTOM                  // Custom gesture
};

// Gesture state
enum class GestureState {
    POSSIBLE,               // Gesture is possible
    BEGAN,                  // Gesture has begun
    CHANGED,                // Gesture has changed
    ENDED,                  // Gesture has ended
    CANCELLED,              // Gesture was cancelled
    FAILED                  // Gesture failed
};

// Gesture direction
enum class GestureDirection {
    LEFT,                   // Left direction
    RIGHT,                  // Right direction
    UP,                     // Up direction
    DOWN,                   // Down direction
    CLOCKWISE,              // Clockwise rotation
    COUNTER_CLOCKWISE,      // Counter-clockwise rotation
    INWARD,                 // Inward (pinch)
    OUTWARD                 // Outward (spread)
};

// Gesture configuration
struct GestureConfig {
    GestureType type;       // Gesture type
    float minDuration;      // Minimum duration (ms)
    float maxDuration;      // Maximum duration (ms)
    float minDistance;      // Minimum movement distance
    float maxDistance;      // Maximum movement distance
    float tolerance;        // Recognition tolerance
    int requiredTouches;    // Required number of touches
    bool requirePressure;   // Whether pressure is required
    float minPressure;      // Minimum pressure threshold
    bool enabled;           // Whether gesture is enabled
    float priority;         // Gesture priority (higher = more important)
    std::string name;       // Gesture name
};

// Tap gesture data
struct TapGesture {
    int tapCount;           // Number of taps (1, 2, or 3)
    TouchPoint location;    // Tap location
    float duration;         // Tap duration
    int fingerCount;        // Number of fingers
    bool isValid;           // Whether tap is valid
};

// Pan gesture data
struct PanGesture {
    TouchPoint startLocation; // Starting location
    TouchPoint currentLocation; // Current location
    TouchPoint velocity;    // Pan velocity
    float distance;         // Total distance moved
    float translationX;     // X translation
    float translationY;     // Y translation
    int fingerCount;        // Number of fingers
    bool isValid;           // Whether pan is valid
};

// Pinch gesture data
struct PinchGesture {
    TouchPoint centerPoint; // Center point between touches
    float scale;            // Scale factor (1.0 = no change)
    float velocity;         // Pinch velocity
    float initialDistance;  // Initial distance between touches
    float currentDistance;  // Current distance between touches
    int fingerCount;        // Number of fingers
    bool isValid;           // Whether pinch is valid
};

// Rotate gesture data
struct RotateGesture {
    TouchPoint centerPoint; // Center point of rotation
    float rotation;         // Rotation angle in degrees
    float velocity;         // Rotation velocity
    float initialAngle;     // Initial angle
    float currentAngle;     // Current angle
    int fingerCount;        // Number of fingers
    bool isValid;           // Whether rotation is valid
};

// Swipe gesture data
struct SwipeGesture {
    GestureDirection direction; // Swipe direction
    float velocity;         // Swipe velocity
    float distance;         // Swipe distance
    TouchPoint startLocation; // Starting location
    TouchPoint endLocation;   // Ending location
    int fingerCount;        // Number of fingers
    bool isValid;           // Whether swipe is valid
};

// Long press gesture data
struct LongPressGesture {
    TouchPoint location;    // Press location
    float duration;         // Press duration
    int fingerCount;        // Number of fingers
    bool isValid;           // Whether long press is valid
};

// Generic gesture data
struct GestureData {
    GestureType type;       // Gesture type
    GestureState state;     // Current state
    float confidence;       // Recognition confidence (0.0 - 1.0)
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    std::vector<TouchPoint> touchPoints; // Associated touch points

    // Union of specific gesture data
    union {
        TapGesture tap;
        PanGesture pan;
        PinchGesture pinch;
        RotateGesture rotate;
        SwipeGesture swipe;
        LongPressGesture longPress;
    } gestureData;

    std::unordered_map<std::string, float> customData; // Custom gesture data
};

// Gesture settings
struct GestureSettings {
    bool enabled;           // Whether gesture recognition is enabled
    bool multiTouchEnabled; // Enable multi-touch gestures
    bool edgeGesturesEnabled; // Enable edge gestures
    bool pressureGesturesEnabled; // Enable pressure-based gestures
    float tapThreshold;     // Tap distance threshold
    float swipeThreshold;   // Swipe velocity threshold
    float pinchThreshold;   // Pinch distance threshold
    float rotateThreshold;  // Rotation angle threshold
    float longPressThreshold; // Long press time threshold
    bool hapticFeedbackEnabled; // Enable haptic feedback
    bool visualFeedbackEnabled; // Enable visual feedback
    int maxActiveGestures;  // Maximum concurrent gestures
    float gestureTimeout;   // Gesture timeout in seconds
    bool enablePrediction;  // Enable gesture prediction
    bool enableSmoothing;   // Enable gesture smoothing
    float smoothingFactor;  // Gesture smoothing factor
};

// Touch device capabilities
struct TouchCapabilities {
    int maxTouchPoints;     // Maximum supported touch points
    bool hasPressure;       // Supports pressure detection
    bool hasSize;           // Supports touch size detection
    bool hasOrientation;    // Supports touch orientation
    float pressureResolution; // Pressure resolution
    float sizeResolution;   // Size resolution
    bool supportsMultiTouch; // Supports multi-touch
    bool supportsPalmRejection; // Supports palm rejection
    std::vector<GestureType> supportedGestures; // Supported gesture types
};

// Gesture recognition result
struct GestureResult {
    GestureType type;       // Recognized gesture type
    float confidence;       // Recognition confidence
    GestureData data;       // Gesture data
    bool isValid;           // Whether recognition was successful
    std::string errorMessage; // Error message if recognition failed
};

// Callback types
using GestureRecognizedCallback = std::function<void(const GestureResult&)>;
using GestureStateChangedCallback = std::function<void(GestureType, GestureState)>;
using TouchEventCallback = std::function<void(const std::vector<TouchPoint>&)>;
using GestureErrorCallback = std::function<void(const std::string&)>;

// ========== GESTURE MANAGER ==========
class GestureManager : public System {
private:
    static GestureManager* instance_;

    TouchProcessor* touchProcessor_;
    PinchProcessor* pinchProcessor_;
    PanProcessor* panProcessor_;
    SwipeProcessor* swipeProcessor_;
    RotateProcessor* rotateProcessor_;
    TapProcessor* tapProcessor_;
    LongPressProcessor* longPressProcessor_;
    GestureCalibrationManager* calibrationManager_;

    // JNI environment
    JNIEnv* env_;
    jobject context_;

    // Gesture state
    std::atomic<bool> initialized_;
    std::atomic<bool> gestureRecognitionActive_;
    TouchCapabilities capabilities_;
    GestureSettings settings_;

    // Current state
    std::vector<TouchPoint> activeTouches_;
    std::unordered_map<GestureType, GestureData> activeGestures_;
    std::vector<GestureResult> gestureHistory_;
    std::mutex gestureMutex_;

    // Event system
    std::unordered_map<std::string, GestureRecognizedCallback> recognizedCallbacks_;
    std::unordered_map<std::string, GestureStateChangedCallback> stateChangedCallbacks_;
    std::unordered_map<std::string, TouchEventCallback> touchEventCallbacks_;
    std::unordered_map<std::string, GestureErrorCallback> errorCallbacks_;

    // Touch event queue
    std::queue<std::vector<TouchPoint>> touchEventQueue_;
    std::mutex touchEventMutex_;
    std::condition_variable touchEventCondition_;

    // Service management
    std::atomic<bool> serviceRunning_;
    std::thread touchThread_;
    std::thread gestureThread_;

    // Settings
    bool adaptiveThresholds_;
    float sensitivityScale_;

public:
    GestureManager();
    ~GestureManager();

    static GestureManager* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // JNI setup
    void setJNIEnvironment(JNIEnv* env, jobject context);

    // Touch capabilities
    TouchCapabilities getTouchCapabilities() const { return capabilities_; }
    bool isMultiTouchSupported() const { return capabilities_.supportsMultiTouch; }
    int getMaxTouchPoints() const { return capabilities_.maxTouchPoints; }
    bool hasPressureSupport() const { return capabilities_.hasPressure; }

    // Gesture settings
    void setSettings(const GestureSettings& settings);
    GestureSettings getSettings() const { return settings_; }
    void setTapThreshold(float threshold);
    void setSwipeThreshold(float threshold);
    void setPinchThreshold(float threshold);
    void setRotateThreshold(float threshold);
    void setLongPressThreshold(float threshold);
    void enableHapticFeedback(bool enable);
    void enableVisualFeedback(bool enable);

    // Gesture control
    void enableGesture(GestureType gesture, bool enable);
    void enableAllGestures(bool enable);
    void disableAllGestures();
    bool isGestureEnabled(GestureType gesture) const;
    void setGesturePriority(GestureType gesture, float priority);
    float getGesturePriority(GestureType gesture) const;

    // Gesture configuration
    void setGestureConfig(GestureType gesture, const GestureConfig& config);
    GestureConfig getGestureConfig(GestureType gesture) const;
    void configureTapGesture(int tapCount, float maxDuration, float maxDistance);
    void configureSwipeGesture(float minVelocity, float maxDuration);
    void configurePinchGesture(float minDistance, float minScale);
    void configureRotateGesture(float minAngle, float minDuration);

    // Touch processing
    void processTouchEvent(const std::vector<TouchPoint>& touches);
    void processTouchDown(int touchId, float x, float y, float pressure = 1.0f, float size = 1.0f);
    void processTouchMove(int touchId, float x, float y, float pressure = 1.0f, float size = 1.0f);
    void processTouchUp(int touchId, float x, float y);
    std::vector<TouchPoint> getActiveTouches() const { return activeTouches_; }
    int getActiveTouchCount() const { return static_cast<int>(activeTouches_.size()); }

    // Gesture recognition
    GestureResult recognizeGesture(const std::vector<TouchPoint>& touches);
    std::vector<GestureResult> recognizeAllGestures(const std::vector<TouchPoint>& touches);
    bool isGesturePossible(GestureType gesture, const std::vector<TouchPoint>& touches) const;
    float getGestureConfidence(GestureType gesture) const;

    // Advanced features
    void enableAdaptiveThresholds(bool enable);
    void setSensitivityScale(float scale);
    void enableGesturePrediction(bool enable);
    void enableGestureSmoothing(bool enable);
    void setMaxActiveGestures(int maxGestures);

    // Custom gestures
    void defineCustomGesture(const std::string& name, const std::vector<std::vector<TouchPoint>>& patterns);
    void removeCustomGesture(const std::string& name);
    std::vector<std::string> getCustomGestures() const;
    GestureResult recognizeCustomGesture(const std::string& name, const std::vector<TouchPoint>& touches);

    // Gesture history
    std::vector<GestureResult> getGestureHistory() const { return gestureHistory_; }
    void clearGestureHistory();
    int getGestureCount(GestureType type) const;
    std::vector<GestureType> getRecentGestures(int count) const;

    // Real-time gesture data
    GestureData getCurrentGestureData(GestureType type) const;
    std::vector<GestureData> getAllActiveGestures() const;
    bool isGestureActive(GestureType type) const;

    // Callback management
    void registerGestureRecognizedCallback(const std::string& id, GestureRecognizedCallback callback);
    void unregisterGestureRecognizedCallback(const std::string& id);
    void registerGestureStateChangedCallback(const std::string& id, GestureStateChangedCallback callback);
    void unregisterGestureStateChangedCallback(const std::string& id);
    void registerTouchEventCallback(const std::string& id, TouchEventCallback callback);
    void unregisterTouchEventCallback(const std::string& id);
    void registerGestureErrorCallback(const std::string& id, GestureErrorCallback callback);
    void unregisterGestureErrorCallback(const std::string& id);

    // Utility functions
    bool isGestureRecognitionActive() const { return gestureRecognitionActive_; }
    std::string getGestureStatus() const;
    void resetGestureState();
    void testGestureRecognition();

    // Advanced touch processing
    void enablePalmRejection(bool enable);
    bool isPalmRejectionEnabled() const;
    void setTouchTimeout(float timeout);
    float getTouchTimeout() const;

    // Performance optimization
    void setMaxProcessingTime(float maxTime);
    void enableParallelProcessing(bool enable);
    void setThreadCount(int threads);

private:
    void initializeDefaults();
    void detectTouchCapabilities();
    void startServiceThreads();
    void stopServiceThreads();
    void touchThreadLoop();
    void gestureThreadLoop();

    // JNI helper methods
    void detectTouchCapabilitiesJNI();
    void processTouchInputJNI(const std::vector<TouchPoint>& touches);
    void triggerHapticFeedbackJNI(GestureType gesture);

    // Touch processing
    void onTouchEvent(const std::vector<TouchPoint>& touches);
    void onTouchBegan(const TouchPoint& touch);
    void onTouchMoved(const TouchPoint& touch);
    void onTouchEnded(const TouchPoint& touch);
    void onTouchCancelled(const TouchPoint& touch);

    // Gesture processing
    void onGestureRecognized(const GestureResult& result);
    void onGestureStateChanged(GestureType type, GestureState state);
    void onGestureError(const std::string& error);

    // Gesture recognition algorithms
    GestureResult recognizeTapGesture(const std::vector<TouchPoint>& touches);
    GestureResult recognizePanGesture(const std::vector<TouchPoint>& touches);
    GestureResult recognizePinchGesture(const std::vector<TouchPoint>& touches);
    GestureResult recognizeRotateGesture(const std::vector<TouchPoint>& touches);
    GestureResult recognizeSwipeGesture(const std::vector<TouchPoint>& touches);
    GestureResult recognizeLongPressGesture(const std::vector<TouchPoint>& touches);

    // Utility functions
    bool isValidTouchPoint(const TouchPoint& touch) const;
    float calculateDistance(const TouchPoint& p1, const TouchPoint& p2) const;
    float calculateAngle(const TouchPoint& p1, const TouchPoint& p2, const TouchPoint& center) const;
    TouchPoint calculateCentroid(const std::vector<TouchPoint>& touches) const;
    float calculateAveragePressure(const std::vector<TouchPoint>& touches) const;
    void updateTouchVelocities();
    void applyTouchSmoothing();
    void validateGestures();
    void updateGestureHistory(const GestureResult& result);
    void cleanupInactiveGestures();
    void applyAdaptiveThresholds();
};

// ========== TOUCH PROCESSOR ==========
class TouchProcessor {
private:
    GestureManager* manager_;

    // Touch state
    std::vector<TouchPoint> activeTouches_;
    std::unordered_map<int, TouchPoint> touchHistory_;
    std::mutex touchMutex_;

    // Processing settings
    bool smoothingEnabled_;
    float smoothingFactor_;
    bool predictionEnabled_;
    int predictionSamples_;
    float touchTimeout_;

public:
    TouchProcessor(GestureManager* manager);
    ~TouchProcessor();

    bool initialize();
    void shutdown();

    // Touch processing
    void processTouches(const std::vector<TouchPoint>& touches);
    void addTouch(const TouchPoint& touch);
    void updateTouch(const TouchPoint& touch);
    void removeTouch(int touchId);
    std::vector<TouchPoint> getActiveTouches() const { return activeTouches_; }

    // Settings
    void enableSmoothing(bool enable);
    void setSmoothingFactor(float factor);
    void enablePrediction(bool enable);
    void setPredictionSamples(int samples);
    void setTouchTimeout(float timeout);

    // Advanced processing
    void applySmoothing();
    void predictTouchPositions();
    void validateTouchData();
    void cleanupOldTouches();

private:
    void updateTouchVelocities();
    void calculateAcceleration();
    bool isTouchValid(const TouchPoint& touch) const;
    void mergeTouchHistory();
};

// ========== PINCH PROCESSOR ==========
class PinchProcessor {
private:
    GestureManager* manager_;

    // Pinch state
    std::atomic<bool> pinchActive_;
    PinchGesture currentPinch_;
    std::vector<TouchPoint> pinchTouches_;
    std::mutex pinchMutex_;

    // Pinch settings
    float minDistanceThreshold_;
    float maxDistanceThreshold_;
    float scaleThreshold_;
    bool requireTwoTouches_;

public:
    PinchProcessor(GestureManager* manager);
    ~PinchProcessor();

    bool initialize();
    void shutdown();

    // Pinch processing
    void processPinch(const std::vector<TouchPoint>& touches);
    bool isPinchActive() const { return pinchActive_; }
    PinchGesture getCurrentPinch() const { return currentPinch_; }

    // Settings
    void setDistanceThreshold(float minDistance, float maxDistance);
    void setScaleThreshold(float threshold);
    void setRequireTwoTouches(bool require);

    // Advanced processing
    void calculatePinchCenter();
    void calculatePinchScale();
    void calculatePinchVelocity();
    void validatePinchGesture();

private:
    float calculateDistanceBetweenTouches(const TouchPoint& t1, const TouchPoint& t2) const;
    TouchPoint calculateMidpoint(const TouchPoint& t1, const TouchPoint& t2) const;
    bool isValidPinch(const std::vector<TouchPoint>& touches) const;
};

// ========== PAN PROCESSOR ==========
class PanProcessor {
private:
    GestureManager* manager_;

    // Pan state
    std::atomic<bool> panActive_;
    PanGesture currentPan_;
    std::vector<TouchPoint> panTouches_;
    std::mutex panMutex_;

    // Pan settings
    float minDistanceThreshold_;
    float maxVelocityThreshold_;
    bool requireSingleTouch_;

public:
    PanProcessor(GestureManager* manager);
    ~PanProcessor();

    bool initialize();
    void shutdown();

    // Pan processing
    void processPan(const std::vector<TouchPoint>& touches);
    bool isPanActive() const { return panActive_; }
    PanGesture getCurrentPan() const { return currentPan_; }

    // Settings
    void setDistanceThreshold(float threshold);
    void setVelocityThreshold(float threshold);
    void setRequireSingleTouch(bool require);

    // Advanced processing
    void calculatePanTranslation();
    void calculatePanVelocity();
    void validatePanGesture();

private:
    bool isValidPan(const std::vector<TouchPoint>& touches) const;
    TouchPoint getPrimaryTouch(const std::vector<TouchPoint>& touches) const;
};

// ========== SWIPE PROCESSOR ==========
class SwipeProcessor {
private:
    GestureManager* manager_;

    // Swipe state
    std::atomic<bool> swipeActive_;
    SwipeGesture currentSwipe_;
    std::vector<TouchPoint> swipeTouches_;
    std::mutex swipeMutex_;

    // Swipe settings
    float minVelocityThreshold_;
    float maxDurationThreshold_;
    float minDistanceThreshold_;

public:
    SwipeProcessor(GestureManager* manager);
    ~SwipeProcessor();

    bool initialize();
    void shutdown();

    // Swipe processing
    void processSwipe(const std::vector<TouchPoint>& touches);
    bool isSwipeActive() const { return swipeActive_; }
    SwipeGesture getCurrentSwipe() const { return currentSwipe_; }

    // Settings
    void setVelocityThreshold(float threshold);
    void setDurationThreshold(float threshold);
    void setDistanceThreshold(float threshold);

    // Advanced processing
    void calculateSwipeDirection();
    void calculateSwipeVelocity();
    void validateSwipeGesture();

private:
    bool isValidSwipe(const std::vector<TouchPoint>& touches) const;
    GestureDirection determineSwipeDirection(const TouchPoint& start, const TouchPoint& end) const;
};

// ========== ROTATE PROCESSOR ==========
class RotateProcessor {
private:
    GestureManager* manager_;

    // Rotate state
    std::atomic<bool> rotateActive_;
    RotateGesture currentRotate_;
    std::vector<TouchPoint> rotateTouches_;
    std::mutex rotateMutex_;

    // Rotate settings
    float minAngleThreshold_;
    float maxDurationThreshold_;
    bool requireTwoTouches_;

public:
    RotateProcessor(GestureManager* manager);
    ~RotateProcessor();

    bool initialize();
    void shutdown();

    // Rotate processing
    void processRotate(const std::vector<TouchPoint>& touches);
    bool isRotateActive() const { return rotateActive_; }
    RotateGesture getCurrentRotate() const { return currentRotate_; }

    // Settings
    void setAngleThreshold(float threshold);
    void setDurationThreshold(float threshold);
    void setRequireTwoTouches(bool require);

    // Advanced processing
    void calculateRotationCenter();
    void calculateRotationAngle();
    void calculateRotationVelocity();
    void validateRotateGesture();

private:
    float calculateAngleBetweenTouches(const TouchPoint& t1, const TouchPoint& t2, const TouchPoint& center) const;
    bool isValidRotation(const std::vector<TouchPoint>& touches) const;
};

// ========== TAP PROCESSOR ==========
class TapProcessor {
private:
    GestureManager* manager_;

    // Tap state
    std::vector<TapGesture> tapHistory_;
    std::chrono::steady_clock::time_point lastTapTime_;
    int tapCount_;
    std::mutex tapMutex_;

    // Tap settings
    float maxTapDuration_;
    float maxTapDistance_;
    int maxTapCount_;

public:
    TapProcessor(GestureManager* manager);
    ~TapProcessor();

    bool initialize();
    void shutdown();

    // Tap processing
    void processTap(const std::vector<TouchPoint>& touches);
    std::vector<TapGesture> getTapHistory() const { return tapHistory_; }

    // Settings
    void setMaxTapDuration(float duration);
    void setMaxTapDistance(float distance);
    void setMaxTapCount(int count);

    // Advanced processing
    void detectDoubleTap();
    void detectTripleTap();
    void validateTapGesture();

private:
    bool isValidTap(const std::vector<TouchPoint>& touches) const;
    float calculateTapDuration(const TouchPoint& touch) const;
    float calculateTapDistance(const TouchPoint& start, const TouchPoint& end) const;
};

// ========== LONG PRESS PROCESSOR ==========
class LongPressProcessor {
private:
    GestureManager* manager_;

    // Long press state
    std::atomic<bool> longPressActive_;
    LongPressGesture currentLongPress_;
    std::vector<TouchPoint> longPressTouches_;
    std::chrono::steady_clock::time_point pressStartTime_;
    std::mutex longPressMutex_;

    // Long press settings
    float minPressDuration_;
    float maxMovementThreshold_;
    bool requireSingleTouch_;

public:
    LongPressProcessor(GestureManager* manager);
    ~LongPressProcessor();

    bool initialize();
    void shutdown();

    // Long press processing
    void processLongPress(const std::vector<TouchPoint>& touches);
    bool isLongPressActive() const { return longPressActive_; }
    LongPressGesture getCurrentLongPress() const { return currentLongPress_; }

    // Settings
    void setMinPressDuration(float duration);
    void setMaxMovementThreshold(float threshold);
    void setRequireSingleTouch(bool require);

    // Advanced processing
    void startLongPressTimer();
    void cancelLongPressTimer();
    void validateLongPressGesture();

private:
    bool isValidLongPress(const std::vector<TouchPoint>& touches) const;
    float calculateMovement(const std::vector<TouchPoint>& touches) const;
};

// ========== GESTURE CALIBRATION MANAGER ==========
class GestureCalibrationManager {
private:
    GestureManager* manager_;

    // Calibration state
    std::atomic<bool> calibrating_;
    std::unordered_map<GestureType, std::vector<GestureData>> calibrationData_;
    std::mutex calibrationMutex_;

    // Calibration settings
    int requiredSamples_;
    float calibrationTimeout_;
    bool autoCalibration_;

public:
    GestureCalibrationManager(GestureManager* manager);
    ~GestureCalibrationManager();

    bool initialize();
    void shutdown();

    // Calibration control
    void startCalibration();
    void stopCalibration();
    bool isCalibrating() const { return calibrating_; }
    float getCalibrationProgress() const;

    // Calibration data
    void addCalibrationSample(GestureType type, const GestureData& data);
    void clearCalibrationData();
    std::unordered_map<GestureType, std::vector<GestureData>> getCalibrationData() const;

    // Settings
    void setRequiredSamples(int samples);
    void setCalibrationTimeout(float timeout);
    void setAutoCalibration(bool enable);

private:
    void processCalibrationData();
    void calculateOptimalThresholds();
    void validateCalibration();
    void applyCalibration();
    void saveCalibrationToStorage();
    void loadCalibrationFromStorage();
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // Touch event callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onTouchDown(
        JNIEnv* env, jobject thiz, jint touchId, jfloat x, jfloat y, jfloat pressure, jfloat size);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onTouchMove(
        JNIEnv* env, jobject thiz, jint touchId, jfloat x, jfloat y, jfloat pressure, jfloat size);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onTouchUp(
        JNIEnv* env, jobject thiz, jint touchId, jfloat x, jfloat y);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onTouchCancelled(
        JNIEnv* env, jobject thiz, jint touchId);

    // Gesture recognition callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onGestureRecognized(
        JNIEnv* env, jobject thiz, jstring gestureType, jfloat confidence, jstring dataJson);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onGestureBegan(
        JNIEnv* env, jobject thiz, jstring gestureType);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onGestureChanged(
        JNIEnv* env, jobject thiz, jstring gestureType, jstring dataJson);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onGestureEnded(
        JNIEnv* env, jobject thiz, jstring gestureType);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onGestureCancelled(
        JNIEnv* env, jobject thiz, jstring gestureType);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onGestureFailed(
        JNIEnv* env, jobject thiz, jstring gestureType, jstring error);

    // Touch capabilities callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onTouchCapabilitiesDetected(
        JNIEnv* env, jobject thiz, jstring capabilitiesJson);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onMultiTouchEnabled(
        JNIEnv* env, jobject thiz, jboolean enabled);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onPalmRejectionEnabled(
        JNIEnv* env, jobject thiz, jboolean enabled);

    // Haptic feedback callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_GestureManager_onHapticFeedbackCompleted(
        JNIEnv* env, jobject thiz, jstring gestureType);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_GESTURE_MANAGER_H
