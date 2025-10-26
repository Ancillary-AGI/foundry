/**
 * @file CloudSystem.h
 * @brief Comprehensive cloud integration system for modern game development
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

namespace FoundryEngine {

/**
 * @class CloudSystem
 * @brief Universal cloud services integration for analytics, builds, and deployment
 */
class CloudSystem : public System {
public:
    enum class CloudProvider {
        AWS,
        Azure,
        GoogleCloud,
        Custom
    };

    struct CloudConfig {
        CloudProvider provider = CloudProvider::AWS;
        std::string apiKey;
        std::string secretKey;
        std::string region = "us-east-1";
        std::string projectId;
        bool enableAnalytics = true;
        bool enableCrashReporting = true;
        bool enableRemoteConfig = true;
        bool enableCloudSave = true;
    };

    struct BuildConfig {
        std::string projectPath;
        std::vector<std::string> targetPlatforms;
        std::string buildConfiguration = "Release";
        bool enableOptimization = true;
        bool enableCompression = true;
        std::unordered_map<std::string, std::string> customParameters;
    };

    struct DeploymentConfig {
        std::string appName;
        std::string version;
        std::vector<std::string> targetStores;
        bool autoPublish = false;
        std::string releaseNotes;
        std::unordered_map<std::string, std::string> storeMetadata;
    };

    CloudSystem();
    ~CloudSystem();

    bool initialize(const CloudConfig& config = CloudConfig{}) override;
    void shutdown() override;
    void update(float deltaTime) override;

    // Analytics
    void trackEvent(const std::string& eventName, 
                   const std::unordered_map<std::string, std::string>& parameters = {});
    void trackUserProperty(const std::string& property, const std::string& value);
    void trackRevenue(float amount, const std::string& currency = "USD", 
                     const std::string& productId = "");
    void setUserId(const std::string& userId);
    void startSession();
    void endSession();

    // Crash reporting
    void reportCrash(const std::string& crashInfo, const std::string& stackTrace = "");
    void reportError(const std::string& error, const std::string& context = "");
    void setBreadcrumb(const std::string& message, const std::string& category = "");
    void setCustomData(const std::string& key, const std::string& value);

    // Remote configuration
    void fetchRemoteConfig();
    std::string getRemoteConfigValue(const std::string& key, const std::string& defaultValue = "");
    bool getRemoteConfigBool(const std::string& key, bool defaultValue = false);
    float getRemoteConfigFloat(const std::string& key, float defaultValue = 0.0f);
    void setRemoteConfigCallback(std::function<void(bool)> callback);

    // Cloud save
    void saveToCloud(const std::string& key, const std::vector<uint8_t>& data);
    std::vector<uint8_t> loadFromCloud(const std::string& key);
    void deleteFromCloud(const std::string& key);
    std::vector<std::string> listCloudSaves();
    void syncCloudSaves();

    // Cloud builds
    std::string startCloudBuild(const BuildConfig& config);
    std::string getBuildStatus(const std::string& buildId);
    std::vector<uint8_t> downloadBuild(const std::string& buildId);
    void cancelBuild(const std::string& buildId);
    std::vector<std::string> listBuilds();

    // Deployment
    std::string deployToStore(const DeploymentConfig& config);
    std::string getDeploymentStatus(const std::string& deploymentId);
    void updateStoreMetadata(const std::string& appId, 
                           const std::unordered_map<std::string, std::string>& metadata);

    // User management
    std::string authenticateUser(const std::string& email, const std::string& password);
    std::string createUser(const std::string& email, const std::string& password);
    void signOutUser();
    bool isUserSignedIn();
    std::string getCurrentUserId();

    // Leaderboards
    void submitScore(const std::string& leaderboardId, float score, 
                    const std::unordered_map<std::string, std::string>& metadata = {});
    std::vector<std::unordered_map<std::string, std::string>> getLeaderboard(
        const std::string& leaderboardId, uint32_t count = 10, uint32_t offset = 0);
    uint32_t getUserRank(const std::string& leaderboardId, const std::string& userId = "");

    // Achievements
    void unlockAchievement(const std::string& achievementId);
    void incrementAchievement(const std::string& achievementId, uint32_t steps = 1);
    std::vector<std::unordered_map<std::string, std::string>> getAchievements();
    bool isAchievementUnlocked(const std::string& achievementId);

private:
    class CloudSystemImpl;
    std::unique_ptr<CloudSystemImpl> impl_;
};

/**
 * @class AssetStreaming
 * @brief CDN-based asset streaming and caching system
 */
class AssetStreaming {
public:
    struct StreamingConfig {
        std::string cdnUrl;
        std::string cacheDirectory = "./cache";
        size_t maxCacheSize = 1024 * 1024 * 1024; // 1GB
        uint32_t maxConcurrentDownloads = 4;
        bool enableCompression = true;
        bool enableDeltaUpdates = true;
    };

    struct AssetInfo {
        std::string assetId;
        std::string url;
        size_t size;
        std::string hash;
        std::string version;
        bool isDownloaded;
        float downloadProgress;
    };

    AssetStreaming();
    ~AssetStreaming();

    void initialize(const StreamingConfig& config);
    void shutdown();
    void update(float deltaTime);

    // Asset management
    void requestAsset(const std::string& assetId, uint32_t priority = 0);
    void cancelAssetRequest(const std::string& assetId);
    bool isAssetAvailable(const std::string& assetId);
    std::string getAssetPath(const std::string& assetId);
    AssetInfo getAssetInfo(const std::string& assetId);

    // Preloading
    void preloadAssets(const std::vector<std::string>& assetIds);
    void preloadAssetsInRadius(const Vector3& position, float radius);
    void setPreloadCallback(std::function<void(const std::string&, bool)> callback);

    // Cache management
    void clearCache();
    void clearAsset(const std::string& assetId);
    size_t getCacheSize();
    void setCacheLimit(size_t limit);
    std::vector<std::string> getCachedAssets();

    // Progress tracking
    float getOverallProgress();
    void setProgressCallback(std::function<void(const std::string&, float)> callback);

private:
    class AssetStreamingImpl;
    std::unique_ptr<AssetStreamingImpl> impl_;
};

/**
 * @class MultiplayerBackend
 * @brief Managed multiplayer services with matchmaking and lobbies
 */
class MultiplayerBackend {
public:
    struct MatchmakingConfig {
        std::string gameMode;
        uint32_t minPlayers = 2;
        uint32_t maxPlayers = 8;
        std::string region = "auto";
        std::unordered_map<std::string, std::string> customProperties;
        float skillRange = 0.2f;
        uint32_t timeoutSeconds = 30;
    };

    struct LobbyInfo {
        std::string lobbyId;
        std::string hostId;
        std::vector<std::string> playerIds;
        std::string gameMode;
        std::unordered_map<std::string, std::string> properties;
        bool isPrivate;
        uint32_t maxPlayers;
    };

    MultiplayerBackend();
    ~MultiplayerBackend();

    void initialize();
    void shutdown();
    void update(float deltaTime);

    // Matchmaking
    void startMatchmaking(const MatchmakingConfig& config);
    void cancelMatchmaking();
    bool isMatchmaking();
    void setMatchFoundCallback(std::function<void(const std::string&)> callback);

    // Lobby management
    std::string createLobby(const std::string& gameMode, uint32_t maxPlayers, bool isPrivate = false);
    void joinLobby(const std::string& lobbyId);
    void leaveLobby();
    std::string getCurrentLobbyId();
    LobbyInfo getLobbyInfo(const std::string& lobbyId = "");
    std::vector<LobbyInfo> findLobbies(const std::string& gameMode = "");

    // Lobby communication
    void sendLobbyMessage(const std::string& message);
    void setLobbyMessageCallback(std::function<void(const std::string&, const std::string&)> callback);
    void setLobbyProperty(const std::string& key, const std::string& value);
    std::string getLobbyProperty(const std::string& key);

    // Game sessions
    void startGameSession();
    void endGameSession();
    void reportGameResult(const std::unordered_map<std::string, float>& playerScores);

private:
    class MultiplayerBackendImpl;
    std::unique_ptr<MultiplayerBackendImpl> impl_;
};

} // namespace FoundryEngine