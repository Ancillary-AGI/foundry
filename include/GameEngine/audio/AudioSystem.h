/**
 * @file AudioSystem.h
 * @brief 3D spatial audio system with procedural generation
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace FoundryEngine {

/**
 * @class AdvancedAudioSystem
 * @brief Comprehensive audio system with 3D spatial audio and procedural generation
 */
class AdvancedAudioSystem : public System {
public:
    struct AudioConfig {
        uint32_t sampleRate = 48000;
        uint32_t bufferSize = 512;
        uint32_t channels = 2;
        bool enableHRTF = true;
        bool enableReverb = true;
        bool enableDoppler = true;
        bool enableOcclusion = true;
        float masterVolume = 1.0f;
    };

    struct SpatialAudioConfig {
        float speedOfSound = 343.0f;
        float dopplerFactor = 1.0f;
        float rolloffFactor = 1.0f;
        float maxDistance = 1000.0f;
        float referenceDistance = 1.0f;
    };

    struct ReverbConfig {
        float roomSize = 0.5f;
        float damping = 0.5f;
        float wetLevel = 0.3f;
        float dryLevel = 0.7f;
        float preDelay = 0.02f;
        float width = 1.0f;
    };

    AdvancedAudioSystem();
    ~AdvancedAudioSystem();

    bool initialize(const AudioConfig& config = AudioConfig{}) override;
    void shutdown() override;
    void update(float deltaTime) override;

    // Audio source management
    uint32_t createAudioSource(const std::string& audioFile);
    uint32_t createStreamingSource(const std::string& audioFile);
    uint32_t createProceduralSource(const std::string& generatorType);
    void destroyAudioSource(uint32_t sourceId);

    // Playback control
    void play(uint32_t sourceId, bool loop = false);
    void pause(uint32_t sourceId);
    void stop(uint32_t sourceId);
    void setVolume(uint32_t sourceId, float volume);
    void setPitch(uint32_t sourceId, float pitch);
    void setPosition(uint32_t sourceId, const Vector3& position);
    void setVelocity(uint32_t sourceId, const Vector3& velocity);

    // 3D spatial audio
    void setListenerPosition(const Vector3& position);
    void setListenerOrientation(const Vector3& forward, const Vector3& up);
    void setListenerVelocity(const Vector3& velocity);
    void setSpatialConfig(const SpatialAudioConfig& config);

    // Environmental audio
    void setReverb(const ReverbConfig& config);
    void setOcclusion(uint32_t sourceId, float occlusionFactor);
    void setObstruction(uint32_t sourceId, float obstructionFactor);
    void setEnvironmentPreset(const std::string& presetName);

    // Procedural audio generation
    uint32_t generateTone(float frequency, float duration, const std::string& waveform = "sine");
    uint32_t generateNoise(const std::string& noiseType, float duration);
    uint32_t generateMusic(const std::string& style, float duration, const std::string& key = "C");
    void setProceduralParameter(uint32_t sourceId, const std::string& parameter, float value);

    // Audio effects
    void addEffect(uint32_t sourceId, const std::string& effectType);
    void removeEffect(uint32_t sourceId, const std::string& effectType);
    void setEffectParameter(uint32_t sourceId, const std::string& effectType, 
                           const std::string& parameter, float value);

    // Voice processing
    uint32_t createVoiceProcessor();
    void processVoiceInput(uint32_t processorId, const std::vector<float>& audioData);
    std::string recognizeSpeech(uint32_t processorId);
    std::vector<float> synthesizeSpeech(const std::string& text, const std::string& voice = "default");

    // Audio analysis
    std::vector<float> getSpectrum(uint32_t sourceId, size_t fftSize = 1024);
    float getRMS(uint32_t sourceId);
    float getPeak(uint32_t sourceId);
    std::vector<float> getBeatDetection(uint32_t sourceId);

    // Performance monitoring
    float getCPUUsage() const;
    size_t getMemoryUsage() const;
    uint32_t getActiveSourceCount() const;
    float getLatency() const;

private:
    class AdvancedAudioSystemImpl;
    std::unique_ptr<AdvancedAudioSystemImpl> impl_;
};

/**
 * @class ProceduralAudioGenerator
 * @brief Generates audio content algorithmically
 */
class ProceduralAudioGenerator {
public:
    struct MusicConfig {
        std::string key = "C";
        std::string scale = "major";
        std::string style = "ambient";
        float tempo = 120.0f;
        float duration = 60.0f;
        std::vector<std::string> instruments;
    };

    struct SoundEffectConfig {
        std::string type = "explosion";
        float intensity = 1.0f;
        float duration = 2.0f;
        std::vector<std::string> layers;
    };

    ProceduralAudioGenerator();
    ~ProceduralAudioGenerator();

    // Music generation
    std::vector<float> generateMusic(const MusicConfig& config);
    std::vector<float> generateMelody(const std::string& key, const std::string& scale, 
                                     float duration, float tempo);
    std::vector<float> generateHarmony(const std::vector<float>& melody, const std::string& key);
    std::vector<float> generateRhythm(float tempo, float duration, const std::string& pattern);

    // Sound effect generation
    std::vector<float> generateSoundEffect(const SoundEffectConfig& config);
    std::vector<float> generateExplosion(float intensity, float duration);
    std::vector<float> generateFootsteps(const std::string& surface, float speed);
    std::vector<float> generateWeaponSound(const std::string& weaponType);
    std::vector<float> generateAmbientSound(const std::string& environment);

    // Synthesis methods
    std::vector<float> synthesizeWaveform(const std::string& waveform, float frequency, 
                                         float duration, float sampleRate = 48000);
    std::vector<float> applyEnvelope(const std::vector<float>& audio, float attack, 
                                    float decay, float sustain, float release);
    std::vector<float> applyFilter(const std::vector<float>& audio, const std::string& filterType, 
                                  float cutoff, float resonance = 1.0f);

private:
    class ProceduralAudioGeneratorImpl;
    std::unique_ptr<ProceduralAudioGeneratorImpl> impl_;
};

} // namespace FoundryEngine