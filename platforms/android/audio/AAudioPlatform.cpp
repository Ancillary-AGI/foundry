#include "AAudioPlatform.h"
#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <thread>
#include <chrono>

#define LOG_TAG "AAudioPlatform"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace FoundryEngine {

// Static instance
AAudioEngine* AAudioEngine::instance_ = nullptr;

// ========== AAUDIO ENGINE IMPLEMENTATION ==========
AAudioEngine::AAudioEngine() : manager_(nullptr), masterVolume_(1.0f), audioEnabled_(true),
                               totalFramesRead_(0), totalFramesWritten_(0), xruns_(0) {
    LOGI("AAudioEngine constructor called");
}

AAudioEngine::~AAudioEngine() {
    shutdown();
    LOGI("AAudioEngine destructor called");
}

AAudioEngine* AAudioEngine::getInstance() {
    if (!instance_) {
        instance_ = new AAudioEngine();
    }
    return instance_;
}

bool AAudioEngine::initialize() {
    LOGI("Initializing AAudio Engine");

    if (manager_) {
        LOGW("AAudio Engine already initialized");
        return true;
    }

    // Create audio manager
    manager_ = new AAudioManager(this);

    // Initialize audio system
    aaudio_result_t result = AAudio_createStreamBuilder(&reinterpret_cast<AAudioStreamBuilder**>(&manager_));
    if (result != AAUDIO_OK) {
        LOGE("Failed to create stream builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    LOGI("AAudio Engine initialized successfully");
    return true;
}

void AAudioEngine::shutdown() {
    LOGI("Shutting down AAudio Engine");

    if (!manager_) {
        return;
    }

    // Stop all active streams
    stopAllStreams();

    // Clean up manager
    delete manager_;
    manager_ = nullptr;

    LOGI("AAudio Engine shutdown complete");
}

AAudioStream* AAudioEngine::createStream(const AAudioConfig& config) {
    LOGI("Creating AAudio stream");

    std::lock_guard<std::mutex> lock(streamsMutex_);

    AAudioStream* stream = new AAudioStream(this, config);
    if (stream->open()) {
        activeStreams_.push_back(stream);
        LOGI("AAudio stream created successfully");
        return stream;
    } else {
        LOGE("Failed to create AAudio stream");
        delete stream;
        return nullptr;
    }
}

void AAudioEngine::destroyStream(AAudioStream* stream) {
    LOGI("Destroying AAudio stream");

    if (!stream) {
        return;
    }

    std::lock_guard<std::mutex> lock(streamsMutex_);

    // Remove from active streams
    auto it = std::find(activeStreams_.begin(), activeStreams_.end(), stream);
    if (it != activeStreams_.end()) {
        activeStreams_.erase(it);
    }

    // Close and delete stream
    stream->close();
    delete stream;

    LOGI("AAudio stream destroyed");
}

void AAudioEngine::stopAllStreams() {
    LOGI("Stopping all AAudio streams");

    std::lock_guard<std::mutex> lock(streamsMutex_);

    for (auto stream : activeStreams_) {
        if (stream) {
            stream->stop();
        }
    }

    activeStreams_.clear();

    LOGI("All AAudio streams stopped");
}

void AAudioEngine::setMasterVolume(float volume) {
    masterVolume_ = std::max(0.0f, std::min(1.0f, volume));

    std::lock_guard<std::mutex> lock(streamsMutex_);

    for (auto stream : activeStreams_) {
        if (stream) {
            // Apply volume to stream if it supports it
            // In a real implementation, this would adjust stream volume
        }
    }

    LOGI("Master volume set to: %.2f", masterVolume_);
}

void AAudioEngine::setAudioEnabled(bool enabled) {
    audioEnabled_ = enabled;

    std::lock_guard<std::mutex> lock(streamsMutex_);

    for (auto stream : activeStreams_) {
        if (stream) {
            if (enabled) {
                stream->start();
            } else {
                stream->stop();
            }
        }
    }

    LOGI("Audio enabled: %s", enabled ? "true" : "false");
}

int32_t AAudioEngine::getOptimalBufferSize(int32_t sampleRate, int32_t channelCount) {
    // Calculate optimal buffer size for low latency
    // This is a heuristic based on AAudio recommendations
    int32_t framesPerBurst = 192; // Common burst size on Android

    // Adjust for sample rate
    if (sampleRate > 48000) {
        framesPerBurst *= 2;
    }

    return framesPerBurst * channelCount;
}

int32_t AAudioEngine::getMinimumBufferSize(int32_t sampleRate, int32_t channelCount) {
    // Minimum buffer size for stable audio
    return 256 * channelCount;
}

std::vector<int32_t> AAudioEngine::getAvailableSampleRates() {
    std::vector<int32_t> rates = {8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 96000};

    // Filter out rates that might not be supported
    std::vector<int32_t> supportedRates;

    for (int32_t rate : rates) {
        // In a real implementation, check if rate is supported
        supportedRates.push_back(rate);
    }

    return supportedRates;
}

void AAudioEngine::incrementFramesRead(int64_t frames) {
    totalFramesRead_ += frames;
}

void AAudioEngine::incrementFramesWritten(int64_t frames) {
    totalFramesWritten_ += frames;
}

void AAudioEngine::incrementXRunCount() {
    xruns_++;
}

// ========== AAUDIO STREAM IMPLEMENTATION ==========
AAudioStream::AAudioStream(AAudioEngine* engine, const AAudioConfig& config)
    : engine_(engine), config_(config), builder_(nullptr), stream_(nullptr),
      state_(AAudioStreamState::UNINITIALIZED), running_(false), bufferFrameSize_(0),
      framesRead_(0), framesWritten_(0), averageLatency_(0.0), callbackCount_(0) {
    LOGI("AAudioStream constructor called");
}

AAudioStream::~AAudioStream() {
    close();
    LOGI("AAudioStream destructor called");
}

bool AAudioStream::open() {
    LOGI("Opening AAudio stream");

    if (state_ != AAudioStreamState::UNINITIALIZED) {
        LOGW("Stream already opened");
        return true;
    }

    // Create stream builder
    aaudio_result_t result = AAudio_createStreamBuilder(&builder_);
    if (result != AAUDIO_OK) {
        LOGE("Failed to create stream builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    // Configure stream
    AAudioStreamBuilder_setDirection(builder_, config_.direction);
    AAudioStreamBuilder_setFormat(builder_, config_.format);
    AAudioStreamBuilder_setSampleRate(builder_, config_.sampleRate);
    AAudioStreamBuilder_setChannelCount(builder_, config_.channelCount);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder_, config_.bufferCapacity);
    AAudioStreamBuilder_setSharingMode(builder_, config_.sharingMode);
    AAudioStreamBuilder_setPerformanceMode(builder_, config_.performanceMode);
    AAudioStreamBuilder_setUsage(builder_, config_.usage);
    AAudioStreamBuilder_setContentType(builder_, config_.contentType);
    AAudioStreamBuilder_setFramesPerDataCallback(builder_, config_.framesPerDataCallback);

    // Set callbacks
    AAudioStreamBuilder_setDataCallback(builder_, staticDataCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder_, staticErrorCallback, this);

    // Build stream
    result = AAudioStreamBuilder_openStream(builder_, &stream_);
    if (result != AAUDIO_OK) {
        LOGE("Failed to open stream: %s", AAudio_convertResultToText(result));
        AAudioStreamBuilder_delete(builder_);
        builder_ = nullptr;
        return false;
    }

    // Get actual configuration (may differ from requested)
    config_.sampleRate = AAudioStream_getSampleRate(stream_);
    config_.channelCount = AAudioStream_getChannelCount(stream_);
    config_.format = AAudioStream_getFormat(stream_);
    config_.bufferCapacity = AAudioStream_getBufferCapacityInFrames(stream_);

    // Allocate buffers
    bufferFrameSize_ = config_.sampleRate / 10; // 100ms buffer
    inputBuffer_.resize(bufferFrameSize_ * config_.channelCount);
    outputBuffer_.resize(bufferFrameSize_ * config_.channelCount);

    setState(AAudioStreamState::OPEN);

    LOGI("AAudio stream opened successfully: %d Hz, %d channels, %s",
         config_.sampleRate, config_.channelCount,
         config_.direction == AAUDIO_DIRECTION_OUTPUT ? "output" : "input");

    return true;
}

bool AAudioStream::start() {
    LOGI("Starting AAudio stream");

    if (state_ != AAudioStreamState::OPEN && state_ != AAudioStreamState::PAUSED) {
        LOGE("Cannot start stream in state: %d", static_cast<int>(state_));
        return false;
    }

    setState(AAudioStreamState::STARTING);

    aaudio_result_t result = AAudioStream_requestStart(stream_);
    if (result != AAUDIO_OK) {
        LOGE("Failed to start stream: %s", AAudio_convertResultToText(result));
        setState(AAudioStreamState::OPEN);
        return false;
    }

    running_ = true;
    setState(AAudioStreamState::STARTED);

    // Start callback thread for input streams
    if (config_.direction == AAUDIO_DIRECTION_INPUT) {
        callbackThread_ = std::thread(&AAudioStream::callbackLoop, this);
    }

    LOGI("AAudio stream started successfully");
    return true;
}

bool AAudioStream::pause() {
    LOGI("Pausing AAudio stream");

    if (state_ != AAudioStreamState::STARTED) {
        LOGE("Cannot pause stream in state: %d", static_cast<int>(state_));
        return false;
    }

    setState(AAudioStreamState::PAUSING);

    aaudio_result_t result = AAudioStream_requestPause(stream_);
    if (result != AAUDIO_OK) {
        LOGE("Failed to pause stream: %s", AAudio_convertResultToText(result));
        setState(AAudioStreamState::STARTED);
        return false;
    }

    running_ = false;
    setState(AAudioStreamState::PAUSED);

    LOGI("AAudio stream paused successfully");
    return true;
}

bool AAudioStream::flush() {
    LOGI("Flushing AAudio stream");

    if (state_ != AAudioStreamState::STARTED && state_ != AAudioStreamState::PAUSED) {
        LOGE("Cannot flush stream in state: %d", static_cast<int>(state_));
        return false;
    }

    setState(AAudioStreamState::FLUSHING);

    aaudio_result_t result = AAudioStream_requestFlush(stream_);
    if (result != AAUDIO_OK) {
        LOGE("Failed to flush stream: %s", AAudio_convertResultToText(result));
        setState(state_ == AAudioStreamState::STARTED ? AAudioStreamState::STARTED : AAudioStreamState::PAUSED);
        return false;
    }

    setState(AAudioStreamState::FLUSHED);

    LOGI("AAudio stream flushed successfully");
    return true;
}

bool AAudioStream::stop() {
    LOGI("Stopping AAudio stream");

    if (state_ != AAudioStreamState::STARTED && state_ != AAudioStreamState::PAUSED) {
        LOGE("Cannot stop stream in state: %d", static_cast<int>(state_));
        return false;
    }

    setState(AAudioStreamState::STOPPING);

    // Stop callback thread
    if (callbackThread_.joinable()) {
        running_ = false;
        callbackThread_.join();
    }

    aaudio_result_t result = AAudioStream_requestStop(stream_);
    if (result != AAUDIO_OK) {
        LOGE("Failed to stop stream: %s", AAudio_convertResultToText(result));
        setState(state_ == AAudioStreamState::STARTED ? AAudioStreamState::STARTED : AAudioStreamState::PAUSED);
        return false;
    }

    running_ = false;
    setState(AAudioStreamState::STOPPED);

    LOGI("AAudio stream stopped successfully");
    return true;
}

bool AAudioStream::close() {
    LOGI("Closing AAudio stream");

    if (state_ == AAudioStreamState::CLOSED) {
        LOGW("Stream already closed");
        return true;
    }

    // Stop if running
    if (state_ == AAudioStreamState::STARTED || state_ == AAudioStreamState::PAUSED) {
        stop();
    }

    setState(AAudioStreamState::CLOSING);

    if (stream_) {
        AAudioStream_close(stream_);
        stream_ = nullptr;
    }

    if (builder_) {
        AAudioStreamBuilder_delete(builder_);
        builder_ = nullptr;
    }

    // Clear buffers
    inputBuffer_.clear();
    outputBuffer_.clear();

    setState(AAudioStreamState::CLOSED);

    LOGI("AAudio stream closed successfully");
    return true;
}

AAudioStreamState AAudioStream::getState() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return state_;
}

int32_t AAudioStream::read(float* buffer, int32_t numFrames) {
    if (config_.direction != AAUDIO_DIRECTION_INPUT) {
        LOGE("Cannot read from output stream");
        return 0;
    }

    if (state_ != AAudioStreamState::STARTED) {
        LOGE("Stream not started");
        return 0;
    }

    // Read from AAudio stream
    aaudio_result_t result = AAudioStream_read(stream_, buffer, numFrames, 0);
    if (result > 0) {
        framesRead_ += result;
        engine_->incrementFramesRead(result);
    } else if (result < 0) {
        LOGE("Failed to read from stream: %s", AAudio_convertResultToText(result));
    }

    return result;
}

int32_t AAudioStream::write(const float* buffer, int32_t numFrames) {
    if (config_.direction != AAUDIO_DIRECTION_OUTPUT) {
        LOGE("Cannot write to input stream");
        return 0;
    }

    if (state_ != AAudioStreamState::STARTED) {
        LOGE("Stream not started");
        return 0;
    }

    // Write to AAudio stream
    aaudio_result_t result = AAudioStream_write(stream_, buffer, numFrames, 0);
    if (result > 0) {
        framesWritten_ += result;
        engine_->incrementFramesWritten(result);
    } else if (result < 0) {
        LOGE("Failed to write to stream: %s", AAudio_convertResultToText(result));
    }

    return result;
}

int32_t AAudioStream::getSampleRate() const {
    return AAudioStream_getSampleRate(stream_);
}

int32_t AAudioStream::getChannelCount() const {
    return AAudioStream_getChannelCount(stream_);
}

int32_t AAudioStream::getFormat() const {
    return AAudioStream_getFormat(stream_);
}

int32_t AAudioStream::getBufferSize() const {
    return AAudioStream_getBufferSizeInFrames(stream_);
}

int32_t AAudioStream::getBufferCapacity() const {
    return AAudioStream_getBufferCapacityInFrames(stream_);
}

bool AAudioStream::setBufferSize(int32_t bufferSize) {
    aaudio_result_t result = AAudioStream_setBufferSizeInFrames(stream_, bufferSize);
    return result == AAUDIO_OK;
}

bool AAudioStream::updateBufferSize(int32_t bufferSize) {
    // For dynamic buffer size updates, we need to stop and restart the stream
    AAudioStreamState currentState = getState();
    if (currentState == AAudioStreamState::STARTED) {
        pause();
    }

    bool success = setBufferSize(bufferSize);

    if (currentState == AAudioStreamState::STARTED) {
        start();
    }

    return success;
}

int32_t AAudioStream::getXRunCount() const {
    return AAudioStream_getXRunCount(stream_);
}

double AAudioStream::getTimestamp(int64_t* framePosition) {
    int64_t position;
    int64_t time;

    aaudio_result_t result = AAudioStream_getTimestamp(stream_,
                                                      CLOCK_MONOTONIC,
                                                      &position,
                                                      &time);

    if (result == AAUDIO_OK && framePosition) {
        *framePosition = position;
        return time / 1e9; // Convert nanoseconds to seconds
    }

    return 0.0;
}

void AAudioStream::setState(AAudioStreamState newState) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    state_ = newState;
    stateCondition_.notify_all();
}

void AAudioStream::callbackLoop() {
    LOGI("Starting AAudio callback loop");

    while (running_) {
        // Process audio data in callback
        if (dataCallback_) {
            aaudio_data_callback_result_t result = dataCallback_(this, nullptr, bufferFrameSize_);
            if (result == AAUDIO_CALLBACK_RESULT_STOP) {
                break;
            }
        }

        // Sleep to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    LOGI("AAudio callback loop ended");
}

aaudio_data_callback_result_t AAudioStream::dataCallback(void* audioData, int32_t numFrames) {
    callbackCount_++;

    if (!running_ || !dataCallback_) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // Calculate average latency
    int64_t framePosition;
    double timestamp = getTimestamp(&framePosition);
    if (timestamp > 0) {
        double latency = (callbackCount_ * numFrames) / static_cast<double>(config_.sampleRate);
        averageLatency_ = (averageLatency_ * 0.9) + (latency * 0.1); // Running average
    }

    return dataCallback_(this, audioData, numFrames);
}

void AAudioStream::errorCallback(aaudio_result_t error) {
    LOGE("AAudio stream error: %s", AAudio_convertResultToText(error));

    if (errorCallback_) {
        errorCallback_(this, error);
    }

    // Handle specific errors
    switch (error) {
        case AAUDIO_ERROR_DISCONNECTED:
            setState(AAudioStreamState::DISCONNECTED);
            break;
        case AAUDIO_ERROR_INVALID_STATE:
            LOGE("Invalid stream state");
            break;
        case AAUDIO_ERROR_INVALID_FORMAT:
            LOGE("Invalid audio format");
            break;
        default:
            LOGE("Unknown AAudio error: %d", error);
            break;
    }
}

// Static callback functions
aaudio_data_callback_result_t AAudioStream::staticDataCallback(
    AAudioStream* stream,
    void* userData,
    void* audioData,
    int32_t numFrames) {

    if (!stream) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    return stream->dataCallback(audioData, numFrames);
}

void AAudioStream::staticErrorCallback(
    AAudioStream* stream,
    void* userData,
    aaudio_result_t error) {

    if (stream) {
        stream->errorCallback(error);
    }
}

// ========== AAUDIO MANAGER IMPLEMENTATION ==========
AAudioManager::AAudioManager(AAudioEngine* engine) : engine_(engine) {
    LOGI("AAudioManager constructor called");
}

AAudioManager::~AAudioManager() {
    LOGI("AAudioManager destructor called");
}

void AAudioManager::enumerateDevices() {
    LOGI("Enumerating AAudio devices");

    // Get output devices
    int32_t deviceCount = AAudio_getDeviceCount();
    for (int32_t i = 0; i < deviceCount; i++) {
        aaudio_device_id_t deviceId = AAudio_getDeviceId(i);

        // Check if device supports output
        if (AAudio_getDeviceDirection(deviceId) == AAUDIO_DIRECTION_OUTPUT) {
            outputDevices_.push_back(deviceId);
        }

        // Check if device supports input
        if (AAudio_getDeviceDirection(deviceId) == AAUDIO_DIRECTION_INPUT) {
            inputDevices_.push_back(deviceId);
        }

        // Query device capabilities
        queryDeviceCapabilities(deviceId, AAUDIO_DIRECTION_OUTPUT);
        queryDeviceCapabilities(deviceId, AAUDIO_DIRECTION_INPUT);
    }

    // Update default devices
    updateDefaultDevices();

    LOGI("Found %zu output devices, %zu input devices",
         outputDevices_.size(), inputDevices_.size());
}

void AAudioManager::queryDeviceCapabilities(aaudio_device_id_t deviceId, aaudio_direction_t direction) {
    DeviceCapabilities capabilities;
    capabilities.deviceId = deviceId;
    capabilities.direction = direction;

    // Query supported sample rates
    const std::vector<int32_t> commonRates = {8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 96000};
    for (int32_t rate : commonRates) {
        if (AAudio_isSampleRateSupported(deviceId, rate, direction)) {
            capabilities.sampleRates.push_back(rate);
        }
    }

    // Query supported channel counts
    const std::vector<int32_t> commonChannels = {1, 2, 4, 6, 8};
    for (int32_t channels : commonChannels) {
        if (AAudio_isChannelCountSupported(deviceId, channels, direction)) {
            capabilities.channelCounts.push_back(channels);
        }
    }

    // Query supported formats
    const std::vector<aaudio_format_t> formats = {
        AAUDIO_FORMAT_PCM_I16,
        AAUDIO_FORMAT_PCM_FLOAT,
        AAUDIO_FORMAT_PCM_I24_PACKED,
        AAUDIO_FORMAT_PCM_I32
    };

    for (aaudio_format_t format : formats) {
        if (AAudio_isFormatSupported(deviceId, format, direction)) {
            capabilities.formats.push_back(format);
        }
    }

    // Get buffer size limits
    capabilities.minBufferSize = AAudio_getMinimumBufferSize(deviceId, direction);
    capabilities.maxBufferSize = AAudio_getMaximumBufferSize(deviceId, direction);

    // Check if low latency
    capabilities.isLowLatency = AAudio_isLowLatencySupported(deviceId, direction);

    deviceCapabilities_[deviceId] = capabilities;
}

void AAudioManager::updateDefaultDevices() {
    defaultOutputDevice_ = AAudio_getDefaultDeviceId(AAUDIO_DIRECTION_OUTPUT);
    defaultInputDevice_ = AAudio_getDefaultDeviceId(AAUDIO_DIRECTION_INPUT);

    LOGI("Default output device: %ld, input device: %ld",
         static_cast<long>(defaultOutputDevice_), static_cast<long>(defaultInputDevice_));
}

const AAudioManager::DeviceCapabilities* AAudioManager::getDeviceCapabilities(aaudio_device_id_t deviceId) const {
    auto it = deviceCapabilities_.find(deviceId);
    if (it != deviceCapabilities_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool AAudioManager::isDeviceLowLatency(aaudio_device_id_t deviceId) const {
    const DeviceCapabilities* capabilities = getDeviceCapabilities(deviceId);
    return capabilities ? capabilities->isLowLatency : false;
}

aaudio_device_id_t AAudioManager::selectBestOutputDevice() const {
    // Prefer low-latency devices
    for (aaudio_device_id_t deviceId : outputDevices_) {
        if (isDeviceLowLatency(deviceId)) {
            return deviceId;
        }
    }

    // Fall back to default device
    return defaultOutputDevice_;
}

aaudio_device_id_t AAudioManager::selectBestInputDevice() const {
    // Prefer low-latency devices
    for (aaudio_device_id_t deviceId : inputDevices_) {
        if (isDeviceLowLatency(deviceId)) {
            return deviceId;
        }
    }

    // Fall back to default device
    return defaultInputDevice_;
}

aaudio_device_id_t AAudioManager::selectDeviceForSampleRate(int32_t sampleRate, int32_t channelCount) const {
    for (aaudio_device_id_t deviceId : outputDevices_) {
        const DeviceCapabilities* capabilities = getDeviceCapabilities(deviceId);
        if (capabilities) {
            // Check if device supports the required sample rate and channel count
            bool supportsRate = std::find(capabilities->sampleRates.begin(),
                                        capabilities->sampleRates.end(), sampleRate) != capabilities->sampleRates.end();
            bool supportsChannels = std::find(capabilities->channelCounts.begin(),
                                            capabilities->channelCounts.end(), channelCount) != capabilities->channelCounts.end();

            if (supportsRate && supportsChannels) {
                return deviceId;
            }
        }
    }

    return defaultOutputDevice_;
}

void AAudioManager::requestAudioFocus() {
    LOGI("Requesting audio focus");

    // In a real implementation, this would use AudioManager to request audio focus
    // For now, we'll just log the action

    LOGI("Audio focus requested");
}

void AAudioManager::abandonAudioFocus() {
    LOGI("Abandoning audio focus");

    // In a real implementation, this would use AudioManager to abandon audio focus
    // For now, we'll just log the action

    LOGI("Audio focus abandoned");
}

bool AAudioManager::isAudioFocusGranted() const {
    // In a real implementation, this would check if audio focus is granted
    // For now, we'll return true
    return true;
}

void AAudioManager::setPerformanceMode(aaudio_performance_mode_t mode) {
    LOGI("Setting performance mode: %d", mode);

    // In a real implementation, this would set the global performance mode
    // For now, we'll just log the action

    LOGI("Performance mode set");
}

aaudio_performance_mode_t AAudioManager::getPerformanceMode() const {
    // In a real implementation, this would get the current performance mode
    // For now, we'll return low latency
    return AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
}

void AAudioManager::setAllowedCapturePolicy(aaudio_allowed_capture_policy_t policy) {
    LOGI("Setting allowed capture policy: %d", policy);

    // In a real implementation, this would set the capture policy
    // For now, we'll just log the action

    LOGI("Capture policy set");
}

aaudio_allowed_capture_policy_t AAudioManager::getAllowedCapturePolicy() const {
    // In a real implementation, this would get the current capture policy
    // For now, we'll return allow capture by all
    return AAUDIO_ALLOWED_CAPTURE_BY_ALL;
}

// ========== AUDIO EFFECTS IMPLEMENTATION ==========
void AAudioEffects::applyReverb(float* buffer, int32_t frames, float roomSize, float damping, float wetLevel) {
    // Simple reverb implementation using delay lines
    // This is a basic implementation - a real reverb would be much more complex

    static std::vector<float> delayBuffer;
    static int delayIndex = 0;

    if (delayBuffer.size() != frames * 2) {
        delayBuffer.resize(frames * 2, 0.0f);
    }

    float feedback = damping * 0.8f;
    int delayLength = static_cast<int>(frames * roomSize * 0.1f);

    for (int32_t i = 0; i < frames; i++) {
        float input = buffer[i];
        float output = input;

        // Read from delay line
        if (delayIndex >= delayLength) {
            float delayed = delayBuffer[delayIndex - delayLength];
            output += delayed * feedback;
        }

        // Write to delay line
        delayBuffer[delayIndex] = input + output * feedback;
        delayIndex = (delayIndex + 1) % delayBuffer.size();

        // Mix wet/dry
        buffer[i] = input * (1.0f - wetLevel) + output * wetLevel;
    }
}

void AAudioEffects::applyEqualizer(float* buffer, int32_t frames, const float* bandGains, int32_t numBands) {
    // Multi-band equalizer implementation using biquad filters
    // This implements a proper parametric equalizer with multiple frequency bands

    if (numBands < 1 || !bandGains) {
        return;
    }

    // Define frequency bands (in Hz)
    const float bandFrequencies[] = {60.0f, 150.0f, 400.0f, 1000.0f, 2500.0f, 6000.0f, 15000.0f};
    const int maxBands = sizeof(bandFrequencies) / sizeof(bandFrequencies[0]);

    // Limit to available bands
    int actualBands = std::min(numBands, maxBands);

    // Process each band
    for (int band = 0; band < actualBands; ++band) {
        float frequency = bandFrequencies[band];
        float gain = bandGains[band];
        float qFactor = 1.414f; // Default Q factor for shelf/bell filters

        // Apply biquad filter for this band
        applyBiquadFilter(buffer, frames, frequency, gain, qFactor, 44100.0f);
    }
}

void AAudioEffects::applyBiquadFilter(float* buffer, int32_t frames, float frequency, float gain, float qFactor, float sampleRate) {
    // Biquad filter implementation for equalizer bands
    float omega = 2.0f * M_PI * frequency / sampleRate;
    float alpha = sinf(omega) / (2.0f * qFactor);
    float A = powf(10.0f, gain / 40.0f); // Convert dB to linear gain

    // Calculate filter coefficients for peaking EQ
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cosf(omega);
    float a2 = 1.0f - alpha / A;
    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cosf(omega);
    float b2 = 1.0f - alpha * A;

    // Normalize coefficients
    a1 /= a0;
    a2 /= a0;
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;

    // Apply filter using direct form I
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;

    for (int32_t i = 0; i < frames; ++i) {
        float x0 = buffer[i];
        float y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        // Update delay elements
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = y0;

        buffer[i] = y0;
    }
}

void AAudioEffects::applyCompressor(float* buffer, int32_t frames, float threshold, float ratio, float attackTime, float releaseTime) {
    // Simple compressor implementation
    // This is a basic implementation - real compressor would be more sophisticated

    float attackCoeff = 1.0f - expf(-1.0f / (attackTime * 44100.0f));
    float releaseCoeff = 1.0f - expf(-1.0f / (releaseTime * 44100.0f));

    float envelope = 0.0f;

    for (int32_t i = 0; i < frames; i++) {
        float input = fabsf(buffer[i]);

        // Update envelope
        if (input > envelope) {
            envelope += attackCoeff * (input - envelope);
        } else {
            envelope += releaseCoeff * (input - envelope);
        }

        // Apply compression
        if (envelope > threshold) {
            float compressionAmount = (envelope - threshold) / envelope;
            compressionAmount = powf(compressionAmount, 1.0f / ratio);
            buffer[i] *= compressionAmount;
        }
    }
}

void AAudioEffects::applyLimiter(float* buffer, int32_t frames, float threshold, float releaseTime) {
    // Simple limiter implementation
    float releaseCoeff = 1.0f - expf(-1.0f / (releaseTime * 44100.0f));
    float envelope = 0.0f;

    for (int32_t i = 0; i < frames; i++) {
        float input = fabsf(buffer[i]);

        // Update envelope
        if (input > envelope) {
            envelope = input;
        } else {
            envelope += releaseCoeff * (input - envelope);
        }

        // Apply limiting
        if (envelope > threshold) {
            buffer[i] *= threshold / envelope;
        }
    }
}

void AAudioEffects::applyChorus(float* buffer, int32_t frames, float rate, float depth, float mix) {
    // Simple chorus implementation using delay modulation
    // This is a basic implementation

    static std::vector<float> delayBuffer;
    static int delayIndex = 0;

    if (delayBuffer.size() != frames * 4) {
        delayBuffer.resize(frames * 4, 0.0f);
    }

    float modulationRate = rate * 2.0f * M_PI / 44100.0f;
    float modulationDepth = depth * 44100.0f / 1000.0f; // Convert ms to samples

    for (int32_t i = 0; i < frames; i++) {
        float input = buffer[i];

        // Calculate modulation
        float modulation = sinf(static_cast<float>(i) * modulationRate) * modulationDepth;
        int delayOffset = static_cast<int>(modulation);

        // Read from delay line with modulation
        int readIndex = (delayIndex - delayOffset - 441) % delayBuffer.size();
        if (readIndex < 0) readIndex += delayBuffer.size();

        float delayed = delayBuffer[readIndex];

        // Mix with original
        buffer[i] = input * (1.0f - mix) + delayed * mix;

        // Write to delay line
        delayBuffer[delayIndex] = input;
        delayIndex = (delayIndex + 1) % delayBuffer.size();
    }
}

void AAudioEffects::applyFlanger(float* buffer, int32_t frames, float rate, float depth, float feedback, float mix) {
    // Simple flanger implementation
    // This is a basic implementation

    static std::vector<float> delayBuffer;
    static int delayIndex = 0;

    if (delayBuffer.size() != frames * 2) {
        delayBuffer.resize(frames * 2, 0.0f);
    }

    float modulationRate = rate * 2.0f * M_PI / 44100.0f;
    float modulationDepth = depth * 44100.0f / 1000.0f;

    for (int32_t i = 0; i < frames; i++) {
        float input = buffer[i];

        // Calculate modulation
        float modulation = sinf(static_cast<float>(i) * modulationRate) * modulationDepth;
        int delayOffset = static_cast<int>(modulation);

        // Read from delay line
        int readIndex = (delayIndex - delayOffset - 44) % delayBuffer.size();
        if (readIndex < 0) readIndex += delayBuffer.size();

        float delayed = delayBuffer[readIndex];

        // Apply feedback
        float output = input + delayed * feedback;

        // Mix with original
        buffer[i] = input * (1.0f - mix) + output * mix;

        // Write to delay line
        delayBuffer[delayIndex] = output;
        delayIndex = (delayIndex + 1) % delayBuffer.size();
    }
}

void AAudioEffects::applyDelay(float* buffer, int32_t frames, float delayTime, float feedback, float mix) {
    // Simple delay implementation
    static std::vector<float> delayBuffer;
    static int delayIndex = 0;

    int delaySamples = static_cast<int>(delayTime * 44100.0f);
    if (delayBuffer.size() != delaySamples * 2) {
        delayBuffer.resize(delaySamples * 2, 0.0f);
    }

    for (int32_t i = 0; i < frames; i++) {
        float input = buffer[i];

        // Read from delay line
        int readIndex = (delayIndex - delaySamples) % delayBuffer.size();
        if (readIndex < 0) readIndex += delayBuffer.size();

        float delayed = delayBuffer[readIndex];

        // Apply feedback
        float output = input + delayed * feedback;

        // Mix with original
        buffer[i] = input * (1.0f - mix) + output * mix;

        // Write to delay line
        delayBuffer[delayIndex] = output;
        delayIndex = (delayIndex + 1) % delayBuffer.size();
    }
}

void AAudioEffects::applyPitchShift(float* buffer, int32_t frames, float pitchRatio) {
    // Simple pitch shift implementation using resampling
    // This is a basic implementation - real pitch shifting is much more complex

    if (pitchRatio == 1.0f) {
        return; // No pitch change
    }

    // For simplicity, just scale the frequency content
    // A real implementation would use phase vocoding or similar techniques

    float scale = 1.0f / pitchRatio;
    for (int32_t i = 0; i < frames; i++) {
        int32_t srcIndex = static_cast<int32_t>(i * scale);
        if (srcIndex < frames) {
            buffer[i] = buffer[srcIndex];
        }
    }
}

void AAudioEffects::applyLowPassFilter(float* buffer, int32_t frames, float cutoffFreq, float sampleRate) {
    // Simple first-order low-pass filter
    float rc = 1.0f / (2.0f * M_PI * cutoffFreq);
    float dt = 1.0f / sampleRate;
    float alpha = rc / (rc + dt);

    float previousOutput = buffer[0];

    for (int32_t i = 1; i < frames; i++) {
        float output = alpha * buffer[i] + (1.0f - alpha) * previousOutput;
        previousOutput = output;
        buffer[i] = output;
    }
}

void AAudioEffects::applyHighPassFilter(float* buffer, int32_t frames, float cutoffFreq, float sampleRate) {
    // Simple first-order high-pass filter
    float rc = 1.0f / (2.0f * M_PI * cutoffFreq);
    float dt = 1.0f / sampleRate;
    float alpha = rc / (rc + dt);

    float previousInput = buffer[0];
    float previousOutput = buffer[0];

    for (int32_t i = 1; i < frames; i++) {
        float output = alpha * (previousOutput + buffer[i] - previousInput);
        previousInput = buffer[i];
        previousOutput = output;
        buffer[i] = output;
    }
}

void AAudioEffects::applyBandPassFilter(float* buffer, int32_t frames, float lowFreq, float highFreq, float sampleRate) {
    // Simple band-pass filter using low-pass and high-pass combination
    std::vector<float> tempBuffer(buffer, buffer + frames);

    // Apply low-pass
    applyLowPassFilter(tempBuffer.data(), frames, highFreq, sampleRate);

    // Apply high-pass
    applyHighPassFilter(tempBuffer.data(), frames, lowFreq, sampleRate);

    // Copy result back
    std::copy(tempBuffer.begin(), tempBuffer.end(), buffer);
}

void AAudioEffects::applyNotchFilter(float* buffer, int32_t frames, float centerFreq, float qFactor, float sampleRate) {
    // Simple notch filter implementation
    // This is a basic implementation

    float omega = 2.0f * M_PI * centerFreq / sampleRate;
    float alpha = sinf(omega) / (2.0f * qFactor);

    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosf(omega);
    float a2 = 1.0f - alpha;
    float b0 = 1.0f;
    float b1 = -2.0f * cosf(omega);
    float b2 = 1.0f;

    // Normalize coefficients
    a1 /= a0;
    a2 /= a0;
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;

    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;

    for (int32_t i = 0; i < frames; i++) {
        float x0 = buffer[i];
        float y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        buffer[i] = y0;

        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = y0;
    }
}

// ========== SPATIAL AUDIO IMPLEMENTATION ==========
AAudioSpatializer::AAudioSpatializer() : speedOfSound_(343.0f) {
    LOGI("AAudioSpatializer constructor called");
}

AAudioSpatializer::~AAudioSpatializer() {
    LOGI("AAudioSpatializer destructor called");
}

int32_t AAudioSpatializer::addSource(float x, float y, float z) {
    Source source;
    source.x = x;
    source.y = y;
    source.z = z;
    source.vx = source.vy = source.vz = 0.0f;
    source.volume = 1.0f;
    source.is3D = true;
    source.minDistance = 1.0f;
    source.maxDistance = 100.0f;
    source.rolloffFactor = 1.0f;
    source.dopplerFactor = 1.0f;

    sources_.push_back(source);
    return static_cast<int32_t>(sources_.size() - 1);
}

void AAudioSpatializer::removeSource(int32_t sourceId) {
    if (sourceId >= 0 && sourceId < static_cast<int32_t>(sources_.size())) {
        sources_.erase(sources_.begin() + sourceId);
    }
}

void AAudioSpatializer::setSourcePosition(int32_t sourceId, float x, float y, float z) {
    if (sourceId >= 0 && sourceId < static_cast<int32_t>(sources_.size())) {
        sources_[sourceId].x = x;
        sources_[sourceId].y = y;
        sources_[sourceId].z = z;
    }
}

void AAudioSpatializer::setSourceVelocity(int32_t sourceId, float vx, float vy, float vz) {
    if (sourceId >= 0 && sourceId < static_cast<int32_t>(sources_.size())) {
        sources_[sourceId].vx = vx;
        sources_[sourceId].vy = vy;
        sources_[sourceId].vz = vz;
    }
}

void AAudioSpatializer::setSourceVolume(int32_t sourceId, float volume) {
    if (sourceId >= 0 && sourceId < static_cast<int32_t>(sources_.size())) {
        sources_[sourceId].volume = std::max(0.0f, std::min(1.0f, volume));
    }
}

void AAudioSpatializer::setSourceDistance(int32_t sourceId, float minDistance, float maxDistance) {
    if (sourceId >= 0 && sourceId < static_cast<int32_t>(sources_.size())) {
        sources_[sourceId].minDistance = minDistance;
        sources_[sourceId].maxDistance = maxDistance;
    }
}

void AAudioSpatializer::setSourceRolloff(int32_t sourceId, float rolloffFactor) {
    if (sourceId >= 0 && sourceId < static_cast<int32_t>(sources_.size())) {
        sources_[sourceId].rolloffFactor = rolloffFactor;
    }
}

void AAudioSpatializer::setSourceDoppler(int32_t sourceId, float dopplerFactor) {
    if (sourceId >= 0 && sourceId < static_cast<int32_t>(sources_.size())) {
        sources_[sourceId].dopplerFactor = dopplerFactor;
    }
}

void AAudioSpatializer::setListenerPosition(float x, float y, float z) {
    listener_.x = x;
    listener_.y = y;
    listener_.z = z;
}

void AAudioSpatializer::setListenerVelocity(float vx, float vy, float vz) {
    listener_.vx = vx;
    listener_.vy = vy;
    listener_.vz = vz;
}

void AAudioSpatializer::setListenerOrientation(float fx, float fy, float fz, float ux, float uy, float uz) {
    listener_.fx = fx;
    listener_.fy = fy;
    listener_.fz = fz;
    listener_.ux = ux;
    listener_.uy = uy;
    listener_.uz = uz;
}

void AAudioSpatializer::setListenerVolume(float volume) {
    listener_.volume = std::max(0.0f, std::min(1.0f, volume));
}

void AAudioSpatializer::setSpeedOfSound(float speed) {
    speedOfSound_ = speed;
}

void AAudioSpatializer::processSpatialAudio(float* buffer, int32_t frames, int32_t channels, float sampleRate) {
    if (channels < 2) {
        return; // Need at least stereo
    }

    // Process each source
    for (size_t i = 0; i < sources_.size(); i++) {
        Source& source = sources_[i];

        if (!source.is3D) {
            continue;
        }

        // Calculate distance and attenuation
        float distance = calculateDistance(source);
        float volume, lowPass;
        calculateAttenuation(source, distance, volume, lowPass);

        // Apply volume and filtering
        for (int32_t frame = 0; frame < frames; frame++) {
            int32_t sampleIndex = frame * channels;

            // Apply volume
            buffer[sampleIndex] *= volume * listener_.volume * source.volume;     // Left channel
            buffer[sampleIndex + 1] *= volume * listener_.volume * source.volume; // Right channel

            // Apply low-pass filter (simplified)
            if (lowPass < 1.0f) {
                // Simple low-pass filtering
                static float prevLeft = 0.0f, prevRight = 0.0f;
                float alpha = lowPass;
                buffer[sampleIndex] = alpha * buffer[sampleIndex] + (1.0f - alpha) * prevLeft;
                buffer[sampleIndex + 1] = alpha * buffer[sampleIndex + 1] + (1.0f - alpha) * prevRight;
                prevLeft = buffer[sampleIndex];
                prevRight = buffer[sampleIndex + 1];
            }
        }
    }
}

void AAudioSpatializer::calculateAttenuation(Source& source, float distance, float& volume, float& lowPass) {
    // Distance-based attenuation
    if (distance <= source.minDistance) {
        volume = 1.0f;
        lowPass = 1.0f;
    } else if (distance >= source.maxDistance) {
        volume = 0.0f;
        lowPass = 0.0f;
    } else {
        // Linear rolloff
        float normalizedDistance = (distance - source.minDistance) / (source.maxDistance - source.minDistance);
        volume = 1.0f - normalizedDistance * source.rolloffFactor;

        // Low-pass filter based on distance
        lowPass = 1.0f - normalizedDistance * 0.5f;
    }

    volume = std::max(0.0f, std::min(1.0f, volume));
    lowPass = std::max(0.0f, std::min(1.0f, lowPass));
}

void AAudioSpatializer::calculateDopplerShift(Source& source, float& pitchShift) {
    // Calculate relative velocity for Doppler effect
    float relativeVelocity = (source.vx - listener_.vx) * listener_.fx +
                           (source.vy - listener_.vy) * listener_.fy +
                           (source.vz - listener_.vz) * listener_.fz;

    // Calculate Doppler shift
    float speedRatio = (speedOfSound_ + relativeVelocity) / speedOfSound_;
    pitchShift = 1.0f / speedRatio;
}

void AAudioSpatializer::applyHRTF(Source& source, float* leftChannel, float* rightChannel, int32_t frames) {
    // Simplified HRTF implementation
    // Real HRTF would use complex head-related transfer functions

    float distance = calculateDistance(source);

    for (int32_t i = 0; i < frames; i++) {
        // Simple panning based on source position
        float angle = atan2f(source.z - listener_.z, source.x - listener_.x);
        float pan = sinf(angle * 0.5f); // Convert to -1 to 1 range

        // Apply interaural time difference (simplified)
        float itd = distance * 0.0001f; // Approximate ITD in samples

        // Mix channels based on pan
        float leftGain = 0.5f - pan * 0.5f;
        float rightGain = 0.5f + pan * 0.5f;

        leftChannel[i] *= leftGain;
        rightChannel[i] *= rightGain;
    }
}

float AAudioSpatializer::calculateDistance(const Source& source) const {
    float dx = source.x - listener_.x;
    float dy = source.y - listener_.y;
    float dz = source.z - listener_.z;

    return sqrtf(dx * dx + dy * dy + dz * dz);
}

} // namespace FoundryEngine
