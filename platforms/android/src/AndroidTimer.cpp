/**
 * @file AndroidTimer.cpp
 * @brief Complete Android timer implementation with high-precision timing
 */

#include "../core/AndroidPlatform.h"
#include <android/log.h>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <future>

#define LOG_TAG "AndroidTimer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace FoundryEngine {

class AndroidTimerImpl : public AndroidTimer {
private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::unordered_map<int, std::future<void>> timeouts_;
    std::unordered_map<int, std::future<void>> intervals_;
    std::unordered_map<int, std::atomic<bool>> intervalFlags_;
    std::unordered_map<int, std::future<void>> animationFrames_;
    std::mutex timerMutex_;
    std::atomic<int> nextId_{1};
    std::atomic<bool> running_{true};

public:
    AndroidTimerImpl() {
        startTime_ = std::chrono::high_resolution_clock::now();
        LOGI("Android timer initialized");
    }

    ~AndroidTimerImpl() {
        running_ = false;
        
        // Cancel all active timers
        std::lock_guard<std::mutex> lock(timerMutex_);
        
        for (auto& [id, flag] : intervalFlags_) {
            flag = false;
        }
        
        timeouts_.clear();
        intervals_.clear();
        animationFrames_.clear();
        intervalFlags_.clear();
        
        LOGI("Android timer shutdown");
    }

    double now() override {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime_);
        return duration.count() / 1000.0; // Return milliseconds with microsecond precision
    }

    int setTimeout(std::function<void()> callback, int delay) override {
        if (!callback || delay < 0) return 0;
        
        int id = nextId_++;
        
        std::lock_guard<std::mutex> lock(timerMutex_);
        
        timeouts_[id] = std::async(std::launch::async, [this, callback, delay, id]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            if (running_) {
                try {
                    callback();
                } catch (const std::exception& e) {
                    LOGE("Exception in setTimeout callback: %s", e.what());
                }
                
                // Clean up after execution
                std::lock_guard<std::mutex> lock(timerMutex_);
                timeouts_.erase(id);
            }
        });
        
        LOGI("Created timeout %d with delay %dms", id, delay);
        return id;
    }

    void clearTimeout(int id) override {
        std::lock_guard<std::mutex> lock(timerMutex_);
        
        auto it = timeouts_.find(id);
        if (it != timeouts_.end()) {
            // The future will be destroyed, effectively canceling the timeout
            timeouts_.erase(it);
            LOGI("Cleared timeout %d", id);
        }
    }

    int setInterval(std::function<void()> callback, int delay) override {
        if (!callback || delay < 0) return 0;
        
        int id = nextId_++;
        
        std::lock_guard<std::mutex> lock(timerMutex_);
        
        intervalFlags_[id] = true;
        
        intervals_[id] = std::async(std::launch::async, [this, callback, delay, id]() {
            while (running_ && intervalFlags_[id]) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                
                if (running_ && intervalFlags_[id]) {
                    try {
                        callback();
                    } catch (const std::exception& e) {
                        LOGE("Exception in setInterval callback: %s", e.what());
                    }
                }
            }
            
            // Clean up after completion
            std::lock_guard<std::mutex> lock(timerMutex_);
            intervals_.erase(id);
            intervalFlags_.erase(id);
        });
        
        LOGI("Created interval %d with delay %dms", id, delay);
        return id;
    }

    void clearInterval(int id) override {
        std::lock_guard<std::mutex> lock(timerMutex_);
        
        auto flagIt = intervalFlags_.find(id);
        if (flagIt != intervalFlags_.end()) {
            flagIt->second = false;
            intervalFlags_.erase(flagIt);
            
            auto intervalIt = intervals_.find(id);
            if (intervalIt != intervals_.end()) {
                intervals_.erase(intervalIt);
            }
            
            LOGI("Cleared interval %d", id);
        }
    }

    int requestAnimationFrame(std::function<void(double)> callback) override {
        if (!callback) return 0;
        
        int id = nextId_++;
        
        std::lock_guard<std::mutex> lock(timerMutex_);
        
        animationFrames_[id] = std::async(std::launch::async, [this, callback, id]() {
            // Target 60 FPS (16.67ms per frame)
            const int frameDelay = 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay));
            
            if (running_) {
                double timestamp = now();
                
                try {
                    callback(timestamp);
                } catch (const std::exception& e) {
                    LOGE("Exception in requestAnimationFrame callback: %s", e.what());
                }
                
                // Clean up after execution
                std::lock_guard<std::mutex> lock(timerMutex_);
                animationFrames_.erase(id);
            }
        });
        
        LOGI("Created animation frame %d", id);
        return id;
    }

    void cancelAnimationFrame(int id) override {
        std::lock_guard<std::mutex> lock(timerMutex_);
        
        auto it = animationFrames_.find(id);
        if (it != animationFrames_.end()) {
            animationFrames_.erase(it);
            LOGI("Canceled animation frame %d", id);
        }
    }

    // Android-specific high-precision timing
    int64_t getNanoTime() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }

    void sleep(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    void sleepMicros(int microseconds) {
        std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
    }

    void sleepNanos(int64_t nanoseconds) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(nanoseconds));
    }

    // Performance timing utilities
    class PerformanceTimer {
    private:
        std::chrono::high_resolution_clock::time_point start_;
        std::string name_;
        
    public:
        PerformanceTimer(const std::string& name) : name_(name) {
            start_ = std::chrono::high_resolution_clock::now();
        }
        
        ~PerformanceTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
            LOGI("Performance [%s]: %.3fms", name_.c_str(), duration.count() / 1000.0);
        }
    };

    std::unique_ptr<PerformanceTimer> createPerformanceTimer(const std::string& name) {
        return std::make_unique<PerformanceTimer>(name);
    }
};

// Global instance
static std::unique_ptr<AndroidTimerImpl> g_timer;

} // namespace FoundryEngine

// JNI functions
extern "C" {

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeInitializeTimer(JNIEnv* env, jobject thiz) {
    using namespace FoundryEngine;
    g_timer = std::make_unique<AndroidTimerImpl>();
    LOGI("Android timer system initialized");
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeShutdownTimer(JNIEnv* env, jobject thiz) {
    using namespace FoundryEngine;
    g_timer.reset();
    LOGI("Android timer system shutdown");
}

JNIEXPORT jdouble JNICALL Java_com_foundryengine_game_GameActivity_nativeGetCurrentTime(JNIEnv* env, jobject thiz) {
    if (!g_timer) return 0.0;
    return g_timer->now();
}

JNIEXPORT jlong JNICALL Java_com_foundryengine_game_GameActivity_nativeGetNanoTime(JNIEnv* env, jobject thiz) {
    if (!g_timer) return 0;
    return g_timer->getNanoTime();
}

JNIEXPORT jint JNICALL Java_com_foundryengine_game_GameActivity_nativeSetTimeout(JNIEnv* env, jobject thiz, jobject callback, jint delay) {
    if (!g_timer) return 0;
    
    // Create a global reference to the callback
    jobject globalCallback = env->NewGlobalRef(callback);
    jclass callbackClass = env->GetObjectClass(globalCallback);
    jmethodID runMethod = env->GetMethodID(callbackClass, "run", "()V");
    
    auto cppCallback = [env, globalCallback, runMethod]() {
        env->CallVoidMethod(globalCallback, runMethod);
        env->DeleteGlobalRef(globalCallback);
    };
    
    return g_timer->setTimeout(cppCallback, delay);
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeClearTimeout(JNIEnv* env, jobject thiz, jint id) {
    if (g_timer) {
        g_timer->clearTimeout(id);
    }
}

JNIEXPORT jint JNICALL Java_com_foundryengine_game_GameActivity_nativeSetInterval(JNIEnv* env, jobject thiz, jobject callback, jint delay) {
    if (!g_timer) return 0;
    
    // Create a global reference to the callback
    jobject globalCallback = env->NewGlobalRef(callback);
    jclass callbackClass = env->GetObjectClass(globalCallback);
    jmethodID runMethod = env->GetMethodID(callbackClass, "run", "()V");
    
    auto cppCallback = [env, globalCallback, runMethod]() {
        env->CallVoidMethod(globalCallback, runMethod);
    };
    
    return g_timer->setInterval(cppCallback, delay);
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeClearInterval(JNIEnv* env, jobject thiz, jint id) {
    if (g_timer) {
        g_timer->clearInterval(id);
    }
}

JNIEXPORT jint JNICALL Java_com_foundryengine_game_GameActivity_nativeRequestAnimationFrame(JNIEnv* env, jobject thiz, jobject callback) {
    if (!g_timer) return 0;
    
    // Create a global reference to the callback
    jobject globalCallback = env->NewGlobalRef(callback);
    jclass callbackClass = env->GetObjectClass(globalCallback);
    jmethodID runMethod = env->GetMethodID(callbackClass, "run", "(D)V");
    
    auto cppCallback = [env, globalCallback, runMethod](double timestamp) {
        env->CallVoidMethod(globalCallback, runMethod, timestamp);
        env->DeleteGlobalRef(globalCallback);
    };
    
    return g_timer->requestAnimationFrame(cppCallback);
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeCancelAnimationFrame(JNIEnv* env, jobject thiz, jint id) {
    if (g_timer) {
        g_timer->cancelAnimationFrame(id);
    }
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeSleep(JNIEnv* env, jobject thiz, jint milliseconds) {
    if (g_timer) {
        g_timer->sleep(milliseconds);
    }
}

} // extern "C"