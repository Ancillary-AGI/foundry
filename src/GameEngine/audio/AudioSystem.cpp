/**
 * @file AudioSystem.cpp
 * @brief Implementation of 3D spatial audio system
 */

#include "GameEngine/audio/AudioSystem.h"
#include <algorithm>
#include <thread>
#include <cmath>

namespace FoundryEngine {

class AdvancedAudioSystem::AdvancedAudioSystemImpl {
public:
    AudioConfig config_;
    SpatialAudioConfig spatialConfig_;
    ReverbConfig reverbConfig_;
    
    // Audio sources
    std::vector<std::unique_ptr<AudioSource>> audioSources_;
    std::unordered_map<uint32_t, size_t> sourceIdToIndex_;
    std::atomic<uint32_t> nextSourceId_{1};
    
    // Listener properties
    Vector3 listenerPosition_{0, 0, 0};
    Vector3 listenerForward_{0, 0, -1};
    Vector3 listenerUp_{0, 1, 0};
    Vector3 listenerVelocity_{0, 0, 0};
    
    // Procedural audio generator
    std::unique_ptr<ProceduralAudioGenerator> proceduralGenerator_;
    
    // Voice processor
    std::unique_ptr<VoiceProcessor> voiceProcessor_;
    
    // Performance metrics
    std::atomic<float> cpuUsage_{0.0f};
    std::atomic<size_t> memoryUsage_{0};
    std::atomic<uint32_t> activeSourceCount_{0};
    std::atomic<float> latency_{0.0f};
    
    // Audio thread
    std::thread audioThread_;
    std::atomic<bool> isRunning_{false};
    
    mutable std::mutex sourcesMutex_;
};

AdvancedAudioSystem::AdvancedAudioSystem() 
    : impl_(std::make_unique<AdvancedAudioSystemImpl>()) {}

AdvancedAudioSystem::~AdvancedAudioSystem() = default;

bool AdvancedAudioSystem::initialize(const AudioConfig& config) {
    impl_->config_ = config;
    
    // Initialize spatial audio configuration
    impl_->spatialConfig_ = SpatialAudioConfig{};
    impl_->reverbConfig_ = ReverbConfig{};
    
    // Initialize procedural audio generator
    impl_->proceduralGenerator_ = std::make_unique<ProceduralAudioGenerator>();
    impl_->proceduralGenerator_->initialize();
    
    // Initialize voice processor
    if (config.enableVoiceProcessing) {
        impl_->voiceProcessor_ = std::make_unique<VoiceProcessor>();
        impl_->voiceProcessor_->initialize();
    }
    
    // Start audio processing thread
    impl_->isRunning_ = true;
    impl_->audioThread_ = std::thread([this]() { audioProcessingThread(); });
    
    return true;
}

void AdvancedAudioSystem::shutdown() {
    impl_->isRunning_ = false;
    
    if (impl_->audioThread_.joinable()) {
        impl_->audioThread_.join();
    }
    
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    impl_->audioSources_.clear();
    impl_->sourceIdToIndex_.clear();
    
    if (impl_->proceduralGenerator_) {
        impl_->proceduralGenerator_->shutdown();
    }
    
    if (impl_->voiceProcessor_) {
        impl_->voiceProcessor_->shutdown();
    }
}

void AdvancedAudioSystem::update(float deltaTime) {
    // Update 3D audio calculations
    update3DAudio();
    
    // Update procedural audio
    if (impl_->proceduralGenerator_) {
        impl_->proceduralGenerator_->update(deltaTime);
    }
    
    // Update voice processing
    if (impl_->voiceProcessor_) {
        impl_->voiceProcessor_->update(deltaTime);
    }
    
    // Update performance metrics
    updatePerformanceMetrics();
}

uint32_t AdvancedAudioSystem::createAudioSource(const std::string& audioFile) {
    auto audioSource = std::make_unique<AudioSource>();
    audioSource->id = impl_->nextSourceId_++;
    audioSource->audioFile = audioFile;
    audioSource->isStreaming = false;
    
    // Load audio data
    if (!loadAudioFile(audioFile, *audioSource)) {
        return INVALID_AUDIO_SOURCE_ID;
    }
    
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    uint32_t sourceId = audioSource->id;
    size_t index = impl_->audioSources_.size();
    
    impl_->audioSources_.push_back(std::move(audioSource));
    impl_->sourceIdToIndex_[sourceId] = index;
    
    return sourceId;
}

uint32_t AdvancedAudioSystem::createStreamingSource(const std::string& audioFile) {
    auto audioSource = std::make_unique<AudioSource>();
    audioSource->id = impl_->nextSourceId_++;
    audioSource->audioFile = audioFile;
    audioSource->isStreaming = true;
    
    // Set up streaming
    if (!setupAudioStreaming(audioFile, *audioSource)) {
        return INVALID_AUDIO_SOURCE_ID;
    }
    
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    uint32_t sourceId = audioSource->id;
    size_t index = impl_->audioSources_.size();
    
    impl_->audioSources_.push_back(std::move(audioSource));
    impl_->sourceIdToIndex_[sourceId] = index;
    
    return sourceId;
}

uint32_t AdvancedAudioSystem::createProceduralSource(const std::string& generatorType) {
    if (!impl_->proceduralGenerator_) {
        return INVALID_AUDIO_SOURCE_ID;
    }
    
    auto audioSource = std::make_unique<AudioSource>();
    audioSource->id = impl_->nextSourceId_++;
    audioSource->isProcedurallyGenerated = true;
    audioSource->proceduralType = generatorType;
    
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    uint32_t sourceId = audioSource->id;
    size_t index = impl_->audioSources_.size();
    
    impl_->audioSources_.push_back(std::move(audioSource));
    impl_->sourceIdToIndex_[sourceId] = index;
    
    return sourceId;
}

void AdvancedAudioSystem::destroyAudioSource(uint32_t sourceId) {
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    auto it = impl_->sourceIdToIndex_.find(sourceId);
    if (it != impl_->sourceIdToIndex_.end()) {
        size_t index = it->second;
        
        if (index < impl_->audioSources_.size()) {
            // Stop the source if playing
            if (impl_->audioSources_[index]->isPlaying) {
                impl_->audioSources_[index]->isPlaying = false;
            }
            
            // Remove from vector (swap with last element)
            if (index < impl_->audioSources_.size() - 1) {
                std::swap(impl_->audioSources_[index], impl_->audioSources_.back());
                
                // Update index mapping for swapped element
                uint32_t swappedId = impl_->audioSources_[index]->id;
                impl_->sourceIdToIndex_[swappedId] = index;
            }
            
            impl_->audioSources_.pop_back();
        }
        
        impl_->sourceIdToIndex_.erase(it);
    }
}

void AdvancedAudioSystem::play(uint32_t sourceId, bool loop) {
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    auto it = impl_->sourceIdToIndex_.find(sourceId);
    if (it != impl_->sourceIdToIndex_.end()) {
        size_t index = it->second;
        if (index < impl_->audioSources_.size()) {
            auto& source = impl_->audioSources_[index];
            source->isPlaying = true;
            source->isLooping = loop;
            source->playbackPosition = 0.0f;
            
            impl_->activeSourceCount_++;
        }
    }
}

void AdvancedAudioSystem::pause(uint32_t sourceId) {
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    auto it = impl_->sourceIdToIndex_.find(sourceId);
    if (it != impl_->sourceIdToIndex_.end()) {
        size_t index = it->second;
        if (index < impl_->audioSources_.size()) {
            impl_->audioSources_[index]->isPlaying = false;
            impl_->activeSourceCount_--;
        }
    }
}

void AdvancedAudioSystem::stop(uint32_t sourceId) {
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    auto it = impl_->sourceIdToIndex_.find(sourceId);
    if (it != impl_->sourceIdToIndex_.end()) {
        size_t index = it->second;
        if (index < impl_->audioSources_.size()) {
            auto& source = impl_->audioSources_[index];
            source->isPlaying = false;
            source->playbackPosition = 0.0f;
            impl_->activeSourceCount_--;
        }
    }
}

void AdvancedAudioSystem::setVolume(uint32_t sourceId, float volume) {
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    auto it = impl_->sourceIdToIndex_.find(sourceId);
    if (it != impl_->sourceIdToIndex_.end()) {
        size_t index = it->second;
        if (index < impl_->audioSources_.size()) {
            impl_->audioSources_[index]->volume = std::clamp(volume, 0.0f, 1.0f);
        }
    }
}

void AdvancedAudioSystem::setPitch(uint32_t sourceId, float pitch) {
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    auto it = impl_->sourceIdToIndex_.find(sourceId);
    if (it != impl_->sourceIdToIndex_.end()) {
        size_t index = it->second;
        if (index < impl_->audioSources_.size()) {
            impl_->audioSources_[index]->pitch = std::max(pitch, 0.1f);
        }
    }
}

void AdvancedAudioSystem::setPosition(uint32_t sourceId, const Vector3& position) {
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    auto it = impl_->sourceIdToIndex_.find(sourceId);
    if (it != impl_->sourceIdToIndex_.end()) {
        size_t index = it->second;
        if (index < impl_->audioSources_.size()) {
            impl_->audioSources_[index]->position = position;
        }
    }
}

void AdvancedAudioSystem::setVelocity(uint32_t sourceId, const Vector3& velocity) {
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    auto it = impl_->sourceIdToIndex_.find(sourceId);
    if (it != impl_->sourceIdToIndex_.end()) {
        size_t index = it->second;
        if (index < impl_->audioSources_.size()) {
            impl_->audioSources_[index]->velocity = velocity;
        }
    }
}

void AdvancedAudioSystem::setListenerPosition(const Vector3& position) {
    impl_->listenerPosition_ = position;
}

void AdvancedAudioSystem::setListenerOrientation(const Vector3& forward, const Vector3& up) {
    impl_->listenerForward_ = forward.normalized();
    impl_->listenerUp_ = up.normalized();
}

void AdvancedAudioSystem::setListenerVelocity(const Vector3& velocity) {
    impl_->listenerVelocity_ = velocity;
}

void AdvancedAudioSystem::setSpatialConfig(const SpatialAudioConfig& config) {
    impl_->spatialConfig_ = config;
}

void AdvancedAudioSystem::setReverb(const ReverbConfig& config) {
    impl_->reverbConfig_ = config;
}

uint32_t AdvancedAudioSystem::generateTone(float frequency, float duration, const std::string& waveform) {
    if (!impl_->proceduralGenerator_) {
        return INVALID_AUDIO_SOURCE_ID;
    }
    
    std::vector<float> audioData = impl_->proceduralGenerator_->synthesizeWaveform(
        waveform, frequency, duration, impl_->config_.sampleRate);
    
    auto audioSource = std::make_unique<AudioSource>();
    audioSource->id = impl_->nextSourceId_++;
    audioSource->isProcedurallyGenerated = true;
    audioSource->audioData = std::move(audioData);
    audioSource->sampleRate = impl_->config_.sampleRate;
    audioSource->channels = 1;
    
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    uint32_t sourceId = audioSource->id;
    size_t index = impl_->audioSources_.size();
    
    impl_->audioSources_.push_back(std::move(audioSource));
    impl_->sourceIdToIndex_[sourceId] = index;
    
    return sourceId;
}

uint32_t AdvancedAudioSystem::generateMusic(const std::string& style, float duration, const std::string& key) {
    if (!impl_->proceduralGenerator_) {
        return INVALID_AUDIO_SOURCE_ID;
    }
    
    ProceduralAudioGenerator::MusicConfig config;
    config.key = key;
    config.style = style;
    config.duration = duration;
    config.tempo = 120.0f;
    
    std::vector<float> audioData = impl_->proceduralGenerator_->generateMusic(config);
    
    auto audioSource = std::make_unique<AudioSource>();
    audioSource->id = impl_->nextSourceId_++;
    audioSource->isProcedurallyGenerated = true;
    audioSource->audioData = std::move(audioData);
    audioSource->sampleRate = impl_->config_.sampleRate;
    audioSource->channels = 2; // Stereo for music
    
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    uint32_t sourceId = audioSource->id;
    size_t index = impl_->audioSources_.size();
    
    impl_->audioSources_.push_back(std::move(audioSource));
    impl_->sourceIdToIndex_[sourceId] = index;
    
    return sourceId;
}

float AdvancedAudioSystem::getCPUUsage() const {
    return impl_->cpuUsage_.load();
}

size_t AdvancedAudioSystem::getMemoryUsage() const {
    return impl_->memoryUsage_.load();
}

uint32_t AdvancedAudioSystem::getActiveSourceCount() const {
    return impl_->activeSourceCount_.load();
}

float AdvancedAudioSystem::getLatency() const {
    return impl_->latency_.load();
}

void AdvancedAudioSystem::audioProcessingThread() {
    while (impl_->isRunning_) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Process audio for all active sources
        processAudioSources();
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        // Calculate CPU usage
        float frameTime = 1000000.0f / impl_->config_.sampleRate * impl_->config_.bufferSize;
        impl_->cpuUsage_ = (processingTime.count() / frameTime) * 100.0f;
        
        // Sleep for buffer duration
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(frameTime)));
    }
}

void AdvancedAudioSystem::processAudioSources() {
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    for (auto& source : impl_->audioSources_) {
        if (source && source->isPlaying) {
            processAudioSource(*source);
        }
    }
}

void AdvancedAudioSystem::processAudioSource(AudioSource& source) {
    // Calculate 3D audio parameters
    if (source.is3D) {
        calculate3DAudioParameters(source);
    }
    
    // Apply effects
    applyAudioEffects(source);
    
    // Update playback position
    float deltaTime = static_cast<float>(impl_->config_.bufferSize) / impl_->config_.sampleRate;
    source.playbackPosition += deltaTime * source.pitch;
    
    // Check for loop or end
    if (source.playbackPosition >= source.duration) {
        if (source.isLooping) {
            source.playbackPosition = 0.0f;
        } else {
            source.isPlaying = false;
            impl_->activeSourceCount_--;
        }
    }
}

void AdvancedAudioSystem::calculate3DAudioParameters(AudioSource& source) {
    // Calculate distance attenuation
    Vector3 sourceToListener = impl_->listenerPosition_ - source.position;
    float distance = sourceToListener.length();
    
    if (distance > 0.0f) {
        // Distance attenuation
        float attenuation = impl_->spatialConfig_.referenceDistance / 
                           (impl_->spatialConfig_.referenceDistance + 
                            impl_->spatialConfig_.rolloffFactor * (distance - impl_->spatialConfig_.referenceDistance));
        
        source.calculatedVolume = source.volume * attenuation;
        
        // Doppler effect
        if (impl_->config_.enableDoppler) {
            Vector3 relativeVelocity = source.velocity - impl_->listenerVelocity_;
            float dopplerShift = (impl_->spatialConfig_.speedOfSound - Vector3::dot(relativeVelocity, sourceToListener.normalized())) /
                                (impl_->spatialConfig_.speedOfSound + Vector3::dot(source.velocity, sourceToListener.normalized()));
            
            source.calculatedPitch = source.pitch * dopplerShift * impl_->spatialConfig_.dopplerFactor;
        }
        
        // HRTF calculation for stereo positioning
        if (impl_->config_.enableHRTF) {
            calculateHRTF(source, sourceToListener.normalized());
        }
    }
}

void AdvancedAudioSystem::calculateHRTF(AudioSource& source, const Vector3& direction) {
    // Simplified HRTF calculation
    // In a real implementation, this would use measured HRTF data
    
    Vector3 right = Vector3::cross(impl_->listenerForward_, impl_->listenerUp_);
    float azimuth = std::atan2(Vector3::dot(direction, right), Vector3::dot(direction, impl_->listenerForward_));
    
    // Simple panning based on azimuth
    source.leftGain = (1.0f - azimuth / (2.0f * M_PI)) * source.calculatedVolume;
    source.rightGain = (1.0f + azimuth / (2.0f * M_PI)) * source.calculatedVolume;
    
    // Clamp gains
    source.leftGain = std::clamp(source.leftGain, 0.0f, 1.0f);
    source.rightGain = std::clamp(source.rightGain, 0.0f, 1.0f);
}

void AdvancedAudioSystem::applyAudioEffects(AudioSource& source) {
    // Apply reverb if enabled
    if (impl_->config_.enableReverb) {
        applyReverb(source);
    }
    
    // Apply other effects
    for (const auto& effect : source.effects) {
        applyEffect(source, effect);
    }
}

void AdvancedAudioSystem::applyReverb(AudioSource& source) {
    // Simplified reverb implementation
    // In a real implementation, this would use proper reverb algorithms
    
    float reverbGain = impl_->reverbConfig_.wetLevel * source.calculatedVolume;
    source.reverbLevel = reverbGain;
}

void AdvancedAudioSystem::update3DAudio() {
    // Update 3D audio calculations for all sources
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    
    for (auto& source : impl_->audioSources_) {
        if (source && source->is3D) {
            calculate3DAudioParameters(*source);
        }
    }
}

void AdvancedAudioSystem::updatePerformanceMetrics() {
    // Update memory usage
    size_t totalMemory = 0;
    
    std::lock_guard<std::mutex> lock(impl_->sourcesMutex_);
    for (const auto& source : impl_->audioSources_) {
        if (source) {
            totalMemory += source->audioData.size() * sizeof(float);
        }
    }
    
    impl_->memoryUsage_ = totalMemory;
}

bool AdvancedAudioSystem::loadAudioFile(const std::string& filePath, AudioSource& source) {
    // Placeholder for audio file loading
    // In a real implementation, this would use audio libraries like libsndfile, FMOD, etc.
    
    // Generate dummy audio data for testing
    source.sampleRate = impl_->config_.sampleRate;
    source.channels = 2;
    source.duration = 5.0f; // 5 seconds
    
    size_t numSamples = static_cast<size_t>(source.duration * source.sampleRate * source.channels);
    source.audioData.resize(numSamples);
    
    // Generate a simple sine wave for testing
    for (size_t i = 0; i < numSamples; i += source.channels) {
        float t = static_cast<float>(i / source.channels) / source.sampleRate;
        float sample = std::sin(2.0f * M_PI * 440.0f * t) * 0.5f; // 440 Hz sine wave
        
        source.audioData[i] = sample;     // Left channel
        if (source.channels > 1) {
            source.audioData[i + 1] = sample; // Right channel
        }
    }
    
    return true;
}

bool AdvancedAudioSystem::setupAudioStreaming(const std::string& filePath, AudioSource& source) {
    // Placeholder for streaming setup
    // In a real implementation, this would set up streaming buffers
    
    source.sampleRate = impl_->config_.sampleRate;
    source.channels = 2;
    source.duration = 300.0f; // 5 minutes for streaming
    source.isStreaming = true;
    
    return true;
}

} // namespace FoundryEngine