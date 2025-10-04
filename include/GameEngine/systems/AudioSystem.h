#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "../math/Vector3.h"

namespace FoundryEngine {

class AudioClip;
class AudioSource;
class AudioListener;

enum class AudioFormat {
    WAV,
    MP3,
    OGG,
    FLAC
};

struct AudioSettings {
    int sampleRate = 44100;
    int channels = 2;
    int bufferSize = 1024;
    float masterVolume = 1.0f;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
    float voiceVolume = 1.0f;
    bool enableReverb = true;
    bool enable3D = true;
    float dopplerScale = 1.0f;
    float speedOfSound = 343.3f;
};

class AudioClip {
public:
    virtual ~AudioClip() = default;
    virtual bool load(const std::string& path) = 0;
    virtual void unload() = 0;
    virtual float getDuration() const = 0;
    virtual int getSampleRate() const = 0;
    virtual int getChannels() const = 0;
    virtual bool isLoaded() const = 0;
};

class AudioSource {
public:
    virtual ~AudioSource() = default;
    
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual bool isPlaying() const = 0;
    virtual bool isPaused() const = 0;
    
    virtual void setClip(AudioClip* clip) = 0;
    virtual AudioClip* getClip() const = 0;
    
    virtual void setVolume(float volume) = 0;
    virtual float getVolume() const = 0;
    
    virtual void setPitch(float pitch) = 0;
    virtual float getPitch() const = 0;
    
    virtual void setLoop(bool loop) = 0;
    virtual bool isLooping() const = 0;
    
    virtual void setTime(float time) = 0;
    virtual float getTime() const = 0;
    
    virtual void setPosition(const Vector3& position) = 0;
    virtual Vector3 getPosition() const = 0;
    
    virtual void setVelocity(const Vector3& velocity) = 0;
    virtual Vector3 getVelocity() const = 0;
    
    virtual void setMinDistance(float distance) = 0;
    virtual float getMinDistance() const = 0;
    
    virtual void setMaxDistance(float distance) = 0;
    virtual float getMaxDistance() const = 0;
    
    virtual void setRolloffMode(int mode) = 0;
    virtual int getRolloffMode() const = 0;
    
    virtual void setSpatialBlend(float blend) = 0;
    virtual float getSpatialBlend() const = 0;
};

class AudioListener {
public:
    virtual ~AudioListener() = default;
    
    virtual void setPosition(const Vector3& position) = 0;
    virtual Vector3 getPosition() const = 0;
    
    virtual void setVelocity(const Vector3& velocity) = 0;
    virtual Vector3 getVelocity() const = 0;
    
    virtual void setOrientation(const Vector3& forward, const Vector3& up) = 0;
    virtual void getOrientation(Vector3& forward, Vector3& up) const = 0;
};

class AudioManager {
public:
    virtual ~AudioManager() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;
    
    virtual void setSettings(const AudioSettings& settings) = 0;
    virtual AudioSettings getSettings() const = 0;
    
    virtual void setMasterVolume(float volume) = 0;
    virtual float getMasterVolume() const = 0;
    
    virtual void setMusicVolume(float volume) = 0;
    virtual float getMusicVolume() const = 0;
    
    virtual void setSFXVolume(float volume) = 0;
    virtual float getSFXVolume() const = 0;
    
    virtual void setVoiceVolume(float volume) = 0;
    virtual float getVoiceVolume() const = 0;
    
    virtual AudioClip* loadClip(const std::string& path) = 0;
    virtual void unloadClip(AudioClip* clip) = 0;
    virtual AudioClip* getClip(const std::string& name) = 0;
    
    virtual AudioSource* createSource() = 0;
    virtual void destroySource(AudioSource* source) = 0;
    
    virtual void playOneShot(AudioClip* clip, float volume = 1.0f) = 0;
    virtual void playOneShotAtPoint(AudioClip* clip, const Vector3& position, float volume = 1.0f) = 0;
    
    virtual void setListener(AudioListener* listener) = 0;
    virtual AudioListener* getListener() const = 0;
    
    virtual void playMusic(const std::string& path, bool loop = true, float fadeInTime = 0.0f) = 0;
    virtual void stopMusic(float fadeOutTime = 0.0f) = 0;
    virtual void pauseMusic() = 0;
    virtual void resumeMusic() = 0;
    virtual bool isMusicPlaying() const = 0;
    
    virtual void setReverbZone(const Vector3& center, float radius, float reverbLevel) = 0;
    virtual void removeReverbZone(const Vector3& center) = 0;
    
    virtual int getActiveSourceCount() const = 0;
    virtual int getMaxSources() const = 0;
    virtual void setMaxSources(int maxSources) = 0;
};

class OpenALAudioManager : public AudioManager {
public:
    bool initialize() override;
    void shutdown() override;
    void update() override;
    void setSettings(const AudioSettings& settings) override;
    AudioSettings getSettings() const override;
    void setMasterVolume(float volume) override;
    float getMasterVolume() const override;
    void setMusicVolume(float volume) override;
    float getMusicVolume() const override;
    void setSFXVolume(float volume) override;
    float getSFXVolume() const override;
    void setVoiceVolume(float volume) override;
    float getVoiceVolume() const override;
    AudioClip* loadClip(const std::string& path) override;
    void unloadClip(AudioClip* clip) override;
    AudioClip* getClip(const std::string& name) override;
    AudioSource* createSource() override;
    void destroySource(AudioSource* source) override;
    void playOneShot(AudioClip* clip, float volume) override;
    void playOneShotAtPoint(AudioClip* clip, const Vector3& position, float volume) override;
    void setListener(AudioListener* listener) override;
    AudioListener* getListener() const override;
    void playMusic(const std::string& path, bool loop, float fadeInTime) override;
    void stopMusic(float fadeOutTime) override;
    void pauseMusic() override;
    void resumeMusic() override;
    bool isMusicPlaying() const override;
    void setReverbZone(const Vector3& center, float radius, float reverbLevel) override;
    void removeReverbZone(const Vector3& center) override;
    int getActiveSourceCount() const override;
    int getMaxSources() const override;
    void setMaxSources(int maxSources) override;
};

} // namespace FoundryEngine