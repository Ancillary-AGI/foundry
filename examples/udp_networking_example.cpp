/**
 * @file udp_networking_example.cpp
 * @brief Example demonstrating UDP networking usage in Foundry
 */

#include "GameEngine/networking/UDPNetworking.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

// Example game state
struct PlayerState {
    float x, y, z;
    float rotation;
    int health;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data(20); // 4*4 + 4 = 20 bytes
        memcpy(&data[0], &x, 4);
        memcpy(&data[4], &y, 4);
        memcpy(&data[8], &z, 4);
        memcpy(&data[12], &rotation, 4);
        memcpy(&data[16], &health, 4);
        return data;
    }

    void deserialize(const std::vector<uint8_t>& data) {
        if (data.size() >= 20) {
            memcpy(&x, &data[0], 4);
            memcpy(&y, &data[4], 4);
            memcpy(&z, &data[8], 4);
            memcpy(&rotation, &data[12], 4);
            memcpy(&health, &data[16], 4);
        }
    }
};

class UDPNetworkingExample {
private:
    Foundry::UDPNetworking* networking;
    std::shared_ptr<Foundry::UDPConnection> connection;
    PlayerState localPlayer;
    bool running;

public:
    UDPNetworkingExample() : networking(nullptr), running(false) {
        localPlayer = {0.0f, 0.0f, 0.0f, 0.0f, 100};
    }

    ~UDPNetworkingExample() {
        disconnect();
        if (networking) {
            networking->shutdown();
            Foundry::destroyUDPNetworking(networking);
        }
    }

    bool initialize() {
        // Create UDP networking instance
        networking = Foundry::createUDPNetworking();
        if (!networking) {
            std::cerr << "Failed to create UDP networking" << std::endl;
            return false;
        }

        if (!networking->initialize()) {
            std::cerr << "Failed to initialize UDP networking" << std::endl;
            return false;
        }

        std::cout << "UDP networking initialized successfully" << std::endl;
        return true;
    }

    bool connect(const std::string& serverAddress, uint16_t port) {
        if (!networking) return false;

        // Create connection
        connection = networking->createConnection();
        if (!connection) {
            std::cerr << "Failed to create connection" << std::endl;
            return false;
        }

        // Set up callbacks
        connection->setConnectCallback([this]() {
            std::cout << "✅ Connected to server!" << std::endl;
            onConnected();
        });

        connection->setDisconnectCallback([this]() {
            std::cout << "❌ Disconnected from server" << std::endl;
            running = false;
        });

        connection->setPacketCallback([this](const Foundry::UDPPacket& packet) {
            handlePacket(packet);
        });

        connection->setErrorCallback([this](const std::string& error) {
            std::cerr << "Network error: " << error << std::endl;
        });

        // Attempt connection
        if (!connection->connect(serverAddress, port)) {
            std::cerr << "Failed to initiate connection" << std::endl;
            return false;
        }

        std::cout << "🔗 Connecting to " << serverAddress << ":" << port << "..." << std::endl;
        return true;
    }

    void disconnect() {
        if (connection) {
            connection->disconnect();
            connection.reset();
        }
        running = false;
    }

    void run() {
        if (!connection) return;

        running = true;
        std::cout << "🚀 Starting game loop..." << std::endl;

        auto lastUpdate = std::chrono::steady_clock::now();
        float deltaTime = 0.016f; // ~60 FPS

        while (running) {
            auto now = std::chrono::steady_clock::now();
            deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastUpdate).count();
            lastUpdate = now;

            // Update networking
            if (networking) {
                networking->update(deltaTime);
            }

            // Simulate player movement
            updatePlayer(deltaTime);

            // Send player state (unreliable for performance)
            sendPlayerState();

            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    void sendChatMessage(const std::string& message) {
        if (!connection) return;

        Foundry::UDPPacket chatPacket;
        chatPacket.type = Foundry::UDPPacketType::Chat;
        chatPacket.timestamp = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        // Add player name + message
        std::string fullMessage = "Player: " + message;
        chatPacket.payload.assign(fullMessage.begin(), fullMessage.end());

        connection->sendPacket(chatPacket, true); // Reliable for chat
        std::cout << "💬 Sent chat: " << message << std::endl;
    }

private:
    void onConnected() {
        // Send initial player state
        sendPlayerState();

        // Send join message
        Foundry::UDPPacket joinPacket;
        joinPacket.type = Foundry::UDPPacketType::PlayerJoin;
        joinPacket.timestamp = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        std::string playerName = "ExamplePlayer";
        joinPacket.payload.assign(playerName.begin(), playerName.end());

        connection->sendPacket(joinPacket, true);
    }

    void updatePlayer(float deltaTime) {
        // Simple movement simulation
        static float time = 0.0f;
        time += deltaTime;

        // Circular movement for demo
        localPlayer.x = std::cos(time) * 5.0f;
        localPlayer.z = std::sin(time) * 5.0f;
        localPlayer.rotation = time;

        // Simulate health changes
        if (std::sin(time * 0.5f) > 0.8f) {
            localPlayer.health = std::min(100, localPlayer.health + 1);
        }
    }

    void sendPlayerState() {
        if (!connection) return;

        Foundry::UDPPacket statePacket;
        statePacket.type = Foundry::UDPPacketType::PlayerState;
        statePacket.timestamp = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        // Serialize player state
        statePacket.payload = localPlayer.serialize();

        // Send unreliably for performance (position updates don't need 100% reliability)
        connection->sendPacket(statePacket, false);
    }

    void handlePacket(const Foundry::UDPPacket& packet) {
        switch (packet.type) {
        case Foundry::UDPPacketType::PlayerJoin:
            onPlayerJoined(packet);
            break;

        case Foundry::UDPPacketType::PlayerLeave:
            onPlayerLeft(packet);
            break;

        case Foundry::UDPPacketType::PlayerState:
            onPlayerStateUpdate(packet);
            break;

        case Foundry::UDPPacketType::WorldState:
            onWorldStateUpdate(packet);
            break;

        case Foundry::UDPPacketType::Chat:
            onChatMessage(packet);
            break;

        case Foundry::UDPPacketType::Ping:
            // Pong is handled automatically by the connection
            break;

        default:
            std::cout << "📦 Received packet type: " << static_cast<int>(packet.type)
                     << " (" << packet.payload.size() << " bytes)" << std::endl;
            break;
        }
    }

    void onPlayerJoined(const Foundry::UDPPacket& packet) {
        std::string playerName(packet.payload.begin(), packet.payload.end());
        std::cout << "👤 Player joined: " << playerName << std::endl;
    }

    void onPlayerLeft(const Foundry::UDPPacket& packet) {
        std::cout << "👋 Player left (ID: " << packet.playerID << ")" << std::endl;
    }

    void onPlayerStateUpdate(const Foundry::UDPPacket& packet) {
        if (packet.payload.size() >= 20) {
            PlayerState remotePlayer;
            remotePlayer.deserialize(packet.payload);

            // In a real game, you'd update the remote player's position
            std::cout << "📍 Player " << packet.playerID << " at ("
                     << remotePlayer.x << ", " << remotePlayer.y << ", " << remotePlayer.z << ")" << std::endl;
        }
    }

    void onWorldStateUpdate(const Foundry::UDPPacket& packet) {
        std::cout << "🌍 World state update (" << packet.payload.size() << " bytes)" << std::endl;
    }

    void onChatMessage(const Foundry::UDPPacket& packet) {
        std::string message(packet.payload.begin(), packet.payload.end());
        std::cout << "💬 " << message << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "🎮 Foundry UDP Networking Example" << std::endl;
    std::cout << "=================================" << std::endl;

    // Parse command line arguments
    std::string serverAddress = "127.0.0.1";
    uint16_t serverPort = 8080;

    if (argc >= 2) {
        serverAddress = argv[1];
    }
    if (argc >= 3) {
        serverPort = static_cast<uint16_t>(std::atoi(argv[2]));
    }

    // Create and initialize networking example
    UDPNetworkingExample example;

    if (!example.initialize()) {
        std::cerr << "Failed to initialize networking example" << std::endl;
        return 1;
    }

    // Connect to server
    if (!example.connect(serverAddress, serverPort)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    // Wait a bit for connection
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Send a test chat message
    example.sendChatMessage("Hello from UDP networking example!");

    // Run the main loop
    example.run();

    std::cout << "👋 Example finished" << std::endl;
    return 0;
}
