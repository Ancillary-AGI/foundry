/**
 * Android Network Platform Implementation
 * Networking capabilities for Android including WiFi, cellular, and Bluetooth
 */

#include "AndroidPlatform.h"
#include <jni.h>
#include <android/log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <memory>

// Network implementation for Android
class AndroidNetwork {
private:
    std::unordered_map<int, std::unique_ptr<std::thread>> activeConnections_;
    std::mutex connectionsMutex_;
    bool initialized_ = false;

    struct ConnectionInfo {
        int socketFd;
        std::string remoteAddress;
        int remotePort;
        bool isConnected;
        std::function<void(const std::vector<uint8_t>&)> dataCallback;
        std::function<void()> disconnectCallback;
    };

    std::unordered_map<int, ConnectionInfo> connections_;

public:
    AndroidNetwork() = default;
    ~AndroidNetwork() { shutdown(); }

    bool initialize() {
        if (initialized_) return true;

        // Initialize network stack
        // On Android, basic socket operations work the same as Linux
        initialized_ = true;
        return true;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(connectionsMutex_);

        // Close all connections
        for (auto& pair : connections_) {
            if (pair.second.socketFd >= 0) {
                close(pair.second.socketFd);
            }
        }

        connections_.clear();

        // Wait for all threads to finish
        for (auto& pair : activeConnections_) {
            if (pair.second && pair.second->joinable()) {
                pair.second->join();
            }
        }

        activeConnections_.clear();
        initialized_ = false;
    }

    // TCP connection management
    int connect(const std::string& address, int port,
                std::function<void(const std::vector<uint8_t>&)> dataCallback,
                std::function<void()> disconnectCallback) {

        int socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFd < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "AndroidNetwork", "Failed to create socket: %s", strerror(errno));
            return -1;
        }

        // Set non-blocking
        int flags = fcntl(socketFd, F_GETFL, 0);
        fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        if (inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr) <= 0) {
            __android_log_print(ANDROID_LOG_ERROR, "AndroidNetwork", "Invalid address: %s", address.c_str());
            close(socketFd);
            return -1;
        }

        int result = ::connect(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (result < 0 && errno != EINPROGRESS) {
            __android_log_print(ANDROID_LOG_ERROR, "AndroidNetwork", "Connect failed: %s", strerror(errno));
            close(socketFd);
            return -1;
        }

        // Store connection info
        int connectionId = socketFd; // Use socket fd as connection id
        ConnectionInfo info;
        info.socketFd = socketFd;
        info.remoteAddress = address;
        info.remotePort = port;
        info.isConnected = (result == 0);
        info.dataCallback = dataCallback;
        info.disconnectCallback = disconnectCallback;

        {
            std::lock_guard<std::mutex> lock(connectionsMutex_);
            connections_[connectionId] = std::move(info);
        }

        // Start connection thread
        activeConnections_[connectionId] = std::make_unique<std::thread>(
            &AndroidNetwork::connectionThread, this, connectionId);

        return connectionId;
    }

    void disconnect(int connectionId) {
        std::lock_guard<std::mutex> lock(connectionsMutex_);

        auto it = connections_.find(connectionId);
        if (it != connections_.end()) {
            if (it->second.socketFd >= 0) {
                close(it->second.socketFd);
            }

            // Call disconnect callback
            if (it->second.disconnectCallback) {
                it->second.disconnectCallback();
            }

            connections_.erase(it);
        }

        // Thread will clean itself up
        auto threadIt = activeConnections_.find(connectionId);
        if (threadIt != activeConnections_.end()) {
            activeConnections_.erase(threadIt);
        }
    }

    bool sendData(int connectionId, const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(connectionsMutex_);

        auto it = connections_.find(connectionId);
        if (it == connections_.end() || !it->second.isConnected) {
            return false;
        }

        ssize_t sent = send(it->second.socketFd, data.data(), data.size(), 0);
        if (sent < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "AndroidNetwork", "Send failed: %s", strerror(errno));
            return false;
        }

        return true;
    }

    bool isConnected(int connectionId) {
        std::lock_guard<std::mutex> lock(connectionsMutex_);

        auto it = connections_.find(connectionId);
        return it != connections_.end() && it->second.isConnected;
    }

    // HTTP operations
    std::vector<uint8_t> httpGet(const std::string& url) {
        // Simplified HTTP GET implementation
        // In a real implementation, you'd use libcurl or similar
        __android_log_print(ANDROID_LOG_INFO, "AndroidNetwork", "HTTP GET not implemented: %s", url.c_str());
        return {};
    }

    std::vector<uint8_t> httpPost(const std::string& url, const std::vector<uint8_t>& data) {
        // Simplified HTTP POST implementation
        __android_log_print(ANDROID_LOG_INFO, "AndroidNetwork", "HTTP POST not implemented: %s", url.c_str());
        return {};
    }

private:
    void connectionThread(int connectionId) {
        while (true) {
            std::lock_guard<std::mutex> lock(connectionsMutex_);

            auto it = connections_.find(connectionId);
            if (it == connections_.end()) {
                break; // Connection closed
            }

            ConnectionInfo& info = it->second;

            // Check if connection is established (for non-blocking connect)
            if (!info.isConnected) {
                struct sockaddr_in addr;
                socklen_t addrLen = sizeof(addr);
                if (getpeername(info.socketFd, (struct sockaddr*)&addr, &addrLen) == 0) {
                    info.isConnected = true;
                    __android_log_print(ANDROID_LOG_INFO, "AndroidNetwork", "Connected to %s:%d",
                                      info.remoteAddress.c_str(), info.remotePort);
                } else if (errno != ENOTCONN && errno != EINPROGRESS) {
                    __android_log_print(ANDROID_LOG_ERROR, "AndroidNetwork", "Connection failed: %s", strerror(errno));
                    break;
                }
            }

            if (info.isConnected) {
                // Try to receive data
                const int bufferSize = 4096;
                uint8_t buffer[bufferSize];

                ssize_t received = recv(info.socketFd, buffer, bufferSize, 0);
                if (received > 0) {
                    std::vector<uint8_t> data(buffer, buffer + received);
                    if (info.dataCallback) {
                        // Note: In a real implementation, you'd need to handle thread safety
                        // for calling callbacks from background threads
                        info.dataCallback(data);
                    }
                } else if (received == 0) {
                    // Connection closed by peer
                    __android_log_print(ANDROID_LOG_INFO, "AndroidNetwork", "Connection closed by peer");
                    break;
                } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    __android_log_print(ANDROID_LOG_ERROR, "AndroidNetwork", "Receive error: %s", strerror(errno));
                    break;
                }
            }

            // Unlock for sleep
            connectionsMutex_.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            connectionsMutex_.lock();
        }

        // Cleanup
        {
            std::lock_guard<std::mutex> lock(connectionsMutex_);
            auto it = connections_.find(connectionId);
            if (it != connections_.end()) {
                if (it->second.disconnectCallback) {
                    it->second.disconnectCallback();
                }
                connections_.erase(it);
            }
        }
    }
};

// Network API functions that can be called from Java
extern "C" {
    JNIEXPORT jlong JNICALL Java_com_foundryengine_game_GameActivity_nativeCreateNetwork(JNIEnv* env, jobject thiz) {
        AndroidNetwork* network = new AndroidNetwork();
        if (network->initialize()) {
            return reinterpret_cast<jlong>(network);
        } else {
            delete network;
            return 0;
        }
    }

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeDestroyNetwork(JNIEnv* env, jobject thiz, jlong networkPtr) {
        AndroidNetwork* network = reinterpret_cast<AndroidNetwork*>(networkPtr);
        if (network) {
            delete network;
        }
    }

    JNIEXPORT jint JNICALL Java_com_foundryengine_game_GameActivity_nativeConnect(JNIEnv* env, jobject thiz, jlong networkPtr, jstring address, jint port) {
        AndroidNetwork* network = reinterpret_cast<AndroidNetwork*>(networkPtr);
        if (!network) return -1;

        const char* addressStr = env->GetStringUTFChars(address, nullptr);
        std::string addr(addressStr);
        env->ReleaseStringUTFChars(address, addressStr);

        // Simplified connection - no callbacks in this basic implementation
        return network->connect(addr, port, nullptr, nullptr);
    }

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeDisconnect(JNIEnv* env, jobject thiz, jlong networkPtr, jint connectionId) {
        AndroidNetwork* network = reinterpret_cast<AndroidNetwork*>(networkPtr);
        if (network) {
            network->disconnect(connectionId);
        }
    }

    JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeSendData(JNIEnv* env, jobject thiz, jlong networkPtr, jint connectionId, jbyteArray data) {
        AndroidNetwork* network = reinterpret_cast<AndroidNetwork*>(networkPtr);
        if (!network) return JNI_FALSE;

        jsize length = env->GetArrayLength(data);
        jbyte* dataPtr = env->GetByteArrayElements(data, nullptr);

        std::vector<uint8_t> dataVec((uint8_t*)dataPtr, (uint8_t*)dataPtr + length);
        bool result = network->sendData(connectionId, dataVec);

        env->ReleaseByteArrayElements(data, dataPtr, JNI_ABORT);

        return result ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeIsConnected(JNIEnv* env, jobject thiz, jlong networkPtr, jint connectionId) {
        AndroidNetwork* network = reinterpret_cast<AndroidNetwork*>(networkPtr);
        return network && network->isConnected(connectionId) ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jbyteArray JNICALL Java_com_foundryengine_game_GameActivity_nativeHttpGet(JNIEnv* env, jobject thiz, jlong networkPtr, jstring url) {
        AndroidNetwork* network = reinterpret_cast<AndroidNetwork*>(networkPtr);
        if (!network) return nullptr;

        const char* urlStr = env->GetStringUTFChars(url, nullptr);
        std::string urlString(urlStr);
        env->ReleaseStringUTFChars(url, urlStr);

        auto data = network->httpGet(urlString);
        if (data.empty()) return nullptr;

        jbyteArray result = env->NewByteArray(data.size());
        env->SetByteArrayRegion(result, 0, data.size(), (jbyte*)data.data());
        return result;
    }

    JNIEXPORT jbyteArray JNICALL Java_com_foundryengine_game_GameActivity_nativeHttpPost(JNIEnv* env, jobject thiz, jlong networkPtr, jstring url, jbyteArray data) {
        AndroidNetwork* network = reinterpret_cast<AndroidNetwork*>(networkPtr);
        if (!network) return nullptr;

        const char* urlStr = env->GetStringUTFChars(url, nullptr);
        std::string urlString(urlStr);
        env->ReleaseStringUTFChars(url, urlStr);

        jsize length = env->GetArrayLength(data);
        jbyte* dataPtr = env->GetByteArrayElements(data, nullptr);

        std::vector<uint8_t> dataVec((uint8_t*)dataPtr, (uint8_t*)dataPtr + length);
        auto response = network->httpPost(urlString, dataVec);

        env->ReleaseByteArrayElements(data, dataPtr, JNI_ABORT);

        if (response.empty()) return nullptr;

        jbyteArray result = env->NewByteArray(response.size());
        env->SetByteArrayRegion(result, 0, response.size(), (jbyte*)response.data());
        return result;
    }
}
