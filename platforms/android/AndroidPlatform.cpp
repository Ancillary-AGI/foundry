/**
 * @file AndroidPlatform.cpp
 * @brief Android platform implementation with UDP networking support
 */

#include "AndroidPlatform.h"
#include "GameEngine/networking/UDPNetworking.h"
#include <android/log.h>
#include <jni.h>
#include <string>
#include <vector>
#include <memory>

#define LOG_TAG "AndroidPlatform"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Android platform implementation
class AndroidPlatformImpl {
private:
    Foundry::UDPNetworking* udpNetworking;
    std::vector<std::shared_ptr<Foundry::UDPConnection>> connections;

public:
    AndroidPlatformImpl() : udpNetworking(nullptr) {
        LOGI("AndroidPlatformImpl created");
    }

    ~AndroidPlatformImpl() {
        shutdown();
    }

    bool initialize() {
        LOGI("Initializing Android platform...");

        // Initialize UDP networking
        udpNetworking = Foundry::createUDPNetworking();
        if (!udpNetworking) {
            LOGE("Failed to create UDP networking instance");
            return false;
        }

        if (!udpNetworking->initialize()) {
            LOGE("Failed to initialize UDP networking");
            delete udpNetworking;
            udpNetworking = nullptr;
            return false;
        }

        LOGI("UDP networking initialized successfully");
        return true;
    }

    void shutdown() {
        LOGI("Shutting down Android platform...");

        // Disconnect all connections
        for (auto& connection : connections) {
            if (connection) {
                connection->disconnect();
            }
        }
        connections.clear();

        // Shutdown UDP networking
        if (udpNetworking) {
            udpNetworking->shutdown();
            Foundry::destroyUDPNetworking(udpNetworking);
            udpNetworking = nullptr;
        }

        LOGI("Android platform shutdown complete");
    }

    void update(float deltaTime) {
        if (udpNetworking) {
            udpNetworking->update(deltaTime);
        }
    }

    // UDP Networking API
    Foundry::UDPNetworking* getUDPNetworking() {
        return udpNetworking;
    }

    std::shared_ptr<Foundry::UDPConnection> createUDPConnection() {
        if (!udpNetworking) return nullptr;

        auto connection = udpNetworking->createConnection();
        if (connection) {
            connections.push_back(connection);
        }
        return connection;
    }

    std::shared_ptr<Foundry::UDPSocket> createUDPServerSocket(uint16_t port) {
        if (!udpNetworking) return nullptr;
        return udpNetworking->createServerSocket(port);
    }

    std::string getUDPStatistics() const {
        if (!udpNetworking) return "UDP networking not available";
        return udpNetworking->getStatistics();
    }
};

// Global platform instance
static std::unique_ptr<AndroidPlatformImpl> g_platform;

// JNI functions for UDP networking
extern "C" {

JNIEXPORT jlong JNICALL Java_com_foundryengine_game_GameActivity_nativeCreateUDPNetworking(JNIEnv* env, jobject thiz) {
    if (!g_platform) {
        LOGE("Platform not initialized");
        return 0;
    }

    auto networking = g_platform->getUDPNetworking();
    return reinterpret_cast<jlong>(networking);
}

JNIEXPORT jlong JNICALL Java_com_foundryengine_game_GameActivity_nativeCreateUDPConnection(JNIEnv* env, jobject thiz) {
    if (!g_platform) {
        LOGE("Platform not initialized");
        return 0;
    }

    auto connection = g_platform->createUDPConnection();
    if (!connection) {
        LOGE("Failed to create UDP connection");
        return 0;
    }

    return reinterpret_cast<jlong>(connection.get());
}

JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeUDPConnect(JNIEnv* env, jobject thiz, jlong connectionPtr, jstring address, jint port) {
    auto connection = reinterpret_cast<Foundry::UDPConnection*>(connectionPtr);
    if (!connection) {
        LOGE("Invalid connection pointer");
        return JNI_FALSE;
    }

    const char* addrStr = env->GetStringUTFChars(address, nullptr);
    std::string addr(addrStr);
    env->ReleaseStringUTFChars(address, addrStr);

    bool result = connection->connect(addr, static_cast<uint16_t>(port));
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeUDPDisconnect(JNIEnv* env, jobject thiz, jlong connectionPtr) {
    auto connection = reinterpret_cast<Foundry::UDPConnection*>(connectionPtr);
    if (connection) {
        connection->disconnect();
    }
}

JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeUDPSendPacket(JNIEnv* env, jobject thiz, jlong connectionPtr, jbyteArray data, jboolean reliable) {
    auto connection = reinterpret_cast<Foundry::UDPConnection*>(connectionPtr);
    if (!connection) {
        LOGE("Invalid connection pointer");
        return JNI_FALSE;
    }

    jsize length = env->GetArrayLength(data);
    jbyte* dataPtr = env->GetByteArrayElements(data, nullptr);

    // Create packet from raw data
    Foundry::UDPPacket packet;
    packet.payload.assign(reinterpret_cast<uint8_t*>(dataPtr),
                         reinterpret_cast<uint8_t*>(dataPtr) + length);
    packet.payloadSize = static_cast<uint16_t>(length);

    // Set packet properties
    static uint16_t sequenceCounter = 1;
    packet.sequenceNumber = sequenceCounter++;
    packet.type = Foundry::UDPPacketType::CustomStart;
    packet.timestamp = static_cast<uint32_t>(time(nullptr));

    bool result = connection->sendPacket(packet, reliable);

    env->ReleaseByteArrayElements(data, dataPtr, JNI_ABORT);

    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jbyteArray JNICALL Java_com_foundryengine_game_GameActivity_nativeUDPReceivePacket(JNIEnv* env, jobject thiz, jlong connectionPtr) {
    auto connection = reinterpret_cast<Foundry::UDPConnection*>(connectionPtr);
    if (!connection) {
        LOGE("Invalid connection pointer");
        return nullptr;
    }

    Foundry::UDPPacket packet;
    if (!connection->receivePacket(packet)) {
        return nullptr;
    }

    // Convert packet payload to Java byte array
    jbyteArray result = env->NewByteArray(packet.payload.size());
    env->SetByteArrayRegion(result, 0, packet.payload.size(),
                           reinterpret_cast<jbyte*>(packet.payload.data()));

    return result;
}

JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeUDPIsConnected(JNIEnv* env, jobject thiz, jlong connectionPtr) {
    auto connection = reinterpret_cast<Foundry::UDPConnection*>(connectionPtr);
    if (!connection) {
        return JNI_FALSE;
    }

    return connection->isConnected() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL Java_com_foundryengine_game_GameActivity_nativeGetUDPStatistics(JNIEnv* env, jobject thiz) {
    if (!g_platform) {
        return env->NewStringUTF("Platform not initialized");
    }

    std::string stats = g_platform->getUDPStatistics();
    return env->NewStringUTF(stats.c_str());
}

JNIEXPORT jlong JNICALL Java_com_foundryengine_game_GameActivity_nativeCreateUDPServerSocket(JNIEnv* env, jobject thiz, jint port) {
    if (!g_platform) {
        LOGE("Platform not initialized");
        return 0;
    }

    auto socket = g_platform->createUDPServerSocket(static_cast<uint16_t>(port));
    if (!socket) {
        LOGE("Failed to create UDP server socket");
        return 0;
    }

    return reinterpret_cast<jlong>(socket.get());
}

} // extern "C"

// Platform interface functions
extern "C" {

bool AndroidPlatform_Initialize() {
    if (g_platform) {
        LOGI("Platform already initialized");
        return true;
    }

    g_platform = std::make_unique<AndroidPlatformImpl>();
    if (!g_platform->initialize()) {
        LOGE("Failed to initialize Android platform");
        g_platform.reset();
        return false;
    }

    LOGI("Android platform initialized successfully");
    return true;
}

void AndroidPlatform_Shutdown() {
    if (g_platform) {
        g_platform->shutdown();
        g_platform.reset();
        LOGI("Android platform shutdown");
    }
}

void AndroidPlatform_Update(float deltaTime) {
    if (g_platform) {
        g_platform->update(deltaTime);
    }
}

} // extern "C"
