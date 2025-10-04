/**
 * Windows Animation Implementation
 * Animation System for Windows Platform
 */

#include "WindowsPlatform.h"
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <string>

// ========== WINDOWS ANIMATION SYSTEM ==========
class WindowsAnimation : public PlatformAnimation {
private:
    std::vector<AnimationClip> clips_;
    std::vector<Bone> skeleton_;
    std::unordered_map<std::string, int> boneMap_;
    float currentTime_ = 0.0f;
    float playbackSpeed_ = 1.0f;
    bool isPlaying_ = false;
    std::string currentClip_;

public:
    WindowsAnimation() = default;

    bool initialize() override {
        // Initialize animation system
        return true;
    }

    void shutdown() override {
        clips_.clear();
        skeleton_.clear();
        boneMap_.clear();
    }

    void update(float deltaTime) override {
        if (!isPlaying_) return;

        currentTime_ += deltaTime * playbackSpeed_;

        // Update bone transformations
        updateBoneTransforms();
    }

    void play(const std::string& clipName, bool loop) override {
        auto it = std::find_if(clips_.begin(), clips_.end(),
            [&](const AnimationClip& clip) { return clip.name == clipName; });

        if (it != clips_.end()) {
            currentClip_ = clipName;
            currentTime_ = 0.0f;
            isPlaying_ = true;
        }
    }

    void pause() override {
        isPlaying_ = false;
    }

    void resume() override {
        isPlaying_ = true;
    }

    void stop() override {
        isPlaying_ = false;
        currentTime_ = 0.0f;
    }

    void setSpeed(float speed) override {
        playbackSpeed_ = speed;
    }

    float getSpeed() const override {
        return playbackSpeed_;
    }

    bool isPlaying() const override {
        return isPlaying_;
    }

    void addClip(const AnimationClip& clip) override {
        clips_.push_back(clip);
    }

    void removeClip(const std::string& clipName) override {
        clips_.erase(
            std::remove_if(clips_.begin(), clips_.end(),
                [&](const AnimationClip& clip) { return clip.name == clipName; }),
            clips_.end()
        );
    }

    void setSkeleton(const std::vector<Bone>& skeleton) override {
        skeleton_ = skeleton;
        buildBoneMap();
    }

    const std::vector<Bone>& getSkeleton() const override {
        return skeleton_;
    }

    std::vector<glm::mat4> getBoneTransforms() const override {
        std::vector<glm::mat4> transforms;
        for (const auto& bone : skeleton_) {
            transforms.push_back(bone.transform);
        }
        return transforms;
    }

private:
    void buildBoneMap() {
        boneMap_.clear();
        for (int i = 0; i < skeleton_.size(); i++) {
            boneMap_[skeleton_[i].name] = i;
        }
    }

    void updateBoneTransforms() {
        if (currentClip_.empty()) return;

        auto it = std::find_if(clips_.begin(), clips_.end(),
            [&](const AnimationClip& clip) { return clip.name == currentClip_; });

        if (it == clips_.end()) return;

        AnimationClip& clip = *it;

        // Sample keyframes based on current time
        for (auto& bone : skeleton_) {
            auto boneIt = boneMap_.find(bone.name);
            if (boneIt != boneMap_.end()) {
                int boneIndex = boneIt->second;

                // Find relevant keyframes for this bone
                auto& keyframes = clip.keyframes[boneIndex];

                // Interpolate between keyframes
                bone.transform = interpolateKeyframes(keyframes, currentTime_);
            }
        }
    }

    glm::mat4 interpolateKeyframes(const std::vector<Keyframe>& keyframes, float time) {
        if (keyframes.empty()) return glm::mat4(1.0f);

        // Find the two keyframes to interpolate between
        Keyframe* prevKeyframe = nullptr;
        Keyframe* nextKeyframe = nullptr;

        for (size_t i = 0; i < keyframes.size(); i++) {
            if (keyframes[i].time <= time) {
                prevKeyframe = &keyframes[i];
            }
            if (keyframes[i].time >= time && nextKeyframe == nullptr) {
                nextKeyframe = &keyframes[i];
                break;
            }
        }

        if (!prevKeyframe) {
            return keyframes[0].transform;
        }

        if (!nextKeyframe) {
            return prevKeyframe->transform;
        }

        // Linear interpolation
        float t = (time - prevKeyframe->time) / (nextKeyframe->time - prevKeyframe->time);
        t = glm::clamp(t, 0.0f, 1.0f);

        return glm::mix(prevKeyframe->transform, nextKeyframe->transform, t);
    }
};
