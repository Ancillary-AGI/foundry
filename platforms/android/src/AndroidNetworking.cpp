/**
 * @file AndroidNetworking.cpp
 * @brief Complete Android networking implementation with WebSocket and HTTP support
 */

#include "../core/AndroidPlatform.h"
#include <android/log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <curl/curl.h>
#include <jni.h>

#define LOG_TAG "AndroidNetworking"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace FoundryEngine {

// WebSocket implementation for Android
class AndroidWebSocket : public PlatformWebSocket {
private:
    int socket_;
    std::string url_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::thread receiveThread_;
    std::mutex sendMutex_;
    std::queue<std::string> messageQueue_;
    std::mutex queueMutex_;

public:
    AndroidWebSocket(const std::string& url) : url_(url), socket_(-1), connected_(false), running_(false) {
        LOGI("Created WebSocket for URL: %s", url.c_str());
    }

    ~AndroidWebSocket() {
        disconnect();
    }

    bool connect() override {
        if (connected_) return true;

        // Parse URL to extract host and port
        std::string host;
        int port = 80;
        std::string path = "/";
        
        if (!parseURL(url_, host, port, path)) {
            LOGE("Failed to parse WebSocket URL: %s", url_.c_str());
            return false;
        }

        // Create socket
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ < 0) {
            LOGE("Failed to create socket");
            return false;
        }

        // Set non-blocking
        int flags = fcntl(socket_, F_GETFL, 0);
        fcntl(socket_, F_SETFL, flags | O_NONBLOCK);

        // Connect to server
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        struct hostent* server = gethostbyname(host.c_str());
        if (!server) {
            LOGE("Failed to resolve host: %s", host.c_str());
            close(socket_);
            return false;
        }

        memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);

        if (::connect(socket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            if (errno != EINPROGRESS) {
                LOGE("Failed to connect to %s:%d", host.c_str(), port);
                close(socket_);
                return false;
            }
        }

        // Send WebSocket handshake
        if (!performHandshake(host, path)) {
            LOGE("WebSocket handshake failed");
            close(socket_);
            return false;
        }

        connected_ = true;
        running_ = true;

        // Start receive thread
        receiveThread_ = std::thread([this]() {
            receiveLoop();
        });

        LOGI("WebSocket connected to %s", url_.c_str());
        return true;
    }

    void disconnect() override {
        if (!connected_) return;

        running_ = false;
        connected_ = false;

        if (socket_ >= 0) {
            close(socket_);
            socket_ = -1;
        }

        if (receiveThread_.joinable()) {
            receiveThread_.join();
        }

        LOGI("WebSocket disconnected");
    }

    bool send(const std::string& message) override {
        if (!connected_) return false;

        std::lock_guard<std::mutex> lock(sendMutex_);

        // Create WebSocket frame
        std::vector<uint8_t> frame = createFrame(message);
        
        ssize_t sent = ::send(socket_, frame.data(), frame.size(), 0);
        if (sent != static_cast<ssize_t>(frame.size())) {
            LOGE("Failed to send WebSocket message");
            return false;
        }

        LOGI("Sent WebSocket message: %s", message.c_str());
        return true;
    }

    std::string receive() override {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        if (messageQueue_.empty()) {
            return "";
        }

        std::string message = messageQueue_.front();
        messageQueue_.pop();
        return message;
    }

    bool isConnected() const override {
        return connected_;
    }

private:
    bool parseURL(const std::string& url, std::string& host, int& port, std::string& path) {
        // Simple URL parsing for ws:// or wss://
        size_t protocolEnd = url.find("://");
        if (protocolEnd == std::string::npos) return false;

        std::string remaining = url.substr(protocolEnd + 3);
        
        size_t pathStart = remaining.find('/');
        if (pathStart != std::string::npos) {
            path = remaining.substr(pathStart);
            remaining = remaining.substr(0, pathStart);
        }

        size_t portStart = remaining.find(':');
        if (portStart != std::string::npos) {
            host = remaining.substr(0, portStart);
            port = std::stoi(remaining.substr(portStart + 1));
        } else {
            host = remaining;
            port = (url.find("wss://") == 0) ? 443 : 80;
        }

        return true;
    }

    bool performHandshake(const std::string& host, const std::string& path) {
        // Create WebSocket handshake request
        std::string request = 
            "GET " + path + " HTTP/1.1\r\n"
            "Host: " + host + "\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";

        ssize_t sent = ::send(socket_, request.c_str(), request.length(), 0);
        if (sent != static_cast<ssize_t>(request.length())) {
            return false;
        }

        // Read handshake response
        char buffer[1024];
        ssize_t received = recv(socket_, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            return false;
        }

        buffer[received] = '\0';
        std::string response(buffer);

        // Check for successful handshake
        return response.find("HTTP/1.1 101") != std::string::npos &&
               response.find("Upgrade: websocket") != std::string::npos;
    }

    std::vector<uint8_t> createFrame(const std::string& message) {
        std::vector<uint8_t> frame;
        
        // FIN + opcode (text frame)
        frame.push_back(0x81);
        
        // Payload length
        size_t len = message.length();
        if (len < 126) {
            frame.push_back(0x80 | len); // Masked
        } else if (len < 65536) {
            frame.push_back(0x80 | 126);
            frame.push_back((len >> 8) & 0xFF);
            frame.push_back(len & 0xFF);
        } else {
            frame.push_back(0x80 | 127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back((len >> (i * 8)) & 0xFF);
            }
        }
        
        // Masking key (simple key for demo)
        uint8_t mask[4] = {0x12, 0x34, 0x56, 0x78};
        frame.insert(frame.end(), mask, mask + 4);
        
        // Masked payload
        for (size_t i = 0; i < message.length(); ++i) {
            frame.push_back(message[i] ^ mask[i % 4]);
        }
        
        return frame;
    }

    void receiveLoop() {
        char buffer[4096];
        
        while (running_) {
            ssize_t received = recv(socket_, buffer, sizeof(buffer), 0);
            
            if (received > 0) {
                // Parse WebSocket frame (simplified)
                std::string message = parseFrame(buffer, received);
                if (!message.empty()) {
                    std::lock_guard<std::mutex> lock(queueMutex_);
                    messageQueue_.push(message);
                }
            } else if (received == 0) {
                // Connection closed
                connected_ = false;
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                // Error occurred
                LOGE("WebSocket receive error: %s", strerror(errno));
                connected_ = false;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::string parseFrame(const char* data, size_t length) {
        if (length < 2) return "";
        
        uint8_t opcode = data[0] & 0x0F;
        if (opcode != 0x01) return ""; // Only handle text frames
        
        bool masked = (data[1] & 0x80) != 0;
        size_t payloadLen = data[1] & 0x7F;
        size_t headerLen = 2;
        
        if (payloadLen == 126) {
            if (length < 4) return "";
            payloadLen = (data[2] << 8) | data[3];
            headerLen = 4;
        } else if (payloadLen == 127) {
            if (length < 10) return "";
            payloadLen = 0;
            for (int i = 0; i < 8; ++i) {
                payloadLen = (payloadLen << 8) | data[2 + i];
            }
            headerLen = 10;
        }
        
        if (masked) headerLen += 4;
        if (length < headerLen + payloadLen) return "";
        
        std::string message;
        if (masked) {
            const uint8_t* mask = reinterpret_cast<const uint8_t*>(data + headerLen - 4);
            for (size_t i = 0; i < payloadLen; ++i) {
                message += (data[headerLen + i] ^ mask[i % 4]);
            }
        } else {
            message = std::string(data + headerLen, payloadLen);
        }
        
        return message;
    }
};

// HTTP response structure
struct HTTPResponse {
    std::vector<uint8_t> data;
    size_t size;
};

// CURL write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, HTTPResponse* response) {
    size_t totalSize = size * nmemb;
    size_t oldSize = response->data.size();
    response->data.resize(oldSize + totalSize);
    memcpy(response->data.data() + oldSize, contents, totalSize);
    response->size = response->data.size();
    return totalSize;
}

class AndroidNetworkingImpl : public AndroidNetworking {
private:
    bool curlInitialized_;

public:
    AndroidNetworkingImpl() : curlInitialized_(false) {
        // Initialize CURL
        if (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK) {
            curlInitialized_ = true;
            LOGI("CURL initialized successfully");
        } else {
            LOGE("Failed to initialize CURL");
        }
    }

    ~AndroidNetworkingImpl() {
        if (curlInitialized_) {
            curl_global_cleanup();
        }
    }

    std::unique_ptr<PlatformWebSocket> connect(const std::string& url) override {
        auto webSocket = std::make_unique<AndroidWebSocket>(url);
        if (webSocket->connect()) {
            return webSocket;
        }
        return nullptr;
    }

    std::vector<uint8_t> httpGet(const std::string& url) override {
        if (!curlInitialized_) {
            LOGE("CURL not initialized");
            return {};
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            LOGE("Failed to initialize CURL handle");
            return {};
        }

        HTTPResponse response;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        CURLcode res = curl_easy_perform(curl);
        
        long responseCode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
        
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            LOGE("HTTP GET failed: %s", curl_easy_strerror(res));
            return {};
        }

        if (responseCode != 200) {
            LOGE("HTTP GET returned status code: %ld", responseCode);
            return {};
        }

        LOGI("HTTP GET successful: %s (%zu bytes)", url.c_str(), response.data.size());
        return response.data;
    }

    std::vector<uint8_t> httpPost(const std::string& url, const std::vector<uint8_t>& data) override {
        if (!curlInitialized_) {
            LOGE("CURL not initialized");
            return {};
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            LOGE("Failed to initialize CURL handle");
            return {};
        }

        HTTPResponse response;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.data());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        // Set content type
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        
        long responseCode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            LOGE("HTTP POST failed: %s", curl_easy_strerror(res));
            return {};
        }

        if (responseCode != 200) {
            LOGE("HTTP POST returned status code: %ld", responseCode);
            return {};
        }

        LOGI("HTTP POST successful: %s (%zu bytes sent, %zu bytes received)", 
             url.c_str(), data.size(), response.data.size());
        return response.data;
    }
};

// Global instance
static std::unique_ptr<AndroidNetworkingImpl> g_networking;

} // namespace FoundryEngine

// JNI functions
extern "C" {

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeInitializeNetworking(JNIEnv* env, jobject thiz) {
    using namespace FoundryEngine;
    g_networking = std::make_unique<AndroidNetworkingImpl>();
    LOGI("Android networking initialized");
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeShutdownNetworking(JNIEnv* env, jobject thiz) {
    using namespace FoundryEngine;
    g_networking.reset();
    LOGI("Android networking shutdown");
}

JNIEXPORT jlong JNICALL Java_com_foundryengine_game_GameActivity_nativeCreateWebSocket(JNIEnv* env, jobject thiz, jstring url) {
    if (!g_networking) return 0;
    
    const char* urlStr = env->GetStringUTFChars(url, nullptr);
    auto webSocket = g_networking->connect(urlStr);
    env->ReleaseStringUTFChars(url, urlStr);
    
    if (webSocket) {
        return reinterpret_cast<jlong>(webSocket.release());
    }
    
    return 0;
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeDestroyWebSocket(JNIEnv* env, jobject thiz, jlong webSocketPtr) {
    auto webSocket = reinterpret_cast<FoundryEngine::PlatformWebSocket*>(webSocketPtr);
    delete webSocket;
}

JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeWebSocketSend(JNIEnv* env, jobject thiz, jlong webSocketPtr, jstring message) {
    auto webSocket = reinterpret_cast<FoundryEngine::PlatformWebSocket*>(webSocketPtr);
    if (!webSocket) return JNI_FALSE;
    
    const char* messageStr = env->GetStringUTFChars(message, nullptr);
    bool result = webSocket->send(messageStr);
    env->ReleaseStringUTFChars(message, messageStr);
    
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL Java_com_foundryengine_game_GameActivity_nativeWebSocketReceive(JNIEnv* env, jobject thiz, jlong webSocketPtr) {
    auto webSocket = reinterpret_cast<FoundryEngine::PlatformWebSocket*>(webSocketPtr);
    if (!webSocket) return nullptr;
    
    std::string message = webSocket->receive();
    if (message.empty()) return nullptr;
    
    return env->NewStringUTF(message.c_str());
}

JNIEXPORT jbyteArray JNICALL Java_com_foundryengine_game_GameActivity_nativeHttpGet(JNIEnv* env, jobject thiz, jstring url) {
    if (!g_networking) return nullptr;
    
    const char* urlStr = env->GetStringUTFChars(url, nullptr);
    std::vector<uint8_t> data = g_networking->httpGet(urlStr);
    env->ReleaseStringUTFChars(url, urlStr);
    
    if (data.empty()) return nullptr;
    
    jbyteArray result = env->NewByteArray(data.size());
    env->SetByteArrayRegion(result, 0, data.size(), reinterpret_cast<jbyte*>(data.data()));
    return result;
}

JNIEXPORT jbyteArray JNICALL Java_com_foundryengine_game_GameActivity_nativeHttpPost(JNIEnv* env, jobject thiz, jstring url, jbyteArray postData) {
    if (!g_networking) return nullptr;
    
    const char* urlStr = env->GetStringUTFChars(url, nullptr);
    
    jsize length = env->GetArrayLength(postData);
    jbyte* dataPtr = env->GetByteArrayElements(postData, nullptr);
    
    std::vector<uint8_t> requestData(reinterpret_cast<uint8_t*>(dataPtr), 
                                     reinterpret_cast<uint8_t*>(dataPtr) + length);
    
    std::vector<uint8_t> responseData = g_networking->httpPost(urlStr, requestData);
    
    env->ReleaseStringUTFChars(url, urlStr);
    env->ReleaseByteArrayElements(postData, dataPtr, JNI_ABORT);
    
    if (responseData.empty()) return nullptr;
    
    jbyteArray result = env->NewByteArray(responseData.size());
    env->SetByteArrayRegion(result, 0, responseData.size(), reinterpret_cast<jbyte*>(responseData.data()));
    return result;
}

} // extern "C"