#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace FoundryEngine {

enum class NetworkProtocol {
    TCP,
    UDP,
    WebSocket,
    WebRTC
};

enum class NetworkRole {
    Client,
    Server,
    Host,
    Peer
};

struct NetworkMessage {
    uint32_t id;
    uint32_t type;
    std::vector<uint8_t> data;
    uint64_t timestamp;
    bool reliable;
    uint32_t channelId;
};

struct NetworkPeer {
    uint32_t id;
    std::string address;
    uint16_t port;
    bool connected;
    float ping;
    uint64_t lastSeen;
    std::unordered_map<std::string, std::string> metadata;
};

class NetworkConnection {
public:
    virtual ~NetworkConnection() = default;
    virtual bool connect(const std::string& address, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool send(const NetworkMessage& message) = 0;
    virtual std::vector<NetworkMessage> receive() = 0;
    virtual float getPing() const = 0;
    virtual std::string getRemoteAddress() const = 0;
    virtual uint16_t getRemotePort() const = 0;
};

class NetworkServer {
public:
    virtual ~NetworkServer() = default;
    virtual bool start(uint16_t port, int maxClients = 32) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual std::vector<NetworkPeer> getConnectedPeers() const = 0;
    virtual bool sendToPeer(uint32_t peerId, const NetworkMessage& message) = 0;
    virtual bool sendToAll(const NetworkMessage& message, uint32_t excludePeerId = 0) = 0;
    virtual void disconnectPeer(uint32_t peerId) = 0;
    virtual std::vector<NetworkMessage> receiveMessages() = 0;
    virtual void setMaxClients(int maxClients) = 0;
    virtual int getMaxClients() const = 0;
    virtual int getConnectedClientCount() const = 0;
};

class NetworkClient {
public:
    virtual ~NetworkClient() = default;
    virtual bool connect(const std::string& address, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool send(const NetworkMessage& message) = 0;
    virtual std::vector<NetworkMessage> receive() = 0;
    virtual float getPing() const = 0;
    virtual NetworkPeer getServerInfo() const = 0;
};

class NetworkManager {
public:
    virtual ~NetworkManager() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;
    
    // Role management
    virtual void setRole(NetworkRole role) = 0;
    virtual NetworkRole getRole() const = 0;
    
    // Server functionality
    virtual bool startServer(uint16_t port, int maxClients = 32) = 0;
    virtual void stopServer() = 0;
    virtual bool isServerRunning() const = 0;
    
    // Client functionality
    virtual bool connectToServer(const std::string& address, uint16_t port) = 0;
    virtual void disconnectFromServer() = 0;
    virtual bool isConnectedToServer() const = 0;
    
    // Peer-to-peer functionality
    virtual bool createRoom(const std::string& roomName, int maxPeers = 8) = 0;
    virtual bool joinRoom(const std::string& roomName) = 0;
    virtual void leaveRoom() = 0;
    virtual bool isInRoom() const = 0;
    virtual std::string getCurrentRoom() const = 0;
    
    // Message handling
    virtual bool sendMessage(const NetworkMessage& message) = 0;
    virtual bool sendMessageToPeer(uint32_t peerId, const NetworkMessage& message) = 0;
    virtual bool sendMessageToAll(const NetworkMessage& message, uint32_t excludePeerId = 0) = 0;
    virtual std::vector<NetworkMessage> receiveMessages() = 0;
    
    // Peer management
    virtual std::vector<NetworkPeer> getConnectedPeers() const = 0;
    virtual NetworkPeer getPeer(uint32_t peerId) const = 0;
    virtual uint32_t getLocalPeerId() const = 0;
    virtual void disconnectPeer(uint32_t peerId) = 0;
    
    // Network statistics
    virtual float getPing(uint32_t peerId = 0) const = 0;
    virtual uint64_t getBytesSent() const = 0;
    virtual uint64_t getBytesReceived() const = 0;
    virtual uint32_t getPacketsSent() const = 0;
    virtual uint32_t getPacketsReceived() const = 0;
    virtual uint32_t getPacketsLost() const = 0;
    virtual float getPacketLossRate() const = 0;
    
    // Message serialization
    virtual NetworkMessage createMessage(uint32_t type, const std::vector<uint8_t>& data, bool reliable = true, uint32_t channelId = 0) = 0;
    virtual void registerMessageHandler(uint32_t messageType, std::function<void(const NetworkMessage&, uint32_t)> handler) = 0;
    virtual void unregisterMessageHandler(uint32_t messageType) = 0;
    
    // Network discovery
    virtual void startDiscovery() = 0;
    virtual void stopDiscovery() = 0;
    virtual std::vector<NetworkPeer> getDiscoveredServers() const = 0;
    virtual void broadcastPresence(const std::unordered_map<std::string, std::string>& metadata = {}) = 0;
    
    // Network configuration
    virtual void setProtocol(NetworkProtocol protocol) = 0;
    virtual NetworkProtocol getProtocol() const = 0;
    virtual void setTimeout(uint32_t timeoutMs) = 0;
    virtual uint32_t getTimeout() const = 0;
    virtual void setMaxRetries(uint32_t maxRetries) = 0;
    virtual uint32_t getMaxRetries() const = 0;
    
    // Callbacks
    virtual void setOnPeerConnectedCallback(std::function<void(const NetworkPeer&)> callback) = 0;
    virtual void setOnPeerDisconnectedCallback(std::function<void(uint32_t)> callback) = 0;
    virtual void setOnMessageReceivedCallback(std::function<void(const NetworkMessage&, uint32_t)> callback) = 0;
    virtual void setOnServerStartedCallback(std::function<void()> callback) = 0;
    virtual void setOnServerStoppedCallback(std::function<void()> callback) = 0;
    virtual void setOnConnectedToServerCallback(std::function<void()> callback) = 0;
    virtual void setOnDisconnectedFromServerCallback(std::function<void()> callback) = 0;
    
    // Network synchronization
    virtual void enableNetworkSync(bool enable) = 0;
    virtual bool isNetworkSyncEnabled() const = 0;
    virtual void syncObject(uint32_t objectId, const std::vector<uint8_t>& data) = 0;
    virtual void unsyncObject(uint32_t objectId) = 0;
    virtual std::vector<uint32_t> getSyncedObjects() const = 0;
    
    // Network interpolation and prediction
    virtual void enableInterpolation(bool enable) = 0;
    virtual bool isInterpolationEnabled() const = 0;
    virtual void setInterpolationDelay(float delayMs) = 0;
    virtual float getInterpolationDelay() const = 0;
    virtual void enablePrediction(bool enable) = 0;
    virtual bool isPredictionEnabled() const = 0;
    
    // Network compression
    virtual void enableCompression(bool enable) = 0;
    virtual bool isCompressionEnabled() const = 0;
    virtual void setCompressionLevel(int level) = 0;
    virtual int getCompressionLevel() const = 0;
    
    // Network encryption
    virtual void enableEncryption(bool enable) = 0;
    virtual bool isEncryptionEnabled() const = 0;
    virtual void setEncryptionKey(const std::string& key) = 0;
    
    // Bandwidth management
    virtual void setBandwidthLimit(uint32_t bytesPerSecond) = 0;
    virtual uint32_t getBandwidthLimit() const = 0;
    virtual uint32_t getCurrentBandwidthUsage() const = 0;
    
    // Network debugging
    virtual void enableNetworkDebug(bool enable) = 0;
    virtual bool isNetworkDebugEnabled() const = 0;
    virtual void simulateLatency(uint32_t latencyMs) = 0;
    virtual void simulatePacketLoss(float lossRate) = 0;
    virtual void simulateJitter(uint32_t jitterMs) = 0;
};

// Platform-specific implementations
class UDPNetworkManager : public NetworkManager {
public:
    bool initialize() override;
    void shutdown() override;
    void update() override;
    void setRole(NetworkRole role) override;
    NetworkRole getRole() const override;
    bool startServer(uint16_t port, int maxClients) override;
    void stopServer() override;
    bool isServerRunning() const override;
    bool connectToServer(const std::string& address, uint16_t port) override;
    void disconnectFromServer() override;
    bool isConnectedToServer() const override;
    bool createRoom(const std::string& roomName, int maxPeers) override;
    bool joinRoom(const std::string& roomName) override;
    void leaveRoom() override;
    bool isInRoom() const override;
    std::string getCurrentRoom() const override;
    bool sendMessage(const NetworkMessage& message) override;
    bool sendMessageToPeer(uint32_t peerId, const NetworkMessage& message) override;
    bool sendMessageToAll(const NetworkMessage& message, uint32_t excludePeerId) override;
    std::vector<NetworkMessage> receiveMessages() override;
    std::vector<NetworkPeer> getConnectedPeers() const override;
    NetworkPeer getPeer(uint32_t peerId) const override;
    uint32_t getLocalPeerId() const override;
    void disconnectPeer(uint32_t peerId) override;
    float getPing(uint32_t peerId) const override;
    uint64_t getBytesSent() const override;
    uint64_t getBytesReceived() const override;
    uint32_t getPacketsSent() const override;
    uint32_t getPacketsReceived() const override;
    uint32_t getPacketsLost() const override;
    float getPacketLossRate() const override;
    NetworkMessage createMessage(uint32_t type, const std::vector<uint8_t>& data, bool reliable, uint32_t channelId) override;
    void registerMessageHandler(uint32_t messageType, std::function<void(const NetworkMessage&, uint32_t)> handler) override;
    void unregisterMessageHandler(uint32_t messageType) override;
    void startDiscovery() override;
    void stopDiscovery() override;
    std::vector<NetworkPeer> getDiscoveredServers() const override;
    void broadcastPresence(const std::unordered_map<std::string, std::string>& metadata) override;
    void setProtocol(NetworkProtocol protocol) override;
    NetworkProtocol getProtocol() const override;
    void setTimeout(uint32_t timeoutMs) override;
    uint32_t getTimeout() const override;
    void setMaxRetries(uint32_t maxRetries) override;
    uint32_t getMaxRetries() const override;
    void setOnPeerConnectedCallback(std::function<void(const NetworkPeer&)> callback) override;
    void setOnPeerDisconnectedCallback(std::function<void(uint32_t)> callback) override;
    void setOnMessageReceivedCallback(std::function<void(const NetworkMessage&, uint32_t)> callback) override;
    void setOnServerStartedCallback(std::function<void()> callback) override;
    void setOnServerStoppedCallback(std::function<void()> callback) override;
    void setOnConnectedToServerCallback(std::function<void()> callback) override;
    void setOnDisconnectedFromServerCallback(std::function<void()> callback) override;
    void enableNetworkSync(bool enable) override;
    bool isNetworkSyncEnabled() const override;
    void syncObject(uint32_t objectId, const std::vector<uint8_t>& data) override;
    void unsyncObject(uint32_t objectId) override;
    std::vector<uint32_t> getSyncedObjects() const override;
    void enableInterpolation(bool enable) override;
    bool isInterpolationEnabled() const override;
    void setInterpolationDelay(float delayMs) override;
    float getInterpolationDelay() const override;
    void enablePrediction(bool enable) override;
    bool isPredictionEnabled() const override;
    void enableCompression(bool enable) override;
    bool isCompressionEnabled() const override;
    void setCompressionLevel(int level) override;
    int getCompressionLevel() const override;
    void enableEncryption(bool enable) override;
    bool isEncryptionEnabled() const override;
    void setEncryptionKey(const std::string& key) override;
    void setBandwidthLimit(uint32_t bytesPerSecond) override;
    uint32_t getBandwidthLimit() const override;
    uint32_t getCurrentBandwidthUsage() const override;
    void enableNetworkDebug(bool enable) override;
    bool isNetworkDebugEnabled() const override;
    void simulateLatency(uint32_t latencyMs) override;
    void simulatePacketLoss(float lossRate) override;
    void simulateJitter(uint32_t jitterMs) override;
};

} // namespace FoundryEngine