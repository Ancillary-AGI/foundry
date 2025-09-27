#ifndef FOUNDRYENGINE_AAUDIO_PLATFORM_H
#define FOUNDRYENGINE_AAUDIO_PLATFORM_H

#include "../../core/Platform.h"
#include <aaudio/AAudio.h>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace FoundryEngine {

// Forward declarations
class AAudioEngine;
class AAudioStream;
class AAudioManager;

// Audio stream configuration
struct AAudioConfig {
    aaudio_direction_t direction = AAUDIO_DIRECTION_OUTPUT;
    aaudio_format_t format = AAUDIO_FORMAT_PCM_FLOAT;
    int32_t sampleRate = 48000;
    int32_t channelCount = 2;
    int32_t bufferCapacity = 2048;
    aaudio_sharing_mode_t sharingMode = AAUDIO_SHARING_MODE_SHARED;
    aaudio_performance_mode_t performanceMode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
    aaudio_usage_t usage = AAUDIO_USAGE_GAME;
    aaudio_content_type_t contentType = AAUDIO_CONTENT_TYPE_SONIFICATION;
    int32_t framesPerDataCallback = 0; // 0 = use default
    bool enableNoiseSuppression = false;
    bool enableAutomaticGainControl = false;
    bool enableAcousticEchoCancellation = false;
};

// Audio stream state
enum class AAudioStreamState {
    UNINITIALIZED,
    OPEN,
    STARTING,
    STARTED,
    PAUSING,
    PAUSED,
    FLUSHING,
    FLUSHED,
    STOPPING,
    STOPPED,
    CLOSING,
    CLOSED,
    DISCONNECTED
};

// Audio data callback
using AAudioDataCallback = std::function<aaudio_data_callback_result_t(
    AAudioStream* stream,
    void* audioData,
    int32_t numFrames
)>;

// Error callback
using AAudioErrorCallback = std::function<void(
    AAudioStream* stream,
    aaudio_result_t error
)>;

// ========== AAUDIO ENGINE ==========
class AAudioEngine {
private:
    static AAudioEngine* instance_;

    AAudioManager* manager_;
    std::vector<AAudioStream*> activeStreams_;
    std::mutex streamsMutex_;

    // Global audio settings
    float masterVolume_ = 1.0f;
    bool audioEnabled_ = true;

    // Performance monitoring
    std::atomic<int64_t> totalFramesRead_;
    std::atomic<int64_t> totalFramesWritten_;
    std::atomic<int32_t> xruns_;

public:
    AAudioEngine();
    ~AAudioEngine();

    static AAudioEngine* getInstance();

    // Engine management
    bool initialize();
    void shutdown();

    // Stream management
    AAudioStream* createStream(const AAudioConfig& config);
    void destroyStream(AAudioStream* stream);
    void stopAllStreams();

    // Global audio control
    void setMasterVolume(float volume);
    float getMasterVolume() const { return masterVolume_; }

    void setAudioEnabled(bool enabled);
    bool isAudioEnabled() const { return audioEnabled_; }

    // Performance monitoring
    int64_t getTotalFramesRead() const { return totalFramesRead_; }
    int64_t getTotalFramesWritten() const { return totalFramesWritten_; }
    int32_t getXRunCount() const { return xruns_; }

    // Utility functions
    int32_t getOptimalBufferSize(int32_t sampleRate, int32_t channelCount);
    int32_t getMinimumBufferSize(int32_t sampleRate, int32_t channelCount);
    std::vector<int32_t> getAvailableSampleRates();

private:
    void incrementFramesRead(int64_t frames);
    void incrementFramesWritten(int64_t frames);
    void incrementXRunCount();
};

// ========== AAUDIO STREAM ==========
class AAudioStream {
private:
    AAudioEngine* engine_;
    AAudioConfig config_;

    AAudioStreamBuilder* builder_;
    AAudioStream* stream_;

    // Stream state
    AAudioStreamState state_;
    std::atomic<bool> running_;
    std::thread callbackThread_;

    // Callbacks
    AAudioDataCallback dataCallback_;
    AAudioErrorCallback errorCallback_;

    // Audio buffers
    std::vector<float> inputBuffer_;
    std::vector<float> outputBuffer_;
    int32_t bufferFrameSize_;

    // Synchronization
    std::mutex stateMutex_;
    std::condition_variable stateCondition_;
    std::mutex callbackMutex_;

    // Performance metrics
    std::atomic<int64_t> framesRead_;
    std::atomic<int64_t> framesWritten_;
    std::atomic<double> averageLatency_;
    std::atomic<int32_t> callbackCount_;

public:
    AAudioStream(AAudioEngine* engine, const AAudioConfig& config);
    ~AAudioStream();

    // Stream lifecycle
    bool open();
    bool start();
    bool pause();
    bool flush();
    bool stop();
    bool close();

    // Stream control
    AAudioStreamState getState() const;
    bool isRunning() const { return running_; }

    // Audio I/O
    int32_t read(float* buffer, int32_t numFrames);
    int32_t write(const float* buffer, int32_t numFrames);

    // Stream information
    int32_t getSampleRate() const;
    int32_t getChannelCount() const;
    int32_t getFormat() const;
    int32_t getBufferSize() const;
    int32_t getBufferCapacity() const;
    int64_t getFramesRead() const { return framesRead_; }
    int64_t getFramesWritten() const { return framesWritten_; }
    double getAverageLatency() const { return averageLatency_; }
    int32_t getCallbackCount() const { return callbackCount_; }

    // Configuration
    const AAudioConfig& getConfig() const { return config_; }
    void setDataCallback(AAudioDataCallback callback) { dataCallback_ = callback; }
    void setErrorCallback(AAudioErrorCallback callback) { errorCallback_ = callback; }

    // Advanced features
    bool setBufferSize(int32_t bufferSize);
    bool updateBufferSize(int32_t bufferSize);
    int32_t getXRunCount() const;
    double getTimestamp(int64_t* framePosition = nullptr);

private:
    // Internal methods
    void setState(AAudioStreamState newState);
    void callbackLoop();
    aaudio_data_callback_result_t dataCallback(void* audioData, int32_t numFrames);
    void errorCallback(aaudio_result_t error);

    // Static callback functions for AAudio
    static aaudio_data_callback_result_t staticDataCallback(
        AAudioStream* stream,
        void* userData,
        void* audioData,
        int32_t numFrames
    );

    static void staticErrorCallback(
        AAudioStream* stream,
        void* userData,
        aaudio_result_t error
    );
};

// ========== AAUDIO MANAGER ==========
class AAudioManager {
private:
    AAudioEngine* engine_;

    // Audio devices
    std::vector<aaudio_device_id_t> outputDevices_;
    std::vector<aaudio_device_id_t> inputDevices_;

    // Default device IDs
    aaudio_device_id_t defaultOutputDevice_;
    aaudio_device_id_t defaultInputDevice_;

    // Device capabilities
    struct DeviceCapabilities {
        aaudio_device_id_t deviceId;
        aaudio_direction_t direction;
        std::vector<int32_t> sampleRates;
        std::vector<int32_t> channelCounts;
        std::vector<aaudio_format_t> formats;
        int32_t minBufferSize;
        int32_t maxBufferSize;
        bool isLowLatency;
    };

    std::unordered_map<aaudio_device_id_t, DeviceCapabilities> deviceCapabilities_;

public:
    AAudioManager(AAudioEngine* engine);
    ~AAudioManager();

    // Device enumeration
    void enumerateDevices();
    const std::vector<aaudio_device_id_t>& getOutputDevices() const { return outputDevices_; }
    const std::vector<aaudio_device_id_t>& getInputDevices() const { return inputDevices_; }
    aaudio_device_id_t getDefaultOutputDevice() const { return defaultOutputDevice_; }
    aaudio_device_id_t getDefaultInputDevice() const { return defaultInputDevice_; }

    // Device capabilities
    const DeviceCapabilities* getDeviceCapabilities(aaudio_device_id_t deviceId) const;
    bool isDeviceLowLatency(aaudio_device_id_t deviceId) const;

    // Device selection
    aaudio_device_id_t selectBestOutputDevice() const;
    aaudio_device_id_t selectBestInputDevice() const;
    aaudio_device_id_t selectDeviceForSampleRate(int32_t sampleRate, int32_t channelCount) const;

    // Audio session management
    void requestAudioFocus();
    void abandonAudioFocus();
    bool isAudioFocusGranted() const;

    // Performance optimization
    void setPerformanceMode(aaudio_performance_mode_t mode);
    aaudio_performance_mode_t getPerformanceMode() const;

    // Audio policy
    void setAllowedCapturePolicy(aaudio_allowed_capture_policy_t policy);
    aaudio_allowed_capture_policy_t getAllowedCapturePolicy() const;

private:
    void queryDeviceCapabilities(aaudio_device_id_t deviceId, aaudio_direction_t direction);
    void updateDefaultDevices();
};

// ========== AUDIO EFFECTS ==========
class AAudioEffects {
public:
    // Reverb effect
    static void applyReverb(float* buffer, int32_t frames, float roomSize, float damping, float wetLevel);

    // Equalizer effect
    static void applyEqualizer(float* buffer, int32_t frames, const float* bandGains, int32_t numBands);

    // Compressor effect
    static void applyCompressor(float* buffer, int32_t frames, float threshold, float ratio, float attackTime, float releaseTime);

    // Limiter effect
    static void applyLimiter(float* buffer, int32_t frames, float threshold, float releaseTime);

    // Chorus effect
    static void applyChorus(float* buffer, int32_t frames, float rate, float depth, float mix);

    // Flanger effect
    static void applyFlanger(float* buffer, int32_t frames, float rate, float depth, float feedback, float mix);

    // Delay effect
    static void applyDelay(float* buffer, int32_t frames, float delayTime, float feedback, float mix);

    // Pitch shift effect
    static void applyPitchShift(float* buffer, int32_t frames, float pitchRatio);

    // Low-pass filter
    static void applyLowPassFilter(float* buffer, int32_t frames, float cutoffFreq, float sampleRate);

    // High-pass filter
    static void applyHighPassFilter(float* buffer, int32_t frames, float cutoffFreq, float sampleRate);

    // Band-pass filter
    static void applyBandPassFilter(float* buffer, int32_t frames, float lowFreq, float highFreq, float sampleRate);

    // Notch filter
    static void applyNotchFilter(float* buffer, int32_t frames, float centerFreq, float qFactor, float sampleRate);
};

// ========== SPATIAL AUDIO ==========
class AAudioSpatializer {
private:
    struct Source {
        float x, y, z;           // Position
        float vx, vy, vz;        // Velocity
        float volume;            // Volume (0.0 to 1.0)
        bool is3D;              // 3D positioning enabled
        float minDistance;      // Minimum distance for attenuation
        float maxDistance;      // Maximum distance for attenuation
        float rolloffFactor;    // Rolloff factor
        float dopplerFactor;    // Doppler effect strength
    };

    struct Listener {
        float x, y, z;          // Position
        float vx, vy, vz;       // Velocity
        float fx, fy, fz;       // Forward vector
        float ux, uy, uz;       // Up vector
        float volume;           // Master volume
    };

    std::vector<Source> sources_;
    Listener listener_;
    float speedOfSound_;

public:
    AAudioSpatializer();
    ~AAudioSpatializer();

    // Source management
    int32_t addSource(float x = 0, float y = 0, float z = 0);
    void removeSource(int32_t sourceId);
    void setSourcePosition(int32_t sourceId, float x, float y, float z);
    void setSourceVelocity(int32_t sourceId, float vx, float vy, float vz);
    void setSourceVolume(int32_t sourceId, float volume);
    void setSourceDistance(int32_t sourceId, float minDistance, float maxDistance);
    void setSourceRolloff(int32_t sourceId, float rolloffFactor);
    void setSourceDoppler(int32_t sourceId, float dopplerFactor);

    // Listener management
    void setListenerPosition(float x, float y, float z);
    void setListenerVelocity(float vx, float vy, float vz);
    void setListenerOrientation(float fx, float fy, float fz, float ux, float uy, float uz);
    void setListenerVolume(float volume);

    // Global settings
    void setSpeedOfSound(float speed);
    float getSpeedOfSound() const { return speedOfSound_; }

    // Spatial processing
    void processSpatialAudio(float* buffer, int32_t frames, int32_t channels, float sampleRate);

private:
    void calculateAttenuation(Source& source, float distance, float& volume, float& lowPass);
    void calculateDopplerShift(Source& source, float& pitchShift);
    void applyHRTF(Source& source, float* leftChannel, float* rightChannel, int32_t frames);
    float calculateDistance(const Source& source) const;
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // AAudio engine callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_AAudioEngine_onStreamStateChanged(
        JNIEnv* env, jobject thiz, jlong streamPtr, jint state);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AAudioEngine_onStreamError(
        JNIEnv* env, jobject thiz, jlong streamPtr, jint error);

    // Audio device callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_AAudioEngine_onDeviceConnected(
        JNIEnv* env, jobject thiz, jlong deviceId);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AAudioEngine_onDeviceDisconnected(
        JNIEnv* env, jobject thiz, jlong deviceId);

    // Audio focus callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_AAudioEngine_onAudioFocusGained(
        JNIEnv* env, jobject thiz);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_AAudioEngine_onAudioFocusLost(
        JNIEnv* env, jobject thiz);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_AAUDIO_PLATFORM_H
