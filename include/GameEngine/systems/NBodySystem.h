#ifndef NEUTRAL_GAMEENGINE_NBODY_SYSTEM_H
#define NEUTRAL_GAMEENGINE_NBODY_SYSTEM_H

#include <vector>
#include <cmath>
#include <algorithm>
#include "../../core/System.h"
#include "../../math/Vector3.h"

namespace FoundryEngine {

class NBodySystem : public System {
public:
    struct Body {
        Vector3 position;
        Vector3 velocity;
        float mass;
        bool fixed = false; // Fixed bodies don't move

        Body(Vector3 pos, Vector3 vel, float m) : position(pos), velocity(vel), mass(m) {}
    };

    std::vector<Body> bodies_;
    float gravitationalConstant_ = 6.67430e-11f;
    float timeStep_ = 0.001f; // Smaller for stability
    int integrationSteps_ = 10; // Sub-steps for accuracy

    // Add a body to the simulation
    void addBody(const Vector3& position, const Vector3& velocity, float mass) {
        bodies_.emplace_back(position, velocity, mass);
    }

    // Remove a body by index
    void removeBody(int index) {
        if (index >= 0 && index < bodies_.size()) {
            bodies_.erase(bodies_.begin() + index);
        }
    }

    // Update the simulation
    void update(float deltaTime) override {
        // Perform multiple integration steps
        for (int step = 0; step < integrationSteps_; ++step) {
            updateBodies(timeStep_);
        }
    }

private:
    // Compute forces and integrate one time step
    void updateBodies(float dt) {
        size_t n = bodies_.size();
        std::vector<Vector3> accelerations(n, Vector3(0, 0, 0));

        // Calculate accelerations due to gravitational forces (O(n^2) brute force)
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                if (i != j) {
                    Vector3 r = bodies_[j].position - bodies_[i].position;
                    float distanceSq = r.magnitudeSq();
                    if (distanceSq > 0) {
                        float force = gravitationalConstant_ * bodies_[i].mass * bodies_[j].mass / distanceSq;
                        float distance = std::sqrt(distanceSq);
                        Vector3 direction = r / distance; // Normalized
                        accelerations[i] += direction * force / bodies_[i].mass;
                    }
                }
            }
        }

        // Integrate velocities and positions (symplectic Euler)
        for (size_t i = 0; i < n; ++i) {
            if (!bodies_[i].fixed) {
                bodies_[i].velocity += accelerations[i] * dt;
                bodies_[i].position += bodies_[i].velocity * dt;
            }
        }
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_NBODY_SYSTEM_H
