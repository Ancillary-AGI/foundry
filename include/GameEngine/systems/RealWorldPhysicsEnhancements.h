#ifndef FOUNDRY_GAMEENGINE_REAL_WORLD_PHYSICS_ENHANCEMENTS_H
#define FOUNDRY_GAMEENGINE_REAL_WORLD_PHYSICS_ENHANCEMENTS_H

#include <vector>
#include "../../core/System.h"
#include "../../math/Vector3.h"

namespace FoundryEngine {

class RealWorldPhysicsEnhancements : public System {
public:
    // Enhanced gravity with variations
    static Vector3 gravitationalForce(const Vector3& position, float mass) {
        // Earth's gravity, but can be enhanced for celestial bodies
        float G = 9.81f; // m/s² near surface
        return Vector3(0, -G * mass, 0); // Simplified vertical gravity
    }

    // Air resistance/drag
    static Vector3 dragForce(const Vector3& velocity, float dragCoefficient = 0.01f, float density = 1.225f, float area = 1.0f) {
        Vector3 drag = -dragCoefficient * density * area * velocity.magnitudeSq() * velocity.normalized() * 0.5f;
        return drag;
    }

    // Friction
    static Vector3 frictionForce(const Vector3& velocity, float coefficient = 0.5f, Vector3 normal = Vector3(0,1,0)) {
        Vector3 velTangent = velocity - velocity.dot(normal) * normal;
        if (velTangent.magnitudeSq() > 0) {
            return -velTangent.normalized() * coefficient * fabs(velocity.dot(normal));
        }
        return Vector3(0,0,0);
    }

    // Elastic collisions
    static void resolveCollision(float& pos1, float& vel1, float mass1, float& pos2, float& vel2, float mass2, float restitution = 0.8f) {
        // 1D approximation, can generalize to 3D
        float totalMass = mass1 + mass2;
        float vel1New = ((mass1 - mass2) * vel1 + 2 * mass2 * vel2) / totalMass;
        float vel2New = (2 * mass1 * vel1 + (mass2 - mass1) * vel2) / totalMass;

        vel1 = vel1New * restitution;
        vel2 = vel2New * restitution;

        // Separate overlapping objects (crude)
        float overlap = (pos1 + 1.0f) - pos2; // Assuming radius 1
        if (overlap > 0) {
            pos1 -= overlap / 2;
            pos2 += overlap / 2;
        }
    }

    // Soft body damping
    static Vector3 dampingForce(const Vector3& velocity, float dampingCoefficient = 0.99f) {
        return velocity * (-dampingCoefficient);
    }

    // Coriolis effect for rotating frames (e.g., Earth's rotation)
    static Vector3 coriolisForce(const Vector3& velocity, float omega = 7.292e-5f, Vector3 axis = Vector3(0,1,0)) {
        // ω × v
        return axis.cross(velocity) * omega;
    }

    void update(float deltaTime) override {
        // Physics enhancements can be applied globally or per entity
        // This is a utility class, so no specific update loop
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_REAL_WORLD_PHYSICS_ENHANCEMENTS_H
