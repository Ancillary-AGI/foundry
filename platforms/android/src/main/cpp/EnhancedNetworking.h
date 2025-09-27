#ifndef FOUNDRYENGINE_ENHANCED_NETWORKING_H
#define FOUNDRYENGINE_ENHANCED_NETWORKING_H

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
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

namespace FoundryEngine {

// Forward declarations
class WebSocketManager;
class WebRTCManager;
class QUICManager;
class NetworkManager;

// Connection states
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    FAILED,
    CLOSED
};

// Message types
enum class MessageType {
    TEXT,
    BINARY,
    PING,
    PONG,
    CLOSE
};

// Network protocol types
enum class NetworkProtocol {
    TCP,
    UDP,
    WEBSOCKET,
    WEBRTC,
    QUIC
};

// Quality of Service levels
enum class QoSLevel {
    RELIABLE_ORDERED,    // TCP-like reliability with ordering
    RELIABLE_UNORDERED,  // Reliable but packets may be reordered
    UNRELIABLE_ORDERED,  // Unreliable but ordered
    UNRELIABLE           // Best effort, no guarantees
};

// Network message
struct NetworkMessage {
    std::string endpoint;
    std::vector<uint8_t> data;
    MessageType type;
    QoSLevel qos;
    uint32_t sequenceNumber;
    double timestamp;
    bool compressed;
};

// WebSocket frame types
enum class WebSocketFrameType {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

// WebSocket connection states
enum class WebSocketState {
    CONNECTING = 0,
    OPEN = 1,
    CLOSING = 2,
    CLOSED = 3
};

// WebRTC connection states
enum class WebRTCState {
    NEW,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    FAILED,
    CLOSED
};

// QUIC connection states
enum class QUICState {
    INITIALIZING,
    HANDSHAKE,
    CONNECTED,
    DISCONNECTED,
    FAILED
};

// Callback types
using NetworkCallback = std::function<void(const NetworkMessage&)>;
using ConnectionCallback = std::function<void(const std::string&, ConnectionState)>;
using ErrorCallback = std::function<void(const std::string&, const std::string&)>;

// ========== NETWORK MANAGER ==========
class NetworkManager : public System {
private:
    static NetworkManager* instance_;

    WebSocketManager* webSocketManager_;
    WebRTCManager* webRTCManager_;
    QUICManager* quicManager_;

    std::unordered_map<std::string, NetworkCallback> messageHandlers_;
    std::unordered_map<std::string, ConnectionCallback> connectionHandlers_;
    std::unordered_map<std::string, ErrorCallback> errorHandlers_;

    bool initialized_;
    std::atomic<bool> running_;

    // Network statistics
    std::atomic<uint64_t> bytesSent_;
    std::atomic<uint64_t> bytesReceived_;
    std::atomic<uint32_t> messagesSent_;
    std::atomic<uint32_t> messagesReceived_;
    std::atomic<double> averageLatency_;

    // JNI environment
    JNIEnv* env_;
    jobject activity_;

public:
    NetworkManager();
    ~NetworkManager();

    static NetworkManager* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // Network management
    void setJNIEnvironment(JNIEnv* env, jobject activity);
    bool isRunning() const { return running_; }

    // Message handling
    void registerMessageHandler(const std::string& endpoint, NetworkCallback callback);
    void unregisterMessageHandler(const std::string& endpoint);
    void registerConnectionHandler(const std::string& endpoint, ConnectionCallback callback);
    void unregisterConnectionHandler(const std::string& endpoint);
    void registerErrorHandler(const std::string& endpoint, ErrorCallback callback);
    void unregisterErrorHandler(const std::string& endpoint);

    // WebSocket operations
    bool connectWebSocket(const std::string& url, const std::string& endpoint);
    void disconnectWebSocket(const std::string& endpoint);
    void sendWebSocketMessage(const std::string& endpoint, const std::string& message);
    void sendWebSocketBinary(const std::string& endpoint, const std::vector<uint8_t>& data);

    // WebRTC operations
    bool initializeWebRTC();
    bool createWebRTCOffer(const std::string& endpoint);
    bool createWebRTCAnswer(const std::string& endpoint, const std::string& offer);
    bool setWebRTCRemoteDescription(const std::string& endpoint, const std::string& sdp);
    void addWebRTCIceCandidate(const std::string& endpoint, const std::string& candidate);
    void sendWebRTCData(const std::string& endpoint, const std::vector<uint8_t>& data);

    // QUIC operations
    bool connectQUIC(const std::string& host, int port, const std::string& endpoint);
    void disconnectQUIC(const std::string& endpoint);
    void sendQUICData(const std::string& endpoint, const std::vector<uint8_t>& data);

    // Statistics
    uint64_t getBytesSent() const { return bytesSent_; }
    uint64_t getBytesReceived() const { return bytesReceived_; }
    uint32_t getMessagesSent() const { return messagesSent_; }
    uint32_t getMessagesReceived() const { return messagesReceived_; }
    double getAverageLatency() const { return averageLatency_; }

private:
    void onMessageReceived(const NetworkMessage& message);
    void onConnectionStateChanged(const std::string& endpoint, ConnectionState state);
    void onError(const std::string& endpoint, const std::string& error);
};

// ========== WEBSOCKET MANAGER ==========
class WebSocketManager {
private:
    NetworkManager* networkManager_;

    struct WebSocketConnection {
        std::string url;
        std::string endpoint;
        int socketFd;
        WebSocketState state;
        std::thread receiveThread;
        std::atomic<bool> running;

        // Frame parsing
        std::vector<uint8_t> frameBuffer;
        bool isFrameComplete;
        uint64_t payloadLength;
        WebSocketFrameType frameType;
        bool isMasked;
        uint32_t maskKey;

        // Message reassembly
        std::vector<uint8_t> messageBuffer;
        bool isMessageComplete;
        MessageType currentMessageType;

        // Callbacks
        std::function<void(const std::string&)> onMessage;
        std::function<void()> onConnect;
        std::function<void()> onDisconnect;
        std::function<void(const std::string&)> onError;
    };

    std::unordered_map<std::string, std::unique_ptr<WebSocketConnection>> connections_;
    std::mutex connectionsMutex_;

public:
    WebSocketManager(NetworkManager* networkManager);
    ~WebSocketManager();

    bool connect(const std::string& url, const std::string& endpoint);
    void disconnect(const std::string& endpoint);
    void sendMessage(const std::string& endpoint, const std::string& message);
    void sendBinary(const std::string& endpoint, const std::vector<uint8_t>& data);
    bool isConnected(const std::string& endpoint) const;

private:
    bool performWebSocketHandshake(const std::string& url, int socketFd);
    std::string generateWebSocketKey();
    std::string computeWebSocketAccept(const std::string& key);
    void receiveLoop(WebSocketConnection* connection);
    void processFrame(WebSocketConnection* connection);
    void handleMessage(WebSocketConnection* connection, const std::vector<uint8_t>& data, MessageType type);
    void sendFrame(int socketFd, WebSocketFrameType type, const std::vector<uint8_t>& data);
    std::vector<uint8_t> maskData(const std::vector<uint8_t>& data, uint32_t mask);
    std::vector<uint8_t> unmaskData(const std::vector<uint8_t>& data, uint32_t mask);
};

// ========== WEBRTC MANAGER ==========
class WebRTCManager {
private:
    NetworkManager* networkManager_;

    struct WebRTCConnection {
        std::string endpoint;
        WebRTCState state;
        std::string localSdp;
        std::string remoteSdp;
        std::vector<std::string> iceCandidates;

        // Data channels
        int dataChannelFd;
        std::thread receiveThread;
        std::atomic<bool> running;

        // STUN/TURN servers
        std::vector<std::string> stunServers;
        std::vector<std::string> turnServers;

        // Callbacks
        std::function<void(const std::string&)> onDataReceived;
        std::function<void(const std::string&)> onStateChanged;
    };

    std::unordered_map<std::string, std::unique_ptr<WebRTCConnection>> connections_;
    std::mutex connectionsMutex_;

    // ICE gathering
    std::atomic<bool> iceGatheringComplete_;
    std::vector<std::string> localIceCandidates_;

public:
    WebRTCManager(NetworkManager* networkManager);
    ~WebRTCManager();

    bool initialize();
    void shutdown();

    bool createPeerConnection(const std::string& endpoint);
    bool createOffer(const std::string& endpoint);
    bool createAnswer(const std::string& endpoint);
    bool setRemoteDescription(const std::string& endpoint, const std::string& sdp);
    void addIceCandidate(const std::string& endpoint, const std::string& candidate);
    void sendData(const std::string& endpoint, const std::vector<uint8_t>& data);

    void addStunServer(const std::string& server);
    void addTurnServer(const std::string& server, const std::string& username, const std::string& password);

private:
    std::string createSdpOffer();
    std::string createSdpAnswer();
    void gatherIceCandidates(const std::string& endpoint);
    void processIceCandidates(const std::string& endpoint);
    void establishDataChannel(const std::string& endpoint);
    void receiveLoop(WebRTCConnection* connection);
    void handleDataReceived(const std::string& endpoint, const std::vector<uint8_t>& data);
};

// ========== QUIC MANAGER ==========
class QUICManager {
private:
    NetworkManager* networkManager_;

    struct QUICConnection {
        std::string host;
        int port;
        std::string endpoint;
        int socketFd;
        QUICState state;
        std::thread receiveThread;
        std::atomic<bool> running;

        // QUIC-specific
        uint64_t connectionId;
        std::unordered_map<uint64_t, std::vector<uint8_t>> streamBuffers;
        std::mutex streamMutex_;

        // Callbacks
        std::function<void(const std::vector<uint8_t>&)> onDataReceived;
        std::function<void()> onConnected;
        std::function<void()> onDisconnected;
    };

    std::unordered_map<std::string, std::unique_ptr<QUICConnection>> connections_;
    std::mutex connectionsMutex_;

    // QUIC parameters
    uint32_t maxStreamData_;
    uint32_t maxData_;
    uint32_t maxStreamsBidi_;
    uint32_t maxStreamsUni_;

public:
    QUICManager(NetworkManager* networkManager);
    ~QUICManager();

    bool connect(const std::string& host, int port, const std::string& endpoint);
    void disconnect(const std::string& endpoint);
    void sendData(const std::string& endpoint, const std::vector<uint8_t>& data);
    bool isConnected(const std::string& endpoint) const;

    void setMaxStreamData(uint32_t max) { maxStreamData_ = max; }
    void setMaxData(uint32_t max) { maxData_ = max; }
    void setMaxStreamsBidi(uint32_t max) { maxStreamsBidi_ = max; }
    void setMaxStreamsUni(uint32_t max) { maxStreamsUni_ = max; }

private:
    bool performQUICHandshake(QUICConnection* connection);
    void sendQUICPacket(QUICConnection* connection, const std::vector<uint8_t>& data);
    void receiveLoop(QUICConnection* connection);
    void processQUICPacket(QUICConnection* connection, const std::vector<uint8_t>& packet);
    void handleStreamData(QUICConnection* connection, uint64_t streamId, const std::vector<uint8_t>& data);
    std::vector<uint8_t> createQUICPacket(uint64_t connectionId, uint64_t streamId, const std::vector<uint8_t>& data);
};

// ========== NETWORK UTILITIES ==========
class NetworkUtils {
public:
    // Address resolution
    static std::vector<std::string> resolveHost(const std::string& hostname);
    static bool isValidIpAddress(const std::string& address);

    // Socket utilities
    static int createSocket(int domain, int type, int protocol);
    static bool setSocketNonBlocking(int socketFd);
    static bool setSocketReusable(int socketFd);
    static bool bindSocket(int socketFd, const std::string& address, int port);
    static bool connectSocket(int socketFd, const std::string& address, int port);
    static void closeSocket(int socketFd);

    // Data compression
    static std::vector<uint8_t> compressData(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decompressData(const std::vector<uint8_t>& data);

    // Base64 encoding/decoding
    static std::string encodeBase64(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decodeBase64(const std::string& data);

    // JSON utilities
    static std::string serializeMessage(const NetworkMessage& message);
    static NetworkMessage deserializeMessage(const std::string& json);

    // Bandwidth estimation
    static double estimateBandwidth(const std::vector<double>& latencies);
    static double calculateLatency(const std::string& host, int port);

    // Network quality assessment
    static std::string getNetworkType();
    static int getSignalStrength();
    static bool isNetworkMetered();
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // WebSocket callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onWebSocketConnected(
        JNIEnv* env, jobject thiz, jstring endpoint);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onWebSocketDisconnected(
        JNIEnv* env, jobject thiz, jstring endpoint);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onWebSocketMessage(
        JNIEnv* env, jobject thiz, jstring endpoint, jbyteArray data);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onWebSocketError(
        JNIEnv* env, jobject thiz, jstring endpoint, jstring error);

    // WebRTC callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onWebRTCStateChanged(
        JNIEnv* env, jobject thiz, jstring endpoint, jint state);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onWebRTCDataReceived(
        JNIEnv* env, jobject thiz, jstring endpoint, jbyteArray data);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onWebRTCIceCandidate(
        JNIEnv* env, jobject thiz, jstring endpoint, jstring candidate);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onWebRTCOffer(
        JNIEnv* env, jobject thiz, jstring endpoint, jstring sdp);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onWebRTCAnswer(
        JNIEnv* env, jobject thiz, jstring endpoint, jstring sdp);

    // QUIC callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onQUICConnected(
        JNIEnv* env, jobject thiz, jstring endpoint);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onQUICDisconnected(
        JNIEnv* env, jobject thiz, jstring endpoint);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onQUICDataReceived(
        JNIEnv* env, jobject thiz, jstring endpoint, jbyteArray data);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onQUICError(
        JNIEnv* env, jobject thiz, jstring endpoint, jstring error);

    // Network utilities callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onNetworkTypeChanged(
        JNIEnv* env, jobject thiz, jstring networkType);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_NetworkManager_onBandwidthChanged(
        JNIEnv* env, jobject thiz, jdouble bandwidth);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_ENHANCED_NETWORKING_H
