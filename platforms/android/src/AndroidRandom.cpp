/**
 * @file AndroidRandom.cpp
 * @brief Complete Android random number generation implementation
 */

#include "../core/AndroidPlatform.h"
#include <android/log.h>
#include <random>
#include <chrono>

#define LOG_TAG "AndroidRandom"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace FoundryEngine {

class AndroidRandomImpl : public AndroidRandom {
private:
    std::mt19937 generator_;
    std::uniform_real_distribution<double> uniformDist_;
    bool seeded_;

public:
    AndroidRandomImpl() : uniformDist_(0.0, 1.0), seeded_(false) {
        // Auto-seed with current time if not manually seeded
        auto now = std::chrono::high_resolution_clock::now();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        generator_.seed(static_cast<unsigned int>(nanos));
        seeded_ = true;
        LOGI("Android random number generator initialized with auto-seed");
    }

    double random() override {
        return uniformDist_(generator_);
    }

    int randomInt(int min, int max) override {
        if (min > max) {
            std::swap(min, max);
        }
        
        std::uniform_int_distribution<int> intDist(min, max);
        return intDist(generator_);
    }

    double randomFloat(double min, double max) override {
        if (min > max) {
            std::swap(min, max);
        }
        
        std::uniform_real_distribution<double> floatDist(min, max);
        return floatDist(generator_);
    }

    void seed(unsigned int seed) override {
        generator_.seed(seed);
        seeded_ = true;
        LOGI("Android random number generator seeded with: %u", seed);
    }

    // Android-specific random utilities
    std::vector<uint8_t> randomBytes(size_t count) {
        std::vector<uint8_t> bytes(count);
        std::uniform_int_distribution<int> byteDist(0, 255);
        
        for (size_t i = 0; i < count; ++i) {
            bytes[i] = static_cast<uint8_t>(byteDist(generator_));
        }
        
        return bytes;
    }

    std::string randomString(size_t length, const std::string& charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") {
        std::string result;
        result.reserve(length);
        
        std::uniform_int_distribution<size_t> charDist(0, charset.length() - 1);
        
        for (size_t i = 0; i < length; ++i) {
            result += charset[charDist(generator_)];
        }
        
        return result;
    }

    bool randomBool(double probability = 0.5) {
        return random() < probability;
    }

    // Gaussian/normal distribution
    double randomGaussian(double mean = 0.0, double stddev = 1.0) {
        std::normal_distribution<double> normalDist(mean, stddev);
        return normalDist(generator_);
    }

    // Exponential distribution
    double randomExponential(double lambda = 1.0) {
        std::exponential_distribution<double> expDist(lambda);
        return expDist(generator_);
    }

    // Shuffle array/vector
    template<typename Iterator>
    void shuffle(Iterator first, Iterator last) {
        std::shuffle(first, last, generator_);
    }

    // Random choice from container
    template<typename Container>
    auto randomChoice(const Container& container) -> decltype(container[0]) {
        if (container.empty()) {
            throw std::runtime_error("Cannot choose from empty container");
        }
        
        std::uniform_int_distribution<size_t> indexDist(0, container.size() - 1);
        return container[indexDist(generator_)];
    }
};

// Global instance
static std::unique_ptr<AndroidRandomImpl> g_random;

} // namespace FoundryEngine

// JNI functions
extern "C" {

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeInitializeRandom(JNIEnv* env, jobject thiz) {
    using namespace FoundryEngine;
    g_random = std::make_unique<AndroidRandomImpl>();
    LOGI("Android random system initialized");
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeShutdownRandom(JNIEnv* env, jobject thiz) {
    using namespace FoundryEngine;
    g_random.reset();
    LOGI("Android random system shutdown");
}

JNIEXPORT jdouble JNICALL Java_com_foundryengine_game_GameActivity_nativeRandom(JNIEnv* env, jobject thiz) {
    if (!g_random) return 0.0;
    return g_random->random();
}

JNIEXPORT jint JNICALL Java_com_foundryengine_game_GameActivity_nativeRandomInt(JNIEnv* env, jobject thiz, jint min, jint max) {
    if (!g_random) return min;
    return g_random->randomInt(min, max);
}

JNIEXPORT jdouble JNICALL Java_com_foundryengine_game_GameActivity_nativeRandomFloat(JNIEnv* env, jobject thiz, jdouble min, jdouble max) {
    if (!g_random) return min;
    return g_random->randomFloat(min, max);
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeRandomSeed(JNIEnv* env, jobject thiz, jint seed) {
    if (g_random) {
        g_random->seed(static_cast<unsigned int>(seed));
    }
}

JNIEXPORT jbyteArray JNICALL Java_com_foundryengine_game_GameActivity_nativeRandomBytes(JNIEnv* env, jobject thiz, jint count) {
    if (!g_random || count <= 0) return nullptr;
    
    std::vector<uint8_t> bytes = g_random->randomBytes(count);
    
    jbyteArray result = env->NewByteArray(bytes.size());
    env->SetByteArrayRegion(result, 0, bytes.size(), reinterpret_cast<jbyte*>(bytes.data()));
    return result;
}

JNIEXPORT jstring JNICALL Java_com_foundryengine_game_GameActivity_nativeRandomString(JNIEnv* env, jobject thiz, jint length, jstring charset) {
    if (!g_random || length <= 0) return nullptr;
    
    std::string charsetStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    
    if (charset) {
        const char* charsetCStr = env->GetStringUTFChars(charset, nullptr);
        charsetStr = charsetCStr;
        env->ReleaseStringUTFChars(charset, charsetCStr);
    }
    
    std::string result = g_random->randomString(length, charsetStr);
    return env->NewStringUTF(result.c_str());
}

JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeRandomBool(JNIEnv* env, jobject thiz, jdouble probability) {
    if (!g_random) return JNI_FALSE;
    return g_random->randomBool(probability) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jdouble JNICALL Java_com_foundryengine_game_GameActivity_nativeRandomGaussian(JNIEnv* env, jobject thiz, jdouble mean, jdouble stddev) {
    if (!g_random) return mean;
    return g_random->randomGaussian(mean, stddev);
}

} // extern "C"