/**
 * @file UDPNetworking.cpp
 * @brief Cross-platform UDP networking implementation for Foundry
 */

#include "GameEngine/networking/UDPNetworking.h"
#include <cstring>
#include <chrono>
#include <algorithm>

namespace Foundry {

// ========== UDPPacket Implementation ==========

std::vector<uint8_t> UDPPacket::serialize() const {
    std::vector<uint8_t> data;
    data.reserve(13 + payload.size()); // Header + payload

    // Write header (13 bytes)
    // Sequence number (2 bytes)
    data.push_back(static_cast<uint8_t>(sequenceNumber & 0xFF));
    data.push_back(static_cast<uint8_t>((sequenceNumber >> 8) & 0xFF));

    // Ack number (2 bytes)
    data.push_back(static_cast<uint8_t>(ackNumber & 0xFF));
    data.push_back(static_cast<uint8_t>((ackNumber >> 8) & 0xFF));

    // Type (1 byte)
    data.push_back(static_cast<uint8_t>(type));

    // Flags (1 byte)
    data.push_back(flags);

    // Timestamp (4 bytes)
    uint32_t ts = timestamp;
    data.push_back(static_cast<uint8_t>(ts & 0xFF));
    data.push_back(static_cast<uint8_t>((ts >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>((ts >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((ts >> 24) & 0xFF));

    // Payload size (2 bytes)
    uint16_t ps = payloadSize;
    data.push_back(static_cast<uint8_t>(ps & 0xFF));
    data.push_back(static_cast<uint8_t>((ps >> 8) & 0xFF));

    // Payload
    data.insert(data.end(), payload.begin(), payload.end());

    return data;
}

bool UDPPacket::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 13) {
        return false;
    }

    size_t offset = 0;

    // Read sequence number
    sequenceNumber = static_cast<uint16_t>(data[offset]) |
                    (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    // Read ack number
    ackNumber = static_cast<uint16_t>(data[offset]) |
               (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    // Read type
    type = static_cast<UDPPacketType>(data[offset]);
    offset += 1;

    // Read flags
    flags = data[offset];
    offset += 1;

    // Read timestamp
    timestamp = static_cast<uint32_t>(data[offset]) |
               (static_cast<uint32_t>(data[offset + 1]) << 8) |
               (static_cast<uint32_t>(data[offset + 2]) << 16) |
               (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Read payload size
    payloadSize = static_cast<uint16_t>(data[offset]) |
                 (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    // Read payload
    if (data.size() - offset != payloadSize) {
        return false;
    }

    payload.assign(data.begin() + offset, data.end());

    return true;
}

// ========== UDPConnection Implementation ==========

UDPConnection::UDPConnection()
    : state(UDPConnectionState::Disconnected)
    , nextSequenceNumber(1)
    , nextAckNumber(0)
    , bytesSent(0)
    , bytesReceived(0)
    , packetsSent(0)
    , packetsReceived(0)
    , packetsLost(0) {
}

UDPConnection::~UDPConnection() {
    disconnect();
}

// ========== UDPSocket Implementation ==========

UDPSocket::UDPSocket() {
}

UDPSocket::~UDPSocket() {
    close();
}

// ========== UDPNetworking Implementation ==========

UDPNetworking::UDPNetworking()
    : initialized(false)
    , simulatedPacketLoss(0.0f) {
}

UDPNetworking::~UDPNetworking() {
    shutdown();
}

bool UDPNetworking::initialize() {
    if (initialized) {
        return true;
    }

    initialized = true;
    return true;
}

void UDPNetworking::shutdown() {
    if (!initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(connectionsMutex);
    for (auto& connection : connections) {
        if (connection) {
            connection->disconnect();
        }
    }
    connections.clear();

    initialized = false;
}

void UDPNetworking::update(float deltaTime) {
    if (!initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(connectionsMutex);
    for (auto& connection : connections) {
        if (connection) {
            connection->update(deltaTime);
        }
    }
}

std::shared_ptr<UDPConnection> UDPNetworking::createConnection() {
    if (!initialized) {
        return nullptr;
    }

    auto connection = createConnectionImpl();
    if (connection) {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        connections.push_back(connection);
    }

    return connection;
}

std::shared_ptr<UDPSocket> UDPNetworking::createServerSocket(uint16_t port) {
    if (!initialized) {
        return nullptr;
    }

    auto socket = createSocket();
    if (socket && socket->create(port)) {
        return socket;
    }

    return nullptr;
}

std::string UDPNetworking::getStatistics() const {
    if (!initialized) {
        return "UDP Networking not initialized";
    }

    std::lock_guard<std::mutex> lock(connectionsMutex);

    char buffer[512];
    snprintf(buffer, sizeof(buffer),
             "UDP Networking Stats:\n"
             "  Connections: %zu\n"
             "  Simulated Packet Loss: %.2f%%\n",
             connections.size(),
             simulatedPacketLoss * 100.0f);

    return std::string(buffer);
}

void UDPNetworking::setSimulatedPacketLoss(float rate) {
    simulatedPacketLoss = std::max(0.0f, std::min(1.0f, rate));
}

// ========== Android Implementation ==========

#ifdef PLATFORM_ANDROID

#include <jni.h>
#include <android/log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>

#define LOG_TAG "UDPNetworking"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class AndroidUDPSocket : public UDPSocket {
private:
    int socketFd;
    bool valid;

public:
    AndroidUDPSocket() : socketFd(-1), valid(false) {}
    ~AndroidUDPSocket() { close(); }

    bool create(uint16_t port = 0) override {
        socketFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socketFd < 0) {
            LOGE("Failed to create UDP socket: %s", strerror(errno));
            return false;
        }

        // Set socket options
        int reuse = 1;
        if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            LOGE("Failed to set SO_REUSEADDR: %s", strerror(errno));
        }

        // Bind to port
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOGE("Failed to bind UDP socket: %s", strerror(errno));
            ::close(socketFd);
            socketFd = -1;
            return false;
        }

        // Set non-blocking
        int flags = fcntl(socketFd, F_GETFL, 0);
        fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);

        // Set buffer sizes
        int bufferSize = 4 * 1024 * 1024; // 4MB
        setsockopt(socketFd, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
        setsockopt(socketFd, SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));

        valid = true;
        LOGI("UDP socket created and bound to port %d", port);
        return true;
    }

    void close() override {
        if (socketFd >= 0) {
            ::close(socketFd);
            socketFd = -1;
        }
        valid = false;
    }

    int sendTo(const std::vector<uint8_t>& data, const std::string& address, uint16_t port) override {
        if (!valid || socketFd < 0) {
            return -1;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) <= 0) {
            LOGE("Invalid address: %s", address.c_str());
            return -1;
        }

        ssize_t sent = sendto(socketFd, data.data(), data.size(), 0,
                             (struct sockaddr*)&addr, sizeof(addr));

        if (sent < 0) {
            LOGE("Send failed: %s", strerror(errno));
            return -1;
        }

        return static_cast<int>(sent);
    }

    int receiveFrom(std::vector<uint8_t>& buffer, size_t maxSize,
                   std::string& outAddress, uint16_t& outPort) override {
        if (!valid || socketFd < 0) {
            return -1;
        }

        buffer.resize(maxSize);

        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);

        ssize_t received = recvfrom(socketFd, buffer.data(), maxSize, 0,
                                   (struct sockaddr*)&addr, &addrLen);

        if (received < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return 0; // No data available
            }
            LOGE("Receive failed: %s", strerror(errno));
            return -1;
        }

        // Get sender address
        char addrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, addrStr, sizeof(addrStr));
        outAddress = addrStr;
        outPort = ntohs(addr.sin_port);

        buffer.resize(received);
        return static_cast<int>(received);
    }

    bool setOption(int option, int value) override {
        if (!valid || socketFd < 0) {
            return false;
        }

        int level = SOL_SOCKET;
        int optname;

        switch (option) {
        case OPTION_REUSEADDR:
            optname = SO_REUSEADDR;
            break;
        case OPTION_BROADCAST:
            optname = SO_BROADCAST;
            break;
        case OPTION_RCVBUF:
            optname = SO_RCVBUF;
            break;
        case OPTION_SNDBUF:
            optname = SO_SNDBUF;
            break;
        default:
            return false;
        }

        if (setsockopt(socketFd, level, optname, &value, sizeof(value)) < 0) {
            LOGE("Failed to set socket option %d: %s", option, strerror(errno));
            return false;
        }

        return true;
    }

    int getOption(int option) override {
        if (!valid || socketFd < 0) {
            return -1;
        }

        int level = SOL_SOCKET;
        int optname;
        int value;
        socklen_t len = sizeof(value);

        switch (option) {
        case OPTION_REUSEADDR:
            optname = SO_REUSEADDR;
            break;
        case OPTION_BROADCAST:
            optname = SO_BROADCAST;
            break;
        case OPTION_RCVBUF:
            optname = SO_RCVBUF;
            break;
        case OPTION_SNDBUF:
            optname = SO_SNDBUF;
            break;
        default:
            return -1;
        }

        if (getsockopt(socketFd, level, optname, &value, &len) < 0) {
            LOGE("Failed to get socket option %d: %s", option, strerror(errno));
            return -1;
        }

        return value;
    }

    bool setNonBlocking(bool nonBlocking) override {
        if (!valid || socketFd < 0) {
            return false;
        }

        int flags = fcntl(socketFd, F_GETFL, 0);
        if (flags < 0) {
            return false;
        }

        if (nonBlocking) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }

        if (fcntl(socketFd, F_SETFL, flags) < 0) {
            LOGE("Failed to set non-blocking mode: %s", strerror(errno));
            return false;
        }

        return true;
    }

    bool isValid() const override {
        return valid && socketFd >= 0;
    }
};

class AndroidUDPConnection : public UDPConnection {
private:
    std::shared_ptr<AndroidUDPSocket> socket;
    std::string remoteAddress;
    uint16_t remotePort;
    uint16_t localPort;
    uint32_t lastPingTime;
    uint32_t ping;
    bool connected;

    // Reliability
    std::unordered_map<uint16_t, std::chrono::steady_clock::time_point> reliablePacketTimes;
    static const uint32_t RELIABLE_TIMEOUT_MS = 5000; // 5 seconds
    static const uint32_t MAX_RETRIES = 5;

public:
    AndroidUDPConnection() : remotePort(0), localPort(0), lastPingTime(0), ping(0), connected(false) {
        socket = std::make_shared<AndroidUDPSocket>();
    }

    ~AndroidUDPConnection() {
        disconnect();
    }

    bool connect(const std::string& address, uint16_t port) override {
        remoteAddress = address;
        remotePort = port;

        // Create socket with random local port
        if (!socket->create(0)) {
            if (onError) onError("Failed to create socket");
            return false;
        }

        state = UDPConnectionState::Connecting;
        connected = true;

        // Send initial connect packet
        UDPPacket connectPacket;
        connectPacket.sequenceNumber = nextSequenceNumber++;
        connectPacket.type = UDPPacketType::Connect;
        connectPacket.timestamp = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        sendPacket(connectPacket, true);

        if (onConnect) onConnect();
        state = UDPConnectionState::Connected;

        LOGI("UDP connection initiated to %s:%d", address.c_str(), port);
        return true;
    }

    void disconnect() override {
        if (!connected) return;

        // Send disconnect packet
        UDPPacket disconnectPacket;
        disconnectPacket.sequenceNumber = nextSequenceNumber++;
        disconnectPacket.type = UDPPacketType::Disconnect;
        disconnectPacket.timestamp = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        sendPacket(disconnectPacket, true);

        socket->close();
        connected = false;
        state = UDPConnectionState::Disconnected;

        if (onDisconnect) onDisconnect();

        LOGI("UDP connection closed");
    }

    bool sendPacket(const UDPPacket& packet, bool reliable = false) override {
        if (!connected) return false;

        UDPPacket packetToSend = packet;

        // Set reliability flag
        if (reliable) {
            packetToSend.flags |= UDPPacket::FLAG_RELIABLE;
            reliablePacketTimes[packet.sequenceNumber] =
                std::chrono::steady_clock::now();
        }

        // Serialize and send
        auto data = packetToSend.serialize();
        int sent = socket->sendTo(data, remoteAddress, remotePort);

        if (sent > 0) {
            bytesSent += sent;
            packetsSent++;
            return true;
        }

        packetsLost++;
        return false;
    }

    bool receivePacket(UDPPacket& packet) override {
        if (!connected) return false;

        std::string senderAddr;
        uint16_t senderPort;
        std::vector<uint8_t> buffer;

        int received = socket->receiveFrom(buffer, 1472, senderAddr, senderPort);
        if (received <= 0) {
            return false;
        }

        // Only accept packets from our remote host
        if (senderAddr != remoteAddress || senderPort != remotePort) {
            return false;
        }

        if (!packet.deserialize(buffer)) {
            return false;
        }

        bytesReceived += received;
        packetsReceived++;

        // Handle acknowledgments for reliable packets
        if (packet.type == UDPPacketType::Ack) {
            // Remove acknowledged packet from reliable queue
            auto it = reliablePacketTimes.find(packet.ackNumber);
            if (it != reliablePacketTimes.end()) {
                reliablePacketTimes.erase(it);
            }
        }

        return true;
    }

    void update(float deltaTime) override {
        if (!connected) return;

        // Send heartbeat
        auto now = std::chrono::steady_clock::now();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        if (now_ms - lastPingTime > 1000) { // 1 second heartbeat
            UDPPacket heartbeat;
            heartbeat.sequenceNumber = nextSequenceNumber++;
            heartbeat.type = UDPPacketType::Heartbeat;
            heartbeat.timestamp = static_cast<uint32_t>(now_ms);

            sendPacket(heartbeat, false);
            lastPingTime = static_cast<uint32_t>(now_ms);
        }

        // Resend reliable packets that timed out
        auto currentTime = std::chrono::steady_clock::now();
        for (auto it = reliablePacketTimes.begin(); it != reliablePacketTimes.end(); ) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - it->second).count();

            if (elapsed > RELIABLE_TIMEOUT_MS) {
                // Packet timed out, resend
                auto packetIt = reliablePackets.find(it->first);
                if (packetIt != reliablePackets.end()) {
                    sendPacket(packetIt->second, true);
                    it->second = currentTime; // Reset timeout
                } else {
                    it = reliablePacketTimes.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    UDPConnectionInfo getConnectionInfo() const override {
        UDPConnectionInfo info;
        info.remoteAddress = remoteAddress;
        info.remotePort = remotePort;
        info.localPort = localPort;
        info.state = state;
        info.ping = ping;
        info.bytesSent = bytesSent;
        info.bytesReceived = bytesReceived;
        info.packetsSent = packetsSent;
        info.packetsReceived = packetsReceived;
        info.packetsLost = packetsLost;
        info.packetLossRate = packetsSent > 0 ?
            static_cast<float>(packetsLost) / packetsSent : 0.0f;

        return info;
    }

    bool isConnected() const override {
        return connected && state == UDPConnectionState::Connected;
    }
};

class AndroidUDPNetworking : public UDPNetworking {
protected:
    std::shared_ptr<UDPSocket> createSocket() override {
        return std::make_shared<AndroidUDPSocket>();
    }

    std::shared_ptr<UDPConnection> createConnectionImpl() override {
        return std::make_shared<AndroidUDPConnection>();
    }
};

#endif // PLATFORM_ANDROID

// ========== Factory Functions ==========

extern "C" {

UDPNetworking* createUDPNetworking() {
#ifdef PLATFORM_ANDROID
    return new AndroidUDPNetworking();
#else
    return nullptr; // Not implemented for this platform
#endif
}

void destroyUDPNetworking(UDPNetworking* networking) {
    delete networking;
}

} // extern "C"

} // namespace Foundry
