#ifndef FOUNDRY_GAMEENGINE_DOPPLER_EFFECT_H
#define FOUNDRY_GAMEENGINE_DOPPLER_EFFECT_H

#include "../../core/System.h"
#include "../../math/Vector3.h"

namespace FoundryEngine {

class DopplerEffect : public System {
public:
    // Audio source
    struct AudioSource {
        Vector3 position;
        Vector3 velocity;
        float baseFrequency;

        AudioSource(const Vector3& pos, const Vector3& vel, float freq) 
            : position(pos), velocity(vel), baseFrequency(freq) {}
    };

    // Listener
    struct Listener {
        Vector3 position;
        Vector3 velocity;
        Vector3 forward; // Facing direction

        Listener(const Vector3& pos = Vector3(0,0,0), const Vector3& vel = Vector3(0,0,0), const Vector3& fwd = Vector3(0,0,-1))
            : position(pos), velocity(vel), forward(fwd) {}
    };

    std::vector<AudioSource> sources_;
    Listener listener_;
    float speedOfSound_ = 343.0f; // m/s at sea level

    void addSource(const Vector3& pos, const Vector3& vel, float frequency) {
        sources_.emplace_back(pos, vel, frequency);
    }

    float computeDopplerFrequency(int sourceIndex) const {
        const AudioSource& src = sources_[sourceIndex];
        Vector3 toListener = listener_.position - src.position;
        float distance = toListener.magnitude();

        // Relative velocities
        Vector3 relativeVel = src.velocity - listener_.velocity;
        float dotProduct = relativeVel.dot(toListener.normalized());

        // Doppler factor: f' = f * (v + vr) / (v - vs)
        float vr = dotProduct; // Component towards listener
        float dopplerRatio = (speedOfSound_ + vr) / (speedOfSound_ - vr);

        // Avoid extreme values
        if (dopplerRatio < 0.1f || dopplerRatio > 10.0f) dopplerRatio = 1.0f;

        return src.baseFrequency * dopplerRatio;
    }

    float computeAmplitude(int sourceIndex, float referenceAmplitude = 1.0f) const {
        const AudioSource& src = sources_[sourceIndex];
        float distance = (src.position - listener_.position).magnitude();
        // Inverse square law, with minimum distance to avoid infinity
        return referenceAmplitude / (distance * distance + 1);
    }

    // Visual Doppler effect for light/shadows
    float computeLightDopplerShift(int sourceIndex, float baseWavelength) const {
        const AudioSource& src = sources_[sourceIndex];
        Vector3 toListener = listener_.position - src.position;
        Vector3 relativeVel = src.velocity - listener_.velocity;
        float vr = relativeVel.dot(toListener.normalized());

        // Relativistic Doppler for light: λ' = λ * sqrt((c + vr)/(c - vr))
        float c = 299792458.0f; // Speed of light
        float ratio = sqrt((c + vr) / (c - vr));

        // For non-rel speeds, simplified
        if (fabs(vr) < 1000) {
            ratio = (c + vr) / (c - vr);
        }

        return baseWavelength * ratio;
    }

    void update(float deltaTime) override {
        // Update positions, velocities if needed
        // In a real system, these would be updated by physics
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_DOPPLER_EFFECT_H
