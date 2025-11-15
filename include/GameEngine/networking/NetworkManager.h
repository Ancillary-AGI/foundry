/**
 * @file NetworkManager.h
 * @brief Network management system for multiplayer functionality
 */

#pragma once

#include "../core/System.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace FoundryEngine {

/**
 * @class NetworkManager
 * @brief Cross-platform network management system
 */
class NetworkManager : public System {
public:
    NetworkManager() = default;
    virtual ~NetworkManager() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;

    // Connection management
    virtual bool connect(const std::string& host, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool isConnecting() const = 0;

    // Data transmission
    virtual bool sendData(const void* data, size_t size, bool reliable = true) = 0;
    virtual bool sendString(const std::string& message, bool reliable = true) = 0;

    // Data reception
    virtual bool hasData() const = 0;
    virtual size_t receiveData(void* buffer, size_t maxSize) = 0;
    virtual std::string receiveString() = 0;

    // Connection information
    virtual std::string getLocalAddress() const = 0;
    virtual std::string getRemoteAddress() const = 0;
    virtual int getLocalPort() const = 0;
    virtual int getRemotePort() const = 0;

    // Network statistics
    virtual uint64_t getBytesSent() const = 0;
    virtual uint64_t getBytesReceived() const = 0;
    virtual float getPing() const = 0;

    // Configuration
    virtual void setMaxPacketSize(size_t size) = 0;
    virtual void setTimeout(float seconds) = 0;
    virtual void setCompressionEnabled(bool enabled) = 0;
    virtual void setEncryptionEnabled(bool enabled) = 0;
};

/**
 * @class UDPNetworkManager
 * @brief UDP-based network manager implementation
 */
class UDPNetworkManager : public NetworkManager {
public:
    bool initialize() override { return true; }
    void shutdown() override {}
    void update(float deltaTime) override {}

    bool connect(const std::string& host, int port) override { return false; }
    void disconnect() override {}
    bool isConnected() const override { return false; }
    bool isConnecting() const override { return false; }

    bool sendData(const void* data, size_t size, bool reliable = true) override { return false; }
    bool sendString(const std::string& message, bool reliable = true) override { return false; }

    bool hasData() const override { return false; }
    size_t receiveData(void* buffer, size_t maxSize) override { return 0; }
    std::string receiveString() override { return ""; }

    std::string getLocalAddress() const override { return "127.0.0.1"; }
    std::string getRemoteAddress() const override { return ""; }
    int getLocalPort() const override { return 0; }
    int getRemotePort() const override { return 0; }

    uint64_t getBytesSent() const override { return 0; }
    uint64_t getBytesReceived() const override { return 0; }
    float getPing() const override { return -1.0f; }

    void setMaxPacketSize(size_t size) override {}
    void setTimeout(float seconds) override {}
    void setCompressionEnabled(bool enabled) override {}
    void setEncryptionEnabled(bool enabled) override {}
};

} // namespace FoundryEngine
