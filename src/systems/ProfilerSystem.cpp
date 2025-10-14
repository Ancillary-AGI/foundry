#include "../../include/GameEngine/systems/ProfilerSystem.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <memory>

namespace FoundryEngine {

// Default Profile Manager Implementation
class DefaultProfileManagerImpl : public SystemImplBase<DefaultProfileManagerImpl> {
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double, std::micro>;

    struct ProfileSample {
        std::string name;
        TimePoint startTime;
        Duration duration;
        uint32_t callCount = 0;
        Duration totalTime;
        Duration minTime = Duration::max();
        Duration maxTime = Duration::zero();
    };

    std::unordered_map<std::string, ProfileSample> samples_;
    std::vector<std::string> activeScopes_;
    TimePoint frameStartTime_;
    Duration frameTime_;
    uint32_t frameCount_ = 0;
    float fps_ = 0.0f;

    friend class SystemImplBase<DefaultProfileManagerImpl>;

    bool onInitialize() override {
        std::cout << "Default Profile Manager initialized" << std::endl;
        frameStartTime_ = Clock::now();
        return true;
    }

    void onShutdown() override {
        samples_.clear();
        activeScopes_.clear();
        std::cout << "Default Profile Manager shutdown" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Update FPS calculation
        frameCount_++;
        auto currentTime = Clock::now();
        auto totalTime = std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - frameStartTime_);
        fps_ = static_cast<float>(frameCount_) / totalTime.count();
    }

public:
    DefaultProfileManagerImpl() : SystemImplBase("DefaultProfileManager") {}

    std::string getStatistics() const override {
        char buffer[512];
        snprintf(buffer, sizeof(buffer),
                 "Profiler Stats - FPS: %.1f, Samples: %zu, Frame Time: %.2fms",
                 fps_, samples_.size(), frameTime_.count() * 1000.0);
        return std::string(buffer);
    }

    void beginFrame() {
        frameStartTime_ = Clock::now();
        activeScopes_.clear();
    }

    void endFrame() {
        auto frameEndTime = Clock::now();
        frameTime_ = std::chrono::duration_cast<Duration>(frameEndTime - frameStartTime_);
    }

    void beginScope(const std::string& name) {
        activeScopes_.push_back(name);

        auto it = samples_.find(name);
        if (it == samples_.end()) {
            samples_[name] = ProfileSample{name, Clock::now()};
        } else {
            it->second.startTime = Clock::now();
        }
    }

    void endScope(const std::string& name) {
        if (activeScopes_.empty() || activeScopes_.back() != name) {
            std::cerr << "Profile scope mismatch: " << name << std::endl;
            return;
        }

        activeScopes_.pop_back();

        auto it = samples_.find(name);
        if (it != samples_.end()) {
            auto endTime = Clock::now();
            auto duration = std::chrono::duration_cast<Duration>(endTime - it->second.startTime);

            it->second.duration = duration;
            it->second.callCount++;
            it->second.totalTime += duration;
            it->second.minTime = std::min(it->second.minTime, duration);
            it->second.maxTime = std::max(it->second.maxTime, duration);
        }
    }

    ProfileSample getSample(const std::string& name) const {
        auto it = samples_.find(name);
        return (it != samples_.end()) ? it->second : ProfileSample{name};
    }

    std::vector<std::string> getSampleNames() const {
        std::vector<std::string> names;
        for (const auto& pair : samples_) {
            names.push_back(pair.first);
        }
        return names;
    }

    void reset() {
        samples_.clear();
        activeScopes_.clear();
        frameCount_ = 0;
        fps_ = 0.0f;
        frameStartTime_ = Clock::now();
    }

    float getFPS() const { return fps_; }
    Duration getFrameTime() const { return frameTime_; }
    uint32_t getFrameCount() const { return frameCount_; }
};

DefaultProfileManager::DefaultProfileManager() : impl_(std::make_unique<DefaultProfileManagerImpl>()) {}
DefaultProfileManager::~DefaultProfileManager() = default;

bool DefaultProfileManager::initialize() { return impl_->initialize(); }
void DefaultProfileManager::shutdown() { impl_->shutdown(); }
void DefaultProfileManager::beginFrame() { impl_->beginFrame(); }
void DefaultProfileManager::endFrame() { impl_->endFrame(); }
void DefaultProfileManager::update(float deltaTime) { impl_->update(deltaTime); }

void DefaultProfileManager::beginScope(const std::string& name) { impl_->beginScope(name); }
void DefaultProfileManager::endScope(const std::string& name) { impl_->endScope(name); }
ProfileSample DefaultProfileManager::getSample(const std::string& name) const { return impl_->getSample(name); }
std::vector<std::string> DefaultProfileManager::getSampleNames() const { return impl_->getSampleNames(); }
void DefaultProfileManager::reset() { impl_->reset(); }
float DefaultProfileManager::getFPS() const { return impl_->getFPS(); }
std::chrono::duration<double, std::micro> DefaultProfileManager::getFrameTime() const { return impl_->getFrameTime(); }
uint32_t DefaultProfileManager::getFrameCount() const { return impl_->getFrameCount(); }

} // namespace FoundryEngine
