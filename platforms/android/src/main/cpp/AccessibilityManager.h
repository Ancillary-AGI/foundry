#ifndef FOUNDRYENGINE_ACCESSIBILITY_MANAGER_H
#define FOUNDRYENGINE_ACCESSIBILITY_MANAGER_H

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

namespace FoundryEngine {

// Forward declarations
class AccessibilityManager;
class ScreenReader;
class VoiceControl;
class MotorAccessibility;
class HighContrastManager;
class TextToSpeechManager;
class SpeechToTextManager;

// Accessibility event types
enum class AccessibilityEventType {
    VIEW_FOCUSED,
    VIEW_CLICKED,
    VIEW_LONG_CLICKED,
    VIEW_SELECTED,
    VIEW_TEXT_CHANGED,
    VIEW_SCROLL,
    WINDOW_STATE_CHANGED,
    NOTIFICATION_STATE_CHANGED,
    ANNOUNCEMENT,
    GESTURE_DETECTION
};

// Accessibility gesture types
enum class AccessibilityGesture {
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN,
    DOUBLE_TAP,
    TRIPLE_TAP,
    PINCH_IN,
    PINCH_OUT,
    SCROLL_UP,
    SCROLL_DOWN,
    CUSTOM_GESTURE
};

// Accessibility service types
enum class AccessibilityServiceType {
    SCREEN_READER,
    VOICE_CONTROL,
    SWITCH_CONTROL,
    VOICE_ACCESS,
    TALKBACK,
    BRAILLE_DISPLAY,
    MAGNIFICATION
};

// Accessibility feedback types
enum class AccessibilityFeedbackType {
    AUDIBLE,
    VISUAL,
    HAPTIC,
    VERBAL
};

// Accessibility node types
enum class AccessibilityNodeType {
    BUTTON,
    TEXT_VIEW,
    IMAGE_VIEW,
    EDIT_TEXT,
    CHECKBOX,
    RADIO_BUTTON,
    TOGGLE_BUTTON,
    SEEK_BAR,
    SWITCH,
    SPINNER,
    WEB_VIEW,
    VIEW_GROUP,
    CUSTOM_VIEW
};

// Accessibility importance levels
enum class AccessibilityImportance {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

// Accessibility event
struct AccessibilityEvent {
    AccessibilityEventType type;
    std::string sourceId;
    std::string text;
    std::string description;
    AccessibilityNodeType nodeType;
    int x, y; // Position on screen
    int width, height; // Size
    bool isEnabled;
    bool isVisible;
    bool isFocused;
    bool isSelected;
    AccessibilityImportance importance;
    double timestamp;
};

// Accessibility node information
struct AccessibilityNodeInfo {
    std::string id;
    std::string text;
    std::string contentDescription;
    std::string hint;
    std::string error;
    AccessibilityNodeType type;
    int x, y;
    int width, height;
    bool isEnabled;
    bool isVisible;
    bool isFocusable;
    bool isFocused;
    bool isSelected;
    bool isChecked;
    bool isCheckable;
    bool isClickable;
    bool isLongClickable;
    bool isScrollable;
    bool isPassword;
    bool isEditable;
    int childCount;
    std::vector<std::string> children;
    std::unordered_map<std::string, std::string> properties;
};

// Voice command
struct VoiceCommand {
    std::string command;
    std::string description;
    std::vector<std::string> aliases;
    float confidence;
    bool enabled;
    std::function<void(const std::vector<std::string>&)> handler;
};

// Motor accessibility profile
struct MotorAccessibilityProfile {
    bool useSwitchControl;
    bool useVoiceControl;
    int switchCount;
    float gestureSensitivity;
    float touchDelay;
    bool stickyKeys;
    bool slowKeys;
    bool mouseKeys;
    bool repeatKeys;
    int repeatDelay;
    int repeatRate;
};

// High contrast settings
struct HighContrastSettings {
    bool enabled;
    float contrastRatio;
    bool invertColors;
    bool grayscale;
    bool highSaturation;
    std::string colorTheme;
};

// Text scaling settings
struct TextScalingSettings {
    float scaleFactor;
    bool boldText;
    bool largerText;
    bool highContrastText;
    std::string fontFamily;
};

// Callback types
using AccessibilityEventCallback = std::function<void(const AccessibilityEvent&)>;
using VoiceCommandCallback = std::function<void(const VoiceCommand&, float)>;
using GestureCallback = std::function<void(AccessibilityGesture, int, int)>;
using AccessibilityStateCallback = std::function<void(bool)>;

// ========== ACCESSIBILITY MANAGER ==========
class AccessibilityManager : public System {
private:
    static AccessibilityManager* instance_;

    ScreenReader* screenReader_;
    VoiceControl* voiceControl_;
    MotorAccessibility* motorAccessibility_;
    HighContrastManager* highContrastManager_;
    TextToSpeechManager* textToSpeechManager_;
    SpeechToTextManager* speechToTextManager_;

    // JNI environment
    JNIEnv* env_;
    jobject context_;

    // Accessibility state
    std::atomic<bool> accessibilityEnabled_;
    std::atomic<bool> screenReaderEnabled_;
    std::atomic<bool> voiceControlEnabled_;
    std::atomic<bool> highContrastEnabled_;
    std::atomic<bool> motorAccessibilityEnabled_;

    // Event handling
    std::unordered_map<std::string, AccessibilityEventCallback> eventCallbacks_;
    std::unordered_map<std::string, VoiceCommandCallback> voiceCallbacks_;
    std::unordered_map<std::string, GestureCallback> gestureCallbacks_;
    std::unordered_map<std::string, AccessibilityStateCallback> stateCallbacks_;

    // Accessibility nodes
    std::unordered_map<std::string, AccessibilityNodeInfo> accessibilityNodes_;
    std::mutex nodesMutex_;

    // Voice commands
    std::vector<VoiceCommand> voiceCommands_;
    std::mutex commandsMutex_;

    // Settings
    MotorAccessibilityProfile motorProfile_;
    HighContrastSettings contrastSettings_;
    TextScalingSettings textSettings_;

    // Service management
    std::atomic<bool> initialized_;
    std::thread serviceThread_;
    std::atomic<bool> serviceThreadRunning_;

public:
    AccessibilityManager();
    ~AccessibilityManager();

    static AccessibilityManager* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // JNI setup
    void setJNIEnvironment(JNIEnv* env, jobject context);

    // Accessibility state
    bool isAccessibilityEnabled() const { return accessibilityEnabled_; }
    bool isScreenReaderEnabled() const { return screenReaderEnabled_; }
    bool isVoiceControlEnabled() const { return voiceControlEnabled_; }
    bool isHighContrastEnabled() const { return highContrastEnabled_; }
    bool isMotorAccessibilityEnabled() const { return motorAccessibilityEnabled_; }

    // Event handling
    void registerEventCallback(const std::string& id, AccessibilityEventCallback callback);
    void unregisterEventCallback(const std::string& id);
    void registerVoiceCallback(const std::string& id, VoiceCommandCallback callback);
    void unregisterVoiceCallback(const std::string& id);
    void registerGestureCallback(const std::string& id, GestureCallback callback);
    void unregisterGestureCallback(const std::string& id);
    void registerStateCallback(const std::string& id, AccessibilityStateCallback callback);
    void unregisterStateCallback(const std::string& id);

    // Accessibility node management
    void addAccessibilityNode(const AccessibilityNodeInfo& node);
    void removeAccessibilityNode(const std::string& id);
    void updateAccessibilityNode(const std::string& id, const AccessibilityNodeInfo& node);
    AccessibilityNodeInfo getAccessibilityNode(const std::string& id) const;
    std::vector<AccessibilityNodeInfo> getAllAccessibilityNodes() const;
    std::vector<AccessibilityNodeInfo> getFocusableNodes() const;

    // Voice commands
    void addVoiceCommand(const VoiceCommand& command);
    void removeVoiceCommand(const std::string& command);
    void enableVoiceCommand(const std::string& command, bool enable);
    std::vector<VoiceCommand> getVoiceCommands() const;
    bool processVoiceCommand(const std::string& text, float confidence);

    // Screen reader operations
    void announceText(const std::string& text, AccessibilityImportance importance = AccessibilityImportance::MEDIUM);
    void announceEvent(const AccessibilityEvent& event);
    void setScreenReaderEnabled(bool enabled);
    void setScreenReaderSpeed(float speed);
    void setScreenReaderPitch(float pitch);
    void setScreenReaderVolume(float volume);

    // Voice control operations
    void setVoiceControlEnabled(bool enabled);
    void setVoiceControlSensitivity(float sensitivity);
    void setVoiceControlTimeout(float timeout);
    void processVoiceInput(const std::string& text, float confidence);

    // Motor accessibility operations
    void setMotorAccessibilityEnabled(bool enabled);
    void setMotorAccessibilityProfile(const MotorAccessibilityProfile& profile);
    MotorAccessibilityProfile getMotorAccessibilityProfile() const;
    void simulateKeyPress(int keyCode, bool longPress = false);
    void simulateTouch(int x, int y, bool longPress = false);
    void simulateGesture(AccessibilityGesture gesture, int x, int y);

    // High contrast operations
    void setHighContrastEnabled(bool enabled);
    void setHighContrastSettings(const HighContrastSettings& settings);
    HighContrastSettings getHighContrastSettings() const;
    void applyHighContrastFilter();
    void removeHighContrastFilter();

    // Text scaling operations
    void setTextScalingSettings(const TextScalingSettings& settings);
    TextScalingSettings getTextScalingSettings() const;
    void applyTextScaling();
    void removeTextScaling();

    // Text-to-Speech operations
    void speakText(const std::string& text, float rate = 1.0f, float pitch = 1.0f, float volume = 1.0f);
    void stopSpeaking();
    void pauseSpeaking();
    void resumeSpeaking();
    bool isSpeaking() const;

    // Speech-to-Text operations
    void startListening();
    void stopListening();
    void setListeningLanguage(const std::string& language);
    bool isListening() const;

    // Accessibility announcements
    void announceGameStart();
    void announceGameEnd();
    void announceLevelChange(int level);
    void announceScoreChange(int score);
    void announceAchievement(const std::string& achievement);
    void announceError(const std::string& error);
    void announceHint(const std::string& hint);

    // Accessibility testing
    void runAccessibilityAudit();
    std::vector<std::string> getAccessibilityViolations();
    void fixAccessibilityViolation(const std::string& violation);

    // Utility functions
    bool isScreenReaderActive() const;
    bool isVoiceControlActive() const;
    bool isHighContrastActive() const;
    bool isMotorAccessibilityActive() const;
    std::string getAccessibilityStatus() const;

private:
    void initializeAccessibilityServices();
    void checkAccessibilityState();
    void updateAccessibilityNodes();
    void processAccessibilityEvents();
    void startServiceThread();
    void stopServiceThread();
    void serviceThreadLoop();

    // JNI helper methods
    bool isAccessibilityServiceEnabledJNI(AccessibilityServiceType type);
    void requestAccessibilityPermissionJNI();
    void announceTextJNI(const std::string& text, int importance);
    void configureScreenReaderJNI(float speed, float pitch, float volume);
    void configureVoiceControlJNI(float sensitivity, float timeout);
    void applyHighContrastJNI(bool enabled, float contrastRatio);
    void applyTextScalingJNI(float scaleFactor, bool boldText);

    // Event processing
    void onAccessibilityEvent(const AccessibilityEvent& event);
    void onVoiceCommand(const VoiceCommand& command, float confidence);
    void onGestureDetected(AccessibilityGesture gesture, int x, int y);
    void onAccessibilityStateChanged(bool enabled);

    // Voice command processing
    VoiceCommand findBestMatchingCommand(const std::string& text);
    float calculateCommandSimilarity(const std::string& command1, const std::string& command2);
    void executeVoiceCommand(const VoiceCommand& command, const std::vector<std::string>& parameters);

    // Accessibility validation
    bool validateAccessibilityNode(const AccessibilityNodeInfo& node);
    bool hasAccessibleName(const AccessibilityNodeInfo& node);
    bool hasAccessibleDescription(const AccessibilityNodeInfo& node);
    bool hasProperContrast(const AccessibilityNodeInfo& node);
    bool hasMinimumSize(const AccessibilityNodeInfo& node);
    bool hasProperFocusOrder(const std::vector<AccessibilityNodeInfo>& nodes);
};

// ========== SCREEN READER ==========
class ScreenReader {
private:
    AccessibilityManager* manager_;

    // TTS settings
    float speechRate_;
    float speechPitch_;
    float speechVolume_;
    std::string language_;
    bool enabled_;

    // Reading state
    std::string currentText_;
    std::atomic<bool> isReading_;
    std::atomic<bool> isPaused_;
    std::queue<std::string> textQueue_;
    std::mutex queueMutex_;

public:
    ScreenReader(AccessibilityManager* manager);
    ~ScreenReader();

    bool initialize();
    void shutdown();

    // Control
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Settings
    void setSpeechRate(float rate);
    void setSpeechPitch(float pitch);
    void setSpeechVolume(float volume);
    void setLanguage(const std::string& language);

    float getSpeechRate() const { return speechRate_; }
    float getSpeechPitch() const { return speechPitch_; }
    float getSpeechVolume() const { return speechVolume_; }
    std::string getLanguage() const { return language_; }

    // Text reading
    void announce(const std::string& text, AccessibilityImportance importance = AccessibilityImportance::MEDIUM);
    void announceEvent(const AccessibilityEvent& event);
    void readCurrentScreen();
    void readNode(const AccessibilityNodeInfo& node);
    void readText(const std::string& text);

    // Control
    void stop();
    void pause();
    void resume();
    bool isReading() const { return isReading_; }
    bool isPaused() const { return isPaused_; }

    // Advanced features
    void setPunctuationMode(bool allPunctuation);
    void setNumberMode(bool readNumbers);
    void setCapitalMode(bool readCapitals);
    void setVerbosityLevel(int level);

private:
    void processTextQueue();
    std::string formatTextForReading(const std::string& text, AccessibilityImportance importance);
    std::string getNodeDescription(const AccessibilityNodeInfo& node);
    void speak(const std::string& text);
};

// ========== VOICE CONTROL ==========
class VoiceControl {
private:
    AccessibilityManager* manager_;

    // Voice recognition settings
    float sensitivity_;
    float timeout_;
    std::string language_;
    bool continuous_;
    bool enabled_;

    // Recognition state
    std::atomic<bool> isListening_;
    std::atomic<bool> isProcessing_;
    std::vector<VoiceCommand> activeCommands_;
    std::mutex commandsMutex_;

    // Speech recognition
    std::unique_ptr<SpeechToTextManager> speechToText_;

public:
    VoiceControl(AccessibilityManager* manager);
    ~VoiceControl();

    bool initialize();
    void shutdown();

    // Control
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Settings
    void setSensitivity(float sensitivity);
    void setTimeout(float timeout);
    void setLanguage(const std::string& language);
    void setContinuousMode(bool continuous);

    float getSensitivity() const { return sensitivity_; }
    float getTimeout() const { return timeout_; }
    std::string getLanguage() const { return language_; }
    bool isContinuousMode() const { return continuous_; }

    // Voice recognition
    void startListening();
    void stopListening();
    bool isListening() const { return isListening_; }
    bool isProcessing() const { return isProcessing_; }

    // Command management
    void addCommand(const VoiceCommand& command);
    void removeCommand(const std::string& command);
    void enableCommand(const std::string& command, bool enable);
    std::vector<VoiceCommand> getCommands() const;

    // Processing
    void processVoiceInput(const std::string& text, float confidence);
    void executeCommand(const VoiceCommand& command, const std::vector<std::string>& parameters);

private:
    void onSpeechRecognized(const std::string& text, float confidence);
    std::vector<std::string> parseCommandParameters(const std::string& text);
    VoiceCommand findMatchingCommand(const std::string& text);
    float calculateConfidence(const std::string& recognized, const std::string& command);
};

// ========== MOTOR ACCESSIBILITY ==========
class MotorAccessibility {
private:
    AccessibilityManager* manager_;

    // Motor accessibility settings
    MotorAccessibilityProfile profile_;
    bool enabled_;

    // Switch control
    std::atomic<int> currentSwitch_;
    std::atomic<int> switchCount_;
    std::vector<std::string> switchActions_;
    std::mutex switchMutex_;

    // Timing
    std::chrono::steady_clock::time_point lastInputTime_;
    std::chrono::steady_clock::time_point lastRepeatTime_;
    std::mutex timingMutex_;

public:
    MotorAccessibility(AccessibilityManager* manager);
    ~MotorAccessibility();

    bool initialize();
    void shutdown();

    // Control
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Profile management
    void setProfile(const MotorAccessibilityProfile& profile);
    MotorAccessibilityProfile getProfile() const { return profile_; }

    // Switch control
    void setSwitchCount(int count);
    int getSwitchCount() const { return switchCount_; }
    void selectNextSwitch();
    void selectPreviousSwitch();
    int getCurrentSwitch() const { return currentSwitch_; }
    void activateCurrentSwitch();
    void addSwitchAction(const std::string& action);
    void removeSwitchAction(const std::string& action);

    // Input simulation
    void simulateKeyPress(int keyCode, bool longPress = false);
    void simulateTouch(int x, int y, bool longPress = false);
    void simulateGesture(AccessibilityGesture gesture, int x, int y);
    void simulateScroll(int x, int y, int deltaX, int deltaY);

    // Timing and delays
    void setInputDelay(int delayMs);
    void setRepeatDelay(int delayMs);
    void setRepeatRate(int rateMs);
    bool shouldRepeatInput();

    // Sticky keys
    void enableStickyKeys(bool enable);
    void enableSlowKeys(bool enable);
    void enableMouseKeys(bool enable);

private:
    void processSwitchInput();
    void executeSwitchAction(const std::string& action);
    bool isValidSwitchAction(const std::string& action);
    void updateSwitchActions();
};

// ========== HIGH CONTRAST MANAGER ==========
class HighContrastManager {
private:
    AccessibilityManager* manager_;

    // High contrast settings
    HighContrastSettings settings_;
    bool enabled_;

    // Color filters
    std::unordered_map<std::string, std::string> colorFilters_;
    std::mutex filterMutex_;

public:
    HighContrastManager(AccessibilityManager* manager);
    ~HighContrastManager();

    bool initialize();
    void shutdown();

    // Control
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Settings
    void setSettings(const HighContrastSettings& settings);
    HighContrastSettings getSettings() const { return settings_; }

    // Color filters
    void setContrastRatio(float ratio);
    void setInvertColors(bool invert);
    void setGrayscale(bool grayscale);
    void setHighSaturation(bool highSat);
    void setColorTheme(const std::string& theme);

    // Filter application
    void applyColorFilter();
    void removeColorFilter();
    void updateColorFilter();

    // Color transformation
    std::string transformColor(const std::string& color);
    std::string increaseContrast(const std::string& color);
    std::string invertColor(const std::string& color);
    std::string convertToGrayscale(const std::string& color);

private:
    void loadColorFilters();
    void saveColorFilters();
    std::string applyColorTransformation(const std::string& color);
    bool isValidColor(const std::string& color);
};

// ========== TEXT TO SPEECH MANAGER ==========
class TextToSpeechManager {
private:
    AccessibilityManager* manager_;

    // TTS engine
    void* ttsEngine_;
    bool initialized_;
    bool speaking_;

    // Settings
    float speechRate_;
    float speechPitch_;
    float speechVolume_;
    std::string language_;
    std::string voice_;

    // Queue
    std::queue<std::string> speechQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::atomic<bool> processingQueue_;

public:
    TextToSpeechManager(AccessibilityManager* manager);
    ~TextToSpeechManager();

    bool initialize();
    void shutdown();

    // Control
    void speak(const std::string& text);
    void stop();
    void pause();
    void resume();
    bool isSpeaking() const { return speaking_; }

    // Settings
    void setSpeechRate(float rate);
    void setSpeechPitch(float pitch);
    void setSpeechVolume(float volume);
    void setLanguage(const std::string& language);
    void setVoice(const std::string& voice);

    float getSpeechRate() const { return speechRate_; }
    float getSpeechPitch() const { return speechPitch_; }
    float getSpeechVolume() const { return speechVolume_; }
    std::string getLanguage() const { return language_; }
    std::string getVoice() const { return voice_; }

    // Advanced features
    void setPunctuationMode(bool allPunctuation);
    void setNumberMode(bool readNumbers);
    void setCapitalMode(bool readCapitals);
    std::vector<std::string> getAvailableVoices() const;
    std::vector<std::string> getAvailableLanguages() const;

private:
    void processSpeechQueue();
    void onSpeechStart();
    void onSpeechEnd();
    void onSpeechError(const std::string& error);
};

// ========== SPEECH TO TEXT MANAGER ==========
class SpeechToTextManager {
private:
    AccessibilityManager* manager_;

    // STT engine
    void* sttEngine_;
    bool initialized_;
    bool listening_;

    // Settings
    std::string language_;
    float sensitivity_;
    float timeout_;
    bool continuous_;

    // Recognition
    std::atomic<bool> isRecognizing_;
    std::function<void(const std::string&, float)> onResult_;
    std::function<void(const std::string&)> onError_;
    std::function<void()> onStart_;
    std::function<void()> onEnd_;

public:
    SpeechToTextManager(AccessibilityManager* manager);
    ~SpeechToTextManager();

    bool initialize();
    void shutdown();

    // Control
    void startListening();
    void stopListening();
    bool isListening() const { return listening_; }
    bool isRecognizing() const { return isRecognizing_; }

    // Settings
    void setLanguage(const std::string& language);
    void setSensitivity(float sensitivity);
    void setTimeout(float timeout);
    void setContinuousMode(bool continuous);

    std::string getLanguage() const { return language_; }
    float getSensitivity() const { return sensitivity_; }
    float getTimeout() const { return timeout_; }
    bool isContinuousMode() const { return continuous_; }

    // Callbacks
    void setOnResultCallback(std::function<void(const std::string&, float)> callback);
    void setOnErrorCallback(std::function<void(const std::string&)> callback);
    void setOnStartCallback(std::function<void()> callback);
    void setOnEndCallback(std::function<void()> callback);

    // Advanced features
    std::vector<std::string> getAvailableLanguages() const;
    bool isLanguageSupported(const std::string& language) const;
    void setMaxAlternatives(int max);
    void enableOfflineRecognition(bool enable);

private:
    void onRecognitionStart();
    void onRecognitionEnd();
    void onRecognitionResult(const std::string& text, float confidence);
    void onRecognitionError(const std::string& error);
    void processRecognitionResult(const std::string& text, float confidence);
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // Accessibility service callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onAccessibilityEvent(
        JNIEnv* env, jobject thiz, jstring eventType, jstring sourceId, jstring text,
        jstring description, int nodeType, int x, int y, int width, int height,
        jboolean enabled, jboolean visible, jboolean focused, jboolean selected,
        int importance);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onGestureDetected(
        JNIEnv* env, jobject thiz, int gestureType, int x, int y);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onVoiceCommand(
        JNIEnv* env, jobject thiz, jstring command, jfloat confidence);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onAccessibilityStateChanged(
        JNIEnv* env, jobject thiz, jboolean enabled);

    // Screen reader callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onScreenReaderStateChanged(
        JNIEnv* env, jobject thiz, jboolean enabled);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onSpeechStart(
        JNIEnv* env, jobject thiz);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onSpeechEnd(
        JNIEnv* env, jobject thiz);

    // Voice control callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onVoiceControlStateChanged(
        JNIEnv* env, jobject thiz, jboolean enabled);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onSpeechRecognitionResult(
        JNIEnv* env, jobject thiz, jstring text, jfloat confidence);

    // Motor accessibility callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onSwitchActivated(
        JNIEnv* env, jobject thiz, int switchIndex);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onMotorAccessibilityStateChanged(
        JNIEnv* env, jobject thiz, jboolean enabled);

    // High contrast callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onHighContrastStateChanged(
        JNIEnv* env, jobject thiz, jboolean enabled);

    // Text scaling callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_AccessibilityManager_onTextScalingChanged(
        JNIEnv* env, jobject thiz, jfloat scaleFactor);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_ACCESSIBILITY_MANAGER_H
