#include "../../include/GameEngine/systems/NetworkSystem.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>

namespace FoundryEngine {

// UDP Network Manager Implementation
class UDPNetworkManagerImpl : public SystemImplBase<UDPNetworkManagerImpl> {
private:
    std::unordered_map<uint32_t, std::unique_ptr<UDPConnection>> connections_;
    std::unique_ptr<UDPSocket> serverSocket_;
    std::vector<std::unique_ptr<UDPSocket>> clientSockets_;

    std::thread networkThread_;
    std::mutex networkMutex_;
    bool running_ = false;

    uint32_t nextConnectionId_ = 1;
    uint16_t serverPort_ = 0;

    friend class SystemImplBase<UDPNetworkManagerImpl>;

    bool onInitialize() override {
        std::cout << "UDP Network Manager initialized" << std::endl;
        running_ = true;

        // Start network thread for handling connections
        networkThread_ = std::thread([this]() {
            while (running_) {
                updateNetwork();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        return true;
    }

    void onShutdown() override {
        running_ = false;

        if (networkThread_.joinable()) {
            networkThread_.join();
        }

        // Clean up all connections and sockets
        connections_.clear();
        serverSocket_.reset();
        clientSockets_.clear();

        std::cout << "UDP Network Manager shutdown" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Handle network updates on main thread
        std::lock_guard<std::mutex> lock(networkMutex_);

        // Process incoming messages, update connection states, etc.
        for (auto& pair : connections_) {
            auto& connection = pair.second;
            if (connection) {
                // Update connection state
                connection->update(deltaTime);
            }
        }
    }

    void updateNetwork() {
        std::lock_guard<std::mutex> lock(networkMutex_);

        // Poll server socket for new connections
        if (serverSocket_) {
            // Accept new connections
        }

        // Poll client sockets
        for (auto& socket : clientSockets_) {
            if (socket) {
                // Handle incoming data
            }
        }
    }

public:
    UDPNetworkManagerImpl() : SystemImplBase("UDPNetworkManager") {}

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Network Stats - Connections: %zu, Server Port: %d",
                 connections_.size(), serverPort_);
        return std::string(buffer);
    }

    UDPConnection* createConnection() {
        std::lock_guard<std::mutex> lock(networkMutex_);

        auto connection = std::make_unique<UDPConnection>(nextConnectionId_++);
        UDPConnection* connectionPtr = connection.get();
        connections_[connectionPtr->getId()] = std::move(connection);
        return connectionPtr;
    }

    void destroyConnection(uint32_t connectionId) {
        std::lock_guard<std::mutex> lock(networkMutex_);
        connections_.erase(connectionId);
    }

    UDPConnection* getConnection(uint32_t connectionId) {
        std::lock_guard<std::mutex> lock(networkMutex_);
        auto it = connections_.find(connectionId);
        return (it != connections_.end()) ? it->second.get() : nullptr;
    }

    UDPSocket* createServerSocket(uint16_t port) {
        std::lock_guard<std::mutex> lock(networkMutex_);

        if (serverSocket_) {
            return nullptr; // Already have a server socket
        }

        serverSocket_ = std::make_unique<UDPSocket>();
        if (serverSocket_->bind(port)) {
            serverPort_ = port;
            return serverSocket_.get();
        }

        serverSocket_.reset();
        return nullptr;
    }

    UDPSocket* createClientSocket() {
        std::lock_guard<std::mutex> lock(networkMutex_);

        auto socket = std::make_unique<UDPSocket>();
        UDPSocket* socketPtr = socket.get();
        clientSockets_.push_back(std::move(socket));
        return socketPtr;
    }

    void destroySocket(UDPSocket* socket) {
        std::lock_guard<std::mutex> lock(networkMutex_);

        if (serverSocket_.get() == socket) {
            serverSocket_.reset();
            serverPort_ = 0;
        } else {
            clientSockets_.erase(
                std::remove_if(clientSockets_.begin(), clientSockets_.end(),
                    [socket](const std::unique_ptr<UDPSocket>& s) { return s.get() == socket; }),
                clientSockets_.end());
        }
    }

    std::vector<uint32_t> getActiveConnections() const {
        std::lock_guard<std::mutex> lock(networkMutex_);
        std::vector<uint32_t> activeConnections;
        for (const auto& pair : connections_) {
            activeConnections.push_back(pair.first);
        }
        return activeConnections;
    }

    bool isServerRunning() const {
        return serverSocket_ != nullptr;
    }

    uint16_t getServerPort() const {
        return serverPort_;
    }
};

UDPNetworkManager::UDPNetworkManager() : impl_(std::make_unique<UDPNetworkManagerImpl>()) {}
UDPNetworkManager::~UDPNetworkManager() = default;

bool UDPNetworkManager::initialize() { return impl_->initialize(); }
void UDPNetworkManager::shutdown() { impl_->shutdown(); }
void UDPNetworkManager::update(float deltaTime) { impl_->update(deltaTime); }

UDPConnection* UDPNetworkManager::createConnection() { return impl_->createConnection(); }
void UDPNetworkManager::destroyConnection(uint32_t connectionId) { impl_->destroyConnection(connectionId); }
UDPConnection* UDPNetworkManager::getConnection(uint32_t connectionId) { return impl_->getConnection(connectionId); }
UDPSocket* UDPNetworkManager::createServerSocket(uint16_t port) { return impl_->createServerSocket(port); }
UDPSocket* UDPNetworkManager::createClientSocket() { return impl_->createClientSocket(); }
void UDPNetworkManager::destroySocket(UDPSocket* socket) { impl_->destroySocket(socket); }
std::vector<uint32_t> UDPNetworkManager::getActiveConnections() const { return impl_->getActiveConnections(); }
bool UDPNetworkManager::isServerRunning() const { return impl_->isServerRunning(); }
uint16_t UDPNetworkManager::getServerPort() const { return impl_->getServerPort(); }

} // namespace FoundryEngine
