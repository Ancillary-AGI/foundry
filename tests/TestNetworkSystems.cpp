#include "gtest/gtest.h"
#include "GameEngine/networking/UDPNetworking.h"
#include "GameEngine/networking/AdvancedNetworking.h"
#include "GameEngine/core/MemoryPool.h"
#include <thread>
#include <chrono>
#include <atomic>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Network Systems tests
 */
class NetworkSystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(2048, 16384);
    }

    void TearDown() override {
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
};

/**
 * @brief Test UDP networking system
 */
TEST_F(NetworkSystemsTest, UDPNetworking) {
    UDPNetworking udp;

    // Test UDP initialization
    EXPECT_TRUE(udp.initialize());
    EXPECT_TRUE(udp.isInitialized());

    // Test socket creation
    SocketID clientSocket = udp.createSocket();
    SocketID serverSocket = udp.createSocket();

    EXPECT_GT(clientSocket, 0);
    EXPECT_GT(serverSocket, 0);

    // Test socket binding
    EXPECT_TRUE(udp.bindSocket(serverSocket, "127.0.0.1", 8888));
    EXPECT_TRUE(udp.bindSocket(clientSocket, "127.0.0.1", 8889));

    // Test connection
    EXPECT_TRUE(udp.connectSocket(clientSocket, "127.0.0.1", 8888));
    EXPECT_TRUE(udp.isSocketConnected(clientSocket));

    // Test data sending
    std::string testMessage = "Hello UDP Network!";
    std::vector<char> messageData(testMessage.begin(), testMessage.end());

    int bytesSent = udp.sendData(clientSocket, messageData.data(), messageData.size());
    EXPECT_EQ(bytesSent, testMessage.length());

    // Test data receiving
    std::vector<char> receiveBuffer(1024);
    int bytesReceived = udp.receiveData(serverSocket, receiveBuffer.data(), receiveBuffer.size());

    EXPECT_GT(bytesReceived, 0);
    std::string receivedMessage(receiveBuffer.begin(), receiveBuffer.begin() + bytesReceived);
    EXPECT_EQ(receivedMessage, testMessage);

    // Test socket options
    udp.setSocketTimeout(clientSocket, 5000); // 5 seconds
    EXPECT_EQ(udp.getSocketTimeout(clientSocket), 5000);

    udp.setSocketBufferSize(clientSocket, 8192);
    EXPECT_EQ(udp.getSocketBufferSize(clientSocket), 8192);

    // Test broadcast
    udp.enableBroadcast(serverSocket, true);
    EXPECT_TRUE(udp.isBroadcastEnabled(serverSocket));

    // Test multicast
    udp.joinMulticastGroup(clientSocket, "224.0.0.1");
    EXPECT_TRUE(udp.isInMulticastGroup(clientSocket, "224.0.0.1"));

    udp.leaveMulticastGroup(clientSocket, "224.0.0.1");
    EXPECT_FALSE(udp.isInMulticastGroup(clientSocket, "224.0.0.1"));

    // Test cleanup
    udp.disconnectSocket(clientSocket);
    udp.closeSocket(serverSocket);
    udp.closeSocket(clientSocket);

    udp.shutdown();
    EXPECT_FALSE(udp.isInitialized());
}

/**
 * @brief Test advanced networking system
 */
TEST_F(NetworkSystemsTest, AdvancedNetworking) {
    AdvancedNetworking net;

    // Test advanced networking initialization
    EXPECT_TRUE(net.initialize());
    EXPECT_TRUE(net.isInitialized());

    // Test connection management
    ConnectionID connection1 = net.createConnection("127.0.0.1", 7777, ConnectionType::TCP);
    ConnectionID connection2 = net.createConnection("127.0.0.1", 7778, ConnectionType::UDP);

    EXPECT_GT(connection1, 0);
    EXPECT_GT(connection2, 0);

    // Test connection properties
    net.setConnectionTimeout(connection1, 10000); // 10 seconds
    EXPECT_EQ(net.getConnectionTimeout(connection1), 10000);

    net.setConnectionBufferSize(connection1, 16384);
    EXPECT_EQ(net.getConnectionBufferSize(connection1), 16384);

    // Test reliable messaging
    net.enableReliableMessaging(connection1, true);
    EXPECT_TRUE(net.isReliableMessagingEnabled(connection1));

    net.setReliabilityWindow(connection1, 32);
    EXPECT_EQ(net.getReliabilityWindow(connection1), 32);

    // Test message queuing
    MessageID msg1 = net.queueMessage(connection1, "Hello", 5, MessagePriority::High);
    MessageID msg2 = net.queueMessage(connection1, "World", 5, MessagePriority::Normal);

    EXPECT_GT(msg1, 0);
    EXPECT_GT(msg2, 0);

    // Test message processing
    net.processOutgoingMessages();
    net.processIncomingMessages();

    // Test bandwidth management
    net.setBandwidthLimit(connection1, 1000000); // 1 Mbps
    EXPECT_EQ(net.getBandwidthLimit(connection1), 1000000);

    net.enableBandwidthThrottling(connection1, true);
    EXPECT_TRUE(net.isBandwidthThrottlingEnabled(connection1));

    // Test latency simulation
    net.enableLatencySimulation(true);
    EXPECT_TRUE(net.isLatencySimulationEnabled());

    net.setSimulatedLatency(50.0f); // 50ms
    EXPECT_FLOAT_EQ(net.getSimulatedLatency(), 50.0f);

    net.setPacketLossRate(0.05f); // 5% packet loss
    EXPECT_FLOAT_EQ(net.getPacketLossRate(), 0.05f);

    // Test network statistics
    NetworkStats stats = net.getConnectionStats(connection1);
    EXPECT_GE(stats.bytesSent, 0);
    EXPECT_GE(stats.bytesReceived, 0);
    EXPECT_GE(stats.packetsSent, 0);
    EXPECT_GE(stats.packetsReceived, 0);

    // Test cleanup
    net.destroyConnection(connection2);
    net.destroyConnection(connection1);

    net.shutdown();
    EXPECT_FALSE(net.isInitialized());
}

/**
 * @brief Test network performance
 */
TEST_F(NetworkSystemsTest, Performance) {
    const int numIterations = 100;

    // Measure network operations performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        UDPNetworking udp;
        udp.initialize();

        SocketID socket = udp.createSocket();
        udp.bindSocket(socket, "127.0.0.1", 9000 + i);

        std::string message = "Performance test message " + std::to_string(i);
        std::vector<char> data(message.begin(), message.end());

        udp.sendData(socket, data.data(), data.size());
        udp.closeSocket(socket);
        udp.shutdown();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numIterations << " network operations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 200ms for 100 operations)
    EXPECT_LT(duration.count(), 200000);
}

/**
 * @brief Test network memory management
 */
TEST_F(NetworkSystemsTest, MemoryManagement) {
    size_t initialMemory = memoryPool->totalAllocated();

    // Create multiple network systems to test memory usage
    std::vector<std::unique_ptr<UDPNetworking>> udpSystems;
    std::vector<std::unique_ptr<AdvancedNetworking>> advancedSystems;

    for (int i = 0; i < 25; ++i) {
        auto udp = std::make_unique<UDPNetworking>();
        udp->initialize();

        SocketID socket = udp->createSocket();
        udp->bindSocket(socket, "127.0.0.1", 10000 + i);

        udpSystems.push_back(std::move(udp));

        auto advanced = std::make_unique<AdvancedNetworking>();
        advanced->initialize();

        ConnectionID conn = advanced->createConnection("127.0.0.1", 11000 + i, ConnectionType::TCP);
        advanced->setConnectionBufferSize(conn, 8192);

        advancedSystems.push_back(std::move(advanced));
    }

    size_t afterAllocationMemory = memoryPool->totalAllocated();
    EXPECT_GT(afterAllocationMemory, initialMemory);

    // Test memory utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    // Clean up
    udpSystems.clear();
    advancedSystems.clear();
}

/**
 * @brief Test network error handling
 */
TEST_F(NetworkSystemsTest, ErrorHandling) {
    UDPNetworking udp;

    // Test invalid operations
    EXPECT_NO_THROW(udp.sendData(99999, nullptr, 0)); // Invalid socket should handle gracefully
    EXPECT_NO_THROW(udp.receiveData(99999, nullptr, 0)); // Invalid socket should handle gracefully

    // Test uninitialized operations
    EXPECT_FALSE(udp.isInitialized());
    EXPECT_NO_THROW(udp.shutdown()); // Should handle multiple shutdowns

    // Test connection error handling
    AdvancedNetworking net;
    EXPECT_NO_THROW(net.queueMessage(99999, nullptr, 0, MessagePriority::Normal)); // Invalid connection should handle gracefully
    EXPECT_NO_THROW(net.setConnectionTimeout(99999, 5000)); // Invalid connection should handle gracefully
}

/**
 * @brief Test network concurrent operations
 */
TEST_F(NetworkSystemsTest, ConcurrentOperations) {
    const int numThreads = 4;
    const int operationsPerThread = 25;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads performing network operations
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&successCount, t]() {
            UDPNetworking udp;
            udp.initialize();

            for (int i = 0; i < operationsPerThread; ++i) {
                SocketID socket = udp.createSocket();
                if (socket > 0) {
                    udp.bindSocket(socket, "127.0.0.1", 12000 + t * 100 + i);

                    std::string message = "Thread " + std::to_string(t) + " Message " + std::to_string(i);
                    std::vector<char> data(message.begin(), message.end());

                    int sent = udp.sendData(socket, data.data(), data.size());
                    udp.closeSocket(socket);

                    if (sent > 0) {
                        successCount++;
                    }
                }
            }

            udp.shutdown();
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify concurrent operations worked
    EXPECT_EQ(successCount.load(), numThreads * operationsPerThread);

    // Memory pool should still be in valid state
    float utilization = memoryPool->utilization();
    EXPECT_GE(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);
}

/**
 * @brief Test network reliability features
 */
TEST_F(NetworkSystemsTest, ReliabilityFeatures) {
    AdvancedNetworking net;
    net.initialize();

    ConnectionID connection = net.createConnection("127.0.0.1", 13000, ConnectionType::TCP);
    net.enableReliableMessaging(connection, true);

    // Test message acknowledgment
    MessageID msg1 = net.queueMessage(connection, "Reliable message 1", 18, MessagePriority::High);
    MessageID msg2 = net.queueMessage(connection, "Reliable message 2", 18, MessagePriority::Normal);

    // Test message retransmission
    net.setRetransmissionTimeout(connection, 1000); // 1 second
    EXPECT_EQ(net.getRetransmissionTimeout(connection), 1000);

    net.setMaxRetransmissions(connection, 5);
    EXPECT_EQ(net.getMaxRetransmissions(connection), 5);

    // Test flow control
    net.enableFlowControl(connection, true);
    EXPECT_TRUE(net.isFlowControlEnabled(connection));

    net.setFlowControlWindow(connection, 1024);
    EXPECT_EQ(net.getFlowControlWindow(connection), 1024);

    // Test congestion control
    net.enableCongestionControl(connection, true);
    EXPECT_TRUE(net.isCongestionControlEnabled(connection));

    net.setCongestionAlgorithm(connection, CongestionAlgorithm::Cubic);
    EXPECT_EQ(net.getCongestionAlgorithm(connection), CongestionAlgorithm::Cubic);

    // Test message ordering
    net.enableMessageOrdering(connection, true);
    EXPECT_TRUE(net.isMessageOrderingEnabled(connection));

    net.setOrderingWindow(connection, 64);
    EXPECT_EQ(net.getOrderingWindow(connection), 64);

    net.destroyConnection(connection);
    net.shutdown();
}

/**
 * @brief Test network security features
 */
TEST_F(NetworkSystemsTest, SecurityFeatures) {
    AdvancedNetworking net;
    net.initialize();

    ConnectionID connection = net.createConnection("127.0.0.1", 14000, ConnectionType::TCP);

    // Test encryption
    net.enableEncryption(connection, true);
    EXPECT_TRUE(net.isEncryptionEnabled(connection));

    net.setEncryptionAlgorithm(connection, EncryptionAlgorithm::AES256);
    EXPECT_EQ(net.getEncryptionAlgorithm(connection), EncryptionAlgorithm::AES256);

    // Test authentication
    net.enableAuthentication(connection, true);
    EXPECT_TRUE(net.isAuthenticationEnabled(connection));

    net.setAuthenticationMethod(connection, AuthenticationMethod::HMAC);
    EXPECT_EQ(net.getAuthenticationMethod(connection), AuthenticationMethod::HMAC);

    // Test access control
    net.addAllowedIP(connection, "192.168.1.100");
    net.addBlockedIP(connection, "10.0.0.50");

    EXPECT_TRUE(net.isIPAllowed(connection, "192.168.1.100"));
    EXPECT_TRUE(net.isIPBlocked(connection, "10.0.0.50"));

    // Test rate limiting
    net.enableRateLimiting(connection, true);
    EXPECT_TRUE(net.isRateLimitingEnabled(connection));

    net.setRateLimit(connection, 1000); // 1000 packets per second
    EXPECT_EQ(net.getRateLimit(connection), 1000);

    // Test DDoS protection
    net.enableDDoSProtection(true);
    EXPECT_TRUE(net.isDDoSProtectionEnabled());

    net.setDDoSThreshold(10000); // 10k packets per second
    EXPECT_EQ(net.getDDoSThreshold(), 10000);

    net.destroyConnection(connection);
    net.shutdown();
}

/**
 * @brief Test network protocol handling
 */
TEST_F(NetworkSystemsTest, ProtocolHandling) {
    AdvancedNetworking net;
    net.initialize();

    ConnectionID tcpConnection = net.createConnection("127.0.0.1", 15000, ConnectionType::TCP);
    ConnectionID udpConnection = net.createConnection("127.0.0.1", 15001, ConnectionType::UDP);

    // Test TCP features
    net.enableKeepAlive(tcpConnection, true);
    EXPECT_TRUE(net.isKeepAliveEnabled(tcpConnection));

    net.setKeepAliveInterval(tcpConnection, 30000); // 30 seconds
    EXPECT_EQ(net.getKeepAliveInterval(tcpConnection), 30000);

    net.enableNagleAlgorithm(tcpConnection, false);
    EXPECT_FALSE(net.isNagleAlgorithmEnabled(tcpConnection));

    // Test UDP features
    net.enableReliableUDP(udpConnection, true);
    EXPECT_TRUE(net.isReliableUDPEnabled(udpConnection));

    net.setUDPRetransmissionTimeout(udpConnection, 500);
    EXPECT_EQ(net.getUDPRetransmissionTimeout(udpConnection), 500);

    // Test protocol switching
    net.switchToProtocol(tcpConnection, NetworkProtocol::WebSocket);
    EXPECT_EQ(net.getCurrentProtocol(tcpConnection), NetworkProtocol::WebSocket);

    // Test message fragmentation
    net.enableMessageFragmentation(true);
    EXPECT_TRUE(net.isMessageFragmentationEnabled());

    net.setFragmentSize(1024); // 1KB fragments
    EXPECT_EQ(net.getFragmentSize(), 1024);

    net.destroyConnection(udpConnection);
    net.destroyConnection(tcpConnection);
    net.shutdown();
}

/**
 * @brief Test network monitoring and diagnostics
 */
TEST_F(NetworkSystemsTest, MonitoringAndDiagnostics) {
    AdvancedNetworking net;
    net.initialize();

    ConnectionID connection = net.createConnection("127.0.0.1", 16000, ConnectionType::TCP);

    // Test connection monitoring
    net.enableConnectionMonitoring(true);
    EXPECT_TRUE(net.isConnectionMonitoringEnabled());

    net.setMonitoringInterval(1000); // 1 second
    EXPECT_EQ(net.getMonitoringInterval(), 1000);

    // Test bandwidth monitoring
    net.enableBandwidthMonitoring(true);
    EXPECT_TRUE(net.isBandwidthMonitoringEnabled());

    NetworkStats stats = net.getConnectionStats(connection);
    EXPECT_GE(stats.bandwidthUp, 0.0f);
    EXPECT_GE(stats.bandwidthDown, 0.0f);

    // Test latency monitoring
    net.enableLatencyMonitoring(true);
    EXPECT_TRUE(net.isLatencyMonitoringEnabled());

    float avgLatency = net.getAverageLatency(connection);
    float maxLatency = net.getMaxLatency(connection);
    float minLatency = net.getMinLatency(connection);

    EXPECT_GE(avgLatency, 0.0f);
    EXPECT_GE(maxLatency, minLatency);

    // Test packet monitoring
    net.enablePacketMonitoring(true);
    EXPECT_TRUE(net.isPacketMonitoringEnabled());

    uint32_t packetsSent = net.getPacketsSent(connection);
    uint32_t packetsReceived = net.getPacketsReceived(connection);
    uint32_t packetsLost = net.getPacketsLost(connection);

    EXPECT_GE(packetsSent, 0);
    EXPECT_GE(packetsReceived, 0);
    EXPECT_GE(packetsLost, 0);

    // Test diagnostic logging
    net.enableDiagnosticLogging(true);
    EXPECT_TRUE(net.isDiagnosticLoggingEnabled());

    net.setLogLevel(LogLevel::Info);
    EXPECT_EQ(net.getLogLevel(), LogLevel::Info);

    net.destroyConnection(connection);
    net.shutdown();
}

} // namespace Tests
} // namespace FoundryEngine
