#ifndef FOUNDRYENGINE_PUSH_NOTIFICATION_MANAGER_H
#define FOUNDRYENGINE_PUSH_NOTIFICATION_MANAGER_H

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
class PushNotificationManager;
class FirebaseManager;
class NotificationChannelManager;
class LocalNotificationManager;
class NotificationScheduler;

// Notification types
enum class NotificationType {
    REMOTE_PUSH,        // Firebase Cloud Messaging
    LOCAL_SCHEDULED,    // Scheduled local notifications
    LOCAL_IMMEDIATE,    // Immediate local notifications
    IN_APP_ALERT,       // In-app alerts
    SYSTEM_ALERT        // System-level alerts
};

// Notification priority levels
enum class NotificationPriority {
    LOW = -1,
    DEFAULT = 0,
    HIGH = 1,
    MAX = 2
};

// Notification visibility levels
enum class NotificationVisibility {
    PRIVATE,        // No content shown on lock screen
    PUBLIC,         // Full content shown on lock screen
    SECRET          // No notification shown on lock screen
};

// Notification categories
enum class NotificationCategory {
    GAME_UPDATE,    // Game updates and news
    SOCIAL,         // Friend requests, messages
    ACHIEVEMENT,    // Achievement unlocked
    PROMOTION,      // Special offers, sales
    SYSTEM,         // System messages, maintenance
    CUSTOM          // Custom categories
};

// Notification channel importance
enum class NotificationChannelImportance {
    NONE = 0,       // No sound or visual interruption
    MIN = 1,        // No sound, visual interruption
    LOW = 2,        // No sound, no visual interruption
    DEFAULT = 3,    // Sound, no visual interruption
    HIGH = 4        // Sound and visual interruption
};

// Notification action types
enum class NotificationActionType {
    OPEN_APP,       // Opens the main app
    OPEN_ACTIVITY,  // Opens a specific activity
    OPEN_URL,       // Opens a URL
    DISMISS,        // Dismisses the notification
    CUSTOM          // Custom action
};

// Notification click behavior
enum class NotificationClickBehavior {
    OPEN_APP,       // Open the main app
    OPEN_ACTIVITY,  // Open a specific activity
    OPEN_URL,       // Open a URL in browser
    DISMISS_ONLY    // Just dismiss the notification
};

// Notification sound types
enum class NotificationSoundType {
    DEFAULT,
    CUSTOM,
    SILENT,
    VIBRATE_ONLY
};

// Notification light patterns
enum class NotificationLightPattern {
    NONE,
    SLOW_BLINK,
    FAST_BLINK,
    CONSTANT
};

// Push notification message
struct PushMessage {
    std::string title;
    std::string body;
    std::string imageUrl;
    std::string actionUrl;
    std::unordered_map<std::string, std::string> data;
    NotificationPriority priority;
    NotificationCategory category;
    bool showBadge;
    int badgeCount;
    std::string sound;
    std::string channelId;
    std::string tag;
    std::string group;
    time_t timestamp;
    std::string senderId;
    std::string messageId;
};

// Notification channel
struct NotificationChannel {
    std::string id;
    std::string name;
    std::string description;
    NotificationChannelImportance importance;
    bool showBadge;
    NotificationSoundType soundType;
    std::string soundResource;
    NotificationLightPattern lightPattern;
    int lightColor;
    std::vector<int> vibrationPattern;
    bool canBypassDnd;
    bool canShowOnLockedScreen;
    std::vector<std::string> allowedCategories;
};

// Notification action
struct NotificationAction {
    std::string id;
    std::string title;
    NotificationActionType type;
    std::string activityClass;
    std::string url;
    std::string icon;
    bool remoteInput;
    std::string remoteInputPlaceholder;
    std::unordered_map<std::string, std::string> extras;
};

// Scheduled notification
struct ScheduledNotification {
    std::string id;
    PushMessage message;
    std::chrono::system_clock::time_point scheduledTime;
    std::chrono::system_clock::time_point repeatInterval;
    int repeatCount;
    bool repeatForever;
    bool allowWhileIdle;
    std::string triggerCondition;
};

// Notification response
struct NotificationResponse {
    std::string notificationId;
    NotificationAction action;
    std::string inputText;
    bool dismissed;
    time_t responseTime;
};

// Notification statistics
struct NotificationStats {
    int totalSent;
    int totalDelivered;
    int totalOpened;
    int totalDismissed;
    int totalFailed;
    std::unordered_map<std::string, int> categoryStats;
    std::unordered_map<std::string, int> channelStats;
    time_t lastUpdated;
};

// Callback types
using NotificationReceivedCallback = std::function<void(const PushMessage&)>;
using NotificationClickedCallback = std::function<void(const NotificationResponse&)>;
using NotificationDismissedCallback = std::function<void(const std::string&)>;
using TokenReceivedCallback = std::function<void(const std::string&)>;
using TokenErrorCallback = std::function<void(const std::string&)>;

// ========== PUSH NOTIFICATION MANAGER ==========
class PushNotificationManager : public System {
private:
    static PushNotificationManager* instance_;

    FirebaseManager* firebaseManager_;
    NotificationChannelManager* channelManager_;
    LocalNotificationManager* localManager_;
    NotificationScheduler* scheduler_;

    // JNI environment
    JNIEnv* env_;
    jobject context_;

    // Notification state
    std::atomic<bool> initialized_;
    std::atomic<bool> firebaseEnabled_;
    std::atomic<bool> localEnabled_;
    std::string firebaseToken_;
    std::string senderId_;

    // Callbacks
    std::unordered_map<std::string, NotificationReceivedCallback> receivedCallbacks_;
    std::unordered_map<std::string, NotificationClickedCallback> clickedCallbacks_;
    std::unordered_map<std::string, NotificationDismissedCallback> dismissedCallbacks_;
    std::unordered_map<std::string, TokenReceivedCallback> tokenCallbacks_;
    std::unordered_map<std::string, TokenErrorCallback> tokenErrorCallbacks_;

    // Notification management
    std::unordered_map<std::string, PushMessage> activeNotifications_;
    std::unordered_map<std::string, ScheduledNotification> scheduledNotifications_;
    std::mutex notificationsMutex_;

    // Statistics
    NotificationStats stats_;
    std::mutex statsMutex_;

    // Settings
    bool autoInit_;
    bool enableAnalytics_;
    bool enableBadge_;
    bool enableSound_;
    bool enableVibration_;
    bool enableLights_;
    int defaultBadgeCount_;

    // Service management
    std::atomic<bool> serviceRunning_;
    std::thread serviceThread_;
    std::thread statsThread_;

public:
    PushNotificationManager();
    ~PushNotificationManager();

    static PushNotificationManager* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // JNI setup
    void setJNIEnvironment(JNIEnv* env, jobject context);

    // Firebase Cloud Messaging
    bool initializeFirebase(const std::string& senderId);
    void enableFirebase(bool enable);
    bool isFirebaseEnabled() const { return firebaseEnabled_; }
    std::string getFirebaseToken() const { return firebaseToken_; }
    void refreshFirebaseToken();
    void subscribeToTopic(const std::string& topic);
    void unsubscribeFromTopic(const std::string& topic);

    // Local notifications
    void enableLocalNotifications(bool enable);
    bool isLocalNotificationsEnabled() const { return localEnabled_; }

    // Notification channels
    void createNotificationChannel(const NotificationChannel& channel);
    void deleteNotificationChannel(const std::string& channelId);
    void updateNotificationChannel(const NotificationChannel& channel);
    std::vector<NotificationChannel> getNotificationChannels() const;
    NotificationChannel getNotificationChannel(const std::string& channelId) const;

    // Push message handling
    void sendPushNotification(const PushMessage& message);
    void sendPushNotification(const std::string& title, const std::string& body,
                            const std::string& channelId = "default");
    void sendScheduledNotification(const ScheduledNotification& notification);
    void cancelNotification(const std::string& notificationId);
    void cancelAllNotifications();
    void cancelNotificationsByTag(const std::string& tag);
    void cancelNotificationsByGroup(const std::string& group);

    // Notification actions
    void addNotificationAction(const NotificationAction& action);
    void removeNotificationAction(const std::string& actionId);
    std::vector<NotificationAction> getNotificationActions() const;

    // Notification groups
    void createNotificationGroup(const std::string& groupId, const std::string& groupName);
    void setNotificationGroupSummary(const std::string& groupId, const std::string& title, const std::string& summary);
    void removeNotificationGroup(const std::string& groupId);

    // Badge management
    void setBadgeCount(int count);
    int getBadgeCount() const { return defaultBadgeCount_; }
    void clearBadgeCount();
    void enableBadge(bool enable);

    // Sound and vibration
    void setSoundEnabled(bool enable);
    void setVibrationEnabled(bool enable);
    void setLightsEnabled(bool enable);
    void setCustomSound(const std::string& soundResource);
    void setVibrationPattern(const std::vector<int>& pattern);
    void setLightPattern(NotificationLightPattern pattern, int color);

    // Analytics and statistics
    void enableAnalytics(bool enable);
    NotificationStats getNotificationStats() const;
    void resetNotificationStats();
    void updateNotificationStats(const std::string& event, const std::string& category = "");

    // Callback management
    void registerNotificationReceivedCallback(const std::string& id, NotificationReceivedCallback callback);
    void unregisterNotificationReceivedCallback(const std::string& id);
    void registerNotificationClickedCallback(const std::string& id, NotificationClickedCallback callback);
    void unregisterNotificationClickedCallback(const std::string& id);
    void registerNotificationDismissedCallback(const std::string& id, NotificationDismissedCallback callback);
    void unregisterNotificationDismissedCallback(const std::string& id);
    void registerTokenReceivedCallback(const std::string& id, TokenReceivedCallback callback);
    void unregisterTokenReceivedCallback(const std::string& id);
    void registerTokenErrorCallback(const std::string& id, TokenErrorCallback callback);
    void unregisterTokenErrorCallback(const std::string& id);

    // Advanced features
    void setNotificationClickBehavior(NotificationClickBehavior behavior);
    void setNotificationVisibility(NotificationVisibility visibility);
    void setNotificationPriority(NotificationPriority priority);
    void setAutoCancel(bool autoCancel);
    void setOnlyAlertOnce(bool onlyAlertOnce);

    // In-app notifications
    void showInAppNotification(const std::string& title, const std::string& message,
                             float duration = 3.0f, const std::string& position = "bottom");
    void dismissInAppNotification();

    // Notification testing
    void testNotification(const PushMessage& message);
    void testLocalNotification();
    void testScheduledNotification();

    // Utility functions
    bool areNotificationsEnabled() const;
    bool isFirebaseAvailable() const;
    std::string getNotificationStatus() const;
    void requestNotificationPermission();

private:
    void initializeDefaults();
    void startServiceThreads();
    void stopServiceThreads();
    void serviceThreadLoop();
    void statsThreadLoop();

    // JNI helper methods
    bool initializeFirebaseJNI(const std::string& senderId);
    void sendPushNotificationJNI(const PushMessage& message);
    void scheduleNotificationJNI(const ScheduledNotification& notification);
    void cancelNotificationJNI(const std::string& notificationId);
    std::string getFirebaseTokenJNI();
    void subscribeToTopicJNI(const std::string& topic);
    void unsubscribeFromTopicJNI(const std::string& topic);

    // Event processing
    void onPushMessageReceived(const PushMessage& message);
    void onNotificationClicked(const NotificationResponse& response);
    void onNotificationDismissed(const std::string& notificationId);
    void onFirebaseTokenReceived(const std::string& token);
    void onFirebaseTokenError(const std::string& error);

    // Statistics
    void incrementStat(const std::string& statName, const std::string& category = "");
    void saveStatsToStorage();
    void loadStatsFromStorage();

    // Notification management
    std::string generateNotificationId();
    std::string generateChannelId();
    bool isValidNotificationChannel(const NotificationChannel& channel);
    bool isValidPushMessage(const PushMessage& message);
    bool isValidScheduledNotification(const ScheduledNotification& notification);
};

// ========== FIREBASE MANAGER ==========
class FirebaseManager {
private:
    PushNotificationManager* manager_;

    // Firebase configuration
    std::string projectId_;
    std::string senderId_;
    std::string apiKey_;
    std::string appId_;
    std::string token_;

    // Firebase state
    std::atomic<bool> initialized_;
    std::atomic<bool> connected_;
    std::vector<std::string> subscribedTopics_;
    std::mutex topicsMutex_;

    // Message handling
    std::queue<PushMessage> messageQueue_;
    std::mutex messageMutex_;
    std::condition_variable messageCondition_;

public:
    FirebaseManager(PushNotificationManager* manager);
    ~FirebaseManager();

    bool initialize(const std::string& senderId);
    void shutdown();

    // Configuration
    void setProjectId(const std::string& projectId);
    void setApiKey(const std::string& apiKey);
    void setAppId(const std::string& appId);

    std::string getProjectId() const { return projectId_; }
    std::string getSenderId() const { return senderId_; }
    std::string getApiKey() const { return apiKey_; }
    std::string getAppId() const { return appId_; }
    std::string getToken() const { return token_; }

    // Connection management
    bool isInitialized() const { return initialized_; }
    bool isConnected() const { return connected_; }
    void connect();
    void disconnect();

    // Topic management
    void subscribeToTopic(const std::string& topic);
    void unsubscribeFromTopic(const std::string& topic);
    std::vector<std::string> getSubscribedTopics() const;

    // Message handling
    void sendMessage(const PushMessage& message);
    void processMessageQueue();

    // Token management
    void refreshToken();
    void invalidateToken();

private:
    void onTokenReceived(const std::string& token);
    void onTokenError(const std::string& error);
    void onMessageReceived(const PushMessage& message);
    void processFirebaseMessage(const std::string& jsonMessage);
    PushMessage parseFirebaseMessage(const std::string& json);
    bool validateFirebaseConfig();
};

// ========== NOTIFICATION CHANNEL MANAGER ==========
class NotificationChannelManager {
private:
    PushNotificationManager* manager_;

    // Channel storage
    std::unordered_map<std::string, NotificationChannel> channels_;
    std::mutex channelsMutex_;

    // Default channels
    std::vector<NotificationChannel> defaultChannels_;

public:
    NotificationChannelManager(PushNotificationManager* manager);
    ~NotificationChannelManager();

    bool initialize();
    void shutdown();

    // Channel management
    void createChannel(const NotificationChannel& channel);
    void deleteChannel(const std::string& channelId);
    void updateChannel(const NotificationChannel& channel);
    NotificationChannel getChannel(const std::string& channelId) const;
    std::vector<NotificationChannel> getAllChannels() const;
    std::vector<NotificationChannel> getChannelsByImportance(NotificationChannelImportance importance) const;

    // Default channels
    void createDefaultChannels();
    void resetToDefaultChannels();
    bool isDefaultChannel(const std::string& channelId) const;

    // Channel validation
    bool isValidChannel(const NotificationChannel& channel);
    bool channelExists(const std::string& channelId) const;
    std::string generateChannelId(const std::string& baseName);

private:
    void loadDefaultChannels();
    void saveChannelsToStorage();
    void loadChannelsFromStorage();
    NotificationChannel createGameNotificationChannel();
    NotificationChannel createSocialNotificationChannel();
    NotificationChannel createAchievementNotificationChannel();
    NotificationChannel createSystemNotificationChannel();
};

// ========== LOCAL NOTIFICATION MANAGER ==========
class LocalNotificationManager {
private:
    PushNotificationManager* manager_;

    // Notification storage
    std::unordered_map<std::string, PushMessage> activeNotifications_;
    std::mutex notificationsMutex_;

    // Settings
    bool soundEnabled_;
    bool vibrationEnabled_;
    bool lightsEnabled_;
    std::string customSound_;
    std::vector<int> vibrationPattern_;
    NotificationLightPattern lightPattern_;
    int lightColor_;

public:
    LocalNotificationManager(PushNotificationManager* manager);
    ~LocalNotificationManager();

    bool initialize();
    void shutdown();

    // Notification sending
    void sendNotification(const PushMessage& message);
    void sendNotification(const std::string& title, const std::string& body,
                         const std::string& channelId = "default");
    void sendNotificationWithActions(const PushMessage& message,
                                   const std::vector<NotificationAction>& actions);

    // Notification management
    void cancelNotification(const std::string& notificationId);
    void cancelAllNotifications();
    std::vector<PushMessage> getActiveNotifications() const;
    bool isNotificationActive(const std::string& notificationId) const;

    // Settings
    void setSoundEnabled(bool enabled);
    void setVibrationEnabled(bool enabled);
    void setLightsEnabled(bool enabled);
    void setCustomSound(const std::string& soundResource);
    void setVibrationPattern(const std::vector<int>& pattern);
    void setLightPattern(NotificationLightPattern pattern, int color);

    bool isSoundEnabled() const { return soundEnabled_; }
    bool isVibrationEnabled() const { return vibrationEnabled_; }
    bool isLightsEnabled() const { return lightsEnabled_; }
    std::string getCustomSound() const { return customSound_; }
    std::vector<int> getVibrationPattern() const { return vibrationPattern_; }
    NotificationLightPattern getLightPattern() const { return lightPattern_; }
    int getLightColor() const { return lightColor_; }

private:
    void applyNotificationSettings(PushMessage& message);
    std::string generateNotificationId();
    bool validateNotification(const PushMessage& message);
    void saveNotificationToHistory(const PushMessage& message);
};

// ========== NOTIFICATION SCHEDULER ==========
class NotificationScheduler {
private:
    PushNotificationManager* manager_;

    // Scheduled notifications
    std::unordered_map<std::string, ScheduledNotification> scheduledNotifications_;
    std::mutex scheduledMutex_;

    // Scheduling thread
    std::atomic<bool> schedulerRunning_;
    std::thread schedulerThread_;
    std::condition_variable schedulerCondition_;

    // Settings
    bool allowWhileIdle_;
    int maxScheduledNotifications_;

public:
    NotificationScheduler(PushNotificationManager* manager);
    ~NotificationScheduler();

    bool initialize();
    void shutdown();

    // Scheduling
    void scheduleNotification(const ScheduledNotification& notification);
    void cancelScheduledNotification(const std::string& notificationId);
    void cancelAllScheduledNotifications();
    std::vector<ScheduledNotification> getScheduledNotifications() const;
    std::vector<ScheduledNotification> getScheduledNotificationsInTimeRange(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const;

    // Settings
    void setAllowWhileIdle(bool allow);
    void setMaxScheduledNotifications(int max);
    bool isAllowWhileIdle() const { return allowWhileIdle_; }
    int getMaxScheduledNotifications() const { return maxScheduledNotifications_; }

    // Advanced scheduling
    void scheduleRepeatingNotification(const std::string& id, const PushMessage& message,
                                     std::chrono::seconds interval, int repeatCount = -1);
    void scheduleDailyNotification(const std::string& id, const PushMessage& message, int hour, int minute);
    void scheduleWeeklyNotification(const std::string& id, const PushMessage& message,
                                  int dayOfWeek, int hour, int minute);

private:
    void startSchedulerThread();
    void stopSchedulerThread();
    void schedulerThreadLoop();
    void processScheduledNotifications();
    void checkAndTriggerNotifications();
    bool isTimeToTrigger(const ScheduledNotification& notification) const;
    void triggerNotification(const ScheduledNotification& notification);
    std::string generateScheduleId();
    bool validateScheduledNotification(const ScheduledNotification& notification);
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // Firebase callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onFirebaseTokenReceived(
        JNIEnv* env, jobject thiz, jstring token);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onFirebaseTokenError(
        JNIEnv* env, jobject thiz, jstring error);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onFirebaseMessageReceived(
        JNIEnv* env, jobject thiz, jstring messageJson);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onFirebaseConnectionChanged(
        JNIEnv* env, jobject thiz, jboolean connected);

    // Notification callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onNotificationReceived(
        JNIEnv* env, jobject thiz, jstring notificationId, jstring title, jstring body);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onNotificationClicked(
        JNIEnv* env, jobject thiz, jstring notificationId, jstring actionId, jstring inputText);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onNotificationDismissed(
        JNIEnv* env, jobject thiz, jstring notificationId);

    // Local notification callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onLocalNotificationScheduled(
        JNIEnv* env, jobject thiz, jstring notificationId);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onLocalNotificationTriggered(
        JNIEnv* env, jobject thiz, jstring notificationId);

    // Permission callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onNotificationPermissionResult(
        JNIEnv* env, jobject thiz, jboolean granted);

    // Statistics callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PushNotificationManager_onNotificationStatsUpdated(
        JNIEnv* env, jobject thiz, jstring statsJson);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_PUSH_NOTIFICATION_MANAGER_H
