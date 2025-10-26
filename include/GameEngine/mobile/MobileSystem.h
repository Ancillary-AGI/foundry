/**
 * @file MobileSystem.h
 * @brief Mobile-first optimization system for iOS and Android
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector2.h"
#include <memory>
#include <vector>
#include <functional>

namespace FoundryEngine {

/**
 * @class MobileSystem
 * @brief Comprehensive mobile platform integration and optimization
 */
class MobileSystem : public System {
public:
    enum class Platform {
        iOS,
        Android,
        Unknown
    };

    enum class DeviceType {
        Phone,
        Tablet,
        Foldable,
        Unknown
    };

    enum class PowerMode {
        HighPerformance,
        Balanced,
        PowerSaver,
        Adaptive
    };

    struct DeviceInfo {
        Platform platform;
        DeviceType deviceType;
        std::string model;
        std::string osVersion;
        uint32_t screenWidth;
        uint32_t screenHeight;
        float screenDensity;
        uint32_t memoryMB;
        std::string gpuModel;
        uint32_t cpuCores;
        bool hasNotch;
        bool supportsTouchID;
        bool supportsFaceID;
    };

    struct BatteryInfo {
        float level; // 0.0 to 1.0
        bool isCharging;
        bool isLowPowerMode;
        float temperature;
        uint32_t cycleCount;
        std::string health;
    };

    struct PerformanceMetrics {
        float cpuUsage;
        float gpuUsage;
        float memoryUsage;
        float batteryDrain;
        float thermalState;
        uint32_t frameRate;
        float frameTime;
    };

    MobileSystem();
    ~MobileSystem();

    bool initialize() override;
    void shutdown() override;
    void update(float deltaTime) override;

    // Device information
    DeviceInfo getDeviceInfo() const;
    Platform getPlatform() const;
    DeviceType getDeviceType() const;
    bool isTablet() const;
    bool hasNotch() const;

    // Battery management
    BatteryInfo getBatteryInfo() const;
    void setPowerMode(PowerMode mode);
    PowerMode getPowerMode() const;
    void enableBatteryOptimization(bool enable);

    // Performance monitoring
    PerformanceMetrics getPerformanceMetrics() const;
    void setPerformanceTarget(uint32_t targetFPS);
    void enableAdaptiveQuality(bool enable);
    void setThermalThrottling(bool enable);

    // Touch input
    struct TouchPoint {
        uint32_t id;
        Vector2 position;
        Vector2 previousPosition;
        float pressure;
        float radius;
        bool isActive;
    };

    std::vector<TouchPoint> getTouchPoints() const;
    void enableMultitouch(bool enable);
    void setTouchSensitivity(float sensitivity);

    // Gestures
    enum class GestureType {
        Tap,
        DoubleTap,
        LongPress,
        Swipe,
        Pinch,
        Rotate,
        Pan
    };

    struct Gesture {
        GestureType type;
        Vector2 position;
        Vector2 velocity;
        float scale;
        float rotation;
        float duration;
    };

    void enableGestureRecognition(bool enable);
    std::vector<Gesture> getGestures() const;
    void setGestureCallback(std::function<void(const Gesture&)> callback);

    // Haptic feedback
    void triggerHapticFeedback(const std::string& type, float intensity = 1.0f);
    void playHapticPattern(const std::vector<float>& pattern);
    bool isHapticSupported() const;

    // Notifications
    void scheduleNotification(const std::string& title, const std::string& body, 
                             uint32_t delaySeconds);
    void cancelNotification(uint32_t notificationId);
    void clearAllNotifications();
    void setBadgeNumber(uint32_t count);

    // App lifecycle
    void setAppStateCallback(std::function<void(const std::string&)> callback);
    void setMemoryWarningCallback(std::function<void()> callback);
    void setOrientationCallback(std::function<void(const std::string&)> callback);

    // In-app purchases
    void initializeStore();
    void purchaseProduct(const std::string& productId);
    void restorePurchases();
    std::vector<std::string> getPurchasedProducts() const;
    void setPurchaseCallback(std::function<void(const std::string&, bool)> callback);

    // Social integration
    void shareText(const std::string& text);
    void shareImage(const std::string& imagePath, const std::string& text = "");
    void openURL(const std::string& url);
    void rateApp();

private:
    class MobileSystemImpl;
    std::unique_ptr<MobileSystemImpl> impl_;
};

/**
 * @class AdaptiveRenderer
 * @brief Dynamic quality scaling for mobile performance
 */
class AdaptiveRenderer {
public:
    struct QualitySettings {
        float renderScale = 1.0f;
        uint32_t shadowQuality = 2; // 0-3
        uint32_t textureQuality = 2; // 0-3
        uint32_t effectsQuality = 2; // 0-3
        bool enablePostProcessing = true;
        bool enableAntiAliasing = true;
        uint32_t maxLights = 8;
        float lodBias = 1.0f;
    };

    struct PerformanceTarget {
        uint32_t targetFPS = 60;
        float maxFrameTime = 16.67f; // ms
        float maxCPUUsage = 0.8f;
        float maxGPUUsage = 0.8f;
        float maxThermalState = 0.7f;
    };

    AdaptiveRenderer();
    ~AdaptiveRenderer();

    void initialize();
    void shutdown();
    void update(float deltaTime);

    // Quality management
    void setQualitySettings(const QualitySettings& settings);
    QualitySettings getQualitySettings() const;
    void setQualityPreset(const std::string& preset);
    std::vector<std::string> getAvailablePresets() const;

    // Performance targeting
    void setPerformanceTarget(const PerformanceTarget& target);
    PerformanceTarget getPerformanceTarget() const;
    void enableAdaptiveScaling(bool enable);

    // Manual overrides
    void setRenderScale(float scale);
    void setShadowQuality(uint32_t quality);
    void setTextureQuality(uint32_t quality);
    void setEffectsQuality(uint32_t quality);

    // Statistics
    float getCurrentFPS() const;
    float getAverageFrameTime() const;
    uint32_t getQualityAdjustments() const;

private:
    class AdaptiveRendererImpl;
    std::unique_ptr<AdaptiveRendererImpl> impl_;
};

/**
 * @class MobileUI
 * @brief Responsive UI system optimized for mobile devices
 */
class MobileUI {
public:
    enum class Orientation {
        Portrait,
        Landscape,
        Auto
    };

    struct SafeArea {
        float top;
        float bottom;
        float left;
        float right;
    };

    struct UIConfig {
        Orientation orientation = Orientation::Auto;
        bool enableSafeArea = true;
        float scaleFactor = 1.0f;
        bool enableAccessibility = true;
        std::string fontFamily = "system";
    };

    MobileUI();
    ~MobileUI();

    void initialize(const UIConfig& config);
    void shutdown();
    void update(float deltaTime);

    // Layout management
    void setOrientation(Orientation orientation);
    Orientation getOrientation() const;
    SafeArea getSafeArea() const;
    Vector2 getScreenSize() const;
    float getScaleFactor() const;

    // Touch-friendly controls
    void setMinimumTouchSize(float size);
    void enableTouchFeedback(bool enable);
    void setTouchFeedbackStyle(const std::string& style);

    // Accessibility
    void enableVoiceOver(bool enable);
    void setAccessibilityLabel(uint32_t elementId, const std::string& label);
    void setAccessibilityHint(uint32_t elementId, const std::string& hint);

    // Keyboard handling
    void showKeyboard(const std::string& type = "default");
    void hideKeyboard();
    bool isKeyboardVisible() const;
    float getKeyboardHeight() const;

    // Navigation
    void enableSwipeNavigation(bool enable);
    void setNavigationGestures(const std::vector<std::string>& gestures);
    void setBackButtonCallback(std::function<void()> callback);

private:
    class MobileUIImpl;
    std::unique_ptr<MobileUIImpl> impl_;
};

} // namespace FoundryEngine