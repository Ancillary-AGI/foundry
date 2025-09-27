#ifndef FOUNDRYENGINE_PLATFORM_SERVICES_H
#define FOUNDRYENGINE_PLATFORM_SERVICES_H

#include "../../core/System.h"
#include <jni.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace FoundryEngine {

// Forward declarations
class PlatformServices;
class BillingManager;
class GamesServices;
class CloudSaveManager;

// Callback types for platform services
using PurchaseCallback = std::function<void(bool success, const std::string& productId, const std::string& token)>;
using AchievementCallback = std::function<void(bool success, const std::string& achievementId)>;
using LeaderboardCallback = std::function<void(bool success, const std::string& leaderboardId)>;
using CloudSaveCallback = std::function<void(bool success, const std::string& data)>;
using AuthCallback = std::function<void(bool success, const std::string& playerId)>;

// Product types for in-app purchases
enum class ProductType {
    CONSUMABLE,      // Used once (coins, gems, etc.)
    NON_CONSUMABLE,  // Permanent (unlock features, characters)
    SUBSCRIPTION     // Recurring subscription
};

// Purchase states
enum class PurchaseState {
    PENDING,         // Purchase initiated but not completed
    COMPLETED,       // Purchase completed successfully
    CANCELLED,       // Purchase cancelled by user
    FAILED          // Purchase failed
};

// Achievement types
enum class AchievementType {
    STANDARD,        // Regular achievement
    INCREMENTAL,     // Achievement with progress steps
    HIDDEN          // Achievement not visible until unlocked
};

// Leaderboard time frames
enum class LeaderboardTimeFrame {
    DAILY,           // Daily leaderboard
    WEEKLY,          // Weekly leaderboard
    ALL_TIME         // All-time leaderboard
};

// Leaderboard collection types
enum class LeaderboardCollection {
    PUBLIC,          // Public leaderboard
    SOCIAL,          // Friends-only leaderboard
    PRIVATE          // Private leaderboard
};

// ========== PLATFORM SERVICES MANAGER ==========
class PlatformServices : public System {
private:
    static PlatformServices* instance_;

    BillingManager* billingManager_;
    GamesServices* gamesServices_;
    CloudSaveManager* cloudSaveManager_;

    bool initialized_;
    bool authenticated_;

    // JNI environment
    JNIEnv* env_;
    jobject activity_;

public:
    PlatformServices();
    ~PlatformServices();

    static PlatformServices* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // Authentication
    void authenticateUser(AuthCallback callback);
    bool isAuthenticated() const { return authenticated_; }
    std::string getCurrentPlayerId() const;

    // Billing/In-App Purchases
    void initializeBilling();
    void purchaseProduct(const std::string& productId, ProductType type, PurchaseCallback callback);
    void consumePurchase(const std::string& purchaseToken);
    bool isBillingSupported() const;

    // Achievements
    void unlockAchievement(const std::string& achievementId, AchievementCallback callback);
    void incrementAchievement(const std::string& achievementId, int steps, AchievementCallback callback);
    void showAchievementsUI();

    // Leaderboards
    void submitScore(const std::string& leaderboardId, int score, LeaderboardCallback callback);
    void showLeaderboard(const std::string& leaderboardId);
    void showAllLeaderboards();

    // Cloud Save
    void saveGameData(const std::string& key, const std::string& data, CloudSaveCallback callback);
    void loadGameData(const std::string& key, CloudSaveCallback callback);
    void deleteGameData(const std::string& key, CloudSaveCallback callback);

    // Utility functions
    void setJNIEnvironment(JNIEnv* env, jobject activity);
    bool isConnected() const;

private:
    void onAuthenticationComplete(bool success, const std::string& playerId);
    void onPurchaseComplete(bool success, const std::string& productId, const std::string& token);
    void onAchievementUnlock(bool success, const std::string& achievementId);
    void onScoreSubmitted(bool success, const std::string& leaderboardId);
    void onCloudSaveComplete(bool success, const std::string& data);
};

// ========== BILLING MANAGER ==========
class BillingManager {
private:
    PlatformServices* services_;
    bool initialized_;

    // Product catalog
    struct Product {
        std::string productId;
        std::string title;
        std::string description;
        std::string price;
        ProductType type;
        bool available;
    };

    std::unordered_map<std::string, Product> products_;
    std::vector<PurchaseCallback> purchaseCallbacks_;

public:
    BillingManager(PlatformServices* services);
    ~BillingManager();

    bool initialize();
    void shutdown();

    // Product management
    void queryProducts(const std::vector<std::string>& productIds);
    void queryPurchases();
    bool isProductAvailable(const std::string& productId) const;
    const Product* getProduct(const std::string& productId) const;

    // Purchase handling
    void initiatePurchase(const std::string& productId, ProductType type);
    void processPurchaseResult(bool success, const std::string& productId, const std::string& token);
    void consumeProduct(const std::string& purchaseToken);

    // JNI callbacks
    void onProductsQueried(const std::vector<Product>& products);
    void onPurchaseResult(const std::string& productId, PurchaseState state, const std::string& token);
    void onPurchasesQueried(const std::vector<std::string>& purchasedProductIds);

private:
    void setupBillingClient();
    void connectToBillingService();
    void disconnectBillingService();
};

// ========== GAMES SERVICES ==========
class GamesServices {
private:
    PlatformServices* services_;
    bool initialized_;

    // Achievement data
    struct Achievement {
        std::string achievementId;
        std::string name;
        std::string description;
        AchievementType type;
        int totalSteps;
        int currentSteps;
        bool unlocked;
        std::string unlockedTime;
    };

    // Leaderboard data
    struct Leaderboard {
        std::string leaderboardId;
        std::string name;
        std::string description;
        LeaderboardCollection collection;
        std::vector<LeaderboardTimeFrame> timeFrames;
    };

    std::unordered_map<std::string, Achievement> achievements_;
    std::unordered_map<std::string, Leaderboard> leaderboards_;
    std::vector<AchievementCallback> achievementCallbacks_;
    std::vector<LeaderboardCallback> leaderboardCallbacks_;

public:
    GamesServices(PlatformServices* services);
    ~GamesServices();

    bool initialize();
    void shutdown();

    // Achievement management
    void loadAchievements();
    void unlockAchievement(const std::string& achievementId);
    void incrementAchievement(const std::string& achievementId, int steps);
    void revealAchievement(const std::string& achievementId);
    bool isAchievementUnlocked(const std::string& achievementId) const;
    int getAchievementProgress(const std::string& achievementId) const;

    // Leaderboard management
    void loadLeaderboards();
    void submitScore(const std::string& leaderboardId, int score);
    void showAchievementsUI();
    void showLeaderboard(const std::string& leaderboardId);
    void showAllLeaderboards();

    // Player data
    void loadPlayerStats();
    void incrementPlayerStat(const std::string& statId, int value);

    // JNI callbacks
    void onAchievementsLoaded(const std::vector<Achievement>& achievements);
    void onAchievementUnlocked(const std::string& achievementId, bool success);
    void onScoreSubmitted(const std::string& leaderboardId, bool success);
    void onLeaderboardsLoaded(const std::vector<Leaderboard>& leaderboards);

private:
    void connectToGamesServices();
    void disconnectGamesServices();
    void setupAchievementsClient();
    void setupLeaderboardsClient();
};

// ========== CLOUD SAVE MANAGER ==========
class CloudSaveManager {
private:
    PlatformServices* services_;
    bool initialized_;

    struct SaveData {
        std::string key;
        std::string data;
        std::string lastModified;
        int version;
    };

    std::unordered_map<std::string, SaveData> saveData_;
    std::vector<CloudSaveCallback> saveCallbacks_;
    std::vector<CloudSaveCallback> loadCallbacks_;

public:
    CloudSaveManager(PlatformServices* services);
    ~CloudSaveManager();

    bool initialize();
    void shutdown();

    // Cloud save operations
    void saveData(const std::string& key, const std::string& data);
    void loadData(const std::string& key);
    void deleteData(const std::string& key);
    void syncAllData();

    // Conflict resolution
    void resolveConflict(const std::string& key, const std::string& localData, const std::string& remoteData);
    void chooseLocalData(const std::string& key);
    void chooseRemoteData(const std::string& key);

    // Data management
    const SaveData* getSaveData(const std::string& key) const;
    bool hasUnsyncedChanges() const;
    void markAsSynced(const std::string& key);

    // JNI callbacks
    void onSaveComplete(const std::string& key, bool success);
    void onLoadComplete(const std::string& key, const std::string& data, bool success);
    void onDeleteComplete(const std::string& key, bool success);
    void onConflictDetected(const std::string& key, const std::string& localData, const std::string& remoteData);

private:
    void connectToCloudSave();
    void disconnectCloudSave();
    void setupSnapshotClient();
    void loadAllSnapshots();
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // Authentication
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onAuthenticationResult(
        JNIEnv* env, jobject thiz, jboolean success, jstring playerId);

    // Billing
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onBillingSetupFinished(
        JNIEnv* env, jobject thiz, jboolean success);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onProductsQueried(
        JNIEnv* env, jobject thiz, jobjectArray products);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onPurchaseResult(
        JNIEnv* env, jobject thiz, jstring productId, jint state, jstring token);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onPurchasesQueried(
        JNIEnv* env, jobject thiz, jobjectArray purchases);

    // Achievements
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onAchievementsLoaded(
        JNIEnv* env, jobject thiz, jobjectArray achievements);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onAchievementUnlocked(
        JNIEnv* env, jobject thiz, jstring achievementId, jboolean success);

    // Leaderboards
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onLeaderboardsLoaded(
        JNIEnv* env, jobject thiz, jobjectArray leaderboards);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onScoreSubmitted(
        JNIEnv* env, jobject thiz, jstring leaderboardId, jboolean success);

    // Cloud Save
    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onCloudSaveComplete(
        JNIEnv* env, jobject thiz, jstring key, jboolean success, jstring data);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onCloudLoadComplete(
        JNIEnv* env, jobject thiz, jstring key, jboolean success, jstring data);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_PlatformServices_onCloudConflict(
        JNIEnv* env, jobject thiz, jstring key, jstring localData, jstring remoteData);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_PLATFORM_SERVICES_H
