/**
 * @file PhysicsSystem.h
 * @brief Physics simulation system
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include <memory>
#include <vector>

namespace FoundryEngine {

// Forward declarations
class RigidBody;
class Collider;
class Constraint;

/**
 * @class PhysicsWorld
 * @brief Physics simulation world manager
 */
class PhysicsWorld : public System {
public:
    PhysicsWorld() = default;
    virtual ~PhysicsWorld() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;

    virtual void step(float deltaTime) = 0;

    // Gravity and global settings
    virtual void setGravity(const Vector3& gravity) = 0;
    virtual Vector3 getGravity() const = 0;

    // Rigid body management
    virtual std::shared_ptr<RigidBody> createRigidBody() = 0;
    virtual void destroyRigidBody(std::shared_ptr<RigidBody> body) = 0;

    // Collision detection
    virtual void raycast(const Vector3& origin, const Vector3& direction,
                        float maxDistance) = 0;

    // Constraints and joints
    virtual std::shared_ptr<Constraint> createConstraint() = 0;
    virtual void destroyConstraint(std::shared_ptr<Constraint> constraint) = 0;
};

/**
 * @class BulletPhysicsWorld
 * @brief Bullet Physics implementation
 */
class BulletPhysicsWorld : public PhysicsWorld {
public:
    bool initialize() override { return true; }
    void shutdown() override {}
    void update(float deltaTime) override {}
    void step(float deltaTime) override {}

    void setGravity(const Vector3& gravity) override {}
    Vector3 getGravity() const override { return Vector3(0, -9.81f, 0); }

    std::shared_ptr<RigidBody> createRigidBody() override { return nullptr; }
    void destroyRigidBody(std::shared_ptr<RigidBody> body) override {}

    void raycast(const Vector3& origin, const Vector3& direction, float maxDistance) override {}

    std::shared_ptr<Constraint> createConstraint() override { return nullptr; }
    void destroyConstraint(std::shared_ptr<Constraint> constraint) override {}
};

} // namespace FoundryEngine
