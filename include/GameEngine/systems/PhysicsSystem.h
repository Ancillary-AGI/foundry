#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "../math/Vector3.h"
#include "../math/Quaternion.h"

namespace FoundryEngine {

class RigidBody;
class Collider;
class Joint;
class CharacterController;

enum class CollisionLayer : uint32_t {
    Default = 1 << 0,
    Static = 1 << 1,
    Dynamic = 1 << 2,
    Kinematic = 1 << 3,
    Trigger = 1 << 4,
    Character = 1 << 5,
    Projectile = 1 << 6,
    UI = 1 << 7
};

struct RaycastHit {
    Vector3 point;
    Vector3 normal;
    float distance;
    RigidBody* rigidbody = nullptr;
    Collider* collider = nullptr;
};

struct CollisionInfo {
    RigidBody* bodyA = nullptr;
    RigidBody* bodyB = nullptr;
    Vector3 contactPoint;
    Vector3 contactNormal;
    float penetrationDepth;
    float impulse;
};

class PhysicsWorld {
public:
    virtual ~PhysicsWorld() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void step(float deltaTime) = 0;
    
    virtual void setGravity(const Vector3& gravity) = 0;
    virtual Vector3 getGravity() const = 0;
    virtual void setTimeScale(float scale) = 0;
    virtual float getTimeScale() const = 0;
    
    virtual RigidBody* createRigidBody() = 0;
    virtual void destroyRigidBody(RigidBody* body) = 0;
    
    virtual Collider* createBoxCollider(const Vector3& size) = 0;
    virtual Collider* createSphereCollider(float radius) = 0;
    virtual Collider* createCapsuleCollider(float radius, float height) = 0;
    virtual Collider* createMeshCollider(const std::vector<Vector3>& vertices, const std::vector<uint32_t>& indices, bool convex = false) = 0;
    virtual void destroyCollider(Collider* collider) = 0;
    
    virtual Joint* createFixedJoint(RigidBody* bodyA, RigidBody* bodyB) = 0;
    virtual Joint* createHingeJoint(RigidBody* bodyA, RigidBody* bodyB, const Vector3& anchor, const Vector3& axis) = 0;
    virtual Joint* createSpringJoint(RigidBody* bodyA, RigidBody* bodyB, float stiffness, float damping) = 0;
    virtual void destroyJoint(Joint* joint) = 0;
    
    virtual CharacterController* createCharacterController(float radius, float height) = 0;
    virtual void destroyCharacterController(CharacterController* controller) = 0;
    
    virtual bool raycast(const Vector3& origin, const Vector3& direction, float maxDistance, RaycastHit& hit, uint32_t layerMask = 0xFFFFFFFF) = 0;
    virtual std::vector<RaycastHit> raycastAll(const Vector3& origin, const Vector3& direction, float maxDistance, uint32_t layerMask = 0xFFFFFFFF) = 0;
    
    virtual std::vector<Collider*> overlapSphere(const Vector3& center, float radius, uint32_t layerMask = 0xFFFFFFFF) = 0;
    virtual std::vector<Collider*> overlapBox(const Vector3& center, const Vector3& size, const Quaternion& rotation, uint32_t layerMask = 0xFFFFFFFF) = 0;
    
    virtual void setCollisionCallback(std::function<void(const CollisionInfo&)> callback) = 0;
    virtual void setTriggerCallback(std::function<void(Collider*, Collider*, bool)> callback) = 0;
    
    virtual void setDebugDrawEnabled(bool enabled) = 0;
    virtual bool isDebugDrawEnabled() const = 0;
    
    virtual void setMaxSubSteps(int maxSubSteps) = 0;
    virtual void setFixedTimeStep(float timeStep) = 0;
};

class BulletPhysicsWorld : public PhysicsWorld {
public:
    bool initialize() override;
    void shutdown() override;
    void step(float deltaTime) override;
    void setGravity(const Vector3& gravity) override;
    Vector3 getGravity() const override;
    void setTimeScale(float scale) override;
    float getTimeScale() const override;
    RigidBody* createRigidBody() override;
    void destroyRigidBody(RigidBody* body) override;
    Collider* createBoxCollider(const Vector3& size) override;
    Collider* createSphereCollider(float radius) override;
    Collider* createCapsuleCollider(float radius, float height) override;
    Collider* createMeshCollider(const std::vector<Vector3>& vertices, const std::vector<uint32_t>& indices, bool convex) override;
    void destroyCollider(Collider* collider) override;
    Joint* createFixedJoint(RigidBody* bodyA, RigidBody* bodyB) override;
    Joint* createHingeJoint(RigidBody* bodyA, RigidBody* bodyB, const Vector3& anchor, const Vector3& axis) override;
    Joint* createSpringJoint(RigidBody* bodyA, RigidBody* bodyB, float stiffness, float damping) override;
    void destroyJoint(Joint* joint) override;
    CharacterController* createCharacterController(float radius, float height) override;
    void destroyCharacterController(CharacterController* controller) override;
    bool raycast(const Vector3& origin, const Vector3& direction, float maxDistance, RaycastHit& hit, uint32_t layerMask) override;
    std::vector<RaycastHit> raycastAll(const Vector3& origin, const Vector3& direction, float maxDistance, uint32_t layerMask) override;
    std::vector<Collider*> overlapSphere(const Vector3& center, float radius, uint32_t layerMask) override;
    std::vector<Collider*> overlapBox(const Vector3& center, const Vector3& size, const Quaternion& rotation, uint32_t layerMask) override;
    void setCollisionCallback(std::function<void(const CollisionInfo&)> callback) override;
    void setTriggerCallback(std::function<void(Collider*, Collider*, bool)> callback) override;
    void setDebugDrawEnabled(bool enabled) override;
    bool isDebugDrawEnabled() const override;
    void setMaxSubSteps(int maxSubSteps) override;
    void setFixedTimeStep(float timeStep) override;
};

} // namespace FoundryEngine
