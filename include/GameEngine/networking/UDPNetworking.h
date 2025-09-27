/**
 * @file UDPNetworking.h
 * @brief Cross-platform UDP networking for Foundry Game Engine
 *
 * Provides ultra-low latency UDP communication with reliability layers,
 * connection management, and platform abstraction.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace Foundry {

// Forward declarations
class UDPSocket;
class UDPConnection;
class UDPPacket;

/**
 * @enum UDPConnectionState
 * @brief States for UDP connections
 */
enum class UDPConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
    Failed
};

/**
 * @enum UDPPacketType
 * @brief Types of UDP packets
 */
enum class UDPPacketType : uint8_t {
    // Core packets
    Connect = 1,
    ConnectAck = 2,
    Disconnect = 3,
    Heartbeat = 4,
    Ack = 5,

    // Game packets
    PlayerInput = 10,
    PlayerState = 11,
    WorldState = 12,
    EntityUpdate = 13,
    Chat = 14,

    // Reliability packets
    ReliableData = 20,
    ReliableAck = 21,

    // Custom packets (user-defined)
    CustomStart = 100
};

/**
 * @struct UDPPacket
 * @brief UDP packet structure with header and payload
 */
struct UDPPacket {
    uint16_t sequenceNumber;     // Packet sequence number
    uint16_t ackNumber;          // Acknowledgment number
    UDPPacketType type;          // Packet type
    uint8_t flags;               // Control flags
    uint32_t timestamp;          // Send timestamp
    uint16_t payloadSize;        // Size of payload
    std::vector<uint8_t> payload; // Packet data

    // Reliability flags
    static const uint8_t FLAG_RELIABLE = 0x01;
    static const uint8_t FLAG_COMPRESSED = 0x02;
    static const uint8_t FLAG_ENCRYPTED = 0x04;

    /**
     * @brief Serialize packet to bytes
     * @return Serialized packet data
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Deserialize bytes to packet
     * @param data Raw packet data
     * @return True if successful
     */
    bool deserialize(const std::vector<uint8_t>& data);
};

/**
 * @struct UDPConnectionInfo
 * @brief Information about a UDP connection
 */
struct UDPConnectionInfo {
    std::string remoteAddress;
    uint16_t remotePort;
    uint16_t localPort;
    UDPConnectionState state;
    uint32_t ping;                    // Round-trip time in ms
    uint32_t bytesSent;
    uint32_t bytesReceived;
    uint32_t packetsSent;
    uint32_t packetsReceived;
    uint32_t packetsLost;
    float packetLossRate;            // 0.0 to 1.0
};

/**
 * @class UDPConnection
 * @brief Represents a UDP connection with reliability features
 */
class UDPConnection {
public:
    UDPConnection();
    virtual ~UDPConnection();

    /**
     * @brief Connect to remote host
     * @param address Remote address
     * @param port Remote port
     * @return True if connection initiated
     */
    virtual bool connect(const std::string& address, uint16_t port) = 0;

    /**
     * @brief Disconnect from remote host
     */
    virtual void disconnect() = 0;

    /**
     * @brief Send packet to remote host
     * @param packet Packet to send
     * @param reliable Whether packet requires acknowledgment
     * @return True if sent successfully
     */
    virtual bool sendPacket(const UDPPacket& packet, bool reliable = false) = 0;

    /**
     * @brief Receive packet from remote host
     * @param packet Received packet
     * @return True if packet received
     */
    virtual bool receivePacket(UDPPacket& packet) = 0;

    /**
     * @brief Update connection (handle timeouts, resends, etc.)
     * @param deltaTime Time since last update
     */
    virtual void update(float deltaTime) = 0;

    /**
     * @brief Get connection information
     * @return Connection info
     */
    virtual UDPConnectionInfo getConnectionInfo() const = 0;

    /**
     * @brief Check if connection is active
     * @return True if connected
     */
    virtual bool isConnected() const = 0;

    // Callback setters
    void setConnectCallback(std::function<void()> callback) { onConnect = callback; }
    void setDisconnectCallback(std::function<void()> callback) { onDisconnect = callback; }
    void setPacketCallback(std::function<void(const UDPPacket&)> callback) { onPacketReceived = callback; }
    void setErrorCallback(std::function<void(const std::string&)> callback) { onError = callback; }

protected:
    UDPConnectionState state;
    std::function<void()> onConnect;
    std::function<void()> onDisconnect;
    std::function<void(const UDPPacket&)> onPacketReceived;
    std::function<void(const std::string&)> onError;

    // Reliability system
    uint16_t nextSequenceNumber;
    uint16_t nextAckNumber;
    std::unordered_map<uint16_t, UDPPacket> reliablePackets;
    std::unordered_map<uint16_t, uint32_t> sentTimes;

    // Statistics
    uint32_t bytesSent;
    uint32_t bytesReceived;
    uint32_t packetsSent;
    uint32_t packetsReceived;
    uint32_t packetsLost;
};

/**
 * @class UDPSocket
 * @brief Platform-specific UDP socket implementation
 */
class UDPSocket {
public:
    UDPSocket();
    virtual ~UDPSocket();

    /**
     * @brief Create UDP socket
     * @param port Local port to bind to (0 for any)
     * @return True if successful
     */
    virtual bool create(uint16_t port = 0) = 0;

    /**
     * @brief Close socket
     */
    virtual void close() = 0;

    /**
     * @brief Send data to address
     * @param data Data to send
     * @param address Remote address
     * @param port Remote port
     * @return Number of bytes sent, -1 on error
     */
    virtual int sendTo(const std::vector<uint8_t>& data,
                      const std::string& address, uint16_t port) = 0;

    /**
     * @brief Receive data from remote host
     * @param buffer Buffer to receive into
     * @param maxSize Maximum bytes to receive
     * @param outAddress Remote address (output)
     * @param outPort Remote port (output)
     * @return Number of bytes received, -1 on error, 0 on timeout
     */
    virtual int receiveFrom(std::vector<uint8_t>& buffer, size_t maxSize,
                           std::string& outAddress, uint16_t& outPort) = 0;

    /**
     * @brief Set socket options
     * @param option Option name
     * @param value Option value
     * @return True if successful
     */
    virtual bool setOption(int option, int value) = 0;

    /**
     * @brief Get socket options
     * @param option Option name
     * @return Option value, -1 on error
     */
    virtual int getOption(int option) = 0;

    /**
     * @brief Set non-blocking mode
     * @param nonBlocking True for non-blocking
     * @return True if successful
     */
    virtual bool setNonBlocking(bool nonBlocking) = 0;

    /**
     * @brief Check if socket is valid
     * @return True if valid
     */
    virtual bool isValid() const = 0;

    // Socket options
    static const int OPTION_REUSEADDR = 1;
    static const int OPTION_BROADCAST = 2;
    static const int OPTION_RCVBUF = 3;
    static const int OPTION_SNDBUF = 4;
};

/**
 * @class UDPNetworking
 * @brief Main UDP networking manager
 */
class UDPNetworking {
public:
    UDPNetworking();
    ~UDPNetworking();

    /**
     * @brief Initialize networking system
     * @return True if successful
     */
    bool initialize();

    /**
     * @brief Shutdown networking system
     */
    void shutdown();

    /**
     * @brief Update networking (call regularly)
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);

    /**
     * @brief Create new UDP connection
     * @return New connection instance
     */
    std::shared_ptr<UDPConnection> createConnection();

    /**
     * @brief Create UDP server socket
     * @param port Port to listen on
     * @return Server socket instance
     */
    std::shared_ptr<UDPSocket> createServerSocket(uint16_t port);

    /**
     * @brief Get networking statistics
     * @return Statistics string
     */
    std::string getStatistics() const;

    /**
     * @brief Set global packet loss simulation (for testing)
     * @param rate Packet loss rate (0.0 to 1.0)
     */
    void setSimulatedPacketLoss(float rate);

private:
    bool initialized;
    std::vector<std::shared_ptr<UDPConnection>> connections;
    std::mutex connectionsMutex;
    float simulatedPacketLoss;

    // Platform-specific socket factory
    virtual std::shared_ptr<UDPSocket> createSocket() = 0;
    virtual std::shared_ptr<UDPConnection> createConnectionImpl() = 0;
};

// Platform-specific implementations
#if defined(PLATFORM_WINDOWS)
class WindowsUDPSocket : public UDPSocket {
    // Windows-specific implementation
};

class WindowsUDPConnection : public UDPConnection {
    // Windows-specific implementation
};

class WindowsUDPNetworking : public UDPNetworking {
    // Windows-specific implementation
};
#elif defined(PLATFORM_APPLE)
class AppleUDPSocket : public UDPSocket {
    // macOS/iOS-specific implementation
};

class AppleUDPConnection : public UDPConnection {
    // macOS/iOS-specific implementation
};

class AppleUDPNetworking : public UDPNetworking {
    // macOS/iOS-specific implementation
};
#elif defined(PLATFORM_ANDROID)
class AndroidUDPSocket : public UDPSocket {
    // Android-specific implementation
};

class AndroidUDPConnection : public UDPConnection {
    // Android-specific implementation
};

class AndroidUDPNetworking : public UDPNetworking {
    // Android-specific implementation
};
#endif

} // namespace Foundry
