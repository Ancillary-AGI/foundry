/**
 * @file AdvancedPhysicsSystem.cpp
 * @brief Implementation of advanced physics system with fluid, cloth, and soft body simulation
 */

#include "GameEngine/physics/AdvancedPhysicsSystem.h"
#include <algorithm>
#include <thread>
#include <execution>

namespace FoundryEngine {

class AdvancedPhysicsSystem::AdvancedPhysicsSystemImpl {
public:
    PhysicsConfig config_;
    
    // Physics world
    std::unique_ptr<PhysicsWorld> world_;
    
    // Simulation systems
    std::unique_ptr<FluidSimulation> fluidSim_;
    std::unique_ptr<ClothSimulation> clothSim_;
    std::unique_ptr<SoftBodySimulation> softBodySim_;
    std::unique_ptr<DestructionSystem> destructionSys_;
    
    // Rigid bodies
    std::vector<std::unique_ptr<RigidBody>> rigidBodies_;
    std::unordered_map<uint32_t, size_t> entityToRigidBody_;
    
    // Constraints
    std::vector<std::unique_ptr<Constraint>> constraints_;
    
    // Performance metrics
    PhysicsStats stats_;
    
    // Threading
    std::vector<std::thread> physicsThreads_;
    std::atomic<bool> isSimulating_{false};
    
    // Collision detection
    std::unique_ptr<BroadPhase> broadPhase_;
    std::unique_ptr<NarrowPhase> narrowPhase_;
    std::vector<ContactPoint> contacts_;
    
    // Spatial partitioning
    std::unique_ptr<SpatialHash> spatialHash_;
};

AdvancedPhysicsSystem::AdvancedPhysicsSystem() 
    : impl_(std::make_unique<AdvancedPhysicsSystemImpl>()) {}

AdvancedPhysicsSystem::~AdvancedPhysicsSystem() = default;

bool AdvancedPhysicsSystem::initialize(const PhysicsConfig& config) {
    impl_->config_ = config;
    
    // Initialize physics world
    impl_->world_ = std::make_unique<PhysicsWorld>();
    impl_->world_->setGravity(config.gravity);
    impl_->world_->setTimeStep(config.timeStep);
    
    // Initialize collision detection
    impl_->broadPhase_ = std::make_unique<BroadPhase>();
    impl_->narrowPhase_ = std::make_unique<NarrowPhase>();
    
    // Initialize spatial partitioning
    impl_->spatialHash_ = std::make_unique<SpatialHash>(config.spatialHashCellSize);
    
    // Initialize fluid simulation
    if (config.enableFluidSimulation) {
        FluidConfig fluidConfig;
        fluidConfig.maxParticles = config.maxFluidParticles;
        fluidConfig.particleRadius = config.fluidParticleRadius;
        fluidConfig.restDensity = config.fluidRestDensity;
        fluidConfig.viscosity = config.fluidViscosity;
        
        impl_->fluidSim_ = std::make_unique<FluidSimulation>();
        impl_->fluidSim_->initialize(fluidConfig);
    }
    
    // Initialize cloth simulation
    if (config.enableClothSimulation) {
        ClothConfig clothConfig;
        clothConfig.stiffness = config.clothStiffness;
        clothConfig.damping = config.clothDamping;
        clothConfig.enableSelfCollision = config.enableClothSelfCollision;
        
        impl_->clothSim_ = std::make_unique<ClothSimulation>();
        impl_->clothSim_->initialize(clothConfig);
    }
    
    // Initialize soft body simulation
    if (config.enableSoftBodySimulation) {
        SoftBodyConfig softBodyConfig;
        softBodyConfig.stiffness = config.softBodyStiffness;
        softBodyConfig.damping = config.softBodyDamping;
        
        impl_->softBodySim_ = std::make_unique<SoftBodySimulation>();
        impl_->softBodySim_->initialize(softBodyConfig);
    }
    
    // Initialize destruction system
    if (config.enableDestruction) {
        impl_->destructionSys_ = std::make_unique<DestructionSystem>();
        impl_->destructionSys_->initialize();
    }
    
    // Start physics threads
    if (config.enableMultithreading) {
        startPhysicsThreads();
    }
    
    return true;
}

void AdvancedPhysicsSystem::shutdown() {
    impl_->isSimulating_ = false;
    
    // Wait for physics threads to finish
    for (auto& thread : impl_->physicsThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Cleanup resources
    impl_->rigidBodies_.clear();
    impl_->constraints_.clear();
    impl_->contacts_.clear();
    
    if (impl_->fluidSim_) {
        impl_->fluidSim_->shutdown();
    }
    
    if (impl_->clothSim_) {
        impl_->clothSim_->shutdown();
    }
    
    if (impl_->softBodySim_) {
        impl_->softBodySim_->shutdown();
    }
    
    if (impl_->destructionSys_) {
        impl_->destructionSys_->shutdown();
    }
}

void AdvancedPhysicsSystem::update(float deltaTime) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Fixed timestep simulation
    static float accumulator = 0.0f;
    accumulator += deltaTime;
    
    while (accumulator >= impl_->config_.timeStep) {
        step(impl_->config_.timeStep);
        accumulator -= impl_->config_.timeStep;
        impl_->stats_.stepsPerformed++;
    }
    
    // Update performance metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    impl_->stats_.updateTime = duration.count() / 1000.0f; // Convert to milliseconds
}

void AdvancedPhysicsSystem::step(float timeStep) {
    // Broad phase collision detection
    updateBroadPhase();
    
    // Narrow phase collision detection
    updateNarrowPhase();
    
    // Integrate forces and velocities
    integrateRigidBodies(timeStep);
    
    // Update fluid simulation
    if (impl_->fluidSim_) {
        impl_->fluidSim_->update(timeStep);
    }
    
    // Update cloth simulation
    if (impl_->clothSim_) {
        impl_->clothSim_->update(timeStep);
    }
    
    // Update soft body simulation
    if (impl_->softBodySim_) {
        impl_->softBodySim_->update(timeStep);
    }
    
    // Solve constraints
    solveConstraints(timeStep);
    
    // Update positions
    updatePositions(timeStep);
    
    // Handle destruction
    if (impl_->destructionSys_) {
        impl_->destructionSys_->update(timeStep);
    }
}

uint32_t AdvancedPhysicsSystem::createRigidBody(const RigidBodyDesc& desc) {
    auto rigidBody = std::make_unique<RigidBody>();
    
    rigidBody->position = desc.position;
    rigidBody->rotation = desc.rotation;
    rigidBody->linearVelocity = desc.linearVelocity;
    rigidBody->angularVelocity = desc.angularVelocity;
    rigidBody->mass = desc.mass;
    rigidBody->friction = desc.friction;
    rigidBody->restitution = desc.restitution;
    rigidBody->isStatic = (desc.mass == 0.0f);
    
    // Create collision shape
    rigidBody->shape = createCollisionShape(desc.shapeType, desc.shapeParams);
    
    // Calculate inertia tensor
    if (!rigidBody->isStatic) {
        rigidBody->invMass = 1.0f / desc.mass;
        rigidBody->inertiaTensor = calculateInertiaTensor(rigidBody->shape.get(), desc.mass);
        rigidBody->invInertiaTensor = rigidBody->inertiaTensor.inverse();
    } else {
        rigidBody->invMass = 0.0f;
        rigidBody->inertiaTensor = Matrix3::zero();
        rigidBody->invInertiaTensor = Matrix3::zero();
    }
    
    uint32_t id = static_cast<uint32_t>(impl_->rigidBodies_.size());
    impl_->rigidBodies_.push_back(std::move(rigidBody));
    
    impl_->stats_.rigidBodiesCreated++;
    return id;
}

void AdvancedPhysicsSystem::destroyRigidBody(uint32_t rigidBodyId) {
    if (rigidBodyId < impl_->rigidBodies_.size()) {
        impl_->rigidBodies_[rigidBodyId].reset();
        impl_->stats_.rigidBodiesDestroyed++;
    }
}

RigidBody* AdvancedPhysicsSystem::getRigidBody(uint32_t rigidBodyId) const {
    if (rigidBodyId < impl_->rigidBodies_.size()) {
        return impl_->rigidBodies_[rigidBodyId].get();
    }
    return nullptr;
}

void AdvancedPhysicsSystem::setGravity(const Vector3& gravity) {
    impl_->world_->setGravity(gravity);
}

Vector3 AdvancedPhysicsSystem::getGravity() const {
    return impl_->world_->getGravity();
}

void AdvancedPhysicsSystem::applyForce(uint32_t rigidBodyId, const Vector3& force, const Vector3& point) {
    auto* rigidBody = getRigidBody(rigidBodyId);
    if (rigidBody && !rigidBody->isStatic) {
        rigidBody->force += force;
        
        // Apply torque if point is not at center of mass
        Vector3 r = point - rigidBody->position;
        rigidBody->torque += Vector3::cross(r, force);
    }
}

void AdvancedPhysicsSystem::applyImpulse(uint32_t rigidBodyId, const Vector3& impulse, const Vector3& point) {
    auto* rigidBody = getRigidBody(rigidBodyId);
    if (rigidBody && !rigidBody->isStatic) {
        rigidBody->linearVelocity += impulse * rigidBody->invMass;
        
        // Apply angular impulse if point is not at center of mass
        Vector3 r = point - rigidBody->position;
        Vector3 angularImpulse = Vector3::cross(r, impulse);
        rigidBody->angularVelocity += rigidBody->invInertiaTensor * angularImpulse;
    }
}

uint32_t AdvancedPhysicsSystem::createFluidEmitter(const FluidEmitterDesc& desc) {
    if (!impl_->fluidSim_) {
        return INVALID_FLUID_EMITTER_ID;
    }
    
    return impl_->fluidSim_->createEmitter(desc);
}

uint32_t AdvancedPhysicsSystem::createCloth(const ClothDesc& desc) {
    if (!impl_->clothSim_) {
        return INVALID_CLOTH_ID;
    }
    
    return impl_->clothSim_->createCloth(desc);
}

uint32_t AdvancedPhysicsSystem::createSoftBody(const SoftBodyDesc& desc) {
    if (!impl_->softBodySim_) {
        return INVALID_SOFT_BODY_ID;
    }
    
    return impl_->softBodySim_->createSoftBody(desc);
}

bool AdvancedPhysicsSystem::raycast(const Vector3& origin, const Vector3& direction, float maxDistance, RaycastHit& hit) const {
    // Perform raycast against all rigid bodies
    float closestDistance = maxDistance;
    bool hasHit = false;
    
    for (const auto& rigidBody : impl_->rigidBodies_) {
        if (!rigidBody) continue;
        
        RaycastHit tempHit;
        if (raycastAgainstShape(origin, direction, rigidBody->shape.get(), 
                               rigidBody->position, rigidBody->rotation, tempHit)) {
            if (tempHit.distance < closestDistance) {
                closestDistance = tempHit.distance;
                hit = tempHit;
                hit.rigidBodyId = static_cast<uint32_t>(
                    std::distance(impl_->rigidBodies_.begin(), 
                                 std::find_if(impl_->rigidBodies_.begin(), impl_->rigidBodies_.end(),
                                            [&](const auto& rb) { return rb.get() == rigidBody.get(); })));
                hasHit = true;
            }
        }
    }
    
    return hasHit;
}

std::vector<uint32_t> AdvancedPhysicsSystem::overlapSphere(const Vector3& center, float radius) const {
    std::vector<uint32_t> overlapping;
    
    for (size_t i = 0; i < impl_->rigidBodies_.size(); ++i) {
        const auto& rigidBody = impl_->rigidBodies_[i];
        if (!rigidBody) continue;
        
        if (sphereOverlapsShape(center, radius, rigidBody->shape.get(), 
                               rigidBody->position, rigidBody->rotation)) {
            overlapping.push_back(static_cast<uint32_t>(i));
        }
    }
    
    return overlapping;
}

AdvancedPhysicsSystem::PhysicsStats AdvancedPhysicsSystem::getPhysicsStats() const {
    return impl_->stats_;
}

void AdvancedPhysicsSystem::resetStats() {
    impl_->stats_ = PhysicsStats{};
}

void AdvancedPhysicsSystem::updateBroadPhase() {
    // Update spatial hash with all rigid bodies
    impl_->spatialHash_->clear();
    
    for (size_t i = 0; i < impl_->rigidBodies_.size(); ++i) {
        const auto& rigidBody = impl_->rigidBodies_[i];
        if (!rigidBody) continue;
        
        // Calculate AABB
        AABB aabb = calculateAABB(rigidBody->shape.get(), rigidBody->position, rigidBody->rotation);
        impl_->spatialHash_->insert(static_cast<uint32_t>(i), aabb);
    }
    
    // Generate potential collision pairs
    auto pairs = impl_->spatialHash_->getPotentialPairs();
    impl_->stats_.broadPhasePairs = pairs.size();
}

void AdvancedPhysicsSystem::updateNarrowPhase() {
    impl_->contacts_.clear();
    
    // Get potential pairs from broad phase
    auto pairs = impl_->spatialHash_->getPotentialPairs();
    
    // Perform narrow phase collision detection
    for (const auto& pair : pairs) {
        auto* bodyA = getRigidBody(pair.first);
        auto* bodyB = getRigidBody(pair.second);
        
        if (!bodyA || !bodyB) continue;
        if (bodyA->isStatic && bodyB->isStatic) continue;
        
        // Check for collision
        ContactManifold manifold;
        if (checkCollision(bodyA, bodyB, manifold)) {
            for (const auto& contact : manifold.contacts) {
                impl_->contacts_.push_back(contact);
            }
        }
    }
    
    impl_->stats_.narrowPhaseTests = pairs.size();
    impl_->stats_.contactsGenerated = impl_->contacts_.size();
}

void AdvancedPhysicsSystem::integrateRigidBodies(float timeStep) {
    // Integrate forces and velocities for all rigid bodies
    std::for_each(std::execution::par_unseq, impl_->rigidBodies_.begin(), impl_->rigidBodies_.end(),
        [this, timeStep](const auto& rigidBody) {
            if (!rigidBody || rigidBody->isStatic) return;
            
            // Apply gravity
            Vector3 gravity = impl_->world_->getGravity();
            rigidBody->force += gravity * rigidBody->mass;
            
            // Integrate linear motion
            Vector3 acceleration = rigidBody->force * rigidBody->invMass;
            rigidBody->linearVelocity += acceleration * timeStep;
            
            // Apply linear damping
            rigidBody->linearVelocity *= (1.0f - impl_->config_.linearDamping * timeStep);
            
            // Integrate angular motion
            Vector3 angularAcceleration = rigidBody->invInertiaTensor * rigidBody->torque;
            rigidBody->angularVelocity += angularAcceleration * timeStep;
            
            // Apply angular damping
            rigidBody->angularVelocity *= (1.0f - impl_->config_.angularDamping * timeStep);
            
            // Clear forces
            rigidBody->force = Vector3::zero();
            rigidBody->torque = Vector3::zero();
        });
}

void AdvancedPhysicsSystem::solveConstraints(float timeStep) {
    // Solve contact constraints using iterative method
    const int iterations = impl_->config_.solverIterations;
    
    for (int iter = 0; iter < iterations; ++iter) {
        // Solve contact constraints
        for (auto& contact : impl_->contacts_) {
            solveContactConstraint(contact, timeStep);
        }
        
        // Solve user-defined constraints
        for (auto& constraint : impl_->constraints_) {
            if (constraint) {
                constraint->solve(timeStep);
            }
        }
    }
}

void AdvancedPhysicsSystem::updatePositions(float timeStep) {
    // Update positions based on velocities
    std::for_each(std::execution::par_unseq, impl_->rigidBodies_.begin(), impl_->rigidBodies_.end(),
        [timeStep](const auto& rigidBody) {
            if (!rigidBody || rigidBody->isStatic) return;
            
            // Update position
            rigidBody->position += rigidBody->linearVelocity * timeStep;
            
            // Update rotation using quaternion integration
            Quaternion angularVelQuat(rigidBody->angularVelocity.x, rigidBody->angularVelocity.y, 
                                     rigidBody->angularVelocity.z, 0.0f);
            Quaternion deltaRotation = angularVelQuat * rigidBody->rotation * (timeStep * 0.5f);
            rigidBody->rotation += deltaRotation;
            rigidBody->rotation.normalize();
        });
}

void AdvancedPhysicsSystem::startPhysicsThreads() {
    impl_->isSimulating_ = true;
    
    // Create worker threads for parallel physics processing
    size_t numThreads = std::thread::hardware_concurrency();
    impl_->physicsThreads_.reserve(numThreads);
    
    for (size_t i = 0; i < numThreads; ++i) {
        impl_->physicsThreads_.emplace_back([this]() {
            physicsWorkerThread();
        });
    }
}

void AdvancedPhysicsSystem::physicsWorkerThread() {
    while (impl_->isSimulating_) {
        // Worker thread processes physics tasks
        // This would be used for parallel collision detection, constraint solving, etc.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

std::unique_ptr<CollisionShape> AdvancedPhysicsSystem::createCollisionShape(ShapeType type, const ShapeParams& params) {
    switch (type) {
        case ShapeType::Sphere:
            return std::make_unique<SphereShape>(params.radius);
        case ShapeType::Box:
            return std::make_unique<BoxShape>(params.dimensions);
        case ShapeType::Capsule:
            return std::make_unique<CapsuleShape>(params.radius, params.height);
        case ShapeType::Cylinder:
            return std::make_unique<CylinderShape>(params.radius, params.height);
        case ShapeType::Mesh:
            return std::make_unique<MeshShape>(params.vertices, params.indices);
        case ShapeType::Heightfield:
            return std::make_unique<HeightfieldShape>(params.heightData, params.width, params.depth, params.scale);
        default:
            return std::make_unique<SphereShape>(1.0f);
    }
}

Matrix3 AdvancedPhysicsSystem::calculateInertiaTensor(const CollisionShape* shape, float mass) {
    // Calculate inertia tensor based on shape type
    switch (shape->getType()) {
        case ShapeType::Sphere: {
            float radius = static_cast<const SphereShape*>(shape)->getRadius();
            float inertia = 0.4f * mass * radius * radius;
            return Matrix3::diagonal(inertia, inertia, inertia);
        }
        case ShapeType::Box: {
            Vector3 dimensions = static_cast<const BoxShape*>(shape)->getDimensions();
            float ix = (1.0f / 12.0f) * mass * (dimensions.y * dimensions.y + dimensions.z * dimensions.z);
            float iy = (1.0f / 12.0f) * mass * (dimensions.x * dimensions.x + dimensions.z * dimensions.z);
            float iz = (1.0f / 12.0f) * mass * (dimensions.x * dimensions.x + dimensions.y * dimensions.y);
            return Matrix3::diagonal(ix, iy, iz);
        }
        default:
            // Default to sphere inertia
            float inertia = 0.4f * mass;
            return Matrix3::diagonal(inertia, inertia, inertia);
    }
}

} // namespace FoundryEngine