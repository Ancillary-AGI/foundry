#include "PushNotificationManager.h"
#include <android/log.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <random>
#include <nlohmann/json.hpp> // For JSON parsing

#define LOG_TAG "PushNotificationManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace FoundryEngine {

// Static instance
PushNotificationManager* PushNotificationManager::instance_ = nullptr;

// ========== PUSH NOTIFICATION MANAGER IMPLEMENTATION ==========
PushNotificationManager::PushNotificationManager() : firebaseManager_(nullptr), channelManager_(nullptr),
                                                     localManager_(nullptr), scheduler_(nullptr),
                                                     env_(nullptr), context_(nullptr), initialized_(false),
                                                     firebaseEnabled_(false), localEnabled_(false),
                                                     autoInit_(true), enableAnalytics_(true),
                                                     enableBadge_(true), enableSound_(true),
                                                     enableVibration_(true), enableLights_(true),
                                                     defaultBadgeCount_(0), serviceRunning_(false) {
    LOGI("PushNotificationManager constructor called");
}

PushNotificationManager::~PushNotificationManager() {
    shutdown();
    LOGI("PushNotificationManager destructor called");
}

PushNotificationManager* PushNotificationManager::getInstance() {
    if (!instance_) {
        instance_ = new PushNotificationManager();
    }
    return instance_;
}

void PushNotificationManager::initialize() {
    LOGI("Initializing Push Notification Manager");

    if (initialized_) {
        LOGW("Push Notification Manager already initialized");
        return;
    }

    // Initialize sub-managers
    firebaseManager_ = new FirebaseManager(this);
    channelManager_ = new NotificationChannelManager(this);
    localManager_ = new LocalNotificationManager(this);
    scheduler_ = new NotificationScheduler(this);

    // Initialize defaults
    initializeDefaults();

    // Start service threads
    startServiceThreads();

    // Initialize notification channels
    if (channelManager_->initialize()) {
        LOGI("Notification Channel Manager initialized successfully");
    } else {
        LOGE("Failed to initialize Notification Channel Manager");
    }

    // Initialize local notifications
    if (localManager_->initialize()) {
        localEnabled_ = true;
        LOGI("Local Notification Manager initialized successfully");
    } else {
        LOGE("Failed to initialize Local Notification Manager");
    }

    // Initialize scheduler
    if (scheduler_->initialize()) {
        LOGI("Notification Scheduler initialized successfully");
    } else {
        LOGE("Failed to initialize Notification Scheduler");
    }

    initialized_ = true;
    LOGI("Push Notification Manager initialized successfully");
}

void PushNotificationManager::update(float dt) {
    // Update statistics
    static float statsUpdateTimer = 0.0f;
    statsUpdateTimer += dt;

    if (statsUpdateTimer >= 60.0f) { // Update every 60 seconds
        saveStatsToStorage();
        statsUpdateTimer = 0.0f;
    }

    // Update sub-managers if needed
    if (firebaseManager_) {
        firebaseManager_->processMessageQueue();
    }

    if (scheduler_) {
        // Scheduler runs on its own thread
    }
}

void PushNotificationManager::shutdown() {
    LOGI("Shutting down Push Notification Manager");

    if (!initialized_) {
        return;
    }

    // Stop service threads
    stopServiceThreads();

    // Shutdown sub-managers
    if (firebaseManager_) {
        firebaseManager_->shutdown();
        delete firebaseManager_;
        firebaseManager_ = nullptr;
    }

    if (channelManager_) {
        channelManager_->shutdown();
        delete channelManager_;
        channelManager_ = nullptr;
    }

    if (localManager_) {
        localManager_->shutdown();
        delete localManager_;
        localManager_ = nullptr;
    }

    if (scheduler_) {
        scheduler_->shutdown();
        delete scheduler_;
        scheduler_ = nullptr;
    }

    // Clear callbacks
    receivedCallbacks_.clear();
    clickedCallbacks_.clear();
    dismissedCallbacks_.clear();
    tokenCallbacks_.clear();
    tokenErrorCallbacks_.clear();

    // Clear notifications
    std::lock_guard<std::mutex> lock(notificationsMutex_);
    activeNotifications_.clear();
    scheduledNotifications_.clear();

    initialized_ = false;
    LOGI("Push Notification Manager shutdown complete");
}

void PushNotificationManager::setJNIEnvironment(JNIEnv* env, jobject context) {
    env_ = env;
    context_ = context;
    LOGI("JNI environment set for Push Notification Manager");
}

bool PushNotificationManager::initializeFirebase(const std::string& senderId) {
    LOGI("Initializing Firebase with sender ID: %s", senderId.c_str());

    if (!firebaseManager_) {
        LOGE("Firebase Manager not available");
        return false;
    }

    senderId_ = senderId;

    if (firebaseManager_->initialize(senderId)) {
        firebaseEnabled_ = true;
        LOGI("Firebase initialized successfully");
        return true;
    } else {
        LOGE("Failed to initialize Firebase");
        return false;
    }
}

void PushNotificationManager::enableFirebase(bool enable) {
    firebaseEnabled_ = enable;
    LOGI("Firebase %s", enable ? "enabled" : "disabled");
}

void PushNotificationManager::refreshFirebaseToken() {
    LOGI("Refreshing Firebase token");

    if (firebaseManager_) {
        firebaseManager_->refreshToken();
    } else {
        LOGE("Firebase Manager not available");
    }
}

void PushNotificationManager::subscribeToTopic(const std::string& topic) {
    LOGI("Subscribing to topic: %s", topic.c_str());

    if (firebaseManager_) {
        firebaseManager_->subscribeToTopic(topic);
    } else {
        LOGE("Firebase Manager not available");
    }
}

void PushNotificationManager::unsubscribeFromTopic(const std::string& topic) {
    LOGI("Unsubscribing from topic: %s", topic.c_str());

    if (firebaseManager_) {
        firebaseManager_->unsubscribeFromTopic(topic);
    } else {
        LOGE("Firebase Manager not available");
    }
}

void PushNotificationManager::enableLocalNotifications(bool enable) {
    localEnabled_ = enable;
    LOGI("Local notifications %s", enable ? "enabled" : "disabled");
}

void PushNotificationManager::createNotificationChannel(const NotificationChannel& channel) {
    LOGI("Creating notification channel: %s", channel.id.c_str());

    if (channelManager_) {
        channelManager_->createChannel(channel);
    } else {
        LOGE("Notification Channel Manager not available");
    }
}

void PushNotificationManager::deleteNotificationChannel(const std::string& channelId) {
    LOGI("Deleting notification channel: %s", channelId.c_str());

    if (channelManager_) {
        channelManager_->deleteChannel(channelId);
    } else {
        LOGE("Notification Channel Manager not available");
    }
}

void PushNotificationManager::updateNotificationChannel(const NotificationChannel& channel) {
    LOGI("Updating notification channel: %s", channel.id.c_str());

    if (channelManager_) {
        channelManager_->updateChannel(channel);
    } else {
        LOGE("Notification Channel Manager not available");
    }
}

std::vector<NotificationChannel> PushNotificationManager::getNotificationChannels() const {
    if (channelManager_) {
        return channelManager_->getAllChannels();
    }
    return std::vector<NotificationChannel>();
}

NotificationChannel PushNotificationManager::getNotificationChannel(const std::string& channelId) const {
    if (channelManager_) {
        return channelManager_->getChannel(channelId);
    }
    return NotificationChannel();
}

void PushNotificationManager::sendPushNotification(const PushMessage& message) {
    LOGI("Sending push notification: %s", message.title.c_str());

    if (!isValidPushMessage(message)) {
        LOGE("Invalid push message");
        return;
    }

    // Update statistics
    incrementStat("totalSent", message.category == NotificationCategory::CUSTOM ?
                 "custom" : std::to_string(static_cast<int>(message.category)));

    // Send via Firebase if enabled
    if (firebaseEnabled_ && firebaseManager_) {
        firebaseManager_->sendMessage(message);
    }

    // Send locally if enabled
    if (localEnabled_ && localManager_) {
        localManager_->sendNotification(message);
    }

    // Store in active notifications
    std::lock_guard<std::mutex> lock(notificationsMutex_);
    activeNotifications_[message.messageId] = message;

    LOGI("Push notification sent: %s", message.messageId.c_str());
}

void PushNotificationManager::sendPushNotification(const std::string& title, const std::string& body,
                                                  const std::string& channelId) {
    PushMessage message;
    message.title = title;
    message.body = body;
    message.channelId = channelId;
    message.messageId = generateNotificationId();
    message.timestamp = time(nullptr);
    message.priority = NotificationPriority::DEFAULT;
    message.category = NotificationCategory::CUSTOM;

    sendPushNotification(message);
}

void PushNotificationManager::sendScheduledNotification(const ScheduledNotification& notification) {
    LOGI("Scheduling notification: %s", notification.id.c_str());

    if (!isValidScheduledNotification(notification)) {
        LOGE("Invalid scheduled notification");
        return;
    }

    if (scheduler_) {
        scheduler_->scheduleNotification(notification);

        // Store in scheduled notifications
        std::lock_guard<std::mutex> lock(notificationsMutex_);
        scheduledNotifications_[notification.id] = notification;

        LOGI("Notification scheduled: %s", notification.id.c_str());
    } else {
        LOGE("Notification Scheduler not available");
    }
}

void PushNotificationManager::cancelNotification(const std::string& notificationId) {
    LOGI("Canceling notification: %s", notificationId.c_str());

    if (localManager_) {
        localManager_->cancelNotification(notificationId);
    }

    if (scheduler_) {
        scheduler_->cancelScheduledNotification(notificationId);
    }

    // Remove from active notifications
    std::lock_guard<std::mutex> lock(notificationsMutex_);
    activeNotifications_.erase(notificationId);
    scheduledNotifications_.erase(notificationId);

    LOGI("Notification canceled: %s", notificationId.c_str());
}

void PushNotificationManager::cancelAllNotifications() {
    LOGI("Canceling all notifications");

    if (localManager_) {
        localManager_->cancelAllNotifications();
    }

    if (scheduler_) {
        scheduler_->cancelAllScheduledNotifications();
    }

    // Clear all notifications
    std::lock_guard<std::mutex> lock(notificationsMutex_);
    activeNotifications_.clear();
    scheduledNotifications_.clear();

    LOGI("All notifications canceled");
}

void PushNotificationManager::cancelNotificationsByTag(const std::string& tag) {
    LOGI("Canceling notifications by tag: %s", tag.c_str());

    std::lock_guard<std::mutex> lock(notificationsMutex_);

    // Cancel active notifications with matching tag
    for (auto it = activeNotifications_.begin(); it != activeNotifications_.end();) {
        if (it->second.tag == tag) {
            if (localManager_) {
                localManager_->cancelNotification(it->second.messageId);
            }
            it = activeNotifications_.erase(it);
        } else {
            ++it;
        }
    }

    // Cancel scheduled notifications with matching tag
    for (auto it = scheduledNotifications_.begin(); it != scheduledNotifications_.end();) {
        if (it->second.message.tag == tag) {
            if (scheduler_) {
                scheduler_->cancelScheduledNotification(it->second.id);
            }
            it = scheduledNotifications_.erase(it);
        } else {
            ++it;
        }
    }

    LOGI("Notifications canceled by tag: %s", tag.c_str());
}

void PushNotificationManager::cancelNotificationsByGroup(const std::string& group) {
    LOGI("Canceling notifications by group: %s", group.c_str());

    std::lock_guard<std::mutex> lock(notificationsMutex_);

    // Cancel active notifications with matching group
    for (auto it = activeNotifications_.begin(); it != activeNotifications_.end();) {
        if (it->second.group == group) {
            if (localManager_) {
                localManager_->cancelNotification(it->second.messageId);
            }
            it = activeNotifications_.erase(it);
        } else {
            ++it;
        }
    }

    // Cancel scheduled notifications with matching group
    for (auto it = scheduledNotifications_.begin(); it != scheduledNotifications_.end();) {
        if (it->second.message.group == group) {
            if (scheduler_) {
                scheduler_->cancelScheduledNotification(it->second.id);
            }
            it = scheduledNotifications_.erase(it);
        } else {
            ++it;
        }
    }

    LOGI("Notifications canceled by group: %s", group.c_str());
}

void PushNotificationManager::setBadgeCount(int count) {
    defaultBadgeCount_ = std::max(0, count);
    LOGI("Badge count set to: %d", defaultBadgeCount_);
}

void PushNotificationManager::clearBadgeCount() {
    defaultBadgeCount_ = 0;
    LOGI("Badge count cleared");
}

void PushNotificationManager::enableBadge(bool enable) {
    enableBadge_ = enable;
    LOGI("Badge %s", enable ? "enabled" : "disabled");
}

void PushNotificationManager::setSoundEnabled(bool enable) {
    enableSound_ = enable;
    if (localManager_) {
        localManager_->setSoundEnabled(enable);
    }
    LOGI("Sound %s", enable ? "enabled" : "disabled");
}

void PushNotificationManager::setVibrationEnabled(bool enable) {
    enableVibration_ = enable;
    if (localManager_) {
        localManager_->setVibrationEnabled(enable);
    }
    LOGI("Vibration %s", enable ? "enabled" : "disabled");
}

void PushNotificationManager::setLightsEnabled(bool enable) {
    enableLights_ = enable;
    if (localManager_) {
        localManager_->setLightsEnabled(enable);
    }
    LOGI("Lights %s", enable ? "enabled" : "disabled");
}

void PushNotificationManager::setCustomSound(const std::string& soundResource) {
    if (localManager_) {
        localManager_->setCustomSound(soundResource);
    }
    LOGI("Custom sound set to: %s", soundResource.c_str());
}

void PushNotificationManager::setVibrationPattern(const std::vector<int>& pattern) {
    if (localManager_) {
        localManager_->setVibrationPattern(pattern);
    }
    LOGI("Vibration pattern updated");
}

void PushNotificationManager::setLightPattern(NotificationLightPattern pattern, int color) {
    if (localManager_) {
        localManager_->setLightPattern(pattern, color);
    }
    LOGI("Light pattern updated");
}

void PushNotificationManager::enableAnalytics(bool enable) {
    enableAnalytics_ = enable;
    LOGI("Analytics %s", enable ? "enabled" : "disabled");
}

NotificationStats PushNotificationManager::getNotificationStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void PushNotificationManager::resetNotificationStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = NotificationStats();
    stats_.lastUpdated = time(nullptr);
    LOGI("Notification stats reset");
}

void PushNotificationManager::updateNotificationStats(const std::string& event, const std::string& category) {
    if (!enableAnalytics_) return;

    incrementStat(event, category);
    LOGI("Notification stats updated: %s (%s)", event.c_str(), category.c_str());
}

void PushNotificationManager::registerNotificationReceivedCallback(const std::string& id, NotificationReceivedCallback callback) {
    receivedCallbacks_[id] = callback;
    LOGI("Notification received callback registered: %s", id.c_str());
}

void PushNotificationManager::unregisterNotificationReceivedCallback(const std::string& id) {
    receivedCallbacks_.erase(id);
    LOGI("Notification received callback unregistered: %s", id.c_str());
}

void PushNotificationManager::registerNotificationClickedCallback(const std::string& id, NotificationClickedCallback callback) {
    clickedCallbacks_[id] = callback;
    LOGI("Notification clicked callback registered: %s", id.c_str());
}

void PushNotificationManager::unregisterNotificationClickedCallback(const std::string& id) {
    clickedCallbacks_.erase(id);
    LOGI("Notification clicked callback unregistered: %s", id.c_str());
}

void PushNotificationManager::registerNotificationDismissedCallback(const std::string& id, NotificationDismissedCallback callback) {
    dismissedCallbacks_[id] = callback;
    LOGI("Notification dismissed callback registered: %s", id.c_str());
}

void PushNotificationManager::unregisterNotificationDismissedCallback(const std::string& id) {
    dismissedCallbacks_.erase(id);
    LOGI("Notification dismissed callback unregistered: %s", id.c_str());
}

void PushNotificationManager::registerTokenReceivedCallback(const std::string& id, TokenReceivedCallback callback) {
    tokenCallbacks_[id] = callback;
    LOGI("Token received callback registered: %s", id.c_str());
}

void PushNotificationManager::unregisterTokenReceivedCallback(const std::string& id) {
    tokenCallbacks_.erase(id);
    LOGI("Token received callback unregistered: %s", id.c_str());
}

void PushNotificationManager::registerTokenErrorCallback(const std::string& id, TokenErrorCallback callback) {
    tokenErrorCallbacks_[id] = callback;
    LOGI("Token error callback registered: %s", id.c_str());
}

void PushNotificationManager::unregisterTokenErrorCallback(const std::string& id) {
    tokenErrorCallbacks_.erase(id);
    LOGI("Token error callback unregistered: %s", id.c_str());
}

void PushNotificationManager::showInAppNotification(const std::string& title, const std::string& message,
                                                   float duration, const std::string& position) {
    LOGI("Showing in-app notification: %s", title.c_str());

    // In a real implementation, this would show an in-app notification
    // For now, just log the action
    LOGI("In-app notification: %s - %s (duration: %.1f, position: %s)",
         title.c_str(), message.c_str(), duration, position.c_str());
}

void PushNotificationManager::dismissInAppNotification() {
    LOGI("Dismissing in-app notification");

    // In a real implementation, this would dismiss the in-app notification
    LOGI("In-app notification dismissed");
}

void PushNotificationManager::testNotification(const PushMessage& message) {
    LOGI("Testing notification: %s", message.title.c_str());

    if (localManager_) {
        localManager_->sendNotification(message);
    }
}

void PushNotificationManager::testLocalNotification() {
    LOGI("Testing local notification");

    PushMessage message;
    message.title = "Test Notification";
    message.body = "This is a test notification from FoundryEngine";
    message.channelId = "default";
    message.messageId = generateNotificationId();
    message.timestamp = time(nullptr);
    message.priority = NotificationPriority::HIGH;
    message.category = NotificationCategory::SYSTEM;

    testNotification(message);
}

void PushNotificationManager::testScheduledNotification() {
    LOGI("Testing scheduled notification");

    PushMessage message;
    message.title = "Scheduled Test";
    message.body = "This is a scheduled test notification";
    message.channelId = "default";
    message.messageId = generateNotificationId();
    message.timestamp = time(nullptr);
    message.priority = NotificationPriority::DEFAULT;
    message.category = NotificationCategory::SYSTEM;

    ScheduledNotification scheduled;
    scheduled.id = generateNotificationId();
    scheduled.message = message;
    scheduled.scheduledTime = std::chrono::system_clock::now() + std::chrono::seconds(10);
    scheduled.repeatInterval = std::chrono::system_clock::now() + std::chrono::seconds(60);
    scheduled.repeatCount = 0;
    scheduled.repeatForever = false;
    scheduled.allowWhileIdle = true;

    sendScheduledNotification(scheduled);
}

bool PushNotificationManager::areNotificationsEnabled() const {
    return localEnabled_ || firebaseEnabled_;
}

bool PushNotificationManager::isFirebaseAvailable() const {
    return firebaseManager_ && firebaseManager_->isInitialized();
}

std::string PushNotificationManager::getNotificationStatus() const {
    std::stringstream status;
    status << "Push Notifications: " << (areNotificationsEnabled() ? "ENABLED" : "DISABLED") << "\n";
    status << "Firebase: " << (firebaseEnabled_ ? "ENABLED" : "DISABLED") << "\n";
    status << "Local: " << (localEnabled_ ? "ENABLED" : "DISABLED") << "\n";
    status << "Badge: " << (enableBadge_ ? "ENABLED" : "DISABLED") << " (" << defaultBadgeCount_ << ")\n";
    status << "Sound: " << (enableSound_ ? "ENABLED" : "DISABLED") << "\n";
    status << "Vibration: " << (enableVibration_ ? "ENABLED" : "DISABLED") << "\n";
    status << "Lights: " << (enableLights_ ? "ENABLED" : "DISABLED") << "\n";
    status << "Analytics: " << (enableAnalytics_ ? "ENABLED" : "DISABLED") << "\n";
    return status.str();
}

void PushNotificationManager::requestNotificationPermission() {
    LOGI("Requesting notification permission");

    // In a real implementation, this would request notification permission
    // For now, just log the action
    LOGI("Notification permission requested");
}

void PushNotificationManager::initializeDefaults() {
    LOGI("Initializing notification defaults");

    // Initialize default statistics
    stats_.totalSent = 0;
    stats_.totalDelivered = 0;
    stats_.totalOpened = 0;
    stats_.totalDismissed = 0;
    stats_.totalFailed = 0;
    stats_.lastUpdated = time(nullptr);

    // Load statistics from storage
    loadStatsFromStorage();

    LOGI("Notification defaults initialized");
}

void PushNotificationManager::startServiceThreads() {
    LOGI("Starting notification service threads");

    serviceRunning_ = true;
    serviceThread_ = std::thread(&PushNotificationManager::serviceThreadLoop, this);

    // Start statistics thread if analytics enabled
    if (enableAnalytics_) {
        statsThread_ = std::thread(&PushNotificationManager::statsThreadLoop, this);
    }

    LOGI("Notification service threads started");
}

void PushNotificationManager::stopServiceThreads() {
    LOGI("Stopping notification service threads");

    serviceRunning_ = false;
    if (serviceThread_.joinable()) {
        serviceThread_.join();
    }

    if (statsThread_.joinable()) {
        statsThread_.join();
    }

    LOGI("Notification service threads stopped");
}

void PushNotificationManager::serviceThreadLoop() {
    LOGI("Notification service thread started");

    while (serviceRunning_) {
        // Process any pending operations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOGI("Notification service thread ended");
}

void PushNotificationManager::statsThreadLoop() {
    LOGI("Notification stats thread started");

    while (serviceRunning_) {
        // Update statistics periodically
        saveStatsToStorage();
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }

    LOGI("Notification stats thread ended");
}

void PushNotificationManager::onPushMessageReceived(const PushMessage& message) {
    LOGI("Push message received: %s", message.title.c_str());

    // Update statistics
    incrementStat("totalDelivered", message.category == NotificationCategory::CUSTOM ?
                 "custom" : std::to_string(static_cast<int>(message.category)));

    // Call registered callbacks
    for (const auto& pair : receivedCallbacks_) {
        pair.second(message);
    }
}

void PushNotificationManager::onNotificationClicked(const NotificationResponse& response) {
    LOGI("Notification clicked: %s", response.notificationId.c_str());

    // Update statistics
    incrementStat("totalOpened");

    // Call registered callbacks
    for (const auto& pair : clickedCallbacks_) {
        pair.second(response);
    }
}

void PushNotificationManager::onNotificationDismissed(const std::string& notificationId) {
    LOGI("Notification dismissed: %s", notificationId.c_str());

    // Update statistics
    incrementStat("totalDismissed");

    // Call registered callbacks
    for (const auto& pair : dismissedCallbacks_) {
        pair.second(notificationId);
    }
}

void PushNotificationManager::onFirebaseTokenReceived(const std::string& token) {
    LOGI("Firebase token received");

    firebaseToken_ = token;

    // Call registered callbacks
    for (const auto& pair : tokenCallbacks_) {
        pair.second(token);
    }
}

void PushNotificationManager::onFirebaseTokenError(const std::string& error) {
    LOGE("Firebase token error: %s", error.c_str());

    // Call registered callbacks
    for (const auto& pair : tokenErrorCallbacks_) {
        pair.second(error);
    }
}

void PushNotificationManager::incrementStat(const std::string& statName, const std::string& category) {
    std::lock_guard<std::mutex> lock(statsMutex_);

    if (statName == "totalSent") {
        stats_.totalSent++;
    } else if (statName == "totalDelivered") {
        stats_.totalDelivered++;
    } else if (statName == "totalOpened") {
        stats_.totalOpened++;
    } else if (statName == "totalDismissed") {
        stats_.totalDismissed++;
    } else if (statName == "totalFailed") {
        stats_.totalFailed++;
    }

    if (!category.empty()) {
        stats_.categoryStats[category]++;
    }

    stats_.lastUpdated = time(nullptr);
}

void PushNotificationManager::saveStatsToStorage() {
    // In a real implementation, this would save statistics to persistent storage
    LOGI("Saving notification stats to storage");
}

void PushNotificationManager::loadStatsFromStorage() {
    // In a real implementation, this would load statistics from persistent storage
    LOGI("Loading notification stats from storage");
}

std::string PushNotificationManager::generateNotificationId() {
    static std::atomic<int> counter(0);
    return "notification_" + std::to_string(time(nullptr)) + "_" + std::to_string(counter++);
}

std::string PushNotificationManager::generateChannelId() {
    static std::atomic<int> counter(0);
    return "channel_" + std::to_string(counter++);
}

bool PushNotificationManager::isValidNotificationChannel(const NotificationChannel& channel) {
    return !channel.id.empty() && !channel.name.empty() && !channel.description.empty();
}

bool PushNotificationManager::isValidPushMessage(const PushMessage& message) {
    return !message.title.empty() && !message.body.empty() && !message.messageId.empty();
}

bool PushNotificationManager::isValidScheduledNotification(const ScheduledNotification& notification) {
    return !notification.id.empty() && isValidPushMessage(notification.message);
}

// ========== FIREBASE MANAGER IMPLEMENTATION ==========
FirebaseManager::FirebaseManager(PushNotificationManager* manager) : manager_(manager), initialized_(false),
                                                                    connected_(false) {
    LOGI("FirebaseManager constructor called");
}

FirebaseManager::~FirebaseManager() {
    shutdown();
    LOGI("FirebaseManager destructor called");
}

bool FirebaseManager::initialize(const std::string& senderId) {
    LOGI("Initializing Firebase Manager with sender ID: %s", senderId.c_str());

    senderId_ = senderId;

    // In a real implementation, this would initialize Firebase SDK
    // For now, simulate initialization
    initialized_ = true;
    connected_ = true;

    // Simulate token generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream tokenStream;
    for (int i = 0; i < 163; i++) {
        tokenStream << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
        if (i < 162) tokenStream << (i % 2 == 0 ? ":" : "");
    }

    token_ = tokenStream.str();

    // Notify manager of token
    manager_->onFirebaseTokenReceived(token_);

    LOGI("Firebase Manager initialized successfully");
    return true;
}

void FirebaseManager::shutdown() {
    LOGI("Shutting down Firebase Manager");

    disconnect();
    initialized_ = false;
}

void FirebaseManager::setProjectId(const std::string& projectId) {
    projectId_ = projectId;
    LOGI("Firebase project ID set to: %s", projectId_.c_str());
}

void FirebaseManager::setApiKey(const std::string& apiKey) {
    apiKey_ = apiKey;
    LOGI("Firebase API key set");
}

void FirebaseManager::setAppId(const std::string& appId) {
    appId_ = appId;
    LOGI("Firebase app ID set to: %s", appId_.c_str());
}

void FirebaseManager::connect() {
    LOGI("Connecting to Firebase");

    if (!initialized_) {
        LOGE("Firebase not initialized");
        return;
    }

    connected_ = true;
    LOGI("Firebase connected");
}

void FirebaseManager::disconnect() {
    LOGI("Disconnecting from Firebase");

    connected_ = false;

    // Clear message queue
    std::lock_guard<std::mutex> lock(messageMutex_);
    while (!messageQueue_.empty()) {
        messageQueue_.pop();
    }

    LOGI("Firebase disconnected");
}

void FirebaseManager::subscribeToTopic(const std::string& topic) {
    LOGI("Subscribing to Firebase topic: %s", topic.c_str());

    std::lock_guard<std::mutex> lock(topicsMutex_);
    subscribedTopics_.push_back(topic);

    LOGI("Subscribed to topic: %s", topic.c_str());
}

void FirebaseManager::unsubscribeFromTopic(const std::string& topic) {
    LOGI("Unsubscribing from Firebase topic: %s", topic.c_str());

    std::lock_guard<std::mutex> lock(topicsMutex_);
    subscribedTopics_.erase(
        std::remove(subscribedTopics_.begin(), subscribedTopics_.end(), topic),
        subscribedTopics_.end());

    LOGI("Unsubscribed from topic: %s", topic.c_str());
}

std::vector<std::string> FirebaseManager::getSubscribedTopics() const {
    std::lock_guard<std::mutex> lock(topicsMutex_);
    return subscribedTopics_;
}

void FirebaseManager::sendMessage(const PushMessage& message) {
    LOGI("Sending Firebase message: %s", message.title.c_str());

    // In a real implementation, this would send the message via Firebase
    // For now, simulate sending
    std::lock_guard<std::mutex> lock(messageMutex_);
    messageQueue_.push(message);

    LOGI("Firebase message queued: %s", message.messageId.c_str());
}

void FirebaseManager::processMessageQueue() {
    std::lock_guard<std::mutex> lock(messageMutex_);

    while (!messageQueue_.empty()) {
        PushMessage message = messageQueue_.front();
        messageQueue_.pop();

        // Simulate message processing
        onMessageReceived(message);
    }
}

void FirebaseManager::refreshToken() {
    LOGI("Refreshing Firebase token");

    // In a real implementation, this would refresh the token
    // For now, simulate token refresh
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream tokenStream;
    for (int i = 0; i < 163; i++) {
        tokenStream << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
        if (i < 162) tokenStream << (i % 2 == 0 ? ":" : "");
    }

    token_ = tokenStream.str();
    manager_->onFirebaseTokenReceived(token_);

    LOGI("Firebase token refreshed");
}

void FirebaseManager::invalidateToken() {
    LOGI("Invalidating Firebase token");

    token_.clear();
    manager_->onFirebaseTokenError("Token invalidated");

    LOGI("Firebase token invalidated");
}

void FirebaseManager::onTokenReceived(const std::string& token) {
    token_ = token;
    manager_->onFirebaseTokenReceived(token);
}

void FirebaseManager::onTokenError(const std::string& error) {
    manager_->onFirebaseTokenError(error);
}

void FirebaseManager::onMessageReceived(const PushMessage& message) {
    manager_->onPushMessageReceived(message);
}

void FirebaseManager::processFirebaseMessage(const std::string& jsonMessage) {
    try {
        PushMessage message = parseFirebaseMessage(jsonMessage);
        onMessageReceived(message);
    } catch (const std::exception& e) {
        LOGE("Failed to process Firebase message: %s", e.what());
    }
}

PushMessage FirebaseManager::parseFirebaseMessage(const std::string& json) {
    // In a real implementation, this would parse the Firebase message JSON
    // For now, return a mock message
    PushMessage message;
    message.title = "Firebase Message";
    message.body = "Message received from Firebase";
    message.messageId = generateMessageId();
    message.timestamp = time(nullptr);
    message.priority = NotificationPriority::DEFAULT;
    message.category = NotificationCategory::CUSTOM;

    return message;
}

bool FirebaseManager::validateFirebaseConfig() {
    return !projectId_.empty() && !apiKey_.empty() && !appId_.empty();
}

std::string FirebaseManager::generateMessageId() {
    static std::atomic<int> counter(0);
    return "firebase_msg_" + std::to_string(time(nullptr)) + "_" + std::to_string(counter++);
}

// ========== NOTIFICATION CHANNEL MANAGER IMPLEMENTATION ==========
NotificationChannelManager::NotificationChannelManager(PushNotificationManager* manager) : manager_(manager) {
    LOGI("NotificationChannelManager constructor called");
}

NotificationChannelManager::~NotificationChannelManager() {
    shutdown();
    LOGI("NotificationChannelManager destructor called");
}

bool NotificationChannelManager::initialize() {
    LOGI("Initializing Notification Channel Manager");

    createDefaultChannels();
    return true;
}

void NotificationChannelManager::shutdown() {
    LOGI("Shutting down Notification Channel Manager");

    std::lock_guard<std::mutex> lock(channelsMutex_);
    channels_.clear();
    defaultChannels_.clear();
}

void NotificationChannelManager::createChannel(const NotificationChannel& channel) {
    LOGI("Creating notification channel: %s", channel.id.c_str());

    if (!isValidChannel(channel)) {
        LOGE("Invalid notification channel");
        return;
    }

    std::lock_guard<std::mutex> lock(channelsMutex_);
    channels_[channel.id] = channel;

    LOGI("Notification channel created: %s", channel.id.c_str());
}

void NotificationChannelManager::deleteChannel(const std::string& channelId) {
    LOGI("Deleting notification channel: %s", channelId.c_str());

    std::lock_guard<std::mutex> lock(channelsMutex_);

    if (isDefaultChannel(channelId)) {
        LOGW("Cannot delete default channel: %s", channelId.c_str());
        return;
    }

    channels_.erase(channelId);

    LOGI("Notification channel deleted: %s", channelId.c_str());
}

void NotificationChannelManager::updateChannel(const NotificationChannel& channel) {
    LOGI("Updating notification channel: %s", channel.id.c_str());

    if (!isValidChannel(channel)) {
        LOGE("Invalid notification channel");
        return;
    }

    std::lock_guard<std::mutex> lock(channelsMutex_);

    if (channels_.find(channel.id) == channels_.end()) {
        LOGE("Channel not found: %s", channel.id.c_str());
        return;
    }

    channels_[channel.id] = channel;

    LOGI("Notification channel updated: %s", channel.id.c_str());
}

NotificationChannel NotificationChannelManager::getChannel(const std::string& channelId) const {
    std::lock_guard<std::mutex> lock(channelsMutex_);

    auto it = channels_.find(channelId);
    if (it != channels_.end()) {
        return it->second;
    }

    return NotificationChannel();
}

std::vector<NotificationChannel> NotificationChannelManager::getAllChannels() const {
    std::lock_guard<std::mutex> lock(channelsMutex_);

    std::vector<NotificationChannel> result;
    for (const auto& pair : channels_) {
        result.push_back(pair.second);
    }

    return result;
}

std::vector<NotificationChannel> NotificationChannelManager::getChannelsByImportance(NotificationChannelImportance importance) const {
    std::lock_guard<std::mutex> lock(channelsMutex_);

    std::vector<NotificationChannel> result;
    for (const auto& pair : channels_) {
        if (pair.second.importance == importance) {
            result.push_back(pair.second);
        }
    }

    return result;
}

void NotificationChannelManager::createDefaultChannels() {
    LOGI("Creating default notification channels");

    defaultChannels_.push_back(createGameNotificationChannel());
    defaultChannels_.push_back(createSocialNotificationChannel());
    defaultChannels_.push_back(createAchievementNotificationChannel());
    defaultChannels_.push_back(createSystemNotificationChannel());

    for (const auto& channel : defaultChannels_) {
        createChannel(channel);
    }

    LOGI("Default notification channels created: %zu channels", defaultChannels_.size());
}

void NotificationChannelManager::resetToDefaultChannels() {
    LOGI("Resetting to default notification channels");

    std::lock_guard<std::mutex> lock(channelsMutex_);

    // Remove non-default channels
    for (auto it = channels_.begin(); it != channels_.end();) {
        if (!isDefaultChannel(it->first)) {
            it = channels_.erase(it);
        } else {
            ++it;
        }
    }

    // Add default channels
    for (const auto& channel : defaultChannels_) {
        channels_[channel.id] = channel;
    }

    LOGI("Reset to default channels completed");
}

bool NotificationChannelManager::isDefaultChannel(const std::string& channelId) const {
    for (const auto& channel : defaultChannels_) {
        if (channel.id == channelId) {
            return true;
        }
    }
    return false;
}

bool NotificationChannelManager::isValidChannel(const NotificationChannel& channel) {
    return !channel.id.empty() && !channel.name.empty() && !channel.description.empty();
}

bool NotificationChannelManager::channelExists(const std::string& channelId) const {
    std::lock_guard<std::mutex> lock(channelsMutex_);
    return channels_.find(channelId) != channels_.end();
}

std::string NotificationChannelManager::generateChannelId(const std::string& baseName) {
    static std::atomic<int> counter(0);
    return baseName + "_" + std::to_string(counter++);
}

NotificationChannel NotificationChannelManager::createGameNotificationChannel() {
    NotificationChannel channel;
    channel.id = "game_updates";
    channel.name = "Game Updates";
    channel.description = "Updates about game content and features";
    channel.importance = NotificationChannelImportance::DEFAULT;
    channel.showBadge = true;
    channel.soundType = NotificationSoundType::DEFAULT;
    channel.lightPattern = NotificationLightPattern::NONE;
    channel.canBypassDnd = false;
    channel.canShowOnLockedScreen = true;
    channel.allowedCategories = {"GAME_UPDATE"};

    return channel;
}

NotificationChannel NotificationChannelManager::createSocialNotificationChannel() {
    NotificationChannel channel;
    channel.id = "social";
    channel.name = "Social";
    channel.description = "Friend requests, messages, and social interactions";
    channel.importance = NotificationChannelImportance::HIGH;
    channel.showBadge = true;
    channel.soundType = NotificationSoundType::DEFAULT;
    channel.lightPattern = NotificationLightPattern::SLOW_BLINK;
    channel.lightColor = 0xFF0000FF; // Blue
    channel.canBypassDnd = true;
    channel.canShowOnLockedScreen = true;
    channel.allowedCategories = {"SOCIAL"};

    return channel;
}

NotificationChannel NotificationChannelManager::createAchievementNotificationChannel() {
    NotificationChannel channel;
    channel.id = "achievements";
    channel.name = "Achievements";
    channel.description = "Achievement unlocks and progress";
    channel.importance = NotificationChannelImportance::HIGH;
    channel.showBadge = true;
    channel.soundType = NotificationSoundType::DEFAULT;
    channel.lightPattern = NotificationLightPattern::FAST_BLINK;
    channel.lightColor = 0xFFFFA500; // Orange
    channel.canBypassDnd = true;
    channel.canShowOnLockedScreen = true;
    channel.allowedCategories = {"ACHIEVEMENT"};

    return channel;
}

NotificationChannel NotificationChannelManager::createSystemNotificationChannel() {
    NotificationChannel channel;
    channel.id = "system";
    channel.name = "System";
    channel.description = "System messages and maintenance notifications";
    channel.importance = NotificationChannelImportance::LOW;
    channel.showBadge = false;
    channel.soundType = NotificationSoundType::SILENT;
    channel.lightPattern = NotificationLightPattern::NONE;
    channel.canBypassDnd = false;
    channel.canShowOnLockedScreen = false;
    channel.allowedCategories = {"SYSTEM"};

    return channel;
}

// ========== LOCAL NOTIFICATION MANAGER IMPLEMENTATION ==========
LocalNotificationManager::LocalNotificationManager(PushNotificationManager* manager) : manager_(manager),
                                                                                       soundEnabled_(true),
                                                                                       vibrationEnabled_(true),
                                                                                       lightsEnabled_(true) {
    LOGI("LocalNotificationManager constructor called");
}

LocalNotificationManager::~LocalNotificationManager() {
    shutdown();
    LOGI("LocalNotificationManager destructor called");
}

bool LocalNotificationManager::initialize() {
    LOGI("Initializing Local Notification Manager");

    // In a real implementation, this would initialize local notification system
    // For now, just log success
    return true;
}

void LocalNotificationManager::shutdown() {
    LOGI("Shutting down Local Notification Manager");

    cancelAllNotifications();
}

void LocalNotificationManager::sendNotification(const PushMessage& message) {
    LOGI("Sending local notification: %s", message.title.c_str());

    if (!validateNotification(message)) {
        LOGE("Invalid notification");
        return;
    }

    // Apply notification settings
    PushMessage processedMessage = message;
    applyNotificationSettings(processedMessage);

    // Store in active notifications
    std::lock_guard<std::mutex> lock(notificationsMutex_);
    activeNotifications_[message.messageId] = processedMessage;

    // Save to history
    saveNotificationToHistory(processedMessage);

    LOGI("Local notification sent: %s", message.messageId.c_str());
}

void LocalNotificationManager::sendNotification(const std::string& title, const std::string& body,
                                               const std::string& channelId) {
    PushMessage message;
    message.title = title;
    message.body = body;
    message.channelId = channelId;
    message.messageId = generateNotificationId();
    message.timestamp = time(nullptr);
    message.priority = NotificationPriority::DEFAULT;
    message.category = NotificationCategory::CUSTOM;

    sendNotification(message);
}

void LocalNotificationManager::sendNotificationWithActions(const PushMessage& message,
                                                          const std::vector<NotificationAction>& actions) {
    LOGI("Sending local notification with actions: %s", message.title.c_str());

    // In a real implementation, this would send notification with actions
    // For now, just send the basic notification
    sendNotification(message);

    LOGI("Local notification with actions sent: %s", message.messageId.c_str());
}

void LocalNotificationManager::cancelNotification(const std::string& notificationId) {
    LOGI("Canceling local notification: %s", notificationId.c_str());

    std::lock_guard<std::mutex> lock(notificationsMutex_);
    activeNotifications_.erase(notificationId);

    LOGI("Local notification canceled: %s", notificationId.c_str());
}

void LocalNotificationManager::cancelAllNotifications() {
    LOGI("Canceling all local notifications");

    std::lock_guard<std::mutex> lock(notificationsMutex_);
    activeNotifications_.clear();

    LOGI("All local notifications canceled");
}

std::vector<PushMessage> LocalNotificationManager::getActiveNotifications() const {
    std::lock_guard<std::mutex> lock(notificationsMutex_);

    std::vector<PushMessage> result;
    for (const auto& pair : activeNotifications_) {
        result.push_back(pair.second);
    }

    return result;
}

bool LocalNotificationManager::isNotificationActive(const std::string& notificationId) const {
    std::lock_guard<std::mutex> lock(notificationsMutex_);
    return activeNotifications_.find(notificationId) != activeNotifications_.end();
}

void LocalNotificationManager::applyNotificationSettings(PushMessage& message) {
    // Apply sound setting
    if (!soundEnabled_) {
        message.sound = "silent";
    } else if (customSound_.empty()) {
        message.sound = "default";
    } else {
        message.sound = customSound_;
    }

    // Apply vibration pattern
    if (!vibrationEnabled_) {
        message.data["vibration"] = "false";
    } else {
        message.data["vibration_pattern"] = "default";
    }

    // Apply light settings
    if (lightsEnabled_) {
        message.data["lights"] = "true";
        message.data["light_color"] = std::to_string(lightColor_);
    } else {
        message.data["lights"] = "false";
    }
}

std::string LocalNotificationManager::generateNotificationId() {
    static std::atomic<int> counter(0);
    return "local_notification_" + std::to_string(time(nullptr)) + "_" + std::to_string(counter++);
}

bool LocalNotificationManager::validateNotification(const PushMessage& message) {
    return !message.title.empty() && !message.body.empty() && !message.messageId.empty();
}

void LocalNotificationManager::saveNotificationToHistory(const PushMessage& message) {
    // In a real implementation, this would save to notification history
    LOGI("Notification saved to history: %s", message.messageId.c_str());
}

// ========== NOTIFICATION SCHEDULER IMPLEMENTATION ==========
NotificationScheduler::NotificationScheduler(PushNotificationManager* manager) : manager_(manager),
                                                                                 schedulerRunning_(false),
                                                                                 allowWhileIdle_(true),
                                                                                 maxScheduledNotifications_(100) {
    LOGI("NotificationScheduler constructor called");
}

NotificationScheduler::~NotificationScheduler() {
    shutdown();
    LOGI("NotificationScheduler destructor called");
}

bool NotificationScheduler::initialize() {
    LOGI("Initializing Notification Scheduler");

    startSchedulerThread();
    return true;
}

void NotificationScheduler::shutdown() {
    LOGI("Shutting down Notification Scheduler");

    stopSchedulerThread();
    cancelAllScheduledNotifications();
}

void NotificationScheduler::scheduleNotification(const ScheduledNotification& notification) {
    LOGI("Scheduling notification: %s", notification.id.c_str());

    if (!validateScheduledNotification(notification)) {
        LOGE("Invalid scheduled notification");
        return;
    }

    std::lock_guard<std::mutex> lock(scheduledMutex_);
    scheduledNotifications_[notification.id] = notification;

    // Notify scheduler thread
    schedulerCondition_.notify_one();

    LOGI("Notification scheduled: %s", notification.id.c_str());
}

void NotificationScheduler::cancelScheduledNotification(const std::string& notificationId) {
    LOGI("Canceling scheduled notification: %s", notificationId.c_str());

    std::lock_guard<std::mutex> lock(scheduledMutex_);
    scheduledNotifications_.erase(notificationId);

    LOGI("Scheduled notification canceled: %s", notificationId.c_str());
}

void NotificationScheduler::cancelAllScheduledNotifications() {
    LOGI("Canceling all scheduled notifications");

    std::lock_guard<std::mutex> lock(scheduledMutex_);
    scheduledNotifications_.clear();

    LOGI("All scheduled notifications canceled");
}

std::vector<ScheduledNotification> NotificationScheduler::getScheduledNotifications() const {
    std::lock_guard<std::mutex> lock(scheduledMutex_);

    std::vector<ScheduledNotification> result;
    for (const auto& pair : scheduledNotifications_) {
        result.push_back(pair.second);
    }

    return result;
}

std::vector<ScheduledNotification> NotificationScheduler::getScheduledNotificationsInTimeRange(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const {
    std::lock_guard<std::mutex> lock(scheduledMutex_);

    std::vector<ScheduledNotification> result;
    for (const auto& pair : scheduledNotifications_) {
        if (pair.second.scheduledTime >= start && pair.second.scheduledTime <= end) {
            result.push_back(pair.second);
        }
    }

    return result;
}

void NotificationScheduler::scheduleRepeatingNotification(const std::string& id, const PushMessage& message,
                                                        std::chrono::seconds interval, int repeatCount) {
    LOGI("Scheduling repeating notification: %s", id.c_str());

    ScheduledNotification notification;
    notification.id = id;
    notification.message = message;
    notification.scheduledTime = std::chrono::system_clock::now() + interval;
    notification.repeatInterval = std::chrono::system_clock::now() + interval;
    notification.repeatCount = repeatCount;
    notification.repeatForever = (repeatCount < 0);
    notification.allowWhileIdle = allowWhileIdle_;

    scheduleNotification(notification);
}

void NotificationScheduler::scheduleDailyNotification(const std::string& id, const PushMessage& message, int hour, int minute) {
    LOGI("Scheduling daily notification: %s", id.c_str());

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    tm* tm_now = localtime(&time_t_now);

    tm scheduledTime = *tm_now;
    scheduledTime.tm_hour = hour;
    scheduledTime.tm_min = minute;
    scheduledTime.tm_sec = 0;

    // If the scheduled time has already passed today, schedule for tomorrow
    if (mktime(&scheduledTime) <= time_t_now) {
        scheduledTime.tm_mday += 1;
    }

    ScheduledNotification notification;
    notification.id = id;
    notification.message = message;
    notification.scheduledTime = std::chrono::system_clock::from_time_t(mktime(&scheduledTime));
    notification.repeatInterval = std::chrono::hours(24);
    notification.repeatCount = -1; // Repeat forever
    notification.repeatForever = true;
    notification.allowWhileIdle = allowWhileIdle_;

    scheduleNotification(notification);
}

void NotificationScheduler::scheduleWeeklyNotification(const std::string& id, const PushMessage& message,
                                                     int dayOfWeek, int hour, int minute) {
    LOGI("Scheduling weekly notification: %s", id.c_str());

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    tm* tm_now = localtime(&time_t_now);

    tm scheduledTime = *tm_now;
    scheduledTime.tm_wday = dayOfWeek;
    scheduledTime.tm_hour = hour;
    scheduledTime.tm_min = minute;
    scheduledTime.tm_sec = 0;

    // If the scheduled time has already passed this week, schedule for next week
    if (mktime(&scheduledTime) <= time_t_now) {
        scheduledTime.tm_mday += 7;
    }

    ScheduledNotification notification;
    notification.id = id;
    notification.message = message;
    notification.scheduledTime = std::chrono::system_clock::from_time_t(mktime(&scheduledTime));
    notification.repeatInterval = std::chrono::hours(24 * 7);
    notification.repeatCount = -1; // Repeat forever
    notification.repeatForever = true;
    notification.allowWhileIdle = allowWhileIdle_;

    scheduleNotification(notification);
}

void NotificationScheduler::startSchedulerThread() {
    LOGI("Starting notification scheduler thread");

    schedulerRunning_ = true;
    schedulerThread_ = std::thread(&NotificationScheduler::schedulerThreadLoop, this);

    LOGI("Notification scheduler thread started");
}

void NotificationScheduler::stopSchedulerThread() {
    LOGI("Stopping notification scheduler thread");

    schedulerRunning_ = false;
    schedulerCondition_.notify_all();

    if (schedulerThread_.joinable()) {
        schedulerThread_.join();
    }

    LOGI("Notification scheduler thread stopped");
}

void NotificationScheduler::schedulerThreadLoop() {
    LOGI("Notification scheduler thread started");

    while (schedulerRunning_) {
        processScheduledNotifications();

        // Sleep for 30 seconds
        std::unique_lock<std::mutex> lock(scheduledMutex_);
        schedulerCondition_.wait_for(lock, std::chrono::seconds(30));
    }

    LOGI("Notification scheduler thread ended");
}

void NotificationScheduler::processScheduledNotifications() {
    auto now = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock(scheduledMutex_);

    for (auto it = scheduledNotifications_.begin(); it != scheduledNotifications_.end();) {
        if (isTimeToTrigger(it->second)) {
            triggerNotification(it->second);

            // Handle repeating notifications
            if (it->second.repeatForever || it->second.repeatCount > 0) {
                ScheduledNotification next = it->second;
                next.scheduledTime = it->second.scheduledTime + std::chrono::duration_cast<std::chrono::system_clock::duration>(it->second.repeatInterval);

                if (!next.repeatForever) {
                    next.repeatCount--;
                }

                it->second = next;
                ++it;
            } else {
                it = scheduledNotifications_.erase(it);
            }
        } else {
            ++it;
        }
    }
}

void NotificationScheduler::checkAndTriggerNotifications() {
    // This method is called periodically to check for due notifications
    processScheduledNotifications();
}

bool NotificationScheduler::isTimeToTrigger(const ScheduledNotification& notification) const {
    return std::chrono::system_clock::now() >= notification.scheduledTime;
}

void NotificationScheduler::triggerNotification(const ScheduledNotification& notification) {
    LOGI("Triggering scheduled notification: %s", notification.id.c_str());

    // Send the notification
    if (manager_) {
        manager_->sendPushNotification(notification.message);
    }
}

std::string NotificationScheduler::generateScheduleId() {
    static std::atomic<int> counter(0);
    return "scheduled_" + std::to_string(time(nullptr)) + "_" + std::to_string(counter++);
}

bool NotificationScheduler::validateScheduledNotification(const ScheduledNotification& notification) {
    return !notification.id.empty() && !notification.message.title.empty() &&
           !notification.message.body.empty() && !notification.message.messageId.empty();
}

} // namespace FoundryEngine
