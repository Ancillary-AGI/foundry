/**
 * @file AdvancedNetworkSystem.h
 * @brief Enterprise-grade networking system with ultra-low latency multiplayer
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include <memory>
#include <vector>
#include <functional>
#include <chrono>

namespace FoundryEngine {

/**
 * @class AdvancedNetworkSystem
 * @brief High-performance networking with prediction and anti-cheat
 */
class AdvancedNetworkSystem : public System {
public:
    enum class NetworkMode {
        Client,
        Server,
        Host,
        Peer2Peer
    };

    enum class Protocol {
        TCP,
        UDP,
        WebRTC,
        QUIC,
        Custom
    };

    struct NetworkConfig {
        NetworkMode mode = NetworkMode::Client;
        Protocol protocol = Protocol::UDP;
        uint16_t port = 7777;
        std::string serverAddress = "127.0.0.1";
        uint32_t maxConnections = 100;
        uint32_t tickRate = 60;
        bool enablePrediction = true;
        bool enableCompression = true;
        bool enableEncryption = true;
        bool enableAntiCheat = true;
        float timeoutSeconds = 30.0f;
    };

    struct ConnectionInfo {
        uint32_t connectionId;
        std::string address;
        uint16_t port;
        float ping;
        float packetLoss;
        uint64_t bytesReceived;
        uint64_t bytesSent;
        std::chrono::steady_clock::time_point connectedTime;
    };

    struct NetworkStats {
        uint32_t activeConnections;
        float averagePing;
        float packetLossRate;
        uint64_t totalBytesReceived;
        uint64_t totalBytesSent;
        uint32_t messagesPerSecond;
        float compressionRatio;
    };

    AdvancedNetworkSystem();
    ~AdvancedNetworkSystem();

    bool initialize(const NetworkConfig& config = NetworkConfig{}) override;
    void shutdown() override;
    void update(float deltaTime) override;

    // Connection management
    bool startServer(uint16_t port);
    bool connectToServer(const std::string& address, uint16_t port);
    void disconnect(uint32_t connectionId = 0);
    void disconnectAll();
    bool isConnected(uint32_t connectionId = 0) const;
    std::vector<ConnectionInfo> getConnections() const;

    // Message handling
    void sendMessage(const std::vector<uint8_t>& data, uint32_t connectionId = 0, 
                    bool reliable = true, uint8_t channel = 0);
    void broadcastMessage(const std::vector<uint8_t>& data, bool reliable = true, 
                         uint8_t channel = 0);
    void setMessageHandler(std::function<void(uint32_t, const std::vector<uint8_t>&)> handler);

    // RPC system
    template<typename... Args>
    void registerRPC(const std::string& name, std::function<void(uint32_t, Args...)> func);
    
    template<typename... Args>
    void callRPC(const std::string& name, uint32_t connectionId, Args... args);
    
    template<typename... Args>
    void broadcastRPC(const std::string& name, Args... args);

    // State synchronization
    void registerSyncVar(const std::string& name, void* variable, size_t size, 
                        float syncRate = 20.0f);
    void unregisterSyncVar(const std::string& name);
    void setSyncVarOwner(const std::string& name, uint32_t connectionId);

    // Client-side prediction
    void enablePrediction(bool enable);
    void setRollbackBuffer(uint32_t frames);
    void confirmServerState(uint32_t frame, const std::vector<uint8_t>& state);
    void rollbackToFrame(uint32_t frame);

    // Anti-cheat
    void enableAntiCheat(bool enable);
    void setCheatDetectionCallback(std::function<void(uint32_t, const std::string&)> callback);
    void reportSuspiciousActivity(uint32_t connectionId, const std::string& reason);

    // Performance monitoring
    NetworkStats getNetworkStats() const;
    float getPing(uint32_t connectionId = 0) const;
    float getPacketLoss(uint32_t connectionId = 0) const;

    // Event callbacks
    void setConnectionCallback(std::function<void(uint32_t)> callback);
    void setDisconnectionCallback(std::function<void(uint32_t)> callback);
    void setErrorCallback(std::function<void(const std::string&)> callback);

private:
    class AdvancedNetworkSystemImpl;
    std::unique_ptr<AdvancedNetworkSystemImpl> impl_;
};

/**
 * @class NetworkPrediction
 * @brief Client-side prediction and lag compensation system
 */
class NetworkPrediction {
public:
    struct PredictionConfig {
        uint32_t maxRollbackFrames = 60;
        float interpolationTime = 0.1f;
        float extrapolationTime = 0.05f;
        bool enableSmoothing = true;
        float smoothingFactor = 0.1f;
    };

    NetworkPrediction();
    ~NetworkPrediction();

    void initialize(const PredictionConfig& config);
    void shutdown();

    // State management
    void saveState(uint32_t frame, const std::vector<uint8_t>& state);
    std::vector<uint8_t> getState(uint32_t frame) const;
    void rollbackToFrame(uint32_t frame);
    void confirmState(uint32_t frame, const std::vector<uint8_t>& authoritative);

    // Input prediction
    void predictInput(uint32_t frame, const std::vector<uint8_t>& input);
    void confirmInput(uint32_t frame, const std::vector<uint8_t>& authoritative);
    std::vector<uint8_t> getInputForFrame(uint32_t frame) const;

    // Interpolation and extrapolation
    std::vector<uint8_t> interpolateStates(const std::vector<uint8_t>& from, 
                                          const std::vector<uint8_t>& to, float t);
    std::vector<uint8_t> extrapolateState(const std::vector<uint8_t>& state, 
                                         const std::vector<uint8_t>& velocity, float deltaTime);

    // Lag compensation
    void setPlayerLatency(uint32_t playerId, float latency);
    uint32_t getCompensatedFrame(uint32_t playerId, uint32_t currentFrame) const;
    std::vector<uint8_t> getCompensatedState(uint32_t playerId, uint32_t frame) const;

private:
    class NetworkPredictionImpl;
    std::unique_ptr<NetworkPredictionImpl> impl_;
};

/**
 * @class AntiCheatSystem
 * @brief Server-side anti-cheat and validation system
 */
class AntiCheatSystem {
public:
    enum class CheatType {
        SpeedHack,
        Teleport,
        Aimbot,
        Wallhack,
        ResourceHack,
        PacketManipulation,
        TimingAnomaly
    };

    struct CheatDetection {
        uint32_t playerId;
        CheatType type;
        float confidence;
        std::string details;
        std::chrono::steady_clock::time_point timestamp;
    };

    AntiCheatSystem();
    ~AntiCheatSystem();

    void initialize();
    void shutdown();
    void update(float deltaTime);

    // Player monitoring
    void registerPlayer(uint32_t playerId);
    void unregisterPlayer(uint32_t playerId);
    void updatePlayerState(uint32_t playerId, const std::vector<uint8_t>& state);
    void updatePlayerInput(uint32_t playerId, const std::vector<uint8_t>& input);

    // Validation rules
    void setMaxSpeed(float maxSpeed);
    void setMaxAcceleration(float maxAcceleration);
    void setValidBounds(const Vector3& min, const Vector3& max);
    void setInputValidation(std::function<bool(const std::vector<uint8_t>&)> validator);

    // Detection callbacks
    void setCheatDetectedCallback(std::function<void(const CheatDetection&)> callback);
    void setSuspiciousActivityCallback(std::function<void(uint32_t, const std::string&)> callback);

    // Statistics
    std::vector<CheatDetection> getRecentDetections() const;
    uint32_t getDetectionCount(CheatType type) const;
    float getFalsePositiveRate() const;

private:
    class AntiCheatSystemImpl;
    std::unique_ptr<AntiCheatSystemImpl> impl_;
};

} // namespace FoundryEngine