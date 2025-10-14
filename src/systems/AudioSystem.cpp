#include "../../include/GameEngine/systems/AudioSystem.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>

namespace FoundryEngine {

// OpenAL Audio Manager Implementation
class OpenALAudioManagerImpl : public SystemImplBase<OpenALAudioManagerImpl> {
private:
    AudioSettings settings_;
    float masterVolume_ = 1.0f;
    float musicVolume_ = 1.0f;
    float sfxVolume_ = 1.0f;
    float voiceVolume_ = 1.0f;

    std::unordered_map<std::string, std::unique_ptr<AudioClip>> clips_;
    std::vector<std::unique_ptr<AudioSource>> sources_;
    std::unique_ptr<AudioListener> listener_;

    AudioSource* musicSource_ = nullptr;
    bool musicPlaying_ = false;
    bool musicPaused_ = false;

    int maxSources_ = 32;
    int activeSourceCount_ = 0;

    friend class SystemImplBase<OpenALAudioManagerImpl>;

    bool onInitialize() override {
        // Initialize OpenAL context and device
        // In a real implementation, this would set up OpenAL
        std::cout << "OpenAL Audio Manager initialized" << std::endl;

        // Create default listener
        listener_ = std::make_unique<OpenALAudioListener>();

        return true;
    }

    void onShutdown() override {
        // Clean up OpenAL resources
        clips_.clear();
        sources_.clear();
        listener_.reset();
        std::cout << "OpenAL Audio Manager shutdown" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Update audio sources and handle 3D audio
        activeSourceCount_ = 0;
        for (auto& source : sources_) {
            if (source && source->isPlaying()) {
                activeSourceCount_++;
                // Update 3D audio calculations
            }
        }
    }

public:
    OpenALAudioManagerImpl() : SystemImplBase("OpenALAudioManager") {}

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Audio Stats - Sources: %d/%d active, Clips: %zu loaded",
                 activeSourceCount_, maxSources_, clips_.size());
        return std::string(buffer);
    }

    void setSettings(const AudioSettings& settings) { settings_ = settings; }
    AudioSettings getSettings() const { return settings_; }

    void setMasterVolume(float volume) { masterVolume_ = std::max(0.0f, std::min(1.0f, volume)); }
    float getMasterVolume() const { return masterVolume_; }

    void setMusicVolume(float volume) { musicVolume_ = std::max(0.0f, std::min(1.0f, volume)); }
    float getMusicVolume() const { return musicVolume_; }

    void setSFXVolume(float volume) { sfxVolume_ = std::max(0.0f, std::min(1.0f, volume)); }
    float getSFXVolume() const { return sfxVolume_; }

    void setVoiceVolume(float volume) { voiceVolume_ = std::max(0.0f, std::min(1.0f, volume)); }
    float getVoiceVolume() const { return voiceVolume_; }

    AudioClip* loadClip(const std::string& path) {
        auto clip = std::make_unique<OpenALAudioClip>();
        if (clip->load(path)) {
            AudioClip* clipPtr = clip.get();
            clips_[path] = std::move(clip);
            return clipPtr;
        }
        return nullptr;
    }

    void unloadClip(AudioClip* clip) {
        for (auto it = clips_.begin(); it != clips_.end(); ++it) {
            if (it->second.get() == clip) {
                clips_.erase(it);
                break;
            }
        }
    }

    AudioClip* getClip(const std::string& name) {
        auto it = clips_.find(name);
        return (it != clips_.end()) ? it->second.get() : nullptr;
    }

    AudioSource* createSource() {
        if (sources_.size() >= maxSources_) {
            return nullptr;
        }

        auto source = std::make_unique<OpenALAudioSource>();
        AudioSource* sourcePtr = source.get();
        sources_.push_back(std::move(source));
        return sourcePtr;
    }

    void destroySource(AudioSource* source) {
        sources_.erase(std::remove_if(sources_.begin(), sources_.end(),
            [source](const std::unique_ptr<AudioSource>& s) { return s.get() == source; }),
            sources_.end());
    }

    void playOneShot(AudioClip* clip, float volume) {
        if (!clip) return;

        AudioSource* source = createSource();
        if (source) {
            source->setClip(clip);
            source->setVolume(volume * masterVolume_);
            source->play();
        }
    }

    void playOneShotAtPoint(AudioClip* clip, const Vector3& position, float volume) {
        if (!clip) return;

        AudioSource* source = createSource();
        if (source) {
            source->setClip(clip);
            source->setVolume(volume * masterVolume_);
            source->setPosition(position);
            source->play();
        }
    }

    void setListener(AudioListener* listener) { listener_.reset(listener); }
    AudioListener* getListener() const { return listener_.get(); }

    void playMusic(const std::string& path, bool loop, float fadeInTime) {
        AudioClip* clip = loadClip(path);
        if (clip) {
            if (!musicSource_) {
                musicSource_ = createSource();
            }
            if (musicSource_) {
                musicSource_->setClip(clip);
                musicSource_->setLoop(loop);
                musicSource_->setVolume(musicVolume_ * masterVolume_);
                musicSource_->play();
                musicPlaying_ = true;
                musicPaused_ = false;
            }
        }
    }

    void stopMusic(float fadeOutTime) {
        if (musicSource_) {
            musicSource_->stop();
            musicPlaying_ = false;
            musicPaused_ = false;
        }
    }

    void pauseMusic() {
        if (musicSource_ && musicPlaying_) {
            musicSource_->pause();
            musicPaused_ = true;
        }
    }

    void resumeMusic() {
        if (musicSource_ && musicPaused_) {
            musicSource_->play();
            musicPaused_ = false;
        }
    }

    bool isMusicPlaying() const { return musicPlaying_ && !musicPaused_; }

    void setReverbZone(const Vector3& center, float radius, float reverbLevel) {
        // Implement reverb zones
    }

    void removeReverbZone(const Vector3& center) {
        // Remove reverb zone
    }

    int getActiveSourceCount() const { return activeSourceCount_; }
    int getMaxSources() const { return maxSources_; }
    void setMaxSources(int maxSources) { maxSources_ = std::max(1, maxSources); }
};

// OpenAL Audio Manager
OpenALAudioManager::OpenALAudioManager() : impl_(std::make_unique<OpenALAudioManagerImpl>()) {}

OpenALAudioManager::~OpenALAudioManager() = default;

bool OpenALAudioManager::initialize() { return impl_->initialize(); }
void OpenALAudioManager::shutdown() { impl_->shutdown(); }
void OpenALAudioManager::update(float deltaTime) { impl_->update(deltaTime); }
void OpenALAudioManager::setSettings(const AudioSettings& settings) { impl_->setSettings(settings); }
AudioSettings OpenALAudioManager::getSettings() const { return impl_->getSettings(); }
void OpenALAudioManager::setMasterVolume(float volume) { impl_->setMasterVolume(volume); }
float OpenALAudioManager::getMasterVolume() const { return impl_->getMasterVolume(); }
void OpenALAudioManager::setMusicVolume(float volume) { impl_->setMusicVolume(volume); }
float OpenALAudioManager::getMusicVolume() const { return impl_->getMusicVolume(); }
void OpenALAudioManager::setSFXVolume(float volume) { impl_->setSFXVolume(volume); }
float OpenALAudioManager::getSFXVolume() const { return impl_->getSFXVolume(); }
void OpenALAudioManager::setVoiceVolume(float volume) { impl_->setVoiceVolume(volume); }
float OpenALAudioManager::getVoiceVolume() const { return impl_->getVoiceVolume(); }
AudioClip* OpenALAudioManager::loadClip(const std::string& path) { return impl_->loadClip(path); }
void OpenALAudioManager::unloadClip(AudioClip* clip) { impl_->unloadClip(clip); }
AudioClip* OpenALAudioManager::getClip(const std::string& name) { return impl_->getClip(name); }
AudioSource* OpenALAudioManager::createSource() { return impl_->createSource(); }
void OpenALAudioManager::destroySource(AudioSource* source) { impl_->destroySource(source); }
void OpenALAudioManager::playOneShot(AudioClip* clip, float volume) { impl_->playOneShot(clip, volume); }
void OpenALAudioManager::playOneShotAtPoint(AudioClip* clip, const Vector3& position, float volume) { impl_->playOneShotAtPoint(clip, position, volume); }
void OpenALAudioManager::setListener(AudioListener* listener) { impl_->setListener(listener); }
AudioListener* OpenALAudioManager::getListener() const { return impl_->getListener(); }
void OpenALAudioManager::playMusic(const std::string& path, bool loop, float fadeInTime) { impl_->playMusic(path, loop, fadeInTime); }
void OpenALAudioManager::stopMusic(float fadeOutTime) { impl_->stopMusic(fadeOutTime); }
void OpenALAudioManager::pauseMusic() { impl_->pauseMusic(); }
void OpenALAudioManager::resumeMusic() { impl_->resumeMusic(); }
bool OpenALAudioManager::isMusicPlaying() const { return impl_->isMusicPlaying(); }
void OpenALAudioManager::setReverbZone(const Vector3& center, float radius, float reverbLevel) { impl_->setReverbZone(center, radius, reverbLevel); }
void OpenALAudioManager::removeReverbZone(const Vector3& center) { impl_->removeReverbZone(center); }
int OpenALAudioManager::getActiveSourceCount() const { return impl_->getActiveSourceCount(); }
int OpenALAudioManager::getMaxSources() const { return impl_->getMaxSources(); }
void OpenALAudioManager::setMaxSources(int maxSources) { impl_->setMaxSources(maxSources); }

// OpenAL Audio Manager
OpenALAudioManager::OpenALAudioManager() : impl_(std::make_unique<OpenALAudioManagerImpl>()) {}

OpenALAudioManager::~OpenALAudioManager() = default;

bool OpenALAudioManager::initialize() { return impl_->initialize(); }
void OpenALAudioManager::shutdown() { impl_->shutdown(); }
void OpenALAudioManager::update() { impl_->update(); }
void OpenALAudioManager::setSettings(const AudioSettings& settings) { impl_->setSettings(settings); }
AudioSettings OpenALAudioManager::getSettings() const { return impl_->getSettings(); }
void OpenALAudioManager::setMasterVolume(float volume) { impl_->setMasterVolume(volume); }
float OpenALAudioManager::getMasterVolume() const { return impl_->getMasterVolume(); }
void OpenALAudioManager::setMusicVolume(float volume) { impl_->setMusicVolume(volume); }
float OpenALAudioManager::getMusicVolume() const { return impl_->getMusicVolume(); }
void OpenALAudioManager::setSFXVolume(float volume) { impl_->setSFXVolume(volume); }
float OpenALAudioManager::getSFXVolume() const { return impl_->getSFXVolume(); }
void OpenALAudioManager::setVoiceVolume(float volume) { impl_->setVoiceVolume(volume); }
float OpenALAudioManager::getVoiceVolume() const { return impl_->getVoiceVolume(); }
AudioClip* OpenALAudioManager::loadClip(const std::string& path) { return impl_->loadClip(path); }
void OpenALAudioManager::unloadClip(AudioClip* clip) { impl_->unloadClip(clip); }
AudioClip* OpenALAudioManager::getClip(const std::string& name) { return impl_->getClip(name); }
AudioSource* OpenALAudioManager::createSource() { return impl_->createSource(); }
void OpenALAudioManager::destroySource(AudioSource* source) { impl_->destroySource(source); }
void OpenALAudioManager::playOneShot(AudioClip* clip, float volume) { impl_->playOneShot(clip, volume); }
void OpenALAudioManager::playOneShotAtPoint(AudioClip* clip, const Vector3& position, float volume) { impl_->playOneShotAtPoint(clip, position, volume); }
void OpenALAudioManager::setListener(AudioListener* listener) { impl_->setListener(listener); }
AudioListener* OpenALAudioManager::getListener() const { return impl_->getListener(); }
void OpenALAudioManager::playMusic(const std::string& path, bool loop, float fadeInTime) { impl_->playMusic(path, loop, fadeInTime); }
void OpenALAudioManager::stopMusic(float fadeOutTime) { impl_->stopMusic(fadeOutTime); }
void OpenALAudioManager::pauseMusic() { impl_->pauseMusic(); }
void OpenALAudioManager::resumeMusic() { impl_->resumeMusic(); }
bool OpenALAudioManager::isMusicPlaying() const { return impl_->isMusicPlaying(); }
void OpenALAudioManager::setReverbZone(const Vector3& center, float radius, float reverbLevel) { impl_->setReverbZone(center, radius, reverbLevel); }
void OpenALAudioManager::removeReverbZone(const Vector3& center) { impl_->removeReverbZone(center); }
int OpenALAudioManager::getActiveSourceCount() const { return impl_->getActiveSourceCount(); }
int OpenALAudioManager::getMaxSources() const { return impl_->getMaxSources(); }
void OpenALAudioManager::setMaxSources(int maxSources) { impl_->setMaxSources(maxSources); }

} // namespace FoundryEngine
