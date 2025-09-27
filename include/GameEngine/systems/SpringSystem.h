#ifndef NEUTRAL_GAMEENGINE_SPRING_SYSTEM_H
#define NEUTRAL_GAMEENGINE_SPRING_SYSTEM_H

#include <vector>
#include <unordered_map>
#include "../../core/System.h"
#include "../../math/Vector3.h"

namespace FoundryEngine {

class SpringSystem : public System {
public:
    enum class SpringType {
        LINEAR,
        NONLINEAR,
        DAMPED,
        TORQUE,
        GEAR
    };

    struct Spring {
        int object1, object2;      // Indices of connected objects
        Vector3 attachmentPoint1;  // Local attachment point on object1
        Vector3 attachmentPoint2;  // Local attachment point on object2
        float restLength;
        float stiffness = 100.0f;
        float damping = 1.0f;
        SpringType type = SpringType::LINEAR;
        bool active = true;

        // Nonlinear spring parameters
        float exponent = 2.0f;
        float maxForce = 1000.0f;

        // Torque spring parameters
        Vector3 axis1, axis2;      // Rotation axes
        float torqueStiffness = 50.0f;

        Spring(int obj1, int obj2, const Vector3& attach1, const Vector3& attach2, float length)
            : object1(obj1), object2(obj2), attachmentPoint1(attach1), attachmentPoint2(attach2),
              restLength(length) {}
    };

    struct PhysicalObject {
        Vector3 position;
        Vector3 velocity;
        Vector3 acceleration;
        Vector3 angularVelocity;
        Quaternion orientation;
        float mass;
        float momentOfInertia = 1.0f;
        bool isStatic = false;

        PhysicalObject(const Vector3& pos, float m) : position(pos), mass(m) {}
    };

    std::vector<PhysicalObject> objects_;
    std::vector<Spring> springs_;
    Vector3 gravity_ = Vector3(0, -9.81f, 0);
    float timeStep_ = 0.016f;
    float timeAccumulator_ = 0.0f;

    // Add object
    int addObject(const Vector3& position, float mass, bool isStatic = false) {
        objects_.emplace_back(position, mass);
        objects_.back().isStatic = isStatic;
        return objects_.size() - 1;
    }

    // Add spring
    void addSpring(int obj1, int obj2, const Vector3& attach1, const Vector3& attach2,
                   float length, SpringType type = SpringType::LINEAR) {
        springs_.emplace_back(obj1, obj2, attach1, attach2, length);
        springs_.back().type = type;
    }

    // Force computation
    void computeForces(float dt) {
        // Reset accelerations
        for (auto& obj : objects_) {
            if (obj.isStatic) continue;
            obj.acceleration = gravity_;
        }

        // Spring forces
        for (const auto& spring : springs_) {
            if (!spring.active) continue;

            PhysicalObject& obj1 = objects_[spring.object1];
            PhysicalObject& obj2 = objects_[spring.object2];

            // World space attachment points
            Vector3 worldAttach1 = obj1.position + spring.attachmentPoint1;
            Vector3 worldAttach2 = obj2.position + spring.attachmentPoint2;

            Vector3 delta = worldAttach2 - worldAttach1;
            float currentLength = delta.magnitude();

            if (currentLength <= 0) continue;

            Vector3 direction = delta / currentLength;
            float displacement = currentLength - spring.restLength;

            // Compute spring force
            Vector3 force(0,0,0);
            switch (spring.type) {
                case SpringType::LINEAR: {
                    float springForce = -spring.stiffness * displacement;
                    force = direction * springForce;
                    break;
                }
                case SpringType::NONLINEAR: {
                    float springForce = -spring.stiffness * std::pow(displacement, spring.exponent);
                    springForce = std::max(-spring.maxForce, std::min(springForce, spring.maxForce));
                    force = direction * springForce;
                    break;
                }
                case SpringType::DAMPED: {
                    Vector3 relativeVel = obj2.velocity - obj1.velocity;
                    float dampForce = spring.damping * direction.dot(relativeVel);
                    float springForce = -spring.stiffness * displacement;
                    force = direction * (springForce - dampForce);
                    break;
                }
            }

            // Apply forces
            if (!obj1.isStatic) {
                obj1.acceleration += force / obj1.mass;

                // Torque if attachment point not at center
                Vector3 torque = spring.attachmentPoint1.cross(force);
                obj1.angularVelocity += torque / obj1.momentOfInertia * dt;
            }

            if (!obj2.isStatic) {
                obj2.acceleration -= force / obj2.mass;

                // Torque
                Vector3 torque = spring.attachmentPoint2.cross(-force);
                obj2.angularVelocity += torque / obj2.momentOfInertia * dt;
            }
        }
    }

    // Integration
    void integrate(float dt) {
        for (auto& obj : objects_) {
            if (obj.isStatic) continue;

            // Linear motion
            obj.velocity += obj.acceleration * dt;
            obj.position += obj.velocity * dt;

            // Angular motion
            Quaternion angularQuat(0, obj.angularVelocity.x, obj.angularVelocity.y, obj.angularVelocity.z);
            angularQuat = angularQuat.mul(obj.orientation).mul(0.5f * dt);
            obj.orientation = (obj.orientation + angularQuat).normalized();

            // Update orientation matrix (simplified)
            // In real implementation, would update full transformation
        }
    }

    void update(float deltaTime) override {
        timeAccumulator_ += deltaTime;

        while (timeAccumulator_ >= timeStep_) {
            computeForces(timeStep_);
            integrate(timeStep_);
            timeAccumulator_ -= timeStep_;
        }
    }

    //Constraints
    void addDistanceConstraint(int obj1, int obj2, float distance) {
        // Add spring with infinite stiffness
        addSpring(obj1, obj2, Vector3(0,0,0), Vector3(0,0,0), distance, SpringType::LINEAR);
        springs_.back().stiffness = 100000.0f; // Very stiff for constraint
    }

    // Cable system
    void createCableSystem(const std::vector<Vector3>& positions, float segmentLength) {
        int numSegments = positions.size() - 1;

        // Create objects
        std::vector<int> objectIndices;
        for (size_t i = 0; i < positions.size(); ++i) {
            bool isStatic = (i == 0 || i == positions.size() - 1); // Fix ends
            objectIndices.push_back(addObject(positions[i], 1.0f, isStatic));
        }

        // Connect with springs
        for (int i = 0; i < numSegments; ++i) {
            addSpring(objectIndices[i], objectIndices[i+1],
                     Vector3(0,0,0), Vector3(0,0,0),
                     segmentLength, SpringType::DAMPED);
        }
    }

    // Rope bridge simulation
    void createBridge(const Vector3& start, const Vector3& end, int numSegments,
                      float segmentLength, const Vector3& gravityAnchor) {
        std::vector<Vector3> positions;
        Vector3 direction = (end - start).normalized();
        for (int i = 0; i <= numSegments; ++i) {
            positions.push_back(start + direction * (segmentLength * i));
        }

        createCableSystem(positions, segmentLength);

        // Add gravity anchoring (optional)
        if (gravityAnchor.magnitude() > 0) {
            int centerObj = objects_.size() / 2;
            addSpring(centerObj, addObject(gravityAnchor, 1000.0f, true),
                     Vector3(0,0,0), Vector3(0,0,0),
                     (gravityAnchor - objects_[centerObj].position).magnitude(),
                     SpringType::LINEAR);
            springs_.back().stiffness = 10.0f; // Light gravity simulation
        }
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_SPRING_SYSTEM_H
